#ifndef MW_MWFILE_H
#define MW_MWFILE_H

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
    unsigned long bytes;
} mwFileAsyncValue;

typedef struct mwFileAsyncResult {
    mwFileAsyncValue value;
    int error;
} mwFileAsyncResult;

typedef void (*mwFileCallback)(void* command, mwFileAsyncResult* result,
                               void* arg);

void mwFileGetDefaultInitParam(mwFileInitParam* param);
int mwFileInit(mwFileInitParam* param);
int mwFileMountPath(const char* mount, const char* path);
int mwFileSetErrorCallback(const char* mount, mwFileErrorCallback callback,
                           void* arg);
void mwFileTick(void);
void* mwFileOpenAsync(const char* path, int flags, mwFileCallback callback,
                      void* arg);
void* mwFileCloseAsync(void* file, int flags, mwFileCallback callback);
void* mwFileReadAsync(void* file, long long offset, void* buffer,
                      int length, int count, mwFileCallback callback, void* arg);
void* mwFileWriteAsync(void* file, long long offset, void* buffer,
                       int length, int count, mwFileCallback callback, void* arg);
unsigned char mwFileIsCommandCompleted(void* command,
                                       mwFileAsyncResult* result);
void mwFileAbortCommand(void* command);
void mwFileFreeCommand(void* command);
mwFileAsyncResult mwFileTell(void* file);
mwFileAsyncResult mwFileSeek(void* file, long long offset, int origin);
int mwFileOpenModeToFlags(const char* mode);

#endif
