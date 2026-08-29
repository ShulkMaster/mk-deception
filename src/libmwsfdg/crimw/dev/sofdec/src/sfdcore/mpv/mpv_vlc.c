#include "sofdec/uty_mem.h"

static const signed short mpvvlt_run_level_0c[16] = {
    0x1201, 0x1101, 0x1001, 0x0F01, 0x0306, 0x0210, 0x020F, 0x020E,
    0x020D, 0x020C, 0x020B, 0x011F, 0x011E, 0x011D, 0x011C, 0x011B,
};
static const signed short mpvvlt_run_level_0b[16] = {
    0x2800, 0x2700, 0x2600, 0x2500, 0x2400, 0x2300, 0x2200, 0x2100,
    0x2000, 0x0E01, 0x0D01, 0x0C01, 0x0B01, 0x0A01, 0x0901, 0x0801,
};
static const signed short mpvvlt_run_level_0a[16] = {
    0x1F00, 0x1E00, 0x1D00, 0x1C00, 0x1B00, 0x1A00, 0x1900, 0x1800,
    0x1700, 0x1600, 0x1500, 0x1400, 0x1300, 0x1200, 0x1100, 0x1000,
};
static const signed short mpvvlt_run_level_1[16] = {
    0x020A, 0x0209, 0x0305, 0x0403, 0x0502, 0x0701, 0x0601, 0x0F00,
    0x0E00, 0x0D00, 0x0C00, 0x011A, 0x0119, 0x0118, 0x0117, 0x0116,
};
static const signed short mpvvlt_run_level_2[16] = {
    0x0B00, 0x0208, 0x0304, 0x0A00, 0x0402, 0x0207, 0x0115, 0x0114,
    0x0900, 0x0113, 0x0112, 0x0501, 0x0303, 0x0800, 0x0206, 0x0111,
};
static const signed short mpvvlt_run_level_4[8] = {
    0x0110, 0x0205, 0x0700, 0x0302, 0x0401, 0x010F, 0x010E, 0x0204,
};

static unsigned int mpvvlt_run_level_8[128];
static unsigned char mpvvlt2_c_dcsiz[1024];
static unsigned char mpvvlt2_y_dcsiz[1024];
static unsigned char mpvvlt_c_dcsiz[128];
static unsigned char mpvvlt_y_dcsiz[128];
static signed short mpvvlt_cbp[512];
static signed short mpvvlt_motion_1[32];
static signed short mpvvlt_motion_0[128];
static signed short mpvvlt_b_mbtype[64];
static signed short mpvvlt_p_mbtype[32];
static signed short mpvvlt_mbai_b_1[32];
static signed short mpvvlt_mbai_b_0[128];
static signed short mpvvlt_mbai_p_1[32];
static signed short mpvvlt_mbai_p_0[128];
static signed short mpvvlt_mbai_i_1[64];
static signed short mpvvlt_mbai_i_0[256];

unsigned int* mpvvlc_run_level_8;
signed short* mpvvlc_run_level_4;
signed short* mpvvlc_run_level_2;
signed short* mpvvlc_run_level_1;
signed short* mpvvlc_run_level_0a;
signed short* mpvvlc_run_level_0b;
signed short* mpvvlc_run_level_0c;
unsigned char* mpvvlc2_c_dcsiz;
unsigned char* mpvvlc2_y_dcsiz;
unsigned char* mpvvlc_c_dcsiz;
unsigned char* mpvvlc_y_dcsiz;
signed short* mpvvlc_cbp;
signed short* mpvvlc_motion_1;
signed short* mpvvlc_motion_0;
signed short* mpvvlc_b_mbtype;
signed short* mpvvlc_p_mbtype;
signed short* mpvvlc_mbai_b_1;
signed short* mpvvlc_mbai_b_0;
signed short* mpvvlc_mbai_p_1;
signed short* mpvvlc_mbai_p_0;
signed short* mpvvlc_mbai_i_1;
signed short* mpvvlc_mbai_i_0;

typedef struct MPVVLCWork {
    signed short b_mbtype[64];
    signed short p_mbtype[32];
    signed short motion_1[32];
    signed short motion_0[128];
    unsigned char c_dcsiz[128];
    unsigned char y_dcsiz[128];
    signed short run_level_0c[16];
    signed short run_level_0b[16];
    signed short run_level_0a[16];
    signed short run_level_1[16];
    signed short run_level_2[16];
    signed short run_level_4[8];
    unsigned int run_level_8[128];
} MPVVLCWork;

typedef struct MPVContext MPVContext;

static inline void mpvvlc_fill_u8(unsigned char* output, long count,
                                  unsigned char value) {
    int i;

    for (i = 0; i < count; i++) {
        *output++ = value;
    }
}

