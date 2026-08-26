typedef unsigned long size_t;

extern void __copy_longs_unaligned(void* destination, const void* source, size_t count);
extern void __copy_longs_rev_unaligned(void* destination, const void* source, size_t count);
extern void __copy_longs_aligned(void* destination, const void* source, size_t count);
extern void __copy_longs_rev_aligned(void* destination, const void* source, size_t count);

int memcmp(const void* left, const void* right, size_t count)
{
    const unsigned char* left_byte;
    const unsigned char* right_byte;

    for (left_byte = (const unsigned char*)left - 1,
         right_byte = (const unsigned char*)right - 1,
         ++count;
         --count;) {
        if (*++left_byte != *++right_byte)
            return *left_byte < *right_byte ? -1 : 1;
    }
    return 0;
}

void* __memrchr(const void* memory, int character, size_t count)
{
    const unsigned char* byte;
    unsigned long value = character & 0xff;

    for (byte = (const unsigned char*)memory + count, ++count; --count;) {
        if (*--byte == value)
            return (void*)byte;
    }
    return 0;
}

void* memchr(const void* memory, int character, size_t count)
{
    const unsigned char* byte;
    unsigned long value = character & 0xff;

    for (byte = (const unsigned char*)memory - 1, ++count; --count;) {
        if ((*++byte & 0xff) == value)
            return (void*)byte;
    }
    return 0;
}

void* memmove(void* destination, const void* source, size_t count)
{
    unsigned char* source_byte;
    unsigned char* destination_byte;
    int reverse = (unsigned int)source < (unsigned int)destination;

    if (count >= 32) {
        if (((unsigned int)destination ^ (unsigned int)source) & 3) {
            if (!reverse)
                __copy_longs_unaligned(destination, source, count);
            else
                __copy_longs_rev_unaligned(destination, source, count);
        } else if (!reverse) {
            __copy_longs_aligned(destination, source, count);
        } else {
            __copy_longs_rev_aligned(destination, source, count);
        }
        return destination;
    }

    if (!reverse) {
        source_byte = (unsigned char*)source - 1;
        destination_byte = (unsigned char*)destination - 1;
        ++count;
        while (--count > 0)
            *++destination_byte = *++source_byte;
    } else {
        source_byte = (unsigned char*)source + count;
        destination_byte = (unsigned char*)destination + count;
        ++count;
        while (--count > 0)
            *--destination_byte = *--source_byte;
    }
    return destination;
}
