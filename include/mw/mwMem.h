#ifndef MW_MWMEM_H
#define MW_MWMEM_H

typedef unsigned char u8;
typedef unsigned long u32;

typedef struct _mwMemHeap _mwMemHeap;

typedef struct MwMemHeapIdentity {
  u8 pad00[0x2C];
  u8 heapIndex;
} MwMemHeapIdentity;

#define MW_MEM_HEAP_MAGIC_VALID 0xBEABBEAB
#define MW_MEM_HEAP_MAGIC_FREED 0xDDDDBDDD

#define MW_MEM_STRATEGY_NORMAL 0
#define MW_MEM_STRATEGY_VIRTUAL 1
#define MW_MEM_STRATEGY_FIXED 2
#define MW_MEM_STRATEGY_HDRLESS 5

typedef struct MwMemHeapCreateParams {
  _mwMemHeap *parentHeap;
  u32 arenaSize;
  u32 field_08;
  u32 strategyType;
  void *initParams;
  const char *name;
  u32 extraSizeShift;
} MwMemHeapCreateParams;

typedef struct MwMemHeapParams {
  void *strategyCallback;
  u32 field_04;
  u8 field_08;
  u8 field_09;
  u8 overflowEnable;
  u32 field_0C;
  u32 field_10;
} MwMemHeapParams;

typedef struct MwMemSystemParams {
  u32 field_00;
  u32 field_04;
} MwMemSystemParams;

typedef struct MwMemHeapInfo {
  u32 field_00;
  u8 *heapStart;
  u8 *heapEnd;
  u32 arenaSize;
  _mwMemHeap *hierPrev;
  _mwMemHeap *hierFirstChild;
  _mwMemHeap *hierNext;
  u32 strategy;
  u32 overflowFlag;
  u8 heapIndex;
  u32 field_28;
  u32 field_2C;
  u32 field_30;
  u32 field_34;
  u32 field_38;
  u32 totalSize;
  u32 blockSize;
} MwMemHeapInfo;

typedef struct MwMemMallocRequest {
  u32 field_00;
  u32 field_04;
  u32 field_08;
  u8 field_0C;
  u8 pad0D[3];
  _mwMemHeap *originHeap;
  _mwMemHeap *heap;
  u32 field_18;
  u32 size;
  u32 field_20;
  u32 field_24;
  u32 field_28;
  u32 flags;
  u32 field_30;
  void *systemParams;
  u32 field_38;
  u32 field_3C;
} MwMemMallocRequest;

struct _mwMemHeap {
  _mwMemHeap *listPrev;
  _mwMemHeap *listNext;
  void *usedList;
  u32 field_0C;
  u32 field_10;
  u32 strategy;
  void *strategyCallback;
  u32 magic;
  _mwMemHeap *hierPrev;
  _mwMemHeap *hierFirstChild;
  _mwMemHeap *hierNext;
  u8 heapIndex;
  u8 overflowFlag;
  u8 pad2E;
  u8 pad2F;
  const char *name;
  u32 arenaSize;
  u8 *heapStart;
  u8 *heapEnd;
  u32 field_48;
  u32 field_4C;
  u32 field_50;
  u32 field_54;
  u32 field_58;
  u32 totalSize;
  u32 field_60;
  u32 blockSize;
  u32 field_68;
  u8 overflowEnable;
  u8 dirty;
  u8 pad6E[2];
  u32 virtAllocCount;
  u32 flags;
  u8 field_78;
  u8 startPad;
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
                                u32 flags, void *a, void *b, void *c);

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
