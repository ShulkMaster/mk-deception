#include "dolphin/db.h"
#include "dolphin/os.h"
#include "runtime/cstring.h"
#include "runtime/cstdlib.h"

#pragma section code_type ".init"

struct RomCopyInfo {
    const void* rom;
    void* address;
    unsigned int size;
};

struct BssInitInfo {
    void* address;
    unsigned int size;
};

struct BootInfo2 {
    /* +0x00 */ unsigned int reserved00;
    /* +0x04 */ unsigned int reserved04;
    /* +0x08 */ unsigned int args_offset;
    /* +0x0C */ unsigned int debug_flag;
};

extern struct RomCopyInfo _rom_copy_info[];
extern struct BssInitInfo _bss_init_info[];

void __init_registers(void);
void __init_hardware(void);
static void __init_data(void);
void __flush_cache(void* address, unsigned int size);
void __init_user(void);
int main(int argc, char** argv);

void InitMetroTRK(void);
void InitMetroTRK_BBA(void);

static unsigned char Debug_BBA;

static volatile unsigned int* const kArenaHi = (volatile unsigned int*)0x80000034;
static volatile unsigned int* const kDebuggerPresent = (volatile unsigned int*)0x80000044;
static struct BootInfo2* volatile* const kBootInfo2 = (struct BootInfo2* volatile*)0x800000F4;
static volatile unsigned short* const kConsoleType = (volatile unsigned short*)0x800030E6;
static volatile unsigned int* const kFallbackDebugFlag = (volatile unsigned int*)0x800030E8;

static void __check_pad3(void)
{
    volatile unsigned short* pad3_button = (volatile unsigned short*)0x800030E4;

    if ((*pad3_button & 0xEEF) == 0xEEF) {
        OSResetSystem(0, 0, 0);
    }
}

static void __set_debug_bba(void)
{
    Debug_BBA = 1;
}

static unsigned char __get_debug_bba(void)
{
    return Debug_BBA;
}

/*
 * Retail 0x80003154.
 *
 * The loader-supplied argument block is based at boot_info + args_offset.
 * Its first word is argc, followed by argc offsets which are relocated to
 * absolute argv pointers in place.
 */
/* TODO: [breakthrough needed] 0.00%; startup register contract remains unresolved. */
void __start(void) {
    struct BootInfo2* boot_info;
    unsigned int debug_flag;
    int argc;
    char** argv;
    int i;

    __init_registers();
    __init_hardware();
    __init_data();

    *kDebuggerPresent = 0;
    boot_info = *kBootInfo2;

    if (boot_info != 0) {
        debug_flag = boot_info->debug_flag;
    } else if (*kArenaHi != 0) {
        debug_flag = *kFallbackDebugFlag;
    } else {
        debug_flag = 0;
    }

    if (debug_flag == 2 || debug_flag == 3) {
        InitMetroTRK();
    } else if (debug_flag == 4) {
        __set_debug_bba();
    }

    if (boot_info != 0 && boot_info->args_offset != 0) {
        unsigned int* arg_block = (unsigned int*)((unsigned char*)boot_info + boot_info->args_offset);

        argc = arg_block[0];
        argv = (char**)&arg_block[1];
        for (i = 0; i < argc; i++) {
            argv[i] = (char*)boot_info + (unsigned int)argv[i];
        }
        *kArenaHi = (unsigned int)argv & ~31U;
    } else {
        argc = 0;
        argv = 0;
    }

    DBInit();
    OSInit();

    if ((*kConsoleType & 0x8000) == 0 || (*kConsoleType & 0x7FFF) == 1) {
        __check_pad3();
    }
    if (__get_debug_bba() == 1) {
        InitMetroTRK_BBA();
    }

    __init_user();
    exit(main(argc, argv));
}

/*
 * Retail 0x80003340. Copy initialized DOL sections to RAM, flush copied
 * executable ranges, then clear every BSS range.
 */
static inline void __copy_rom_section(
    void* destination, const void* source, unsigned int size)
{
    if (size != 0 && destination != source) {
        memcpy(destination, source, size);
        __flush_cache(destination, size);
    }
}

static inline void __init_bss_section(void* destination, unsigned int size)
{
    if (size != 0) {
        memset(destination, 0, size);
    }
}

/* TODO: [breakthrough needed] 78.333336%; copy/clear behavior matches retail,
 * but prologue/address-load and argument-copy source shape remains unresolved. */
static void __init_data(void) {
    struct RomCopyInfo* copy;
    struct BssInitInfo* bss;

    copy = _rom_copy_info;
    while (1) {
        if (copy->size == 0) {
            break;
        }
        __copy_rom_section(copy->address, copy->rom, copy->size);
        copy++;
    }

    bss = _bss_init_info;
    while (1) {
        if (bss->size == 0) {
            break;
        }
        __init_bss_section(bss->address, bss->size);
        bss++;
    }
}
