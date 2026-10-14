#include "dolphin/types.h"
#include "runtime/__mem.h"

void* memset(void* destination, int value, unsigned int size)
{
    __fill_mem(destination, value, size);
    return destination;
}

void __fill_mem(void* destination, int value, unsigned long size)
{
    u8* byte_destination;
    u32* word_destination;
    u32 count;
    u32 fill = (u8)value;

    byte_destination = (u8*)destination - 1;
    if (size >= 32) {
        count = ~(u32)byte_destination & 3;
        if (count != 0) {
            size -= count;
            do {
                *++byte_destination = fill;
            } while (--count != 0);
        }
        if (fill != 0)
            fill |= fill << 24 | fill << 16 | fill << 8;

        word_destination = (u32*)(byte_destination + 1) - 1;
        count = size >> 5;
        if (count != 0) {
            do {
                *++word_destination = fill;
                *++word_destination = fill;
                *++word_destination = fill;
                *++word_destination = fill;
                *++word_destination = fill;
                *++word_destination = fill;
                *++word_destination = fill;
                *++word_destination = fill;
            } while (--count != 0);
        }
        count = (size & 31) >> 2;
        if (count != 0) {
            do {
                *++word_destination = fill;
            } while (--count != 0);
        }
        byte_destination = (u8*)(word_destination + 1) - 1;
        size &= 3;
    }
    if (size != 0) {
        do {
            *++byte_destination = fill;
        } while (--size != 0);
    }
}

void* memcpy(void* destination, const void* source, unsigned long size)
{
    const u8* source_byte;
    u8* destination_byte;
    int count;

    if (source >= destination) {
        source_byte = (const u8*)source - 1;
        destination_byte = (u8*)destination - 1;
        count = size + 1;
        while (--count != 0)
            *++destination_byte = *++source_byte;
    } else {
        source_byte = (const u8*)source + size;
        destination_byte = (u8*)destination + size;
        count = size + 1;
        while (--count != 0)
            *--destination_byte = *--source_byte;
    }
    return destination;
}
