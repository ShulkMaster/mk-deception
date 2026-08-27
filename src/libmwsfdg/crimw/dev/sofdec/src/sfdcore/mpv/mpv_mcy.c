typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed int s32;

typedef struct MPVMC16Context MPVMC16Context;
typedef void (*MPVMC16Function)(MPVMC16Context* context);

struct MPVMC16Context {
    MPVMC16Function functions08[4];
    u8 field_10[8];
    u8* destination;
    u32 field_1c;
    s32 reference_stride;
    const u8* reference0;
    const u8* reference1;
    u8 field_2c[8];
    MPVMC16Function functions16[4];
};

static const MPVMC16Function mpvmc16_oneref1p_func_table[4] = {0, 0, 0, 0};

static inline u32 mpvmc16_pack_avg4(const u8* reference0,
                                    const u8* reference1)
{
    return (((reference0[0] +
              (reference0[1] + reference1[0] + reference1[1] + 2))
             << 22) &
            0xFF000000) |
           (((reference0[1] +
              (reference0[2] + reference1[1] + reference1[2] + 2))
             << 14) &
            0x00FF0000) |
           (((reference0[2] +
              (reference0[3] + reference1[2] + reference1[3] + 2))
             << 6) &
            0x0000FF00) |
           (((reference0[3] +
              (reference0[4] + reference1[3] + reference1[4] + 2)) >>
             2) &
            0x000000FF);
}

static inline u32 mpvmc16_avg_words(u32 left, u32 right)
{
    u32 different = left ^ right;
    return (left & right) + ((different & 0xFEFEFEFE) >> 1) +
           (different & 0x01010101);
}

void MPVMC16_OneRef4p_TuneC(MPVMC16Context* context)
{
    s32 row;
    const u8* reference0 = context->reference0;
    const u8* reference1 = context->reference1;
    u8* destination = context->destination;

    for (row = 0; row < 16; row++) {
        u32* output = (u32*)destination;

        output[0] = mpvmc16_pack_avg4(reference0, reference1);
        output[1] = mpvmc16_pack_avg4(reference0 + 4, reference1 + 4);
        output[16] = mpvmc16_pack_avg4(reference0 + 8, reference1 + 8);
        output[17] = mpvmc16_pack_avg4(reference0 + 12, reference1 + 12);
        destination += 8;
        if (row == 7) {
            destination += 0x40;
        }
        reference0 += context->reference_stride;
        reference1 += context->reference_stride;
    }
}

void MPVMC16_OneRefH2_TuneC(MPVMC16Context* context)
{
    s32 row;
    const u8* reference = context->reference0;
    u8* destination = context->destination;
    u32 alignment = (u32)reference & 3;

    reference -= alignment;
    switch (alignment) {
    case 0:
        for (row = 0; row < 16; row++) {
            const u32* words = (const u32*)reference;
            u32* output = (u32*)destination;
            output[0] = mpvmc16_avg_words(
                words[0], (words[0] << 8) | (words[1] >> 24));
            output[1] = mpvmc16_avg_words(
                words[1], (words[1] << 8) | (words[2] >> 24));
            output[16] = mpvmc16_avg_words(
                words[2], (words[2] << 8) | (words[3] >> 24));
            output[17] = mpvmc16_avg_words(
                words[3], (words[3] << 8) | (words[4] >> 24));
            reference += context->reference_stride;
            destination += 8;
            if (row == 7) {
                destination += 0x40;
            }
        }
        break;
    case 1:
        for (row = 0; row < 16; row++) {
            const u32* words = (const u32*)reference;
            u32* output = (u32*)destination;
            output[0] = mpvmc16_avg_words(
                (words[0] << 8) | (words[1] >> 24),
                (words[0] << 16) | (words[1] >> 16));
            output[1] = mpvmc16_avg_words(
                (words[1] << 8) | (words[2] >> 24),
                (words[1] << 16) | (words[2] >> 16));
            output[16] = mpvmc16_avg_words(
                (words[2] << 8) | (words[3] >> 24),
                (words[2] << 16) | (words[3] >> 16));
            output[17] = mpvmc16_avg_words(
                (words[3] << 8) | (words[4] >> 24),
                (words[3] << 16) | (words[4] >> 16));
            reference += context->reference_stride;
            destination += 8;
            if (row == 7) {
                destination += 0x40;
            }
        }
        break;
    case 2:
        for (row = 0; row < 16; row++) {
            const u32* words = (const u32*)reference;
            u32* output = (u32*)destination;
            output[0] = mpvmc16_avg_words(
                (words[0] << 16) | (words[1] >> 16),
                (words[0] << 24) | (words[1] >> 8));
            output[1] = mpvmc16_avg_words(
                (words[1] << 16) | (words[2] >> 16),
                (words[1] << 24) | (words[2] >> 8));
            output[16] = mpvmc16_avg_words(
                (words[2] << 16) | (words[3] >> 16),
                (words[2] << 24) | (words[3] >> 8));
            output[17] = mpvmc16_avg_words(
                (words[3] << 16) | (words[4] >> 16),
                (words[3] << 24) | (words[4] >> 8));
            reference += context->reference_stride;
            destination += 8;
            if (row == 7) {
                destination += 0x40;
            }
        }
        break;
    default:
        for (row = 0; row < 16; row++) {
            const u32* words = (const u32*)reference;
            u32* output = (u32*)destination;
            output[0] = mpvmc16_avg_words(
                (words[0] << 24) | (words[1] >> 8), words[1]);
            output[1] = mpvmc16_avg_words(
                (words[1] << 24) | (words[2] >> 8), words[2]);
            output[16] = mpvmc16_avg_words(
                (words[2] << 24) | (words[3] >> 8), words[3]);
            output[17] = mpvmc16_avg_words(
                (words[3] << 24) | (words[4] >> 8), words[4]);
            reference += context->reference_stride;
            destination += 8;
            if (row == 7) {
                destination += 0x40;
            }
        }
        break;
    }
}

