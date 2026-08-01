/*
 * Port readiness:
 *   Structs: PARTIAL
 *   Matching: 45.39% (.text)
 *   Linked: NO
 *   Status: REVIEW
 *   Gaps: overflow diagnostics, system parameters, and heap subtype payloads.
 */
#include "mw/mwMem.h"
#include "mw/mwMemPriv.h"

typedef struct FixedBlockHeapInitParams {
    u32 field_0x00;
    u32 count;
    u32 blockSize;
    u32 maxBlock;
    u32 flags;
} FixedBlockHeapInitParams;

typedef struct HdrlessHeapInitParams {
    u32 field_0x00;
    u32 count;
    u32 blockSize;
    u32 flags;
} HdrlessHeapInitParams;

typedef struct MwMemOverflowInfo {
    u32 reason;
    void* ptr;
    _mwMemHeap* originHeap;
    _mwMemHeap* destHeap;
    const char* originName;
    const char* destName;
    const char* file;
    u32 size;
    u32 line;
    u32 field_0x28;
    u32 field_0x2C;
    u32 field_0x30;
    void* systemParams;
    u32 field_0x38;
    u32 field_0x3C;
    u32 field_0x40;
    u32 field_0x44;
    u32 field_0x48;
} MwMemOverflowInfo;

typedef int OSHeapHandle;

static const char stringBase0[] =
    "1.5 rev 1"
    "System Heap"
    "mwMem.c"
    "MEM_ALWAYS_FAIL"
    "Assertion failure: MEM_ALWAYS_FAIL";

/* MWCC emits .sbss in reverse declaration order. */
u32 gap_08_80510ECC_sbss;
MwMemSystemParams systemParams;
u32 StrategyAllocationActive;
int SystemInitialize;
u32 heapCount;
_mwMemHeap* mwMemSystemOverflowHeap;
_mwMemHeap* newWrapperDefaultHeap;
_mwMemHeap* SystemHeap;
_mwMemHeap* HeapList;

static u8 heapIndexArray[0x100];

_mwMemHeap** SystemHeapTable[3] = {
    &SystemHeap,
    &mwMemSystemOverflowHeap,
    &newWrapperDefaultHeap,
};

const char* heapName = &stringBase0[10];

extern OSHeapHandle GameCubeSystemHeap;

void priv_mwMem_CritSecEnter(void);
void priv_mwMem_CritSecExit(void);
int privGetAlignFromMwMemFlags(u32 flags);

void normHeapResetHeap(_mwMemHeap* heap, int param);
void normHeapInitHeap(_mwMemHeap* heap, void* params);
void* normHeapMallocMem(u32 size, _mwMemHeap* heap, u32 flags, MwMemMallocRequest* request);
void normHeapFreeMemFromBlock(void* block);

void fixedBlockHeapResetHeap(_mwMemHeap* heap, int param);
void fixedBlockHeapInitHeap(_mwMemHeap* heap, FixedBlockHeapInitParams* params);
void* fixedBlockHeapAlloc(u32 size, _mwMemHeap* heap, u32 flags, MwMemMallocRequest* request);
void fixedBlockHeapFreeBlock(_mwMemHeap* heap, void* block);
int mwMemFixedBlockHeapGetHeapSize(FixedBlockHeapInitParams* params);

void hdrlessHeapResetHeap(_mwMemHeap* heap);
void hdrlessHeapInitHeap(_mwMemHeap* heap, HdrlessHeapInitParams* params);
void* hdrlessHeapAlloc(u32 size, _mwMemHeap* heap, u32 flags, MwMemMallocRequest* request);
void hdrlessHeapFreeBlock(_mwMemHeap* heap, void* block);
int mwMemHeaderlessFixedBlockGetHeapSize(HdrlessHeapInitParams* params);

void mwMemUserConfigAttemptingOverflowHeapCallback(MwMemOverflowInfo* info);
void mwMemUserConfigOutofMemoryCallback(MwMemOverflowInfo* info);
int mwMemUserConfigAssert(void);

void* OSAllocFromHeap(OSHeapHandle heap, u32 size);
void OSFreeToHeap(OSHeapHandle heap, void* ptr);
void OSPanic(const char* file, int line, const char* msg, ...);
void* privGetOSMemory(u32 size);
int privConsoleMemSystemInit(void);
void* memcpy(void* dst, const void* src, u32 size);
void* memset(void* dst, int val, u32 size);

static void _mwMemFreeVirtual(void* ptr, const char* file, u32 line);
static void* _mwMemMallocVirtual(MwMemMallocRequest* request);
static int privSystemCreateFromBuffer(u8* buffer, u32 size, _mwMemHeap** outHeap,
                                      const char* name);
static int privSystemCreateAutomated(u32 size, _mwMemHeap** outHeap, const char* name);

static int mwMemHeapHasValidMagic(_mwMemHeap* heap) {
    if (heap == 0) {
        return 0;
    }
    return heap->magic + 0x41550000 == 0xBEAB;
}

