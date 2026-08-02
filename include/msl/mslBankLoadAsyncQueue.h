#ifndef MSL_BANK_LOAD_ASYNC_QUEUE_H
#define MSL_BANK_LOAD_ASYNC_QUEUE_H

struct _mslSystem;

struct _mslAsyncResponse {
    void (*callback)(_mslAsyncResponse*);
    int status;
    void* result;
};

extern "C" int mslBankLoadAsyncCancelNamed(char* filename);

extern "C" void mslBankLoadAsync(
    _mslSystem* system, unsigned long flags, char* filename,
    _mslAsyncResponse* response);

#endif
