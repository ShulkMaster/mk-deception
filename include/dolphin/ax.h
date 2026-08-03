#ifndef DOLPHIN_AX_H
#define DOLPHIN_AX_H

#include "dolphin/sp.h"

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

typedef struct AXVoiceSrc {
    unsigned short ratio_hi;
    unsigned short ratio_lo;
    unsigned short current_fraction;
    short last_samples[4];
} AXVoiceSrc;

typedef void (*AXVoiceCallback)(void* callback_data);

#ifdef __cplusplus
extern "C" {
#endif

_AXVPB* AXAcquireVoice(unsigned long priority, AXVoiceCallback callback,
                       void* callback_data);
void AXFreeVoice(_AXVPB* voice);
void AXSetVoicePriority(_AXVPB* voice, unsigned long priority);
void AXSetVoiceSrc(_AXVPB* voice, AXVoiceSrc* source);
void AXSetVoiceAdpcm(_AXVPB* voice, SPADPCM* adpcm);
void AXSetVoiceState(_AXVPB* voice, unsigned short state);
void AXInitEx(int mode);
void AXSetMode(int mode);
void AXRegisterCallback(void (*callback)(void));

void MIXInit(void);
int MIXGetSoundMode(void);
void MIXUpdateSettings(void);
void MIXInitChannel(_AXVPB* voice, int mode, int aux_a, int aux_b, int aux_c,
                    unsigned char pan, unsigned char surround_pan,
                    unsigned long fader);
void MIXSetPan(_AXVPB* voice, unsigned char pan);
void MIXSetSPan(_AXVPB* voice, unsigned char pan);
void MIXSetFader(_AXVPB* voice, long volume);
void MIXReleaseChannel(_AXVPB* voice);

#ifdef __cplusplus
}
#endif

#endif
