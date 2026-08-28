#include "dolphin/types.h"

typedef struct MPVBDECVlcDescriptor {
    const s16* table;
    s32 code_length;
} MPVBDECVlcDescriptor;

typedef struct MPVBDECContextView {
    u8 field_0x0000[0x1100];
    s16 dc_sign_masks[16];
    s8 scan[64];
    f32 coefficient_scale[64];
    u8 vlc_group[32];
    MPVBDECVlcDescriptor vlc_desc[6];
} MPVBDECContextView;

static const s16 mpvbdec_bitmsk[16] = {
    -1, 0x7fff, 0x3fff, 0x1fff, 0x0fff, 0x07ff, 0x03ff, 0x01ff,
    0x00ff, 0x007f, 0x003f, 0x001f, 0x000f, 0x0007, 0x0003, 0x0001,
};

static const s8 zigzag2seq[64] = {
    0, 1, 8, 16, 9, 2, 3, 10, 17, 24, 32, 25, 18, 11, 4, 5,
    12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13, 6, 7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63,
};

static const u8 org_iqm[64] = {
    8, 16, 19, 22, 26, 27, 29, 34, 16, 16, 22, 24, 27, 29, 34, 37,
    19, 22, 26, 27, 29, 34, 34, 38, 22, 22, 26, 27, 29, 34, 37, 40,
    22, 26, 27, 29, 32, 35, 40, 48, 26, 27, 29, 32, 35, 40, 48, 58,
    26, 27, 29, 34, 38, 46, 56, 69, 27, 29, 35, 38, 46, 56, 69, 83,
};

static const u8 group_tbl[32] = {
    5, 4, 3, 3, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

extern u8 mpvbdec_dfl_iqm[64];
extern u8 mpvbdec_zigzag[64];

extern s16* mpvvlc_run_level_4;
extern s16* mpvvlc_run_level_2;
extern s16* mpvvlc_run_level_1;
extern s16* mpvvlc_run_level_0a;
extern s16* mpvvlc_run_level_0b;
extern s16* mpvvlc_run_level_0c;

extern void DCT_FsriInitScanTbl(const s8 source[64], s8 destination[64]);
extern void UTY_MemcpyDword(u32* destination, const u32* source, u32 count);
extern void* memcpy(void* destination, const void* source, unsigned long size);

void MPVBDEC_Init(MPVBDECContextView* context)
{
    s8 linear[64];
    s8 scan[64];
    s8* output_scan;
    s16* output_masks;
    s32 i;

    for (i = 0; i < 64; i++) {
        linear[i] = (s8)i;
    }
    DCT_FsriInitScanTbl(linear, scan);

    for (i = 0; i < 64; i++) {
        mpvbdec_zigzag[i] = scan[zigzag2seq[i]];
        mpvbdec_dfl_iqm[scan[i]] = org_iqm[i];
    }

    output_scan = context->scan;
    if (output_scan != 0) {
        UTY_MemcpyDword((u32*)output_scan, (const u32*)mpvbdec_zigzag, 16);
    }
    output_masks = context->dc_sign_masks;
    if (output_masks != 0) {
        UTY_MemcpyDword((u32*)output_masks, (const u32*)mpvbdec_bitmsk, 8);
    }
    memcpy(context->vlc_group, group_tbl, sizeof(group_tbl));

    context->vlc_desc[0].table = mpvvlc_run_level_4 - 8;
    context->vlc_desc[0].code_length = 0x15;
    context->vlc_desc[1].table = mpvvlc_run_level_2 - 16;
    context->vlc_desc[1].code_length = 0x13;
    context->vlc_desc[2].table = mpvvlc_run_level_1 - 16;
    context->vlc_desc[2].code_length = 0x12;
    context->vlc_desc[3].table = mpvvlc_run_level_0a - 16;
    context->vlc_desc[3].code_length = 0x11;
    context->vlc_desc[4].table = mpvvlc_run_level_0b - 16;
    context->vlc_desc[4].code_length = 0x10;
    context->vlc_desc[5].table = mpvvlc_run_level_0c - 16;
    context->vlc_desc[5].code_length = 0x0f;
}

/* MWCC emits these trailing .bss definitions in reverse declaration order. */
u8 mpvbdec_zigzag[64];
u8 mpvbdec_dfl_iqm[64];
