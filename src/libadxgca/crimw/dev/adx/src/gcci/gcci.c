#include "dolphin/cache.h"
#include "dolphin/dvd.h"
#include "dolphin/os.h"
#include "runtime/cstring.h"

#define GCCI_MAX_HANDLES 40
#define GCCI_DEFAULT_SECTOR_LENGTH 0x800
#define GCCI_DVD_PRIORITY 2
#define GCCI_CANCEL_TIMEOUT_MS 2000

typedef enum GcCiStatus {
    GCCI_STATUS_IDLE = 0,
    GCCI_STATUS_COMPLETE = 1,
    GCCI_STATUS_TRANSFERRING = 2,
    GCCI_STATUS_ERROR = 3
} GcCiStatus;

typedef struct GcCiObject {
    signed char used;
    unsigned char reserved_01;
    signed char status;
    unsigned char reserved_03;
    unsigned char reserved_04[4];
    void* transfer_buffer;
    long dvd_status;
    long sector_length;
    long file_size;
    long sector_count;
    long sector_position;
    long transfer_length;
    long request_sectors;
    DVDFileInfo file_info;
} GcCiObject;

typedef struct GcCiDebug {
    long dvd_status;
    signed char transfer_status;
    unsigned char reserved_05[3];
    long canceling;
} GcCiDebug;

typedef void (*GcCiErrorCallback)(void* object, const char* message,
                                  void* handle);

typedef struct CvFsInterface {
    void (*ExecServer)(void);
    void (*EntryErrFunc)(GcCiErrorCallback callback, void* object);
    int (*GetFileSize)(const char* filename);
    int (*GetFreeSize)(void);
    void* (*Open)(const char* filename, void* parameter, int mode);
    void (*Close)(void* object);
    int (*Seek)(void* object, int offset, int origin);
    int (*Tell)(void* object);
    int (*ReqRd)(void* object, int sectors, void* buffer);
    int (*ReqWr)(void* object, int sectors, void* buffer);
    void (*StopTr)(void* object);
    GcCiStatus (*GetStat)(void* object);
    int (*GetSctLen)(void* object);
    void (*SetSctLen)(void* object, int sector_length);
    int (*GetNumTr)(void* object);
    int (*ChangeDir)(const char* dirname);
    int (*IsExistFile)(const char* filename);
    int (*GetNumFiles)(void);
    int (*LoadDirInfo)(const char* dirname, void* info, int count);
    int (*GetMaxByteRate)(void* object);
    int (*MakeDir)(const char* dirname);
    int (*RemoveDir)(const char* dirname);
    int (*DeleteFile)(const char* filename);
    int (*GetFileSizeEx)(const char* filename, void* parameter);
    int (*OptFn1)(void* object, int operation, int arg2, int arg3);
    int (*OptFn2)(void* object, int arg1, int arg2, int arg3);
} CvFsInterface;

void gcci_rd_cbfn(long result, DVDFileInfo* file_info);
int gcCiGetNumTr(void* object);
void gcCiSetSctLen(void* object, int sector_length);
int gcCiGetSctLen(void* object);
GcCiStatus gcCiGetStat(void* object);
void gcCiStopTr(void* object);
int gcCiReqRd(void* object, int sectors, void* buffer);
int gcCiTell(void* object);
int gcCiSeek(void* object, int offset, int origin);
void gcCiClose(void* object);
void* gcCiOpen(const char* filename, void* parameter, int mode);
int gcCiGetFileSize(const char* filename);
void gcCiEntryErrFunc(GcCiErrorCallback callback, void* object);
void gcCiExecServer(void);
CvFsInterface* gcCiGetInterface(void);

const char gcg_ci_build_str[] =
    "\nGCCI Ver.1.09 Build:Sep  3 2004 17:47:55\n";

