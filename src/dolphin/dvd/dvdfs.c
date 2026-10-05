#include "dolphin/dvd.h"
#include "dolphin/os.h"
#include "ctype.h"

struct FSTEntry {
    unsigned long type_and_name_offset;
    unsigned long parent_or_position;
    unsigned long next_or_length;
};

#define ENTRY_IS_DIRECTORY(entry) \
    (((FstStart[(entry)].type_and_name_offset & 0xFF000000) == 0) ? 0 : 1)
#define ENTRY_NAME_OFFSET(entry) \
    (FstStart[(entry)].type_and_name_offset & 0x00FFFFFF)
#define ENTRY_PARENT(entry) (FstStart[(entry)].parent_or_position)
#define ENTRY_NEXT(entry) (FstStart[(entry)].next_or_length)
#define FILE_POSITION(entry) (FstStart[(entry)].parent_or_position)
#define FILE_LENGTH(entry) (FstStart[(entry)].next_or_length)
extern volatile unsigned long __DIRegs[] : 0xCC006000;
#define DI_REGS __DIRegs

static OSBootInfo* BootInfo;
static struct FSTEntry* FstStart;
static char* FstStringStart;
static unsigned long MaxEntryNum;
static unsigned long currentDirectory;

OSThreadQueue __DVDThreadQueue;
unsigned long __DVDLongFileNameFlag;

static void cbForReadAsync(long result, DVDCommandBlock* block);
static void cbForReadSync(long result, DVDCommandBlock* block);

void __DVDFSInit(void)
{
    BootInfo = (OSBootInfo*)0x80000000;
    FstStart = BootInfo->fst_location;
    if (FstStart) {
        MaxEntryNum = FstStart->next_or_length;
        FstStringStart = (char*)FstStart + MaxEntryNum * sizeof(struct FSTEntry);
    }
}

static int isSame(const char* path, const char* name)
{
    while (*name) {
        if (tolower(*path++) != tolower(*name++)) return 0;
    }
    if (*path == '/' || *path == 0) {
        return 1;
    }
    return 0;
}

long DVDConvertPathToEntrynum(const char* path)
{
    const char* component_end;
    char* name;
    int wants_directory;
    unsigned long component_length;
    unsigned long directory;
    unsigned long entry;
    const char* original_path = path;
    const char* extension_start;
    int illegal;
    int extension;

    directory = currentDirectory;

    for (;;) {
        if (*path == 0) {
            return directory;
        } else if (*path == '/') {
            directory = 0;
            path++;
            continue;
        } else if (*path == '.') {
            if (path[1] == '.') {
                if (path[2] == '/') {
                    directory = ENTRY_PARENT(directory);
                    path += 3;
                    continue;
                } else if (path[2] == 0) {
                    return ENTRY_PARENT(directory);
                }
            } else if (path[1] == '/') {
                path += 2;
                continue;
            } else if (path[1] == 0) {
                return directory;
            }
        }

        if (__DVDLongFileNameFlag == 0) {
            extension = 0;
            illegal = 0;
            for (component_end = path; *component_end != 0 && *component_end != '/';
                 component_end++) {
                if (*component_end == '.') {
                    if (component_end - path > 8 || extension == 1) {
                        illegal = 1;
                        break;
                    }
                    extension = 1;
                    extension_start = component_end + 1;
                } else if (*component_end == ' ') {
                    illegal = 1;
                }
            }
            if (extension == 1 && component_end - extension_start > 3) {
                illegal = 1;
            }
            if (illegal) {
                OSPanic("dvdfs.c", 387,
                        "DVDConvertEntrynumToPath(possibly DVDOpen or DVDChangeDir or DVDOpenDir): "
                        "specified directory or file (%s) doesn't match standard 8.3 format. This is a "
                        "temporary restriction and will be removed soon\n", original_path);
            }
        } else {
            for (component_end = path; *component_end != 0 && *component_end != '/';
                 component_end++) {
            }
        }

        wants_directory = (*component_end == 0) ? 0 : 1;
        component_length = component_end - path;
        component_end = path;

        for (entry = directory + 1; entry < ENTRY_NEXT(directory);
             entry = ENTRY_IS_DIRECTORY(entry) ? ENTRY_NEXT(entry) : entry + 1) {
            if (ENTRY_IS_DIRECTORY(entry) == 0 && wants_directory == 1) {
                continue;
            }
            name = FstStringStart + ENTRY_NAME_OFFSET(entry);
            if (isSame(component_end, name) == 1) {
                goto next_level;
            }
        }
        return -1;

    next_level:
        if (!wants_directory) {
            return entry;
        }
        directory = entry;
        path += component_length + 1;
    }
}

int DVDFastOpen(long entry_number, DVDFileInfo* file_info)
{
    if (entry_number < 0 || (unsigned long)entry_number >= MaxEntryNum ||
        ENTRY_IS_DIRECTORY(entry_number)) return 0;
    file_info->start_address = FILE_POSITION(entry_number);
    file_info->length = FILE_LENGTH(entry_number);
    file_info->callback = 0;
    file_info->cb.state = 0;
    return 1;
}

