#ifndef MW_MWMEMHEAP_H
#define MW_MWMEMHEAP_H

typedef struct _mwMemHeap _mwMemHeap;

typedef struct FixedHeapConfig {
    unsigned long mkobjSize;
    unsigned long mksobjSize;
    unsigned long mkprocSize;
    unsigned long bigstackSize;
    unsigned long tinystackSize;
    char pad14[0x08];
    unsigned long fixed16Size;
    unsigned long fixed32Size;
    unsigned long fixed64Size;
    unsigned long fixed128Size;
    unsigned long fixed512Size;
    unsigned long fixed1024Size;
} FixedHeapConfig;

typedef struct MwMemOverflowInfo {
    _mwMemHeap* originHeap;
    _mwMemHeap* destHeap;
    const char* originName;
    const char* destName;
    const char* file;
    unsigned long size;
    unsigned long line;
} MwMemOverflowInfo;

extern _mwMemHeap* overflow_heap;
extern _mwMemHeap* permanent_heap;
extern _mwMemHeap* mwfile_heap;
extern _mwMemHeap* section_heap;
extern _mwMemHeap* wave_heap;
extern _mwMemHeap* MWSOUND_HEAP;
extern _mwMemHeap* MWSOUND_TEMP_HEAP;
extern _mwMemHeap* movie_heap;
extern _mwMemHeap* mkobj_heap;
extern _mwMemHeap* mksobj_heap;
extern _mwMemHeap* konquest_objects_heap;
extern _mwMemHeap* konquest_subobjects_heap;
extern _mwMemHeap* mkproc_heap;
extern _mwMemHeap* tinystack_heap;
extern _mwMemHeap* bigstack_heap;
extern _mwMemHeap* section_table_heap;
extern _mwMemHeap* fixed_block_16_heap;
extern _mwMemHeap* fixed_block_32_heap;
extern _mwMemHeap* fixed_block_64_heap;
extern _mwMemHeap* fixed_block_128_heap;
extern _mwMemHeap* fixed_block_512_heap;
extern _mwMemHeap* fixed_block_1024_heap;

void mwMemUserConfigAttemptingOverflowHeapCallback(MwMemOverflowInfo* info);
void mwMemUserConfigOutofMemoryCallback(MwMemOverflowInfo* info);
void mwMemUserConfigInitMemSystem(void);
int mwMemUserConfigAssert(void);
void mwMemUserConfigPrintf(const char* str);
void mwMemDestroyFixedBlockHeaps(void);
void mwMemAllocateFixedBlockHeaps(FixedHeapConfig* config);

#endif