static void mwMemResetHeapByStrategy(_mwMemHeap* heap, int wipeMode) {
    switch (heap->strategy) {
    case MW_MEM_STRATEGY_HDRLESS:
        hdrlessHeapResetHeap(heap);
        break;
    case MW_MEM_STRATEGY_FIXED:
        fixedBlockHeapResetHeap(heap, wipeMode);
        break;
    default:
        if ((int)heap->strategy >= 0) {
            normHeapResetHeap(heap, wipeMode);
        }
        break;
    }
}

static void mwMemInitHeapByStrategy(_mwMemHeap* heap, MwMemHeapCreateParams* create) {
    switch (heap->strategy) {
    case MW_MEM_STRATEGY_HDRLESS:
        hdrlessHeapInitHeap(heap, (HdrlessHeapInitParams*)create->initParams);
        break;
    case MW_MEM_STRATEGY_FIXED:
        fixedBlockHeapInitHeap(heap, (FixedBlockHeapInitParams*)create->initParams);
        break;
    default:
        if ((int)heap->strategy >= 0) {
            normHeapInitHeap(heap, 0);
        }
        break;
    }
}

static int mwMemAllocStatSize(_mwMemHeap* heap, void* block) {
    MwMemUsedHdr* usedHdr;
    int size;

    switch (heap->strategy) {
    case MW_MEM_STRATEGY_HDRLESS:
        return heap->blockSize + heap->blockPrefixSize;
    case MW_MEM_STRATEGY_FIXED:
        return heap->blockSize + 0x10 + heap->blockPrefixSize;
    default:
        usedHdr = (MwMemUsedHdr*)privGetUsedHdrFromBlock(block);
        size = privGetStatSizeFromUsed(usedHdr);
        return size;
    }
}

static void mwMemFreeBlockByStrategy(_mwMemHeap* heap, void* block) {
    switch (heap->strategy) {
    case MW_MEM_STRATEGY_HDRLESS:
        hdrlessHeapFreeBlock(heap, block);
        break;
    case MW_MEM_STRATEGY_FIXED:
        fixedBlockHeapFreeBlock(heap, block);
        break;
    default:
        if ((int)heap->strategy >= 0) {
            normHeapFreeMemFromBlock(block);
        }
        break;
    }
}

static void* mwMemAllocByStrategy(MwMemMallocRequest* request, _mwMemHeap* heap, u32 flags) {
    switch (heap->strategy) {
    case MW_MEM_STRATEGY_HDRLESS:
        return hdrlessHeapAlloc(request->size, heap, flags, request);
    case MW_MEM_STRATEGY_FIXED:
        return fixedBlockHeapAlloc(request->size, heap, flags, request);
    default:
        if ((int)heap->strategy >= 0) {
            return normHeapMallocMem(request->size, heap, flags, request);
        }
        return 0;
    }
}

static _mwMemHeap* mwMemFindHeapForPointer(void* ptr) {
    _mwMemHeap* heap;
    _mwMemHeap* child;

    heap = SystemHeap;
    if (heap != 0) {
        heap = heap->hierFirstChild;
    }
    while (heap != 0) {
        if (ptr >= (void*)heap->heapStart && ptr < (void*)heap->heapEnd) {
            child = heap->hierFirstChild;
            if (child == 0) {
                return heap;
            }
            heap = child;
        } else {
            heap = heap->hierNext;
        }
    }
    return 0;
}

static u8 mwMemAllocateHeapIndex(void) {
    u32 index;

    for (index = 0; index < 0x100; index++) {
        if (heapIndexArray[index] == 0) {
            heapIndexArray[index] = 1;
            return (u8)index;
        }
    }
    return 0;
}

static void privWipeHeap(_mwMemHeap* heap) {
    MwMemUsedHdr* usedHdr;
    MwMemUsedHdr* nextHdr;
    _mwMemHeap* sibling;
    int keepBlock;

    if (heap == 0 || !mwMemHeapHasValidMagic(heap)) {
        return;
    }

    usedHdr = (MwMemUsedHdr*)heap->usedList;
    while (usedHdr != 0) {
        keepBlock = 1;
        if (heap->hierFirstChild != 0) {
            keepBlock = 0;
            for (sibling = heap->hierFirstChild; sibling != 0; sibling = sibling->hierNext) {
                if (privGetBlockFromUsedHdr(usedHdr) == sibling) {
                    keepBlock = 1;
                    break;
                }
            }
        }
        if (keepBlock) {
            _mwMemFreeVirtual(privGetBlockFromUsedHdr(usedHdr), &stringBase0[0x16], 0x1625);
            usedHdr = (MwMemUsedHdr*)heap->usedList;
        } else {
            nextHdr = (MwMemUsedHdr*)usedHdr->next;
            usedHdr = nextHdr;
        }
    }

    mwMemResetHeapByStrategy(heap, 1);
}

