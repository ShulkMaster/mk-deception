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

#define CFT_CB_TBL(tbl) ((float*)((u8*)(tbl) + 0x1000))
#define CFT_CR_TBL(tbl) ((float*)((u8*)(tbl) + 0x2000))

#define CFT_MAKE_CHROMA_TBL(tbl)                                               \
    {                                                                          \
        s32 i;                                                                 \
        for (i = 0; i < 256; i++) {                                            \
            CFT_CB_TBL(tbl)[i * 4 + 3] = 2.017f * (float)(i - 128) + 0.5f;     \
            CFT_CB_TBL(tbl)[i * 4 + 2] = -0.392f * (float)(i - 128) + 0.5f;    \
            CFT_CB_TBL(tbl)[i * 4 + 1] = 0.0f;                                 \
            CFT_CB_TBL(tbl)[i * 4 + 0] = 0.0f;                                 \
            CFT_CR_TBL(tbl)[i * 4 + 3] = 0.0f;                                 \
            CFT_CR_TBL(tbl)[i * 4 + 2] = -0.813f * (float)(i - 128) + 0.5f;    \
            CFT_CR_TBL(tbl)[i * 4 + 1] = 1.596f * (float)(i - 128) + 0.5f;     \
            CFT_CR_TBL(tbl)[i * 4 + 0] = 0.0f;                                 \
        }                                                                      \
    }

/* clamp to the byte range */
#define CFT_CLIP255(x) (((x) < 0.0f) ? 0.0f : (((x) > 255.0f) ? 255.0f : (x)))

void CFT_MakeArgb8888Alp3211Tbl(
    CFTArgbTable table, u8 alpha0, u8 alpha1, u8 alpha2)
{
    float* y = (float*)table;
    float v;
    s32 i;

    CFT_MAKE_CHROMA_TBL(y);
    for (i = 0; i < 48; i++) {
        y[i * 4 + 3] = -16.0f * (255.0f / 219.0f) + 0.5f;
        y[i * 4 + 2] = -16.0f * (255.0f / 219.0f) + 0.5f;
        y[i * 4 + 1] = -16.0f * (255.0f / 219.0f) + 0.5f;
        y[i * 4 + 0] = (float)alpha0;
    }
    for (i = 48; i < 130; i++) {
        v = (255.0f / 55.0f) * CFT_CLIP255((float)i - 68.0f) + 0.5f;
        y[i * 4 + 3] = v;
        y[i * 4 + 2] = v;
        y[i * 4 + 1] = v;
        y[i * 4 + 0] = (float)alpha1;
    }
    for (i = 130; i < 256; i++) {
        v = (255.0f / 111.0f) * CFT_CLIP255(247.0f - (float)i) + 0.5f;
        y[i * 4 + 3] = v;
        y[i * 4 + 2] = v;
        y[i * 4 + 1] = v;
        y[i * 4 + 0] = (float)alpha2;
    }
}

void CFT_MakeArgb8888Alp3110Tbl(
    CFTArgbTable table, u8 alpha0, u8 alpha1, u8 alpha2)
{
    float* y = (float*)table;
    float v;
    s32 i;

    CFT_MAKE_CHROMA_TBL(y);
    for (i = 0; i < 9; i++) {
        y[i * 4 + 3] = 0.0f;
        y[i * 4 + 2] = 0.0f;
        y[i * 4 + 1] = 0.0f;
        y[i * 4 + 0] = (float)alpha0;
    }
    for (i = 9; i < 134; i++) {
        v = (255.0f / 110.0f) * CFT_CLIP255((float)i - 16.0f) + 0.5f;
        y[i * 4 + 3] = v;
        y[i * 4 + 2] = v;
        y[i * 4 + 1] = v;
        y[i * 4 + 0] = (float)alpha1;
    }
    for (i = 134; i < 256; i++) {
        v = (255.0f / 110.0f) * CFT_CLIP255(251.0f - (float)i) + 0.5f;
        y[i * 4 + 3] = v;
        y[i * 4 + 2] = v;
        y[i * 4 + 1] = v;
        y[i * 4 + 0] = (float)alpha2;
    }
}

void CFT_MakeArgb8888AlpLumiTbl(
    s32 reverse, s32 low, s32 high, CFTArgbTable table)
{
    float* y = (float*)table;
    float* cb = (float*)((u8*)y + 0x1000);
    float* cr = (float*)((u8*)y + 0x2000);
    float v;
    float scale;
    s32 range;
    s32 i;

    for (i = 0; i < 256; i++) {
        v = 1.164f * (float)(i - 16) + 0.5f;
        y[i * 4 + 3] = v;
        y[i * 4 + 2] = v;
        y[i * 4 + 1] = v;
        cb[i * 4 + 3] = 2.017f * (float)(i - 128) + 0.5f;
        cb[i * 4 + 2] = -0.392f * (float)(i - 128) + 0.5f;
        cb[i * 4 + 1] = 0.0f;
        cb[i * 4 + 0] = 0.0f;
        cr[i * 4 + 3] = 0.0f;
        cr[i * 4 + 2] = -0.813f * (float)(i - 128) + 0.5f;
        cr[i * 4 + 1] = 1.596f * (float)(i - 128) + 0.5f;
        cr[i * 4 + 0] = 0.0f;
    }
    range = high - low;
    scale = 255.0f / (float)range;
    if (reverse == 1) {
        for (i = 0; i < 256; i++) {
            if (i < low) {
                y[i * 4] = 255.0f;
            } else if (i > high) {
                y[i * 4] = 0.0f;
            } else {
                y[i * 4] = scale * (float)(range - (i - low));
            }
        }
    } else {
        for (i = 0; i < 256; i++) {
            if (i < low) {
                y[i * 4] = 0.0f;
            } else if (i > high) {
                y[i * 4] = 255.0f;
            } else {
                y[i * 4] = scale * (float)(i - low);
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

/* TODO: [near miss] 99.64179%; link-data requests fix table placement;
 * recover the earlier converter's real table-reference boundary. */
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
