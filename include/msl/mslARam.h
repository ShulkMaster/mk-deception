#ifndef MSL_ARAM_H
#define MSL_ARAM_H

#include "dolphin/ar.h"

struct mslARQRequest {
    ARQRequest request;
    union {
        void* stream_buffer;
        mslARQRequest* next_free;
    };
    void* callback_data;
};

typedef char MslARQRequestSize[
    sizeof(mslARQRequest) == 0x28 ? 1 : -1];

class MSLGCN_ARamBlock {
public:
    int reference_count;
    MSLGCN_ARamBlock* parent;
    unsigned char channel_count;
    unsigned char unknown09;
    unsigned char allocation_kind;
    unsigned char owns_buffers;
    int buffer_size;
    int base;
    int secondary_base;

    static MSLGCN_ARamBlock* CreateBankBlock(int size);
    static MSLGCN_ARamBlock* GetObject(void);
    static void FreeObject(MSLGCN_ARamBlock* block);
    ~MSLGCN_ARamBlock();
    void FreeResources(void);
    void Release(void);
    void SetParent(MSLGCN_ARamBlock* parent);
    void SetARamBuffers(int primary, int secondary, int size);
    void SetNumChannels(int channels);
};

#ifdef __cplusplus
extern "C" {
#endif

void i_ARQCALLBACK_ReturnArqAndUserStreamBuffer(
    u32 request_address);
void i_ARQCALLBACK_ReturnArq(u32 request_address);
mslARQRequest* mslGetArqRequest(void);
void mslArqRequest_Init(void);

#ifdef __cplusplus
}
#endif

#endif