static void privWipeVirtual(_mwMemHeap* virtualHeap) {
    _mwMemHeap* heap;
    MwMemUsedHdr* usedHdr;
    MwMemUsedHdr* nextHdr;
    void* block;

    if (virtualHeap == 0 || !mwMemHeapHasValidMagic(virtualHeap)) {
        return;
    }

    for (heap = HeapList; heap != 0; heap = heap->listPrev) {
        if (heap->virtAllocCount == 0) {
            continue;
        }
        usedHdr = (MwMemUsedHdr*)heap->usedList;
        while (usedHdr != 0) {
            nextHdr = (MwMemUsedHdr*)usedHdr->next;
            if (usedHdr->heapIndex == virtualHeap->heapIndex) {
                block = privGetBlockFromUsedHdr(usedHdr);
                _mwMemFreeVirtual(block, &stringBase0[0x16], 0x15CA);
                heap->virtAllocCount--;
            }
            usedHdr = nextHdr;
        }
    }
}

static void privWipeHeapHierarchy(_mwMemHeap* heap) {
    _mwMemHeap* cursor;
    _mwMemHeap* child;

    if (heap == 0 || !mwMemHeapHasValidMagic(heap)) {
        return;
    }

    if (heap->strategy == MW_MEM_STRATEGY_VIRTUAL) {
        privWipeVirtual(heap);
        return;
    }

    if (heap->hierFirstChild == 0) {
        privWipeHeap(heap);
        return;
    }

    cursor = heap;
    do {
        child = cursor->hierFirstChild;
        while (child != 0 && child->dirty == 0) {
            cursor = child;
            child = cursor->hierFirstChild;
        }
        child = cursor->hierNext;
        while (child != 0 && child->dirty == 0) {
            cursor = child;
            child = cursor->hierNext;
        }
        child = cursor->hierFirstChild;
        if (child != 0 && child->dirty == 0) {
            continue;
        }
        if (cursor->dirty == 0) {
            privWipeHeap(cursor);
        }
    } while (cursor != heap);
}

static void privFreeHeap(_mwMemHeap* heap) {
    _mwMemHeap* parent;
    _mwMemHeap* sibling;
    _mwMemHeap* prev;
    u32 zero;

    if (heap == 0) {
        return;
    }

    zero = 0;
    heapCount--;
    heapIndexArray[heap->heapIndex] = 0;

    if (!mwMemHeapHasValidMagic(heap)) {
        return;
    }

    parent = heap->hierPrev;
    if (parent != 0) {
        if (parent->hierFirstChild == heap) {
            if (heap->hierNext == 0) {
                parent->hierFirstChild = 0;
            } else {
                parent->hierFirstChild = heap->hierNext;
            }
        } else {
            for (sibling = parent->hierFirstChild; sibling != 0; sibling = sibling->hierNext) {
                if (sibling->hierNext == heap) {
                    sibling->hierNext = heap->hierNext;
                    break;
                }
            }
        }
    }

    if (heap->listNext == 0 && heap->listPrev == 0) {
        HeapList = 0;
        OSFreeToHeap(GameCubeSystemHeap, heap);
    } else if (heap->listNext == 0) {
        if (heap->listPrev != 0) {
            HeapList = heap->listPrev;
            heap->listPrev->listNext = 0;
        }
    } else if (heap->listPrev == 0) {
        heap->listNext->listPrev = 0;
    } else {
        prev = heap->listPrev;
        prev->listNext = heap->listNext;
        heap->listNext->listPrev = prev;
    }

    heap->magic = MW_MEM_HEAP_MAGIC_FREED;
    _mwMemFreeVirtual(heap, &stringBase0[0x16], 0x14F2);
}

static void privFreeVirtual(_mwMemHeap* heap) {
    privWipeVirtual(heap);
    privFreeHeap(heap);
}

static void privFreeHeapHierarchy(_mwMemHeap* heap) {
    _mwMemHeap* cursor;
    _mwMemHeap* next;

    if (heap == 0 || !mwMemHeapHasValidMagic(heap)) {
        return;
    }

    if (heap->strategy == MW_MEM_STRATEGY_VIRTUAL) {
        privFreeVirtual(heap);
        return;
    }

    if (heap->hierFirstChild == 0) {
        privFreeHeap(heap);
        return;
    }

    cursor = heap;
    do {
        while (cursor->hierFirstChild != 0) {
            cursor = cursor->hierFirstChild;
        }
        while (cursor->hierNext != 0) {
            cursor = cursor->hierNext;
        }
        next = cursor;
        privFreeHeap(cursor);
        cursor = next;
    } while (cursor != heap);
}

