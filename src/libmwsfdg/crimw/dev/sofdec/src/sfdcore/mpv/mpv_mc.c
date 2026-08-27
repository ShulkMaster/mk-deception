#include "sofdec/mpv_mc.h"

static const MPVMCFunction mpvmc_oneref1p_func_table[4] = {0, 0, 0, 0};

static inline u32 mpvmc_pack_avg4(const u8* reference0, const u8* reference1)
{
    return (((reference0[0] + (reference0[1] + reference1[0] + reference1[1] + 2)) << 22) &
            0xFF000000) |
           (((reference0[1] + (reference0[2] + reference1[1] + reference1[2] + 2)) << 14) &
            0x00FF0000) |
           (((reference0[2] + (reference0[3] + reference1[2] + reference1[3] + 2)) << 6) &
            0x0000FF00) |
           (((reference0[3] + (reference0[4] + reference1[3] + reference1[4] + 2)) >> 2) &
            0x000000FF);
}

static inline u32 mpvmc_avg_words(u32 left, u32 right)
{
    u32 different = left ^ right;
    return (left & right) + ((different & 0xFEFEFEFE) >> 1) +
           (different & 0x01010101);
}

static inline void mpvmc_h2_align1_row(const u8* reference, u8* destination)
{
    const u32* words = (const u32*)reference;
    u32 source0 = (words[0] << 8) | (words[1] >> 24);
    u32 adjacent0 = (words[0] << 16) | (words[1] >> 16);
    u32 source1 = (words[1] << 8) | reference[8];
    u32 adjacent1 = (words[1] << 16) | *(const unsigned short*)(reference + 8);
    ((u32*)destination)[0] = mpvmc_avg_words(source0, adjacent0);
    ((u32*)destination)[1] = mpvmc_avg_words(source1, adjacent1);
}

static inline void mpvmc_h2_align2_row(const u8* reference, u8* destination)
{
    const u32* words = (const u32*)reference;
    u32 source0 = (words[0] << 16) | (words[1] >> 16);
    u32 adjacent0 = (words[0] << 24) | (words[1] >> 8);
    u32 source1 = (words[1] << 16) | (words[2] >> 16);
    u32 adjacent1 = (words[1] << 24) | (words[2] >> 8);
    ((u32*)destination)[0] = mpvmc_avg_words(source0, adjacent0);
    ((u32*)destination)[1] = mpvmc_avg_words(source1, adjacent1);
}

static inline void mpvmc_copy_shift1_row(const u8* reference, u8* destination)
{
    const u32* words = (const u32*)reference;
    ((u32*)destination)[0] = (words[0] << 8) | (words[1] >> 24);
    ((u32*)destination)[1] = (words[1] << 8) | reference[8];
}

static inline void mpvmc_copy_shift2_row(const u16* reference, u8* destination)
{
    u32 middle = *(const u32*)(reference + 1);
    ((u32*)destination)[0] = ((u32)reference[0] << 16) | (middle >> 16);
    ((u32*)destination)[1] = (middle << 16) |
                             reference[3];
}

static inline void mpvmc_copy_shift3_row(const u8* reference, u8* destination)
{
    const u32* words = (const u32*)reference;
    ((u32*)destination)[0] = (words[0] << 24) | (words[1] >> 8);
    ((u32*)destination)[1] = (words[1] << 24) | (words[2] >> 8);
}

void MPVMC08_OneRef4p_TuneC(MPVMCContext* context)
{
    int row;
    u32 stride = context->reference_stride;
    const u8* reference0 = context->reference0;
    const u8* reference1 = context->reference1;
    u8* destination = context->destination;

    for (row = 0; row < 8; row++) {
        ((u32*)destination)[0] = mpvmc_pack_avg4(reference0, reference1);
        ((u32*)destination)[1] = mpvmc_pack_avg4(reference0 + 4, reference1 + 4);
        destination += 8;
        reference0 += stride;
        reference1 += stride;
    }
}

