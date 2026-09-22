#include "dolphin/ax.h"
#include "dolphin/os.h"

typedef struct MIXChannel {
    AXVPB* voice;
    u32 mode;
    s32 input;
    s32 aux_a;
    s32 aux_b;
    s32 pan;
    s32 surround_pan;
    s32 fader;
    s32 volumes[6];
    u16 v;
    u16 v1;
    u16 vL;
    u16 vL1;
    u16 vR;
    u16 vR1;
    u16 vS;
    u16 vS1;
    u16 vAL;
    u16 vAL1;
    u16 vAR;
    u16 vAR1;
    u16 vAS;
    u16 vAS1;
    u16 vBL;
    u16 vBL1;
    u16 vBR;
    u16 vBR1;
    u16 vBS;
    u16 vBS1;
} MIXChannel;

static MIXChannel __MIXChannel[64];
static s32 __MIXDvdStreamAttenCurrent;
static s32 __MIXDvdStreamAttenUser;
static u32 __MIXSoundMode;

extern const u32 __MIXPanTable[128];
extern const u16 __MIXVolumeTable[965];
extern const s16 __MIX_DPL2_front[128];
extern const s16 __MIX_DPL2_rear[128];

u8 __MIXAIVolumeTable[50] = {
    0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02,
    0x02, 0x02, 0x02, 0x03, 0x03, 0x04, 0x04, 0x05,
    0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C,
    0x0E, 0x10, 0x12, 0x14, 0x16, 0x19, 0x1C, 0x20,
    0x24, 0x28, 0x2D, 0x32, 0x39, 0x40, 0x47, 0x50,
    0x5A, 0x65, 0x71, 0x7F, 0x8F, 0xA0, 0xB4, 0xCA,
    0xE3, 0xFF
};

static inline u16 __MIXGetVolume(s32 attenuation)
{
    if (attenuation <= -904) {
        return 0;
    }
    if (attenuation >= 60) {
        return 0xFF64;
    }
    return __MIXVolumeTable[attenuation + 904];
}

/* TODO: [breakthrough needed] 69.148930%; retail folds contiguous mixer-table base offsets; retain separate extern tables pending data ownership recovery. */
static void __MIXSetPan(MIXChannel* channel)
{
    u32 pan;
    u32 surround_pan;
    u32 inverse_pan;
    u32 inverse_surround_pan;

    pan = channel->pan;
    surround_pan = channel->surround_pan;
    inverse_pan = 127 - pan;
    inverse_surround_pan = 127 - surround_pan;
    if (__MIXSoundMode == 3) {
        channel->volumes[0] = __MIX_DPL2_front[pan];
        channel->volumes[1] = __MIX_DPL2_front[inverse_pan];
        channel->volumes[2] = __MIX_DPL2_front[inverse_surround_pan];
        channel->volumes[3] = __MIX_DPL2_front[surround_pan];
        channel->volumes[4] = __MIX_DPL2_rear[inverse_pan];
        channel->volumes[5] = __MIX_DPL2_rear[pan];
        return;
    }
    channel->volumes[0] = __MIXPanTable[pan];
    channel->volumes[1] = __MIXPanTable[inverse_pan];
    channel->volumes[2] = __MIXPanTable[inverse_surround_pan];
    channel->volumes[3] = __MIXPanTable[surround_pan];
}

/* TODO: [near miss] 96.800000%; retail initializes the loop pointer directly in r29; honest assignment forms retain the extra r0 copy, while a combined initializer crashes MWCC. */
void MIXInit(void)
{
    MIXChannel* channel = __MIXChannel;
    s32 i;

    for (i = 0; i < 64; i++, channel++) {
        channel->mode = 0x50000000;
        channel->input = 0;
        channel->aux_a = -0x3C0;
        channel->aux_b = -0x3C0;
        channel->fader = 0;
        channel->pan = 0x40;
        channel->surround_pan = 0x7F;
        channel->vBS = 0;
        channel->vBR = 0;
        channel->vBL = 0;
        channel->vAS = 0;
        channel->vAR = 0;
        channel->vAL = 0;
        channel->vS = 0;
        channel->vR = 0;
        channel->vL = 0;
        channel->v = 0;
        __MIXSetPan(channel);
    }
    __MIXDvdStreamAttenCurrent = 0;
    __MIXDvdStreamAttenUser = 0;
    __MIXSoundMode = 1;
}

