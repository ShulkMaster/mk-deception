#include "dolphin/dvd.h"
#include "dolphin/os.h"

typedef struct FSTEntry {
    unsigned long type_and_name_offset;
    unsigned long parent_or_position;
    unsigned long next_or_length;
} FSTEntry;

#define ENTRY_IS_DIRECTORY(entry) \
    ((FstStart[(entry)].type_and_name_offset & 0xFF000000) != 0)
#define ENTRY_NAME_OFFSET(entry) \
    (FstStart[(entry)].type_and_name_offset & 0x00FFFFFF)
#define ENTRY_PARENT(entry) (FstStart[(entry)].parent_or_position)
#define ENTRY_NEXT(entry) (FstStart[(entry)].next_or_length)
#define FILE_POSITION(entry) (FstStart[(entry)].parent_or_position)
#define FILE_LENGTH(entry) (FstStart[(entry)].next_or_length)
#define DI_REGS ((volatile unsigned long*)0xCC006000)

extern int tolower(int character);

static OSBootInfo* BootInfo;
static FSTEntry* FstStart;
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
        FstStringStart = (char*)FstStart + MaxEntryNum * sizeof(FSTEntry);
    }
}

static int isSame(const char* path, const char* name)
{
    while (*name) {
        if (tolower(*path++) != tolower(*name++)) return 0;
    }
    return *path == '/' || *path == 0;
}

long DVDConvertPathToEntrynum(const char* path)
{
    const char* component_end;
    const char* extension_start = 0;
    const char* original_path = path;
    char* name;
    unsigned long directory = currentDirectory;
    unsigned long entry;
    unsigned long component_length;
    int wants_directory;
    int extension;
    int illegal;

    for (;;) {
        if (!*path) return directory;
        if (*path == '/') {
            directory = 0;
            path++;
            continue;
        }
        if (*path == '.') {
            if (path[1] == '.' && path[2] == '/') {
                directory = ENTRY_PARENT(directory);
                path += 3;
                continue;
            }
            if (path[1] == '.' && !path[2]) return ENTRY_PARENT(directory);
            if (path[1] == '/') {
                path += 2;
                continue;
            }
            if (!path[1]) return directory;
        }

        if (!__DVDLongFileNameFlag) {
            extension = 0;
            illegal = 0;
            for (component_end = path; *component_end && *component_end != '/';
                 component_end++) {
                if (*component_end == '.') {
                    if (component_end - path > 8 || extension) {
                        illegal = 1;
                        break;
                    }
                    extension = 1;
                    extension_start = component_end + 1;
                } else if (*component_end == ' ') {
                    illegal = 1;
                }
            }
            if (extension && component_end - extension_start > 3) illegal = 1;
            if (illegal) {
                OSPanic("dvdfs.c", 387,
                        "DVDConvertEntrynumToPath(possibly DVDOpen or DVDChangeDir or DVDOpenDir): "
                        "specified directory or file (%s) doesn't match standard 8.3 format. This is a "
                        "temporary restriction and will be removed soon\n", original_path);
            }
        } else {
            for (component_end = path;
                 *component_end && *component_end != '/'; component_end++) {}
        }

        wants_directory = *component_end != 0;
        component_length = component_end - path;
        for (entry = directory + 1; entry < ENTRY_NEXT(directory);
             entry = ENTRY_IS_DIRECTORY(entry) ? ENTRY_NEXT(entry) : entry + 1) {
            if (!ENTRY_IS_DIRECTORY(entry) && wants_directory) continue;
            name = FstStringStart + ENTRY_NAME_OFFSET(entry);
            if (isSame(path, name)) break;
        }
        if (entry == ENTRY_NEXT(directory)) return -1;
        if (!wants_directory) return entry;
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

int DVDGetCurrentDir(char* path, unsigned long max_length)
{
    unsigned long position = entryToPath(currentDirectory, path, max_length);
    if (position == max_length) {
        path[max_length - 1] = 0;
        return 0;
    }
    if (ENTRY_IS_DIRECTORY(currentDirectory)) {
        if (position == max_length - 1) {
            path[position] = 0;
            return 0;
        }
        path[position++] = '/';
    }
    path[position] = 0;
    return 1;
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
    DVDCommandBlock* block = &file_info->cb;
    long state;
    long result;
    int enabled;

    if (!(0 <= offset && offset <= file_info->length)) {
        OSPanic("dvdfs.c", 820,
                "DVDRead(): specified area is out of the file  ");
    }
    if (!(0 <= offset + length &&
          offset + length < file_info->length + 32)) {
        OSPanic("dvdfs.c", 826,
                "DVDRead(): specified area is out of the file  ");
    }
    if (!DVDReadAbsAsyncPrio(block, address, length,
                             file_info->start_address + offset,
                             cbForReadSync, priority)) return -1;
    enabled = OSDisableInterrupts();
    for (;;) {
        state = ((volatile DVDCommandBlock*)block)->state;
        if (state == 0) {
            result = block->transferred_size;
            break;
        }
        if (state == -1) {
            result = -1;
            break;
        }
        if (state == 10) {
            result = -3;
            break;
        }
        OSSleepThread(&__DVDThreadQueue);
    }
    OSRestoreInterrupts(enabled);
    return result;
}

static void cbForReadSync(long result, DVDCommandBlock* block)
{
    OSWakeupThread(&__DVDThreadQueue);
}

long DVDGetTransferredSize(DVDFileInfo* file_info)
{
    DVDCommandBlock* block = &file_info->cb;

    switch (block->state) {
    case 0: case -1: case 3: case 4: case 5:
    case 6: case 7: case 10: case 11:
        return block->transferred_size;
    case 2:
        return 0;
    case 1:
        return block->transferred_size +
               (block->current_transfer_size - DI_REGS[6]);
    }
    return 0;
}
