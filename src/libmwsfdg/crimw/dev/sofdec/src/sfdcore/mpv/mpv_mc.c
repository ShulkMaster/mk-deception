#include "sofdec/mpv_mc.h"

static const MPVMCFunction mpvmc_oneref1p_func_table[4] = {0, 0, 0, 0};


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

/* Pack eight rounded 2x2 pixel averages from the two reference rows. */
void MPVMC08_OneRef4p_TuneC(MPVMCContext* context)
{
    s32 row;
    s32 stride;
    const u8* reference0;
    const u8* reference1;
    u32* destination;
    s32 top0, bottom0, top1, bottom1, top2, bottom2;
    u32 sum0, sum1, sum2, sum3, sum4, sum5, sum6, sum7;

    stride = context->reference_stride;
    reference0 = context->reference0;
    reference1 = context->reference1;
    destination = (u32*)context->destination;
    for (row = 0; row < 8; row++) {
        top0 = reference0[0];
        bottom0 = reference1[0];
        __dcbt((void*)reference1, stride);
        top1 = reference0[1];
        bottom1 = reference1[1];
        sum0 = (u32)top0 + (u32)top1 + (u32)bottom0 + (u32)bottom1 + 2;
        top2 = reference0[2];
        bottom2 = reference1[2];
        sum1 = (u32)top1 + (u32)top2 + (u32)bottom1 + (u32)bottom2 + 2;
        top0 = reference0[3];
        bottom0 = reference1[3];
        sum2 = (u32)top2 + (u32)top0 + (u32)bottom2 + (u32)bottom0 + 2;
        top1 = reference0[4];
        bottom1 = reference1[4];
        sum3 = (u32)top0 + (u32)top1 + (u32)bottom0 + (u32)bottom1 + 2;
        top2 = reference0[5];
        bottom2 = reference1[5];
        sum4 = (u32)top1 + (u32)top2 + (u32)bottom1 + (u32)bottom2 + 2;
        top0 = reference0[6];
        bottom0 = reference1[6];
        sum5 = (u32)top2 + (u32)top0 + (u32)bottom2 + (u32)bottom0 + 2;
        top1 = reference0[7];
        bottom1 = reference1[7];
        sum6 = (u32)top0 + (u32)top1 + (u32)bottom0 + (u32)bottom1 + 2;
        top2 = reference0[8];
        bottom2 = reference1[8];
        sum7 = (u32)top1 + (u32)top2 + (u32)bottom1 + (u32)bottom2 + 2;
        destination[0] = (((sum0 << 22) & 0xFF000000) | ((sum1 << 14) & 0x00FF0000) | ((sum2 << 6) & 0x0000FF00) | ((sum3 >> 2) & 0x000000FF));
        destination[1] = (((sum4 << 22) & 0xFF000000) | ((sum5 << 14) & 0x00FF0000) | ((sum6 << 6) & 0x0000FF00) | ((sum7 >> 2) & 0x000000FF));
        reference0 += stride;
        reference1 += stride;
        destination += 2;
    }
}

