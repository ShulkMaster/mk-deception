#include "sofdec/cft.h"

#include "runtime/asm_sequences.inc"

static const char cft_version_string[] =
    "\nCRI CFT/GC Ver.1.57 Build:Sep  3 2004 11:38:10\n";
const char* CFT_version = cft_version_string;

static u32 gqr_save = 0;
static float cr_r[256];
static float cr_g[256];
static float cb_b[256];
static float cb_g[256];
static float y__r[256] __attribute__((aligned(32)));
static const char* CFT_dummy;

static inline float clamp_table_value(float value)
{
    return value < 0.0f ? 0.0f : (value > 255.0f ? 255.0f : value);
}

static inline void make_chroma_tables(CFTArgbTable table)
{
    s32 i;

    for (i = 0; i < 256; i++) {
        table[1][i][3] = 2.017f * (float)(i - 128) + 0.5f;
        table[1][i][2] = -0.392f * (float)(i - 128) + 0.5f;
        table[1][i][1] = 0.0f;
        table[1][i][0] = 0.0f;
        table[2][i][3] = 0.0f;
        table[2][i][2] = -0.813f * (float)(i - 128) + 0.5f;
        table[2][i][1] = 1.596f * (float)(i - 128) + 0.5f;
        table[2][i][0] = 0.0f;
    }
}

/* TODO: [breakthrough] 94.28%; indexed chroma loop recovers retail conversion homes;
 * channel owners and luminance conversion scheduling remain. */
void CFT_MakeArgb8888Alp3211Tbl(
    CFTArgbTable table, u8 alpha0, u8 alpha1, u8 alpha2)
{
    s32 i;
    s32 luminance_sample;
    s32 middle_pairs;
    s32 high_pairs;
    float* y_table;

    make_chroma_tables(table);
    for (i = 0; i < 48; i++) {
        table[0][i][3] = -16.0f * (255.0f / 219.0f) + 0.5f;
        table[0][i][2] = -16.0f * (255.0f / 219.0f) + 0.5f;
        table[0][i][1] = -16.0f * (255.0f / 219.0f) + 0.5f;
        table[0][i][0] = alpha0;
    }
    y_table = &table[0][48][0];
    luminance_sample = 48;
    for (middle_pairs = 0; middle_pairs < 41; middle_pairs++) {
        float luminance0 =
            (255.0f / 55.0f) * clamp_table_value((float)luminance_sample - 68.0f) + 0.5f;
        float next_sample;
        float luminance1;

        y_table[3] = luminance0;
        next_sample = (float)++luminance_sample - 68.0f;
        y_table[2] = luminance0;
        y_table[1] = luminance0;
        y_table[0] = alpha1;
        luminance1 = (255.0f / 55.0f) * clamp_table_value(next_sample) + 0.5f;
        ++luminance_sample;
        y_table[7] = luminance1;
        y_table[6] = luminance1;
        y_table[5] = luminance1;
        y_table[4] = alpha1;
        y_table += 8;
    }
    y_table = &table[0][130][0];
    luminance_sample = 130;
    for (high_pairs = 0; high_pairs < 63; high_pairs++) {
        float luminance0 =
            2.2972972f * clamp_table_value(247.0f - (float)luminance_sample) + 0.5f;
        float next_sample;
        float luminance1;

        y_table[3] = luminance0;
        next_sample = 247.0f - (float)++luminance_sample;
        y_table[2] = luminance0;
        y_table[1] = luminance0;
        y_table[0] = alpha2;
        luminance1 = 2.2972972f * clamp_table_value(next_sample) + 0.5f;
        ++luminance_sample;
        y_table[7] = luminance1;
        y_table[6] = luminance1;
        y_table[5] = luminance1;
        y_table[4] = alpha2;
        y_table += 8;
    }
}

/* TODO: [breakthrough] 94.55%; indexed chroma loop recovers retail 0x30 frame;
 * separate channel owners and high-luminance conversion homes remain. */
