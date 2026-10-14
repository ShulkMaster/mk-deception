#include "dolphin/types.h"
#include "sofdec/uty_mem.h"

#define MPVVLC_VLC_AREA_SIZE 0x5B0

#define MBT_INTRA 0x01
#define MBT_PATTERN 0x02
#define MBT_BACKWARD 0x04
#define MBT_FORWARD 0x08
#define MBT_QUANT 0x10
#define MBT_FIXED 0x20

#define MBAI_STUFFING 34
#define MBAI_ESCAPE 35
#define MBAI_ERROR 36

#define RL(len, run, level) (((len) << 16) | ((run) << 8) | (level))
#define DCSIZ(size, len) (((size) << 4) | (len))
#define CBP(cbp, len) ((((cbp) & 3) << 14) | ((cbp) << 8) | (len))
#define MV(len, code) (((len) << 8) | (u8)(code))
#define MBTYPE(type, len) (((type) << 8) | (len))
#define MBAI(type, inc, len) (((type) << 10) | ((inc) << 4) | (len))

static const u32 mpvvlt_run_level_0c[8] = {
    0x12011101, 0x10010F01, 0x03060210, 0x020F020E, 0x020D020C, 0x020B011F, 0x011E011D, 0x011C011B,
};
static const u32 mpvvlt_run_level_0b[8] = {
    0x28002700, 0x26002500, 0x24002300, 0x22002100, 0x20000E01, 0x0D010C01, 0x0B010A01, 0x09010801,
};
static const u32 mpvvlt_run_level_0a[8] = {
    0x1F001E00, 0x1D001C00, 0x1B001A00, 0x19001800, 0x17001600, 0x15001400, 0x13001200, 0x11001000,
};
static const u32 mpvvlt_run_level_1[8] = {
    0x020A0209, 0x03050403, 0x05020701, 0x06010F00, 0x0E000D00, 0x0C00011A, 0x01190118, 0x01170116,
};
static const u32 mpvvlt_run_level_2[8] = {
    0x0B000208, 0x03040A00, 0x04020207, 0x01150114, 0x09000113, 0x01120501, 0x03030800, 0x02060111,
};
static const u32 mpvvlt_run_level_4[4] = {
    0x01100205, 0x07000302, 0x0401010F, 0x010E0204,
};

s16 *mpvvlc_mbai_i_0;
s16 *mpvvlc_mbai_i_1;
s16 *mpvvlc_mbai_p_0;
s16 *mpvvlc_mbai_p_1;
s16 *mpvvlc_mbai_b_0;
s16 *mpvvlc_mbai_b_1;
s16 *mpvvlc_p_mbtype;
s16 *mpvvlc_b_mbtype;
s16 *mpvvlc_motion_0;
s16 *mpvvlc_motion_1;
s16 *mpvvlc_cbp;
s8 *mpvvlc_y_dcsiz;
s8 *mpvvlc_c_dcsiz;
s8 *mpvvlc2_y_dcsiz;
s8 *mpvvlc2_c_dcsiz;
u32 *mpvvlc_run_level_0c;
u32 *mpvvlc_run_level_0b;
u32 *mpvvlc_run_level_0a;
u32 *mpvvlc_run_level_1;
u32 *mpvvlc_run_level_2;
u32 *mpvvlc_run_level_4;
u32 *mpvvlc_run_level_8;

static s16 mpvvlt_mbai_i_0[0x100];
static s16 mpvvlt_mbai_i_1[0x40];
static s16 mpvvlt_mbai_p_0[0x80];
static s16 mpvvlt_mbai_p_1[0x20];
static s16 mpvvlt_mbai_b_0[0x80];
static s16 mpvvlt_mbai_b_1[0x20];
static s16 mpvvlt_p_mbtype[0x20];
static s16 mpvvlt_b_mbtype[0x40];
static s16 mpvvlt_motion_0[0x80];
static s16 mpvvlt_motion_1[0x20];
static s16 mpvvlt_cbp[0x200];
static s8 mpvvlt_y_dcsiz[0x80];
static s8 mpvvlt_c_dcsiz[0x80];
static s8 mpvvlt2_y_dcsiz[0x400];
static s8 mpvvlt2_c_dcsiz[0x400];
static u32 mpvvlt_run_level_8[0x80];

static void mpvvlc_InitMbaiIpic(void);
static void mpvvlc_InitMbaiPpic(void);
static void mpvvlc_InitMbaiBpic(void);
static void mpvvlc_InitMbTypePpic(void);
static void mpvvlc_InitMbTypeBpic(void);
static void mpvvlc_InitMotion(void);
static s16 *mpvvlc_InitCbpSub1(s16 *p);
static s16 *mpvvlc_InitCbpSub2(s16 *p);
static inline void mpvvlc_InitCbp(void);
static void mpvvlc_InitDcSizY(void);
static void mpvvlc_InitDcSizC(void);
static void mpvvlc2_InitDcSizY(void);
static void mpvvlc2_InitDcSizC(void);
static void mpvvlc_InitIntRunLevel(void);