#define mpvvlc_pack_motion(length, code) \
    ((signed short)(((length) << 8) | (unsigned char)(code)))
#define mpvvlc_pack_cbp(cbp, length) \
    ((signed short)(((unsigned int)(cbp) << 8) | (length)))
#define mpvvlc_pack_mbai(value, flags, length) \
    ((signed short)(((value) << 4) | (flags) | (length)))
static inline signed short mpvvlc_pack_mbai_entry(int base, int flags,
                                                   int length) {
    return (signed short)(base | flags | length);
}
#define mpvvlc_pack_mbai_base(base, flags, length) \
    ((signed short)((base) | (flags) | (length)))
#define mpvvlc_store_cbp_pair(output, index, cbp)                    \
    do {                                                             \
        signed short mpvvlc_cbp_value = mpvvlc_pack_cbp((cbp), 8);  \
        (output)[index] = mpvvlc_cbp_value;                          \
        (output)[(index) + 1] = mpvvlc_cbp_value;                    \
    } while (0)

static inline void mpvvlc_fill_s16(signed short* output, long count,
                                    signed short value) {
    int i;

    for (i = 0; i < count; i++) {
        output[i] = value;
    }
}

static inline void mpvvlc_fill_u32(unsigned int* output, long count,
                                    unsigned int value) {
    int i;

    for (i = 0; i < count; i++) {
        output[i] = value;
    }
}

#define mpvvlc_emit_mbai_p_8(output, base)                                \
    do {                                                                  \
        *(output)++ = mpvvlc_pack_mbai_base((base), 0x0000, 8);          \
        *(output)++ = mpvvlc_pack_mbai_base((base), 0xA000, 11);         \
        mpvvlc_fill_s16((output), 2,                                     \
                         mpvvlc_pack_mbai_base((base), 0x8800, 10));      \
        (output) += 2;                                                    \
        mpvvlc_fill_s16((output), 4,                                     \
                         mpvvlc_pack_mbai_base((base), 0xA800, 9));       \
        (output) += 4;                                                    \
    } while (0)

#define mpvvlc_emit_mbai_p_16(output, base)                               \
    do {                                                                  \
        mpvvlc_fill_s16((output), 2,                                     \
                         mpvvlc_pack_mbai_base((base), 0x0000, 7));       \
        (output) += 2;                                                    \
        mpvvlc_fill_s16((output), 2,                                     \
                         mpvvlc_pack_mbai_base((base), 0xA000, 10));      \
        (output) += 2;                                                    \
        mpvvlc_fill_s16((output), 4,                                     \
                         mpvvlc_pack_mbai_base((base), 0x8800, 9));       \
        (output) += 4;                                                    \
        mpvvlc_fill_s16((output), 8,                                     \
                         mpvvlc_pack_mbai_base((base), 0xA800, 8));       \
        (output) += 8;                                                    \
    } while (0)

#define mpvvlc_emit_mbai_b_8(output, base)                                \
    do {                                                                  \
        mpvvlc_fill_s16((output), 2,                                     \
                         mpvvlc_pack_mbai_base((base), 0x0000, 8));       \
        (output) += 2;                                                    \
        *(output)++ = mpvvlc_pack_mbai_base((base), 0x9000, 11);         \
        *(output)++ = mpvvlc_pack_mbai_base((base), 0x9800, 11);         \
        mpvvlc_fill_s16((output), 2,                                     \
                         mpvvlc_pack_mbai_base((base), 0xB000, 10));      \
        (output) += 2;                                                    \
        mpvvlc_fill_s16((output), 2,                                     \
                         mpvvlc_pack_mbai_base((base), 0xB800, 10));      \
        (output) += 2;                                                    \
    } while (0)

#define mpvvlc_emit_mbai_b_16(output, base)                               \
    do {                                                                  \
        mpvvlc_fill_s16((output), 2,                                     \
                         mpvvlc_pack_mbai_base((base), 0x0000, 7));       \
        (output) += 2;                                                    \
        *(output)++ = mpvvlc_pack_mbai_base((base), 0xA000, 11);         \
        *(output)++ = mpvvlc_pack_mbai_base((base), 0xA800, 11);         \
        mpvvlc_fill_s16((output), 2,                                     \
                         mpvvlc_pack_mbai_base((base), 0x9000, 10));      \
        (output) += 2;                                                    \
        mpvvlc_fill_s16((output), 2,                                     \
                         mpvvlc_pack_mbai_base((base), 0x9800, 10));      \
        (output) += 2;                                                    \
        mpvvlc_fill_s16((output), 4,                                     \
                         mpvvlc_pack_mbai_base((base), 0xB000, 9));       \
        (output) += 4;                                                    \
        mpvvlc_fill_s16((output), 4,                                     \
                         mpvvlc_pack_mbai_base((base), 0xB800, 9));       \
        (output) += 4;                                                    \
    } while (0)

