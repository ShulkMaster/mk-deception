#ifndef MW_MWMEM_H
#define MW_MWMEM_H

typedef unsigned char u8;
typedef unsigned long u32;

typedef struct _mwMemHeap _mwMemHeap;

/** Normal/fixed allocation header. Retail layout: 0x10 bytes. */
typedef struct MwMemUsedHeader {
  struct MwMemUsedHeader *previous; /**< Retail offset 0x00. */
  struct MwMemUsedHeader *next;     /**< Retail offset 0x04. */
  u32 allocationSize;               /**< Retail offset 0x08. */
  u8 prefixSize;                    /**< Retail offset 0x0C. */
  u8 field_0D;                      /**< Retail offset 0x0D; purpose unknown. */
  u8 flags;                         /**< Retail offset 0x0E. */
  u8 alignmentPadding;              /**< Retail offset 0x0F; byte before user block. */
} MwMemUsedHeader;

/** Partial heap identity view. Known retail extent: 0x2D bytes. */
typedef struct MwMemHeapIdentity {
  u8 pad00[0x2C]; /**< Retail offsets 0x00-0x2B; fields unknown. */
  u8 heapIndex;   /**< Retail offset 0x2C. */
} MwMemHeapIdentity;

#define MW_MEM_HEAP_MAGIC_VALID 0xBEABBEAB
#define MW_MEM_HEAP_MAGIC_FREED 0xDDDDBDDD

#define MW_MEM_STRATEGY_NORMAL 0
#define MW_MEM_STRATEGY_VIRTUAL 1
#define MW_MEM_STRATEGY_FIXED 2
#define MW_MEM_STRATEGY_OVERFLOW 4
#define MW_MEM_STRATEGY_HDRLESS 5

/** Parameters used to create a Midway memory heap. Retail layout: 0x1C bytes. */
typedef struct MwMemHeapCreateParams {
  _mwMemHeap *parentHeap; /**< Retail offset 0x00. */
  u32 arenaSize;          /**< Retail offset 0x04. */
  u32 field_08;           /**< Retail offset 0x08; purpose unknown. */
  u32 strategyType;       /**< Retail offset 0x0C. */
  void *initParams;       /**< Retail offset 0x10. */
  const char *name;       /**< Retail offset 0x14. */
  u32 extraSizeShift;     /**< Retail offset 0x18. */
} MwMemHeapCreateParams;

/** Mutable heap parameters. Retail layout: 0x14 bytes. */
typedef struct MwMemHeapParams {
  void *strategyCallback; /**< Retail offset 0x00. */
  u32 field_04;           /**< Retail offset 0x04; purpose unknown. */
  u8 field_08;            /**< Retail offset 0x08; purpose unknown. */
  u8 field_09;            /**< Retail offset 0x09; purpose unknown. */
  u8 overflowEnable;      /**< Retail offset 0x0A. */
  u32 field_0C;           /**< Retail offset 0x0C; purpose unknown. */
  u32 field_10;           /**< Retail offset 0x10; purpose unknown. */
} MwMemHeapParams;

/** Memory-system configuration words. Retail layout: 0x08 bytes. */
typedef struct MwMemSystemParams {
  u32 field_00; /**< Retail offset 0x00; purpose unknown. */
  u32 field_04; /**< Retail offset 0x04; purpose unknown. */
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
  u32 field_28;              /**< Retail offset 0x28; returned by `mslMainRamUsed`. */
  u32 field_2C;              /**< Retail offset 0x2C; purpose unknown. */
  u32 field_30;              /**< Retail offset 0x30; purpose unknown. */
  u32 field_34;              /**< Retail offset 0x34; purpose unknown. */
  u32 field_38;              /**< Retail offset 0x38; purpose unknown. */
  u32 totalSize;             /**< Retail offset 0x3C. */
  u32 blockSize;             /**< Retail offset 0x40. */
} MwMemHeapInfo;

/** Internal allocation request passed to heap strategies. Retail layout: 0x40 bytes. */
typedef struct MwMemMallocRequest {
  u32 field_00;          /**< Retail offset 0x00; purpose unknown. */
  u32 field_04;          /**< Retail offset 0x04; purpose unknown. */
  u32 field_08;          /**< Retail offset 0x08; purpose unknown. */
  u8 field_0C;           /**< Retail offset 0x0C; purpose unknown. */
  u8 pad0D[3];           /**< Retail offsets 0x0D-0x0F; alignment padding. */
  _mwMemHeap *originHeap; /**< Retail offset 0x10. */
  _mwMemHeap *heap;      /**< Retail offset 0x14. */
  u32 field_18;          /**< Retail offset 0x18; purpose unknown. */
  u32 size;              /**< Retail offset 0x1C. */
  u32 field_20;          /**< Retail offset 0x20; purpose unknown. */
  u32 field_24;          /**< Retail offset 0x24; purpose unknown. */
  u32 field_28;          /**< Retail offset 0x28; purpose unknown. */
  u32 flags;             /**< Retail offset 0x2C. */
  u32 field_30;          /**< Retail offset 0x30; purpose unknown. */
  void *systemParams;    /**< Retail offset 0x34. */
  u32 field_38;          /**< Retail offset 0x38; purpose unknown. */
  u32 field_3C;          /**< Retail offset 0x3C; purpose unknown. */
} MwMemMallocRequest;