s32 MPVVLC_IsVlcSizErr(void)
{
    s32 siz;
    s32 sz = sizeof(s16);

    if (sz != 2) {
        return 1;
    }
    siz = MPVVLC_VLC_AREA_SIZE - (sizeof(mpvvlt_run_level_8) + sizeof(mpvvlt_run_level_4) +
                      sizeof(mpvvlt_run_level_2) + sizeof(mpvvlt_run_level_1) +
                      sizeof(mpvvlt_run_level_0a) + sizeof(mpvvlt_run_level_0b) +
                      sizeof(mpvvlt_run_level_0c) + sizeof(mpvvlt_y_dcsiz) +
                      sizeof(mpvvlt_c_dcsiz) + sizeof(mpvvlt_motion_0) + sizeof(mpvvlt_motion_1) +
                      sizeof(mpvvlt_p_mbtype) + sizeof(mpvvlt_b_mbtype));
    return siz < 0;
}

void MPVVLC_Init(u8 *vlc, void *ixa)
{
    mpvvlc_InitMbaiIpic();
    mpvvlc_InitMbaiPpic();
    mpvvlc_InitMbaiBpic();
    mpvvlc_InitMbTypePpic();
    mpvvlc_InitMbTypeBpic();
    mpvvlc_InitMotion();
    mpvvlc_InitCbp();
    mpvvlc_InitDcSizY();
    mpvvlc_InitDcSizC();
    mpvvlc2_InitDcSizY();
    mpvvlc2_InitDcSizC();
    mpvvlc_InitIntRunLevel();

    mpvvlc_mbai_i_0 = mpvvlt_mbai_i_0;
    mpvvlc_mbai_i_1 = mpvvlt_mbai_i_1;
    mpvvlc_mbai_p_0 = mpvvlt_mbai_p_0;
    mpvvlc_mbai_p_1 = mpvvlt_mbai_p_1;
    mpvvlc_mbai_b_0 = mpvvlt_mbai_b_0;
    mpvvlc_mbai_b_1 = mpvvlt_mbai_b_1;
    mpvvlc_p_mbtype = mpvvlt_p_mbtype;
    mpvvlc_b_mbtype = mpvvlt_b_mbtype;
    mpvvlc_motion_0 = mpvvlt_motion_0;
    mpvvlc_motion_1 = mpvvlt_motion_1;
    mpvvlc_cbp = mpvvlt_cbp;
    mpvvlc_y_dcsiz = mpvvlt_y_dcsiz;
    mpvvlc_c_dcsiz = mpvvlt_c_dcsiz;
    mpvvlc2_y_dcsiz = mpvvlt2_y_dcsiz;
    mpvvlc2_c_dcsiz = mpvvlt2_c_dcsiz;
    mpvvlc_run_level_0c = (u32 *)mpvvlt_run_level_0c;
    mpvvlc_run_level_0b = (u32 *)mpvvlt_run_level_0b;
    mpvvlc_run_level_0a = (u32 *)mpvvlt_run_level_0a;
    mpvvlc_run_level_1 = (u32 *)mpvvlt_run_level_1;
    mpvvlc_run_level_2 = (u32 *)mpvvlt_run_level_2;
    mpvvlc_run_level_4 = (u32 *)mpvvlt_run_level_4;
    mpvvlc_run_level_8 = mpvvlt_run_level_8;

    if (vlc != 0) {
        u8 *p;
        u8 *rl;

        p = vlc + MPVVLC_VLC_AREA_SIZE;
        p -= sizeof(mpvvlt_run_level_8);
        mpvvlc_run_level_8 = (u32 *)p;
        UTY_MemcpyDword((unsigned int*)p, (unsigned int*)mpvvlt_run_level_8, sizeof(mpvvlt_run_level_8) / 4);
        p -= sizeof(mpvvlt_run_level_4);
        mpvvlc_run_level_4 = (u32 *)p;
        UTY_MemcpyDword((unsigned int*)p, (unsigned int*)mpvvlt_run_level_4, sizeof(mpvvlt_run_level_4) / 4);
        p -= sizeof(mpvvlt_run_level_2);
        mpvvlc_run_level_2 = (u32 *)p;
        UTY_MemcpyDword((unsigned int*)p, (unsigned int*)mpvvlt_run_level_2, sizeof(mpvvlt_run_level_2) / 4);
        p -= sizeof(mpvvlt_run_level_1);
        mpvvlc_run_level_1 = (u32 *)p;
        UTY_MemcpyDword((unsigned int*)p, (unsigned int*)mpvvlt_run_level_1, sizeof(mpvvlt_run_level_1) / 4);
        p -= sizeof(mpvvlt_run_level_0a);
        mpvvlc_run_level_0a = (u32 *)p;
        UTY_MemcpyDword((unsigned int*)p, (unsigned int*)mpvvlt_run_level_0a, sizeof(mpvvlt_run_level_0a) / 4);
        p -= sizeof(mpvvlt_run_level_0b);
        mpvvlc_run_level_0b = (u32 *)p;
        UTY_MemcpyDword((unsigned int*)p, (unsigned int*)mpvvlt_run_level_0b, sizeof(mpvvlt_run_level_0b) / 4);
        p -= sizeof(mpvvlt_run_level_0c);
        mpvvlc_run_level_0c = (u32 *)p;
        UTY_MemcpyDword((unsigned int*)p, (unsigned int*)mpvvlt_run_level_0c, sizeof(mpvvlt_run_level_0c) / 4);
        rl = p;
        p -= sizeof(mpvvlt_y_dcsiz);
        mpvvlc_y_dcsiz = (s8 *)p;
        UTY_MemcpyDword((unsigned int*)p, (unsigned int*)mpvvlt_y_dcsiz, sizeof(mpvvlt_y_dcsiz) / 4);
        p = rl - (sizeof(mpvvlt_y_dcsiz) + sizeof(mpvvlt_c_dcsiz));
        mpvvlc_c_dcsiz = (s8 *)p;
        UTY_MemcpyDword((unsigned int*)p, (unsigned int*)mpvvlt_c_dcsiz, sizeof(mpvvlt_c_dcsiz) / 4);
        p = rl - (sizeof(mpvvlt_y_dcsiz) + sizeof(mpvvlt_c_dcsiz) + sizeof(mpvvlt_motion_0));
        mpvvlc_motion_0 = (s16 *)p;
        UTY_MemcpyDword((unsigned int*)p, (unsigned int*)mpvvlt_motion_0, sizeof(mpvvlt_motion_0) / 4);
        p = rl - (sizeof(mpvvlt_y_dcsiz) + sizeof(mpvvlt_c_dcsiz) + sizeof(mpvvlt_motion_0) +
              sizeof(mpvvlt_motion_1));
        mpvvlc_motion_1 = (s16 *)p;
        UTY_MemcpyDword((unsigned int*)p, (unsigned int*)mpvvlt_motion_1, sizeof(mpvvlt_motion_1) / 4);
        p = rl - (sizeof(mpvvlt_y_dcsiz) + sizeof(mpvvlt_c_dcsiz) + sizeof(mpvvlt_motion_0) +
              sizeof(mpvvlt_motion_1) + sizeof(mpvvlt_p_mbtype));
        mpvvlc_p_mbtype = (s16 *)p;
        UTY_MemcpyDword((unsigned int*)p, (unsigned int*)mpvvlt_p_mbtype, sizeof(mpvvlt_p_mbtype) / 4);
        p = rl - (sizeof(mpvvlt_y_dcsiz) + sizeof(mpvvlt_c_dcsiz) + sizeof(mpvvlt_motion_0) +
              sizeof(mpvvlt_motion_1) + sizeof(mpvvlt_p_mbtype) + sizeof(mpvvlt_b_mbtype));
        mpvvlc_b_mbtype = (s16 *)p;
        UTY_MemcpyDword((unsigned int*)p, (unsigned int*)mpvvlt_b_mbtype, sizeof(mpvvlt_b_mbtype) / 4);
    }
}

