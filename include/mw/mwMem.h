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

/** Normal/fixed allocation header. Retail layout: 0x10 bytes. */
typedef struct MwMemUsedHeader {
  struct MwMemUsedHeader *previous; /**< Retail offset 0x00. */
  struct MwMemUsedHeader *next;     /**< Retail offset 0x04. */
  u32 allocationSize;               /**< Retail offset 0x08. */
  u8 prefixSize;                    /**< Retail offset 0x0C. */
  u8 heapIndex;                     /**< Retail offset 0x0D; owning heap index. */
  u8 flags;                         /**< Retail offset 0x0E. */
  u8 alignmentPadding;              /**< Retail offset 0x0F; byte before user block. */
} MwMemUsedHeader;

#define MW_MEM_HEAP_MAGIC_VALID 0xBEABBEAB
#define MW_MEM_HEAP_MAGIC_FREED 0xDDDDDDDD

#define MW_MEM_STRATEGY_NORMAL 0
#define MW_MEM_STRATEGY_VIRTUAL 1
#define MW_MEM_STRATEGY_FIXED 2
#define MW_MEM_STRATEGY_OVERFLOW 4
#define MW_MEM_STRATEGY_HDRLESS 5

typedef enum mwMemFlags {
  MWMEM_DEFAULT = 0
} mwMemFlags;

/** Parameters used to create a Midway memory heap. Retail layout: 0x1C bytes. */
typedef struct MwMemHeapCreateParams {
  _mwMemHeap *parentHeap; /**< Retail offset 0x00. */
  u32 arenaSize;          /**< Retail offset 0x04. */
  u32 field_0x08;           /**< Retail offset 0x08; purpose unknown. */
  u32 strategyType;       /**< Retail offset 0x0C. */
  union {
    void *initParams;
    MwMemFixedParams *fixedInitParams;
    MwMemHeaderlessParams *headerlessInitParams;
  };                       /**< Retail offset 0x10. */
  const char *name;       /**< Retail offset 0x14. */
  u32 extraSizeShift;     /**< Retail offset 0x18. */
} MwMemHeapCreateParams;

/** Mutable heap parameters. Retail layout: 0x14 bytes. */
typedef struct MwMemHeapParams {
  MwMemStrategyCallback strategyCallback; /**< Retail offset 0x00. */
  u32 field_0x04;           /**< Retail offset 0x04; purpose unknown. */
  u8 paramByte0; /**< Retail offset 0x08; defaults to 0xAB; purpose unknown. */
  u8 paramByte1; /**< Retail offset 0x09; defaults to 0xDC; purpose unknown. */
  u8 overflowEnable;      /**< Retail offset 0x0A. */
  u32 diagnosticValue;      /**< Retail offset 0x0C; maps to heap offset 0x40. */
  u32 field_0x10;           /**< Retail offset 0x10; maps to heap offset 0x44. */
} MwMemHeapParams;

/** Memory-system configuration words. Retail layout: 0x08 bytes. */
typedef struct MwMemSystemParams {
  u32 field_0x00; /**< Retail offset 0x00; purpose unknown. */
  u32 field_0x04; /**< Retail offset 0x04; purpose unknown. */
} MwMemSystemParams;

/** Heap information populated by `mwMemHeapGetInfo`. Retail layout: 0x44 bytes. */
typedef struct MwMemHeapInfo {
  const char *name;          /**< Retail offset 0x00. */
  u8 *heapStart;             /**< Retail offset 0x04. */
  u8 *heapEnd;               /**< Retail offset 0x08. */
  u32 arenaSize;             /**< Retail offset 0x0C. */
  _mwMemHeap *hierPrev;      /**< Retail offset 0x10. */
  _mwMemHeap *hierFirstChild; /**< Retail offset 0x14. */
  _mwMemHeap *hierNext;      /**< Retail offset 0x18. */
  u32 strategy;              /**< Retail offset 0x1C. */
  u32 overflowFlag;          /**< Retail offset 0x20. */
  u8 heapIndex;              /**< Retail offset 0x24. */
  u32 currentUsedSize;       /**< Retail offset 0x28; returned by `mslMainRamUsed`. */
  u32 peakUsedSize;          /**< Retail offset 0x2C. */
  u32 totalManagedSize;      /**< Retail offset 0x30. */
  u32 currentAllocationCount; /**< Retail offset 0x34. */
  u32 peakAllocationCount;   /**< Retail offset 0x38. */
  u32 totalSize;             /**< Retail offset 0x3C. */
  u32 blockSize;             /**< Retail offset 0x40. */
} MwMemHeapInfo;