static void privAddHeapToHeapList(_mwMemHeap* heap, _mwMemHeap* parent) {
    _mwMemHeap* head;
    _mwMemHeap* system;
    u32 zero;

    zero = 0;
    head = HeapList;
    if (head == 0) {
        heap->listPrev = 0;
        heap->listNext = 0;
        heap->hierPrev = 0;
        heap->hierFirstChild = 0;
        heap->hierNext = 0;
        HeapList = heap;
        return;
    }

    heap->listPrev = head;
    heap->listNext = 0;
    head->listNext = heap;
    HeapList = heap;

    if (parent == 0) {
        system = SystemHeap;
        if (system->hierNext == 0) {
            heap->hierFirstChild = 0;
            heap->hierPrev = 0;
            heap->hierNext = 0;
            system->hierNext = heap;
        } else {
            heap->hierFirstChild = 0;
            heap->hierPrev = 0;
            heap->hierNext = system->hierNext;
            system->hierNext = heap;
        }
        return;
    }

    if (parent->hierFirstChild == 0) {
        parent->hierFirstChild = heap;
        heap->hierPrev = parent;
        heap->hierFirstChild = 0;
        heap->hierNext = 0;
    } else {
        heap->hierPrev = parent;
        heap->hierFirstChild = 0;
        heap->hierNext = parent->hierFirstChild;
        parent->hierFirstChild = heap;
    }
}

static int privInitSystemHeap(u32 arenaSize, u8* buffer, u32 strategyType,
                              _mwMemHeap** outHeap, const char* name) {
    _mwMemHeap* heap;
    MwMemHeapParams defaultParams;

    heap = (_mwMemHeap*)buffer;
    heap->currentFreeSize = 0;
    heap->blockSize = 0;
    heap->flags = 0;
    heap->arenaSize = arenaSize;
    heap->dirty = 0;

    if (arenaSize != 0) {
        heap->heapStart = buffer + 0x80;
        heap->heapEnd = heap->heapStart + arenaSize;
        heap->magic = MW_MEM_HEAP_MAGIC_VALID;
        heap->heapIndex = mwMemAllocateHeapIndex();
        heapCount++;
        heap->name = name;
        heap->overflowFlag = 0;
        heap->strategy = strategyType;
        heap->strategyCallback = 0;
        heap->peakUsedSize = 0;
        heap->peakAllocationCount = 0;
        privAddHeapToHeapList(heap, 0);
    }

    mwMemResetHeapByStrategy(heap, 0);
    *outHeap = heap;

    mwMemHeapGetDefaultParams(&defaultParams);
    mwMemHeapSetParams(heap, &defaultParams);

    if (mwMemSystemOverflowHeap == 0) {
        mwMemSystemOverflowHeap = heap;
    }

    return 1;
}

void mwMemHeapGetMaxFreeBlock(_mwMemHeap* heap, u32* outCount, u32* outSize) {
    MwMemUsedHeader* freeNode;
    u32 maxSize;
    u32 count;

    switch (heap->strategy) {
    case MW_MEM_STRATEGY_HDRLESS:
        *outCount = heap->currentFreeSize / (heap->blockSize + heap->blockPrefixSize);
        if (*outCount == 0) {
            *outSize = 0;
        } else {
            *outSize = heap->blockSize;
        }
        return;
    case MW_MEM_STRATEGY_FIXED:
        *outCount = heap->currentFreeSize / (heap->blockSize + heap->blockPrefixSize + 0x10);
        if (*outCount == 0) {
            *outSize = 0;
        } else {
            *outSize = heap->blockSize;
        }
        return;
    default:
        if ((int)heap->strategy < 0) {
            *outCount = 0;
            *outSize = 0;
            return;
        }
        freeNode = heap->freeList;
        maxSize = 0;
        count = 0;
        while (freeNode != 0) {
            if (freeNode->allocationSize > maxSize) {
                maxSize = freeNode->allocationSize;
            }
            freeNode = freeNode->next;
            count++;
        }
        *outCount = count;
        *outSize = maxSize;
        return;
    }
}

void* mwMemHeapStrategyCallback(MwMemMallocRequest* request, _mwMemHeap* heap, u32 flags,
                                void* context) {
    void* result;

    (void)context;
    (void)flags;

    switch (heap->strategy) {
    case MW_MEM_STRATEGY_FIXED:
        result = fixedBlockHeapAlloc(request->size, heap, request->flags, request);
        break;
    case MW_MEM_STRATEGY_HDRLESS:
        result = hdrlessHeapAlloc(request->size, heap, request->flags, request);
        break;
    case MW_MEM_STRATEGY_NORMAL:
        result = normHeapMallocMem(request->size, heap, request->flags, request);
        break;
    default:
        if (mwMemUserConfigAssert() != 0) {
            OSPanic(&stringBase0[0x16], 0x1060, &stringBase0[0x2E]);
        }
        result = 0;
        break;
    }

    if (result != 0 && request->heap->strategy == MW_MEM_STRATEGY_VIRTUAL) {
        request->originHeap->virtAllocCount++;
    }
    return result;
}

