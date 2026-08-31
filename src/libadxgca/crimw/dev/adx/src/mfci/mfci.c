#include "cri/svm.h"
#include "runtime/cstdio.h"
#include "runtime/cstdlib.h"
#include "runtime/cstring.h"

#define MFCI_MAX_HANDLES 40
#define MFCI_DEFAULT_SECTOR_LENGTH 0x800

typedef enum MfCiStatus {
    MFCI_STATUS_IDLE = 0,
    MFCI_STATUS_COMPLETE = 1,
    MFCI_STATUS_TRANSFERRING = 2
} MfCiStatus;

typedef struct MfCiObject {
    signed char used;
    signed char status;
    unsigned char reserved_02[2];
    int sector_length;
    int file_size;
    int sector_count;
    int sector_position;
    int transfer_length;
    int request_sectors;
    char filename[20];
    int source_offset;
    int request_length;
} MfCiObject;

typedef void (*MfCiErrorCallback)(void* object, const char* message,
                                  void* handle);

typedef struct CvFsInterface {
    void (*ExecServer)(void);
    void (*EntryErrFunc)(MfCiErrorCallback callback, void* object);
    int (*GetFileSize)(const char* filename);
    int (*GetFreeSize)(void);
    void* (*Open)(const char* filename, void* parameter, int mode);
    void (*Close)(void* object);
    int (*Seek)(void* object, int offset, int origin);
    int (*Tell)(void* object);
    int (*ReqRd)(void* object, int sectors, void* buffer);
    int (*ReqWr)(void* object, int sectors, void* buffer);
    void (*StopTr)(void* object);
    MfCiStatus (*GetStat)(void* object);
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

int mfCiGetNumTr(void* object);
void mfCiSetSctLen(void* object, int sector_length);
int mfCiGetSctLen(void* object);
MfCiStatus mfCiGetStat(void* object);
void mfCiStopTr(void* object);
int mfCiReqRd(void* object, int sectors, void* buffer);
int mfCiTell(void* object);
int mfCiSeek(void* object, int offset, int origin);
void mfCiClose(void* object);
void* mfCiOpen(const char* filename, void* parameter, int mode);
int mfCiGetFileSize(const char* filename);
void mfCiEntryErrFunc(MfCiErrorCallback callback, void* object);
void mfCiExecServer(void);
CvFsInterface* mfCiGetInterface(void);

const char* const mfci_build =
    "\nMFCI/GC Ver.1.09 Build:Sep  3 2004 17:48:35\n";

/* Diagnostics retained by the retail entry-indexed memory-file API. */
static const char mfci_bad_entry[] =
    "E1041001:invalid entry number.(mfCiOpenEntry)";
static const char mfci_bad_entry_mode[] =
    "E1041002:rw is illigal.(mfCiOpenEntry)";
static const char mfci_no_entry_handle[] =
    "E1041002:not enough handle resource.(mfCiOpenEntry)";
static const char mfci_bad_filename_length[] =
    "E01100308:length of '%s' is not 17 bytes.(mfci_get_adr_size)";
static const char mfci_bad_filename_format[] =
    "E01100309:illegal file name format '%s'(mfci_get_adr_size)";

MfCiErrorCallback mfci_err_func;
void* mfci_err_obj;
char mfci_err_str[300];
MfCiObject mfci_obj[MFCI_MAX_HANDLES];

extern CvFsInterface mfci_vtbl;

typedef char MfCiObjectSizeCheck[sizeof(MfCiObject) == 0x38 ? 1 : -1];
typedef char CvFsInterfaceSizeCheck[sizeof(CvFsInterface) == 0x68 ? 1 : -1];

static inline unsigned char* mfci_get_adr_size(const char* filename,
                                                int* file_size)
{
    char* end;
    unsigned long address;

    if (strlen(filename) != 17) {
        sprintf(mfci_err_str, mfci_bad_filename_length, filename);
        if (mfci_err_func != 0) {
            mfci_err_func(mfci_err_obj, mfci_err_str, 0);
        }
    }
    if (filename[8] != '.') {
        sprintf(mfci_err_str, mfci_bad_filename_format, filename);
        if (mfci_err_func != 0) {
            mfci_err_func(mfci_err_obj, mfci_err_str, 0);
        }
    }

    end = (char*)filename;
    address = strtoul(filename, &end, 16);
    if (*end != '\0') {
        end++;
    }
    if (file_size != 0) {
        *file_size = (int)strtoul(end, &end, 16);
    }
    return (unsigned char*)address;
}

int mfCiGetNumTr(void* object)
{
    MfCiObject* handle = (MfCiObject*)object;

    if (handle == 0) {
        if (mfci_err_func != 0) {
            mfci_err_func(mfci_err_obj, "E0092912:handl is null.", 0);
        }
        return 0;
    }
    return handle->transfer_length;
}

void mfCiSetSctLen(void* object, int sector_length)
{
    MfCiObject* handle = (MfCiObject*)object;
    int byte_position;

    if (handle == 0) {
        if (mfci_err_func != 0) {
            mfci_err_func(mfci_err_obj, "E0040302:handl is null.", 0);
        }
        return;
    }

    byte_position = handle->sector_position * handle->sector_length;
    handle->sector_length = sector_length;
    handle->sector_count =
        ((handle->sector_length + handle->file_size) - 1) /
        handle->sector_length;
    handle->sector_position = byte_position / handle->sector_length;
    handle->transfer_length = handle->request_sectors * sector_length;
}

int mfCiGetSctLen(void* object)
{
    MfCiObject* handle = (MfCiObject*)object;

    if (handle == 0) {
        if (mfci_err_func != 0) {
            mfci_err_func(mfci_err_obj, "E0040301:handl is null.", 0);
        }
        return 0;
    }
    return handle->sector_length;
}

MfCiStatus mfCiGetStat(void* object)
{
    MfCiObject* handle = (MfCiObject*)object;

    if (handle == 0) {
        if (mfci_err_func != 0) {
            mfci_err_func(mfci_err_obj, "E0092912:handl is null.", 0);
        }
        return MFCI_STATUS_IDLE;
    }
    return (MfCiStatus)handle->status;
}

void mfCiStopTr(void* object)
{
    MfCiObject* handle = (MfCiObject*)object;

    if (handle == 0) {
        if (mfci_err_func != 0) {
            mfci_err_func(mfci_err_obj, "E0092912:handl is null.", 0);
        }
        return;
    }

    SVM_Lock();
    handle->status = MFCI_STATUS_IDLE;
    SVM_Unlock();
}

int mfCiReqRd(void* object, int sectors, void* buffer)
{
    MfCiObject* handle = (MfCiObject*)object;
    unsigned char* source;
    int file_size;
    int available_length;
    int request_sectors;
    int sector_length;
    int source_offset;
    int request_length;

    if (handle == 0) {
        if (mfci_err_func != 0) {
            mfci_err_func(mfci_err_obj, "E01100307:handl is null.", 0);
        }
        return 0;
    }
    if (sectors < 0) {
        if (mfci_err_func != 0) {
            mfci_err_func(mfci_err_obj,
                          "E01100308:nsct < 0.(mfCiReqRd)", handle);
        }
        return 0;
    }
    if (buffer == 0) {
        if (mfci_err_func != 0) {
            mfci_err_func(mfci_err_obj,
                          "E01100309:buf is null.(mfCiReqRd)", handle);
        }
        return 0;
    }
    if (sectors == 0) {
        handle->status = MFCI_STATUS_COMPLETE;
        return 0;
    }
    if (handle->status == MFCI_STATUS_TRANSFERRING) {
        return 0;
    }

    SVM_Lock();
    handle->transfer_length = 0;
    request_sectors = handle->sector_count - handle->sector_position;
    if (sectors < request_sectors) {
        request_sectors = sectors;
    }
    handle->request_sectors = request_sectors;
    sector_length = handle->sector_length;
    request_length = handle->request_sectors * sector_length;
    source_offset = handle->sector_position * sector_length;
    if (request_length == 0) {
        handle->status = MFCI_STATUS_COMPLETE;
        SVM_Unlock();
        return 0;
    }
    handle->source_offset = source_offset;
    handle->request_length = request_length;
    handle->status = MFCI_STATUS_TRANSFERRING;

    source = mfci_get_adr_size(handle->filename, &file_size);
    available_length = handle->request_length;
    if (available_length > file_size - handle->source_offset) {
        available_length = file_size - handle->source_offset;
    }
    SVM_Unlock();

    memcpy(buffer, source + handle->source_offset, handle->request_length);
    memset((unsigned char*)buffer + available_length, 0,
           handle->request_length - available_length);

    SVM_Lock();
    handle->transfer_length =
        handle->request_sectors * handle->sector_length;
    handle->sector_position += handle->request_sectors;
    handle->status = MFCI_STATUS_COMPLETE;
    SVM_Unlock();
    return handle->request_sectors;
}

int mfCiTell(void* object)
{
    MfCiObject* handle = (MfCiObject*)object;

    if (handle == 0) {
        if (mfci_err_func != 0) {
            mfci_err_func(mfci_err_obj, "E01100306:handl is null.", 0);
        }
        return 0;
    }
    return handle->sector_position;
}

int mfCiSeek(void* object, int offset, int origin)
{
    MfCiObject* handle = (MfCiObject*)object;
    int position;

    if (handle == 0) {
        if (mfci_err_func != 0) {
            mfci_err_func(mfci_err_obj, "E01100305:handl is null.", 0);
        }
        return 0;
    }

    SVM_Lock();
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
    position = handle->sector_position;
    if (position <= 0) {
        position = 0;
    }
    handle->sector_position = position;
    SVM_Unlock();
    return handle->sector_position;
}

void mfCiClose(void* object)
{
    MfCiObject* handle = (MfCiObject*)object;

    if (handle != 0) {
        mfCiStopTr(handle);
        if (handle->used == 1) {
            handle->used = 0;
            memset(handle, 0, sizeof(*handle));
        }
    }
}

void* mfCiOpen(const char* filename, void* parameter, int mode)
{
    MfCiObject* handle;
    int index;
    int file_size;

    (void)parameter;
    if (filename == 0) {
        if (mfci_err_func != 0) {
            mfci_err_func(mfci_err_obj,
                          "E01100301:fname is null.(mfCiOpen)", 0);
        }
        return 0;
    }
    if (mode != 0) {
        if (mfci_err_func != 0) {
            mfci_err_func(mfci_err_obj,
                          "E01100302:rw is illigal.(mfCiOpen)", 0);
        }
        return 0;
    }

    handle = 0;
    for (index = 0; index < MFCI_MAX_HANDLES; index++) {
        if (mfci_obj[index].used == 0) {
            handle = &mfci_obj[index];
            break;
        }
    }
    if (handle == 0) {
        if (mfci_err_func != 0) {
            mfci_err_func(
                mfci_err_obj,
                "E01100303:not enough handle resource.(mfCiOpen)", 0);
        }
        return 0;
    }

    strcpy(handle->filename, filename);
    handle->sector_length = MFCI_DEFAULT_SECTOR_LENGTH;
    mfci_get_adr_size(handle->filename, &file_size);
    handle->file_size = file_size;
    handle->sector_count =
        ((handle->sector_length + handle->file_size) - 1) /
        handle->sector_length;
    handle->sector_position = 0;
    handle->request_sectors = 0;
    handle->transfer_length = 0;
    handle->status = MFCI_STATUS_IDLE;
    handle->used = 1;
    return handle;
}

int mfCiGetFileSize(const char* filename)
{
    int file_size;

    mfci_get_adr_size(filename, &file_size);
    return file_size;
}

void mfCiEntryErrFunc(MfCiErrorCallback callback, void* object)
{
    mfci_err_func = callback;
    mfci_err_obj = object;
}

void mfCiExecServer(void)
{
    /* Memory-backed transfers complete synchronously in mfCiReqRd. */
}

CvFsInterface* mfCiGetInterface(void)
{
    return &mfci_vtbl;
}

CvFsInterface mfci_vtbl = {
    mfCiExecServer,
    mfCiEntryErrFunc,
    mfCiGetFileSize,
    0,
    mfCiOpen,
    mfCiClose,
    mfCiSeek,
    mfCiTell,
    mfCiReqRd,
    0,
    mfCiStopTr,
    mfCiGetStat,
    mfCiGetSctLen,
    mfCiSetSctLen,
    mfCiGetNumTr,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0
};
