#include "dolphin/types.h"

typedef float CFTMtx3D[3][3];
typedef unsigned char CFTConvTable[256];
typedef float CFTArgbTable[3][256][4];

typedef struct CFTYcc420Planar {
    const u8* y;
    const u8* cb;
    const u8* cr;
    s32 y_stride;
    s32 cb_stride;
    s32 cr_stride;
} CFTYcc420Planar;

typedef struct CFTArgb8888Output {
    u8* data;
    s32 width;
    s32 height;
    s32 stride;
} CFTArgb8888Output;

typedef struct CFTYccPlanes {
    u32* y;
    u16* cb;
    u16* cr;
    s32 y_pitch;
    s32 c_pitch;
    s32 c_height;
} CFTYccPlanes;

typedef char CFTYcc420PlanarSizeCheck[
    sizeof(CFTYcc420Planar) == 0x18 ? 1 : -1];
typedef char CFTArgb8888OutputSizeCheck[
    sizeof(CFTArgb8888Output) == 0x10 ? 1 : -1];
typedef char CFTYccPlanesSizeCheck[sizeof(CFTYccPlanes) == 0x18 ? 1 : -1];

extern void CFT_MakeInvConvTableCustom(CFTConvTable luma,
                                       CFTConvTable chroma_u,
                                       CFTConvTable chroma_v);
extern void CFT_MakeInverseMtx3D(CFTMtx3D matrix, CFTMtx3D inverse);
extern void CFT_MakeMtx3D(CFTMtx3D left, CFTMtx3D right,
                          CFTMtx3D product);
extern void mwPlyCalcYccPlane(const void* source, s32 width, s32 height,
                              CFTYccPlanes* planes);

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

typedef struct CFTColorConversionState {
    CFTMtx3D basic_ccir601;
    CFTMtx3D yuv_rgb_coeff;
    float* cr_rgb;
    float* cb_rgb;
    float* y_rgb;
    CFTConvTable conv_v;
    CFTConvTable conv_u;
    CFTConvTable conv_y;
} CFTColorConversionState;

typedef char CFTColorConversionStateSizeCheck[
    sizeof(CFTColorConversionState) == 0x354 ? 1 : -1];

static CFTColorConversionState cft_color_state;

void CFT_MakeArgb8888ColAdjTbl(CFTArgbTable table)
{
    CFTColorConversionState* state = &cft_color_state;
    s32 index;

    state->y_rgb = &table[0][0][0];
    state->cb_rgb = &table[1][0][0];
    state->cr_rgb = &table[2][0][0];
    CFT_MakeInvConvTableCustom(state->conv_y, state->conv_u,
                               state->conv_v);
    CFT_MakeInverseMtx3D(cft_rgb_yuv_coeff, state->yuv_rgb_coeff);

    for (index = 0; index < 256; index++) {
        s32 offset = index * 4;

        state->y_rgb[offset + 1] =
            state->yuv_rgb_coeff[0][0] * (float)state->conv_y[index];
        state->y_rgb[offset + 2] =
            state->yuv_rgb_coeff[1][0] * (float)state->conv_y[index];
        state->y_rgb[offset + 3] =
            state->yuv_rgb_coeff[2][0] * (float)state->conv_y[index];
        state->y_rgb[offset] = 255.0f;
        state->cb_rgb[offset + 1] = state->yuv_rgb_coeff[0][1] *
            ((float)state->conv_u[index] - 128.0f);
        state->cb_rgb[offset + 2] = state->yuv_rgb_coeff[1][1] *
            ((float)state->conv_u[index] - 128.0f);
        state->cb_rgb[offset + 3] = state->yuv_rgb_coeff[2][1] *
            ((float)state->conv_u[index] - 128.0f);
        state->cr_rgb[offset + 1] = state->yuv_rgb_coeff[0][2] *
            ((float)state->conv_v[index] - 128.0f);
        state->cr_rgb[offset + 2] = state->yuv_rgb_coeff[1][2] *
            ((float)state->conv_v[index] - 128.0f);
        state->cr_rgb[offset + 3] = state->yuv_rgb_coeff[2][2] *
            ((float)state->conv_v[index] - 128.0f);
    }
}