void MPVMC08_OneRefH2_TuneC(MPVMCContext* context)
{
    int row;
    int alignment = (u32)context->reference0 & 3;
    u32 stride = context->reference_stride;
    u8* destination = context->destination;
    const u8* reference = context->reference0;

    switch (alignment) {
    case 0:
        for (row = 0; row < 8; row++) {
            const u32* words = (const u32*)reference;
            u32 adjacent0 = (words[0] << 8) | (words[1] >> 24);
            u32 adjacent1 = (words[1] << 8) | reference[8];
            ((u32*)destination)[0] = mpvmc_avg_words(words[0], adjacent0);
            ((u32*)destination)[1] = mpvmc_avg_words(words[1], adjacent1);
            reference += stride;
            destination += 8;
        }
        break;
    case 1:
        reference -= 1;
        for (row = 0; row < 4; row++) {
            mpvmc_h2_align1_row(reference, destination);
            reference += stride;
            mpvmc_h2_align1_row(reference, destination + 8);
            reference += stride;
            destination += 16;
        }
        break;
    case 2:
        reference -= 2;
        for (row = 0; row < 4; row++) {
            mpvmc_h2_align2_row(reference, destination);
            reference += stride;
            mpvmc_h2_align2_row(reference, destination + 8);
            reference += stride;
            destination += 16;
        }
        break;
    default:
        reference -= 3;
        for (row = 0; row < 8; row++) {
            const u32* words = (const u32*)reference;
            u32 source0 = (words[0] << 24) | (words[1] >> 8);
            u32 source1 = (words[1] << 24) | (words[2] >> 8);
            ((u32*)destination)[0] = mpvmc_avg_words(source0, words[1]);
            ((u32*)destination)[1] = mpvmc_avg_words(source1, words[2]);
            reference += stride;
            destination += 8;
        }
        break;
    }
}

void MPVMC08_OneRefV2_TuneC(MPVMCContext* context)
{
    int row;
    int alignment = (u32)context->reference0 & 3;
    u32 stride = context->reference_stride;
    u8* destination = context->destination;
    const u8* reference0 = context->reference0 - alignment;
    const u8* reference1 = context->reference1 - alignment;

    switch (alignment) {
    case 0:
        for (row = 0; row < 4; row++) {
            const u32* words0 = (const u32*)reference0;
            const u32* words1 = (const u32*)reference1;
            ((u32*)destination)[0] = mpvmc_avg_words(words0[0], words1[0]);
            ((u32*)destination)[1] = mpvmc_avg_words(words0[1], words1[1]);
            reference0 += stride;
            reference1 += stride;
            words0 = (const u32*)reference0;
            words1 = (const u32*)reference1;
            ((u32*)destination)[2] = mpvmc_avg_words(words0[0], words1[0]);
            ((u32*)destination)[3] = mpvmc_avg_words(words0[1], words1[1]);
            reference0 += stride;
            reference1 += stride;
            destination += 16;
        }
        break;
    case 1:
        for (row = 0; row < 8; row++) {
            const u32* words0 = (const u32*)reference0;
            const u32* words1 = (const u32*)reference1;
            u32 a0 = (words0[0] << 8) | (words0[1] >> 24);
            u32 a1 = (words0[1] << 8) | reference0[8];
            u32 b0 = (words1[0] << 8) | (words1[1] >> 24);
            u32 b1 = (words1[1] << 8) | reference1[8];
            ((u32*)destination)[0] = mpvmc_avg_words(a0, b0);
            ((u32*)destination)[1] = mpvmc_avg_words(a1, b1);
            reference0 += stride;
            reference1 += stride;
            destination += 8;
        }
        break;
    case 2:
        for (row = 0; row < 8; row++) {
            const u32* words0 = (const u32*)reference0;
            const u32* words1 = (const u32*)reference1;
            u32 a0 = (words0[0] << 16) | (words0[1] >> 16);
            u32 a1 = (words0[1] << 16) | *(const unsigned short*)(reference0 + 8);
            u32 b0 = (words1[0] << 16) | (words1[1] >> 16);
            u32 b1 = (words1[1] << 16) | *(const unsigned short*)(reference1 + 8);
            ((u32*)destination)[0] = mpvmc_avg_words(a0, b0);
            ((u32*)destination)[1] = mpvmc_avg_words(a1, b1);
            reference0 += stride;
            reference1 += stride;
            destination += 8;
        }
        break;
    default:
        for (row = 0; row < 8; row++) {
            const u32* words0 = (const u32*)reference0;
            const u32* words1 = (const u32*)reference1;
            u32 a0 = (words0[0] << 24) | (words0[1] >> 8);
            u32 a1 = (words0[1] << 24) | (words0[2] >> 8);
            u32 b0 = (words1[0] << 24) | (words1[1] >> 8);
            u32 b1 = (words1[1] << 24) | (words1[2] >> 8);
            ((u32*)destination)[0] = mpvmc_avg_words(a0, b0);
            ((u32*)destination)[1] = mpvmc_avg_words(a1, b1);
            reference0 += stride;
            reference1 += stride;
            destination += 8;
        }
        break;
    }
}

