#include "dolphin/types.h"

extern const u8* MPV_SearchDelim(const u8* data, int length, int mask);
extern int MPV_CheckDelim(const u8* data);

static s16 mpvemp_mbai[36] = {
    0,      0x0101, 0x0303, 0x0203, 0x0304, 0x0204,
    0x0305, 0x0205, 0x0707, 0x0607, 0x0B08, 0x0A08,
    0x0908, 0x0808, 0x0708, 0x0608, 0x170A, 0x160A,
    0x150A, 0x140A, 0x130A, 0x120A, 0x230B, 0x220B,
    0x210B, 0x200B, 0x1F0B, 0x1E0B, 0x1D0B, 0x1C0B,
    0x1B0B, 0x1A0B, 0x190B, 0x180B, 0x0F0B, 0x080B,
};

/* These operations are open-coded in retail so that the reader stays in
 * registers. They correspond to the shared bit-reader macros used by Sofdec. */
#define PEEK_BITS(value, count)                                                \
    do {                                                                       \
        (value) = bits >> (32 - (count));                                      \
        if (bit_offset > 32 - (count)) {                                       \
            (value) |= next_bits >> (64 - (count) - bit_offset);               \
        }                                                                      \
    } while (0)

#define SKIP_BITS(count)                                                       \
    do {                                                                       \
        bit_offset += (count);                                                 \
        if (bit_offset >= 32) {                                                \
            bit_offset -= 32;                                                  \
            bits = next_bits << bit_offset;                                    \
            next_bits = *words++;                                              \
        } else {                                                               \
            bits <<= (count);                                                  \
        }                                                                      \
    } while (0)

#define READ_BITS(value, count)                                                \
    do {                                                                       \
        PEEK_BITS(value, count);                                               \
        SKIP_BITS(count);                                                      \
    } while (0)

#define READ_CODE(value, count)                                                \
    do {                                                                       \
        int split = 32 - (count);                                              \
        if (bit_offset >= split) {                                             \
            bit_offset -= split;                                               \
            if (bit_offset != 0) {                                             \
                bits |= next_bits >> ((count) - bit_offset);                   \
                (value) = bits >> split;                                       \
                bits = next_bits << bit_offset;                                \
            } else {                                                           \
                (value) = bits >> split;                                       \
                bits = next_bits;                                              \
            }                                                                  \
            next_bits = *words++;                                              \
        } else {                                                               \
            (value) = bits >> split;                                           \
            bit_offset += (count);                                             \
            bits <<= (count);                                                  \
        }                                                                      \
    } while (0)

#define SKIP_HEADER_BITS()                                                     \
    do {                                                                       \
        if (bit_offset >= 27) {                                                \
            bit_offset -= 27;                                                  \
            bits = next_bits;                                                  \
            if (bit_offset != 0) {                                             \
                bits <<= bit_offset;                                           \
            }                                                                  \
            next_bits = *words++;                                              \
        } else {                                                               \
            bits <<= 5;                                                        \
            bit_offset += 5;                                                   \
        }                                                                      \
    } while (0)

#define READ_FLAG(value)                                                       \
    do {                                                                       \
        (value) = bits >> 31;                                                  \
        if (bit_offset == 31) {                                                \
            bits = next_bits;                                                  \
            next_bits = *words++;                                              \
            bit_offset = 0;                                                    \
        } else {                                                               \
            bits <<= 1;                                                        \
            bit_offset += 1;                                                   \
        }                                                                      \
    } while (0)

