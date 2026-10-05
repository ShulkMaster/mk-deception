#include "mw/mwMem.h"
#include "mw/mwMemFixed.h"
#include "mw/mwMemHdrless.h"
#include "mw/mwMemHeap.h"
#include "mw/mwMemNormal.h"
#include "mw/mwMemPlatform.h"
#include "mw/mwMemPriv.h"
#include "mw/mwMem_MultiThread.h"
#include "runtime/cstring.h"
#include "dolphin/os.h"
#include "dolphin/os_alloc.h"

static const char stringBase0[] =
    "1.5 rev 1\0"
    "System Heap\0"
    "mwMem.c\0"
    "MEM_ALWAYS_FAIL\0"
    "Assertion failure: MEM_ALWAYS_FAIL";

static MwMemSystemParams systemParams;
static int SystemInitialize;
int heapCount;
static _mwMemHeap* mwMemSystemOverflowHeap;
static _mwMemHeap* newWrapperDefaultHeap;
_mwMemHeap* SystemHeap;
_mwMemHeap* HeapList;

static u8 heapIndexArray[0x100];

_mwMemHeap** SystemHeapTable[3] = {
    &SystemHeap,
    &mwMemSystemOverflowHeap,
    &newWrapperDefaultHeap,
};

static void _mwMemFreeVirtual(void* ptr, const char* file, u32 line);
static void* _mwMemMallocVirtual(MwMemMallocRequest* request);
static int privSystemCreateFromBuffer(u8* buffer, u32 size, _mwMemHeap** outHeap,
                                      const char* name);
static int privSystemCreateAutomated(u32 size, _mwMemHeap** outHeap, const char* name);

static inline void mwMemResetHeapByStrategy(_mwMemHeap* heap, int wipeMode) {
    switch (heap->strategy) {
    case MW_MEM_STRATEGY_FIXED:
        fixedBlockHeapResetHeap(heap, wipeMode);
        break;
    case MW_MEM_STRATEGY_HDRLESS:
        hdrlessHeapResetHeap(heap);
        break;
    case MW_MEM_STRATEGY_NORMAL:
    case MW_MEM_STRATEGY_VIRTUAL:
    case 3:
    case MW_MEM_STRATEGY_OVERFLOW:
        normHeapResetHeap(heap, wipeMode);
        break;
    case MW_MEM_STRATEGY_FORCE_32BIT:
    default:
        break;
    }
}

static inline void mwMemInitHeapByStrategy(_mwMemHeap* heap, u32 strategy,
                                           MwMemHeapCreateParams* create) {
    switch (strategy) {
    case MW_MEM_STRATEGY_FIXED:
        fixedBlockHeapInitHeap(heap, create->fixedInitParams);
        break;
    case MW_MEM_STRATEGY_HDRLESS:
        hdrlessHeapInitHeap(heap, create->headerlessInitParams);
        break;
    case MW_MEM_STRATEGY_NORMAL:
    case MW_MEM_STRATEGY_VIRTUAL:
    case 3:
    case MW_MEM_STRATEGY_OVERFLOW:
        normHeapInitHeap(heap);
        break;
    case MW_MEM_STRATEGY_FORCE_32BIT:
    default:
        break;
    }
}

static inline int mwMemHeapHasValidMagic(_mwMemHeap* heap) {
    if (heap->magic == MW_MEM_HEAP_MAGIC_VALID) {
        return 1;
    }
    return 0;
}

static inline u32 mwMemAllocStatSize(_mwMemHeap* heap, void* block) {
    MwMemUsedHeader* usedHdr;
    u32 size;

    switch (heap->strategy) {
    case MW_MEM_STRATEGY_HDRLESS:
        return heap->blockSize + heap->blockPrefixSize;
    case MW_MEM_STRATEGY_FIXED:
        size = heap->blockSize + heap->blockPrefixSize + sizeof(MwMemUsedHeader);
        return size;
    case MW_MEM_STRATEGY_NORMAL:
    case MW_MEM_STRATEGY_VIRTUAL:
    case 3:
    case MW_MEM_STRATEGY_OVERFLOW:
        usedHdr = privGetUsedHdrFromBlock(block);
        size = privGetStatSizeFromUsed(usedHdr);
        return size;
    case MW_MEM_STRATEGY_FORCE_32BIT:
    default:
        return 0;
    }
}