void CFT_MakeYcc422ColAdjTbl(u32 table[4][256])
{
    CFTColorConversionState* state = &cft_color_state;
    u32* alpha_high = table[0];
    u32* alpha_low = table[1];
    u32* chroma_high = table[2];
    u32* chroma_low = table[3];
    float y_coefficient;
    float u_coefficient;
    float v_coefficient;
    float u_offset;
    float v_offset;
    s32 index;

    CFT_MakeInvConvTableCustom(state->conv_y, state->conv_u,
                               state->conv_v);
    CFT_MakeInverseMtx3D(cft_rgb_yuv_coeff, state->yuv_rgb_coeff);
    CFT_MakeMtx3D(cft_rgb_yuv_ccir601, state->yuv_rgb_coeff,
                  state->basic_ccir601);
    y_coefficient = state->basic_ccir601[0][0];
    u_coefficient = state->basic_ccir601[1][1];
    v_coefficient = state->basic_ccir601[2][2];
    u_offset = 128.0f * u_coefficient;
    v_offset = 128.0f * v_coefficient;

    for (index = 0; index < 256; index++) {
        float value;
        u32 converted;

        value = y_coefficient * (float)state->conv_y[index] + 16.5f;
        if (value < 0.0f) value = 0.0f;
        if (value > 255.0f) value = 255.0f;
        converted = (u32)value;
        alpha_high[index] = converted << 24;
        alpha_low[index] = converted << 8;

        value = 128.5f +
                (u_coefficient * (float)state->conv_u[index] - u_offset);
        if (value < 0.0f) value = 0.0f;
        if (value > 255.0f) value = 255.0f;
        chroma_high[index] = (u32)value << 16;

        value = 128.5f +
                (v_coefficient * (float)state->conv_v[index] - v_offset);
        if (value < 0.0f) value = 0.0f;
        if (value > 255.0f) value = 255.0f;
        chroma_low[index] = (u32)value;
    }
}

static inline u32 cftMakeAlphaPair(u8 first, u8 second)
{
    return ((u32)first << 24) | ((u32)second << 8);
}

static inline void cftApplyDynamicAlphaRow(
    u32* output, const u8** source, const u8* table)
{
    const u8* y = *source;
    u32 first_alpha = cftMakeAlphaPair(table[y[0]], table[y[1]]);
    u32 second_alpha = cftMakeAlphaPair(table[y[2]], table[y[3]]);

    output[0] &= first_alpha | 0x00FF00FF;
    output[1] &= second_alpha | 0x00FF00FF;
    *source = y + 4;
}

