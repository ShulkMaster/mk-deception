#include "mw/mwMemHeap.h"

extern void MEMPRINT(const char* format, ...);
extern void OSReport(const char* format, ...);
extern u32 mwMemSystemGetAvailSize(void);
extern void memDebugHeap(_mwMemHeap* heap);
extern u32 mwMemFixedBlockHeapGetHeapSize(u32 block_size, u32 block_count);
extern u32 mwMemHeaderlessFixedBlockGetHeapSize(u32 block_size, u32 block_count);

_mwMemHeap* overflow_heap;
_mwMemHeap* permanent_heap;
_mwMemHeap* mwfile_heap;
_mwMemHeap* section_heap;
_mwMemHeap* wave_heap;
_mwMemHeap* MWSOUND_HEAP;
_mwMemHeap* MWSOUND_TEMP_HEAP;
_mwMemHeap* movie_heap;
_mwMemHeap* mkobj_heap;
_mwMemHeap* mksobj_heap;
_mwMemHeap* konquest_objects_heap;
_mwMemHeap* konquest_subobjects_heap;
_mwMemHeap* mkproc_heap;
_mwMemHeap* tinystack_heap;
_mwMemHeap* bigstack_heap;
_mwMemHeap* section_table_heap;
_mwMemHeap* fixed_block_16_heap;
_mwMemHeap* fixed_block_32_heap;
_mwMemHeap* fixed_block_64_heap;
_mwMemHeap* fixed_block_128_heap;
_mwMemHeap* fixed_block_512_heap;
_mwMemHeap* fixed_block_1024_heap;

static void mwMemHeapInit(void);

void mwMemUserConfigAttemptingOverflowHeapCallback(MwMemOverflowInfo* info) {
    MwMemHeapInfo origin_info;
    MwMemHeapInfo destination_info;
    float size_kb;

    mwMemHeapGetInfo(info->originHeap, &origin_info);
    mwMemHeapGetInfo(info->destHeap, &destination_info);
    size_kb = (float)info->size;
    size_kb *= 1.0f / 1024.0f;
    MEMPRINT(">> OVERFLOW_HEAP: size: %f K origin heap: %s, dest heap: %s, file: %s L: %d\n",
             size_kb, origin_info.name, destination_info.name, info->file, info->line);
}

void mwMemUserConfigOutofMemoryCallback(MwMemOverflowInfo* info) {
    MwMemHeapInfo heap_info;
    float size_kb;

    mwMemHeapGetInfo(info->destHeap, &heap_info);
    MEMPRINT(">> Out of RAM \n");
    size_kb = (float)info->size;
    size_kb *= 1.0f / 1024.0f;
    MEMPRINT("      FAILURE:  cannot allocate: %f K  from heap: %s\n",
             size_kb, heap_info.name);
}

void mwMemUserConfigInitMemSystem(void) {
    u32 available = mwMemSystemGetAvailSize();
    OSReport("Free memory size: %0.2f mb\n", (float)available / 1048576.0f);
    mwMemSystemCreateSystemHeap(0, available, 0);
    mwMemHeapInit();
}

int mwMemUserConfigAssert(void) { return 1; }

void mwMemUserConfigPrintf(const char* format, ...) {}

static void* movie_strategy(MwMemMallocRequest* request, _mwMemHeap* source, u32 flags,
                            void* context) {
    void* block;
    _mwMemHeap* system_overflow;

    system_overflow = mwMemSystemGetHeap(1);
    block = mwMemHeapStrategyCallback(request, wave_heap, flags, context);
    if (block == 0) {
        block = mwMemHeapStrategyCallback(request, permanent_heap, flags, context);
        if (block == 0) {
            block = mwMemHeapStrategyCallback(request, section_heap, flags, context);
            if (block == 0) {
                block = mwMemHeapStrategyCallback(request, system_overflow, flags, context);
            }
        }
    }
    if (block == 0) {
        memDebugHeap(wave_heap);
        memDebugHeap(permanent_heap);
        memDebugHeap(system_overflow);
    }
    return block;
}

static void* fixed1024_strategy(MwMemMallocRequest* request, _mwMemHeap* source, u32 flags,
                                void* context) {
    void* block = mwMemHeapStrategyCallback(request, fixed_block_1024_heap, flags, context);
    if (block == 0) block = mwMemHeapStrategyCallback(request, wave_heap, flags, context);
    return block;
}

#define DEFINE_FIXED_STRATEGY(name, own_heap, next_heap, message)                              \
    static void* name(MwMemMallocRequest* request, _mwMemHeap* source, u32 flags,              \
                      void* context) {                                                         \
        void* block = mwMemHeapStrategyCallback(request, own_heap, flags, context);             \
        if (block == 0) {                                                                       \
            MEMPRINT(message);                                                                  \
            block = mwMemHeapStrategyCallback(request, next_heap, flags, context);              \
            if (block == 0) block = mwMemHeapStrategyCallback(request, wave_heap, flags, context); \
        }                                                                                       \
        return block;                                                                           \
    }