#define MPVVLC_SET_MBAI_I(kmax, kmin, len, n)                                                  \
    {                                                                                      \
        s32 k;                                                                      \
        s32 i;                                                                      \
        s16 v;                                                                      \
        for (k = (kmax); k >= (kmin); k--) {                                           \
            v = MBAI(MBT_INTRA | MBT_QUANT, k, (len) + 2);                         \
            for (i = 0; i < (n); i++) *p++ = v;                                    \
            v = MBAI(MBT_INTRA, k, (len) + 1);                                     \
            for (i = 0; i < (n); i++) *p++ = v;                                    \
        }                                                                              \
    }

static void mpvvlc_InitMbaiIpic(void)
{
    s16 *p;
    s32 i;

    p = mpvvlt_mbai_i_0;
    for (i = 0; i < 16; i++) *p++ = MBAI(0, MBAI_ERROR, 0);
    for (i = 0; i < 2; i++) *p++ = MBAI(0, MBAI_ESCAPE, 11);
    for (i = 0; i < 12; i++) *p++ = MBAI(0, MBAI_ERROR, 0);
    for (i = 0; i < 2; i++) *p++ = MBAI(0, MBAI_STUFFING, 11);
    for (i = 0; i < 16; i++) *p++ = MBAI(0, MBAI_ERROR, 0);
    MPVVLC_SET_MBAI_I(33, 22, 11, 1);
    MPVVLC_SET_MBAI_I(21, 16, 10, 2);
    MPVVLC_SET_MBAI_I(15, 10, 8, 8);
    MPVVLC_SET_MBAI_I(9, 8, 7, 16);

    p = mpvvlt_mbai_i_1;
    for (i = 0; i < 4; i++) *p++ = MBAI(0, MBAI_ERROR, 0);
    MPVVLC_SET_MBAI_I(7, 6, 5, 1);
    MPVVLC_SET_MBAI_I(5, 4, 4, 2);
    MPVVLC_SET_MBAI_I(3, 2, 3, 4);
    MPVVLC_SET_MBAI_I(1, 1, 1, 16);
}

