#ifndef DOLPHIN_AX_H
#define DOLPHIN_AX_H

#include "dolphin/types.h"

typedef struct AXPBMIX {
    unsigned short vL, vDeltaL, vR, vDeltaR;
    unsigned short vAuxAL, vDeltaAuxAL, vAuxAR, vDeltaAuxAR;
    unsigned short vAuxBL, vDeltaAuxBL, vAuxBR, vDeltaAuxBR;
    unsigned short vAuxBS, vDeltaAuxBS, vS, vDeltaS;
    unsigned short vAuxAS, vDeltaAuxAS;
} AXPBMIX;

typedef struct AXPBITD {
    unsigned short flag, bufferHi, bufferLo, shiftL, shiftR;
    unsigned short targetShiftL, targetShiftR;
} AXPBITD;

typedef struct AXPBUPDATE {
    unsigned short updNum[5];
    unsigned short dataHi, dataLo;
} AXPBUPDATE;

typedef struct AXPBDPOP {
    short aL, aAuxAL, aAuxBL, aR, aAuxAR, aAuxBR, aS, aAuxAS, aAuxBS;
} AXPBDPOP;

typedef struct AXPBVE {
    unsigned short currentVolume;
    short currentDelta;
} AXPBVE;

typedef struct AXPBFIR {
    unsigned short numCoefs, coefsHi, coefsLo;
} AXPBFIR;

typedef struct AXPBADDR {
    unsigned short loopFlag, format;
    unsigned short loopAddressHi, loopAddressLo;
    unsigned short endAddressHi, endAddressLo;
    unsigned short currentAddressHi, currentAddressLo;
} AXPBADDR;

typedef struct AXPBADPCM {
    unsigned short a[8][2];
    unsigned short gain, pred_scale, yn1, yn2;
} AXPBADPCM;

typedef struct AXPBSRC {
    unsigned short ratioHi, ratioLo, currentAddressFrac;
    unsigned short last_samples[4];
} AXPBSRC;

typedef struct AXPBADPCMLOOP {
    unsigned short loop_pred_scale, loop_yn1, loop_yn2;
} AXPBADPCMLOOP;

typedef struct AXPBLPF {
    unsigned short on, yn1, a0, b0;
} AXPBLPF;

typedef struct AXPB {
    unsigned short nextHi, nextLo, currHi, currLo;
    unsigned short srcSelect, coefSelect, mixerCtrl, state, type;
    AXPBMIX mix;
    AXPBITD itd;
    AXPBUPDATE update;
    AXPBDPOP dpop;
    AXPBVE ve;
    AXPBFIR fir;
    AXPBADDR addr;
    AXPBADPCM adpcm;
    AXPBSRC src;
    AXPBADPCMLOOP adpcmLoop;
    AXPBLPF lpf;
    /* Reserved DSP parameter-block words, confirmed by the retail 0xF4 size. */
    unsigned short pad[25];
} AXPB;

typedef struct _AXVPB {
    struct _AXVPB* next;
    struct _AXVPB* prev;
    struct _AXVPB* next1;
    unsigned long priority;
    void (*callback)(void*);
    unsigned long user_context;
    unsigned long index, sync, depop;
    unsigned long updateMS, updateCounter, updateTotal;
    unsigned short* updateWrite;
    unsigned short updateData[128];
    void* itdBuffer;
    AXPB pb;
} AXVPB;

typedef struct AXPBITDBUFFER { short data[32]; } AXPBITDBUFFER;
typedef struct AXPBU { unsigned short data[128]; } AXPBU;

typedef struct AXSPB {
    unsigned short dpopLHi, dpopLLo; short dpopLDelta;
    unsigned short dpopRHi, dpopRLo; short dpopRDelta;
    unsigned short dpopSHi, dpopSLo; short dpopSDelta;
    unsigned short dpopALHi, dpopALLo; short dpopALDelta;
    unsigned short dpopARHi, dpopARLo; short dpopARDelta;
    unsigned short dpopASHi, dpopASLo; short dpopASDelta;
    unsigned short dpopBLHi, dpopBLLo; short dpopBLDelta;
    unsigned short dpopBRHi, dpopBRLo; short dpopBRDelta;
    unsigned short dpopBSHi, dpopBSLo; short dpopBSDelta;
} AXSPB;

typedef char AXVPBLayoutSize[sizeof(AXVPB) == 0x22C ? 1 : -1];
typedef char AXPBLayoutSize[sizeof(AXPB) == 0xF4 ? 1 : -1];

