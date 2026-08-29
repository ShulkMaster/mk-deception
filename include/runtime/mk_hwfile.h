#ifndef MK_HWFILE_H
#define MK_HWFILE_H
#include "mw/mwFile.h"
typedef struct MkHwFileRequest MkHwFileRequest;
typedef void (*MkHwFileOpenCallback)(void*, MkHwFileRequest*, int);
struct MkHwFileRequest {
    void* mwFile;
    void* openCommand;
    MkHwFileOpenCallback lateOpenCallback;
    void* lateOpenCallbackArg;
};
typedef struct MkHwFileHandlePool {
    int count;
    int freeCount;
    MkHwFileRequest* base;
    int handleSize;
    signed char* freelist;
} MkHwFileHandlePool;
int mk_hwfile_link_late_open_callback(MkHwFileRequest*, MkHwFileOpenCallback, void*);
int mk_hwfile_is_file_ready(MkHwFileRequest*);
void mk_hwfile_busywait_dowork(void);
int mk_hwfile_write_blocking(MkHwFileRequest*, void*, int);
MkHwFileRequest* mk_hwfile_open_blocking(const char*, const char*);
void mk_hwfile_free_request(void*);
void mk_hwfile_wait_for_completion(void**);
void mk_hwfile_wait_for_completion_or_null_request(MkHwFileRequest**);
void mk_hwfile_close(MkHwFileRequest*);
int mk_hwfile_tell(MkHwFileRequest*);
int mk_hwfile_seek(MkHwFileRequest*, int, int);
void mk_hwfile_cancel(MkHwFileRequest*);
void* mk_hwfile_read_async(MkHwFileRequest*, int, void*, int);
int mk_hwfile_read(MkHwFileRequest*, void*, int);
MkHwFileRequest* mk_hwfile_open(const char*, const char*);
void mk_hwfile_init(void);

MkHwFileRequest* debug_file_open(const char* path, const char* mode);
int debug_file_write(MkHwFileRequest* file, void* buffer, int length);
void debug_file_close(MkHwFileRequest* file);
#endif