int MIXGetSoundMode(void)
{
    return __MIXSoundMode;
}

/* TODO: [breakthrough needed] 88.296650%; the 0x60-byte channel layout and
 * all mode paths agree; AX scheduling and the update path remain incomplete. */
void MIXInitChannel(AXVPB* voice, u32 mode, int input, int aux_a, int aux_b,
                    int pan, int surround_pan, int fader)
{
    MIXChannel* channel;
    BOOL enabled;
    u16 mixer_control;
    u16* mix;

    channel = &__MIXChannel[voice->index];
    channel->voice = voice;
    channel->mode = mode & 7;
    channel->input = input;
    channel->aux_a = aux_a;
    channel->aux_b = aux_b;
    channel->pan = pan;
    channel->surround_pan = surround_pan;
    channel->fader = fader;
    __MIXSetPan(channel);

    if (channel->mode & 4) {
        channel->v = 0;
    } else {
        channel->v = __MIXGetVolume(input);
    }

    mixer_control = 0;
    switch (__MIXSoundMode) {
    case 0:
        channel->vL = __MIXGetVolume(channel->fader + channel->volumes[2]);
        channel->vR = __MIXGetVolume(channel->fader + channel->volumes[2]);
        channel->vS = __MIXGetVolume(channel->fader + channel->volumes[3] - 30);
        if (channel->mode & 1) {
            channel->vAL = __MIXGetVolume(channel->aux_a + channel->volumes[2]);
            channel->vAR = __MIXGetVolume(channel->aux_a + channel->volumes[2]);
            channel->vAS = __MIXGetVolume(channel->aux_a + channel->volumes[3] - 30);
        } else {
            channel->vAL = __MIXGetVolume(channel->fader + channel->aux_a + channel->volumes[2]);
            channel->vAR = __MIXGetVolume(channel->fader + channel->aux_a + channel->volumes[2]);
            channel->vAS = __MIXGetVolume(channel->fader + channel->aux_a + channel->volumes[3] - 30);
        }
        if (channel->mode & 2) {
            channel->vBL = __MIXGetVolume(channel->aux_b + channel->volumes[2]);
            channel->vBR = __MIXGetVolume(channel->aux_b + channel->volumes[2]);
            channel->vBS = __MIXGetVolume(channel->aux_b + channel->volumes[3] - 30);
        } else {
            channel->vBL = __MIXGetVolume(channel->fader + channel->aux_b + channel->volumes[2]);
            channel->vBR = __MIXGetVolume(channel->fader + channel->aux_b + channel->volumes[2]);
            channel->vBS = __MIXGetVolume(channel->fader + channel->aux_b + channel->volumes[3] - 30);
        }
        break;
    case 1:
    case 2:
        channel->vL = __MIXGetVolume(channel->fader + channel->volumes[0] + channel->volumes[2]);
        channel->vR = __MIXGetVolume(channel->fader + channel->volumes[1] + channel->volumes[2]);
        channel->vS = __MIXGetVolume(channel->fader + channel->volumes[3] - 30);
        if (channel->mode & 1) {
            channel->vAL = __MIXGetVolume(channel->aux_a + channel->volumes[0] + channel->volumes[2]);
            channel->vAR = __MIXGetVolume(channel->aux_a + channel->volumes[1] + channel->volumes[2]);
            channel->vAS = __MIXGetVolume(channel->aux_a + channel->volumes[3] - 30);
        } else {
            channel->vAL = __MIXGetVolume(channel->fader + channel->aux_a + channel->volumes[0] + channel->volumes[2]);
            channel->vAR = __MIXGetVolume(channel->fader + channel->aux_a + channel->volumes[1] + channel->volumes[2]);
            channel->vAS = __MIXGetVolume(channel->fader + channel->aux_a + channel->volumes[3] - 30);
        }
        if (channel->mode & 2) {
            channel->vBL = __MIXGetVolume(channel->aux_b + channel->volumes[0] + channel->volumes[2]);
            channel->vBR = __MIXGetVolume(channel->aux_b + channel->volumes[1] + channel->volumes[2]);
            channel->vBS = __MIXGetVolume(channel->aux_b + channel->volumes[3] - 30);
        } else {
            channel->vBL = __MIXGetVolume(channel->fader + channel->aux_b + channel->volumes[0] + channel->volumes[2]);
            channel->vBR = __MIXGetVolume(channel->fader + channel->aux_b + channel->volumes[1] + channel->volumes[2]);
            channel->vBS = __MIXGetVolume(channel->fader + channel->aux_b + channel->volumes[3] - 30);
        }
        break;
    case 3:
        channel->vL = __MIXGetVolume(channel->fader + channel->volumes[0] + channel->volumes[2]);
        channel->vR = __MIXGetVolume(channel->fader + channel->volumes[1] + channel->volumes[2]);
        channel->vBL = __MIXGetVolume(channel->fader + channel->volumes[4] + channel->volumes[3]);
        channel->vBR = __MIXGetVolume(channel->fader + channel->volumes[5] + channel->volumes[3]);
        if (channel->mode & 1) {
            channel->vAL = __MIXGetVolume(channel->aux_a + channel->volumes[0] + channel->volumes[2]);
            channel->vAR = __MIXGetVolume(channel->aux_a + channel->volumes[1] + channel->volumes[2]);
            channel->vAS = __MIXGetVolume(channel->aux_a + channel->volumes[4] + channel->volumes[3]);
            channel->vBS = __MIXGetVolume(channel->aux_a + channel->volumes[5] + channel->volumes[3]);
        } else {
            channel->vAL = __MIXGetVolume(channel->fader + channel->aux_a + channel->volumes[0] + channel->volumes[2]);
            channel->vAR = __MIXGetVolume(channel->fader + channel->aux_a + channel->volumes[1] + channel->volumes[2]);
            channel->vAS = __MIXGetVolume(channel->fader + channel->aux_a + channel->volumes[4] + channel->volumes[3]);
            channel->vBS = __MIXGetVolume(channel->fader + channel->aux_a + channel->volumes[5] + channel->volumes[3]);
        }
        mixer_control |= 0x4000;
        break;
    }

    enabled = OSDisableInterrupts();
    voice->pb.ve.currentVolume = channel->v;
    voice->pb.ve.currentDelta = 0;
    mix = (u16*)&voice->pb.mix;
    if ((*mix++ = channel->vL) != 0) mixer_control |= 1;
    *mix++ = 0;
    if ((*mix++ = channel->vR) != 0) mixer_control |= 2;
    *mix++ = 0;
    if ((*mix++ = channel->vAL) != 0) mixer_control |= 0x10;
    *mix++ = 0;
    if ((*mix++ = channel->vAR) != 0) mixer_control |= 0x20;
    *mix++ = 0;
    if ((*mix++ = channel->vBL) != 0) mixer_control |= 0x200;
    *mix++ = 0;
    if ((*mix++ = channel->vBR) != 0) mixer_control |= 0x400;
    *mix++ = 0;
    if ((*mix++ = channel->vBS) != 0) mixer_control |= 0x1000;
    *mix++ = 0;
    if ((*mix++ = channel->vS) != 0) mixer_control |= 4;
    *mix++ = 0;
    if ((*mix++ = channel->vAS) != 0) mixer_control |= 0x80;
    *mix++ = 0;
    voice->pb.mixerCtrl = mixer_control;
    voice->sync |= AX_SYNC_FLAG_COPYMXRCTRL | AX_SYNC_FLAG_COPYAXPBMIX | AX_SYNC_FLAG_COPYVOL;
    OSRestoreInterrupts(enabled);
}