static void mpvvlc_InitIntRunLevel(void) {
    unsigned int* output = mpvvlt_run_level_8;
    int i;

    for (i = 0; i < 4; i++) *output++ = 0x00000000;
    for (i = 0; i < 4; i++) *output++ = 0x00064040;
    for (i = 0; i < 2; i++) *output++ = 0x00080202;
    for (i = 0; i < 2; i++) *output++ = 0x00080109;
    for (i = 0; i < 2; i++) *output++ = 0x00080400;
    for (i = 0; i < 2; i++) *output++ = 0x00080108;
    for (i = 0; i < 4; i++) *output++ = 0x00070107;
    for (i = 0; i < 4; i++) *output++ = 0x00070106;
    for (i = 0; i < 4; i++) *output++ = 0x00070201;
    for (i = 0; i < 4; i++) *output++ = 0x00070105;
    *output++ = 0x0009010D;
    *output++ = 0x00090600;
    *output++ = 0x0009010C;
    *output++ = 0x0009010B;
    *output++ = 0x00090203;
    *output++ = 0x00090301;
    *output++ = 0x00090500;
    *output++ = 0x0009010A;
    for (i = 0; i < 8; i++) *output++ = 0x00060300;
    for (i = 0; i < 8; i++) *output++ = 0x00060104;
    for (i = 0; i < 8; i++) *output++ = 0x00060103;
    mpvvlc_fill_u32(output, 16, 0x00050200); output += 16;
    mpvvlc_fill_u32(output, 16, 0x00050102); output += 16;
    mpvvlc_fill_u32(output, 32, 0x00040101);
}

static void mpvvlc2_InitDcSizC(void) {
    unsigned char* output = mpvvlt2_c_dcsiz;

    mpvvlc_fill_u8(output, 256, 0x02); output += 256;
    mpvvlc_fill_u8(output, 256, 0x12); output += 256;
    mpvvlc_fill_u8(output, 256, 0x22); output += 256;
    mpvvlc_fill_u8(output, 128, 0x33); output += 128;
    mpvvlc_fill_u8(output, 64, 0x44); output += 64;
    mpvvlc_fill_u8(output, 32, 0x55); output += 32;
    mpvvlc_fill_u8(output, 16, 0x66); output += 16;
    output[0] = 0x77; output[1] = 0x77;
    output[2] = 0x77; output[3] = 0x77;
    output[4] = 0x77; output[5] = 0x77;
    output[6] = 0x77; output[7] = 0x77;
    output[8] = 0x88; output[9] = 0x88;
    output[10] = 0x88; output[11] = 0x88;
    output[12] = 0x99; output[13] = 0x99;
    output[14] = 0xAA;
    output[15] = 0xBA;
}

static void mpvvlc2_InitDcSizY(void) {
    unsigned char* output = mpvvlt2_y_dcsiz;

    mpvvlc_fill_u8(output, 256, 0x12); output += 256;
    mpvvlc_fill_u8(output, 256, 0x22); output += 256;
    mpvvlc_fill_u8(output, 128, 0x03); output += 128;
    mpvvlc_fill_u8(output, 128, 0x33); output += 128;
    mpvvlc_fill_u8(output, 128, 0x43); output += 128;
    mpvvlc_fill_u8(output, 64, 0x54); output += 64;
    mpvvlc_fill_u8(output, 32, 0x65); output += 32;
    mpvvlc_fill_u8(output, 16, 0x76); output += 16;
    output[0] = 0x87; output[1] = 0x87;
    output[2] = 0x87; output[3] = 0x87;
    output[4] = 0x87; output[5] = 0x87;
    output[6] = 0x87; output[7] = 0x87;
    output[8] = 0x98; output[9] = 0x98;
    output[10] = 0x98; output[11] = 0x98;
    output[12] = 0xA9; output[13] = 0xA9;
    output[14] = 0xB9; output[15] = 0xB9;
}

static void mpvvlc_InitDcSizC(void) {
    unsigned char* output = mpvvlt_c_dcsiz;

    mpvvlc_fill_u8(output, 32, 0x02);
    output += 32;
    mpvvlc_fill_u8(output, 32, 0x12);
    output += 32;
    mpvvlc_fill_u8(output, 32, 0x22);
    output += 32;
    mpvvlc_fill_u8(output, 16, 0x33);
    output += 16;
    mpvvlc_fill_u8(output, 8, 0x44);
    output += 8;
    mpvvlc_fill_u8(output, 4, 0x55);
    output += 4;
    mpvvlc_fill_u8(output, 2, 0x66);
    output += 2;
    *output++ = 0x77;
    *output = 0x88;
}