#pragma dont_inline on
static void _mwMemFreeVirtual(void* ptr, const char* file, u32 line) {
    _mwMemHeap* heap;
    int statSize;

    (void)file;
    (void)line;

    priv_mwMem_CritSecEnter();
    if (ptr == 0) {
        priv_mwMem_CritSecExit();
        return;
    }

    heap = mwMemFindHeapForPointer(ptr);
    if (heap == 0) {
        priv_mwMem_CritSecExit();
        return;
    }

    statSize = mwMemAllocStatSize(heap, ptr);
    privUpdateStatsRemoveMemory(heap, statSize);
    mwMemFreeBlockByStrategy(heap, ptr);
    priv_mwMem_CritSecExit();
}
#pragma dont_inline reset

static void* _mwMemMallocVirtual(MwMemMallocRequest* request) {
    _mwMemHeap* heap;
    _mwMemHeap* originHeap;
    _mwMemHeap* overflowHeap;
    void* result;
    MwMemOverflowInfo overflowInfo;
    u32 statSize;

    priv_mwMem_CritSecEnter();
    heap = request->heap;
    if (!mwMemHeapHasValidMagic(heap)) {
        priv_mwMem_CritSecExit();
        return 0;
    }

    request->field_0x00 = 0;
    request->field_0x04 = 0;
    request->field_0x0C = 0;
    request->field_0x08 = 0;
    request->field_0x18 = 0;

    if (heap->strategyCallback != 0) {
        StrategyAllocationActive = 1;
        result = ((void* (*)(MwMemMallocRequest*, _mwMemHeap*, u32, void*, void*,
                              void*))heap->strategyCallback)(request, heap, request->flags, 0, 0,
                                                            0);
        StrategyAllocationActive = 0;
    } else {
        result = mwMemAllocByStrategy(request, heap, request->flags);
    }

    if (result == 0 && heap->overflowEnable != 0) {
        memset(&overflowInfo, 0, sizeof(overflowInfo));
        overflowInfo.originHeap = request->originHeap;
        overflowHeap = mwMemSystemOverflowHeap;
        overflowInfo.destHeap = overflowHeap;
        overflowInfo.originName = request->originHeap->name;
        overflowInfo.destName = overflowHeap->name;
        overflowInfo.file = (const char*)request->field_0x08;
        overflowInfo.size = request->size;
        overflowInfo.systemParams = &systemParams;
        overflowInfo.field_0x38 = request->originHeap->field_0x68;
        overflowInfo.field_0x40 = request->field_0x24;
        overflowInfo.field_0x44 = request->field_0x28;
        overflowInfo.field_0x48 = request->field_0x20;
        mwMemUserConfigAttemptingOverflowHeapCallback(&overflowInfo);
        heap->overflowFlag = 1;
        if (overflowHeap != 0 && overflowHeap->magic == MW_MEM_HEAP_MAGIC_VALID) {
            result = normHeapMallocMem(request->size, overflowHeap, request->flags, request);
        }
    }

    if (result != 0) {
        originHeap = request->originHeap;
        statSize = mwMemAllocStatSize(originHeap, result);
        privUpdateStatsAddMemory(originHeap, statSize);
    }

    priv_mwMem_CritSecExit();
    return result;
}

void _mwMemFree(void* ptr, int a, int b) {
    _mwMemFreeVirtual(ptr, (const char*)a, (u32)b);
}

_mwMemHeap* _mwMemHeapCreate(MwMemHeapCreateParams* create, MwMemHeapParams* defaults, u32 a,
                              u32 b) {
    _mwMemHeap* parent;
    _mwMemHeap* heap;
    void* savedCallback;
    u8 savedOverflow;
    u32 arenaSize;
    u32 allocSize;

    (void)a;
    (void)b;

    if (heapCount > 0x100 || create == 0 || create->parentHeap == 0) {
        return 0;
    }

    parent = create->parentHeap;
    if (parent->strategy != MW_MEM_STRATEGY_VIRTUAL) {
        return 0;
    }

    switch (create->strategyType) {
    case MW_MEM_STRATEGY_FIXED:
        if (create->initParams == 0) {
            return 0;
        }
        arenaSize = (u32)mwMemFixedBlockHeapGetHeapSize((FixedBlockHeapInitParams*)create->initParams);
        break;
    case MW_MEM_STRATEGY_HDRLESS:
        if (create->initParams == 0) {
            return 0;
        }
        arenaSize =
            (u32)mwMemHeaderlessFixedBlockGetHeapSize((HdrlessHeapInitParams*)create->initParams);
        break;
    default:
        if (create->arenaSize == 0) {
            return 0;
        }
        arenaSize = create->arenaSize + (create->extraSizeShift << 4);
        break;
    }

    if (arenaSize == 0) {
        return 0;
    }

    savedCallback = parent->strategyCallback;
    savedOverflow = parent->overflowEnable;
    parent->strategyCallback = 0;
    parent->overflowEnable = 0;
    allocSize = ((arenaSize + 0x8F) & ~0xF) + 0x80;
    heap = (_mwMemHeap*)_mwMemMalloc(parent, allocSize, 0x10, 0, 0, 0);
    parent->strategyCallback = savedCallback;
    parent->overflowEnable = savedOverflow;

    if (heap == 0) {
        return 0;
    }

    heap->heapStart = (u8*)heap + 0x80;
    heap->heapEnd = heap->heapStart + arenaSize;
    heap->magic = MW_MEM_HEAP_MAGIC_VALID;
    heap->heapIndex = mwMemAllocateHeapIndex();
    heapCount++;
    heap->name = create->name;
    heap->arenaSize = arenaSize;
    heap->overflowFlag = 0;
    heap->strategy = create->strategyType;
    heap->strategyCallback = 0;
    heap->peakUsedSize = 0;
    heap->peakAllocationCount = 0;
    privAddHeapToHeapList(heap, parent);
    mwMemInitHeapByStrategy(heap, create);
    mwMemHeapSetParams(heap, defaults);
    return heap;
}

