typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

typedef void (*InitFunc)(void);

#include "dolphin/db.h"
#include "dolphin/os.h"
#include "runtime/cstring.h"

#pragma section code_type ".init"

typedef struct RomCopyInfo {
    const void* rom;
    void* address;
    u32 size;
} RomCopyInfo;

typedef struct BssInitInfo {
    void* address;
    u32 size;
} BssInitInfo;

typedef struct BootInfo2 {
    /* +0x00 */ u32 reserved00;
    /* +0x04 */ u32 reserved04;
    /* +0x08 */ u32 args_offset;
    /* +0x0C */ u32 debug_flag;
} BootInfo2;

extern RomCopyInfo _rom_copy_info[];
extern BssInitInfo _bss_init_info[];

void __init_registers(void);
void __init_hardware(void);
static void __init_data(void);
void __flush_cache(void* address, u32 size);
void __init_user(void);
int main(int argc, char** argv);
void exit(int status);

void InitMetroTRK(void);
void InitMetroTRK_BBA(void);

static u8 Debug_BBA;

static volatile u32* const kArenaHi = (volatile u32*)0x80000034;
static volatile u32* const kDebuggerPresent = (volatile u32*)0x80000044;
static BootInfo2* volatile* const kBootInfo2 = (BootInfo2* volatile*)0x800000F4;
static volatile u16* const kConsoleType = (volatile u16*)0x800030E6;
static volatile u32* const kFallbackDebugFlag = (volatile u32*)0x800030E8;

static void __check_pad3(void)
{
    volatile u16* pad3_button = (volatile u16*)0x800030E4;

    if ((*pad3_button & 0xEEF) == 0xEEF) {
        OSResetSystem(0, 0, 0);
    }
}

static void __set_debug_bba(void)
{
    Debug_BBA = 1;
}

static u8 __get_debug_bba(void)
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
void __start(void) {
    BootInfo2* boot_info;
    u32 debug_flag;
    int argc;
    char** argv;
    int i;

    /*
     * Matching status: readable software lift. Retail keeps argc/argv in
     * fixed startup registers and shares this TU with excluded hardware code.
     */
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
        u32* arg_block = (u32*)((u8*)boot_info + boot_info->args_offset);

        argc = (int)arg_block[0];
        argv = (char**)&arg_block[1];
        for (i = 0; i < argc; i++) {
            argv[i] = (char*)boot_info + (u32)argv[i];
        }
        *kArenaHi = (u32)argv & ~31U;
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
    void* destination, const void* source, u32 size)
{
    if (size != 0 && destination != source) {
        memcpy(destination, source, size);
        __flush_cache(destination, size);
    }
}

static inline void __init_bss_section(void* destination, u32 size)
{
    if (size != 0) {
        memset(destination, 0, size);
    }
}

static void __init_data(void) {
    RomCopyInfo* copy;
    BssInitInfo* bss;

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