void CFT_MakeArgb8888Alp3110Tbl(
    CFTArgbTable table, u8 alpha0, u8 alpha1, u8 alpha2)
{
    s32 i;
    s32 high_pairs;
    float* y_table;

    make_chroma_tables(table);
    for (i = 0; i < 9; i++) {
        table[0][i][3] = 0.0f;
        table[0][i][2] = 0.0f;
        table[0][i][1] = 0.0f;
        table[0][i][0] = alpha0;
    }
    y_table = &table[0][9][0];
    i = 9;
    for (; i < 134; i++) {
        float luminance =
            (255.0f / 110.0f) * clamp_table_value((float)i - 16.0f) + 0.5f;

        y_table[3] = luminance;
        y_table[2] = luminance;
        y_table[1] = luminance;
        y_table[0] = alpha1;
        y_table += 4;
    }
    y_table = &table[0][134][0];
    i = 134;
    for (high_pairs = 0; high_pairs < 61; high_pairs++) {
        float luminance0 =
            (255.0f / 110.0f) * clamp_table_value(251.0f - (float)i) + 0.5f;
        float next_chroma;
        float luminance1;

        y_table[3] = luminance0;
        next_chroma = 251.0f - (float)++i;
        y_table[2] = luminance0;
        y_table[1] = luminance0;
        y_table[0] = alpha2;
        luminance1 =
            (255.0f / 110.0f) * clamp_table_value(next_chroma) + 0.5f;
        ++i;
        y_table[7] = luminance1;
        y_table[6] = luminance1;
        y_table[5] = luminance1;
        y_table[4] = alpha2;
        y_table += 8;
    }
}

/* TODO: [near miss] 96.79%; six cursor/constant entry-schedule rows
 * remain; stop until new compiler-boundary evidence. */
void CFT_MakeArgb8888AlpLumiTbl(
    s32 reverse, s32 low, s32 high, CFTArgbTable table)
{
    s32 i;
    s32 range;
    float scale;
    float (*y)[4] = table[0];
    float (*cb)[4] = table[1];
    float (*cr)[4] = table[2];
    float rounding_bias = 0.5f;
    s32 component;

    for (component = 0; component < 256; component++) {
        float luminance;

        luminance = 1.16400003f * (float)(component - 16) + rounding_bias;

        (*y)[3] = luminance;
        (*y)[2] = luminance;
        (*y)[1] = luminance;
        (*cb)[3] = 2.017f * (float)(component - 128) + rounding_bias;
        (*cb)[2] = -0.392f * (float)(component - 128) + rounding_bias;
        (*cb)[1] = 0.0f;
        (*cb)[0] = 0.0f;
        (*cr)[3] = 0.0f;
        (*cr)[2] = -0.813f * (float)(component - 128) + rounding_bias;
        (*cr)[1] = 1.596f * (float)(component - 128) + rounding_bias;
        (*cr)[0] = 0.0f;
        y++;
        cb++;
        cr++;
    }

    y = table[0];
    range = high - low;
    scale = 255.0f / (float)range;
    if (reverse == 1) {
        for (i = 0; i < 256; i++) {
            if (i < low) {
                y[i][0] = 255.0f;
            } else if (i > high) {
                y[i][0] = 0.0f;
            } else {
                y[i][0] = scale * (float)(range - (i - low));
            }
        }
    } else {
        for (i = 0; i < 256; i++) {
            if (i < low) {
                y[i][0] = 0.0f;
            } else if (i > high) {
                y[i][0] = 255.0f;
            } else {
                y[i][0] = scale * (float)(i - low);
            }
        }
    }
}