void MPVMC08_OneRef1p_TuneC(MPVMCContext* context)
{
    int row;
    int alignment = (u32)context->reference0 & 7;
    u32 stride = context->reference_stride;
    const u8* reference = context->reference0;
    u8* destination = context->destination;

    switch (alignment) {
    case 0: {
        double row0;
        double row1;
        double row2;
        double row3;
        double row4;
        double row5;
        double row6;
        double row7;

        row0 = *(const double*)reference;
        reference += stride;
        row1 = *(const double*)reference;
        reference += stride;
        row2 = *(const double*)reference;
        reference += stride;
        row3 = *(const double*)reference;
        reference += stride;
        row4 = *(const double*)reference;
        reference += stride;
        row5 = *(const double*)reference;
        reference += stride;
        row6 = *(const double*)reference;
        reference += stride;
        row7 = *(const double*)reference;
        ((double*)destination)[0] = row0;
        ((double*)destination)[1] = row1;
        ((double*)destination)[2] = row2;
        ((double*)destination)[3] = row3;
        ((double*)destination)[4] = row4;
        ((double*)destination)[5] = row5;
        ((double*)destination)[6] = row6;
        ((double*)destination)[7] = row7;
        break;
    }
    case 4:
        for (row = 0; row < 8; row++) {
            ((u32*)destination)[0] = ((const u32*)reference)[0];
            ((u32*)destination)[1] = ((const u32*)reference)[1];
            reference += stride;
            destination += 8;
        }
        break;
    case 2:
    case 6: {
        const u16* source = (const u16*)reference;
        u32 pitch = stride / 2;

        for (row = 0; row < 2; row++) {
            mpvmc_copy_shift2_row(source, destination);
            source += pitch;
            mpvmc_copy_shift2_row(source, destination + 8);
            source += pitch;
            mpvmc_copy_shift2_row(source, destination + 16);
            source += pitch;
            mpvmc_copy_shift2_row(source, destination + 24);
            source += pitch;
            destination += 32;
        }
        break;
    }
    case 1:
    case 5:
        reference -= 1;
        for (row = 0; row < 8; row++) {
            mpvmc_copy_shift1_row(reference, destination);
            reference += stride;
            destination += 8;
        }
        break;
    case 3:
    case 7:
        reference -= 3;
        for (row = 0; row < 8; row++) {
            mpvmc_copy_shift3_row(reference, destination);
            reference += stride;
            destination += 8;
        }
        break;
    }
}

void MPVMC08_Init(MPVMCFunction functions[4])
{
    functions[0] = mpvmc_oneref1p_func_table[0];
    functions[1] = mpvmc_oneref1p_func_table[1];
    functions[2] = mpvmc_oneref1p_func_table[2];
    functions[3] = mpvmc_oneref1p_func_table[3];
}