static void cnvDynamicYcc420plnToA256UserTable(
    const CFTYcc420Planar* source,
    const CFTArgb8888Output* destination,
    const u8* table)
{
    const u8* y = source->y;
    u32* output = (u32*)destination->data;
    s32 source_stride = source->y_stride;
    s32 width = destination->width;
    s32 width_in_blocks = width / 4;
    s32 height_in_blocks = destination->height / 4;
    s32 source_row_advance = source_stride * 3 + (source_stride - width);
    s32 output_row_advance =
        ((destination->stride - width) / 4) * 64;
    s32 block_y;

    for (block_y = 0; block_y < height_in_blocks; block_y++) {
        s32 block_x;
        for (block_x = 0; block_x < width_in_blocks; block_x++) {
            s32 row_advance = source_stride - 4;

            cftApplyDynamicAlphaRow(output, &y, table);
            y += row_advance;
            cftApplyDynamicAlphaRow(output + 2, &y, table);
            y += row_advance;
            cftApplyDynamicAlphaRow(output + 4, &y, table);
            y += row_advance;
            cftApplyDynamicAlphaRow(output + 6, &y, table);
            y += row_advance;
            y -= source_stride * 4;
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

static void cnvStaticYcc420plnToA256V(
    const CFTYcc420Planar* source,
    const CFTArgb8888Output* destination)
{
    const u8* y = source->y;
    u32* output = (u32*)destination->data;
    s32 source_stride = source->y_stride;
    s32 aligned_stride = source_stride & ~3;
    s32 source_rewind = (source_stride * 4) & ~15;
    s32 width = destination->width;
    s32 width_in_blocks = width / 4;
    s32 height_in_blocks = destination->height / 4;
    s32 source_row_advance = (source_stride >> 2) * 12;
    s32 output_row_advance =
        ((destination->stride - width) / 4) * 64;
    s32 block_y;

    for (block_y = 0; block_y < height_in_blocks; block_y++) {
        s32 block_x;
        for (block_x = 0; block_x < width_in_blocks; block_x++) {
            cftApplyStaticAlphaRow(output, *(const u32*)y);
            y += aligned_stride;
            cftApplyStaticAlphaRow(output + 2, *(const u32*)y);
            y += aligned_stride;
            cftApplyStaticAlphaRow(output + 4, *(const u32*)y);
            y += aligned_stride;
            cftApplyStaticAlphaRow(output + 6, *(const u32*)y);
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

void CFT_Argb420ToArgb8(const void* source, void* destination,
                        s32 width, s32 height)
{
    CFTYccPlanes planes;
    u8* y0;
    u8* y1;
    u8* cb;
    u8* cr;
    u32* output0 = (u32*)destination;
    u32* output1 = (u32*)((u8*)destination + 0x20);
    s32 y_step;
    s32 c_step;
    s32 y_rewind;
    s32 block_y;

    mwPlyCalcYccPlane(source, width, height, &planes);
    y0 = (u8*)planes.y;
    y1 = y0 + ((((u8*)planes.cb - y0) / 2) & ~3);
    cb = (u8*)planes.cb;
    cr = (u8*)planes.cr;
    y_step = planes.y_pitch & ~3;
    c_step = planes.c_height & ~1;
    y_rewind = (planes.y_pitch * 2) & ~7;

    for (block_y = 0; block_y < height / 4; block_y++) {
        u8* cb_next = cb + c_step;
        u8* cr_next = cr + c_step;
        s32 block_x;

        for (block_x = 0; block_x < width / 4; block_x++) {
            if (block_x >= 0 && block_x < width / 4) {
                cftStorePixelQuad(output0, output1,
                                  *(u32*)y1, *(u32*)y0,
                                  *(u16*)cb, *(u16*)cr);
                cftStorePixelQuad(output0 + 2, output1 + 2,
                                  *(u32*)y1, *(u32*)y0,
                                  *(u16*)cb, *(u16*)cr);
            } else {
                output0[0] = output1[0] = 0;
                output0[1] = output1[1] = 0;
                output0[2] = output1[2] = 0;
                output0[3] = output1[3] = 0;
            }

            y0 += y_step;
            y1 += y_step;
            if (block_x >= 0 && block_x < width / 4) {
                cftStorePixelQuad(output0 + 4, output1 + 4,
                                  *(u32*)y1, *(u32*)y0,
                                  *(u16*)cb_next, *(u16*)cr_next);
                cftStorePixelQuad(output0 + 6, output1 + 6,
                                  *(u32*)y1, *(u32*)y0,
                                  *(u16*)cb, *(u16*)cr);
            } else {
                output0[4] = output1[4] = 0;
                output0[5] = output1[5] = 0;
                output0[6] = output1[6] = 0;
                output0[7] = output1[7] = 0;
            }

            y0 += y_step + 4 - y_rewind;
            y1 += y_step + 4 - y_rewind;
            cb += 2;
            cr += 2;
            cb_next += 2;
            cr_next += 2;
            output0 += 16;
            output1 += 16;
        }
        y0 += y_step;
        y1 += y_step;
        cb += c_step;
        cr += c_step;
    }
}

/* Retail split-layout tails for the read-only and zero-initialized sections. */
const u32 gap_04_80319FA4_rodata = 0;
u32 gap_06_804AF97C_bss;
