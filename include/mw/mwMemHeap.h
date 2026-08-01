#ifndef MW_MWMEMHEAP_H
#define MW_MWMEMHEAP_H

#include "mw/mwMem.h"

/** Fixed-block heap sizing configuration. Retail layout: 0x34 bytes. */
typedef struct FixedHeapConfig {
    unsigned long mkobjSize;    /**< Retail offset 0x00. */
    unsigned long mksobjSize;   /**< Retail offset 0x04. */
    unsigned long mkprocSize;   /**< Retail offset 0x08. */
    unsigned long bigstackSize; /**< Retail offset 0x0C. */
    unsigned long tinystackSize; /**< Retail offset 0x10. */
    char pad14[0x08]; /**< Retail offsets 0x14-0x1B; purpose unknown. */
    unsigned long fixed16Size;   /**< Retail offset 0x1C. */
    unsigned long fixed32Size;   /**< Retail offset 0x20. */
    unsigned long fixed64Size;   /**< Retail offset 0x24. */
    unsigned long fixed128Size;  /**< Retail offset 0x28. */
    unsigned long fixed512Size;  /**< Retail offset 0x2C. */
    unsigned long fixed1024Size; /**< Retail offset 0x30. */
} FixedHeapConfig;

/** Heap-overflow callback payload. Retail layout: 0x40 bytes. */
typedef struct MwMemOverflowInfo {
    unsigned long field_00; /**< Retail offset 0x00; purpose unknown. */
    unsigned long field_04; /**< Retail offset 0x04; purpose unknown. */
    _mwMemHeap* originHeap;  /**< Retail offset 0x08. */
    _mwMemHeap* destHeap;    /**< Retail offset 0x0C. */
    unsigned long field_10; /**< Retail offset 0x10; purpose unknown. */
    unsigned long size;     /**< Retail offset 0x14. */
    unsigned char pad18[0x20]; /**< Retail offsets 0x18-0x37. */
    const char* file;       /**< Retail offset 0x38. */
    unsigned long line;     /**< Retail offset 0x3C. */
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
void mwMemUserConfigPrintf(const char* format, ...);
void mwMemDestroyFixedBlockHeaps(void);
void mwMemAllocateFixedBlockHeaps(FixedHeapConfig* config);

#endif
