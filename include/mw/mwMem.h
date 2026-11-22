#ifndef MW_MWMEM_H
#define MW_MWMEM_H

#include "dolphin/types.h"

typedef struct _mwMemHeap _mwMemHeap;
typedef struct MwMemFixedParams MwMemFixedParams;
typedef struct MwMemHeaderlessParams MwMemHeaderlessParams;
typedef struct MwMemMallocRequest MwMemMallocRequest;
typedef struct MwMemOverflowInfo MwMemOverflowInfo;
typedef void *(*MwMemStrategyCallback)(u32 size, _mwMemHeap *heap, u32 flags,
                                      MwMemMallocRequest *request);

typedef struct _memUsedHdr {
  struct _memUsedHdr *previous;
  struct _memUsedHdr *next;
  u32 allocationSize;
  u8 prefixSize;
  u8 heapIndex;
  u8 flags;
  u8 alignmentPadding;
} MwMemUsedHeader;

typedef struct _memFreeHdr {
  struct _memFreeHdr *previous;
  struct _memFreeHdr *next;
  u32 allocationSize;
  u8 prefixSize;
  u8 heapIndex;
  u8 flags;
  u8 alignmentPadding;
} MwMemFreeHeader;

#define MW_MEM_HEAP_MAGIC_VALID 0xBEABBEAB
#define MW_MEM_HEAP_MAGIC_FREED 0xDDDDDDDD

typedef enum MwMemStrategy {
  MW_MEM_STRATEGY_NORMAL = 0,
  MW_MEM_STRATEGY_VIRTUAL = 1,
  MW_MEM_STRATEGY_FIXED = 2,
  MW_MEM_STRATEGY_OVERFLOW = 4,
  MW_MEM_STRATEGY_HDRLESS = 5,
  MW_MEM_STRATEGY_FORCE_32BIT = 0x7FFFFFFF
} MwMemStrategy;

typedef enum mwMemFlags {
  MWMEM_DEFAULT = 0
} mwMemFlags;

typedef struct MwMemHeapCreateParams {
  _mwMemHeap *parentHeap;
  u32 arenaSize;
  u32 field_0x08;
  u32 strategyType;
  union {
    MwMemFixedParams *fixedInitParams;
    MwMemHeaderlessParams *headerlessInitParams;
  };
  const char *name;
  u32 extraSizeShift;
} MwMemHeapCreateParams;

typedef struct MwMemHeapParams {
  MwMemStrategyCallback strategyCallback;
  u32 field_0x04;
  u8 paramByte0;
  u8 paramByte1;
  u8 overflowEnable;
  u32 diagnosticValue;
  u32 field_0x10;
} MwMemHeapParams;

typedef struct MwMemSystemParams {
  u32 field_0x00;
  u32 field_0x04;
} MwMemSystemParams;

typedef struct MwMemHeapInfo {
  const char *name;
  u8 *heapStart;
  u8 *heapEnd;
  u32 arenaSize;
  _mwMemHeap *hierPrev;
  _mwMemHeap *hierFirstChild;
  _mwMemHeap *hierNext;
  u32 strategy;
  u32 overflowFlag;
  u8 heapIndex;
  u32 currentUsedSize;
  u32 peakUsedSize;
  u32 totalManagedSize;
  u32 currentAllocationCount;
  u32 peakAllocationCount;
  u32 totalSize;
  u32 blockSize;
} MwMemHeapInfo;

struct MwMemMallocRequest {
  u32 allocationSize;
  u32 userSize;
  u32 allocationFlags;
  u8 alignmentPadding;
  u8 pad0D[3];
  _mwMemHeap *allocationHeap;
  _mwMemHeap *heap;
  u32 prefixSize;
  u32 size;
  const char *file;
  const char *function;
  u32 line;
  u32 flags;
};

struct MwMemOverflowInfo {
  u32 reason;
  void *ptr;
  _mwMemHeap *originHeap;
  _mwMemHeap *destHeap;
  u32 field_0x10;
  u32 size;
  u32 field_0x18;
  u32 field_0x1C;
  u32 field_0x20;
  u32 field_0x24;
  u32 field_0x28;
  u32 systemParam;
  u32 heapDiagnostic;
  u32 field_0x34;
  const char *sourceFunction;
  u32 line;
  const char *file;
};

struct MwMemHeaderlessParams {
  u32 field_0x00;
  u32 blockCount;
  u32 blockSize;
  u32 flags;
};

struct MwMemFixedParams {
  u32 field_0x00;
  u32 blockCount;
  u32 blockSize;
  u32 sizeThreshold;
  u32 flags;
};

