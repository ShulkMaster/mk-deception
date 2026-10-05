#include "dolphin/db.h"
#include "dolphin/os.h"
#include "runtime/cstring.h"
#include "runtime/cstdlib.h"
#include "runtime/asm_sequences.inc"

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

extern struct RomCopyInfo _rom_copy_info[];
extern struct BssInitInfo _bss_init_info[];
extern char _SDA_BASE_[];
extern char _SDA2_BASE_[];

static void __init_registers(void);
void __init_hardware(void);
static void __init_data(void);
void __flush_cache(void* address, unsigned int size);
void __init_user(void);
int main(int argc, char** argv);

void InitMetroTRK(void);
void InitMetroTRK_BBA(void);

static unsigned char Debug_BBA;

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

__declspec(weak) asm void __start(void)
{
    SEQ___start();
}

static asm void __init_registers(void)
{
    SEQ___init_registers();
}

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