void MIXReleaseChannel(AXVPB* voice)
{
    __MIXChannel[voice->index].voice = 0;
}

void MIXSetInput(AXVPB* voice, long input)
{
    MIXChannel* channel = &__MIXChannel[voice->index];

    channel->input = input;
    channel->mode |= 0x10000000;
}

void MIXSetPan(AXVPB* voice, int pan)
{
    MIXChannel* channel = &__MIXChannel[voice->index];

    if (pan < 0) {
        pan = 0;
    } else if (pan > 127) {
        pan = 127;
    }
    channel->pan = pan;
    __MIXSetPan(channel);
    channel->mode |= 0x40000000;
}

void MIXSetSPan(AXVPB* voice, int pan)
{
    MIXChannel* channel = &__MIXChannel[voice->index];

    if (pan < 0) {
        pan = 0;
    } else if (pan > 127) {
        pan = 127;
    }
    channel->surround_pan = pan;
    __MIXSetPan(channel);
    channel->mode |= 0x40000000;
}

void MIXSetFader(AXVPB* voice, int volume)
{
    MIXChannel* channel = &__MIXChannel[voice->index];

    channel->fader = volume;
    channel->mode |= 0x40000000;
}

/* TODO: [breakthrough] 87.814220%; donor update state machine, AX publication,
 * and global volume-table ownership are recovered; 8-byte body residue remains. */