static void mpvvlc_InitDcSizY(void) {
    unsigned char* output = mpvvlt_y_dcsiz;

    mpvvlc_fill_u8(output, 32, 0x12); output += 32;
    mpvvlc_fill_u8(output, 32, 0x22); output += 32;
    mpvvlc_fill_u8(output, 16, 0x03); output += 16;
    mpvvlc_fill_u8(output, 16, 0x33); output += 16;
    mpvvlc_fill_u8(output, 16, 0x43); output += 16;
    mpvvlc_fill_u8(output, 8, 0x54); output += 8;
    mpvvlc_fill_u8(output, 4, 0x65); output += 4;
    mpvvlc_fill_u8(output, 2, 0x76); output += 2;
    *output++ = 0x87;
    *output = 0x87;
}

static signed short* mpvvlc_InitCbpSub2(signed short* output)
{
    mpvvlc_fill_s16(output, 4, 0xA207); output += 4;
    mpvvlc_fill_s16(output, 4, 0x9207); output += 4;
    mpvvlc_fill_s16(output, 4, 0x8A07); output += 4;
    mpvvlc_fill_s16(output, 4, 0x8607); output += 4;
    mpvvlc_fill_s16(output, 4, 0x6107); output += 4;
    mpvvlc_fill_s16(output, 4, 0x5107); output += 4;
    mpvvlc_fill_s16(output, 4, 0x4907); output += 4;
    mpvvlc_fill_s16(output, 4, 0x4507); output += 4;
    mpvvlc_fill_s16(output, 8, 0xFF06); output += 8;
    mpvvlc_fill_s16(output, 8, 0xC406); output += 8;
    mpvvlc_fill_s16(output, 8, 0x2406); output += 8;
    mpvvlc_fill_s16(output, 8, 0x1806); output += 8;
    mpvvlc_fill_s16(output, 16, 0xBE05); output += 16;
    mpvvlc_fill_s16(output, 16, 0x8205); output += 16;
    mpvvlc_fill_s16(output, 16, 0x7D05); output += 16;
    mpvvlc_fill_s16(output, 16, 0x4105); output += 16;
    mpvvlc_fill_s16(output, 16, 0x3805); output += 16;
    mpvvlc_fill_s16(output, 16, 0x3405); output += 16;
    mpvvlc_fill_s16(output, 16, 0x2C05); output += 16;
    mpvvlc_fill_s16(output, 16, 0x1C05); output += 16;
    mpvvlc_fill_s16(output, 16, 0x2805); output += 16;
    mpvvlc_fill_s16(output, 16, 0x1405); output += 16;
    mpvvlc_fill_s16(output, 16, 0x3005); output += 16;
    mpvvlc_fill_s16(output, 16, 0x0C05); output += 16;
    mpvvlc_fill_s16(output, 32, 0x2004); output += 32;
    mpvvlc_fill_s16(output, 32, 0x1004); output += 32;
    mpvvlc_fill_s16(output, 32, 0x0804); output += 32;
    mpvvlc_fill_s16(output, 32, 0x0404); output += 32;
    mpvvlc_fill_s16(output, 64, 0x3C03); output += 64;
    return output;
}

static signed short* mpvvlc_InitCbpSub1(signed short* output) {
    output[0] = 0;
    output[1] = 0;
    output[2] = mpvvlc_pack_cbp(0xE7, 9);
    output[3] = mpvvlc_pack_cbp(0xDB, 9);
    output[4] = mpvvlc_pack_cbp(0xFB, 9);
    output[5] = mpvvlc_pack_cbp(0xF7, 9);
    output[6] = mpvvlc_pack_cbp(0xEF, 9);
    output[7] = mpvvlc_pack_cbp(0xDF, 9);
    mpvvlc_store_cbp_pair(output, 8, 0xBA);
    mpvvlc_store_cbp_pair(output, 10, 0xB6);
    mpvvlc_store_cbp_pair(output, 12, 0xAE);
    mpvvlc_store_cbp_pair(output, 14, 0x9E);
    mpvvlc_store_cbp_pair(output, 16, 0x79);
    mpvvlc_store_cbp_pair(output, 18, 0x75);
    mpvvlc_store_cbp_pair(output, 20, 0x6D);
    mpvvlc_store_cbp_pair(output, 22, 0x5D);
    mpvvlc_store_cbp_pair(output, 24, 0xA6);
    mpvvlc_store_cbp_pair(output, 26, 0x9A);
    mpvvlc_store_cbp_pair(output, 28, 0x65);
    mpvvlc_store_cbp_pair(output, 30, 0x59);
    mpvvlc_store_cbp_pair(output, 32, 0xEB);
    mpvvlc_store_cbp_pair(output, 34, 0xD7);
    mpvvlc_store_cbp_pair(output, 36, 0xF3);
    mpvvlc_store_cbp_pair(output, 38, 0xCF);
    mpvvlc_store_cbp_pair(output, 40, 0xAA);
    mpvvlc_store_cbp_pair(output, 42, 0x96);
    mpvvlc_store_cbp_pair(output, 44, 0xB2);
    mpvvlc_store_cbp_pair(output, 46, 0x8E);
    mpvvlc_store_cbp_pair(output, 48, 0x69);
    mpvvlc_store_cbp_pair(output, 50, 0x55);
    mpvvlc_store_cbp_pair(output, 52, 0x71);
    mpvvlc_store_cbp_pair(output, 54, 0x4D);
    mpvvlc_store_cbp_pair(output, 56, 0xE3);
    mpvvlc_store_cbp_pair(output, 58, 0xD3);
    mpvvlc_store_cbp_pair(output, 60, 0xCB);
    mpvvlc_store_cbp_pair(output, 62, 0xC7);
    return output + 64;
}