int DVDOpen(const char* file_name, DVDFileInfo* file_info)
{
    long entry;
    char current_directory[128];

    entry = DVDConvertPathToEntrynum(file_name);
    if (entry < 0) {
        DVDGetCurrentDir(current_directory, sizeof(current_directory));
        OSReport("Warning: DVDOpen(): file '%s' was not found under %s.\n",
                 file_name, current_directory);
        return 0;
    }
    if (ENTRY_IS_DIRECTORY(entry)) return 0;
    file_info->start_address = FILE_POSITION(entry);
    file_info->length = FILE_LENGTH(entry);
    file_info->callback = 0;
    file_info->cb.state = 0;
    return 1;
}

int DVDClose(DVDFileInfo* file_info)
{
    DVDCancel(&file_info->cb);
    return 1;
}

static unsigned long copyName(char* destination, const char* source,
                              unsigned long max_length)
{
    unsigned long remaining = max_length;
    while (remaining && *source) {
        *destination++ = *source++;
        remaining--;
    }
    return max_length - remaining;
}

static unsigned long entryToPath(unsigned long entry, char* path,
                                 unsigned long max_length)
{
    unsigned long position;
    const char* name;

    if (!entry) return 0;
    name = FstStringStart + ENTRY_NAME_OFFSET(entry);
    position = entryToPath(ENTRY_PARENT(entry), path, max_length);
    if (position == max_length) return position;
    path[position++] = '/';
    position += copyName(path + position, name, max_length - position);
    return position;
}

static int DVDConvertEntrynumToPath(long entry, char* path,
                                    unsigned long max_length)
{
    unsigned long position = entryToPath(entry, path, max_length);

    if (position == max_length) {
        path[max_length - 1] = 0;
        return 0;
    }
    if (ENTRY_IS_DIRECTORY(entry)) {
        if (position == max_length - 1) {
            path[position] = 0;
            return 0;
        }
        path[position++] = '/';
    }
    path[position] = 0;
    return 1;
}

int DVDGetCurrentDir(char* path, unsigned long max_length)
{
    return DVDConvertEntrynumToPath(currentDirectory, path, max_length);
}

int DVDReadAsyncPrio(DVDFileInfo* file_info, void* address, long length,
                     long offset, DVDCallback callback, long priority)
{
    if (!(0 <= offset && offset <= file_info->length)) {
        OSPanic("dvdfs.c", 750,
                "DVDReadAsync(): specified area is out of the file  ");
    }
    if (!(0 <= offset + length &&
          offset + length < file_info->length + 32)) {
        OSPanic("dvdfs.c", 756,
                "DVDReadAsync(): specified area is out of the file  ");
    }
    file_info->callback = callback;
    DVDReadAbsAsyncPrio(&file_info->cb, address, length,
                        file_info->start_address + offset, cbForReadAsync,
                        priority);
    return 1;
}

static void cbForReadAsync(long result, DVDCommandBlock* block)
{
    DVDFileInfo* file_info = (DVDFileInfo*)block;
    if (file_info->callback) file_info->callback(result, file_info);
}

long DVDReadPrio(DVDFileInfo* file_info, void* address, long length,
                 long offset, long priority)
{
    long result;
    DVDCommandBlock* block;
    long state;
    int enabled;
    long retVal;

    if (!(0 <= offset && offset <= file_info->length)) {
        OSPanic("dvdfs.c", 820,
                "DVDRead(): specified area is out of the file  ");
    }
    if (!(0 <= offset + length &&
          offset + length < file_info->length + 32)) {
        OSPanic("dvdfs.c", 826,
                "DVDRead(): specified area is out of the file  ");
    }
    block = &file_info->cb;
    result = DVDReadAbsAsyncPrio(block, address, length,
                             file_info->start_address + offset,
                             cbForReadSync, priority);
    if (result == 0) return -1;
    enabled = OSDisableInterrupts();
    for (;;) {
        state = ((volatile DVDCommandBlock*)block)->state;
        if (state == 0) {
            retVal = block->transferred_size;
            break;
        }
        if (state == -1) {
            retVal = -1;
            break;
        }
        if (state == 10) {
            retVal = -3;
            break;
        }
        OSSleepThread(&__DVDThreadQueue);
    }
    OSRestoreInterrupts(enabled);
    return retVal;
}

static void cbForReadSync(long result, DVDCommandBlock* block)
{
    OSWakeupThread(&__DVDThreadQueue);
}

long DVDGetTransferredSize(DVDFileInfo* file_info)
{
    long bytes;
    DVDCommandBlock* block = &file_info->cb;

    switch (block->state) {
    case DVD_STATE_COVER_CLOSED:
    case DVD_STATE_NO_DISK:
    case DVD_STATE_COVER_OPEN:
    case DVD_STATE_WRONG_DISK:
    case DVD_STATE_FATAL_ERROR:
    case DVD_STATE_MOTOR_STOPPED:
    case DVD_STATE_CANCELED:
    case DVD_STATE_RETRY:
    case DVD_STATE_END:
        bytes = block->transferred_size;
        break;
    case DVD_STATE_WAITING:
        bytes = 0;
        break;
    case DVD_STATE_BUSY:
        bytes = block->transferred_size +
                (block->current_transfer_size - DI_REGS[6]);
        break;
    }

    return bytes;
}
