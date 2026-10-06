#include "dolphin/types.h"
#include "sofdec/cft.h"
#include "sofdec/mws_ycc.h"

typedef float CFTMtx3D[3][3];
typedef unsigned char CFTConvTable[256];

extern void CFT_MakeInvConvTableCustom(CFTConvTable luma,
                                       CFTConvTable chroma_u,
                                       CFTConvTable chroma_v);
extern void CFT_MakeInverseMtx3D(CFTMtx3D matrix, CFTMtx3D inverse);
extern void CFT_MakeMtx3D(CFTMtx3D left, CFTMtx3D right,
                          CFTMtx3D product);

static CFTMtx3D cft_rgb_yuv_coeff = {
    {0.30078125f, 0.5859375f, 0.11328125f},
    {-0.171875f, -0.33984375f, 0.51171875f},
    {0.51171875f, -0.4296875f, -0.08203125f}
};

static CFTMtx3D cft_rgb_yuv_ccir601 = {
    {0.257f, 0.504f, 0.098f},
    {-0.148f, -0.291f, 0.439f},
    {0.439f, -0.368f, -0.071f}
};

static CFTMtx3D cft_basic_ccir601;
static CFTMtx3D cft_yuv_rgb_coeff;
static float* cft_ptr_cr_rgb;
static float* cft_ptr_cb_rgb;
static float* cft_ptr_y__rgb;
static CFTConvTable cft_conv_v_itbl;
static CFTConvTable cft_conv_u_itbl;
static CFTConvTable cft_conv_y_itbl;

/* TODO: [near miss] 99.26%; Y/row offset pair and steward-owned BSS placement remain. */
void CFT_MakeArgb8888ColAdjTbl(CFTArgbTable table)
{
    u8* conv_y;
    s32 offset;
    u8* conv_u;
    u8* conv_v;
    float (*yuv_coeff)[3];
    s32 index;

    cft_ptr_y__rgb = &table[0][0][0];
    cft_ptr_cb_rgb = &table[1][0][0];
    cft_ptr_cr_rgb = &table[2][0][0];
    CFT_MakeInvConvTableCustom(cft_conv_y_itbl, cft_conv_u_itbl,
                               cft_conv_v_itbl);
    CFT_MakeInverseMtx3D(cft_rgb_yuv_coeff, cft_yuv_rgb_coeff);
    yuv_coeff = cft_yuv_rgb_coeff;
    conv_u = cft_conv_u_itbl;
    conv_v = cft_conv_v_itbl;
    conv_y = cft_conv_y_itbl;

    for (index = 0; index < 256; index++) {
        offset = index * 4;

        cft_ptr_y__rgb[offset + 1] =
            cft_yuv_rgb_coeff[0][0] * (float)*conv_y;
        cft_ptr_y__rgb[offset + 2] =
            yuv_coeff[1][0] * (float)*conv_y;
        cft_ptr_y__rgb[offset + 3] =
            yuv_coeff[2][0] * (float)*conv_y;
        conv_y++;
        cft_ptr_y__rgb[offset] = 255.0f;
        cft_ptr_cb_rgb[offset + 1] = yuv_coeff[0][1] *
            ((float)*conv_u - 128.0f);
        cft_ptr_cb_rgb[offset + 2] = yuv_coeff[1][1] *
            ((float)*conv_u - 128.0f);
        cft_ptr_cb_rgb[offset + 3] = yuv_coeff[2][1] *
            ((float)*conv_u - 128.0f);
        conv_u++;
        cft_ptr_cr_rgb[offset + 1] = yuv_coeff[0][2] *
            ((float)*conv_v - 128.0f);
        cft_ptr_cr_rgb[offset + 2] = yuv_coeff[1][2] *
            ((float)*conv_v - 128.0f);
        cft_ptr_cr_rgb[offset + 3] = yuv_coeff[2][2] *
            ((float)*conv_v - 128.0f);
        conv_v++;
    }
}

/* TODO: [breakthrough needed] 99.93%; body agrees; BSS first-use evidence for discarded preparation helper is missing. */
void CFT_MakeYcc422ColAdjTbl(void* table)
{
    u32* alpha_high = table;
    u32* alpha_low = alpha_high + 256;
    u32* chroma_high = alpha_low + 256;
    u32* chroma_low = chroma_high + 256;
    float u_offset;
    float v_offset;
    float y_coefficient;
    float u_coefficient;
    float v_coefficient;
    s32 index;

    CFT_MakeInvConvTableCustom(cft_conv_y_itbl, cft_conv_u_itbl,
                               cft_conv_v_itbl);
    CFT_MakeInverseMtx3D(cft_rgb_yuv_coeff, cft_yuv_rgb_coeff);
    CFT_MakeMtx3D(cft_rgb_yuv_ccir601, cft_yuv_rgb_coeff,
                  cft_basic_ccir601);
    y_coefficient = cft_basic_ccir601[0][0];
    u_coefficient = cft_basic_ccir601[1][1];
    v_coefficient = cft_basic_ccir601[2][2];
    u_offset = 128.0f * u_coefficient;
    v_offset = 128.0f * v_coefficient;

    for (index = 0; index < 256; index++) {
        float value;
        u32 converted;

        value = y_coefficient * (float)cft_conv_y_itbl[index] + 16.5f;
        if (value < 0.0f) value = 0.0f;
        if (value > 255.0f) value = 255.0f;
        converted = value;
        alpha_high[index] = converted << 24;
        alpha_low[index] = converted << 8;

        value = 128.5f +
                (u_coefficient * (float)cft_conv_u_itbl[index] - u_offset);
        if (value < 0.0f) value = 0.0f;
        if (value > 255.0f) value = 255.0f;
        converted = value;
        chroma_high[index] = converted << 16;

        value = 128.5f +
                (v_coefficient * (float)cft_conv_v_itbl[index] - v_offset);
        if (value < 0.0f) value = 0.0f;
        if (value > 255.0f) value = 255.0f;
        converted = value;
        chroma_low[index] = converted;
    }
}