/* TODO: [blocked] 33.27%; vendor cache/update assembly boundary needs specific authorization; retain C fallback. */
void CFT_Ycc420plnToY84C44(
    const CFTYcc420Planar* src,
    u8* dst_y,
    u8* dst_c,
    s32 dst_y_stride,
    s32 height)
{
    s32 tile_y;
    s32 tile_x;
    f64* y_output = (f64*)dst_y;
    u32* c_output = (u32*)dst_c;
    const f64* y_row0 = (const f64*)src->y;
    const u32* cb_row0 = (const u32*)src->cb;
    const u32* cr_row0 = (const u32*)src->cr;
    s32 chroma_count = src->y_stride / 2 / 4;
    s32 chroma_skip = (src->cb_stride - src->y_stride / 2) / 4;

    for (tile_y = 0; tile_y < height / 4; tile_y++) {
        const f64* row0 = y_row0;
        const f64* row1 = (const f64*)((const u8*)row0 + src->y_stride);
        const f64* row2 = (const f64*)((const u8*)row1 + src->y_stride);
        const f64* row3 = (const f64*)((const u8*)row2 + src->y_stride);
        s32 y_count = src->y_stride / 8;

        while (y_count-- > 0) {
            *y_output++ = *row0++;
            *y_output++ = *row1++;
            *y_output++ = *row2++;
            *y_output++ = *row3++;
        }
        y_output += ((dst_y_stride - src->y_stride) / 8) * 4;
        y_row0 = (const f64*)((const u8*)row0 + ((src->y_stride * 3) / 8) * 8);
    }

    for (tile_y = 0; tile_y < height / 8; tile_y++) {
        const u32* cr0 = cr_row0;
        const u32* cr1 = (const u32*)((const u8*)cr0 + src->cb_stride);
        const u32* cr2 = (const u32*)((const u8*)cr1 + src->cb_stride);
        const u32* cr3 = (const u32*)((const u8*)cr2 + src->cb_stride);
        const u32* cb0 = cb_row0;
        const u32* cb1 = (const u32*)((const u8*)cb0 + src->cb_stride);
        const u32* cb2 = (const u32*)((const u8*)cb1 + src->cb_stride);
        const u32* cb3 = (const u32*)((const u8*)cb2 + src->cb_stride);

        for (tile_x = 0; tile_x < chroma_count; tile_x++) {
            u32 cb;
            u32 cr;
#define STORE_CHROMA_ROW(cbRow, crRow)                                      \
            do {                                                            \
                cb = *(cbRow)++;                                            \
                cr = *(crRow)++;                                            \
                *c_output++ = (cb & 0xff000000) |                           \
                    ((cr >> 8) & 0x00ff0000) |                              \
                    ((cb >> 8) & 0x0000ff00) | ((cr >> 16) & 0xff);         \
                *c_output++ = ((cb << 16) & 0xff000000) |                   \
                    ((cr << 8) & 0x00ff0000) |                              \
                    ((cb << 8) & 0x0000ff00) | (cr & 0xff);                 \
            } while (0)
            STORE_CHROMA_ROW(cb0, cr0);
            STORE_CHROMA_ROW(cb1, cr1);
            STORE_CHROMA_ROW(cb2, cr2);
            STORE_CHROMA_ROW(cb3, cr3);
#undef STORE_CHROMA_ROW
        }
        c_output += chroma_skip * 8;
        cb_row0 = cb0 + (src->cb_stride / 4) * 3 + chroma_skip;
        cr_row0 = cr0 + (src->cb_stride / 4) * 3 + chroma_skip;
    }
}

static void cnvDynamicYcc420plnToArgb8888(
    const CFTYcc420Planar* src,
    const CFTArgb8888Output* dst,
    CFTArgbTable table);
static void cnvStaticYcc420plnToArgb8888(
    const CFTYcc420Planar* src, const CFTArgb8888Output* dst);

#pragma push
#pragma peephole off
#pragma scheduling off
static void cnvDynamicYcc420plnToArgb8888(
    const CFTYcc420Planar* src,
    const CFTArgb8888Output* dst,
    CFTArgbTable table)
{
    u32 slot[8];

    asm {
        SEQ_cnvDynamicYcc420plnToArgb8888_Body()
    }
}
#pragma pop

#pragma push
#pragma peephole off
#pragma scheduling off
static void cnvStaticYcc420plnToArgb8888(
    const CFTYcc420Planar* src, const CFTArgb8888Output* dst)
{
    u32 slot[8];

    asm {
        SEQ_cnvStaticYcc420plnToArgb8888_Body()
    }
}
#pragma pop

void CFT_Ycc420plnToArgb8888(
    const CFTYcc420Planar* src,
    const CFTArgb8888Output* dst,
    CFTArgbTable table)
{
    if (table == 0) {
        cnvStaticYcc420plnToArgb8888(src, dst);
    } else {
        cnvDynamicYcc420plnToArgb8888(src, dst, table);
    }
}

/* TODO: [near miss] 99.64%; table setup follows retail .bss order (cr_r..y__r, needed by the asm
 * converters); retail emits the five table addi in y__r-first order. */
void CFT_Ycc420plnToArgb8888Init(void)
{
    s32 i;
    float* y_luminance;
    float* cb_green;
    float* cb_blue;
    float* cr_red;
    float* cr_green;
    float value;
    s32 offset;

    cr_red = cr_r;
    cr_green = cr_g;
    cb_blue = cb_b;
    cb_green = cb_g;
    y_luminance = y__r;
    CFT_dummy = CFT_version;
    for (i = 0; i != 256; i++) {
        offset = i - 16;
        value = 1.164f * (float)offset;
        offset = i - 128;
        *y_luminance++ = value;
        *cb_green++ = -0.392f * (float)offset;
        *cb_blue++ = 2.017f * (float)offset;
        *cr_red++ = 1.596f * (float)offset;
        *cr_green++ = -0.813f * (float)offset;
    }
}
