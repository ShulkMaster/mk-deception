#ifndef MSL_STREAM_FILE_H
#define MSL_STREAM_FILE_H

#include "mw/mwFile.h"

typedef struct mslStreamFileRequest mslStreamFileRequest;

typedef void (*mslStreamFileCallback)(
    void* buffer, unsigned long offset, int size, int error,
    int final_chunk, void* callback_data);

#ifdef __cplusplus
extern "C" {
#endif

void mslStreamFile_CancelRequest(void* handle);
void mslStreamFile_Initialize(void);
void mslStreamFile_ReturnBuffer_FromInterrupt(void* buffer);
int mslStreamFile_ReturnBuffer(void* buffer);
mslStreamFileRequest* mslStreamFile_QueueRequest(
    _mwFile* file, unsigned long offset, unsigned long size, int priority,
    mslStreamFileCallback callback, void* callback_data);

#ifdef __cplusplus
}
#endif

#endif