DEFINE_FIXED_STRATEGY(fixed512_strategy, fixed_block_512_heap, fixed_block_1024_heap,
                      "overflowing 512 word fixed block heap into 1024 word heap\n")
DEFINE_FIXED_STRATEGY(fixed128_strategy, fixed_block_128_heap, fixed_block_512_heap,
                      "overflowing 128 word fixed block heap into 512 word heap\n")
DEFINE_FIXED_STRATEGY(fixed64_strategy, fixed_block_64_heap, fixed_block_128_heap,
                      "overflowing 64 word fixed block heap into 128 word heap\n")
DEFINE_FIXED_STRATEGY(fixed32_strategy, fixed_block_32_heap, fixed_block_64_heap,
                      "overflowing 32 word fixed block heap into 64 word heap\n")
DEFINE_FIXED_STRATEGY(fixed16_strategy, fixed_block_16_heap, fixed_block_32_heap,
                      "overflowing 16 word fixed block heap into 32 word heap\n")

void mwMemDestroyFixedBlockHeaps(void) {
    if (mkobj_heap != 0) mwMemHeapDestroy(mkobj_heap);
    mkobj_heap = 0;
    if (mksobj_heap != 0) mwMemHeapDestroy(mksobj_heap);
    mksobj_heap = 0;
    if (konquest_objects_heap != 0) mwMemHeapDestroy(konquest_objects_heap);
    konquest_objects_heap = 0;
    if (konquest_subobjects_heap != 0) mwMemHeapDestroy(konquest_subobjects_heap);
    konquest_subobjects_heap = 0;
    if (mkproc_heap != 0) mwMemHeapDestroy(mkproc_heap);
    mkproc_heap = 0;
    if (bigstack_heap != 0) mwMemHeapDestroy(bigstack_heap);
    bigstack_heap = 0;
    if (tinystack_heap != 0) mwMemHeapDestroy(tinystack_heap);
    tinystack_heap = 0;
    if (fixed_block_16_heap != 0) mwMemHeapDestroy(fixed_block_16_heap);
    fixed_block_16_heap = 0;
    if (fixed_block_32_heap != 0) mwMemHeapDestroy(fixed_block_32_heap);
    fixed_block_32_heap = 0;
    if (fixed_block_64_heap != 0) mwMemHeapDestroy(fixed_block_64_heap);
    fixed_block_64_heap = 0;
    if (fixed_block_128_heap != 0) mwMemHeapDestroy(fixed_block_128_heap);
    fixed_block_128_heap = 0;
    if (fixed_block_512_heap != 0) mwMemHeapDestroy(fixed_block_512_heap);
    fixed_block_512_heap = 0;
    if (fixed_block_1024_heap != 0) mwMemHeapDestroy(fixed_block_1024_heap);
    fixed_block_1024_heap = 0;
}

static _mwMemHeap* createFixedHeap(MwMemHeapCreateParams* create, MwMemHeapParams* defaults,
                                    u32 block_size, u32 block_count, const char* name,
                                    void* callback) {
    MwMemFixedParams fixed;
    fixed.field_0x00 = 2;
    fixed.blockCount = block_count;
    fixed.blockSize = block_size;
    fixed.sizeThreshold = 4;
    fixed.flags = 8;
    create->strategyType = MW_MEM_STRATEGY_FIXED;
    create->initParams = &fixed;
    create->name = name;
    defaults->strategyCallback = callback;
    return _mwMemHeapCreate(create, defaults, 0, 0);
}

