#include "dolphin/ax.h"

typedef struct MIXUpdate {
    s16 value;
    s16 delta;
} MIXUpdate;

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
    MIXUpdate updates[10];
} MIXChannel;

static MIXChannel __MIXChannel[64];
static s32 __MIXDvdStreamAttenCurrent;
static s32 __MIXDvdStreamAttenUser;
static u32 __MIXSoundMode;

extern const u32 __MIXPanTable[128];
extern const u16 __MIXVolumeTable[965];
extern const s16 __MIX_DPL2_front[128];
extern const s16 __MIX_DPL2_rear[128];

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

void MIXInit(void)
{
    MIXChannel* channel;
    s32 i;

    channel = __MIXChannel;
    i = 0;
    do {
        channel->mode = 0x50000000;
        channel->input = 0;
        channel->aux_a = -0x3C0;
        channel->aux_b = -0x3C0;
        channel->fader = 0;
        channel->pan = 0x40;
        channel->surround_pan = 0x7F;
        channel->updates[9].value = 0;
        channel->updates[8].value = 0;
        channel->updates[7].value = 0;
        channel->updates[6].value = 0;
        channel->updates[5].value = 0;
        channel->updates[4].value = 0;
        channel->updates[3].value = 0;
        channel->updates[2].value = 0;
        channel->updates[1].value = 0;
        channel->updates[0].value = 0;
        __MIXSetPan(channel);
        i++;
        channel++;
    } while (i < 64);
    __MIXDvdStreamAttenCurrent = 0;
    __MIXDvdStreamAttenUser = 0;
    __MIXSoundMode = 1;
}

int MIXGetSoundMode(void)
{
    return __MIXSoundMode;
}

void MIXInitChannel(AXVPB* voice, u32 mode, int input, int aux_a, int aux_b,
                    int pan, int surround_pan, int fader)
{
    MIXChannel* channel = &__MIXChannel[voice->index];

    channel->voice = voice;
    channel->mode = mode & 7;
    channel->input = input;
    channel->aux_a = aux_a;
    channel->aux_b = aux_b;
    channel->pan = pan;
    channel->surround_pan = surround_pan;
    channel->fader = fader;
    __MIXSetPan(channel);

    if ((channel->mode & 4) != 0) {
        channel->updates[0].value = 0;
    } else {
        channel->updates[0].value = __MIXGetVolume(input);
    }

    if (__MIXSoundMode == 0) {
        channel->updates[1].value =
            __MIXGetVolume(fader + channel->volumes[2]);
        channel->updates[2].value =
            __MIXGetVolume(fader + channel->volumes[2]);
        channel->updates[3].value =
            __MIXGetVolume(fader + channel->volumes[3] - 30);
    }
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

void MIXUpdateSettings(void)
{
    MIXChannel* channel;
    u32 i;
    u32 j;

    channel = __MIXChannel;
    for (i = 0; i < 64; i++, channel++) {
        if (channel->voice == 0) {
            continue;
        }

        if ((channel->mode & 0x20000000) != 0) {
            channel->updates[0].value = channel->updates[0].delta;
            channel->mode &= ~0x20000000;
        }

        if ((channel->mode & 0x10000000) != 0) {
            if ((channel->mode & 4) != 0) {
                channel->updates[0].delta = 0;
            } else {
                channel->updates[0].delta =
                    __MIXGetVolume(channel->input);
            }
            channel->mode &= ~0x10000000;
            channel->mode |= 0x20000000;
        }

        if ((channel->mode & 0x80000000) != 0) {
            for (j = 0; j < 10; j++) {
                channel->updates[j].value = channel->updates[j].delta;
            }
            channel->mode &= ~0x80000000;
        }
    }
}
