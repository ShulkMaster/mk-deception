#ifndef DOLPHIN_AX_H
#define DOLPHIN_AX_H

/*
 * Game-used AX voice parameter block fields. The opaque regions are owned by
 * the Dolphin AX library; only fields accessed by the MSL playback layer are
 * exposed here.
 */
struct _AXVPB {
    unsigned char pad00[0x1C];
    unsigned long sync;           /* +0x01C */
    unsigned char pad20[0x126];
    unsigned short state;         /* +0x146 -- bit 0 is active */
    unsigned short depop;         /* +0x148 */
    unsigned char pad14A[0x5C];
    unsigned short loop;          /* +0x1A6 */
    unsigned short format;        /* +0x1A8 */
    unsigned short current_hi;    /* +0x1AA */
    unsigned short current_lo;    /* +0x1AC */
    unsigned short end_hi;        /* +0x1AE */
    unsigned short end_lo;        /* +0x1B0 */
    unsigned short loop_hi;       /* +0x1B2 */
    unsigned short loop_lo;       /* +0x1B4 */
    unsigned char pad1B6[0x28];
    unsigned short ratio_hi;      /* +0x1DE */
    unsigned short ratio_lo;      /* +0x1E0 */
    unsigned char pad1E2[0x0A];
    unsigned short adpcm_loop_pred_scale; /* +0x1EC */
    unsigned short adpcm_loop_yn1;        /* +0x1EE */
    unsigned short adpcm_loop_yn2;        /* +0x1F0 */
};

typedef char AXVPBLayoutSize[
    sizeof(_AXVPB) == 0x1F4 ? 1 : -1];

#endif