static inline u32 cftMakeAlphaPair(u32 first, u32 second)
{
    return (first << 24) | (second << 8);
}

static inline const u8* cftApplyDynamicAlphaRow(
    u32* output, const u8* source, const u8* table)
{
    u32 first_alpha = cftMakeAlphaPair(table[source[0]], table[source[1]]);
    u32 second_alpha = cftMakeAlphaPair(table[source[2]], table[source[3]]);

    output[0] &= first_alpha | 0x00FF00FF;
    output[1] &= second_alpha | 0x00FF00FF;
    return source + 4;
}

/* TODO: [breakthrough] 61.77%; full tile rewind and loop snapshots recovered;
 * row-pointer and pair-packing schedule still differs. */
static void cnvDynamicYcc420plnToA256UserTable(
    const CFTYcc420Planar* source,
    const CFTArgb8888Output* destination,
    const u8* table)
{
    s32 source_rewind;
    s32 output_row_advance;
    const u8* row4;
    const u8* y = source->y;
    const u8* row3;
    s32 block_y;
    s32 block_x;
    s32 width_in_blocks = destination->width / 4;
    s32 height_in_blocks = destination->height / 4;
    s32 source_stride = source->y_stride;
    u32* output = (u32*)destination->data;
    const u8* row2;
    s32 source_row_advance;

    source_rewind = source_stride * 4;
    source_row_advance = source_stride * 3 +
                         (source_stride - destination->width);
    output_row_advance =
        ((destination->stride - destination->width) / 4) * 64;

    for (block_y = 0; block_y < height_in_blocks; block_y++) {
        for (block_x = 0; block_x < width_in_blocks; block_x++) {
            s32 row_jump = source_stride - 4;

            row2 = cftApplyDynamicAlphaRow(output, y, table);
            row2 += row_jump;
            row3 = cftApplyDynamicAlphaRow(output + 2, row2, table);
            row3 += row_jump;
            row4 = cftApplyDynamicAlphaRow(output + 4, row3, table);
            row4 += row_jump;
            y = cftApplyDynamicAlphaRow(output + 6, row4, table);
            y += row_jump;
            y -= source_rewind;
            y += 4;
            output += 16;
        }
        y += source_row_advance;
        output = (u32*)((u8*)output + output_row_advance);
    }
}

static inline u32 cftMakeDirectAlphaMask0(u32 pixels)
{
    return (pixels & 0xFF000000) | ((pixels >> 8) & 0x0000FF00) |
           0x00FF00FF;
}

static inline u32 cftMakeDirectAlphaMask1(u32 pixels)
{
    return ((pixels << 16) & 0xFF000000) |
           ((pixels << 8) & 0x0000FF00) | 0x00FF00FF;
}

static inline void cftApplyStaticAlphaRow(u32* output, u32 pixels)
{
    output[0] &= cftMakeDirectAlphaMask0(pixels);
    output[1] &= cftMakeDirectAlphaMask1(pixels);
}

/* TODO: [breakthrough needed] 84.27%; unsigned row advance recovered;
 * equivalent mask operations retain scheduling and register differences. */
static void cnvStaticYcc420plnToA256V(
    const CFTYcc420Planar* source,
    const CFTArgb8888Output* destination)
{
    const u8* y = source->y;
    u32* output = (u32*)destination->data;
    s32 source_stride = source->y_stride;
    s32 aligned_stride = source_stride & ~3;
    s32 source_rewind = aligned_stride * 4;
    s32 width = destination->width;
    s32 width_in_blocks = width / 4;
    s32 height_in_blocks = destination->height / 4;
    s32 source_row_advance = ((u32)source_stride >> 2) * 12;
    s32 output_row_advance =
        ((destination->stride - width) / 4) * 64;
    s32 block_y;

    for (block_y = 0; block_y < height_in_blocks; block_y++) {
        s32 block_x;
        for (block_x = 0; block_x < width_in_blocks; block_x++) {
            u32 pixels = *(const u32*)y;
            cftApplyStaticAlphaRow(output, pixels);
            y += aligned_stride;
            pixels = *(const u32*)y;
            cftApplyStaticAlphaRow(output + 2, pixels);
            y += aligned_stride;
            pixels = *(const u32*)y;
            cftApplyStaticAlphaRow(output + 4, pixels);
            y += aligned_stride;
            pixels = *(const u32*)y;
            cftApplyStaticAlphaRow(output + 6, pixels);
            y += aligned_stride;
            y -= source_rewind;
            y += 4;
            output += 16;
        }
        y += source_row_advance;
        output = (u32*)((u8*)output + output_row_advance);
    }
}