/** Headerless fixed-block heap creation parameters. Retail layout: 0x10 bytes. */
typedef struct MwMemHeaderlessParams {
  u32 field_00;   /**< Retail offset 0x00; purpose unknown. */
  u32 blockCount; /**< Retail offset 0x04. */
  u32 blockSize;  /**< Retail offset 0x08. */
  u32 flags;      /**< Retail offset 0x0C. */
} MwMemHeaderlessParams;

/** Fixed-block heap creation parameters. Retail layout: 0x14 bytes. */
typedef struct MwMemFixedParams {
  u32 field_00;      /**< Retail offset 0x00; purpose unknown. */
  u32 blockCount;    /**< Retail offset 0x04. */
  u32 blockSize;     /**< Retail offset 0x08. */
  u32 sizeThreshold; /**< Retail offset 0x0C. */
  u32 flags;         /**< Retail offset 0x10. */
} MwMemFixedParams;

/**
 * Core Midway heap object. Retail layout: 0x7C bytes.
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
  u32 strategy;               /**< Retail offset 0x14. */
  void *strategyCallback;     /**< Retail offset 0x18. */
  u32 magic;                  /**< Retail offset 0x1C. */
  _mwMemHeap *hierPrev;       /**< Retail offset 0x20. */
  _mwMemHeap *hierFirstChild; /**< Retail offset 0x24. */
  _mwMemHeap *hierNext;       /**< Retail offset 0x28. */
  u8 heapIndex;               /**< Retail offset 0x2C. */
  u8 overflowFlag;            /**< Retail offset 0x2D. */
  u8 pad2E;                   /**< Retail offset 0x2E; unknown/padding. */
  u8 pad2F;                   /**< Retail offset 0x2F; unknown/padding. */
  const char *name;           /**< Retail offset 0x30. */
  u32 arenaSize;              /**< Retail offset 0x34. */
  u8 *heapStart;              /**< Retail offset 0x38. */
  u8 *heapEnd;                /**< Retail offset 0x3C. */
  u8 pad40[0x08];             /**< Retail offsets 0x40-0x47; fields unknown. */
  u32 currentUsedSize;        /**< Retail offset 0x48. */
  u32 peakUsedSize;           /**< Retail offset 0x4C. */
  u32 totalManagedSize;       /**< Retail offset 0x50; heapEnd - heapStart. */
  u32 currentAllocationCount; /**< Retail offset 0x54. */
  u32 peakAllocationCount;    /**< Retail offset 0x58. */
  u32 currentFreeSize;        /**< Retail offset 0x5C. */
  u32 field_60;               /**< Retail offset 0x60; purpose unknown. */
  u32 blockSize;              /**< Retail offset 0x64. */
  u32 field_68;               /**< Retail offset 0x68; purpose unknown. */
  u8 overflowEnable;          /**< Retail offset 0x6C. */
  u8 dirty;                   /**< Retail offset 0x6D. */
  u8 pad6E[2];                /**< Retail offsets 0x6E-0x6F; alignment padding. */
  u32 virtAllocCount;         /**< Retail offset 0x70. */
  u32 flags;                  /**< Retail offset 0x74. */
  u8 arenaAlignmentPadding;   /**< Retail offset 0x78. */
  u8 blockPrefixSize;         /**< Retail offset 0x79. */
};

extern _mwMemHeap *HeapList;
extern _mwMemHeap *SystemHeap;
extern _mwMemHeap *mwMemSystemOverflowHeap;
extern u32 heapCount;

void *_mwMemMalloc(_mwMemHeap *heap, u32 size, u32 flags, void *file,
                   void *func, void *line);

void _mwMemFree(void *ptr, int a, int b);

void *_mwMemRealloc(void *ptr, _mwMemHeap *heap, u32 size, u32 flags,
                    void *file, void *func, void *line);

void *_mwMemCalloc(_mwMemHeap *heap, u32 nmemb, u32 size, u32 flags, void *file,
                   void *func, void *line);

_mwMemHeap *_mwMemHeapCreate(MwMemHeapCreateParams *create,
                             MwMemHeapParams *defaults, u32 a, u32 b);

void mwMemHeapGetMaxFreeBlock(_mwMemHeap *heap, u32 *outCount, u32 *outSize);

void *mwMemHeapStrategyCallback(MwMemMallocRequest *request, _mwMemHeap *heap,
                                u32 flags, void *context);

void mwMemHeapGetInfo(_mwMemHeap *heap, MwMemHeapInfo *info);

int mwMemSystemGetDefaultParams(MwMemSystemParams *params);

int mwMemSystemSetParams(MwMemSystemParams *params);

int mwMemHeapGetDefaultParams(MwMemHeapParams *params);

int mwMemHeapGetParams(_mwMemHeap *heap, MwMemHeapParams *params);

int mwMemHeapSetParams(_mwMemHeap *heap, MwMemHeapParams *params);

_mwMemHeap *mwMemSystemGetHeap(u32 which);
int mwMemSystemSetHeap(u32 which, _mwMemHeap *heap);
int mwMemHeapWipe(_mwMemHeap *heap);
int mwMemHeapDestroy(_mwMemHeap *heap);
u32 mwMemVirtualHeapGetHeapSize(_mwMemHeap *heap);
int mwMemSystemCreateSystemHeap(void *buffer, u32 size,
                                MwMemSystemParams *params);
_mwMemHeap *mwMemExtSystemHeapCreate(_mwMemHeap *parent, void *buffer, u32 size,
                                     const char *name);
int mwMemSystemIsCreated(void);

#endif