void MPVMC16_OneRefV2_TuneC(MPVMC16Context* context)
{
    s32 row;
    const u8* reference0 = context->reference0;
    const u8* reference1 = context->reference1;
    u8* destination = context->destination;
    u32 alignment = (u32)reference0 & 3;

    reference0 -= alignment;
    reference1 -= alignment;
    switch (alignment) {
    case 0:
        for (row = 0; row < 16; row++) {
            const u32* words0 = (const u32*)reference0;
            const u32* words1 = (const u32*)reference1;
            u32* output = (u32*)destination;
            output[0] = mpvmc16_avg_words(words0[0], words1[0]);
            output[1] = mpvmc16_avg_words(words0[1], words1[1]);
            output[16] = mpvmc16_avg_words(words0[2], words1[2]);
            output[17] = mpvmc16_avg_words(words0[3], words1[3]);
            reference0 += context->reference_stride;
            reference1 += context->reference_stride;
            destination += 8;
            if (row == 7) {
                destination += 0x40;
            }
        }
        break;
    case 1:
        for (row = 0; row < 16; row++) {
            const u32* words0 = (const u32*)reference0;
            const u32* words1 = (const u32*)reference1;
            u32* output = (u32*)destination;
            output[0] = mpvmc16_avg_words(
                (words0[0] << 8) | (words0[1] >> 24),
                (words1[0] << 8) | (words1[1] >> 24));
            output[1] = mpvmc16_avg_words(
                (words0[1] << 8) | (words0[2] >> 24),
                (words1[1] << 8) | (words1[2] >> 24));
            output[16] = mpvmc16_avg_words(
                (words0[2] << 8) | (words0[3] >> 24),
                (words1[2] << 8) | (words1[3] >> 24));
            output[17] = mpvmc16_avg_words(
                (words0[3] << 8) | reference0[16],
                (words1[3] << 8) | reference1[16]);
            reference0 += context->reference_stride;
            reference1 += context->reference_stride;
            destination += 8;
            if (row == 7) {
                destination += 0x40;
            }
        }
        break;
    case 2:
        for (row = 0; row < 16; row++) {
            const u32* words0 = (const u32*)reference0;
            const u32* words1 = (const u32*)reference1;
            u32* output = (u32*)destination;
            output[0] = mpvmc16_avg_words(
                (words0[0] << 16) | (words0[1] >> 16),
                (words1[0] << 16) | (words1[1] >> 16));
            output[1] = mpvmc16_avg_words(
                (words0[1] << 16) | (words0[2] >> 16),
                (words1[1] << 16) | (words1[2] >> 16));
            output[16] = mpvmc16_avg_words(
                (words0[2] << 16) | (words0[3] >> 16),
                (words1[2] << 16) | (words1[3] >> 16));
            output[17] = mpvmc16_avg_words(
                (words0[3] << 16) |
                    *(const unsigned short*)(reference0 + 16),
                (words1[3] << 16) |
                    *(const unsigned short*)(reference1 + 16));
            reference0 += context->reference_stride;
            reference1 += context->reference_stride;
            destination += 8;
            if (row == 7) {
                destination += 0x40;
            }
        }
        break;
    default:
        for (row = 0; row < 16; row++) {
            const u32* words0 = (const u32*)reference0;
            const u32* words1 = (const u32*)reference1;
            u32* output = (u32*)destination;
            output[0] = mpvmc16_avg_words(
                (words0[0] << 24) | (words0[1] >> 8),
                (words1[0] << 24) | (words1[1] >> 8));
            output[1] = mpvmc16_avg_words(
                (words0[1] << 24) | (words0[2] >> 8),
                (words1[1] << 24) | (words1[2] >> 8));
            output[16] = mpvmc16_avg_words(
                (words0[2] << 24) | (words0[3] >> 8),
                (words1[2] << 24) | (words1[3] >> 8));
            output[17] = mpvmc16_avg_words(
                (words0[3] << 24) | (words0[4] >> 8),
                (words1[3] << 24) | (words1[4] >> 8));
            reference0 += context->reference_stride;
            reference1 += context->reference_stride;
            destination += 8;
            if (row == 7) {
                destination += 0x40;
            }
        }
        break;
    }
}

