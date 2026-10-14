/* BUILD: -inline auto,deferred emits definitions in reverse source order;
 * this order also gives retail's reverse first-use .bss layout. */
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

static CFTConvTable cft_conv_y_itbl;
static CFTConvTable cft_conv_u_itbl;
static CFTConvTable cft_conv_v_itbl;
static float* cft_ptr_y__rgb;
static float* cft_ptr_cb_rgb;
static float* cft_ptr_cr_rgb;
static CFTMtx3D cft_yuv_rgb_coeff;
static CFTMtx3D cft_basic_ccir601;

/* clamp to the byte range */
#define CFT_CLIP255(value)    \
    if ((value) < 0.0f) {     \
        (value) = 0.0f;       \
    }                         \
    if ((value) > 255.0f) {   \
        (value) = 255.0f;     \
    }

/* One row of the table-driven alpha fill: two packs of table[luma] bytes, the row pointer
 * advanced by four inside the last index. */
#define CFT_A256_ROW(dst, y, tbl)                                          \
    v0 = (((u32)(tbl)[(y)[0] & 0xFF] & 0xFF) << 24);                       \
    v0 |= (u32)(tbl)[(y)[1] & 0xFF] << 8;                                  \
    v1 = (((u32)(tbl)[(y)[2] & 0xFF] & 0xFF) << 24);                       \
    v1 |= (u32)(tbl)[((y) += 4)[-1] & 0xFF] << 8;                          \
    (dst)[0] &= v0 | 0x00FF00FF;                                           \
    (dst)[1] &= v1 | 0x00FF00FF

/* One row of the static luma-as-alpha conversion: the low word goes through t first. */
#define CFT_A256_ROW_STATIC(dst, w, t)                                     \
    (t) = (((w) << 16) & 0xFF000000) | (((w) & 0xFF) << 8);               \
    (dst)[0] &= (((w) & 0xFF000000) | (((w) >> 8) & 0x0000FF00)) | 0x00FF00FF; \
    (dst)[1] &= (t) | 0x00FF00FF

static void cnvDynamicYcc420plnToA256UserTable(
    const CFTYcc420Planar* source,
    const CFTArgb8888Output* destination,
    const u8* table);
static void cnvStaticYcc420plnToA256V(
    const CFTYcc420Planar* source,
    const CFTArgb8888Output* destination);

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
        u16* cr_next = (u16*)(c_step + (u32)cr);
        s32 block_x;
        cb_next = (u16*)(c_step + (u32)cb);

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

static void cnvStaticYcc420plnToA256V(
    const CFTYcc420Planar* source,
    const CFTArgb8888Output* destination)
{
    s32 block_y;
    s32 block_x;
    s32 width_in_blocks = destination->width / 4;
    s32 height_in_blocks = destination->height / 4;
    s32 y_step = source->y_stride / sizeof(u32);
    s32 output_skip = (destination->stride - destination->width) / 4 * 16;
    u32* output = (u32*)destination->data;
    u32* y = (u32*)source->y;
    u32 pixels;
    u32 low = 0;

    for (block_y = 0; block_y < height_in_blocks; block_y++) {
        for (block_x = 0; block_x < width_in_blocks; block_x++) {
            pixels = *y;
            y += y_step;
            CFT_A256_ROW_STATIC(output, pixels, low);
            pixels = *y;
            y += y_step;
            CFT_A256_ROW_STATIC(output + 2, pixels, low);
            pixels = *y;
            y += y_step;
            CFT_A256_ROW_STATIC(output + 4, pixels, low);
            pixels = *y;
            y += y_step;
            CFT_A256_ROW_STATIC(output + 6, pixels, low);
            y -= y_step * 4;
            low = (u32)y + 4;
            y = (u32*)low;
            low = (u32)output + 64;
            output = (u32*)low;
        }
        y += y_step * 3;
        output += output_skip;
    }
}