static void mpvvlc_InitMotion(void) {
    signed short* output = mpvvlt_motion_0;
    int code;

    mpvvlc_fill_s16(output, 24, 0x007F);
    output += 24;
    for (code = 16; code >= 11; code--) {
        *output++ = mpvvlc_pack_motion(11, code);
        *output++ = mpvvlc_pack_motion(11, -code);
    }
    for (code = 10; code >= 8; code--) {
        mpvvlc_fill_s16(output, 2, mpvvlc_pack_motion(10, code));
        output += 2;
        mpvvlc_fill_s16(output, 2, mpvvlc_pack_motion(10, -code));
        output += 2;
    }
    for (code = 7; code >= 5; code--) {
        mpvvlc_fill_s16(output, 8, mpvvlc_pack_motion(8, code));
        output += 8;
        mpvvlc_fill_s16(output, 8, mpvvlc_pack_motion(8, -code));
        output += 8;
    }
    mpvvlc_fill_s16(output, 16, mpvvlc_pack_motion(7, 4));
    output += 16;
    mpvvlc_fill_s16(output, 16, mpvvlc_pack_motion(7, -4));

    output = mpvvlt_motion_1;
    mpvvlc_fill_s16(output, 2, 0x007F);
    output += 2;
    *output++ = mpvvlc_pack_motion(5, 3);
    *output++ = mpvvlc_pack_motion(5, -3);
    mpvvlc_fill_s16(output, 2, mpvvlc_pack_motion(4, 2));
    output += 2;
    mpvvlc_fill_s16(output, 2, mpvvlc_pack_motion(4, -2));
    output += 2;
    mpvvlc_fill_s16(output, 4, mpvvlc_pack_motion(3, 1));
    output += 4;
    mpvvlc_fill_s16(output, 4, mpvvlc_pack_motion(3, -1));
    output += 4;
    mpvvlc_fill_s16(output, 16, mpvvlc_pack_motion(1, 0));
}

static void mpvvlc_InitMbTypeBpic(void) {
    signed short* output = mpvvlt_b_mbtype;
    int i;
    long entry_count = 16;

    *output++ = 0x1F00;
    *output++ = 0x1106;
    *output++ = 0x1606;
    *output++ = 0x1A06;
    for (i = 0; i < 2; i++) *output++ = 0x1E05;
    for (i = 0; i < 2; i++) *output++ = 0x0105;
    for (i = 0; i < 4; i++) *output++ = 0x0804;
    for (i = 0; i < 4; i++) *output++ = 0x0A04;
    for (i = 0; i < 8; i++) *output++ = 0x0403;
    for (i = 0; i < 8; i++) *output++ = 0x0603;
    for (i = 0; i < entry_count; i++) *output++ = 0x0C02;
    for (i = 0; i < entry_count; i++) *output++ = 0x0E02;
}

static void mpvvlc_InitMbTypePpic(void) {
    signed short* output = mpvvlt_p_mbtype;
    int i;
    long entry_count = 16;

    *output++ = 0x1106;
    *output++ = 0x1205;
    *output++ = 0x1A05;
    *output++ = 0x0105;
    for (i = 0; i < 4; i++) *output++ = 0x0803;
    for (i = 0; i < 8; i++) *output++ = 0x0202;
    for (i = 0; i < entry_count; i++) *output++ = 0x0A01;
}