typedef AXPBSRC AXVoiceSrc;

typedef struct _AXPROFILE {
    unsigned long long axFrameStart, auxProcessingStart, auxProcessingEnd;
    unsigned long long userCallbackStart, userCallbackEnd, axFrameEnd;
    unsigned long axNumVoices;
} AXPROFILE;

typedef struct AX_AUX_DATA { signed long* l; signed long* r; signed long* s; } AX_AUX_DATA;
typedef struct AX_AUX_DATA_DPL2 { signed long* l; signed long* r; signed long* ls; signed long* rs; } AX_AUX_DATA_DPL2;
typedef char AXPROFILELayoutSize[sizeof(AXPROFILE) == 0x38 ? 1 : -1];

typedef void (*AXVoiceCallback)(void* callback_data);
typedef void (*AXCallback)(void);

#define AX_MAX_VOICES 64
#define AX_PRIORITY_STACKS 32
#define AX_DSP_SLAVE_LENGTH 0xF80
#define AX_SRC_TYPE_NONE 0
#define AX_SRC_TYPE_LINEAR 1
#define AX_SRC_TYPE_4TAP_8K 2
#define AX_SRC_TYPE_4TAP_12K 3
#define AX_SRC_TYPE_4TAP_16K 4
#define AX_SYNC_FLAG_COPYALL       (1UL << 31)
#define AX_SYNC_FLAG_COPYADPCMLOOP (1UL << 20)
#define AX_SYNC_FLAG_COPYRATIO     (1UL << 19)
#define AX_SYNC_FLAG_COPYSRC       (1UL << 18)
#define AX_SYNC_FLAG_COPYADPCM     (1UL << 17)
#define AX_SYNC_FLAG_COPYCURADDR   (1UL << 16)
#define AX_SYNC_FLAG_COPYENDADDR   (1UL << 15)
#define AX_SYNC_FLAG_COPYLOOPADDR  (1UL << 14)
#define AX_SYNC_FLAG_COPYLOOP      (1UL << 13)
#define AX_SYNC_FLAG_COPYADDR      (1UL << 12)
#define AX_SYNC_FLAG_COPYFIR       (1UL << 11)
#define AX_SYNC_FLAG_SWAPVOL       (1UL << 10)
#define AX_SYNC_FLAG_COPYVOL       (1UL << 9)
#define AX_SYNC_FLAG_COPYDPOP      (1UL << 8)
#define AX_SYNC_FLAG_COPYUPDATE    (1UL << 7)
#define AX_SYNC_FLAG_COPYTSHIFT    (1UL << 6)
#define AX_SYNC_FLAG_COPYITD       (1UL << 5)
#define AX_SYNC_FLAG_COPYAXPBMIX   (1UL << 4)
#define AX_SYNC_FLAG_COPYTYPE      (1UL << 3)
#define AX_SYNC_FLAG_COPYSTATE     (1UL << 2)
#define AX_SYNC_FLAG_COPYMXRCTRL   (1UL << 1)
#define AX_SYNC_FLAG_COPYSELECT    (1UL << 0)

#ifdef __cplusplus
extern "C" {
#endif

AXVPB* AXAcquireVoice(unsigned long priority, AXVoiceCallback callback,
                      unsigned long user_context);
void AXFreeVoice(AXVPB* voice);
void AXSetVoicePriority(AXVPB* voice, unsigned long priority);
void AXSetVoiceSrcType(AXVPB* voice, unsigned long type);
void AXSetVoiceSrc(AXVPB* voice, AXPBSRC* source);
void AXSetVoiceAdpcm(AXVPB* voice, AXPBADPCM* adpcm);
void AXSetVoiceState(AXVPB* voice, unsigned short state);
void AXSetVoiceAddr(AXVPB* voice, AXPBADDR* address);
void AXInitEx(unsigned long output_buffer_mode);
void AXSetMode(unsigned long mode);
AXCallback AXRegisterCallback(AXCallback callback);

void MIXInit(void);
int MIXGetSoundMode(void);
void MIXUpdateSettings(void);
void MIXInitChannel(AXVPB* voice, unsigned long mode, int input, int aux_a,
                    int aux_b, int pan, int surround_pan, int fader);
void MIXSetPan(AXVPB* voice, int pan);
void MIXSetSPan(AXVPB* voice, int pan);
void MIXSetInput(AXVPB* voice, long volume);
void MIXSetFader(AXVPB* voice, int volume);
void MIXReleaseChannel(AXVPB* voice);

#ifdef __cplusplus
}
#endif

#endif