static void cnvDynamicYcc420plnToA256UserTable(
    const CFTYcc420Planar* source,
    const CFTArgb8888Output* destination,
    const u8* table)
{
    s32 rewind;
    s32 output_skip;
    const u8* row4;
    const u8* y = source->y;
    const u8* row3;
    s32 block_y;
    s32 block_x;
    s32 width_in_blocks = destination->width / 4;
    s32 height_in_blocks = destination->height / 4;
    s32 y_stride = source->y_stride;
    u32* output = (u32*)destination->data;
    const u8* row2;
    s32 y_skip;
    u32 v0, v1;

    rewind = y_stride * 4;
    y_skip = y_stride * 3 + (y_stride - destination->width);
    output_skip = (destination->stride - destination->width) / 4 * 64;
    for (block_y = 0; block_y < height_in_blocks; block_y++) {
        for (block_x = 0; block_x < width_in_blocks; block_x++) {
            row2 = (u8*)(y_stride - 4);
            row3 = row2;
            row4 = row2;
            CFT_A256_ROW(output, y, table);
            row2 += (u32)(y + 4) - 4;
            CFT_A256_ROW(output + 2, row2, table);
            row3 += (u32)(row2 + 4) - 4;
            CFT_A256_ROW(output + 4, row3, table);
            row4 += (u32)(row3 + 4) - 4;
            CFT_A256_ROW(output + 6, row4, table);
            row3 = (u8*)(y_stride - 4);
            row3 += (u32)(row4 + 4) - 4;
            y = row3 - rewind + 4;
            output += 16;
        }
        y += y_skip;
        output = (u32*)((u8*)output + output_skip);
    }
}

void CFT_MakeYcc422ColAdjTbl(void* table)
{
    u32* alpha_high = (u32*)table;
    u32* alpha_low = alpha_high + 0x100;
    u32* chroma_high = alpha_low + 0x100;
    u32* chroma_low = chroma_high + 0x100;
    float value;
    float u_offset;
    float v_offset;
    float y_coefficient;
    float u_coefficient;
    float v_coefficient;
    u32 converted;
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
        value = y_coefficient * (float)cft_conv_y_itbl[index] + 16.5f;
        CFT_CLIP255(value);
        converted = (u32)value;
        alpha_high[index] = converted << 24;
        alpha_low[index] = converted << 8;
        value = 128.5f + (u_coefficient * (float)cft_conv_u_itbl[index] - u_offset);
        CFT_CLIP255(value);
        converted = (u32)value;
        chroma_high[index] = converted << 16;
        value = 128.5f + (v_coefficient * (float)cft_conv_v_itbl[index] - v_offset);
        CFT_CLIP255(value);
        converted = (u32)value;
        chroma_low[index] = converted;
    }
}

void CFT_MakeArgb8888ColAdjTbl(void* buffer)
{
    s32 index;

    cft_ptr_y__rgb = (float*)buffer;
    cft_ptr_cb_rgb = (float*)((u8*)buffer + 0x1000);
    cft_ptr_cr_rgb = (float*)((u8*)buffer + 0x2000);
    CFT_MakeInvConvTableCustom(cft_conv_y_itbl, cft_conv_u_itbl,
                               cft_conv_v_itbl);
    CFT_MakeInverseMtx3D(cft_rgb_yuv_coeff, cft_yuv_rgb_coeff);
    for (index = 0; index < 256; index++) {
        cft_ptr_y__rgb[(index << 2) + 1] = cft_yuv_rgb_coeff[0][0] * (float)cft_conv_y_itbl[index];
        cft_ptr_y__rgb[(index << 2) + 2] = cft_yuv_rgb_coeff[1][0] * (float)cft_conv_y_itbl[index];
        cft_ptr_y__rgb[(index << 2) + 3] = cft_yuv_rgb_coeff[2][0] * (float)cft_conv_y_itbl[index];
        cft_ptr_y__rgb[(index << 2) + 0] = 255.0f;
        cft_ptr_cb_rgb[(index << 2) + 1] = cft_yuv_rgb_coeff[0][1] * ((float)cft_conv_u_itbl[index] - 128.0f);
        cft_ptr_cb_rgb[(index << 2) + 2] = cft_yuv_rgb_coeff[1][1] * ((float)cft_conv_u_itbl[index] - 128.0f);
        cft_ptr_cb_rgb[(index << 2) + 3] = cft_yuv_rgb_coeff[2][1] * ((float)cft_conv_u_itbl[index] - 128.0f);
        cft_ptr_cr_rgb[(index << 2) + 1] = cft_yuv_rgb_coeff[0][2] * ((float)cft_conv_v_itbl[index] - 128.0f);
        cft_ptr_cr_rgb[(index << 2) + 2] = cft_yuv_rgb_coeff[1][2] * ((float)cft_conv_v_itbl[index] - 128.0f);
        cft_ptr_cr_rgb[(index << 2) + 3] = cft_yuv_rgb_coeff[2][2] * ((float)cft_conv_v_itbl[index] - 128.0f);
    }
}