static void mpvvlc_InitMbaiBpic(void)
{
    signed short* output1 = mpvvlt_mbai_b_1;
    signed short* output = mpvvlt_mbai_b_0;
    int base;

    mpvvlc_fill_s16(output, 8, 0x0240); output += 8;
    *output++ = 0x023B;
    mpvvlc_fill_s16(output, 6, 0x0240); output += 6;
    *output++ = 0x022B;
    mpvvlc_fill_s16(output, 8, 0x0240); output += 8;

    for (base = 0x210; base >= 0x160; base -= 0x10) {
        *output++ = mpvvlc_pack_mbai_base(base, 0, 11);
    }
    for (base = 0x150; base >= 0x100; base -= 0x10) {
        mpvvlc_fill_s16(output, 2,
                         mpvvlc_pack_mbai_base(base, 0, 10));
        output += 2;
    }

    for (base = 0x0F0; base >= 0x0A0; base -= 0x10) {
        mpvvlc_emit_mbai_b_8(output, base);
    }
    mpvvlc_emit_mbai_b_16(output, 0x090);
    mpvvlc_emit_mbai_b_16(output, 0x080);

    output = output1;
    mpvvlc_fill_s16(output, 2, 0x0240); output += 2;
    *output++ = 0x0075;
    *output++ = 0x0065;
    mpvvlc_fill_s16(output, 2, 0x0054); output += 2;
    mpvvlc_fill_s16(output, 2, 0x0044); output += 2;
    mpvvlc_fill_s16(output, 2, 0x0033); output += 2;
    *output++ = 0xB035;
    *output++ = 0xB835;
    mpvvlc_fill_s16(output, 2, 0x0023); output += 2;
    *output++ = 0xB025;
    *output++ = 0xB825;
    mpvvlc_fill_s16(output, 2, 0x0011); output += 2;
    *output++ = 0xA015;
    *output++ = 0xA815;
    mpvvlc_fill_s16(output, 2, 0x9014); output += 2;
    mpvvlc_fill_s16(output, 2, 0x9814); output += 2;
    mpvvlc_fill_s16(output, 4, 0xB013); output += 4;
    mpvvlc_fill_s16(output, 4, 0xB813);
}

static void mpvvlc_InitMbaiPpic(void)
{
    signed short* output = mpvvlt_mbai_p_0;
    int base;

    mpvvlc_fill_s16(output, 8, mpvvlc_pack_mbai(0x24, 0, 0));
    output += 8;
    *output++ = mpvvlc_pack_mbai(0x23, 0, 11);
    mpvvlc_fill_s16(output, 6, mpvvlc_pack_mbai(0x24, 0, 0));
    output += 6;
    *output++ = mpvvlc_pack_mbai(0x22, 0, 11);
    mpvvlc_fill_s16(output, 8, mpvvlc_pack_mbai(0x24, 0, 0));
    output += 8;

    for (base = 0x210; base >= 0x160; base -= 0x10) {
        *output++ = mpvvlc_pack_mbai_base(base, 0, 11);
    }

    for (base = 0x150; base >= 0x100; base -= 0x10) {
        *output++ = mpvvlc_pack_mbai_base(base, 0x0000, 10);
        *output++ = mpvvlc_pack_mbai_base(base, 0xA800, 11);
    }

    for (base = 0x0F0; base >= 0x0A0; base -= 0x10) {
        mpvvlc_emit_mbai_p_8(output, base);
    }
    mpvvlc_emit_mbai_p_16(output, 0x090);
    mpvvlc_emit_mbai_p_16(output, 0x080);

    output = mpvvlt_mbai_p_1;
    mpvvlc_fill_s16(output, 2, mpvvlc_pack_mbai(0x24, 0, 0));
    output += 2;
    base = 0x070;
    *output++ = mpvvlc_pack_mbai_entry(base, 0x0000, 5);
    base -= 0x10;
    *output++ = mpvvlc_pack_mbai_entry(base, 0x0000, 5);
    base -= 0x10;
    *output++ = mpvvlc_pack_mbai_entry(base, 0x0000, 4);
    *output++ = mpvvlc_pack_mbai_entry(base, 0xA800, 5);
    base -= 0x10;
    *output++ = mpvvlc_pack_mbai_entry(base, 0x0000, 4);
    *output++ = mpvvlc_pack_mbai_entry(base, 0xA800, 5);
    base -= 0x10;
    *output++ = mpvvlc_pack_mbai_entry(base, 0x0000, 3);
    *output++ = mpvvlc_pack_mbai_entry(base, 0x8800, 5);
    mpvvlc_fill_s16(output, 2,
                     mpvvlc_pack_mbai_entry(base, 0xA800, 4));
    output += 2;
    base -= 0x10;
    *output++ = mpvvlc_pack_mbai_entry(base, 0x0000, 3);
    *output++ = mpvvlc_pack_mbai_entry(base, 0x8800, 5);
    mpvvlc_fill_s16(output, 2,
                     mpvvlc_pack_mbai_entry(base, 0xA800, 4));
    output += 2;
    base -= 0x10;
    mpvvlc_fill_s16(output, 2,
                     mpvvlc_pack_mbai_entry(base, 0x0000, 1));
    output += 2;
    mpvvlc_fill_s16(output, 2,
                     mpvvlc_pack_mbai_entry(base, 0xA000, 4));
    output += 2;
    mpvvlc_fill_s16(output, 4,
                     mpvvlc_pack_mbai_entry(base, 0x8800, 3));
    output += 4;
    mpvvlc_fill_s16(output, 8,
                     mpvvlc_pack_mbai_entry(base, 0xA800, 2));
}