void* _mwMemRealloc(void* ptr, _mwMemHeap* heap, u32 size, u32 flags, void* file, void* func,
                    void* line) {
    MwMemMallocRequest request;
    MwMemOverflowInfo oomInfo;
    _mwMemHeap* owner;
    void* newBlock;
    u32 oldSize;
    u32 copySize;

    if (ptr == 0) {
        memset(&request, 0, sizeof(request));
        request.heap = heap;
        request.originHeap = heap;
        request.flags = flags;
        request.field_0x08 = (u32)file;
        request.field_0x18 = (u32)func;
        request.field_0x20 = (u32)line;
        request.size = size;
        newBlock = _mwMemMallocVirtual(&request);
        if (newBlock == 0) {
            memset(&oomInfo, 0, sizeof(oomInfo));
            oomInfo.reason = 3;
            oomInfo.ptr = 0;
            oomInfo.size = size;
            oomInfo.destHeap = heap;
            oomInfo.file = (const char*)file;
            oomInfo.systemParams = &systemParams;
            oomInfo.field_0x38 = heap->field_0x68;
            oomInfo.field_0x40 = (u32)func;
            oomInfo.field_0x44 = (u32)line;
            mwMemUserConfigOutofMemoryCallback(&oomInfo);
        }
        return newBlock;
    }

    if (size == 0) {
        _mwMemFreeVirtual(ptr, &stringBase0[0x16], 0x878);
        return 0;
    }

    owner = mwMemFindHeapForPointer(ptr);
    if (owner->strategy == MW_MEM_STRATEGY_FIXED || owner->strategy == MW_MEM_STRATEGY_HDRLESS) {
        oldSize = owner->blockSize;
    } else {
        oldSize = (u32)privGetUserSizeFromUsed(
            (MwMemUsedHdr*)privGetUsedHdrFromBlock(ptr));
    }

    memset(&request, 0, sizeof(request));
    request.heap = heap;
    request.originHeap = heap;
    request.flags = flags;
    request.field_0x08 = (u32)file;
    request.field_0x18 = (u32)func;
    request.field_0x20 = (u32)line;
    request.size = size;
    newBlock = _mwMemMallocVirtual(&request);
    if (newBlock == 0) {
        memset(&oomInfo, 0, sizeof(oomInfo));
        oomInfo.reason = 3;
        oomInfo.ptr = ptr;
        oomInfo.size = size;
        oomInfo.destHeap = heap;
        oomInfo.file = (const char*)file;
        oomInfo.systemParams = &systemParams;
        oomInfo.field_0x38 = heap->field_0x68;
        oomInfo.field_0x40 = (u32)func;
        oomInfo.field_0x44 = (u32)line;
        mwMemUserConfigOutofMemoryCallback(&oomInfo);
        return 0;
    }

    copySize = size;
    if (copySize > oldSize) {
        copySize = oldSize;
    }
    memcpy(newBlock, ptr, copySize);
    _mwMemFreeVirtual(ptr, &stringBase0[0x16], 0x867);
    return newBlock;
}

