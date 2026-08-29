#include "mw/mwMemHeap.h"
#include "dolphin/os.h"
#include "mw/mwMemDebug.h"
#include "mw/mwMemPlatform.h"

_mwMemHeap* overflow_heap = 0;
_mwMemHeap* permanent_heap = 0;
_mwMemHeap* mwfile_heap = 0;
_mwMemHeap* section_heap = 0;
_mwMemHeap* wave_heap = 0;
_mwMemHeap* MWSOUND_HEAP = 0;
_mwMemHeap* MWSOUND_TEMP_HEAP = 0;
_mwMemHeap* movie_heap = 0;
_mwMemHeap* mkobj_heap = 0;
_mwMemHeap* mksobj_heap = 0;
_mwMemHeap* konquest_objects_heap = 0;
_mwMemHeap* konquest_subobjects_heap = 0;
_mwMemHeap* mkproc_heap = 0;
_mwMemHeap* tinystack_heap = 0;
_mwMemHeap* bigstack_heap = 0;
_mwMemHeap* section_table_heap = 0;
_mwMemHeap* fixed_block_16_heap = 0;
_mwMemHeap* fixed_block_32_heap = 0;
_mwMemHeap* fixed_block_64_heap = 0;
_mwMemHeap* fixed_block_128_heap = 0;
_mwMemHeap* fixed_block_512_heap = 0;
_mwMemHeap* fixed_block_1024_heap = 0;

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
             size_kb, origin_info.name, destination_info.name, info->sourceFunction, info->line);
}

/*
 * Retail control flow, calls, conversions, widths, and frame size agree.
 * Objdiff's five remaining records are GPR operand coloring only.
 */
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

int mwMemUserConfigAssert(const char* expression, const char* file, u32 line) {
    (void)expression;
    (void)file;
    (void)line;
    return 1;
}

void mwMemUserConfigPrintf(const char* format, ...) {}

