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

/* MWCC emits .sbss in reverse declaration order. */
u32 gap_08_80510ECC_sbss;
MwMemSystemParams systemParams;
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
    case 3: /* unnamed retail normal-block strategy */
    case MW_MEM_STRATEGY_OVERFLOW:
        normHeapResetHeap(heap, wipeMode);
        break;
    default:
        break;
    }
}

static inline void mwMemInitHeapByStrategy(_mwMemHeap* heap, MwMemHeapCreateParams* create) {
    switch (heap->strategy) {
    case MW_MEM_STRATEGY_FIXED:
        fixedBlockHeapInitHeap(heap, create->fixedInitParams);
        break;
    case MW_MEM_STRATEGY_HDRLESS:
        hdrlessHeapInitHeap(heap, create->headerlessInitParams);
        break;
    case MW_MEM_STRATEGY_NORMAL:
    case MW_MEM_STRATEGY_VIRTUAL:
    case 3: /* unnamed retail normal-block strategy */
    case MW_MEM_STRATEGY_OVERFLOW:
        normHeapInitHeap(heap);
        break;
    default:
        break;
    }
}

static inline int mwMemAllocStatSize(_mwMemHeap* heap, void* block) {
    MwMemUsedHeader* usedHdr;
    int size;

    switch (heap->strategy) {
    case MW_MEM_STRATEGY_HDRLESS:
        return heap->blockSize + heap->blockPrefixSize;
    case MW_MEM_STRATEGY_FIXED:
        return heap->blockSize + (heap->blockPrefixSize + 0x10);
    case MW_MEM_STRATEGY_NORMAL:
    case MW_MEM_STRATEGY_VIRTUAL:
    case 3:
    case MW_MEM_STRATEGY_OVERFLOW:
        usedHdr = privGetUsedHdrFromBlock(block);
        size = privGetStatSizeFromUsed(usedHdr);
        return size;
    default:
        return 0;
    }
}

static void privWipeHeap(_mwMemHeap* heap) {
    MwMemUsedHeader* usedHdr;
    _mwMemHeap* firstChild;
    int keepBlock;
    _mwMemHeap* sibling;
    void* block;
    int strategy;

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

        strategy = (int)heap->strategy;
        switch (strategy) {
        case MW_MEM_STRATEGY_FIXED:
            fixedBlockHeapResetHeap(heap, 1);
            break;
        case MW_MEM_STRATEGY_HDRLESS:
            hdrlessHeapResetHeap(heap);
            break;
        case MW_MEM_STRATEGY_NORMAL:
        case MW_MEM_STRATEGY_VIRTUAL:
        case 3: /* unnamed retail normal-block strategy */
        case MW_MEM_STRATEGY_OVERFLOW:
            normHeapResetHeap(heap, 1);
            break;
        default:
            break;
        }
    }
}

/* Retail virtual-heap destruction retains this scan as a call boundary. */
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