static inline void privAttemptingOverflowCallBack(MwMemMallocRequest* request, void* ptr) {
    MwMemOverflowInfo info;

    info.reason = 0;
    info.ptr = ptr;
    info.originHeap = request->heap;
    info.destHeap = mwMemSystemOverflowHeap;
    info.field_0x10 = 0;
    info.size = request->size;
    info.field_0x24 = 0;
    info.field_0x28 = 0;
    info.field_0x20 = 0;
    info.field_0x1C = 0;
    info.field_0x18 = 0;
    info.systemParam = systemParams.field_0x00;
    info.heapDiagnostic = request->heap->diagnosticValue;
    info.field_0x34 = 0;
    info.sourceFunction = request->function;
    info.line = request->line;
    info.file = request->file;
    mwMemUserConfigAttemptingOverflowHeapCallback(&info);
}

static inline void privOutOfMemoryCallBack(MwMemMallocRequest* request, u32 reason, void* ptr) {
    MwMemOverflowInfo info;

    info.reason = reason;
    info.ptr = ptr;
    info.originHeap = request->heap;
    info.destHeap = request->heap;
    info.field_0x10 = 0;
    info.size = request->size;
    info.field_0x24 = 0;
    info.field_0x28 = 0;
    info.field_0x20 = 0;
    info.field_0x1C = 0;
    info.field_0x18 = 0;
    info.systemParam = systemParams.field_0x00;
    info.heapDiagnostic = request->heap->diagnosticValue;
    info.field_0x34 = 0;
    info.sourceFunction = request->function;
    info.line = request->line;
    info.file = request->file;
    mwMemUserConfigOutofMemoryCallback(&info);
}

static inline u32 privGetNewValidHeapIndex(void) {
    u32 index;

    for (index = 0; index < sizeof(heapIndexArray); index++) {
        if (heapIndexArray[index] == 0) {
            heapIndexArray[index] = 1;
            break;
        }
    }
    return index;
}

/* TODO: [near miss] 95.08%; only diagnostic string addressing remains (sdata literal vs retail @stringBase0; TU string-pool mode). */
static void privWipeHeap(_mwMemHeap* heap) {
    MwMemUsedHeader* usedHdr;
    _mwMemHeap* firstChild;
    int keepBlock;
    _mwMemHeap* sibling;
    void* block;

    if (heap != 0 && heap->magic + 0x41550000 == 0xBEAB) {
        usedHdr = heap->usedList;
        while (usedHdr != 0) {
            block = privGetBlockFromUsedHdr(usedHdr);
            keepBlock = 1;
            firstChild = heap->hierFirstChild;
            if (firstChild != 0) {
                sibling = firstChild;
                while (sibling != 0) {
                    if (block == sibling) {
                        keepBlock = 0;
                        break;
                    }
                    sibling = sibling->hierNext;
                }
            }
            if (keepBlock) {
                _mwMemFreeVirtual(block, "mwMem.c", 0x1625);
                usedHdr = heap->usedList;
            } else {
                usedHdr = usedHdr->next;
            }
        }

        switch (heap->strategy) {
        case MW_MEM_STRATEGY_FIXED:
            fixedBlockHeapResetHeap(heap, 1);
            break;
        case MW_MEM_STRATEGY_HDRLESS:
            hdrlessHeapResetHeap(heap);
            break;
        case MW_MEM_STRATEGY_NORMAL:
        case MW_MEM_STRATEGY_VIRTUAL:
        case 3:
        case MW_MEM_STRATEGY_OVERFLOW:
            normHeapResetHeap(heap, 1);
            break;
        case MW_MEM_STRATEGY_FORCE_32BIT:
        default:
            break;
        }
    }
}

#pragma dont_inline on
static void privWipeVirtual(_mwMemHeap* virtualHeap) {
    _mwMemHeap* heap;
    MwMemUsedHeader* usedHdr;
    MwMemUsedHeader* nextHdr;
    const char* strings;
    void* block;

    if (virtualHeap != 0 && virtualHeap->magic + 0x41550000 == 0xBEAB) {
        for (heap = HeapList; heap != 0; heap = heap->listPrev) {
            if (heap->virtAllocCount != 0) {
                strings = stringBase0;
                usedHdr = heap->usedList;
                while (usedHdr != 0) {
                    nextHdr = usedHdr->next;
                    if (usedHdr->heapIndex == virtualHeap->heapIndex) {
                        block = privGetBlockFromUsedHdr(usedHdr);
                        _mwMemFreeVirtual(block, strings + 0x16, 0x15CA);
                        heap->virtAllocCount--;
                    }
                    usedHdr = nextHdr;
                }
            }
        }
    }
}
#pragma dont_inline reset