void MIXUpdateSettings(void)
{
    int i;
    int set_new_mix_level;
    int set_new_input_level;
    MIXChannel* channel;
    AXVPB* voice;
    u16 mixer_control;
    u16* mix;

    for (i = 0; i < AX_MAX_VOICES; i++) {
        set_new_input_level = 0;
        set_new_mix_level = 0;
        channel = &__MIXChannel[i];
        voice = channel->voice;
        if (voice != 0) {
            mixer_control = 0;
            if (channel->mode & 0x20000000) {
                channel->v = channel->v1;
                channel->mode &= ~0x20000000;
                set_new_input_level = 1;
            }
            if (channel->mode & 0x10000000) {
                if (channel->mode & 4) {
                    channel->v1 = 0;
                } else {
                    channel->v1 = __MIXGetVolume(channel->input);
                }
                channel->mode &= ~0x10000000;
                channel->mode |= 0x20000000;
                set_new_input_level = 1;
            }
            if (channel->mode & 0x80000000) {
                channel->vL = channel->vL1;
                channel->vR = channel->vR1;
                channel->vS = channel->vS1;
                channel->vAL = channel->vAL1;
                channel->vAR = channel->vAR1;
                channel->vAS = channel->vAS1;
                channel->vBL = channel->vBL1;
                channel->vBR = channel->vBR1;
                channel->vBS = channel->vBS1;
                channel->mode &= ~0x80000000;
                set_new_mix_level = 1;
            }
            if (channel->mode & 0x40000000) {
                switch (__MIXSoundMode) {
                case 0:
                    channel->vL1 = __MIXGetVolume(channel->fader + channel->volumes[2]);
                    channel->vR1 = __MIXGetVolume(channel->fader + channel->volumes[2]);
                    channel->vS1 = __MIXGetVolume(channel->fader + channel->volumes[3] - 30);
                    if (channel->mode & 1) {
                        channel->vAL1 = __MIXGetVolume(channel->aux_a + channel->volumes[2]);
                        channel->vAR1 = __MIXGetVolume(channel->aux_a + channel->volumes[2]);
                        channel->vAS1 = __MIXGetVolume(channel->aux_a + channel->volumes[3] - 30);
                    } else {
                        channel->vAL1 = __MIXGetVolume(channel->fader + channel->aux_a + channel->volumes[2]);
                        channel->vAR1 = __MIXGetVolume(channel->fader + channel->aux_a + channel->volumes[2]);
                        channel->vAS1 = __MIXGetVolume(channel->fader + channel->aux_a + channel->volumes[3] - 30);
                    }
                    if (channel->mode & 2) {
                        channel->vBL1 = __MIXGetVolume(channel->aux_b + channel->volumes[2]);
                        channel->vBR1 = __MIXGetVolume(channel->aux_b + channel->volumes[2]);
                        channel->vBS1 = __MIXGetVolume(channel->aux_b + channel->volumes[3] - 30);
                    } else {
                        channel->vBL1 = __MIXGetVolume(channel->fader + channel->aux_b + channel->volumes[2]);
                        channel->vBR1 = __MIXGetVolume(channel->fader + channel->aux_b + channel->volumes[2]);
                        channel->vBS1 = __MIXGetVolume(channel->fader + channel->aux_b + channel->volumes[3] - 30);
                    }
                    break;
                case 1:
                case 2:
                    channel->vL1 = __MIXGetVolume(channel->fader + channel->volumes[0] + channel->volumes[2]);
                    channel->vR1 = __MIXGetVolume(channel->fader + channel->volumes[1] + channel->volumes[2]);
                    channel->vS1 = __MIXGetVolume(channel->fader + channel->volumes[3] - 30);
                    if (channel->mode & 1) {
                        channel->vAL1 = __MIXGetVolume(channel->aux_a + channel->volumes[0] + channel->volumes[2]);
                        channel->vAR1 = __MIXGetVolume(channel->aux_a + channel->volumes[1] + channel->volumes[2]);
                        channel->vAS1 = __MIXGetVolume(channel->aux_a + channel->volumes[3] - 30);
                    } else {
                        channel->vAL1 = __MIXGetVolume(channel->fader + channel->aux_a + channel->volumes[0] + channel->volumes[2]);
                        channel->vAR1 = __MIXGetVolume(channel->fader + channel->aux_a + channel->volumes[1] + channel->volumes[2]);
                        channel->vAS1 = __MIXGetVolume(channel->fader + channel->aux_a + channel->volumes[3] - 30);
                    }
                    if (channel->mode & 2) {
                        channel->vBL1 = __MIXGetVolume(channel->aux_b + channel->volumes[0] + channel->volumes[2]);
                        channel->vBR1 = __MIXGetVolume(channel->aux_b + channel->volumes[1] + channel->volumes[2]);
                        channel->vBS1 = __MIXGetVolume(channel->aux_b + channel->volumes[3] - 30);
                    } else {
                        channel->vBL1 = __MIXGetVolume(channel->fader + channel->aux_b + channel->volumes[0] + channel->volumes[2]);
                        channel->vBR1 = __MIXGetVolume(channel->fader + channel->aux_b + channel->volumes[1] + channel->volumes[2]);
                        channel->vBS1 = __MIXGetVolume(channel->fader + channel->aux_b + channel->volumes[3] - 30);
                    }
                    break;
                case 3:
                    channel->vL1 = __MIXGetVolume(channel->fader + channel->volumes[0] + channel->volumes[2]);
                    channel->vR1 = __MIXGetVolume(channel->fader + channel->volumes[1] + channel->volumes[2]);
                    channel->vBL1 = __MIXGetVolume(channel->fader + channel->volumes[4] + channel->volumes[3]);
                    channel->vBR1 = __MIXGetVolume(channel->fader + channel->volumes[5] + channel->volumes[3]);
                    if (channel->mode & 1) {
                        channel->vAL1 = __MIXGetVolume(channel->aux_a + channel->volumes[0] + channel->volumes[2]);
                        channel->vAR1 = __MIXGetVolume(channel->aux_a + channel->volumes[1] + channel->volumes[2]);
                        channel->vAS1 = __MIXGetVolume(channel->aux_a + channel->volumes[4] + channel->volumes[3]);
                        channel->vBS1 = __MIXGetVolume(channel->aux_a + channel->volumes[5] + channel->volumes[3]);
                    } else {
                        channel->vAL1 = __MIXGetVolume(channel->fader + channel->aux_a + channel->volumes[0] + channel->volumes[2]);
                        channel->vAR1 = __MIXGetVolume(channel->fader + channel->aux_a + channel->volumes[1] + channel->volumes[2]);
                        channel->vAS1 = __MIXGetVolume(channel->fader + channel->aux_a + channel->volumes[4] + channel->volumes[3]);
                        channel->vBS1 = __MIXGetVolume(channel->fader + channel->aux_a + channel->volumes[5] + channel->volumes[3]);
                    }
                    mixer_control |= 0x4000;
                    break;
                }
                channel->mode &= ~0x40000000;
                channel->mode |= 0x80000000;
                set_new_mix_level = 1;
            }
            if (set_new_input_level) {
                voice->pb.ve.currentVolume = channel->v;
                voice->pb.ve.currentDelta = (s16)((channel->v1 - channel->v) / 160);
                voice->sync |= 0x200;
            }
            if (set_new_mix_level) {
                mix = (u16*)&voice->pb.mix;
                if ((*mix++ = channel->vL)) mixer_control |= 1;
                if ((*mix++ = (u16)((channel->vL1 - channel->vL) / 160))) mixer_control |= 8;
                if ((*mix++ = channel->vR)) mixer_control |= 2;
                if ((*mix++ = (u16)((channel->vR1 - channel->vR) / 160))) mixer_control |= 8;
                if ((*mix++ = channel->vAL)) mixer_control |= 0x10;
                if ((*mix++ = (u16)((channel->vAL1 - channel->vAL) / 160))) mixer_control |= 0x40;
                if ((*mix++ = channel->vAR)) mixer_control |= 0x20;
                if ((*mix++ = (u16)((channel->vAR1 - channel->vAR) / 160))) mixer_control |= 0x40;
                if ((*mix++ = channel->vBL)) mixer_control |= 0x200;
                if ((*mix++ = (u16)((channel->vBL1 - channel->vBL) / 160))) mixer_control |= 0x800;
                if ((*mix++ = channel->vBR)) mixer_control |= 0x400;
                if ((*mix++ = (u16)((channel->vBR1 - channel->vBR) / 160))) mixer_control |= 0x800;
                if ((*mix++ = channel->vBS)) mixer_control |= 0x1000;
                if ((*mix++ = (u16)((channel->vBS1 - channel->vBS) / 160))) mixer_control |= 0x2000;
                if ((*mix++ = channel->vS)) mixer_control |= 4;
                if ((*mix++ = (u16)((channel->vS1 - channel->vS) / 160))) mixer_control |= 8;
                if ((*mix++ = channel->vAS)) mixer_control |= 0x80;
                if ((*mix++ = (u16)((channel->vAS1 - channel->vAS) / 160))) mixer_control |= 0x100;
                voice->pb.mixerCtrl = mixer_control;
                voice->sync |= 0x12;
            }
        }
    }
    if (__MIXDvdStreamAttenUser > __MIXDvdStreamAttenCurrent) {
        __MIXDvdStreamAttenCurrent++;
        AISetStreamVolLeft(__MIXAIVolumeTable[__MIXDvdStreamAttenCurrent]);
        AISetStreamVolRight(__MIXAIVolumeTable[__MIXDvdStreamAttenCurrent]);
    } else if (__MIXDvdStreamAttenUser < __MIXDvdStreamAttenCurrent) {
        __MIXDvdStreamAttenCurrent--;
        AISetStreamVolLeft(__MIXAIVolumeTable[__MIXDvdStreamAttenCurrent]);
        AISetStreamVolRight(__MIXAIVolumeTable[__MIXDvdStreamAttenCurrent]);
    }
}