#define MBT_P_MC (MBT_FIXED | MBT_FORWARD)
#define MBT_P_PAT (MBT_FIXED | MBT_PATTERN)
#define MBT_P_MCPAT (MBT_FIXED | MBT_FORWARD | MBT_PATTERN)

#define MPVVLC_SET_MBAI_P0(kmax, kmin, len)                                                    \
    {                                                                                      \
        s32 k;                                                                      \
        for (k = (kmax); k >= (kmin); k--) {                                           \
            *p++ = MBAI(0, k, (len));                                              \
        }                                                                              \
    }
#define MPVVLC_SET_MBAI_P1(kmax, kmin, len)                                                    \
    {                                                                                      \
        s32 k;                                                                      \
        for (k = (kmax); k >= (kmin); k--) {                                           \
            *p++ = MBAI(0, k, (len));                                              \
            *p++ = MBAI(MBT_P_MCPAT, k, (len) + 1);                                \
        }                                                                              \
    }
#define MPVVLC_SET_MBAI_P2(kmax, kmin, len)                                                    \
    {                                                                                      \
        s32 k;                                                                      \
        s32 i;                                                                      \
        for (k = (kmax); k >= (kmin); k--) {                                           \
            *p++ = MBAI(0, k, (len));                                              \
            *p++ = MBAI(MBT_P_PAT, k, (len) + 2);                                  \
            for (i = 0; i < 2; i++) *p++ = MBAI(MBT_P_MCPAT, k, (len) + 1);        \
        }                                                                              \
    }
#define MPVVLC_SET_MBAI_P3(kmax, kmin, len)                                                    \
    {                                                                                      \
        s32 k;                                                                      \
        s32 i;                                                                      \
        for (k = (kmax); k >= (kmin); k--) {                                           \
            *p++ = MBAI(0, k, (len));                                              \
            *p++ = MBAI(MBT_P_MC, k, (len) + 3);                                   \
            for (i = 0; i < 2; i++) *p++ = MBAI(MBT_P_PAT, k, (len) + 2);          \
            for (i = 0; i < 4; i++) *p++ = MBAI(MBT_P_MCPAT, k, (len) + 1);        \
        }                                                                              \
    }
#define MPVVLC_SET_MBAI_P4(kmax, kmin, len)                                                    \
    {                                                                                      \
        s32 k;                                                                      \
        s32 i;                                                                      \
        for (k = (kmax); k >= (kmin); k--) {                                           \
            for (i = 0; i < 2; i++) *p++ = MBAI(0, k, (len));                      \
            for (i = 0; i < 2; i++) *p++ = MBAI(MBT_P_MC, k, (len) + 3);           \
            for (i = 0; i < 4; i++) *p++ = MBAI(MBT_P_PAT, k, (len) + 2);          \
            for (i = 0; i < 8; i++) *p++ = MBAI(MBT_P_MCPAT, k, (len) + 1);        \
        }                                                                              \
    }

static void mpvvlc_InitMbaiPpic(void)
{
    s16 *p;
    s32 i;

    p = mpvvlt_mbai_p_0;
    for (i = 0; i < 8; i++) *p++ = MBAI(0, MBAI_ERROR, 0);
    *p++ = MBAI(0, MBAI_ESCAPE, 11);
    for (i = 0; i < 6; i++) *p++ = MBAI(0, MBAI_ERROR, 0);
    *p++ = MBAI(0, MBAI_STUFFING, 11);
    for (i = 0; i < 8; i++) *p++ = MBAI(0, MBAI_ERROR, 0);
    MPVVLC_SET_MBAI_P0(33, 22, 11);
    MPVVLC_SET_MBAI_P1(21, 16, 10);
    MPVVLC_SET_MBAI_P3(15, 10, 8);
    MPVVLC_SET_MBAI_P4(9, 8, 7);

    p = mpvvlt_mbai_p_1;
    for (i = 0; i < 2; i++) *p++ = MBAI(0, MBAI_ERROR, 0);
    MPVVLC_SET_MBAI_P0(7, 6, 5);
    MPVVLC_SET_MBAI_P1(5, 4, 4);
    MPVVLC_SET_MBAI_P2(3, 2, 3);
    MPVVLC_SET_MBAI_P4(1, 1, 1);
}

#define MBT_B_FWD (MBT_FIXED | MBT_FORWARD)
#define MBT_B_FWDPAT (MBT_FIXED | MBT_FORWARD | MBT_PATTERN)
#define MBT_B_BWD (MBT_FIXED | MBT_BACKWARD)
#define MBT_B_BWDPAT (MBT_FIXED | MBT_BACKWARD | MBT_PATTERN)
#define MBT_B_BI (MBT_FIXED | MBT_FORWARD | MBT_BACKWARD)
#define MBT_B_BIPAT (MBT_FIXED | MBT_FORWARD | MBT_BACKWARD | MBT_PATTERN)

