typedef unsigned char u8;
typedef unsigned long u32;
typedef unsigned long size_t;

static void TRK_fill_mem(void* destination, int value, size_t size);

__declspec(section ".init") void* TRK_memcpy(void* destination, const void* source,
                                               size_t size)
{
    const u8* source_byte = (const u8*)source - 1;
    u8* destination_byte = (u8*)destination - 1;

    size++;
    while (--size != 0)
        *++destination_byte = *++source_byte;
    return destination;
}

__declspec(section ".init") void* TRK_memset(void* destination, int value, size_t size)
{
    TRK_fill_mem(destination, value, size);
    return destination;
}

static void TRK_fill_mem(void* destination, int value, size_t size)
{
    u8* byte_destination = (u8*)destination - 1;
    u32 fill = (u8)value;
    u32 count;
    u32* word_destination;

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

        word_destination = (u32*)(byte_destination - 3);
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
        byte_destination = (u8*)word_destination + 3;
        size &= 3;
    }
    if (size != 0) {
        do {
            *++byte_destination = fill;
        } while (--size != 0);
    }
}
