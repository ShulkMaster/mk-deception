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

#ifdef __cplusplus
extern "C" {
#endif

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
int mwMemUserConfigAssert(const char* expression, const char* file, u32 line);
void mwMemUserConfigPrintf(const char* format, ...);
void mwMemDestroyFixedBlockHeaps(void);
void mwMemAllocateFixedBlockHeaps(FixedHeapConfig* config);

#ifdef __cplusplus
}
#endif

#endif