static void* movie_strategy(u32 size, _mwMemHeap* source, u32 flags,
                            MwMemMallocRequest* request) {
    void* block;
    _mwMemHeap* system_overflow;

    system_overflow = mwMemSystemGetHeap(1);
    block = mwMemHeapStrategyCallback(size, wave_heap, flags, request);
    if (block == 0) {
        block = mwMemHeapStrategyCallback(size, permanent_heap, flags, request);
        if (block == 0) {
            block = mwMemHeapStrategyCallback(size, section_heap, flags, request);
            if (block == 0) {
                block = mwMemHeapStrategyCallback(size, system_overflow, flags, request);
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

#pragma optimize_for_size on
static void* fixed1024_strategy(u32 size, _mwMemHeap* source, u32 flags,
                                MwMemMallocRequest* request) {
    void* block;

    block = mwMemHeapStrategyCallback(size, fixed_block_1024_heap, flags, request);
    if (block == 0) block = mwMemHeapStrategyCallback(size, wave_heap, flags, request);
    return block;
}

#define DEFINE_FIXED_STRATEGY(name, own_heap, next_heap, message)                              \
    static void* name(u32 size, _mwMemHeap* source, u32 flags,                                \
                      MwMemMallocRequest* request) {                                            \
        void* block;                                                                           \
                                                                                               \
        block = mwMemHeapStrategyCallback(size, own_heap, flags, request);                     \
        if (block == 0) {                                                                       \
            MEMPRINT(message);                                                                  \
            block = mwMemHeapStrategyCallback(size, next_heap, flags, request);                \
            if (block == 0) block = mwMemHeapStrategyCallback(size, wave_heap, flags, request); \
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
#pragma optimize_for_size reset

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

#pragma optimize_for_size on
/*
 * Retail heap order, parameters, callbacks, access widths, calls, and 920-byte
 * code size agree. The remaining diff has an identical opcode histogram and is
 * confined to GPR allocation and scheduling of independent parameter stores.
 */
void mwMemAllocateFixedBlockHeaps(FixedHeapConfig* config) {
    MwMemHeapCreateParams create;
    MwMemHeapParams defaults;
    MwMemFixedParams fixed;
    mwMemHeapGetDefaultParams(&defaults);
    defaults.paramByte0 = 0xAB;
    defaults.paramByte1 = 0;
    defaults.overflowEnable = 1;
    defaults.strategyCallback = 0;
    create.parentHeap = wave_heap;
    create.arenaSize = 1;
    create.field_0x08 = 4;
    create.strategyType = MW_MEM_STRATEGY_FIXED;
    create.fixedInitParams = &fixed;
    create.extraSizeShift = 0;
    fixed.field_0x00 = 2;
    fixed.sizeThreshold = 8;

    create.name = "MKOBJ fixed block heap";
    fixed.blockCount = config->mkobjCount;
    fixed.blockSize = 0x100;
    fixed.flags = 4;
    mkobj_heap = _mwMemHeapCreate(&create, &defaults, 0, 0);

    create.name = "MKSOBJ fixed block heap";
    fixed.blockCount = config->mksobjCount;
    fixed.blockSize = 0x90;
    fixed.flags = 4;
    mksobj_heap = _mwMemHeapCreate(&create, &defaults, 0, 0);

    create.name = "MKPROC fixed block heap";
    fixed.blockCount = config->mkprocCount;
    fixed.blockSize = 0xD0;
    fixed.flags = 3;
    mkproc_heap = _mwMemHeapCreate(&create, &defaults, 0, 0);

    create.name = "fixed block 16 heap";
    fixed.blockCount = config->fixed16Count;
    fixed.blockSize = 0x40;
    fixed.flags = 4;
    defaults.strategyCallback = fixed16_strategy;
    fixed_block_16_heap = _mwMemHeapCreate(&create, &defaults, 0, 0);

    create.name = "fixed block 32 heap";
    fixed.blockCount = config->fixed32Count;
    fixed.blockSize = 0x80;
    fixed.flags = 4;
    defaults.strategyCallback = fixed32_strategy;
    fixed_block_32_heap = _mwMemHeapCreate(&create, &defaults, 0, 0);

    create.name = "fixed block 64 heap";
    fixed.blockCount = config->fixed64Count;
    fixed.blockSize = 0x100;
    fixed.flags = 4;
    defaults.strategyCallback = fixed64_strategy;
    fixed_block_64_heap = _mwMemHeapCreate(&create, &defaults, 0, 0);

    create.name = "fixed block 128 heap";
    fixed.blockCount = config->fixed128Count;
    fixed.blockSize = 0x200;
    fixed.flags = 4;
    defaults.strategyCallback = fixed128_strategy;
    fixed_block_128_heap = _mwMemHeapCreate(&create, &defaults, 0, 0);

    create.name = "fixed block 512 heap";
    fixed.blockCount = config->fixed512Count;
    fixed.blockSize = 0x800;
    fixed.flags = 4;
    defaults.strategyCallback = fixed512_strategy;
    fixed_block_512_heap = _mwMemHeapCreate(&create, &defaults, 0, 0);

    create.name = "fixed block 1024 heap";
    fixed.blockCount = config->fixed1024Count;
    fixed.blockSize = 0x1000;
    fixed.flags = 4;
    defaults.strategyCallback = fixed1024_strategy;
    fixed_block_1024_heap = _mwMemHeapCreate(&create, &defaults, 0, 0);

    fixed.sizeThreshold = 0;
    create.name = "TINYSTACK fixed block heap";
    fixed.blockCount = config->tinystackCount;
    fixed.blockSize = 0x200;
    fixed.flags = 3;
    defaults.strategyCallback = 0;
    tinystack_heap = _mwMemHeapCreate(&create, &defaults, 0, 0);

    create.name = "BIGSTACK fixed block heap";
    fixed.blockCount = config->bigstackCount;
    fixed.blockSize = 0x4000;
    fixed.flags = 3;
    defaults.strategyCallback = 0;
    bigstack_heap = _mwMemHeapCreate(&create, &defaults, 0, 0);
}
#pragma optimize_for_size reset

static inline _mwMemHeap* createNormalHeap(MwMemHeapCreateParams* create,
                                            _mwMemHeap* parent, u32 size,
                                            const char* name,
                                            MwMemHeapParams* defaults) {
    create->parentHeap = parent;
    create->arenaSize = size;
    create->field_0x08 = 0x10;
    create->strategyType = MW_MEM_STRATEGY_NORMAL;
    create->name = name;
    create->extraSizeShift = 0;
    return _mwMemHeapCreate(create, defaults, 0, 0);
}

static inline _mwMemHeap* createOptionalNormalHeap(MwMemHeapCreateParams* create,
                                                    _mwMemHeap* parent, u32 size,
                                                    const char* name,
                                                    MwMemHeapParams* defaults) {
    create->parentHeap = parent;
    create->arenaSize = size;
    create->field_0x08 = 0x10;
    create->strategyType = MW_MEM_STRATEGY_NORMAL;
    create->name = name;
    create->extraSizeShift = 0;
    if (create->arenaSize == 0) {
        return 0;
    }
    return _mwMemHeapCreate(create, defaults, 0, 0);
}

#pragma optimize_for_size on
/*
 * Retail topology, branch/call sequence, stack layout, and 1024-byte code size
 * agree. Objdiff's remaining records are operand-register coloring only.
 */
static void mwMemHeapInit(void) {
    _mwMemHeap* system_heap = mwMemSystemGetHeap(0);
    MwMemHeapCreateParams create;
    MwMemHeapParams defaults;
    MwMemFixedParams section_fixed;
    MwMemHeapCreateParams section_create;
    MwMemHeapParams section_defaults;
    u32 free_size;
    u32 free_count;

    mwMemHeapGetDefaultParams(&defaults);
    defaults.paramByte0 = 0xAB;
    defaults.paramByte1 = 0;
    permanent_heap =
        createNormalHeap(&create, system_heap, 0x562800, "Permanent heap", &defaults);
    section_heap =
        createNormalHeap(&create, system_heap, 0x7DA800, "Section heap", &defaults);
    MWSOUND_HEAP =
        createOptionalNormalHeap(&create, system_heap, 0xA7000, "mwSound heap", &defaults);
    MWSOUND_TEMP_HEAP =
        createOptionalNormalHeap(&create, system_heap, 0, "mwSound Temp heap", &defaults);
    mwfile_heap =
        createNormalHeap(&create, system_heap, 0x2C800, "mwFile heap", &defaults);
    wave_heap = createNormalHeap(&create, system_heap, 0x362800, "Wave heap", &defaults);
    mwMemSystemSetHeap(2, permanent_heap);

    create.parentHeap = system_heap;
    create.arenaSize = mwMemVirtualHeapGetHeapSize();
    create.field_0x08 = 0x10;
    create.strategyType = MW_MEM_STRATEGY_VIRTUAL;
    create.name = "MPEG heap";
    create.extraSizeShift = 0;
    defaults.strategyCallback = movie_strategy;
    movie_heap = _mwMemHeapCreate(&create, &defaults, 0, 0);

    mwMemHeapGetDefaultParams(&defaults);
    defaults.paramByte0 = 0xAB;
    defaults.paramByte1 = 0;
    mwMemHeapGetParams(permanent_heap, &section_defaults);
    section_fixed.field_0x00 = 2;
    section_fixed.blockCount = 0x3C;
    section_fixed.blockSize = 0xC8;
    section_fixed.sizeThreshold = 0;
    section_fixed.flags = 3;
    section_create.parentHeap = permanent_heap;
    section_create.arenaSize = 1;
    section_create.field_0x08 = 3;
    section_create.strategyType = MW_MEM_STRATEGY_FIXED;
    section_create.fixedInitParams = &section_fixed;
    section_create.name = "SECTION TABLE fixed block heap";
    section_create.extraSizeShift = 0;
    section_table_heap = _mwMemHeapCreate(&section_create, &section_defaults, 0, 0);

    mwMemHeapGetMaxFreeBlock(system_heap, &free_size, &free_count);
    MEMPRINT("==>> allocated OVERFLOW_HEAP size: %6.2f K\n", (float)free_size / 1024.0f);
    OSReport("==>> allocated OVERFLOW_HEAP size: %6.2f K\n", (float)free_size / 1024.0f);
    defaults.strategyCallback = 0;
    defaults.overflowEnable = 1;
    create.parentHeap = system_heap;
    create.arenaSize = free_size;
    create.field_0x08 = 0x10;
    create.strategyType = MW_MEM_STRATEGY_OVERFLOW;
    create.name = "OVERFLOW Heap";
    create.extraSizeShift = 0;
    overflow_heap = _mwMemHeapCreate(&create, &defaults, 0, 0);
    mwMemSystemSetHeap(1, overflow_heap);
}
#pragma optimize_for_size reset