/** Internal allocation request passed to heap strategies. Retail layout: 0x30 bytes. */
struct MwMemMallocRequest {
  u32 allocationSize;      /**< Retail offset 0x00; allocator result size. */
  u32 userSize;            /**< Retail offset 0x04; aligned user size. */
  u32 allocationFlags;     /**< Retail offset 0x08; allocator result flags. */
  u8 alignmentPadding;     /**< Retail offset 0x0C; bytes before user data. */
  u8 pad0D[3];           /**< Retail offsets 0x0D-0x0F; alignment padding. */
  _mwMemHeap *allocationHeap; /**< Retail offset 0x10; allocator-selected heap. */
  _mwMemHeap *heap;      /**< Retail offset 0x14. */
  u32 prefixSize;          /**< Retail offset 0x18; allocator prefix size. */
  u32 size;              /**< Retail offset 0x1C. */
  const char *file;       /**< Retail offset 0x20; allocation source file. */
  const char *function;   /**< Retail offset 0x24; allocation source function. */
  u32 line;               /**< Retail offset 0x28; allocation source line. */
  u32 flags;             /**< Retail offset 0x2C. */
};

/** Allocation failure/overflow callback payload. Retail layout: 0x44 bytes. */
struct MwMemOverflowInfo {
  u32 reason;                    /**< Retail offset 0x00. */
  void *ptr;                     /**< Retail offset 0x04. */
  _mwMemHeap *originHeap;        /**< Retail offset 0x08. */
  _mwMemHeap *destHeap;          /**< Retail offset 0x0C. */
  u32 field_0x10;                /**< Retail offset 0x10; purpose unknown. */
  u32 size;                      /**< Retail offset 0x14. */
  u32 field_0x18;                /**< Retail offset 0x18; purpose unknown. */
  u32 field_0x1C;                /**< Retail offset 0x1C; purpose unknown. */
  u32 field_0x20;                /**< Retail offset 0x20; purpose unknown. */
  u32 field_0x24;                /**< Retail offset 0x24; purpose unknown. */
  u32 field_0x28;                /**< Retail offset 0x28; purpose unknown. */
  u32 systemParam;               /**< Retail offset 0x2C; copied system parameter. */
  u32 heapDiagnostic;            /**< Retail offset 0x30; copied from heap +0x40. */
  u32 field_0x34;                /**< Retail offset 0x34; purpose unknown. */
  const char *sourceFunction;    /**< Retail offset 0x38; allocation source function. */
  u32 line;                      /**< Retail offset 0x3C; diagnostic source line. */
  const char *file;              /**< Retail offset 0x40; allocation source file. */
};

/** Headerless fixed-block heap creation parameters. Retail layout: 0x10 bytes. */
struct MwMemHeaderlessParams {
  u32 field_0x00;   /**< Retail offset 0x00; purpose unknown. */
  u32 blockCount; /**< Retail offset 0x04. */
  u32 blockSize;  /**< Retail offset 0x08. */
  u32 flags;      /**< Retail offset 0x0C. */
};

/** Fixed-block heap creation parameters. Retail layout: 0x14 bytes. */
struct MwMemFixedParams {
  u32 field_0x00;      /**< Retail offset 0x00; purpose unknown. */
  u32 blockCount;    /**< Retail offset 0x04. */
  u32 blockSize;     /**< Retail offset 0x08. */
  u32 sizeThreshold; /**< Retail offset 0x0C. */
  u32 flags;         /**< Retail offset 0x10. */
};

