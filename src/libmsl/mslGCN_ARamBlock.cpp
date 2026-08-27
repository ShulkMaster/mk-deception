#include "msl/mslBank.h"
#include "msl/ExtHeapMgr.h"
#include "dolphin/os.h"
#include "runtime/cstring.h"
#include "msl/mslStreamFile.h"
#include "msl/mslARam.h"
#include "msl/mslgcn.h"
#include "mw/mwMemHeap.h"
#include "mw/mwMemNewDelete.h"
extern ExternalHeap* g_MSL_GCN_ARAM_Heap;

mslARQRequest mslARQ_Req_Pool[24];
mslARQRequest* mslARQ_Req_FreeList;
int gap_08_8051173C_sbss;

static inline MSLGCN_ARamBlock* allocate_block(void) {
    MSLGCN_ARamBlock* block = (MSLGCN_ARamBlock*)
        operator new(
            sizeof(MSLGCN_ARamBlock), MWSOUND_HEAP, (mwMemFlags)0x10,
            "MSLGCN_ARamBlock", 0, 0);

    if (block != 0) {
        block->parent = 0;
        block->channel_count = 1;
        block->unknown09 = 0;
        block->allocation_kind = 0;
        block->owns_buffers = 0;
        block->buffer_size = 0;
        block->base = 0;
        block->secondary_base = 0;
        block->reference_count = 1;
    }
    return block;
}

// With deferred inlining, MWCC emits these definitions in reverse source order.
MSLGCN_ARamBlock* MSLGCN_ARamBlock::GetObject(void) {
    return allocate_block();
}

void MSLGCN_ARamBlock::FreeObject(MSLGCN_ARamBlock* block) {
    delete block;
}

MSLGCN_ARamBlock::~MSLGCN_ARamBlock() {
    FreeResources();
}

void MSLGCN_ARamBlock::Release(void) {
    unsigned long enabled = OSDisableInterrupts();

    if (--reference_count == 0) {
        if (unknown09 != 0) {
            _MSL_GCN_BREAK();
        } else {
            FreeResources();
            SetParent(0);
            FreeObject(this);
        }
    }
    OSRestoreInterrupts(enabled);
}

void MSLGCN_ARamBlock::SetParent(MSLGCN_ARamBlock* new_parent) {
    if (parent != new_parent) {
        if (new_parent != 0) {
            unsigned long enabled = OSDisableInterrupts();
            new_parent->reference_count++;
            OSRestoreInterrupts(enabled);
        }
        if (parent != 0) {
            parent->Release();
        }
        parent = new_parent;
    }
}

void MSLGCN_ARamBlock::FreeResources(void) {
    if (owns_buffers != 0) {
        owns_buffers = 0;
        if (base != 0) {
            ExternalHeap_Free(g_MSL_GCN_ARAM_Heap, base);
        }
        if (secondary_base != 0) {
            ExternalHeap_Free(g_MSL_GCN_ARAM_Heap, secondary_base);
        }
    }

    buffer_size = 0;
    base = 0;
    secondary_base = 0;
    allocation_kind = 2;
}

void MSLGCN_ARamBlock::SetNumChannels(int channels) {
    if (channels < 2) {
        channel_count = 1;
    } else {
        channel_count = 2;
    }
}

void MSLGCN_ARamBlock::SetARamBuffers(
    int primary, int secondary, int size) {
    if (owns_buffers != 0) {
        owns_buffers = 0;
        if (base != 0) {
            ExternalHeap_Free(g_MSL_GCN_ARAM_Heap, base);
        }
        if (secondary_base != 0) {
            ExternalHeap_Free(g_MSL_GCN_ARAM_Heap, secondary_base);
        }
    }

    buffer_size = 0;
    base = 0;
    secondary_base = 0;
    allocation_kind = 2;
    buffer_size = size;
    base = primary;
    secondary_base = secondary;
    allocation_kind = 0;
}

static inline int allocate_bank_buffers(
    MSLGCN_ARamBlock* block, int size) {
    block->channel_count = 1;
    block->FreeResources();
    block->owns_buffers = 1;
    block->buffer_size = size;
    block->base = ExternalHeap_Alloc(g_MSL_GCN_ARAM_Heap, size);
    if (block->base == 0) {
        block->FreeResources();
        return 0;
    }
    if (block->channel_count == 2) {
        block->secondary_base =
            ExternalHeap_Alloc(g_MSL_GCN_ARAM_Heap, size);
        if (block->secondary_base == 0) {
            block->FreeResources();
            return 0;
        }
    }
    block->allocation_kind = 0;
    return size;
}

MSLGCN_ARamBlock* MSLGCN_ARamBlock::CreateBankBlock(int size) {
    MSLGCN_ARamBlock* block = allocate_block();

    if (block != 0) {
        int allocated_size = allocate_bank_buffers(block, size);
        if (allocated_size != 0) {
            block->allocation_kind = 1;
        }
        if (allocated_size == 0) {
            FreeObject(block);
            block = 0;
        }
    }
    return block;
}

extern "C" void mslArqRequest_Init(void) {
    int index;

    mslARQ_Req_FreeList = mslARQ_Req_Pool;
    for (index = 0; index < 23; index++) {
        mslARQ_Req_Pool[index].next_free =
            &mslARQ_Req_Pool[index + 1];
    }
    mslARQ_Req_Pool[23].next_free = 0;
}

extern "C" mslARQRequest* mslGetArqRequest(void) {
    mslARQRequest* request;
    unsigned long enabled = OSDisableInterrupts();

    request = mslARQ_Req_FreeList;
    if (request != 0) {
        mslARQ_Req_FreeList = request->next_free;
        memset(request, 0, sizeof(mslARQRequest));
    }
    OSRestoreInterrupts(enabled);
    return request;
}

extern "C" void i_ARQCALLBACK_ReturnArq(unsigned long request_address) {
    mslARQRequest* request = (mslARQRequest*)request_address;
    memset(request, 0, sizeof(mslARQRequest));
    request->next_free = mslARQ_Req_FreeList;
    mslARQ_Req_FreeList = request;
}

extern "C" void i_ARQCALLBACK_ReturnArqAndUserStreamBuffer(
    unsigned long request_address) {
    mslARQRequest* request = (mslARQRequest*)request_address;
    mslStreamFile_ReturnBuffer_FromInterrupt(
        request->stream_buffer);
    memset(request, 0, sizeof(mslARQRequest));
    request->next_free = mslARQ_Req_FreeList;
    mslARQ_Req_FreeList = request;
}