void MPVMC16_OneRef1p_TuneC(MPVMC16Context* context)
{
    s32 row;
    const u8* reference = context->reference0;

    switch ((u32)reference & 7) {
    case 0: {
        const double* source = (const double*)reference;
        double* destination = (double*)context->destination;
        s32 pitch = context->reference_stride / 8;

        for (row = 0; row < 8; row++) {
            destination[0] = source[0];
            destination[8] = source[1];
            source += pitch;
            destination++;
        }
        destination += 8;
        for (row = 0; row < 8; row++) {
            destination[0] = source[0];
            destination[8] = source[1];
            source += pitch;
            destination++;
        }
        break;
    }
    case 4: {
        const u32* source = (const u32*)reference;
        u32* destination = (u32*)context->destination;
        s32 pitch = context->reference_stride / 4;

        for (row = 0; row < 8; row++) {
            destination[0] = source[0];
            destination[1] = source[1];
            destination[16] = source[2];
            destination[17] = source[3];
            source += pitch;
            destination += 2;
        }
        destination += 16;
        for (row = 0; row < 8; row++) {
            destination[0] = source[0];
            destination[1] = source[1];
            destination[16] = source[2];
            destination[17] = source[3];
            source += pitch;
            destination += 2;
        }
        break;
    }
    case 2:
    case 6: {
        const u16* source = (const u16*)reference;
        u32* destination = (u32*)context->destination;
        s32 pitch = context->reference_stride / 2;

        for (row = 0; row < 16; row++) {
            u32 word0 = *(const u32*)(source + 1);
            u32 word1 = *(const u32*)(source + 3);
            u32 word2 = *(const u32*)(source + 5);
            destination[0] = ((u32)source[0] << 16) | (word0 >> 16);
            destination[1] = (word0 << 16) | (word1 >> 16);
            destination[16] = (word1 << 16) | (word2 >> 16);
            destination[17] = (word2 << 16) | source[7];
            source += pitch;
            destination += 2;
            if (row == 7) {
                destination += 16;
            }
        }
        break;
    }
    case 1:
    case 5: {
        u8* destination = context->destination;
        s32 pitch = context->reference_stride;

        for (row = 0; row < 16; row++) {
            u32 word0 = *(const u32*)(reference - 1);
            u32 word1 = *(const u32*)(reference + 3);
            u32 word2 = *(const u32*)(reference + 7);
            u32 word3 = *(const u32*)(reference + 11);
            u32* output = (u32*)destination;
            output[0] = (word0 << 8) | (word1 >> 24);
            output[1] = (word1 << 8) | (word2 >> 24);
            output[16] = (word2 << 8) | (word3 >> 24);
            output[17] = (word3 << 8) | reference[15];
            reference += pitch;
            destination += 8;
            if (row == 7) {
                destination += 0x40;
            }
        }
        break;
    }
    case 3:
    case 7: {
        u8* destination = context->destination;
        s32 pitch = context->reference_stride;

        for (row = 0; row < 16; row++) {
            u32 word0 = *(const u32*)(reference - 3);
            u32 word1 = *(const u32*)(reference + 1);
            u32 word2 = *(const u32*)(reference + 5);
            u32 word3 = *(const u32*)(reference + 9);
            u32 word4 = *(const u32*)(reference + 13);
            u32* output = (u32*)destination;
            output[0] = (word0 << 24) | (word1 >> 8);
            output[1] = (word1 << 24) | (word2 >> 8);
            output[16] = (word2 << 24) | (word3 >> 8);
            output[17] = (word3 << 24) | (word4 >> 8);
            reference += pitch;
            destination += 8;
            if (row == 7) {
                destination += 0x40;
            }
        }
        break;
    }
    }
}

void MPVMC16_Init(MPVMC16Context* context)
{
    context->functions16[0] = mpvmc16_oneref1p_func_table[0];
    context->functions16[1] = mpvmc16_oneref1p_func_table[1];
    context->functions16[2] = mpvmc16_oneref1p_func_table[2];
    context->functions16[3] = mpvmc16_oneref1p_func_table[3];
}
