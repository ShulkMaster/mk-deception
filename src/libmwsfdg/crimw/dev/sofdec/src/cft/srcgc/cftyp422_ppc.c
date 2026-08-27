typedef unsigned char u8;
typedef signed int s32;
typedef unsigned int u32;
typedef unsigned long long u64;

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

typedef float CFTArgbTable[3][256][4];

static const char cft_version_string[] =
    "\nCRI CFT/GC Ver.1.57 Build:Sep  3 2004 11:38:10\n";
const char* CFT_version = cft_version_string;

static u32 gqr_save;
static float cr_r[256];
static float cr_g[256];
static float cb_b[256];
static float cb_g[256];
static float y__r[256] __attribute__((aligned(32)));
static const char* CFT_dummy;

static inline float clamp_table_value(float value)
{
    if (value < 0.0f) {
        value = 0.0f;
    } else if (value > 255.0f) {
        value = 255.0f;
    }
    return value;
}

static inline u8 clamp_channel(float value)
{
    if (value <= 0.0f) {
        return 0;
    }
    if (value >= 255.0f) {
        return 255;
    }
    return (u8)value;
}

static inline void make_chroma_tables(CFTArgbTable table)
{
    s32 i;
    float* cb = &table[1][0][0];
    float* cr = &table[2][0][0];

    for (i = 0; i < 256; i += 2) {
        cb[3] = 2.017f * (float)(i - 128) + 0.5f;
        cb[2] = -0.392f * (float)(i - 128) + 0.5f;
        cb[1] = 0.0f;
        cb[0] = 0.0f;
        cr[3] = 0.0f;
        cr[2] = -0.813f * (float)(i - 128) + 0.5f;
        cr[1] = 1.596f * (float)(i - 128) + 0.5f;
        cr[0] = 0.0f;
        cb[7] = 2.017f * (float)(i - 127) + 0.5f;
        cb[6] = -0.392f * (float)(i - 127) + 0.5f;
        cb[5] = 0.0f;
        cb[4] = 0.0f;
        cr[7] = 0.0f;
        cr[6] = -0.813f * (float)(i - 127) + 0.5f;
        cr[5] = 1.596f * (float)(i - 127) + 0.5f;
        cr[4] = 0.0f;
        cb += 8;
        cr += 8;
    }
}

void CFT_MakeArgb8888Alp3211Tbl(
    CFTArgbTable table, u8 alpha0, u8 alpha1, u8 alpha2)
{
    s32 i;
    float* y_table;

    make_chroma_tables(table);
    for (i = 0; i < 48; i++) {
        table[0][i][0] = (float)alpha0;
        table[0][i][1] = -18.130136f;
        table[0][i][2] = -18.130136f;
        table[0][i][3] = -18.130136f;
    }
    y_table = &table[0][i][0];
    for (; i < 130; i += 2) {
        float luminance0 =
            4.6363635f * clamp_table_value((float)i - 68.0f) + 0.5f;
        float luminance1 =
            4.6363635f * clamp_table_value((float)(i + 1) - 68.0f) + 0.5f;

        y_table[0] = (float)alpha1;
        y_table[1] = luminance0;
        y_table[2] = luminance0;
        y_table[3] = luminance0;
        y_table[4] = (float)alpha1;
        y_table[5] = luminance1;
        y_table[6] = luminance1;
        y_table[7] = luminance1;
        y_table += 8;
    }
    for (; i < 256; i += 2) {
        float luminance0 =
            2.2972972f * clamp_table_value(247.0f - (float)i) + 0.5f;
        float luminance1 =
            2.2972972f * clamp_table_value(247.0f - (float)(i + 1)) + 0.5f;

        y_table[0] = (float)alpha2;
        y_table[1] = luminance0;
        y_table[2] = luminance0;
        y_table[3] = luminance0;
        y_table[4] = (float)alpha2;
        y_table[5] = luminance1;
        y_table[6] = luminance1;
        y_table[7] = luminance1;
        y_table += 8;
    }
}

void CFT_MakeArgb8888Alp3110Tbl(
    CFTArgbTable table, u8 alpha0, u8 alpha1, u8 alpha2)
{
    s32 i;
    float* y_table;

    make_chroma_tables(table);
    for (i = 0; i < 9; i++) {
        table[0][i][0] = (float)alpha0;
        table[0][i][1] = 0.0f;
        table[0][i][2] = 0.0f;
        table[0][i][3] = 0.0f;
    }
    y_table = &table[0][i][0];
    for (; i < 134; i++) {
        float luminance =
            2.3181818f * clamp_table_value((float)i - 16.0f) + 0.5f;

        y_table[0] = (float)alpha1;
        y_table[1] = luminance;
        y_table[2] = luminance;
        y_table[3] = luminance;
        y_table += 4;
    }
    for (; i < 256; i += 2) {
        float luminance0 =
            2.3181818f * clamp_table_value(251.0f - (float)i) + 0.5f;
        float luminance1 =
            2.3181818f * clamp_table_value(251.0f - (float)(i + 1)) + 0.5f;

        y_table[0] = (float)alpha2;
        y_table[1] = luminance0;
        y_table[2] = luminance0;
        y_table[3] = luminance0;
        y_table[4] = (float)alpha2;
        y_table[5] = luminance1;
        y_table[6] = luminance1;
        y_table[7] = luminance1;
        y_table += 8;
    }
}