#define MPVVLC_SET_MBAI_B0(kmax, kmin, len)                                                    \
    {                                                                                      \
        s32 k;                                                                      \
        for (k = (kmax); k >= (kmin); k--) {                                           \
            *p++ = MBAI(0, k, (len));                                              \
        }                                                                              \
    }
#define MPVVLC_SET_MBAI_B1(kmax, kmin, len)                                                    \
    {                                                                                      \
        s32 k;                                                                      \
        s32 i;                                                                      \
        for (k = (kmax); k >= (kmin); k--) {                                           \
            for (i = 0; i < 2; i++) *p++ = MBAI(0, k, (len));                      \
        }                                                                              \
    }
#define MPVVLC_SET_MBAI_B2(kmax, kmin, len)                                                    \
    {                                                                                      \
        s32 k;                                                                      \
        s32 i;                                                                      \
        for (k = (kmax); k >= (kmin); k--) {                                           \
            for (i = 0; i < 2; i++) *p++ = MBAI(0, k, (len));                      \
            *p++ = MBAI(MBT_B_BI, k, (len) + 2);                                   \
            *p++ = MBAI(MBT_B_BIPAT, k, (len) + 2);                                \
        }                                                                              \
    }
#define MPVVLC_SET_MBAI_B3(kmax, kmin, len)                                                    \
    {                                                                                      \
        s32 k;                                                                      \
        s32 i;                                                                      \
        for (k = (kmax); k >= (kmin); k--) {                                           \
            for (i = 0; i < 2; i++) *p++ = MBAI(0, k, (len));                      \
            *p++ = MBAI(MBT_B_BWD, k, (len) + 3);                                  \
            *p++ = MBAI(MBT_B_BWDPAT, k, (len) + 3);                               \
            for (i = 0; i < 2; i++) *p++ = MBAI(MBT_B_BI, k, (len) + 2);           \
            for (i = 0; i < 2; i++) *p++ = MBAI(MBT_B_BIPAT, k, (len) + 2);        \
        }                                                                              \
    }
#define MPVVLC_SET_MBAI_B4(kmax, kmin, len)                                                    \
    {                                                                                      \
        s32 k;                                                                      \
        s32 i;                                                                      \
        for (k = (kmax); k >= (kmin); k--) {                                           \
            for (i = 0; i < 2; i++) *p++ = MBAI(0, k, (len));                      \
            *p++ = MBAI(MBT_B_FWD, k, (len) + 4);                                  \
            *p++ = MBAI(MBT_B_FWDPAT, k, (len) + 4);                               \
            for (i = 0; i < 2; i++) *p++ = MBAI(MBT_B_BWD, k, (len) + 3);          \
            for (i = 0; i < 2; i++) *p++ = MBAI(MBT_B_BWDPAT, k, (len) + 3);       \
            for (i = 0; i < 4; i++) *p++ = MBAI(MBT_B_BI, k, (len) + 2);           \
            for (i = 0; i < 4; i++) *p++ = MBAI(MBT_B_BIPAT, k, (len) + 2);        \
        }                                                                              \
    }

static void mpvvlc_InitMbaiBpic(void)
{
    s16 *p;
    s32 i;

    p = mpvvlt_mbai_b_0;
    for (i = 0; i < 8; i++) *p++ = MBAI(0, MBAI_ERROR, 0);
    *p++ = MBAI(0, MBAI_ESCAPE, 11);
    for (i = 0; i < 6; i++) *p++ = MBAI(0, MBAI_ERROR, 0);
    *p++ = MBAI(0, MBAI_STUFFING, 11);
    for (i = 0; i < 8; i++) *p++ = MBAI(0, MBAI_ERROR, 0);
    MPVVLC_SET_MBAI_B0(33, 22, 11);
    MPVVLC_SET_MBAI_B1(21, 16, 10);
    MPVVLC_SET_MBAI_B3(15, 10, 8);
    MPVVLC_SET_MBAI_B4(9, 8, 7);

    p = mpvvlt_mbai_b_1;
    for (i = 0; i < 2; i++) *p++ = MBAI(0, MBAI_ERROR, 0);
    MPVVLC_SET_MBAI_B0(7, 6, 5);
    MPVVLC_SET_MBAI_B1(5, 4, 4);
    MPVVLC_SET_MBAI_B2(3, 2, 3);
    MPVVLC_SET_MBAI_B4(1, 1, 1);
}

