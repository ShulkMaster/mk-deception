typedef unsigned char u8;
typedef unsigned long u32;
typedef unsigned long size_t;

__declspec(section ".init") void __fill_mem(void* destination, int value, size_t size);

__declspec(section ".init") void* memset(void* destination, int value, size_t size)
{
    __fill_mem(destination, value, size);
    return destination;
}

__declspec(section ".init") void __fill_mem(void* destination, int value, size_t size)
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
                *++byte_destination = (u8)fill;
            } while (--count != 0);
        }
        if (fill != 0)
            fill = fill << 24 | fill << 16 | fill << 8 | fill;

        word_destination = (u32*)(byte_destination - 3);
        count = size >> 5;
        if (count != 0) {
            do {
                word_destination[1] = fill;
                word_destination[2] = fill;
                word_destination[3] = fill;
                word_destination[4] = fill;
                word_destination[5] = fill;
                word_destination[6] = fill;
                word_destination[7] = fill;
                *(word_destination += 8) = fill;
            } while (--count != 0);
        }
        count = (size >> 2) & 7;
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
            *++byte_destination = (u8)fill;
        } while (--size != 0);
    }
}

__declspec(section ".init") void* memcpy(void* destination, const void* source, size_t size)
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