/* TODO: [blocked] 29.78%; vendor bit-insert assembly boundary lacks specific authorization; retain C fallback. */
void MPVMC08_OneRefH2_TuneC(MPVMCContext* context)
{
    int row;
    int alignment = (unsigned long)context->reference0 & 3;
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

/* TODO: [near miss] 92.94%; all alignment operations agree; stop at aligned average scheduling and case1/2 coloring. */
void MPVMC08_OneRefV2_TuneC(MPVMCContext *context)
{
    s32 row;
    u32 *destination;
    const u8 *reference0;
    const u8 *reference1;
    s32 stride;

    u32 x0, x1;
    u32 w0, a0, w1, a1, w2, a2;
    u32 m1 = 0xFEFEFEFE;
    u32 m2 = 0x01010101;

    reference0 = context->reference0;
    reference1 = context->reference1;
    destination = (u32*)context->destination;
    stride = context->reference_stride;
    switch ((u32)reference0 & 3) {
    case 0:
        for (row = 0; row < 8; row++) {
            w0 = ((const u32*)reference0)[0];
            a0 = ((const u32*)reference1)[0];
            w1 = ((const u32*)reference0)[1];
            a1 = ((const u32*)reference1)[1];
            x0 = w0 ^ a0;
            x1 = w1 ^ a1;
            destination[0] = (w0 & a0) + ((x0 & m1) >> 1) + (x0 & m2);
            destination[1] = (w1 & a1) + (((x1 & m1) >> 1) + (x1 & m2));
            reference0 += stride;
            reference1 += stride;
            destination += 2;
        }
        break;
    case 1:
        reference0 -= 1;
        reference1 -= 1;
        for (row = 0; row < 8; row++) {
            w1 = ((const u32*)reference0)[1];
            a1 = ((const u32*)reference1)[1];
            w0 = ((const u32*)reference0)[0];
            w2 = reference0[8];
            reference0 += stride;
            a0 = ((const u32*)reference1)[0];
            a2 = reference1[8];
            reference1 += stride;
            w0 = (w0 << 8) | (w1 >> 24);
            a0 = (a0 << 8) | (a1 >> 24);
            w2 = (w1 << 8) | w2;
            a2 = (a1 << 8) | a2;
            x0 = w0 ^ a0;
            x1 = w2 ^ a2;
            destination[0] = (w0 & a0) + ((x0 & m1) >> 1) + (x0 & m2);
            destination[1] = (w2 & a2) + ((x1 & m1) >> 1) + (x1 & m2);
            destination += 2;
        }
        break;
    case 2:
        reference0 -= 2;
        reference1 -= 2;
        for (row = 0; row < 8; row++) {
            w1 = ((const u32*)reference0)[1];
            a1 = ((const u32*)reference1)[1];
            w0 = ((const u32*)reference0)[0];
            w2 = *(const u16*)(reference0 + 8);
            reference0 += stride;
            a0 = ((const u32*)reference1)[0];
            a2 = *(const u16*)(reference1 + 8);
            reference1 += stride;
            w0 = (w0 << 16) | (w1 >> 16);
            a0 = (a0 << 16) | (a1 >> 16);
            w2 = (w1 << 16) | w2;
            a2 = (a1 << 16) | a2;
            x0 = w0 ^ a0;
            x1 = w2 ^ a2;
            destination[0] = (w0 & a0) + ((x0 & m1) >> 1) + (x0 & m2);
            destination[1] = (w2 & a2) + ((x1 & m1) >> 1) + (x1 & m2);
            destination += 2;
        }
        break;
    case 3:
        reference0 -= 3;
        reference1 -= 3;
        for (row = 0; row < 8; row++) {
            w1 = ((const u32*)reference0)[1];
            a1 = ((const u32*)reference1)[1];
            w0 = ((const u32*)reference0)[0];
            w2 = ((const u32*)reference0)[2];
            reference0 += stride;
            a0 = ((const u32*)reference1)[0];
            a2 = ((const u32*)reference1)[2];
            reference1 += stride;
            w0 = (w0 << 24) | (w1 >> 8);
            a0 = (a0 << 24) | (a1 >> 8);
            w2 = (w1 << 24) | (w2 >> 8);
            a2 = (a1 << 24) | (a2 >> 8);
            x0 = w0 ^ a0;
            x1 = w2 ^ a2;
            destination[0] = (w0 & a0) + ((x0 & m1) >> 1) + (x0 & m2);
            destination[1] = (w2 & a2) + ((x1 & m1) >> 1) + (x1 & m2);
            destination += 2;
        }
        break;
    }
}

/* TODO: [blocked] 24.18%; vendor indexed-update assembly lacks specific authorization; retain C fallback. */
void MPVMC08_OneRef1p_TuneC(MPVMCContext* context)
{
    int row;
    int alignment = (unsigned long)context->reference0 & 7;
    u32 stride = context->reference_stride;
    const u8* reference = context->reference0;
    u8* destination = context->destination;

    switch (alignment) {
    case 0: {
        double* double_destination = (double*)destination;
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
        double_destination[0] = row0;
        double_destination[1] = row1;
        double_destination[2] = row2;
        double_destination[3] = row3;
        double_destination[4] = row4;
        double_destination[5] = row5;
        double_destination[6] = row6;
        double_destination[7] = row7;
        break;
    }
    case 4: {
        u32* word_destination = (u32*)destination;
        for (row = 0; row < 8; row++) {
            word_destination[0] = ((const u32*)reference)[0];
            word_destination[1] = ((const u32*)reference)[1];
            reference += stride;
            word_destination += 2;
        }
        break;
    }
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
