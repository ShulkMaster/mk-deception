#ifndef MSL_ARAM_H
#define MSL_ARAM_H

#include "dolphin/ar.h"

struct mslARQRequest {
    ARQRequest request;          /* +0x00 -- Nintendo DMA queue request */
    /* +0x20 is the free-list link before checkout and the DMA buffer after. */
    union {
        void* stream_buffer;
        mslARQRequest* next_free;
    };                            /* +0x20 */
    void* callback_data;          /* +0x24 */
}; /* 0x28 */

typedef char MslARQRequestSize[
    sizeof(mslARQRequest) == 0x28 ? 1 : -1];

class MSLGCN_ARamBlock {
public:
    int reference_count;          /* +0x00 */
    MSLGCN_ARamBlock* parent;      /* +0x04 */
    unsigned char channel_count;  /* +0x08 */
    unsigned char unknown09;      /* +0x09 */
    unsigned char allocation_kind;/* +0x0A */
    unsigned char owns_buffers;   /* +0x0B */
    int buffer_size;              /* +0x0C */
    int base;                     /* +0x10 -- primary ARAM buffer */
    int secondary_base;           /* +0x14 */

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
    unsigned long request_address);
void i_ARQCALLBACK_ReturnArq(unsigned long request_address);
mslARQRequest* mslGetArqRequest(void);
void mslArqRequest_Init(void);

#ifdef __cplusplus
}
#endif

#endif
