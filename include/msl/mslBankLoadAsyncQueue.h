#ifndef MSL_BANK_LOAD_ASYNC_QUEUE_H
#define MSL_BANK_LOAD_ASYNC_QUEUE_H

struct _mslSystem;

struct _mslAsyncResponse {
    void (*callback)(_mslAsyncResponse*);
    int status;
    void* result;
};

void mslAsyncBegin(_mslAsyncResponse* response, void* user_data);
void mslAsyncComplete(_mslAsyncResponse* response, bool succeeded,
                      void* result, void* error);

#ifdef __cplusplus
extern "C" {
#endif

int mslBankLoadAsyncCancelNamed(char* filename);

void mslBankLoadAsync(
    _mslSystem* system, unsigned long flags, char* filename,
    _mslAsyncResponse* response);

#ifdef __cplusplus
}
#endif

#endif