void mwMemAllocateFixedBlockHeaps(FixedHeapConfig* config) {
    MwMemHeapParams defaults;
    MwMemHeapCreateParams create;
    mwMemHeapGetDefaultParams(&defaults);
    defaults.field_0x08 = 0xAB;
    defaults.field_0x09 = 0;
    defaults.overflowEnable = 1;
    create.parentHeap = wave_heap;
    create.arenaSize = 1;
    create.extraSizeShift = 0;

    mkobj_heap = createFixedHeap(&create, &defaults, 0x100, config->mkobjSize,
                                 "MKOBJ fixed block heap", 0);
    mksobj_heap = createFixedHeap(&create, &defaults, 0x90, config->mksobjSize,
                                  "MKSOBJ fixed block heap", 0);
    mkproc_heap = createFixedHeap(&create, &defaults, 0xD0, config->mkprocSize,
                                  "MKPROC fixed block heap", 0);
    fixed_block_16_heap = createFixedHeap(&create, &defaults, 0x40, config->fixed16Size,
                                          "fixed block 16 heap", fixed16_strategy);
    fixed_block_32_heap = createFixedHeap(&create, &defaults, 0x80, config->fixed32Size,
                                          "fixed block 32 heap", fixed32_strategy);
    fixed_block_64_heap = createFixedHeap(&create, &defaults, 0x100, config->fixed64Size,
                                          "fixed block 64 heap", fixed64_strategy);
    fixed_block_128_heap = createFixedHeap(&create, &defaults, 0x200, config->fixed128Size,
                                           "fixed block 128 heap", fixed128_strategy);
    fixed_block_512_heap = createFixedHeap(&create, &defaults, 0x800, config->fixed512Size,
                                           "fixed block 512 heap", fixed512_strategy);
    fixed_block_1024_heap = createFixedHeap(&create, &defaults, 0x1000, config->fixed1024Size,
                                            "fixed block 1024 heap", fixed1024_strategy);
    tinystack_heap = createFixedHeap(&create, &defaults, 0x200, config->tinystackSize,
                                     "TINYSTACK fixed block heap", 0);
    bigstack_heap = createFixedHeap(&create, &defaults, 0x4000, config->bigstackSize,
                                    "BIGSTACK fixed block heap", 0);
}

static _mwMemHeap* createNormalHeap(_mwMemHeap* parent, u32 size, const char* name,
                                    MwMemHeapParams* defaults) {
    MwMemHeapCreateParams create;
    create.parentHeap = parent;
    create.arenaSize = size;
    create.field_0x08 = 0x10;
    create.strategyType = MW_MEM_STRATEGY_NORMAL;
    create.initParams = 0;
    create.name = name;
    create.extraSizeShift = 0;
    return _mwMemHeapCreate(&create, defaults, 0, 0);
}

static void mwMemHeapInit(void) {
    _mwMemHeap* system_heap = mwMemSystemGetHeap(0);
    MwMemHeapParams defaults;
    MwMemHeapCreateParams create;
    MwMemHeaderlessParams headerless;
    u32 free_size;
    u32 free_count;

    mwMemHeapGetDefaultParams(&defaults);
    defaults.field_0x08 = 0xAB;
    defaults.field_0x09 = 0;
    permanent_heap = createNormalHeap(system_heap, 0x562800, "Permanent heap", &defaults);
    section_heap = createNormalHeap(system_heap, 0x7DA800, "Section heap", &defaults);
    MWSOUND_HEAP = createNormalHeap(system_heap, 0xA7000, "mwSound heap", &defaults);
    MWSOUND_TEMP_HEAP = 0;
    mwfile_heap = createNormalHeap(system_heap, 0x2C800, "mwFile heap", &defaults);
    wave_heap = createNormalHeap(system_heap, 0x362800, "Wave heap", &defaults);
    mwMemSystemSetHeap(2, permanent_heap);

    create.parentHeap = system_heap;
    create.arenaSize = mwMemVirtualHeapGetHeapSize(system_heap);
    create.field_0x08 = 0x10;
    create.strategyType = MW_MEM_STRATEGY_VIRTUAL;
    create.initParams = 0;
    create.name = "MPEG heap";
    create.extraSizeShift = 0;
    defaults.strategyCallback = movie_strategy;
    movie_heap = _mwMemHeapCreate(&create, &defaults, 0, 0);

    mwMemHeapGetDefaultParams(&defaults);
    mwMemHeapGetParams(permanent_heap, &defaults);
    headerless.field_0x00 = 2;
    headerless.blockCount = 0x3C;
    headerless.blockSize = 0xC8;
    headerless.flags = 3;
    create.parentHeap = permanent_heap;
    create.arenaSize = 1;
    create.field_0x08 = 3;
    create.strategyType = MW_MEM_STRATEGY_HDRLESS;
    create.initParams = &headerless;
    create.name = "SECTION TABLE fixed block heap";
    create.extraSizeShift = 0;
    section_table_heap = _mwMemHeapCreate(&create, &defaults, 0, 0);

    mwMemHeapGetMaxFreeBlock(system_heap, &free_size, &free_count);
    MEMPRINT("==>> allocated OVERFLOW_HEAP size: %6.2f K\n", (float)free_size / 1024.0f);
    OSReport("==>> allocated OVERFLOW_HEAP size: %6.2f K\n", (float)free_size / 1024.0f);
    defaults.strategyCallback = 0;
    defaults.overflowEnable = 1;
    create.parentHeap = system_heap;
    create.arenaSize = free_size;
    create.field_0x08 = 0x10;
    create.strategyType = MW_MEM_STRATEGY_OVERFLOW;
    create.initParams = 0;
    create.name = "OVERFLOW Heap";
    create.extraSizeShift = 0;
    overflow_heap = _mwMemHeapCreate(&create, &defaults, 0, 0);
    mwMemSystemSetHeap(1, overflow_heap);
}