void* _mwMemCalloc(_mwMemHeap* heap, u32 nmemb, u32 size, u32 flags, void* file, void* func,
                   void* line) {
    MwMemMallocRequest request;
    MwMemOverflowInfo oomInfo;
    u32 total;
    u32 align;
    void* result;

    total = nmemb * size;
    align = (u32)privGetAlignFromMwMemFlags(flags);
    if (align == 4) {
        total = (total + ((1U << align) - 1U)) & ~((1U << align) - 1U);
    } else {
        total = (total + (1U << align) + 0xF) & ~0xFU;
    }

    memset(&request, 0, sizeof(request));
    request.heap = heap;
    request.originHeap = heap;
    request.flags = flags;
    request.field_0x08 = (u32)file;
    request.field_0x18 = (u32)func;
    request.field_0x20 = (u32)line;
    request.size = total;
    result = _mwMemMallocVirtual(&request);
    if (result != 0) {
        memset(result, 0, total);
        return result;
    }

    memset(&oomInfo, 0, sizeof(oomInfo));
    oomInfo.reason = 2;
    oomInfo.ptr = 0;
    oomInfo.size = total;
    oomInfo.destHeap = heap;
    oomInfo.file = (const char*)file;
    oomInfo.systemParams = &systemParams;
    oomInfo.field_0x38 = heap->field_0x68;
    oomInfo.field_0x40 = (u32)func;
    oomInfo.field_0x44 = (u32)line;
    mwMemUserConfigOutofMemoryCallback(&oomInfo);
    return 0;
}

void* _mwMemMalloc(_mwMemHeap* heap, u32 size, u32 flags, void* file, void* func, void* line) {
    MwMemMallocRequest request;
    MwMemOverflowInfo oomInfo;
    void* result;

    memset(&request, 0, sizeof(request));
    request.heap = heap;
    request.originHeap = heap;
    request.flags = flags;
    request.field_0x08 = (u32)file;
    request.field_0x18 = (u32)func;
    request.field_0x20 = (u32)line;
    request.size = size;
    result = _mwMemMallocVirtual(&request);
    if (result == 0) {
        memset(&oomInfo, 0, sizeof(oomInfo));
        oomInfo.reason = 1;
        oomInfo.ptr = 0;
        oomInfo.size = size;
        oomInfo.destHeap = heap;
        oomInfo.file = (const char*)file;
        oomInfo.systemParams = &systemParams;
        oomInfo.field_0x38 = heap->field_0x68;
        oomInfo.field_0x40 = (u32)func;
        oomInfo.field_0x44 = (u32)line;
        mwMemUserConfigOutofMemoryCallback(&oomInfo);
    }
    return result;
}

void mwMemHeapGetInfo(_mwMemHeap* heap, MwMemHeapInfo* info) {
    info->name = heap->name;
    info->heapStart = heap->heapStart;
    info->heapEnd = heap->heapEnd;
    info->arenaSize = heap->arenaSize;
    info->hierPrev = heap->hierPrev;
    info->hierFirstChild = heap->hierFirstChild;
    info->hierNext = heap->hierNext;
    info->strategy = heap->strategy;
    info->overflowFlag = heap->overflowFlag;
    info->heapIndex = heap->heapIndex;
    info->field_0x28 = heap->currentUsedSize;
    info->field_0x2C = heap->peakUsedSize;
    info->field_0x30 = heap->totalManagedSize;
    info->field_0x34 = heap->currentAllocationCount;
    info->field_0x38 = heap->peakAllocationCount;
    info->totalSize = heap->currentFreeSize;
    info->blockSize = heap->blockSize;
}

int mwMemSystemGetDefaultParams(MwMemSystemParams* params) {
    params->field_0x00 = 0;
    params->field_0x04 = 0;
    return 1;
}

int mwMemSystemSetParams(MwMemSystemParams* params) {
    MwMemSystemParams defaults;

    if (params != 0) {
        systemParams.field_0x00 = params->field_0x00;
        systemParams.field_0x04 = params->field_0x04;
    } else {
        systemParams.field_0x00 = 0;
        systemParams.field_0x04 = 0;
        mwMemSystemGetDefaultParams(&defaults);
        mwMemSystemSetParams(&defaults);
    }
    return 1;
}

int mwMemHeapGetDefaultParams(MwMemHeapParams* params) {
    params->strategyCallback = 0;
    params->field_0x04 = 0;
    params->field_0x08 = 0xAB;
    params->field_0x09 = 0xDC;
    params->overflowEnable = 1;
    params->field_0x0C = 0;
    params->field_0x10 = 0;
    return 1;
}

int mwMemHeapGetParams(_mwMemHeap* heap, MwMemHeapParams* params) {
    params->strategyCallback = heap->strategyCallback;
    params->field_0x04 = heap->field_0x68;
    params->field_0x08 = heap->pad2E;
    params->field_0x09 = heap->pad2F;
    params->overflowEnable = heap->overflowEnable;
    params->field_0x0C = heap->currentUsedSize;
    params->field_0x10 = heap->peakUsedSize;
    return 1;
}

int mwMemHeapSetParams(_mwMemHeap* heap, MwMemHeapParams* params) {
    MwMemHeapParams defaults;

    if (params != 0) {
        heap->strategyCallback = params->strategyCallback;
        heap->field_0x68 = params->field_0x04;
        heap->pad2E = params->field_0x08;
        heap->pad2F = params->field_0x09;
        heap->overflowEnable = params->overflowEnable;
        heap->currentUsedSize = params->field_0x0C;
        heap->peakUsedSize = params->field_0x10;
    } else {
        mwMemHeapGetDefaultParams(&defaults);
        heap->strategyCallback = 0;
        heap->field_0x68 = 0;
        heap->pad2E = 0xAB;
        heap->pad2F = 0xDC;
        heap->overflowEnable = 1;
        heap->currentUsedSize = 0;
        heap->peakUsedSize = 0;
    }
    return 1;
}