struct _mwMemHeap {
  _mwMemHeap *listPrev;
  _mwMemHeap *listNext;
  MwMemUsedHeader *usedList;
  MwMemFreeHeader *freeList;
  MwMemFreeHeader *freeTail;
  int strategy;
  MwMemStrategyCallback strategyCallback;
  u32 magic;
  _mwMemHeap *hierPrev;
  _mwMemHeap *hierFirstChild;
  _mwMemHeap *hierNext;
  u8 heapIndex;
  u8 overflowFlag;
  u8 paramByte0;
  u8 paramByte1;
  const char *name;
  u32 arenaSize;
  u8 *heapStart;
  u8 *heapEnd;
  u32 diagnosticValue;
  u32 field_0x44;
  u32 currentUsedSize;
  u32 peakUsedSize;
  u32 totalManagedSize;
  u32 currentAllocationCount;
  u32 peakAllocationCount;
  u32 currentFreeSize;
  u32 sizeThreshold;
  u32 blockSize;
  u32 field_0x68;
  u8 overflowEnable;
  u8 dirty;
  u8 ownsBuffer;
  u8 pad6F;
  u32 virtAllocCount;
  u32 flags;
  u8 arenaAlignmentPadding;
  u8 blockPrefixSize;
  u8 pad7A[2];
  void *originalBuffer;
};

typedef char MwMemFreeHeaderSizeCheck[
    sizeof(MwMemFreeHeader) == 0x10 ? 1 : -1];
typedef char MwMemUsedHeaderSizeCheck[
    sizeof(MwMemUsedHeader) == 0x10 ? 1 : -1];
typedef char MwMemHeapCreateParamsSizeCheck[
    sizeof(MwMemHeapCreateParams) == 0x1C ? 1 : -1];
typedef char MwMemHeapParamsSizeCheck[
    sizeof(MwMemHeapParams) == 0x14 ? 1 : -1];
typedef char MwMemMallocRequestSizeCheck[
    sizeof(MwMemMallocRequest) == 0x30 ? 1 : -1];
typedef char MwMemOverflowInfoSizeCheck[
    sizeof(MwMemOverflowInfo) == 0x44 ? 1 : -1];
typedef char MwMemHeaderlessParamsSizeCheck[
    sizeof(MwMemHeaderlessParams) == 0x10 ? 1 : -1];
typedef char MwMemFixedParamsSizeCheck[
    sizeof(MwMemFixedParams) == 0x14 ? 1 : -1];
typedef char MwMemHeapSizeCheck[sizeof(_mwMemHeap) == 0x80 ? 1 : -1];

#ifdef __cplusplus
extern "C" {
#endif

extern _mwMemHeap *HeapList;
extern _mwMemHeap *SystemHeap;
extern int heapCount;

void *_mwMemMalloc(_mwMemHeap *heap, u32 size, u32 flags,
                   const char *file, const char *function, u32 line);

void _mwMemFree(void *ptr, const char *file, u32 line);

void *_mwMemRealloc(void *ptr, _mwMemHeap *heap, u32 size, u32 flags,
                    const char *file, const char *function, u32 line);

void *_mwMemCalloc(_mwMemHeap *heap, u32 nmemb, u32 size, u32 flags,
                   const char *file, const char *function, u32 line);

_mwMemHeap *_mwMemHeapCreate(MwMemHeapCreateParams *create,
                             MwMemHeapParams *defaults,
                             const char *function, u32 line);

void mwMemHeapGetMaxFreeBlock(_mwMemHeap *heap, u32 *outSize, u32 *outCount);

void *mwMemHeapStrategyCallback(u32 size, _mwMemHeap *heap, u32 flags,
                                MwMemMallocRequest *request);

int mwMemHeapGetInfo(const _mwMemHeap *heap, MwMemHeapInfo *info);

int mwMemSystemGetDefaultParams(MwMemSystemParams *params);

int mwMemSystemSetParams(const MwMemSystemParams *params);

int mwMemHeapGetDefaultParams(MwMemHeapParams *params);

int mwMemHeapGetParams(const _mwMemHeap *heap, MwMemHeapParams *params);

int mwMemHeapSetParams(_mwMemHeap *heap, const MwMemHeapParams *params);

_mwMemHeap *mwMemSystemGetHeap(u32 which);
int mwMemSystemSetHeap(int which, _mwMemHeap *heap);
int mwMemHeapWipe(_mwMemHeap *heap);
int mwMemHeapDestroy(_mwMemHeap *heap);
u32 mwMemVirtualHeapGetHeapSize(void);
int mwMemSystemCreateSystemHeap(void *buffer, u32 size,
                                MwMemSystemParams *params);
_mwMemHeap *mwMemExtSystemHeapCreate(_mwMemHeap *parent, void *buffer, u32 size,
                                     const char *name);
int mwMemSystemIsCreated(void);

#ifdef __cplusplus
}
#endif

#endif
