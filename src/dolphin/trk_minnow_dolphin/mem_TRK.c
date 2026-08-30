#pragma section code_type ".init"
#include "dolphin/trk.h"
#pragma section code_type ".text"
typedef unsigned long size_t;

static void TRK_fill_mem(void* destination, int value, size_t size);

#pragma section code_type ".init"

void* TRK_memcpy(void* destination, const void* source, size_t size)
{
    const u8* source_byte = (const u8*)source - 1;
    u8* destination_byte = (u8*)destination - 1;

    size++;
    while (--size != 0)
        *++destination_byte = *++source_byte;
    return destination;
}

void* TRK_memset(void* destination, int value, size_t size)
{
    TRK_fill_mem(destination, value, size);
    return destination;
}

#pragma section code_type ".text"
#pragma dont_inline on
static void TRK_fill_mem(void* destination, int value, size_t size)
{
#define MOVE_CURSOR(destination, amount, destination_width, source_width)      \
    ((u##destination_width*)destination) =                                    \
        ((u##destination_width*)(((u##source_width*)destination) + (amount))) - 1
#define MOVE_TO_BYTE_CURSOR(destination, amount) MOVE_CURSOR(destination, amount, 8, 32)
#define MOVE_TO_WORD_CURSOR(destination, amount) MOVE_CURSOR(destination, amount, 32, 8)

    u32 fill = (u8)value;
    u32 count;
    u32 index;

    MOVE_TO_BYTE_CURSOR(destination, 0);

    if (size >= 32) {
        count = ~(u32)destination & 3;
        if (count != 0) {
            size -= count;
            do {
                *++((u8*)destination) = fill;
            } while (--count != 0);
        }
        if (fill != 0)
            fill |= fill << 24 | fill << 16 | fill << 8;

        MOVE_TO_WORD_CURSOR(destination, 4);
        MOVE_TO_WORD_CURSOR(destination, 1);
        count = size >> 5;
        if (count != 0) {
            do {
                for (index = 0; index < 8; index++) {
                    *++((u32*)destination) = fill;
                }
            } while (--count != 0);
        }
        count = (size & 31) >> 2;
        if (count != 0) {
            do {
                *++((u32*)destination) = fill;
            } while (--count != 0);
        }
        MOVE_TO_BYTE_CURSOR(destination, 1);
        size &= 3;
    }
    if (size != 0) {
        do {
            *++((u8*)destination) = fill;
        } while (--size != 0);
    }

#undef MOVE_TO_WORD_CURSOR
#undef MOVE_TO_BYTE_CURSOR
#undef MOVE_CURSOR
}
#pragma dont_inline reset