static void mpvvlc_InitMbaiIpic(void)
{
    signed short* output = mpvvlt_mbai_i_0;
    int base;
    int i;

    mpvvlc_fill_s16(output, 16, 0x0240); output += 16;
    mpvvlc_fill_s16(output, 2, 0x023B); output += 2;
    mpvvlc_fill_s16(output, 12, 0x0240); output += 12;
    mpvvlc_fill_s16(output, 2, 0x022B); output += 2;
    mpvvlc_fill_s16(output, 16, 0x0240); output += 16;

    for (base = 0x210; base >= 0x160; base -= 0x10) {
        *output++ = mpvvlc_pack_mbai_entry(base, 0x4400, 13);
        *output++ = mpvvlc_pack_mbai_entry(base, 0x0400, 12);
    }

    for (base = 0x150; base >= 0x100; base -= 0x10) {
        mpvvlc_fill_s16(output, 2,
                         mpvvlc_pack_mbai_entry(base, 0x4400, 12));
        output += 2;
        mpvvlc_fill_s16(output, 2,
                         mpvvlc_pack_mbai_entry(base, 0x0400, 11));
        output += 2;
    }

    for (base = 0x0F0; base >= 0x0A0; base -= 0x10) {
        mpvvlc_fill_s16(output, 8,
                         mpvvlc_pack_mbai_entry(base, 0x4400, 10));
        output += 8;
        mpvvlc_fill_s16(output, 8,
                         mpvvlc_pack_mbai_entry(base, 0x0400, 9));
        output += 8;
    }

    base = 0x090;
    for (i = 0; i < 2; i++) {
        mpvvlc_fill_s16(output, 16,
                         mpvvlc_pack_mbai_entry(base, 0x4400, 9));
        output += 16;
        mpvvlc_fill_s16(output, 16,
                         mpvvlc_pack_mbai_entry(base, 0x0400, 8));
        output += 16;
        base -= 0x10;
    }

    output = mpvvlt_mbai_i_1;
    mpvvlc_fill_s16(output, 4, 0x0240); output += 4;
    *output++ = 0x4477;
    *output++ = 0x0476;
    *output++ = 0x4467;
    *output++ = 0x0466;
    mpvvlc_fill_s16(output, 2, 0x4456); output += 2;
    mpvvlc_fill_s16(output, 2, 0x0455); output += 2;
    mpvvlc_fill_s16(output, 2, 0x4446); output += 2;
    mpvvlc_fill_s16(output, 2, 0x0445); output += 2;
    mpvvlc_fill_s16(output, 4, 0x4435); output += 4;
    mpvvlc_fill_s16(output, 4, 0x0434); output += 4;
    mpvvlc_fill_s16(output, 4, 0x4425); output += 4;
    mpvvlc_fill_s16(output, 4, 0x0424); output += 4;
    mpvvlc_fill_s16(output, 16, 0x4413); output += 16;
    mpvvlc_fill_s16(output, 16, 0x0412);
}