_mwMemHeap* mwMemSystemGetHeap(u32 which) {
    return *SystemHeapTable[which];
}

int mwMemSystemSetHeap(u32 which, _mwMemHeap* heap) {
    switch (which) {
    case 0:
        return 0;
    case 1:
        mwMemSystemOverflowHeap = heap;
        return 1;
    case 2:
        newWrapperDefaultHeap = heap;
        return 1;
    default:
        return 0;
    }
}

int mwMemHeapWipe(_mwMemHeap* heap) {
    _mwMemHeap* cursor;

    /*
     * Soft ceiling (~84%): hierarchy walk in privWipeHeapHierarchy does not
     * match retail control flow / reg schedule. Readable wipe API OK;
     * leave NonMatching -- do not Matching-grind wipe helpers.
     */
    if (HeapList == 0) {
        return 1;
    }
    if (heap == 0) {
        cursor = HeapList;
        while (cursor != 0) {
            if (cursor != SystemHeap) {
                privWipeHeapHierarchy(cursor);
            }
            cursor = cursor->listPrev;
        }
    } else {
        privWipeHeapHierarchy(heap);
    }
    return 1;
}

int mwMemHeapDestroy(_mwMemHeap* heap) {
    _mwMemHeap* cursor;
    u32 zero;

    /*
     * Soft ceiling (~88%): privFreeHeapHierarchy sibling walk + list unlink
     * schedule. Leave NonMatching -- callable algo OK; Matching grind deferred.
     */
    if (HeapList == 0) {
        return 1;
    }
    if (heap == 0) {
        cursor = HeapList;
        while (cursor != 0) {
            if (cursor != SystemHeap) {
                privFreeHeapHierarchy(cursor);
            }
            cursor = cursor->listPrev;
        }
        zero = 0;
        HeapList = 0;
        return 1;
    }
    if (heap->hierPrev == 0) {
        return 0;
    }
    privFreeHeapHierarchy(heap);
    return 1;
}

u32 mwMemVirtualHeapGetHeapSize(_mwMemHeap* heap) {
    (void)heap;
    return 0x9F;
}

static int privSystemCreateFromBuffer(u8* buffer, u32 size, _mwMemHeap** outHeap,
                                      const char* name) {
    /* Soft ceiling: ~99.45% -- argument-register coloring at the init call. */
    u32 arenaSize;

    if (size == 0 || buffer == 0) {
        return 0;
    }
    arenaSize = (size - 0x81) & ~0xFU;
    return privInitSystemHeap(arenaSize, buffer, MW_MEM_STRATEGY_NORMAL, outHeap, name);
}

#pragma optimize_for_size on
static int privSystemCreateAutomated(u32 size, _mwMemHeap** outHeap, const char* name) {
    u8* buffer;
    u32 arenaSize;
    void* probe;

    privConsoleMemSystemInit();
    probe = OSAllocFromHeap(GameCubeSystemHeap, size);
    if (probe == 0) {
        return 0;
    }
    OSFreeToHeap(GameCubeSystemHeap, probe);
    arenaSize = (size - 0x81) & ~0xFU;
    buffer = (u8*)privGetOSMemory(arenaSize + 0x80);
    return privInitSystemHeap(arenaSize, buffer, 1, outHeap, name);
}
#pragma optimize_for_size reset

#pragma optimize_for_size on
int mwMemSystemCreateSystemHeap(void* buffer, u32 size, MwMemSystemParams* params) {
    /* Soft ceiling: ~99.79% -- heapName string-pool relocation labels only. */
    _mwMemHeap* heap;
    int result;

    heap = 0;
    result = 0;
    if (SystemInitialize == 0) {
        memset(heapIndexArray, 0, sizeof(heapIndexArray));
        if (buffer == 0 && size != 0) {
            result = privSystemCreateAutomated(size, &heap, heapName);
        } else {
            result = privSystemCreateFromBuffer((u8*)buffer, size, &heap, heapName);
        }
    }
    if (result != 0) {
        mwMemSystemSetParams(params);
        SystemHeap = heap;
        SystemInitialize = result;
    }
    return result;
}
#pragma optimize_for_size reset

_mwMemHeap* mwMemExtSystemHeapCreate(_mwMemHeap* parent, void* buffer, u32 size,
                                     const char* name) {
    _mwMemHeap* heap;

    (void)parent;
    privSystemCreateFromBuffer((u8*)buffer, size, &heap, name);
    return heap;
}

int mwMemSystemIsCreated(void) {
    return SystemInitialize;
}