CvFsInterface gcg_ci_vtbl = {
    gcCiExecServer, gcCiEntryErrFunc, gcCiGetFileSize, 0,
    gcCiOpen, gcCiClose, gcCiSeek, gcCiTell, gcCiReqRd, 0,
    gcCiStopTr, gcCiGetStat, gcCiGetSctLen, gcCiSetSctLen, gcCiGetNumTr,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

GcCiDebug gcg_ci_debug;
void* gcg_ci_err_obj;
GcCiErrorCallback gcg_ci_err_func;
GcCiObject gcg_ci_obj[GCCI_MAX_HANDLES];
const char* volatile gcg_ci_build_ptr;
extern int gcg_ci_rdmode[1];
extern char gcg_ci_root_dir[256];

typedef char GcCiObjectSizeCheck[sizeof(GcCiObject) == 0x64 ? 1 : -1];
typedef char GcCiDebugSizeCheck[sizeof(GcCiDebug) == 0x0C ? 1 : -1];
typedef char CvFsInterfaceSizeCheck[sizeof(CvFsInterface) == 0x68 ? 1 : -1];

void gcCiInit(void)
{
    gcg_ci_debug.transfer_status = 0;
    gcg_ci_err_obj = 0;
    gcg_ci_err_func = 0;
}

void gcci_rd_cbfn(long result, DVDFileInfo* file_info)
{
}

int gcCiGetNumTr(void* object)
{
    GcCiObject* handle = object;

    if (handle == 0) {
        if (gcg_ci_err_func != 0) {
            gcg_ci_err_func(gcg_ci_err_obj, "E0092912:handl is null.", 0);
        }
        return 0;
    }
    return handle->transfer_length;
}

void gcCiSetSctLen(void* object, int sector_length)
{
    GcCiObject* handle = object;
    int byte_position;

    if (handle == 0) {
        if (gcg_ci_err_func != 0) {
            gcg_ci_err_func(gcg_ci_err_obj, "E0040302:handl is null.", 0);
        }
        return;
    }
    if (handle->sector_length % 32 != 0) {
        if (gcg_ci_err_func != 0) {
            gcg_ci_err_func(gcg_ci_err_obj, "E0040303:invalidate size.", 0);
        }
        return;
    }

    byte_position = handle->sector_position * handle->sector_length;
    handle->sector_length = sector_length;
    handle->sector_count =
        (handle->file_size + (handle->sector_length - 1)) / handle->sector_length;
    handle->sector_position = byte_position / handle->sector_length;
    handle->transfer_length = handle->request_sectors * sector_length;
}

int gcCiGetSctLen(void* object)
{
    GcCiObject* handle = object;

    if (handle == 0) {
        if (gcg_ci_err_func != 0) {
            gcg_ci_err_func(gcg_ci_err_obj, "E0040301:handl is null.", 0);
        }
        return 0;
    }
    return handle->sector_length;
}

GcCiStatus gcCiGetStat(void* object)
{
    GcCiObject* handle = object;

    if (handle == 0) {
        if (gcg_ci_err_func != 0) {
            gcg_ci_err_func(gcg_ci_err_obj, "E0092912:handl is null.", 0);
        }
        return GCCI_STATUS_IDLE;
    }
    return handle->status;
}

static inline void gcci_call_error(const char* message, void* handle)
{
    if (gcg_ci_err_func != 0) {
        gcg_ci_err_func(gcg_ci_err_obj, message, handle);
    }
}

static inline int gcci_is_idle(const GcCiObject* handle)
{
    int idle = 0;

    if (handle->status == GCCI_STATUS_COMPLETE ||
        handle->status == GCCI_STATUS_IDLE) {
        idle = 1;
    }
    return idle;
}

static inline int gcci_is_dvd_finished(const GcCiObject* handle)
{
    int done = 0;

    if (handle->dvd_status == DVD_STATE_END ||
        handle->dvd_status == DVD_STATE_CANCELED) {
        done = 1;
    }
    return done;
}

void gcCiStopTr(void* object)
{
    GcCiObject* handle;
    int cancel_result;
    unsigned long start;
    unsigned long current;
    unsigned long elapsed;

    handle = object;
    if (handle == 0) {
        gcci_call_error("E0092912:handl is null.", 0);
        return;
    }
    if (handle->status == GCCI_STATUS_COMPLETE ||
        handle->status == GCCI_STATUS_IDLE) {
        return;
    }
    DVDGetCommandBlockStatus(&handle->file_info.cb);
    DVDGetDriveStatus();
    gcg_ci_debug.canceling = 1;
    cancel_result = DVDCancel(&handle->file_info.cb);
    gcg_ci_debug.canceling = 0;
    if (cancel_result < 0) {
        gcci_call_error("E0092917:DVDCancel failed.", handle);
        return;
    }
    start = OSTicksToMilliseconds(OSGetTick());
    while (!gcci_is_dvd_finished(handle)) {
        handle->dvd_status = DVDGetCommandBlockStatus(&handle->file_info.cb);
        gcg_ci_debug.dvd_status = handle->dvd_status;
        current = OSTicksToMilliseconds(OSGetTick());
        elapsed = (current >= start) ? (current - start)
                                     : (0xFFFFFFFF - start + current);
        if (elapsed > GCCI_CANCEL_TIMEOUT_MS) {
            gcci_call_error("E0092918:DVDCancel time out.", handle);
            break;
        }
    }
    handle->status = GCCI_STATUS_IDLE;
    gcg_ci_debug.transfer_status = GCCI_STATUS_IDLE;
    DVDGetCommandBlockStatus(&handle->file_info.cb);
    DVDGetDriveStatus();
}

static inline void gcci_update_transfer(GcCiObject* handle)
{
    int nbyte;
    int read_length;
    unsigned char* fill;

    if (handle->used != 1 || handle->status != GCCI_STATUS_TRANSFERRING) {
        return;
    }

    handle->dvd_status = DVDGetCommandBlockStatus(&handle->file_info.cb);
    gcg_ci_debug.dvd_status = handle->dvd_status;
    switch (handle->dvd_status) {
    case DVD_STATE_FATAL_ERROR:
        handle->status = GCCI_STATUS_ERROR;
        gcg_ci_debug.transfer_status = GCCI_STATUS_ERROR;
        break;
    case DVD_STATE_END:
        read_length = handle->request_sectors * handle->sector_length;
        DCInvalidateRange(handle->transfer_buffer, read_length);
        handle->transfer_length = read_length;
        handle->sector_position += handle->request_sectors;
        if (handle->sector_position * handle->sector_length >
            handle->file_size) {
            nbyte =
                handle->sector_position * handle->sector_length -
                handle->file_size;
            fill = (unsigned char*)handle->transfer_buffer +
                   handle->transfer_length - nbyte;
            memset(fill, 0, nbyte);
            DCStoreRange(fill, nbyte);
        }
        handle->status = GCCI_STATUS_COMPLETE;
        gcg_ci_debug.transfer_status = GCCI_STATUS_COMPLETE;
        break;
    case DVD_STATE_CANCELED:
        nbyte = DVDGetTransferredSize(&handle->file_info);
        DCInvalidateRange(handle->transfer_buffer, nbyte);
        handle->transfer_length =
            handle->sector_length * (nbyte / handle->sector_length);
        handle->sector_position += nbyte / handle->sector_length;
        handle->status = gcg_ci_debug.transfer_status = GCCI_STATUS_IDLE;
        break;
    }
}

static inline long gcci_is_any_transferring(GcCiObject* table)
{
    long index;

    for (index = 0; index < GCCI_MAX_HANDLES; index++) {
        if (table[index].used == 1 &&
            table[index].status == GCCI_STATUS_TRANSFERRING) {
            return 1;
        }
    }
    return 0;
}

static inline void gcci_update_table(GcCiObject* table)
{
    long index;

    for (index = 0; index < GCCI_MAX_HANDLES; index++) {
        gcci_update_transfer(&table[index]);
    }
}

int gcCiReqRd(void* object, int sectors, void* buffer)
{
    int aligned_length;
    GcCiObject* handle = object;
    int offset;
    int length;
    int result;

    if (handle == 0) {
        if (gcg_ci_err_func != 0) {
            gcg_ci_err_func(gcg_ci_err_obj, "E0092912:handl is null.", 0);
        }
        return 0;
    }
    if (sectors < 0) {
        if (gcg_ci_err_func != 0) {
            gcg_ci_err_func(gcg_ci_err_obj,
                            "E0092913:nsct < 0.(gcCiReqRd)", handle);
        }
        return 0;
    }
    if (buffer == 0) {
        if (gcg_ci_err_func != 0) {
            gcg_ci_err_func(gcg_ci_err_obj,
                            "E0092914:buf is null.(gcCiReqRd)", handle);
        }
        return 0;
    }
    if (!gcci_is_idle(handle)) {
        return 0;
    }
    if (!gcci_is_dvd_finished(handle)) {
        return 0;
    }

    if (gcci_is_any_transferring(gcg_ci_obj)) {
        return 0;
    }

    if (sectors == 0) {
        handle->status = GCCI_STATUS_COMPLETE;
        gcg_ci_debug.transfer_status = GCCI_STATUS_COMPLETE;
        return 0;
    }

    handle->transfer_length = 0;
    handle->transfer_buffer = buffer;
    handle->request_sectors = sectors;

    gcci_update_table(gcg_ci_obj);

    offset = handle->sector_position * handle->sector_length;
    length = handle->request_sectors * handle->sector_length;
    if (offset + length > handle->file_size) {
        length = handle->file_size - offset;
        if (length < 0) {
            handle->status = GCCI_STATUS_COMPLETE;
            gcg_ci_debug.transfer_status = GCCI_STATUS_COMPLETE;
            return sectors;
        }
    }

    aligned_length = (length + 31) & ~31;
    DCInvalidateRange(buffer, aligned_length);
    if (gcg_ci_rdmode[0] == 0) {
        result = DVDReadAsyncPrio(&handle->file_info, buffer, aligned_length,
                                  offset, gcci_rd_cbfn, GCCI_DVD_PRIORITY);
    } else {
        result = DVDReadPrio(&handle->file_info, buffer, aligned_length, offset,
                             GCCI_DVD_PRIORITY);
    }
    if (result == 0) {
        return 0;
    }

    handle->status = GCCI_STATUS_TRANSFERRING;
    gcg_ci_debug.transfer_status = GCCI_STATUS_TRANSFERRING;
    return handle->request_sectors;
}

int gcCiTell(void* object)
{
    GcCiObject* handle = object;

    if (handle == 0) {
        if (gcg_ci_err_func != 0) {
            gcg_ci_err_func(gcg_ci_err_obj, "E0092912:handl is null.", 0);
        }
        return 0;
    }
    return handle->sector_position;
}

/* TODO: [near miss] 97.84%; seek and clamp operations agree;
 * lower-clamp branch polarity and extra retail jump remain. */
int gcCiSeek(void* object, int offset, int origin)
{
    GcCiObject* handle = object;
    long position;

    if (handle == 0) {
        gcci_call_error("E0092912:handl is null.", 0);
        return 0;
    }
    if (origin == 0) {
        handle->sector_position = offset;
    } else if (origin == 2) {
        handle->sector_position = handle->sector_count + offset;
    } else if (origin == 1) {
        handle->sector_position += offset;
    }

    position = handle->sector_count;
    if (handle->sector_position < position) {
        position = handle->sector_position;
    }
    handle->sector_position = position;
    handle->sector_position = (handle->sector_position > 0) ? handle->sector_position : 0;
    return handle->sector_position;
}

void gcCiClose(void* object)
{
    GcCiObject* handle;

    handle = object;
    if (handle == 0) {
        return;
    }
    gcCiStopTr(object);
    DVDClose(&handle->file_info);
    handle->used = 0;
    memset(handle, 0, sizeof(GcCiObject));
}

static inline void gcci_make_path(char* path, const char* filename)
{
    char* current;
    unsigned long length;
    unsigned long index;

    strcpy(path, gcg_ci_root_dir);
    strcat(path, filename);
    length = strlen(path);
    current = path;
    for (index = 0; index < length; index++) {
        if (*current == '\\') {
            *current = '/';
        }
        current++;
    }
}

static inline GcCiObject* gcci_find_unused_handle(void)
{
    GcCiObject* handle;
    GcCiObject* current;
    int index;

    handle = 0;
    current = gcg_ci_obj;
    for (index = 0; index < GCCI_MAX_HANDLES; current++, index++) {
        if (current->used == 0) {
            handle = &gcg_ci_obj[index];
            break;
        }
    }
    return handle;
}

void* gcCiOpen(const char* filename, void* parameter, int mode)
{
    char path[256];
    GcCiObject* handle;
    int file_size;

    if (filename == 0) {
        if (gcg_ci_err_func != 0) {
            gcg_ci_err_func(gcg_ci_err_obj,
                            "E0092908:fname is null.(gcCiOpen)", 0);
        }
        return 0;
    }
    if (mode != 0) {
        if (gcg_ci_err_func != 0) {
            gcg_ci_err_func(gcg_ci_err_obj,
                            "E0092909:rw is illigal.(gcCiOpen)", 0);
        }
        return 0;
    }

    handle = gcci_find_unused_handle();
    if (handle == 0) {
        if (gcg_ci_err_func != 0) {
            gcg_ci_err_func(
                gcg_ci_err_obj,
                "E0092910:not enough handle resource.(gcCiOpen)", 0);
        }
        return 0;
    }

    gcci_make_path(path, filename);
    if (DVDOpen(path, &handle->file_info) == 0) {
        if (gcg_ci_err_func != 0) {
            gcg_ci_err_func(gcg_ci_err_obj,
                            "E0092911:DVDOpen fail.(gcCiOpen)", 0);
        }
        memset(handle, 0, sizeof(*handle));
        return 0;
    }

    handle->sector_length = GCCI_DEFAULT_SECTOR_LENGTH;
    file_size = handle->file_info.length;
    if ((file_size & 0x80000000U) != 0) {
        file_size = 0x7FFFFFFF;
    }
    handle->file_size = file_size;
    handle->sector_count =
        (int)((unsigned int)handle->file_size +
              ((unsigned int)handle->sector_length - 1U)) /
        handle->sector_length;
    handle->sector_position = 0;
    handle->transfer_buffer = 0;
    handle->request_sectors = 0;
    handle->transfer_length = 0;
    handle->status = GCCI_STATUS_IDLE;
    handle->used = 1;
    return handle;
}

int gcCiGetFileSize(const char* filename)
{
    DVDFileInfo file_info;
    char path[256];
    int file_size;

    if (filename == 0) {
        if (gcg_ci_err_func != 0) {
            gcg_ci_err_func(gcg_ci_err_obj,
                            "E0092901:fname is null.(gcCiGetFileSize)", 0);
        }
        return 0;
    }

    gcci_make_path(path, filename);
    if (DVDOpen(path, &file_info) == 0) {
        if (gcg_ci_err_func != 0) {
            gcg_ci_err_func(
                gcg_ci_err_obj,
                "E0040201:can't open a file.(gcCiGetFileSize)", 0);
        }
        return 0;
    }

    file_size = file_info.length;
    if ((file_size & 0x80000000U) != 0) {
        file_size = 0x7FFFFFFF;
    }
    if (DVDClose(&file_info) == 0) {
        if (gcg_ci_err_func != 0) {
            gcg_ci_err_func(
                gcg_ci_err_obj,
                "E0040202:can't close a file.(gcCiGetFileSize)", 0);
        }
        return 0;
    }
    return file_size;
}

void gcCiEntryErrFunc(GcCiErrorCallback callback, void* object)
{
    gcg_ci_err_func = callback;
    gcg_ci_err_obj = object;
}

static inline void gcci_update_transfers(void)
{
    int index = 0;
    GcCiObject* current = gcg_ci_obj;

    do {
        gcci_update_transfer(current);
        index++;
        current++;
    } while (index < GCCI_MAX_HANDLES);
}

/* TODO: [near miss] 97.87234%; loop and registers match; overflow subtraction schedules one slot early. */
void gcCiExecServer(void)
{
    gcci_update_transfers();
}

CvFsInterface* gcCiGetInterface(void)
{
    gcg_ci_build_ptr = gcg_ci_build_ptr;
    memset(gcg_ci_root_dir, 0, sizeof(gcg_ci_root_dir));
    gcg_ci_err_func = 0;
    gcg_ci_err_obj = 0;
    memset(&gcg_ci_debug, 0, sizeof(gcg_ci_debug));
    return &gcg_ci_vtbl;
}