void CFT_Ycc420plnToA256V(const CFTYcc420Planar* source,
                          const CFTArgb8888Output* destination,
                          const u8* table)
{
    if (table == 0) {
        cnvStaticYcc420plnToA256V(source, destination);
    } else {
        cnvDynamicYcc420plnToA256UserTable(source, destination, table);
    }
}

static inline u32 cftPackEvenPixels(u32 luma, u16 chroma)
{
    return (luma & 0xFF000000) | ((luma >> 8) & 0x0000FF00) |
           (((u32)chroma << 8) & 0x00FF0000) | ((u32)chroma >> 8);
}

static inline u32 cftPackOddPixels(u32 luma, u16 chroma)
{
    return ((luma << 16) & 0xFF000000) |
           ((luma << 8) & 0x0000FF00) |
           (((u32)chroma << 16) & 0x00FF0000) | ((u32)chroma & 0xFF);
}

static inline void cftStorePixelQuad(
    u32* first_output, u32* second_output,
    u32 first_luma, u32 second_luma,
    u16 first_chroma, u16 second_chroma)
{
    first_output[1] = cftPackOddPixels(first_luma, first_chroma);
    second_output[1] = cftPackOddPixels(second_luma, second_chroma);
    first_output[0] = cftPackEvenPixels(first_luma, first_chroma);
    second_output[0] = cftPackEvenPixels(second_luma, second_chroma);
}

/* TODO: [near miss] 99.89%; two commutative next-row additions remain; stop at operand encoding. */
void CFT_Argb420ToArgb8(const void* source, void* destination,
                        s32 width, s32 height)
{
    MwsYccPlane planes;
    u8* y0;
    u8* y1;
    u16* cb;
    u16* cr;
    u32* output0;
    u32* output1;
    s32 y_step;
    s32 c_step;
    s32 y_rewind;
    s32 block_y;
    s32 blocks_across;
    s32 blocks_down;
    u16* chroma_base;
    u16* cb_next;
    u16 chroma_u;
    u16 chroma_v;

    mwPlyCalcYccPlane((void*)source, width, height, &planes);
    blocks_across = width / 4;
    y0 = planes.y;
    chroma_base = planes.cb;
    y1 = (u8*)planes.y +
        ((((unsigned long)chroma_base - (unsigned long)planes.y) >> 1) & ~3UL);
    cb = chroma_base;
    cr = planes.cr;
    output0 = destination;
    blocks_down = height / 4;
    output1 = output0 + 8;
    y_step = planes.y_pitch & ~3;
    c_step = planes.cb_pitch & ~1;
    y_rewind = y_step * 2;

    for (block_y = 0; block_y < blocks_down; block_y++) {
        u16* cr_next = (u16*)((u8*)cr + c_step);
        s32 block_x;
        cb_next = (u16*)((u8*)cb + c_step);

        for (block_x = 0; block_x < blocks_across; block_x++) {
            if (block_x >= 0 && block_x < blocks_across) {
                u32 first_luma = *(u32*)y1;
                u32 second_luma = *(u32*)y0;
                u16 first_chroma = *cb;
                u16 second_chroma = *cr;
                cftStorePixelQuad(output0, output1,
                                  first_luma, second_luma,
                                  first_chroma, second_chroma);
                chroma_u = *cb;
                chroma_v = *cr;
                cftStorePixelQuad(output0 + 2, output1 + 2,
                                  *(u32*)y1, *(u32*)y0,
                                  chroma_u, chroma_v);
            } else {
                output1[0] = output0[0] = 0;
                output1[1] = output0[1] = 0;
                output1[2] = output0[2] = 0;
                output1[3] = output0[3] = 0;
            }

            y0 += y_step;
            y1 += y_step;
            if (block_x >= 0 && block_x < blocks_across) {
                cftStorePixelQuad(output0 + 4, output1 + 4,
                                  *(u32*)y1, *(u32*)y0,
                                  *cb_next, *cr_next);
                cftStorePixelQuad(output0 + 6, output1 + 6,
                                  *(u32*)y1, *(u32*)y0,
                                  *cb, *cr);
            } else {
                output1[4] = output0[4] = 0;
                output1[5] = output0[5] = 0;
                output1[6] = output0[6] = 0;
                output1[7] = output0[7] = 0;
            }

            y0 += y_step;
            y1 += y_step;
            y0 += 4;
            y1 += 4;
            y0 -= y_rewind;
            output0 += 16;
            output1 += 16;
            cb++;
            cb_next++;
            cr++;
            cr_next++;
            y1 -= y_rewind;
        }
        y0 += y_step;
        y1 += y_step;
        cb = (u16*)((u8*)cb + c_step);
        cr = (u16*)((u8*)cr + c_step);
    }
}