void CFT_MakeArgb8888AlpLumiTbl(
    s32 reverse, s32 low, s32 high, CFTArgbTable table)
{
    s32 i;
    s32 range;
    float scale;
    float* y = &table[0][0][0];
    float* cb = &table[1][0][0];
    float* cr = &table[2][0][0];

    i = 0;
    do {
        float luminance = 1.16400003f * (float)(i - 16) + 0.5f;
        float chroma = (float)(i - 128);

        y[3] = luminance;
        y[2] = luminance;
        y[1] = luminance;
        cb[3] = 2.017f * chroma + 0.5f;
        cb[2] = -0.392f * chroma + 0.5f;
        cb[1] = 0.0f;
        cb[0] = 0.0f;
        cr[3] = 0.0f;
        cr[2] = -0.813f * chroma + 0.5f;
        cr[1] = 1.596f * chroma + 0.5f;
        cr[0] = 0.0f;
        y += 4;
        cb += 4;
        cr += 4;
        i++;
    } while (i != 256);

    y = &table[0][0][0];
    range = high - low;
    scale = 255.0f / (float)range;
    if (reverse == 1) {
        for (i = 0; i < 256; i += 2) {
            s32 next = i + 1;

            if (i < low) {
                y[0] = 255.0f;
            } else if (i > high) {
                y[0] = 0.0f;
            } else {
                y[0] = scale * (float)(range - (i - low));
            }
            if (next < low) {
                y[4] = 255.0f;
            } else if (next > high) {
                y[4] = 0.0f;
            } else {
                y[4] = scale * (float)(range - (next - low));
            }
            y += 8;
        }
    } else {
        for (i = 0; i < 256; i += 2) {
            s32 next = i + 1;

            if (i < low) {
                y[0] = 0.0f;
            } else if (i > high) {
                y[0] = 255.0f;
            } else {
                y[0] = scale * (float)(i - low);
            }
            if (next < low) {
                y[4] = 0.0f;
            } else if (next > high) {
                y[4] = 255.0f;
            } else {
                y[4] = scale * (float)(next - low);
            }
            y += 8;
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
    s32 tile_y;
    s32 tile_x;
    u64* y_output = (u64*)dst_y;
    u32* c_output = (u32*)dst_c;

    (void)dst_y_stride;
    for (tile_y = 0; tile_y < height / 4; tile_y++) {
        const u64* row0 = (const u64*)(src->y + tile_y * 4 * src->y_stride);
        const u64* row1 = (const u64*)((const u8*)row0 + src->y_stride);
        const u64* row2 = (const u64*)((const u8*)row1 + src->y_stride);
        const u64* row3 = (const u64*)((const u8*)row2 + src->y_stride);

        for (tile_x = 0; tile_x < src->y_stride / 8; tile_x++) {
            *y_output++ = *row0++;
            *y_output++ = *row1++;
            *y_output++ = *row2++;
            *y_output++ = *row3++;
        }
    }

    for (tile_y = 0; tile_y < height / 8; tile_y++) {
        const u32* cb0 = (const u32*)(src->cb + tile_y * 4 * src->cb_stride);
        const u32* cb1 = (const u32*)((const u8*)cb0 + src->cb_stride);
        const u32* cb2 = (const u32*)((const u8*)cb1 + src->cb_stride);
        const u32* cb3 = (const u32*)((const u8*)cb2 + src->cb_stride);
        const u32* cr0 = (const u32*)(src->cr + tile_y * 4 * src->cb_stride);
        const u32* cr1 = (const u32*)((const u8*)cr0 + src->cb_stride);
        const u32* cr2 = (const u32*)((const u8*)cr1 + src->cb_stride);
        const u32* cr3 = (const u32*)((const u8*)cr2 + src->cb_stride);

        for (tile_x = 0; tile_x < src->y_stride / 16; tile_x++) {
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
    }
}

static void cnvDynamicYcc420plnToArgb8888(
    const CFTYcc420Planar* src,
    const CFTArgb8888Output* dst,
    CFTArgbTable table);
static void cnvStaticYcc420plnToArgb8888(
    const CFTYcc420Planar* src, const CFTArgb8888Output* dst);

static inline void store_dynamic_pixel(
    u8* tile, s32 pixel, u8 y, u8 cb, u8 cr, CFTArgbTable table)
{
    float* yt = table[0][y];
    float* cbt = table[1][cb];
    float* crt = table[2][cr];

    tile[pixel * 2] = clamp_channel(yt[0] + cbt[0] + crt[0]);
    tile[pixel * 2 + 1] = clamp_channel(yt[1] + cbt[1] + crt[1]);
    tile[32 + pixel * 2] = clamp_channel(yt[2] + cbt[2] + crt[2]);
    tile[33 + pixel * 2] = clamp_channel(yt[3] + cbt[3] + crt[3]);
}

static void cnvDynamicYcc420plnToArgb8888(
    const CFTYcc420Planar* src,
    const CFTArgb8888Output* dst,
    CFTArgbTable table)
{
    s32 tile_y;
    s32 tile_x;
    s32 dy;
    s32 dx;

    for (tile_y = 0; tile_y < dst->height / 4; tile_y++) {
        const u8* y = src->y + tile_y * 4 * src->y_stride;
        const u8* cb = src->cb + tile_y * 2 * src->cb_stride;
        const u8* cr = src->cr + tile_y * 2 * src->cb_stride;
        u8* output = dst->data + tile_y * 16 * dst->stride;

        for (tile_x = 0; tile_x < src->y_stride / 4; tile_x++) {
            u8* tile = output + tile_x * 64;
            for (dy = 0; dy < 4; dy++) {
                for (dx = 0; dx < 4; dx++) {
                    s32 chroma_x = tile_x * 2 + dx / 2;
                    s32 chroma_y = dy / 2;
                    store_dynamic_pixel(
                        tile,
                        dy * 4 + dx,
                        y[dy * src->y_stride + tile_x * 4 + dx],
                        cb[chroma_y * src->cb_stride + chroma_x],
                        cr[chroma_y * src->cb_stride + chroma_x],
                        table);
                }
            }
        }
    }
}

static inline void store_static_pixel(u8* tile, s32 pixel, u8 y, u8 cb, u8 cr)
{
    float red_chroma = cr_r[cr];
    float green_cr = cr_g[cr];
    float blue_chroma = cb_b[cb];
    float green_cb = cb_g[cb];
    float luminance = y__r[y];

    tile[pixel * 2] = 255;
    tile[pixel * 2 + 1] = clamp_channel(luminance + red_chroma);
    tile[32 + pixel * 2] =
        clamp_channel(luminance + green_cr + green_cb);
    tile[33 + pixel * 2] = clamp_channel(luminance + blue_chroma);
}

static void cnvStaticYcc420plnToArgb8888(
    const CFTYcc420Planar* src, const CFTArgb8888Output* dst)
{
    s32 tile_y;
    s32 tile_x;
    s32 dy;
    s32 dx;

    for (tile_y = 0; tile_y < dst->height / 4; tile_y++) {
        const u8* y = src->y + tile_y * 4 * src->y_stride;
        const u8* cb = src->cb + tile_y * 2 * src->cb_stride;
        const u8* cr = src->cr + tile_y * 2 * src->cb_stride;
        u8* output = dst->data + tile_y * 16 * dst->stride;

        for (tile_x = 0; tile_x < src->y_stride / 4; tile_x++) {
            u8* tile = output + tile_x * 64;
            for (dy = 0; dy < 4; dy++) {
                for (dx = 0; dx < 4; dx++) {
                    s32 chroma_x = tile_x * 2 + dx / 2;
                    s32 chroma_y = dy / 2;
                    store_static_pixel(
                        tile,
                        dy * 4 + dx,
                        y[dy * src->y_stride + tile_x * 4 + dx],
                        cb[chroma_y * src->cb_stride + chroma_x],
                        cr[chroma_y * src->cb_stride + chroma_x]);
                }
            }
        }
    }
}

void CFT_Ycc420plnToArgb8888(
    const CFTYcc420Planar* src,
    const CFTArgb8888Output* dst,
    CFTArgbTable table)
{
    if (table != 0) {
        cnvDynamicYcc420plnToArgb8888(src, dst, table);
    } else {
        cnvStaticYcc420plnToArgb8888(src, dst);
    }
}

void CFT_Ycc420plnToArgb8888Init(void)
{
    s32 i;
    float* cb_green = cb_g;
    float* cb_blue = cb_b;
    float* y_luminance = y__r;
    float* cr_red = cr_r;
    float* cr_green = cr_g;

    CFT_dummy = CFT_version;
    i = 0;
    do {
        *cb_green++ = -0.392f * (float)(i - 128);
        *cb_blue++ = 2.017f * (float)(i - 128);
        *y_luminance++ = 1.164f * (float)(i - 16);
        *cr_red++ = 1.596f * (float)(i - 128);
        *cr_green++ = -0.813f * (float)(i - 128);
        i++;
    } while (i != 256);
}