#pragma dont_inline on
static void privWipeHeapHierarchy(_mwMemHeap* heap) {
    _mwMemHeap* cursor;

    if (heap != 0 && heap->magic + 0x41550000 == 0xBEAB) {
        if (heap->strategy == MW_MEM_STRATEGY_VIRTUAL) {
            privWipeVirtual(heap);
        } else if (heap->hierFirstChild == 0) {
            privWipeHeap(heap);
        } else {
            do {
                cursor = heap;
                while (cursor->hierFirstChild != 0 &&
                       cursor->hierFirstChild->dirty == 0) {
                    while (cursor->hierFirstChild != 0 &&
                           cursor->hierFirstChild->dirty == 0) {
                        cursor = cursor->hierFirstChild;
                    }
                    while (cursor->hierNext != 0 && cursor->hierNext->dirty == 0) {
                        cursor = cursor->hierNext;
                    }
                }
                if (cursor->dirty == 0) {
                    privWipeHeap(cursor);
                }
            } while (cursor != heap);
        }
    }
}
#pragma dont_inline reset

static void privFreeHeap(_mwMemHeap* heap) {
    _mwMemHeap* parent;
    _mwMemHeap* hier_next;
    _mwMemHeap* sibling;
    _mwMemHeap* next;
    _mwMemHeap* prev;

    if (heap == 0) {
        return;
    }

    heapCount--;
    heapIndexArray[heap->heapIndex] = 0;

    if (heap->magic + 0x41550000 != 0xBEAB) {
        return;
    }

    parent = heap->hierPrev;
    if (parent != 0) {
        sibling = parent->hierFirstChild;
        hier_next = heap->hierNext;
        if (sibling == heap) {
            if (hier_next == 0) {
                parent->hierFirstChild = 0;
            } else {
                parent->hierFirstChild = hier_next;
            }
        } else {
            while (sibling->hierNext != heap) {
                sibling = sibling->hierNext;
            }
            sibling->hierNext = hier_next;
        }
    }

    next = heap->listNext;
    if (next == 0 && heap->listPrev == 0) {
        HeapList = 0;
        OSFreeToHeap(GameCubeSystemHeap, heap);
    } else if (next == 0 && heap->listPrev != 0) {
        prev = heap->listPrev;
        HeapList = prev;
        prev->listNext = 0;
    } else if (next != 0 && heap->listPrev == 0) {
        next->listPrev = 0;
    } else {
        prev = heap->listPrev;
        next->listPrev = prev;
        prev->listNext = next;
    }

    heap->magic = MW_MEM_HEAP_MAGIC_FREED;
    _mwMemFreeVirtual(heap, &stringBase0[0x16], 0x14F2);
}

static void privFreeVirtual(_mwMemHeap* heap) {
    privWipeVirtual(heap);
    privFreeHeap(heap);
}

#pragma dont_inline on
static void privFreeHeapHierarchy(_mwMemHeap* heap) {
    _mwMemHeap* cursor;

    if (heap != 0 && heap->magic + 0x41550000 == 0xBEAB) {
        if (heap->strategy == MW_MEM_STRATEGY_VIRTUAL) {
            privFreeVirtual(heap);
            return;
        }

        if (heap->hierFirstChild == 0) {
            privFreeHeap(heap);
            return;
        }

        do {
            cursor = heap;
            while (cursor->hierFirstChild != 0) {
                while (cursor->hierFirstChild != 0) {
                    cursor = cursor->hierFirstChild;
                }
                while (cursor->hierNext != 0) {
                    cursor = cursor->hierNext;
                }
            }
            privFreeHeap(cursor);
        } while (cursor != heap);
    }
}
#pragma dont_inline reset

