#ifndef MW_MWFILE_H
#define MW_MWFILE_H

typedef struct _mwFile _mwFile;
typedef struct mwFileCommand mwFileCommand;

typedef enum mwTargetMemAlign {
    MW_TARGET_MEM_ALIGN_DEFAULT = 0
} mwTargetMemAlign;

typedef struct mwFileInitParam {
    unsigned long field_0x00;
    short max_open_files;       /* +0x04 */
    short field_0x06;
    short field_0x08;
    short field_0x0A;
    unsigned long flags;        /* +0x0C */
    short field_0x10;
    short field_0x12;
    void* allocator_context;    /* +0x14 */
    unsigned long field_0x18;
} mwFileInitParam; /* 0x1C */

typedef int (*mwFileErrorCallback)(int operation, int error);

typedef union mwFileAsyncValue {
    void* pointer;
    _mwFile* file;
    unsigned long bytes;
} mwFileAsyncValue;

typedef struct _mwFileAsyncResult {
    mwFileAsyncValue value;
    int error;
} _mwFileAsyncResult;

typedef _mwFileAsyncResult mwFileAsyncResult;

typedef void (*mwFileCallback)(mwFileCommand* command, mwFileAsyncResult result,
                               void* arg);

#ifdef __cplusplus
extern "C" {
#endif

void mwFileGetDefaultInitParam(mwFileInitParam* param);
int mwFileInit(mwFileInitParam* param);
int mwFileMountPath(const char* mount, const char* path);
int mwFileSetErrorCallback(const char* mount, mwFileErrorCallback callback,
                           void* arg);
void mwFileTick(void);
int mwFileClose(_mwFile* file);
_mwFile* mwFileOpen(const char* path, int flags);
mwFileCommand* mwFileOpenAsync(const char* path, int flags,
                               mwFileCallback callback, void* arg);
mwFileCommand* mwFileCloseAsync(_mwFile* file, int flags,
                                mwFileCallback callback);
mwFileCommand* mwFileReadAsync(_mwFile* file, long long offset, void* buffer,
                               unsigned long length, int count,
                               mwFileCallback callback, void* arg);
mwFileCommand* mwFileWriteAsync(_mwFile* file, long long offset, void* buffer,
                                unsigned long length, int count,
                                mwFileCallback callback, void* arg);
unsigned char mwFileIsCommandCompleted(mwFileCommand* command,
                                       mwFileAsyncResult* result);
mwFileAsyncResult mwFileWaitForCompletion(mwFileCommand* command);
void mwFileAbortCommand(mwFileCommand* command);
void mwFileFreeCommand(mwFileCommand* command);
long long mwFileTell(_mwFile* file);
long long mwFileSeek(_mwFile* file, long long offset, int origin);
long long mwFileGetSize(_mwFile* file);
int mwFileOpenModeToFlags(const char* mode);

#ifdef __cplusplus
}
#endif

#endif