static void mpvvlc_InitMbTypePpic(void)
{
    s16 *p;
    s32 i;

    p = mpvvlt_p_mbtype;
    *p++ = MBTYPE(MBT_INTRA | MBT_QUANT, 6);
    *p++ = MBTYPE(MBT_PATTERN | MBT_QUANT, 5);
    *p++ = MBTYPE(MBT_FORWARD | MBT_PATTERN | MBT_QUANT, 5);
    *p++ = MBTYPE(MBT_INTRA, 5);
    for (i = 0; i < 4; i++) *p++ = MBTYPE(MBT_FORWARD, 3);
    for (i = 0; i < 8; i++) *p++ = MBTYPE(MBT_PATTERN, 2);
    for (i = 0; i < 16; i++) *p++ = MBTYPE(MBT_FORWARD | MBT_PATTERN, 1);
}

static void mpvvlc_InitMbTypeBpic(void)
{
    s16 *p;
    s32 i;

    p = mpvvlt_b_mbtype;
    *p++ = MBTYPE(0x1F, 0);
    *p++ = MBTYPE(MBT_INTRA | MBT_QUANT, 6);
    *p++ = MBTYPE(MBT_BACKWARD | MBT_PATTERN | MBT_QUANT, 6);
    *p++ = MBTYPE(MBT_FORWARD | MBT_PATTERN | MBT_QUANT, 6);
    for (i = 0; i < 2; i++) *p++ = MBTYPE(MBT_FORWARD | MBT_BACKWARD | MBT_PATTERN | MBT_QUANT, 5);
    for (i = 0; i < 2; i++) *p++ = MBTYPE(MBT_INTRA, 5);
    for (i = 0; i < 4; i++) *p++ = MBTYPE(MBT_FORWARD, 4);
    for (i = 0; i < 4; i++) *p++ = MBTYPE(MBT_FORWARD | MBT_PATTERN, 4);
    for (i = 0; i < 8; i++) *p++ = MBTYPE(MBT_BACKWARD, 3);
    for (i = 0; i < 8; i++) *p++ = MBTYPE(MBT_BACKWARD | MBT_PATTERN, 3);
    for (i = 0; i < 16; i++) *p++ = MBTYPE(MBT_FORWARD | MBT_BACKWARD, 2);
    for (i = 0; i < 16; i++) *p++ = MBTYPE(MBT_FORWARD | MBT_BACKWARD | MBT_PATTERN, 2);
}

#define MPVVLC_SET_MOTION(len, kmax, kmin, n)                                                  \
    {                                                                                      \
        s32 k;                                                                      \
        s32 i;                                                                      \
        s16 v;                                                                      \
        for (k = (kmax); k >= (kmin); k--) {                                           \
            v = MV(len, k);                                                        \
            for (i = 0; i < (n); i++) *p++ = v;                                    \
            v = MV(len, -k);                                                       \
            for (i = 0; i < (n); i++) *p++ = v;                                    \
        }                                                                              \
    }

#define MPVVLC_SET_MOTION_POS(len, kmax, kmin, n)                                              \
    {                                                                                      \
        s32 k;                                                                      \
        s32 i;                                                                      \
        s16 v;                                                                      \
        for (k = (kmax); k >= (kmin); k--) {                                           \
            v = MV(len, k);                                                        \
            for (i = 0; i < (n); i++) *p++ = v;                                    \
        }                                                                              \
    }

static void mpvvlc_InitMotion(void)
{
    s16 *p;
    s32 i;

    p = mpvvlt_motion_0;
    for (i = 0; i < 24; i++) *p++ = 0x7F;
    MPVVLC_SET_MOTION(11, 16, 11, 1);
    MPVVLC_SET_MOTION(10, 10, 8, 2);
    MPVVLC_SET_MOTION(8, 7, 5, 8);
    MPVVLC_SET_MOTION(7, 4, 4, 16);

    p = mpvvlt_motion_1;
    for (i = 0; i < 2; i++) *p++ = 0x7F;
    MPVVLC_SET_MOTION(5, 3, 3, 1);
    MPVVLC_SET_MOTION(4, 2, 2, 2);
    MPVVLC_SET_MOTION(3, 1, 1, 4);
    MPVVLC_SET_MOTION_POS(1, 0, 0, 16);
}

