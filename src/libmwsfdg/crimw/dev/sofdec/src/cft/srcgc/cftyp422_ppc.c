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

void CFT_Ycc420plnToY84C44(
    const CFTYcc420Planar* src,
    u8* dst_y,
    u8* dst_c,
    s32 dst_y_stride,
    s32 height)
{
    register s32 ofs;
    s32 n;
    f64* y3;
    s32 yskip;
    s32 dskip;
    f64* y2;
    f64* y1;
    register f64* d;
    register f64* y0;
    s32 yw2;
    s32 hblk;
    s32 cnt;
    s32 i;
    register f64 w0;
    register f64 w1;
    register f64 w2;
    register f64 w3;
    u32* cbp3;
    u32* cbp2;
    u32* cbp1;
    u32* crp3;
    u32* crp2;
    u32* crp1;
    s32 o3;
    s32 o2;
    s32 o1;
    s32 cskip;
    u32* cbp0;
    u32* crp0;
    u32 crv;
    u32 cbv;
    register u32* c;
    s32 ywidth;
    s32 yw3;

    ywidth = src->y_stride;
    yw2 = ywidth * 2;
    y0 = (f64*)src->y;
    d = (f64*)dst_y - 1;
    yw3 = ywidth * 3;
    yskip = yw3 / 8 * 8;
    cnt = ywidth / 8;
    hblk = height / 4;
    dskip = (dst_y_stride - ywidth) / 8 * 32;
    ofs = 8;
    for (i = 0; i < hblk; i++) {
        y1 = (f64*)(ywidth + (u32)y0);
        y2 = (f64*)(yw2 + (u32)y0);
        y3 = (f64*)(yw3 + (u32)y0);
        n = cnt;
        while (n-- > 0) {
            asm { SEQ_CFT_Ycc420plnToY84C44_DcbzY(d, ofs) }
            w0 = *y0;
            w1 = *y1;
            w2 = *y2;
            w3 = *y3;
            asm { SEQ_CFT_Ycc420plnToY84C44_DcbtY(y0, ofs) }
            asm { SEQ_CFT_Ycc420plnToY84C44_StoreY0(w0, d) }
            asm { SEQ_CFT_Ycc420plnToY84C44_StoreY1(w1, d) }
            asm { SEQ_CFT_Ycc420plnToY84C44_StoreY2(w2, d) }
            asm { SEQ_CFT_Ycc420plnToY84C44_StoreY3(w3, d) }
            y0++;
            y3++;
            y2++;
            y1++;
        }
        d = (f64*)((u8*)d + dskip);
        y0 = (f64*)((u8*)y0 + yskip);
    }

    cnt = src->y_stride / 2 / 4;
    cskip = (src->cb_stride - src->y_stride / 2) / 4;
    hblk = height / 2 / 4;
    ywidth = src->cb_stride / 4;
    yw3 = ywidth * 3;
    o1 = ywidth * 4;
    o2 = ywidth * 8;
    o3 = yw3 * 4;
    c = (u32*)dst_c - 1;
    cbp0 = (u32*)src->cb;
    crp0 = (u32*)src->cr;
    ofs = 4;
    for (i = 0; i < hblk; i++) {
        crp1 = (u32*)(o1 + (u32)crp0);
        crp2 = (u32*)(o2 + (u32)crp0);
        crp3 = (u32*)(o3 + (u32)crp0);
        cbp1 = (u32*)(o1 + (u32)cbp0);
        cbp2 = (u32*)(o2 + (u32)cbp0);
        cbp3 = (u32*)(o3 + (u32)cbp0);
        for (n = 0; n < cnt; n++) {
            asm { SEQ_CFT_Ycc420plnToY84C44_DcbzC(c, ofs) }
#define STORE_CHROMA_ROW(cbRow, crRow)                                       \
            crv = *(crRow)++;                                                \
            cbv = *(cbRow)++;                                                \
            *++c = (cbv & 0xFF000000) | ((crv >> 8) & 0x00FF0000) |          \
                ((cbv >> 8) & 0xFF00) | ((crv >> 16) & 0xFF);                \
            *++c = ((cbv << 16) & 0xFF000000) | ((crv << 8) & 0x00FF0000) |  \
                ((cbv << 8) & 0xFF00) | (crv & 0xFF)
            STORE_CHROMA_ROW(cbp0, crp0);
            STORE_CHROMA_ROW(cbp1, crp1);
            STORE_CHROMA_ROW(cbp2, crp2);
            STORE_CHROMA_ROW(cbp3, crp3);
#undef STORE_CHROMA_ROW
        }
        c += cskip * 8;
        cbp0 += yw3 + cskip;
        crp0 += yw3 + cskip;
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