static void privAddHeapToHeapList(_mwMemHeap* heap, _mwMemHeap* parent) {
    if (HeapList == 0) {
        heap->listPrev = 0;
        heap->listNext = 0;
        heap->hierPrev = 0;
        heap->hierFirstChild = 0;
        heap->hierNext = 0;
        HeapList = heap;
        return;
    }

    heap->listPrev = HeapList;
    heap->listNext = 0;
    HeapList->listNext = heap;
    HeapList = heap;

    if (parent == 0) {
        if (SystemHeap->hierNext == 0) {
            heap->hierFirstChild = 0;
            heap->hierPrev = 0;
            heap->hierNext = 0;
            SystemHeap->hierNext = heap;
        } else {
            heap->hierFirstChild = 0;
            heap->hierNext = SystemHeap->hierNext;
            heap->hierPrev = 0;
            SystemHeap->hierNext = heap;
        }
        return;
    }

    if (parent->hierFirstChild == 0) {
        parent->hierFirstChild = heap;
        heap->hierFirstChild = 0;
        heap->hierNext = 0;
        heap->hierPrev = parent;
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

    heap = (_mwMemHeap*)(((unsigned long)buffer + 0xF) & ~0xFUL);
    heap->sizeThreshold = 0;
    heap->blockSize = 0;
    heap->flags = 0;
    heap->originalBuffer = buffer;
    heap->ownsBuffer = strategyType;

    if (heap != 0) {
        heap->heapEnd = (heap->heapStart = (u8*)heap + sizeof(*heap)) + arenaSize;
        heap->name = name;
        heap->magic = MW_MEM_HEAP_MAGIC_VALID;
        heapCount++;
        heap->heapIndex = privGetNewValidHeapIndex();
        heap->arenaSize = arenaSize;
        heap->overflowFlag = 0;
        heap->strategy = MW_MEM_STRATEGY_NORMAL;
        heap->strategyCallback = 0;
        heap->peakUsedSize = 0;
        heap->peakAllocationCount = 0;
        privAddHeapToHeapList(heap, 0);
    }

    mwMemResetHeapByStrategy(heap, 0);
    *outHeap = heap;

    defaultParams.strategyCallback = 0;
    defaultParams.field_0x04 = 0;
    defaultParams.paramByte0 = 0xAB;
    defaultParams.paramByte1 = 0xDC;
    defaultParams.overflowEnable = 1;
    defaultParams.diagnosticValue = 0;
    defaultParams.field_0x10 = 0;
    mwMemHeapSetParams(heap, &defaultParams);

    if (mwMemSystemOverflowHeap == 0) {
        mwMemSystemOverflowHeap = heap;
    }

    return 1;
}

void mwMemHeapGetMaxFreeBlock(_mwMemHeap* heap, u32* outSize, u32* outCount) {
    MwMemUsedHeader* freeNode;
    u32 maxSize;
    u32 count;

    switch (heap->strategy) {
    case MW_MEM_STRATEGY_HDRLESS: {
        u32 block_size = heap->blockSize;
        u32 block_prefix_size = heap->blockPrefixSize;

        *outCount = heap->currentFreeSize / (block_size + block_prefix_size);
        if (*outCount == 0) {
            *outSize = 0;
        } else {
            *outSize = block_size;
        }
        return;
    }
    case MW_MEM_STRATEGY_FIXED: {
        u32 block_prefix_size = heap->blockPrefixSize;
        u32 block_size = heap->blockSize;

        *outCount = heap->currentFreeSize / (block_size + block_prefix_size + sizeof(MwMemUsedHeader));
        if (*outCount == 0) {
            *outSize = 0;
        } else {
            *outSize = block_size;
        }
        return;
    }
    case MW_MEM_STRATEGY_NORMAL:
    case MW_MEM_STRATEGY_VIRTUAL:
    case 3:
    case MW_MEM_STRATEGY_OVERFLOW:
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
    case MW_MEM_STRATEGY_FORCE_32BIT:
    default:
        *outCount = 0;
        *outSize = 0;
        return;
    }
}

#pragma opt_common_subs off
void* mwMemHeapStrategyCallback(u32 size, _mwMemHeap* heap, u32 flags,
                                MwMemMallocRequest* request) {
    void* result;

    switch (heap->strategy) {
    case MW_MEM_STRATEGY_VIRTUAL:
        result = 0;
        break;
    case MW_MEM_STRATEGY_FIXED:
        result = fixedBlockHeapAlloc(size, heap, flags, request);
        break;
    case MW_MEM_STRATEGY_HDRLESS:
        result = hdrlessHeapAlloc(size, heap, flags, request);
        break;
    case MW_MEM_STRATEGY_NORMAL:
    case 3:
    case MW_MEM_STRATEGY_OVERFLOW:
        result = normHeapMallocMem(size, heap, flags, request);
        break;
    case MW_MEM_STRATEGY_FORCE_32BIT:
    default:
        if (mwMemUserConfigAssert(&stringBase0[0x1E], &stringBase0[0x16], 0x1060) != 0) {
            OSPanic(&stringBase0[0x16], 0x1060, &stringBase0[0x2E]);
        }
        result = 0;
        break;
    }

    if (result != 0 && request->heap->strategy == MW_MEM_STRATEGY_VIRTUAL) {
        request->allocationHeap->virtAllocCount++;
    }
    return result;
}
#pragma opt_common_subs reset

/* TODO: [near miss] 98.88%; only ptr/heap saved GPRs swap (r31/r30) remains. */
static void _mwMemFreeVirtual(void* ptr, const char* file, u32 line) {
    _mwMemHeap* cursor;
    _mwMemHeap* heap;
    u32 statSize;
    int strategy;

    priv_mwMem_CritSecEnter();
    if (ptr == 0) {
        priv_mwMem_CritSecExit();
        return;
    }

    heap = 0;
    cursor = *SystemHeapTable[0];
    do {
        if ((u8*)ptr >= cursor->heapStart && (u8*)ptr < cursor->heapEnd) {
            heap = cursor;
            if (cursor->hierFirstChild == 0) {
                break;
            }
            cursor = cursor->hierFirstChild;
        } else {
            cursor = cursor->hierNext;
        }
    } while (cursor != 0);

    strategy = heap->strategy;
    switch (strategy) {
    case MW_MEM_STRATEGY_HDRLESS:
        statSize = heap->blockSize + heap->blockPrefixSize;
        break;
    case MW_MEM_STRATEGY_FIXED:
        statSize = heap->blockSize + heap->blockPrefixSize + sizeof(MwMemUsedHeader);
        break;
    case MW_MEM_STRATEGY_NORMAL:
    case MW_MEM_STRATEGY_VIRTUAL:
    case 3:
    case MW_MEM_STRATEGY_OVERFLOW:
        statSize = privGetStatSizeFromUsed(privGetUsedHdrFromBlock(ptr));
        break;
    case MW_MEM_STRATEGY_FORCE_32BIT:
    default:
        statSize = 0;
        break;
    }
    privUpdateStatsRemoveMemory(heap, statSize);
    strategy = heap->strategy;
    switch (strategy) {
    case MW_MEM_STRATEGY_HDRLESS:
        hdrlessHeapFreeBlock(heap, ptr);
        break;
    case MW_MEM_STRATEGY_FIXED:
        fixedBlockHeapFreeBlock(heap, ptr);
        break;
    case MW_MEM_STRATEGY_NORMAL:
    case MW_MEM_STRATEGY_VIRTUAL:
    case 3:
    case MW_MEM_STRATEGY_OVERFLOW:
        normHeapFreeMemFromBlock(ptr);
        break;
    case MW_MEM_STRATEGY_FORCE_32BIT:
    default:
        break;
    }
    priv_mwMem_CritSecExit();
}

/* TODO: [near miss] 99.97%; overflow magic test branches ble ('> 0') vs retail beq; static-local suffix $342 vs retail $314 (TU deferred numbering). */
static void* _mwMemMallocVirtual(MwMemMallocRequest* request) {
    static u32 StrategyAllocationActive;
    void* result;
    _mwMemHeap* heap;
    u32 statSize;
    int strategy;

    priv_mwMem_CritSecEnter();
    heap = request->heap;
    if (heap->magic != MW_MEM_HEAP_MAGIC_VALID) {
        priv_mwMem_CritSecExit();
        return 0;
    }

    request->allocationSize = 0;
    request->userSize = 0;
    request->alignmentPadding = 0;
    request->allocationFlags = 0;
    request->allocationHeap = 0;
    request->prefixSize = 0;

    if (heap->strategyCallback != 0) {
        StrategyAllocationActive = 1;
        result = heap->strategyCallback(request->size, heap, request->flags, request);
        StrategyAllocationActive = 0;
    } else {
        strategy = heap->strategy;
        switch (strategy) {
        case MW_MEM_STRATEGY_FIXED:
            result = fixedBlockHeapAlloc(request->size, heap, request->flags, request);
            break;
        case MW_MEM_STRATEGY_HDRLESS:
            result = hdrlessHeapAlloc(request->size, heap, request->flags, request);
            break;
        case MW_MEM_STRATEGY_NORMAL:
        case MW_MEM_STRATEGY_VIRTUAL:
        case 3:
        case MW_MEM_STRATEGY_OVERFLOW:
            result = normHeapMallocMem(request->size, heap, request->flags, request);
            break;
        case MW_MEM_STRATEGY_FORCE_32BIT:
        default:
            result = 0;
            break;
        }
    }

    if (result == 0 && heap->overflowEnable != 0) {
        _mwMemHeap* overflowHeap;

        privAttemptingOverflowCallBack(request, 0);
        heap->overflowFlag = 1;
        overflowHeap = mwMemSystemOverflowHeap;
        if (mwMemHeapHasValidMagic(overflowHeap) > 0) {
            result = normHeapMallocMem(request->size, overflowHeap, request->flags, request);
        }
    }

    if (result != 0) {
        _mwMemHeap* originHeap = request->allocationHeap;

        statSize = mwMemAllocStatSize(originHeap, result);
        privUpdateStatsAddMemory(originHeap, statSize);
    }

    priv_mwMem_CritSecExit();
    return result;
}

void _mwMemFree(void* ptr, const char* file, u32 line) {
    _mwMemFreeVirtual(ptr, file, line);
}

/* TODO: [near miss] 99.76%; zero-store constant r5 vs retail r0 and arena temp r0 vs r4 remain (coloring). */
_mwMemHeap* _mwMemHeapCreate(MwMemHeapCreateParams* create, MwMemHeapParams* defaults,
                              const char* function, u32 line) {
    _mwMemHeap* heap;
    _mwMemHeap* parent;
    u8 savedOverflow;
    MwMemStrategyCallback savedCallback;
    u32 strategy;
    const char* name;
    u32 arenaSize;
    u32 allocSize;
    u32 extraSizeShift;

    if (heapCount > 0x100) {
        return 0;
    }
    if (create == 0) {
        return 0;
    }

    parent = create->parentHeap;
    arenaSize = create->arenaSize;
    strategy = create->strategyType;
    name = create->name;
    extraSizeShift = create->extraSizeShift;
    if (parent->strategy == MW_MEM_STRATEGY_VIRTUAL) {
        return 0;
    }

    switch (strategy) {
    case MW_MEM_STRATEGY_FIXED:
        if (create->fixedInitParams != 0) {
            arenaSize = mwMemFixedBlockHeapGetHeapSize(create->fixedInitParams);
        }
        break;
    case MW_MEM_STRATEGY_HDRLESS:
        if (create->headerlessInitParams != 0) {
            arenaSize = mwMemHeaderlessFixedBlockGetHeapSize(create->headerlessInitParams);
        }
        break;
    case MW_MEM_STRATEGY_NORMAL:
    case MW_MEM_STRATEGY_VIRTUAL:
    case 3:
    case MW_MEM_STRATEGY_OVERFLOW:
        if (arenaSize != 0) {
            arenaSize += extraSizeShift << 4;
        }
        break;
    case MW_MEM_STRATEGY_FORCE_32BIT:
    default:
        arenaSize = 0;
        break;
    }

    if (arenaSize == 0) {
        return 0;
    }

    savedCallback = parent->strategyCallback;
    parent->strategyCallback = 0;
    arenaSize -= sizeof(*heap);
    arenaSize &= ~0xFU;
    savedOverflow = parent->overflowEnable;
    parent->overflowEnable = 0;
    allocSize = arenaSize + sizeof(*heap);
    allocSize = MW_MEM_ALIGN_UP_16(allocSize);
    heap = _mwMemMalloc(parent, allocSize, 0x10, name, function, line);
    parent->strategyCallback = savedCallback;
    parent->overflowEnable = savedOverflow;

    if (heap != 0) {
        heap->heapEnd = (heap->heapStart = (u8*)heap + sizeof(*heap)) + arenaSize;
        heap->name = name;
        heap->magic = MW_MEM_HEAP_MAGIC_VALID;
        heapCount++;
        heap->heapIndex = privGetNewValidHeapIndex();
        heap->arenaSize = arenaSize;
        heap->overflowFlag = 0;
        heap->strategy = strategy;
        heap->strategyCallback = 0;
        heap->peakUsedSize = 0;
        heap->peakAllocationCount = 0;
        privAddHeapToHeapList(heap, parent);
    }
    mwMemInitHeapByStrategy(heap, strategy, create);
    mwMemHeapSetParams(heap, defaults);
    return heap;
}

/* TODO: [near miss] 98.57%; bounded-copy clamp result register (retail mr r28,r29) remains; request stores and call order agree. */
void* _mwMemRealloc(void* ptr, _mwMemHeap* heap, u32 size, u32 flags,
                    const char* file, const char* function, u32 line) {
    MwMemMallocRequest request;
    _mwMemHeap* owner;
    _mwMemHeap* cursor;
    void* newBlock;
    u32 oldSize;
    u32 copySize;

    copySize = size;
    request.file = file;
    request.function = function;
    request.line = line;
    request.size = copySize;
    request.heap = heap;
    request.flags = flags;
    privGetAlignFromMwMemFlags(flags);

    if (ptr == 0) {
        newBlock = _mwMemMallocVirtual(&request);
        if (newBlock == 0) {
            privOutOfMemoryCallBack(&request, 3, ptr);
        }
    } else if (copySize != 0) {
        owner = 0;
        cursor = *SystemHeapTable[0];
        do {
            if ((u8*)ptr >= cursor->heapStart &&
                (u8*)ptr < cursor->heapEnd) {
                owner = cursor;
                if (cursor->hierFirstChild == 0) break;
                cursor = cursor->hierFirstChild;
            } else {
                cursor = cursor->hierNext;
            }
        } while (cursor != 0);
        if (owner->strategy == MW_MEM_STRATEGY_HDRLESS ||
            owner->strategy == MW_MEM_STRATEGY_FIXED) {
            oldSize = owner->blockSize;
        } else {
            oldSize = privGetUserSizeFromUsed(
                privGetUsedHdrFromBlock(ptr));
        }

        newBlock = _mwMemMallocVirtual(&request);
        if (newBlock != 0) {
            copySize = copySize > oldSize ? oldSize : copySize;
            newBlock = memcpy(newBlock, ptr, copySize);
            _mwMemFreeVirtual(ptr, &stringBase0[0x16], 0x867);
        } else {
            privOutOfMemoryCallBack(&request, 3, ptr);
        }
    } else {
        newBlock = 0;
        _mwMemFreeVirtual(ptr, &stringBase0[0x16], 0x878);
    }
    return newBlock;
}

#pragma opt_common_subs off
void* _mwMemCalloc(_mwMemHeap* heap, u32 nmemb, u32 size, u32 flags,
                   const char* file, const char* function, u32 line) {
    MwMemMallocRequest request;
    u32 total;
    void* result;
    int align;
    u32 aligned_total;

    total = nmemb * size;
    request.heap = heap;
    request.file = file;
    request.function = function;
    request.line = line;
    request.size = total;
    request.flags = flags;
    align = privGetAlignFromMwMemFlags(flags);
    if (align == 4) {
        u32 alignment_mask = (1U << align) - 1U;
        aligned_total = (total + alignment_mask) & ~alignment_mask;
    } else {
        aligned_total = (total + (1U << align) + 0xF) & ~0xFU;
    }

    request.size = aligned_total;
    result = _mwMemMallocVirtual(&request);
    if (result != 0) {
        result = memset(result, 0, aligned_total);
    } else {
        privOutOfMemoryCallBack(&request, 2, 0);
    }
    return result;
}

void* _mwMemMalloc(_mwMemHeap* heap, u32 size, u32 flags, const char* file,
                   const char* function, u32 line) {
    MwMemMallocRequest request;
    void* result;

    request.heap = heap;
    request.file = file;
    request.function = function;
    request.line = line;
    request.size = size;
    request.flags = flags;
    result = _mwMemMallocVirtual(&request);
    if (result == 0) {
        privOutOfMemoryCallBack(&request, 1, 0);
    }
    return result;
}
#pragma opt_common_subs reset

int mwMemHeapGetInfo(const _mwMemHeap* heap, MwMemHeapInfo* info) {
    info->name = heap->name;
    info->heapStart = heap->heapStart;
    info->heapEnd = heap->heapEnd;
    info->arenaSize = heap->arenaSize;
    info->hierFirstChild = heap->hierFirstChild;
    info->hierPrev = heap->hierPrev;
    info->hierNext = heap->hierNext;
    info->strategy = heap->strategy;
    info->overflowFlag = heap->overflowFlag;
    info->heapIndex = heap->heapIndex;
    info->currentUsedSize = heap->currentUsedSize;
    info->peakUsedSize = heap->peakUsedSize;
    info->totalManagedSize = heap->totalManagedSize;
    info->currentAllocationCount = heap->currentAllocationCount;
    info->peakAllocationCount = heap->peakAllocationCount;
    info->totalSize = heap->currentFreeSize;
    info->blockSize = heap->blockSize;
    return 1;
}

int mwMemSystemGetDefaultParams(MwMemSystemParams* params) {
    params->field_0x00 = 0;
    params->field_0x04 = 0;
    return 1;
}

#pragma inline_depth(2)
int mwMemSystemSetParams(const MwMemSystemParams* params) {
    MwMemSystemParams defaults;

    if (params != 0) {
        systemParams.field_0x00 = params->field_0x00;
        systemParams.field_0x04 = params->field_0x04;
    } else {
        mwMemSystemGetDefaultParams(&defaults);
        mwMemSystemSetParams(&defaults);
    }
    return 1;
}
#pragma inline_depth reset

int mwMemHeapGetDefaultParams(MwMemHeapParams* params) {
    params->strategyCallback = 0;
    params->field_0x04 = 0;
    params->paramByte0 = 0xAB;
    params->paramByte1 = 0xDC;
    params->overflowEnable = 1;
    params->diagnosticValue = 0;
    params->field_0x10 = 0;
    return 1;
}

int mwMemHeapGetParams(const _mwMemHeap* heap, MwMemHeapParams* params) {
    params->strategyCallback = heap->strategyCallback;
    params->field_0x04 = heap->field_0x68;
    params->paramByte0 = heap->paramByte0;
    params->paramByte1 = heap->paramByte1;
    params->overflowEnable = heap->overflowEnable;
    params->diagnosticValue = heap->diagnosticValue;
    params->field_0x10 = heap->field_0x44;
    return 1;
}

#pragma inline_depth(2)
int mwMemHeapSetParams(_mwMemHeap* heap, const MwMemHeapParams* params) {
    MwMemHeapParams defaults;

    if (params != 0) {
        heap->strategyCallback = params->strategyCallback;
        heap->field_0x68 = params->field_0x04;
        heap->paramByte0 = params->paramByte0;
        heap->paramByte1 = params->paramByte1;
        heap->overflowEnable = params->overflowEnable;
        heap->diagnosticValue = params->diagnosticValue;
        heap->field_0x44 = params->field_0x10;
        return 1;
    }

    mwMemHeapGetDefaultParams(&defaults);
    mwMemHeapSetParams(heap, &defaults);
    return 1;
}
#pragma inline_depth reset

_mwMemHeap* mwMemSystemGetHeap(u32 which) {
    return *SystemHeapTable[which];
}

int mwMemSystemSetHeap(int which, _mwMemHeap* heap) {
    switch (which) {
    case 0:
        return 0;
    case 1:
        mwMemSystemOverflowHeap = heap;
        return 1;
    case 2:
        newWrapperDefaultHeap = heap;
        return 1;
    case 3:
    default:
        return 0;
    }
}

int mwMemHeapWipe(_mwMemHeap* heap) {
    _mwMemHeap* cursor;

    if (HeapList == 0) {
        return 1;
    }
    if (heap == 0) {
        cursor = HeapList;
        do {
            _mwMemHeap* previous = cursor->listPrev;
            if (cursor != SystemHeap) {
                privWipeHeapHierarchy(cursor);
            }
            cursor = previous;
        } while (cursor != 0);
    } else {
        privWipeHeapHierarchy(heap);
    }
    return 1;
}

int mwMemHeapDestroy(_mwMemHeap* heap) {
    _mwMemHeap* cursor;

    if (HeapList == 0) {
        return 1;
    }
    if (heap == 0) {
        cursor = HeapList;
        do {
            _mwMemHeap* previous = cursor->listPrev;
            if (cursor != SystemHeap) {
                privFreeHeapHierarchy(cursor);
            }
            cursor = previous;
        } while (cursor != 0);
        HeapList = 0;
        return 1;
    }
    if (heap->hierPrev == 0) {
        return 0;
    }
    privFreeHeapHierarchy(heap);
    return 1;
}

u32 mwMemVirtualHeapGetHeapSize(void) {
    return sizeof(_mwMemHeap) + 0x1F;
}

#pragma dont_inline on
static int privSystemCreateFromBuffer(u8* buffer, u32 size, _mwMemHeap** outHeap,
                                      const char* name) {
    u32 arenaSize;

    if (size == 0 || buffer == 0) {
        return 0;
    }
    arenaSize = (size - (sizeof(_mwMemHeap) + 1)) & ~0xFU;
    return privInitSystemHeap(arenaSize, buffer, MW_MEM_STRATEGY_NORMAL, outHeap, name);
}
#pragma dont_inline reset

#pragma dont_inline on
static int privSystemCreateAutomated(u32 size, _mwMemHeap** outHeap, const char* name) {
    u8* buffer;
    u32 arenaSize;
    void* probe;
    int available;

    privConsoleMemSystemInit();
    probe = OSAllocFromHeap(GameCubeSystemHeap, size);
    if (probe == 0) {
        available = 0;
    } else {
        OSFreeToHeap(GameCubeSystemHeap, probe);
        available = 1;
    }
    if (!available) {
        return 0;
    }
    arenaSize = (size - (sizeof(_mwMemHeap) + 1)) & ~0xFU;
    buffer = privGetOSMemory(arenaSize + sizeof(_mwMemHeap));
    return privInitSystemHeap(arenaSize, buffer, 1, outHeap, name);
}
#pragma dont_inline reset

int mwMemSystemCreateSystemHeap(void* buffer, u32 size, MwMemSystemParams* params) {
    static const char* const heapName = &stringBase0[10];
    _mwMemHeap* heap;
    int result;

    heap = 0;
    result = 0;
    if (SystemInitialize == 0) {
        memset(heapIndexArray, 0, sizeof(heapIndexArray));
        if (buffer == 0 && size != 0) {
            result = privSystemCreateAutomated(size, &heap, heapName);
        } else {
            result = privSystemCreateFromBuffer(buffer, size, &heap, heapName);
        }
    }
    if (result != 0) {
        mwMemSystemSetParams(params);
        SystemHeap = heap;
        SystemInitialize = result;
    }
    return result;
}

_mwMemHeap* mwMemExtSystemHeapCreate(_mwMemHeap* parent, void* buffer, u32 size,
                                     const char* name) {
    _mwMemHeap* heap;

    privSystemCreateFromBuffer(buffer, size, &heap, name);
    return heap;
}

int mwMemSystemIsCreated(void) {
    return SystemInitialize;
}