void MPVVLC_Init(MPVVLCWork* work, MPVContext* decoder) {
    signed short* cbp_output;

    mpvvlc_InitMbaiIpic();
    mpvvlc_InitMbaiPpic();
    mpvvlc_InitMbaiBpic();
    mpvvlc_InitMbTypePpic();
    mpvvlc_InitMbTypeBpic();
    mpvvlc_InitMotion();
    cbp_output = mpvvlc_InitCbpSub1(mpvvlt_cbp);
    mpvvlc_InitCbpSub2(cbp_output);
    mpvvlc_InitDcSizY();
    mpvvlc_InitDcSizC();
    mpvvlc2_InitDcSizY();
    mpvvlc2_InitDcSizC();
    mpvvlc_InitIntRunLevel();

    mpvvlc_run_level_8 = mpvvlt_run_level_8;
    mpvvlc_run_level_4 = (signed short*)mpvvlt_run_level_4;
    mpvvlc_run_level_2 = (signed short*)mpvvlt_run_level_2;
    mpvvlc_run_level_1 = (signed short*)mpvvlt_run_level_1;
    mpvvlc_run_level_0a = (signed short*)mpvvlt_run_level_0a;
    mpvvlc_run_level_0b = (signed short*)mpvvlt_run_level_0b;
    mpvvlc_run_level_0c = (signed short*)mpvvlt_run_level_0c;
    mpvvlc2_c_dcsiz = mpvvlt2_c_dcsiz;
    mpvvlc2_y_dcsiz = mpvvlt2_y_dcsiz;
    mpvvlc_c_dcsiz = mpvvlt_c_dcsiz;
    mpvvlc_y_dcsiz = mpvvlt_y_dcsiz;
    mpvvlc_cbp = mpvvlt_cbp;
    mpvvlc_motion_1 = mpvvlt_motion_1;
    mpvvlc_motion_0 = mpvvlt_motion_0;
    mpvvlc_b_mbtype = mpvvlt_b_mbtype;
    mpvvlc_p_mbtype = mpvvlt_p_mbtype;
    mpvvlc_mbai_b_1 = mpvvlt_mbai_b_1;
    mpvvlc_mbai_b_0 = mpvvlt_mbai_b_0;
    mpvvlc_mbai_p_1 = mpvvlt_mbai_p_1;
    mpvvlc_mbai_p_0 = mpvvlt_mbai_p_0;
    mpvvlc_mbai_i_1 = mpvvlt_mbai_i_1;
    mpvvlc_mbai_i_0 = mpvvlt_mbai_i_0;

    if (work != 0) {
        mpvvlc_run_level_8 = work->run_level_8;
        UTY_MemcpyDword(work->run_level_8, mpvvlt_run_level_8, 128);
        mpvvlc_run_level_4 = work->run_level_4;
        UTY_MemcpyDword((unsigned int*)work->run_level_4,
                        (const unsigned int*)mpvvlt_run_level_4, 4);
        mpvvlc_run_level_2 = work->run_level_2;
        UTY_MemcpyDword((unsigned int*)work->run_level_2,
                        (const unsigned int*)mpvvlt_run_level_2, 8);
        mpvvlc_run_level_1 = work->run_level_1;
        UTY_MemcpyDword((unsigned int*)work->run_level_1,
                        (const unsigned int*)mpvvlt_run_level_1, 8);
        mpvvlc_run_level_0a = work->run_level_0a;
        UTY_MemcpyDword((unsigned int*)work->run_level_0a,
                        (const unsigned int*)mpvvlt_run_level_0a, 8);
        mpvvlc_run_level_0b = work->run_level_0b;
        UTY_MemcpyDword((unsigned int*)work->run_level_0b,
                        (const unsigned int*)mpvvlt_run_level_0b, 8);
        mpvvlc_run_level_0c = work->run_level_0c;
        UTY_MemcpyDword((unsigned int*)work->run_level_0c,
                        (const unsigned int*)mpvvlt_run_level_0c, 8);
        mpvvlc_y_dcsiz = work->y_dcsiz;
        UTY_MemcpyDword((unsigned int*)work->y_dcsiz,
                        (const unsigned int*)mpvvlt_y_dcsiz, 32);
        mpvvlc_c_dcsiz = work->c_dcsiz;
        UTY_MemcpyDword((unsigned int*)work->c_dcsiz,
                        (const unsigned int*)mpvvlt_c_dcsiz, 32);
        mpvvlc_motion_0 = work->motion_0;
        UTY_MemcpyDword((unsigned int*)work->motion_0,
                        (const unsigned int*)mpvvlt_motion_0, 64);
        mpvvlc_motion_1 = work->motion_1;
        UTY_MemcpyDword((unsigned int*)work->motion_1,
                        (const unsigned int*)mpvvlt_motion_1, 16);
        mpvvlc_p_mbtype = work->p_mbtype;
        UTY_MemcpyDword((unsigned int*)work->p_mbtype,
                        (const unsigned int*)mpvvlt_p_mbtype, 16);
        mpvvlc_b_mbtype = work->b_mbtype;
        UTY_MemcpyDword((unsigned int*)work->b_mbtype,
                        (const unsigned int*)mpvvlt_b_mbtype, 32);
    }

}

int MPVVLC_IsVlcSizErr(void) {
    int size_delta = (int)sizeof(MPVVLCWork) - 0x5B0;

    if (size_delta > 0) {
        return 1;
    }
    return size_delta < 0;
}