static s16 *mpvvlc_InitCbpSub1(s16 *p)
{
    s32 i;

    for (i = 0; i < 2; i++) *p++ = CBP(0, 0);
    *p++ = CBP(39, 9);
    *p++ = CBP(27, 9);
    *p++ = CBP(59, 9);
    *p++ = CBP(55, 9);
    *p++ = CBP(47, 9);
    *p++ = CBP(31, 9);
    for (i = 0; i < 2; i++) *p++ = CBP(58, 8);
    for (i = 0; i < 2; i++) *p++ = CBP(54, 8);
    for (i = 0; i < 2; i++) *p++ = CBP(46, 8);
    for (i = 0; i < 2; i++) *p++ = CBP(30, 8);
    for (i = 0; i < 2; i++) *p++ = CBP(57, 8);
    for (i = 0; i < 2; i++) *p++ = CBP(53, 8);
    for (i = 0; i < 2; i++) *p++ = CBP(45, 8);
    for (i = 0; i < 2; i++) *p++ = CBP(29, 8);
    for (i = 0; i < 2; i++) *p++ = CBP(38, 8);
    for (i = 0; i < 2; i++) *p++ = CBP(26, 8);
    for (i = 0; i < 2; i++) *p++ = CBP(37, 8);
    for (i = 0; i < 2; i++) *p++ = CBP(25, 8);
    for (i = 0; i < 2; i++) *p++ = CBP(43, 8);
    for (i = 0; i < 2; i++) *p++ = CBP(23, 8);
    for (i = 0; i < 2; i++) *p++ = CBP(51, 8);
    for (i = 0; i < 2; i++) *p++ = CBP(15, 8);
    for (i = 0; i < 2; i++) *p++ = CBP(42, 8);
    for (i = 0; i < 2; i++) *p++ = CBP(22, 8);
    for (i = 0; i < 2; i++) *p++ = CBP(50, 8);
    for (i = 0; i < 2; i++) *p++ = CBP(14, 8);
    for (i = 0; i < 2; i++) *p++ = CBP(41, 8);
    for (i = 0; i < 2; i++) *p++ = CBP(21, 8);
    for (i = 0; i < 2; i++) *p++ = CBP(49, 8);
    for (i = 0; i < 2; i++) *p++ = CBP(13, 8);
    for (i = 0; i < 2; i++) *p++ = CBP(35, 8);
    for (i = 0; i < 2; i++) *p++ = CBP(19, 8);
    for (i = 0; i < 2; i++) *p++ = CBP(11, 8);
    for (i = 0; i < 2; i++) *p++ = CBP(7, 8);
    return p;
}

static s16 *mpvvlc_InitCbpSub2(s16 *p)
{
    s32 i;

    for (i = 0; i < 4; i++) *p++ = CBP(34, 7);
    for (i = 0; i < 4; i++) *p++ = CBP(18, 7);
    for (i = 0; i < 4; i++) *p++ = CBP(10, 7);
    for (i = 0; i < 4; i++) *p++ = CBP(6, 7);
    for (i = 0; i < 4; i++) *p++ = CBP(33, 7);
    for (i = 0; i < 4; i++) *p++ = CBP(17, 7);
    for (i = 0; i < 4; i++) *p++ = CBP(9, 7);
    for (i = 0; i < 4; i++) *p++ = CBP(5, 7);
    for (i = 0; i < 8; i++) *p++ = CBP(63, 6);
    for (i = 0; i < 8; i++) *p++ = CBP(3, 6);
    for (i = 0; i < 8; i++) *p++ = CBP(36, 6);
    for (i = 0; i < 8; i++) *p++ = CBP(24, 6);
    for (i = 0; i < 16; i++) *p++ = CBP(62, 5);
    for (i = 0; i < 16; i++) *p++ = CBP(2, 5);
    for (i = 0; i < 16; i++) *p++ = CBP(61, 5);
    for (i = 0; i < 16; i++) *p++ = CBP(1, 5);
    for (i = 0; i < 16; i++) *p++ = CBP(56, 5);
    for (i = 0; i < 16; i++) *p++ = CBP(52, 5);
    for (i = 0; i < 16; i++) *p++ = CBP(44, 5);
    for (i = 0; i < 16; i++) *p++ = CBP(28, 5);
    for (i = 0; i < 16; i++) *p++ = CBP(40, 5);
    for (i = 0; i < 16; i++) *p++ = CBP(20, 5);
    for (i = 0; i < 16; i++) *p++ = CBP(48, 5);
    for (i = 0; i < 16; i++) *p++ = CBP(12, 5);
    for (i = 0; i < 32; i++) *p++ = CBP(32, 4);
    for (i = 0; i < 32; i++) *p++ = CBP(16, 4);
    for (i = 0; i < 32; i++) *p++ = CBP(8, 4);
    for (i = 0; i < 32; i++) *p++ = CBP(4, 4);
    for (i = 0; i < 64; i++) *p++ = CBP(60, 3);
    return p;
}

static inline void mpvvlc_InitCbp(void)
{
    s16 *p;

    p = mpvvlc_InitCbpSub1(mpvvlt_cbp);
    mpvvlc_InitCbpSub2(p);
}

static void mpvvlc_InitDcSizY(void)
{
    s8 *p;
    s32 i;

    p = mpvvlt_y_dcsiz;
    for (i = 0; i < 32; i++) *p++ = DCSIZ(1, 2);
    for (i = 0; i < 32; i++) *p++ = DCSIZ(2, 2);
    for (i = 0; i < 16; i++) *p++ = DCSIZ(0, 3);
    for (i = 0; i < 16; i++) *p++ = DCSIZ(3, 3);
    for (i = 0; i < 16; i++) *p++ = DCSIZ(4, 3);
    for (i = 0; i < 8; i++) *p++ = DCSIZ(5, 4);
    for (i = 0; i < 4; i++) *p++ = DCSIZ(6, 5);
    for (i = 0; i < 2; i++) *p++ = DCSIZ(7, 6);
    for (i = 0; i < 2; i++) *p++ = DCSIZ(8, 7);
}