/**
 * Core Midway heap header. Retail layout: 0x80 bytes.
 *
 * Member names are inferred. The documented offsets describe this recovered
 * retail layout. System-heap initialization places its arena at `heap + 0x80`.
 */
struct _mwMemHeap {
  _mwMemHeap *listPrev;       /**< Retail offset 0x00. */
  _mwMemHeap *listNext;       /**< Retail offset 0x04. */
  MwMemUsedHeader *usedList;  /**< Retail offset 0x08. */
  MwMemUsedHeader *freeList;  /**< Retail offset 0x0C. */
  MwMemUsedHeader *freeTail;  /**< Retail offset 0x10. */
  int strategy;               /**< Retail offset 0x14. */
  MwMemStrategyCallback strategyCallback; /**< Retail offset 0x18. */
  u32 magic;                  /**< Retail offset 0x1C. */
  _mwMemHeap *hierPrev;       /**< Retail offset 0x20. */
  _mwMemHeap *hierFirstChild; /**< Retail offset 0x24. */
  _mwMemHeap *hierNext;       /**< Retail offset 0x28. */
  u8 heapIndex;               /**< Retail offset 0x2C. */
  u8 overflowFlag;            /**< Retail offset 0x2D. */
  u8 paramByte0;              /**< Retail offset 0x2E; mutable heap parameter. */
  u8 paramByte1;              /**< Retail offset 0x2F; mutable heap parameter. */
  const char *name;           /**< Retail offset 0x30. */
  u32 arenaSize;              /**< Retail offset 0x34. */
  u8 *heapStart;              /**< Retail offset 0x38. */
  u8 *heapEnd;                /**< Retail offset 0x3C. */
  u32 diagnosticValue;        /**< Retail offset 0x40; copied into failure diagnostics. */
  u32 field_0x44;             /**< Retail offset 0x44; purpose unknown. */
  u32 currentUsedSize;        /**< Retail offset 0x48. */
  u32 peakUsedSize;           /**< Retail offset 0x4C. */
  u32 totalManagedSize;       /**< Retail offset 0x50; heapEnd - heapStart. */
  u32 currentAllocationCount; /**< Retail offset 0x54. */
  u32 peakAllocationCount;    /**< Retail offset 0x58. */
  u32 currentFreeSize;        /**< Retail offset 0x5C. */
  u32 sizeThreshold;          /**< Retail offset 0x60; fixed-heap size threshold. */
  u32 blockSize;              /**< Retail offset 0x64. */
  u32 field_0x68;               /**< Retail offset 0x68; purpose unknown. */
  u8 overflowEnable;          /**< Retail offset 0x6C. */
  u8 dirty;                   /**< Retail offset 0x6D. */
  u8 ownsBuffer;              /**< Retail offset 0x6E; heap releases its backing buffer. */
  u8 pad6F;                   /**< Retail offset 0x6F; alignment padding. */
  u32 virtAllocCount;         /**< Retail offset 0x70. */
  u32 flags;                  /**< Retail offset 0x74. */
  u8 arenaAlignmentPadding;   /**< Retail offset 0x78. */
  u8 blockPrefixSize;         /**< Retail offset 0x79. */
  u8 pad7A[2];                /**< Retail offsets 0x7A-0x7B. */
  void *originalBuffer;       /**< Retail offset 0x7C; unaligned backing allocation. */
};

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
extern _mwMemHeap *mwMemSystemOverflowHeap;
extern u32 heapCount;

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

int mwMemHeapGetInfo(_mwMemHeap *heap, MwMemHeapInfo *info);

int mwMemSystemGetDefaultParams(MwMemSystemParams *params);

int mwMemSystemSetParams(MwMemSystemParams *params);

int mwMemHeapGetDefaultParams(MwMemHeapParams *params);

int mwMemHeapGetParams(_mwMemHeap *heap, MwMemHeapParams *params);

int mwMemHeapSetParams(_mwMemHeap *heap, MwMemHeapParams *params);

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
