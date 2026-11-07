#include "sofdec/mpv_mc.h"

#include "runtime/asm_sequences.inc"

static const MPVMCFunction mpvmc_oneref1p_func_table[4] = {0, 0, 0, 0};

typedef enum MPVMCByteMask {
    MPVMC_BYTE_HIGH_BITS = 0xFEFEFEFE,
    MPVMC_BYTE_LOW_BIT = 0x01010101
} MPVMCByteMask;


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

asm void MPVMC08_OneRefH2_TuneC(MPVMCContext* context)
{
    SEQ_MPVMC08_OneRefH2_TuneC();
}


static inline u32 mpvmc_avg_masked(u32 w, u32 a, u32 m1, u32 m2)
{
    u32 x = w ^ a;
    return (w & a) + ((x & m1) >> 1) + (x & m2);
}

void MPVMC08_OneRefV2_TuneC(MPVMCContext *context)
{
    s32 row;
    u32 *destination;
    const u8 *reference0;
    const u8 *reference1;
    s32 stride;

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
            destination[0] = mpvmc_avg_masked(w0, a0, m1, m2);
            destination[1] = mpvmc_avg_masked(w1, a1, m1, m2);
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
            destination[0] = mpvmc_avg_masked(w0, a0, m1, m2);
            destination[1] = mpvmc_avg_masked(w2, a2, m1, m2);
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
            destination[0] = mpvmc_avg_masked(w0, a0, m1, m2);
            destination[1] = mpvmc_avg_masked(w2, a2, m1, m2);
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
            destination[0] = mpvmc_avg_masked(w0, a0, m1, m2);
            destination[1] = mpvmc_avg_masked(w2, a2, m1, m2);
            destination += 2;
        }
        break;
    }
}

void MPVMC08_OneRef1p_TuneC(MPVMCContext* context)
{
    register const u8* s = context->reference0;

    asm { SEQ_MPVMC08_OneRef1p_TuneC_Dcbt(s) }
    switch ((u32)s & 7) {
    case 0: {
        register s32 st;

        asm { SEQ_MPVMC08_OneRef1p_TuneC_Copy0(st, s) }
        break;
    }
    case 4: {
        register s32 st;
        register const u8* p;
        register u32 w;

        asm { SEQ_MPVMC08_OneRef1p_TuneC_Copy4(st, p, s, w) }
        break;
    }
    case 2:
    case 6: {
        register u32 t0;
        register u32 st;

        asm { SEQ_MPVMC08_OneRef1p_TuneC_Copy2(st, s, t0) }
        break;
    }
    case 1:
    case 5: {
        register s32 st;
        register u32* d;

        asm { SEQ_MPVMC08_OneRef1p_TuneC_Copy1(st, s, d) }
        break;
    }
    case 3:
    case 7: {
        register s32 st;
        register u32* d;

        asm { SEQ_MPVMC08_OneRef1p_TuneC_Copy3(st, s, d) }
        break;
    }
    }
}

void MPVMC08_Init(MPVMCFunction functions[4])
{
    functions[0] = mpvmc_oneref1p_func_table[0];
    functions[1] = mpvmc_oneref1p_func_table[1];
    functions[2] = mpvmc_oneref1p_func_table[2];
    functions[3] = mpvmc_oneref1p_func_table[3];
}