static void mpvvlc_InitDcSizC(void)
{
    s8 *p;
    s32 i;

    p = mpvvlt_c_dcsiz;
    for (i = 0; i < 32; i++) *p++ = DCSIZ(0, 2);
    for (i = 0; i < 32; i++) *p++ = DCSIZ(1, 2);
    for (i = 0; i < 32; i++) *p++ = DCSIZ(2, 2);
    for (i = 0; i < 16; i++) *p++ = DCSIZ(3, 3);
    for (i = 0; i < 8; i++) *p++ = DCSIZ(4, 4);
    for (i = 0; i < 4; i++) *p++ = DCSIZ(5, 5);
    for (i = 0; i < 2; i++) *p++ = DCSIZ(6, 6);
    *p++ = DCSIZ(7, 7);
    *p++ = DCSIZ(8, 8);
}

static void mpvvlc2_InitDcSizY(void)
{
    s8 *p;
    s32 i;

    p = mpvvlt2_y_dcsiz;
    for (i = 0; i < 256; i++) *p++ = DCSIZ(1, 2);
    for (i = 0; i < 256; i++) *p++ = DCSIZ(2, 2);
    for (i = 0; i < 128; i++) *p++ = DCSIZ(0, 3);
    for (i = 0; i < 128; i++) *p++ = DCSIZ(3, 3);
    for (i = 0; i < 128; i++) *p++ = DCSIZ(4, 3);
    for (i = 0; i < 64; i++) *p++ = DCSIZ(5, 4);
    for (i = 0; i < 32; i++) *p++ = DCSIZ(6, 5);
    for (i = 0; i < 16; i++) *p++ = DCSIZ(7, 6);
    for (i = 0; i < 8; i++) *p++ = DCSIZ(8, 7);
    for (i = 0; i < 4; i++) *p++ = DCSIZ(9, 8);
    for (i = 0; i < 2; i++) *p++ = DCSIZ(10, 9);
    for (i = 0; i < 2; i++) *p++ = DCSIZ(11, 9);
}

static void mpvvlc2_InitDcSizC(void)
{
    s8 *p;
    s32 i;

    p = mpvvlt2_c_dcsiz;
    for (i = 0; i < 256; i++) *p++ = DCSIZ(0, 2);
    for (i = 0; i < 256; i++) *p++ = DCSIZ(1, 2);
    for (i = 0; i < 256; i++) *p++ = DCSIZ(2, 2);
    for (i = 0; i < 128; i++) *p++ = DCSIZ(3, 3);
    for (i = 0; i < 64; i++) *p++ = DCSIZ(4, 4);
    for (i = 0; i < 32; i++) *p++ = DCSIZ(5, 5);
    for (i = 0; i < 16; i++) *p++ = DCSIZ(6, 6);
    for (i = 0; i < 8; i++) *p++ = DCSIZ(7, 7);
    for (i = 0; i < 4; i++) *p++ = DCSIZ(8, 8);
    for (i = 0; i < 2; i++) *p++ = DCSIZ(9, 9);
    *p++ = DCSIZ(10, 10);
    *p++ = DCSIZ(11, 10);
}

static void mpvvlc_InitIntRunLevel(void)
{
    u32 *p;
    s32 i;

    p = mpvvlt_run_level_8;
    for (i = 0; i < 4; i++) *p++ = RL(0, 0, 0);
    for (i = 0; i < 4; i++) *p++ = RL(6, 0x40, 0x40);
    for (i = 0; i < 2; i++) *p++ = RL(8, 2, 2);
    for (i = 0; i < 2; i++) *p++ = RL(8, 1, 9);
    for (i = 0; i < 2; i++) *p++ = RL(8, 4, 0);
    for (i = 0; i < 2; i++) *p++ = RL(8, 1, 8);
    for (i = 0; i < 4; i++) *p++ = RL(7, 1, 7);
    for (i = 0; i < 4; i++) *p++ = RL(7, 1, 6);
    for (i = 0; i < 4; i++) *p++ = RL(7, 2, 1);
    for (i = 0; i < 4; i++) *p++ = RL(7, 1, 5);
    *p++ = RL(9, 1, 13);
    *p++ = RL(9, 6, 0);
    *p++ = RL(9, 1, 12);
    *p++ = RL(9, 1, 11);
    *p++ = RL(9, 2, 3);
    *p++ = RL(9, 3, 1);
    *p++ = RL(9, 5, 0);
    *p++ = RL(9, 1, 10);
    for (i = 0; i < 8; i++) *p++ = RL(6, 3, 0);
    for (i = 0; i < 8; i++) *p++ = RL(6, 1, 4);
    for (i = 0; i < 8; i++) *p++ = RL(6, 1, 3);
    for (i = 0; i < 16; i++) *p++ = RL(5, 2, 0);
    for (i = 0; i < 16; i++) *p++ = RL(5, 1, 2);
    for (i = 0; i < 32; i++) *p++ = RL(4, 1, 1);
}