/* Retail wiping keeps the leaf walkers as calls inside this traversal. */
#pragma dont_inline on
static void privWipeHeapHierarchy(_mwMemHeap* heap) {
    _mwMemHeap* cursor;
    _mwMemHeap* child;

    if (heap == 0 || heap->magic + 0x41550000 != 0xBEAB) {
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
#pragma dont_inline reset

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

    if (heap->magic + 0x41550000 != 0xBEAB) {
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
            sibling = parent->hierFirstChild;
            while (sibling->hierNext != heap) {
                sibling = sibling->hierNext;
            }
            sibling->hierNext = heap->hierNext;
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

/* Retail destruction calls this hierarchy walker out-of-line. */
#pragma dont_inline on
static void privFreeHeapHierarchy(_mwMemHeap* heap) {
    _mwMemHeap* cursor;

    if (heap == 0 || heap->magic + 0x41550000 != 0xBEAB) {
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
        do {
            while (cursor->hierFirstChild != 0) {
                cursor = cursor->hierFirstChild;
            }
            while (cursor->hierNext != 0) {
                cursor = cursor->hierNext;
            }
        } while (cursor->hierFirstChild != 0);
        privFreeHeap(cursor);
    } while (cursor != heap);
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
    u32 index = 0;
    u8* indexSlot;
    u32 remaining;

    heap = (_mwMemHeap*)(((unsigned long)buffer + 0xF) & ~0xFUL);
    heap->field_0x60 = 0;
    heap->blockSize = 0;
    heap->flags = 0;
    heap->originalBuffer = buffer;
    heap->ownsBuffer = strategyType;

    if (heap != 0) {
        heap->heapStart = (u8*)heap + 0x80;
        heap->heapEnd = heap->heapStart + arenaSize;
        heap->name = name;
        indexSlot = heapIndexArray;
        heap->magic = MW_MEM_HEAP_MAGIC_VALID;
        heapCount++;
        for (remaining = 0; remaining < 0x100; remaining++) {
            if (*indexSlot == 0) {
                heapIndexArray[index] = 1;
                break;
            }
            index++;
            indexSlot++;
        }
        heap->heapIndex = index;
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
    defaultParams.field_0x08 = 0xAB;
    defaultParams.field_0x09 = 0xDC;
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
    case MW_MEM_STRATEGY_NORMAL:
    case MW_MEM_STRATEGY_VIRTUAL:
    case 3: /* unnamed retail normal-block strategy */
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
    default:
        *outCount = 0;
        *outSize = 0;
        return;
    }
}

void* mwMemHeapStrategyCallback(u32 size, _mwMemHeap* heap, u32 flags,
                                MwMemMallocRequest* request) {
    void* result;

    switch (heap->strategy) {
    case MW_MEM_STRATEGY_FIXED:
        result = fixedBlockHeapAlloc(size, heap, flags, request);
        break;
    case MW_MEM_STRATEGY_HDRLESS:
        result = hdrlessHeapAlloc(size, heap, flags, request);
        break;
    case MW_MEM_STRATEGY_NORMAL:
    case 3: /* unnamed retail normal-block strategy */
    case MW_MEM_STRATEGY_OVERFLOW:
        result = normHeapMallocMem(size, heap, flags, request);
        break;
    case MW_MEM_STRATEGY_VIRTUAL:
        result = 0;
        break;
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

static void _mwMemFreeVirtual(void* ptr, const char* file, u32 line) {
    _mwMemHeap* cursor;
    _mwMemHeap* heap;
    MwMemUsedHeader* usedHdr;
    int statSize;

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
            if (cursor->hierFirstChild == 0) break;
            cursor = cursor->hierFirstChild;
        } else {
            cursor = cursor->hierNext;
        }
    } while (cursor != 0);

    switch (heap->strategy) {
    case MW_MEM_STRATEGY_HDRLESS:
        statSize = heap->blockSize + heap->blockPrefixSize;
        break;
    case MW_MEM_STRATEGY_FIXED:
        statSize = heap->blockSize + (heap->blockPrefixSize + 0x10);
        break;
    case MW_MEM_STRATEGY_NORMAL:
    case MW_MEM_STRATEGY_VIRTUAL:
    case 3:
    case MW_MEM_STRATEGY_OVERFLOW:
        usedHdr = privGetUsedHdrFromBlock(ptr);
        statSize = privGetStatSizeFromUsed(usedHdr);
        break;
    default:
        statSize = 0;
        break;
    }
    privUpdateStatsRemoveMemory(heap, statSize);
    switch (heap->strategy) {
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
    default:
        break;
    }
    priv_mwMem_CritSecExit();
}

static void* _mwMemMallocVirtual(MwMemMallocRequest* request) {
    static u32 StrategyAllocationActive;
    void* result;
    _mwMemHeap* heap;
    MwMemOverflowInfo overflowInfo;
    u32 statSize;

    priv_mwMem_CritSecEnter();
    heap = request->heap;
    if (heap->magic + 0x41550000 != 0xBEAB) {
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
        result = heap->strategyCallback(request->size, heap, request->flags, request, 0, 0);
        StrategyAllocationActive = 0;
    } else {
        switch (heap->strategy) {
        case MW_MEM_STRATEGY_FIXED:
            result = fixedBlockHeapAlloc(request->size, heap, request->flags, request);
            break;
        case MW_MEM_STRATEGY_HDRLESS:
            result = hdrlessHeapAlloc(request->size, heap, request->flags, request);
            break;
        case MW_MEM_STRATEGY_NORMAL:
        case MW_MEM_STRATEGY_VIRTUAL:
        case 3: /* unnamed retail normal-block strategy */
        case MW_MEM_STRATEGY_OVERFLOW:
            result = normHeapMallocMem(request->size, heap, request->flags, request);
            break;
        default:
            result = 0;
            break;
        }
    }

    if (result == 0 && heap->overflowEnable != 0) {
        _mwMemHeap* overflowHeap;

        overflowInfo.reason = 0;
        overflowInfo.ptr = 0;
        overflowInfo.originHeap = request->heap;
        overflowInfo.destHeap = mwMemSystemOverflowHeap;
        overflowInfo.field_0x10 = 0;
        overflowInfo.size = request->size;
        overflowInfo.field_0x24 = 0;
        overflowInfo.field_0x28 = 0;
        overflowInfo.field_0x20 = 0;
        overflowInfo.field_0x1C = 0;
        overflowInfo.field_0x18 = 0;
        overflowInfo.systemParam = systemParams.field_0x00;
        overflowInfo.heapDiagnostic = request->heap->diagnosticValue;
        overflowInfo.field_0x34 = 0;
        overflowInfo.sourceFunction = request->function;
        overflowInfo.line = request->line;
        overflowInfo.file = request->file;
        mwMemUserConfigAttemptingOverflowHeapCallback(&overflowInfo);
        heap->overflowFlag = 1;
        overflowHeap = mwMemSystemOverflowHeap;
        if (overflowHeap->magic == MW_MEM_HEAP_MAGIC_VALID) {
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

_mwMemHeap* _mwMemHeapCreate(MwMemHeapCreateParams* create, MwMemHeapParams* defaults,
                              const char* function, u32 line) {
    _mwMemHeap* parent;
    _mwMemHeap* heap;
    MwMemStrategyCallback savedCallback;
    const char* name;
    u8 savedOverflow;
    u8 index;
    u8* indexSlot;
    u32 arenaSize;
    u32 allocSize;
    u32 remaining;
    u32 strategy;

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
    case 3: /* unnamed retail normal-block strategy */
    case MW_MEM_STRATEGY_OVERFLOW:
        if (arenaSize != 0) {
            arenaSize += create->extraSizeShift << 4;
        }
        break;
    default:
        arenaSize = 0;
        break;
    }

    if (arenaSize == 0) {
        return 0;
    }

    savedCallback = parent->strategyCallback;
    parent->strategyCallback = 0;
    arenaSize = (arenaSize - 0x80) & ~0xFU;
    savedOverflow = parent->overflowEnable;
    parent->overflowEnable = 0;
    allocSize = (arenaSize + 0x8F) & ~0xFU;
    heap = (_mwMemHeap*)_mwMemMalloc(parent, allocSize, 0x10, name, function, line);
    parent->strategyCallback = savedCallback;
    parent->overflowEnable = savedOverflow;

    if (heap != 0) {
        heap->heapStart = (u8*)(heap + 1);
        heap->heapEnd = heap->heapStart + arenaSize;
        index = 0;
        heap->name = name;
        indexSlot = heapIndexArray;
        heap->magic = MW_MEM_HEAP_MAGIC_VALID;
        heapCount++;
        for (remaining = 0; remaining < 0x100; remaining++) {
            if (*indexSlot == 0) {
                heapIndexArray[index] = 1;
                break;
            }
            index++;
            indexSlot++;
        }
        heap->heapIndex = index;
        heap->arenaSize = arenaSize;
        heap->overflowFlag = 0;
        heap->strategy = strategy;
        heap->strategyCallback = 0;
        heap->peakUsedSize = 0;
        heap->peakAllocationCount = 0;
        privAddHeapToHeapList(heap, parent);
    }
    mwMemInitHeapByStrategy(heap, create);
    mwMemHeapSetParams(heap, defaults);
    return heap;
}

void* _mwMemRealloc(void* ptr, _mwMemHeap* heap, u32 size, u32 flags,
                    const char* file, const char* function, u32 line) {
    MwMemMallocRequest request;
    _mwMemHeap* owner;
    _mwMemHeap* cursor;
    void* newBlock;
    u32 oldSize;
    u32 copySize;

    copySize = size;
    request.heap = heap;
    request.flags = flags;
    request.file = file;
    request.function = function;
    request.line = line;
    request.size = copySize;
    privGetAlignFromMwMemFlags(flags);

    if (ptr == 0) {
        MwMemOverflowInfo nullOomInfo;

        newBlock = _mwMemMallocVirtual(&request);
        if (newBlock == 0) {
            nullOomInfo.reason = 3;
            nullOomInfo.ptr = ptr;
            nullOomInfo.originHeap = request.heap;
            nullOomInfo.destHeap = request.heap;
            nullOomInfo.field_0x10 = 0;
            nullOomInfo.size = request.size;
            nullOomInfo.field_0x24 = 0;
            nullOomInfo.field_0x28 = 0;
            nullOomInfo.field_0x20 = 0;
            nullOomInfo.field_0x1C = 0;
            nullOomInfo.field_0x18 = 0;
            nullOomInfo.systemParam = systemParams.field_0x00;
            nullOomInfo.heapDiagnostic = request.heap->diagnosticValue;
            nullOomInfo.field_0x34 = 0;
            nullOomInfo.sourceFunction = request.function;
            nullOomInfo.line = request.line;
            nullOomInfo.file = request.file;
            mwMemUserConfigOutofMemoryCallback(&nullOomInfo);
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
            if (copySize > oldSize) {
                copySize = oldSize;
            }
            newBlock = memcpy(newBlock, ptr, copySize);
            _mwMemFreeVirtual(ptr, &stringBase0[0x16], 0x867);
        } else {
            MwMemOverflowInfo reallocOomInfo;

            reallocOomInfo.reason = 3;
            reallocOomInfo.ptr = ptr;
            reallocOomInfo.originHeap = request.heap;
            reallocOomInfo.destHeap = request.heap;
            reallocOomInfo.field_0x10 = 0;
            reallocOomInfo.size = request.size;
            reallocOomInfo.field_0x24 = 0;
            reallocOomInfo.field_0x28 = 0;
            reallocOomInfo.field_0x20 = 0;
            reallocOomInfo.field_0x1C = 0;
            reallocOomInfo.field_0x18 = 0;
            reallocOomInfo.systemParam = systemParams.field_0x00;
            reallocOomInfo.heapDiagnostic = request.heap->diagnosticValue;
            reallocOomInfo.field_0x34 = 0;
            reallocOomInfo.sourceFunction = request.function;
            reallocOomInfo.line = request.line;
            reallocOomInfo.file = request.file;
            mwMemUserConfigOutofMemoryCallback(&reallocOomInfo);
        }
    } else {
        newBlock = 0;
        _mwMemFreeVirtual(ptr, &stringBase0[0x16], 0x878);
    }
    return newBlock;
}

void* _mwMemCalloc(_mwMemHeap* heap, u32 nmemb, u32 size, u32 flags,
                   const char* file, const char* function, u32 line) {
    MwMemMallocRequest request;
    MwMemOverflowInfo oomInfo;
    u32 total;
    u32 align;
    void* result;

    total = nmemb * size;
    request.heap = heap;
    request.file = file;
    request.function = function;
    request.line = line;
    request.size = total;
    request.flags = flags;
    align = (u32)privGetAlignFromMwMemFlags(flags);
    if (align == 4) {
        total = (total + ((1U << align) - 1U)) & ~((1U << align) - 1U);
    } else {
        total = (total + (1U << align) + 0xF) & ~0xFU;
    }

    request.size = total;
    result = _mwMemMallocVirtual(&request);
    if (result != 0) {
        result = memset(result, 0, total);
    } else {
        oomInfo.reason = 2;
        oomInfo.ptr = 0;
        oomInfo.originHeap = request.heap;
        oomInfo.destHeap = request.heap;
        oomInfo.field_0x10 = 0;
        oomInfo.size = request.size;
        oomInfo.field_0x24 = 0;
        oomInfo.field_0x28 = 0;
        oomInfo.field_0x20 = 0;
        oomInfo.field_0x1C = 0;
        oomInfo.field_0x18 = 0;
        oomInfo.systemParam = systemParams.field_0x00;
        oomInfo.heapDiagnostic = request.heap->diagnosticValue;
        oomInfo.field_0x34 = 0;
        oomInfo.sourceFunction = request.function;
        oomInfo.line = request.line;
        oomInfo.file = request.file;
        mwMemUserConfigOutofMemoryCallback(&oomInfo);
    }
    return result;
}

void* _mwMemMalloc(_mwMemHeap* heap, u32 size, u32 flags, const char* file,
                   const char* function, u32 line) {
    MwMemMallocRequest request;
    MwMemOverflowInfo oomInfo;
    void* result;

    request.heap = heap;
    request.file = file;
    request.function = function;
    request.line = line;
    request.size = size;
    request.flags = flags;
    result = _mwMemMallocVirtual(&request);
    if (result == 0) {
        oomInfo.reason = 1;
        oomInfo.ptr = 0;
        oomInfo.originHeap = request.heap;
        oomInfo.destHeap = request.heap;
        oomInfo.field_0x10 = 0;
        oomInfo.size = request.size;
        oomInfo.field_0x18 = 0;
        oomInfo.field_0x1C = 0;
        oomInfo.field_0x20 = 0;
        oomInfo.field_0x24 = 0;
        oomInfo.field_0x28 = 0;
        oomInfo.systemParam = systemParams.field_0x00;
        oomInfo.heapDiagnostic = request.heap->diagnosticValue;
        oomInfo.field_0x34 = 0;
        oomInfo.sourceFunction = request.function;
        oomInfo.line = request.line;
        oomInfo.file = request.file;
        mwMemUserConfigOutofMemoryCallback(&oomInfo);
    }
    return result;
}

int mwMemHeapGetInfo(_mwMemHeap* heap, MwMemHeapInfo* info) {
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
int mwMemSystemSetParams(MwMemSystemParams* params) {
    MwMemSystemParams defaults;

    if (params != 0) {
        u32 field_0x04 = params->field_0x04;
        u32 field_0x00 = params->field_0x00;

        systemParams.field_0x00 = field_0x00;
        systemParams.field_0x04 = field_0x04;
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
    params->field_0x08 = 0xAB;
    params->field_0x09 = 0xDC;
    params->overflowEnable = 1;
    params->diagnosticValue = 0;
    params->field_0x10 = 0;
    return 1;
}

int mwMemHeapGetParams(_mwMemHeap* heap, MwMemHeapParams* params) {
    params->strategyCallback = heap->strategyCallback;
    params->field_0x04 = heap->field_0x68;
    params->field_0x08 = heap->pad2E;
    params->field_0x09 = heap->pad2F;
    params->overflowEnable = heap->overflowEnable;
    params->diagnosticValue = heap->diagnosticValue;
    params->field_0x10 = heap->field_0x44;
    return 1;
}

#pragma inline_depth(2)
int mwMemHeapSetParams(_mwMemHeap* heap, MwMemHeapParams* params) {
    MwMemHeapParams defaults;

    if (params != 0) {
        heap->strategyCallback = params->strategyCallback;
        heap->field_0x68 = params->field_0x04;
        heap->pad2E = params->field_0x08;
        heap->pad2F = params->field_0x09;
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

u32 mwMemVirtualHeapGetHeapSize(_mwMemHeap* heap) {
    return 0x9F;
}

/* Retail callers retain this helper boundary; TU-wide noauto changes other functions. */
#pragma dont_inline on
static int privSystemCreateFromBuffer(u8* buffer, u32 size, _mwMemHeap** outHeap,
                                      const char* name) {
    u32 arenaSize;

    if (size == 0 || buffer == 0) {
        return 0;
    }
    arenaSize = (size - 0x81) & ~0xFU;
    return privInitSystemHeap(arenaSize, buffer, MW_MEM_STRATEGY_NORMAL, outHeap, name);
}
#pragma dont_inline reset

/* Retail system creation calls this helper rather than cloning its probe path. */
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
    arenaSize = (size - 0x81) & ~0xFU;
    buffer = privGetOSMemory(arenaSize + 0x80);
    return privInitSystemHeap(arenaSize, buffer, 1, outHeap, name);
}
#pragma dont_inline reset

int mwMemSystemCreateSystemHeap(void* buffer, u32 size, MwMemSystemParams* params) {
    /* Retail differs only in stmw/lmw selection and the local heap-name label. */
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

_mwMemHeap* mwMemExtSystemHeapCreate(_mwMemHeap* parent, void* buffer, u32 size,
                                     const char* name) {
    _mwMemHeap* heap;

    privSystemCreateFromBuffer((u8*)buffer, size, &heap, name);
    return heap;
}

int mwMemSystemIsCreated(void) {
    return SystemInitialize;
}