int MPV_IsEmptyPpic(const u8* data, int length, int macroblock_count)
{
    const u32* words;
    const u32* aligned;
    u32 bits;
    u32 next_bits;
    u32 value;
    u32 second;
    int bit_offset;
    int remaining;
    s16 code;
    int offset;
    const u8* delimiter;
    int delimiter_type;

    aligned = (const u32*)((u32)data & ~3);
    bit_offset = (data - (const u8*)aligned) * 8;
    second = aligned[1];
    value = aligned[0] << bit_offset;
    if (bit_offset != 0) {
        bits = second << bit_offset;
        value |= second >> (32 - bit_offset);
    } else {
        bits = second;
    }
    next_bits = aligned[2];
    words = aligned + 3;
    if (value != 0x101) {
        return 0;
    }
    SKIP_HEADER_BITS();
    READ_FLAG(value);
    if (value != 0) {
        return 0;
    }
    READ_FLAG(value);
    if (value == 0) {
        return 0;
    }
    READ_BITS(value, 5);
    if (value != 7) {
        return 0;
    }

    remaining = macroblock_count - 1;
    for (;;) {
        PEEK_BITS(value, 11);
        if (value != 8) {
            break;
        }
        SKIP_BITS(11);
        remaining -= 33;
        if (remaining <= 33) {
            break;
        }
    }
    if (remaining <= 0 || remaining > 33) {
        return 0;
    }
    code = mpvemp_mbai[remaining];
    READ_CODE(value, (u8)code);
    if (value != (u32)((u16)code >> 8)) {
        return 0;
    }
    READ_BITS(value, 5);
    if (value != 7) {
        return 0;
    }

    offset = ((const u8*)words + ((bit_offset + 7) >> 3) - 8) - data;
    if (offset > length) {
        return 0;
    }

    for (;;) {
        delimiter = MPV_SearchDelim(data + offset, length - offset, 0xCC);
        if (delimiter == 0) {
            return 0;
        }
        delimiter_type = MPV_CheckDelim(delimiter);
        if ((delimiter_type & 4) != 0) {
            int picture_type = (((signed char)delimiter[5] << 1) & 6) |
                               ((delimiter[6] >> 7) & 1);
            if (picture_type != 3) {
                return 0;
            }
            offset = delimiter + 1 - data;
        } else if ((delimiter_type & 0x40) != 0) {
            offset = delimiter + 1 - data;
        } else if ((delimiter_type & 0x80) != 0) {
            return delimiter + 4 - data <= length;
        } else if ((delimiter_type & 8) != 0) {
            if ((delimiter[7] & 0x40) == 0) {
                return 0;
            }
            return delimiter + 7 - data <= length;
        } else {
            return 0;
        }
    }
}

int MPV_IsEmptyBpic(const u8* data, int length, int macroblock_count)
{
    const u32* words;
    const u32* aligned;
    u32 bits;
    u32 next_bits;
    u32 value;
    u32 second;
    int bit_offset;
    int remaining;
    s16 code;
    int macroblock_type;

    aligned = (const u32*)((u32)data & ~3);
    bit_offset = (data - (const u8*)aligned) * 8;
    second = aligned[1];
    value = aligned[0] << bit_offset;
    if (bit_offset != 0) {
        bits = second << bit_offset;
        value |= second >> (32 - bit_offset);
    } else {
        bits = second;
    }
    next_bits = aligned[2];
    words = aligned + 3;
    if (value != 0x101) {
        return 0;
    }
    SKIP_HEADER_BITS();
    READ_FLAG(value);
    if (value != 0) {
        return 0;
    }
    READ_FLAG(value);
    if (value == 0) {
        return 0;
    }

    PEEK_BITS(macroblock_type, 6);
    if ((macroblock_type < 22 && macroblock_type != 11) ||
        macroblock_type >= 24) {
        return 0;
    }
    if (macroblock_type == 11) {
        SKIP_BITS(6);
    } else {
        SKIP_BITS(5);
    }

    remaining = macroblock_count - 1;
    for (;;) {
        PEEK_BITS(value, 11);
        if (value != 8) {
            break;
        }
        SKIP_BITS(11);
        remaining -= 33;
        if (remaining <= 33) {
            break;
        }
    }
    if (remaining <= 0 || remaining > 33) {
        return 0;
    }
    code = mpvemp_mbai[remaining];
    READ_CODE(value, (u8)code);
    if (value != (u32)((u16)code >> 8)) {
        return 0;
    }

    PEEK_BITS(macroblock_type, 6);
    if ((macroblock_type < 22 && macroblock_type != 11) ||
        macroblock_type >= 24) {
        return 0;
    }
    if (macroblock_type == 11) {
        SKIP_BITS(6);
    } else {
        SKIP_BITS(5);
    }
    return ((const u8*)words + ((bit_offset + 7) >> 3) - 8) - data <= length;
}

#undef READ_BITS
#undef READ_CODE
#undef READ_FLAG
#undef SKIP_HEADER_BITS
#undef SKIP_BITS
#undef PEEK_BITS
