#include "cri/sj.h"
#include "dolphin/arq.h"
#include "dolphin/ax.h"
#include "dolphin/cache.h"
#include "runtime/cstring.h"

#define AXRNA_MAX_HANDLES 16
#define AXRNA_MAX_CHANNELS 2
#define AXRNA_BUFFER_SAMPLES 0x1000
#define AXRNA_TRANSFER_BYTES 0x2000

typedef struct RNAResource RNAResource;
typedef void (*RNAErrorCallback)(void* object, const char* message);

typedef struct AXRNAHandle {
    signed char used;
    unsigned char switches;
    signed char allocated_channels;
    signed char num_channels;
    int play_position;
    AXVPB* voices[AXRNA_MAX_CHANNELS];
    RNAResource* resources[AXRNA_MAX_CHANNELS];
    u32 aram_addresses[AXRNA_MAX_CHANNELS];
    int buffer_size;
    int sample_rate;
    u32 request_owners[AXRNA_MAX_CHANNELS];
    SJ* inputs[AXRNA_MAX_CHANNELS];
    SJ* buffers[AXRNA_MAX_CHANNELS];
    SJCK input_chunks[AXRNA_MAX_CHANNELS];
    SJCK buffer_chunks[AXRNA_MAX_CHANNELS];
    /* Cleared asynchronously by the ARQ completion callbacks. */
    volatile int transfer_pending[AXRNA_MAX_CHANNELS];
    int transfer_samples;
    int transfer_position;
    volatile int flash_pending[AXRNA_MAX_CHANNELS];
    int flash_samples;
    int flash_position;
    int bits_per_sample;
    int output_volume;
    int output_pan[AXRNA_MAX_CHANNELS];
    int surround_pan;
    int aux_a;
    int aux_b;
    int fader;
    short adjust_sample_rate;
    short sample_rate_state;
    int source_type;
    ARQRequest requests[AXRNA_MAX_CHANNELS];
} AXRNAHandle;

typedef char AXRNAHandleSizeCheck[sizeof(AXRNAHandle) == 0xE8 ? 1 : -1];

void GCRNA_LockCs(void);
void GCRNA_UnlockCs(void);
void RNAERR_CallErrFunc(const char* message);
void RNAERR_EntryErrFunc(RNAErrorCallback callback, void* object);
RNAResource* RNARES_Create(void);
void RNARES_Destroy(RNAResource* resource);
u32 RNARES_GetBuf(RNAResource* resource);
u32 RNARES_GetBufSize(RNAResource* resource);
void RNARES_Init(const char* build);
void RNARES_Finish(void);
void AXRNA_ExecHndl(AXRNAHandle* handle);
void axrna_end_flash(unsigned long request_address);
void axrna_end_trans(unsigned long request_address);
void axrna_update_play(AXRNAHandle* handle);
void AXRNA_SetPlaySw(AXRNAHandle* handle, int enabled);
void AXRNA_SetTransSw(AXRNAHandle* handle, int enabled);
void AXRNA_Destroy(AXRNAHandle* handle);
void axrna_voice_drop(void* voice);

static const char axrna_build_string[] =
    "\nAXRNA Ver.1.04 Build:Sep  3 2004 17:49:08\n";
const char* const axrna_build = axrna_build_string;
static const char axrna_switch_off[] = "OFF";
static const char axrna_switch_on[] = "ON ";
/* Retained retail diagnostic labels; the table is part of the object data. */
static const char* const axrna_switch_names[] = {
    axrna_switch_off,
    axrna_switch_on,
};

int axrna_def_src_type = 1;
int axrna_pan_tbl[31] = {
    0, 4, 8, 12, 16, 20, 24, 28, 33, 37, 41, 45, 49, 53, 57, 64,
    68, 72, 76, 81, 85, 89, 93, 98, 102, 106, 110, 115, 119, 123, 127,
};

u32 axrna_init_cnt;
unsigned char* axrna_zero_dat;
int axrna_def_adjsfreq_fg;
int axrna_foo_cnt;
int axrna_update_pos;
int axrna_update_hist[32];
unsigned char axrna_zero_dat_real[0x103E];
AXRNAHandle axrna_obj[AXRNA_MAX_HANDLES];

static inline int axrna_get_play_switch(const AXRNAHandle* handle)
{
    if (handle == 0) {
        return -1;
    }
    return (handle->switches >> 1) & 1;
}

static inline int axrna_get_transfer_switch(const AXRNAHandle* handle)
{
    if (handle == 0) {
        return -1;
    }
    return handle->switches & 1;
}

static inline u32 axrna_get_voice_address(const AXVPB* voice)
{
    /* The SDK ABI stores this 32-bit DSP address as two adjacent halfwords. */
    return *(const u32*)&voice->pb.addr.currentAddressHi;
}

static inline void axrna_set_source_type(AXRNAHandle* handle, int source_type)
{
    if (handle != 0) {
        handle->source_type = source_type;
        handle->sample_rate_state = 1;
    }
}

static inline void axrna_wait(int count)
{
    int i;

    for (i = 0; i < count; i++) {
    }
}

void AXRNA_SetAdjsfreqFlg(AXRNAHandle* handle, int enabled)
{
    if (handle != 0) {
        handle->adjust_sample_rate = enabled;
    }
}

int AXRNA_DiscardData(AXRNAHandle* handle, int samples)
{
    return 0;
}

int AXRNA_SetStmHdInfo(AXRNAHandle* handle, void* stream_header)
{
    return 0;
}

void AXRNA_SetBitPerSmpl(AXRNAHandle* handle, int bits)
{
    if (handle != 0) {
        handle->bits_per_sample = bits;
    }
}

void AXRNA_SetOutPan(AXRNAHandle* handle, int channel, int pan)
{
    AXVPB* voice;

    if (handle != 0) {
        if (channel < handle->allocated_channels) {
            pan = pan < 15 ? pan : 15;
            pan = pan > -15 ? pan : -15;
            if (pan != handle->output_pan[channel]) {
                handle->output_pan[channel] = pan;
                GCRNA_LockCs();
                voice = handle->voices[channel];
                if (voice != 0) {
                    MIXSetPan(
                        voice, (unsigned char)axrna_pan_tbl[pan + 15]);
                }
                GCRNA_UnlockCs();
            }
        }
    }
}

void AXRNA_SetOutVol(AXRNAHandle* handle, int volume)
{
    int channel;

    if (handle == 0) {
        return;
    }
    volume = volume < 0 ? volume : 0;
    volume = volume > -999 ? volume : -999;
    if (volume == handle->output_volume) {
        return;
    }
    handle->output_volume = volume;
    for (channel = 0; channel < handle->allocated_channels; channel++) {
        GCRNA_LockCs();
        if (handle->voices[channel] != 0) {
            MIXSetInput(handle->voices[channel], volume);
        }
        GCRNA_UnlockCs();
    }
}

void AXRNA_SetSfreq(AXRNAHandle* handle, int sample_rate)
{
    AXPBSRC source;
    int channel;
    int adjusted_rate;

    if (handle == 0) {
        return;
    }
    handle->sample_rate = sample_rate;
    adjusted_rate = (sample_rate * 1124 + 1124) / 1125;
    for (channel = 0; channel < handle->allocated_channels; channel++) {
        GCRNA_LockCs();
        if (handle->voices[channel] != 0) {
            if (handle->adjust_sample_rate == 1) {
                if (sample_rate == 32000 && handle->sample_rate_state == 0) {
                    axrna_set_source_type(handle, AX_SRC_TYPE_NONE);
                }
                source.ratioHi = adjusted_rate / 32000;
                source.ratioLo = (adjusted_rate << 8) / 125;
            } else {
                source.ratioHi = sample_rate / 32000;
                source.ratioLo = (sample_rate << 8) / 125;
            }
            source.currentAddressFrac = 0;
            source.last_samples[0] = 0;
            source.last_samples[1] = 0;
            source.last_samples[2] = 0;
            source.last_samples[3] = 0;
            AXSetVoiceSrcType(handle->voices[channel], handle->source_type);
            AXSetVoiceSrc(handle->voices[channel], &source);
        }
        GCRNA_UnlockCs();
    }
}

void AXRNA_SetNumChan(AXRNAHandle* handle, int channels)
{
    if (handle != 0) {
        handle->num_channels = (signed char)channels;
    }
}

void AXRNA_ExecServer(void)
{
    int i;

    for (i = 0; i < AXRNA_MAX_HANDLES; i++) {
        if (axrna_obj[i].used == 1) {
            AXRNA_ExecHndl(&axrna_obj[i]);
        }
    }
}

void AXRNA_ExecHndl(AXRNAHandle* handle)
{
    SJCK buffer_chunk;
    SJCK buffer_remainder;
    SJCK input_chunk;
    SJCK input_remainder;
    int channel;
    int transfer_size;

    if (handle == 0) {
        return;
    }
    if (axrna_get_play_switch(handle) == 1) {
        axrna_update_play(handle);
    }
    if (axrna_get_transfer_switch(handle) == 1) {
        for (channel = 0; channel < handle->num_channels; channel++) {
            if (handle->voices[channel] == 0 ||
                handle->transfer_pending[channel] != 0) {
                continue;
            }
            handle->buffers[channel]->interface->get_chunk(
                handle->buffers[channel], 0, AXRNA_TRANSFER_BYTES,
                &buffer_chunk);
            handle->inputs[channel]->interface->get_chunk(
                handle->inputs[channel], 1, buffer_chunk.len, &input_chunk);
            transfer_size = buffer_chunk.len;
            if (input_chunk.len < transfer_size) {
                transfer_size = input_chunk.len;
            }
            transfer_size = (transfer_size / 32) * 32;
            SJ_SplitChunk(&buffer_chunk, transfer_size, &buffer_chunk,
                          &buffer_remainder);
            handle->buffers[channel]->interface->unget_chunk(
                handle->buffers[channel], 0, &buffer_remainder);
            SJ_SplitChunk(&input_chunk, transfer_size, &input_chunk,
                          &input_remainder);
            handle->inputs[channel]->interface->unget_chunk(
                handle->inputs[channel], 1, &input_remainder);
            if (transfer_size == 0) {
                return;
            }
            /* Mismatched channel chunks indicate a broken stereo SJ pair. */
            while (input_chunk.len != buffer_chunk.len) {
            }
            handle->input_chunks[channel] = input_chunk;
            handle->buffer_chunks[channel] = buffer_chunk;
            handle->transfer_samples = (unsigned int)transfer_size >> 1;
            DCFlushRange(handle->input_chunks[channel].data,
                         handle->input_chunks[channel].len);
            handle->transfer_pending[channel] = 1;
            ARQPostRequest(
                &handle->requests[channel], handle->request_owners[channel], 0,
                1, (unsigned long)input_chunk.data,
                (unsigned long)buffer_chunk.data, transfer_size,
                axrna_end_trans);
            while (handle->transfer_pending[channel] != 0) {
            }
        }
        return;
    }
    if (axrna_get_play_switch(handle) == 1 &&
        handle->flash_position < handle->buffer_size) {
        for (channel = 0; channel < handle->num_channels; channel++) {
            if (handle->flash_pending[channel] != 0) {
                continue;
            }
            handle->buffers[channel]->interface->get_chunk(
                handle->buffers[channel], 0, AXRNA_TRANSFER_BYTES,
                &buffer_chunk);
            transfer_size = (buffer_chunk.len / 32) * 32;
            SJ_SplitChunk(&buffer_chunk, transfer_size, &buffer_chunk,
                          &buffer_remainder);
            handle->buffers[channel]->interface->unget_chunk(
                handle->buffers[channel], 0, &buffer_remainder);
            if (transfer_size == 0) {
                return;
            }
            handle->buffer_chunks[channel] = buffer_chunk;
            handle->flash_samples = (unsigned int)transfer_size >> 1;
            DCFlushRange(axrna_zero_dat, AXRNA_BUFFER_SAMPLES);
            handle->flash_pending[channel] = 1;
            ARQPostRequest(
                &handle->requests[channel], handle->request_owners[channel], 0,
                1, (unsigned long)axrna_zero_dat,
                (unsigned long)buffer_chunk.data, transfer_size,
                axrna_end_flash);
            while (handle->flash_pending[channel] != 0) {
            }
        }
    }
}

void axrna_end_flash(unsigned long request_address)
{
    ARQRequest* request = (ARQRequest*)request_address;
    int owner = (int)(request->owner & 0x7FFFFFFF);
    int channel = owner % AXRNA_MAX_CHANNELS;
    AXRNAHandle* handle = &axrna_obj[owner / AXRNA_MAX_CHANNELS];

    if (handle->flash_pending[channel] == 1) {
        handle->buffers[channel]->interface->put_chunk(
            handle->buffers[channel], 1, &handle->buffer_chunks[channel]);
        handle->flash_pending[channel] = 0;
        if (channel == handle->num_channels - 1) {
            handle->flash_position += handle->flash_samples;
        }
    }
}

void axrna_end_trans(unsigned long request_address)
{
    ARQRequest* request = (ARQRequest*)request_address;
    int owner = (int)(request->owner & 0x7FFFFFFF);
    int channel = owner % AXRNA_MAX_CHANNELS;
    AXRNAHandle* handle = &axrna_obj[owner / AXRNA_MAX_CHANNELS];

    if (handle->transfer_pending[channel] == 1) {
        handle->inputs[channel]->interface->put_chunk(
            handle->inputs[channel], 0, &handle->input_chunks[channel]);
        handle->buffers[channel]->interface->put_chunk(
            handle->buffers[channel], 1, &handle->buffer_chunks[channel]);
        handle->transfer_pending[channel] = 0;
        if (channel == handle->num_channels - 1) {
            handle->transfer_position += handle->transfer_samples;
        }
    }
}

void axrna_update_play(AXRNAHandle* handle)
{
    SJCK chunk;
    AXVPB* voice;
    int current_position;
    int previous_position;
    int release_bytes;
    int channel;
    int played;

    voice = handle->voices[handle->num_channels - 1];
    previous_position = handle->play_position;
    if (voice == 0) {
        return;
    }
    current_position =
        axrna_get_voice_address(voice) -
        handle->aram_addresses[handle->num_channels - 1];
    axrna_update_hist[axrna_update_pos++] = current_position;
    if (axrna_update_pos == 32) {
        axrna_update_pos = 0;
    }
    /* A DSP address outside the allocated ARAM buffer is unrecoverable. */
    if (current_position < 0 || current_position > handle->buffer_size) {
        while (1) {
        }
    }
    if (previous_position == -1) {
        if (current_position == 0) {
            played = 0;
        } else {
            previous_position = 0;
            handle->play_position = 0;
        }
    }
    if (previous_position != -1) {
        if (current_position > previous_position) {
            played = current_position - previous_position;
        } else {
            played = AXRNA_BUFFER_SAMPLES -
                     (previous_position - current_position);
        }
    }
    played = (played / 2048) * 2048;
    if (played > 0) {
        /* Soft ceiling: release byte count/channel index register coloring. */
        release_bytes = played * 2;
        for (channel = 0; channel < handle->num_channels; channel++) {
            handle->buffers[channel]->interface->get_chunk(
                handle->buffers[channel], 1, release_bytes, &chunk);
            handle->buffers[channel]->interface->put_chunk(
                handle->buffers[channel], 0, &chunk);
        }
        handle->play_position += played;
        if (handle->play_position >= AXRNA_BUFFER_SAMPLES) {
            handle->play_position -= AXRNA_BUFFER_SAMPLES;
        }
    }
}

int AXRNA_GetNumRoom(AXRNAHandle* handle)
{
    SJ* buffer;

    if (handle == 0) {
        return -1;
    }
    buffer = handle->buffers[handle->num_channels - 1];
    return (unsigned int)buffer->interface->get_num_data(buffer, 0) >> 1;
}

int AXRNA_GetNumData(AXRNAHandle* handle)
{
    SJ* buffer;
    int num_data;

    if (handle == 0) {
        return -1;
    }
    buffer = handle->buffers[handle->num_channels - 1];
    num_data = AXRNA_BUFFER_SAMPLES -
               ((unsigned int)buffer->interface->get_num_data(buffer, 0) >> 1) -
               handle->flash_position;
    if (num_data < 0) {
        num_data = 0;
    }
    return num_data;
}

void AXRNA_SetPlaySw(AXRNAHandle* handle, int enabled)
{
    AXPBADDR address;
    int channel;
    u32 start;
    u32 end;

    if (handle == 0 || enabled == axrna_get_play_switch(handle)) {
        return;
    }
    GCRNA_LockCs();
    if (enabled == 1) {
        handle->play_position = -1;
        for (channel = 0; channel < handle->num_channels; channel++) {
            if (handle->voices[channel] != 0) {
                start = handle->aram_addresses[channel];
                end = start + handle->buffer_size - 1;
                address.loopFlag = 1;
                address.format = 10;
                address.loopAddressHi = start >> 16;
                address.loopAddressLo = start;
                address.endAddressHi = end >> 16;
                address.endAddressLo = end;
                address.currentAddressHi = start >> 16;
                address.currentAddressLo = start;
                AXSetVoiceAddr(handle->voices[channel], &address);
                AXSetVoiceState(handle->voices[channel], 1);
            }
        }
        handle->switches |= 2;
    } else if (enabled == 0) {
        for (channel = 0; channel < handle->num_channels; channel++) {
            if (handle->voices[channel] != 0) {
                AXSetVoiceState(handle->voices[channel], 0);
            }
        }
        for (channel = 0; channel < handle->allocated_channels; channel++) {
            handle->buffers[channel]->interface->reset(handle->buffers[channel]);
        }
        handle->switches &= 1;
    } else {
        RNAERR_CallErrFunc("E1070309:Illigal parameter(sw).\n");
    }
    GCRNA_UnlockCs();
}

void AXRNA_SetTransSw(AXRNAHandle* handle, int enabled)
{
    int channel;
    int retry;

    if (handle == 0 || enabled == axrna_get_transfer_switch(handle)) {
        return;
    }
    if (enabled == 1) {
        GCRNA_LockCs();
        for (channel = 0; channel < handle->num_channels; channel++) {
            handle->buffers[channel]->interface->reset(handle->buffers[channel]);
            memset(&handle->input_chunks[channel], 0, sizeof(SJCK));
            memset(&handle->buffer_chunks[channel], 0, sizeof(SJCK));
            memset(&handle->requests[channel], 0, sizeof(ARQRequest));
            handle->transfer_pending[channel] = 0;
        }
        handle->transfer_samples = 0;
        handle->transfer_position = 0;
        handle->flash_samples = 0;
        handle->flash_position = 0;
        handle->play_position = -1;
        handle->switches |= 1;
        GCRNA_UnlockCs();
        return;
    }
    if (enabled == 0) {
        for (channel = 0; channel < handle->num_channels; channel++) {
            retry = 0;
            while (handle->transfer_pending[channel] != 0 && retry < 200) {
                axrna_wait(100000);
                retry++;
            }
            if (retry == 200) {
                RNAERR_CallErrFunc(
                    "E2071701:DMA transfer(data) to A-RAM did not finish.\n");
                return;
            }
            retry = 0;
            while (handle->flash_pending[channel] != 0 && retry < 200) {
                axrna_wait(100000);
                retry++;
            }
            if (retry == 200) {
                RNAERR_CallErrFunc(
                    "E2071701:DMA transfer(flash) to A-RAM did not finish.\n");
                return;
            }
        }
        handle->switches &= 2;
    } else {
        RNAERR_CallErrFunc("E1070308:Illigal parameter(sw).\n");
    }
}

void AXRNA_Destroy(AXRNAHandle* handle)
{
    int channel;

    if (handle == 0) {
        return;
    }
    AXRNA_SetPlaySw(handle, 0);
    AXRNA_SetTransSw(handle, 0);
    for (channel = 0; channel < handle->allocated_channels; channel++) {
        if (handle->buffers[channel] != 0) {
            handle->buffers[channel]->interface->destroy(handle->buffers[channel]);
        }
        if (handle->resources[channel] != 0) {
            RNARES_Destroy(handle->resources[channel]);
        }
        GCRNA_LockCs();
        if (handle->voices[channel] != 0) {
            MIXReleaseChannel(handle->voices[channel]);
            AXFreeVoice(handle->voices[channel]);
        }
        GCRNA_UnlockCs();
    }
    memset(handle, 0, sizeof(AXRNAHandle));
}

AXRNAHandle* AXRNA_Create(SJ** inputs, int max_channels)
{
    AXRNAHandle* handle;
    int handle_index;
    int channel;

    if (max_channels <= 0) {
        RNAERR_CallErrFunc("E1070301:Illigal parameter(maxnch<=0).\n");
        return 0;
    }
    if (inputs == 0) {
        RNAERR_CallErrFunc("E1070302:Illigal parameter(sj=null).\n");
        return 0;
    }
    for (channel = 0; channel < max_channels; channel++) {
        if (inputs[channel] == 0) {
            RNAERR_CallErrFunc("E1070303:Illigal parameter(sj[]=null).\n");
            return 0;
        }
    }
    for (handle_index = 0; handle_index < AXRNA_MAX_HANDLES; handle_index++) {
        if (axrna_obj[handle_index].used == 0) {
            break;
        }
    }
    if (handle_index == AXRNA_MAX_HANDLES) {
        RNAERR_CallErrFunc("E1070304:Not enough RNA handle.\n");
        return 0;
    }

    handle = &axrna_obj[handle_index];
    handle->num_channels = (signed char)max_channels;
    handle->allocated_channels = (signed char)max_channels;
    for (channel = 0; channel < handle->allocated_channels; channel++) {
        handle->inputs[channel] = inputs[channel];
    }
    handle->output_volume = 0;
    handle->surround_pan = 127;
    handle->aux_a = -999;
    handle->aux_b = -999;
    handle->fader = 0;
    for (channel = 0; channel < handle->allocated_channels; channel++) {
        handle->request_owners[channel] =
            0x80000000 | (handle_index * AXRNA_MAX_CHANNELS + channel);
        handle->resources[channel] = RNARES_Create();
        if (handle->resources[channel] == 0) {
            RNAERR_CallErrFunc("E1070305:Can't create RNARES.\n");
            AXRNA_Destroy(handle);
            return 0;
        }
        handle->aram_addresses[channel] =
            RNARES_GetBuf(handle->resources[channel]);
        handle->buffer_size = RNARES_GetBufSize(handle->resources[channel]);
        handle->buffers[channel] = SJRBF_Create(
            (void*)(handle->aram_addresses[channel] * 2),
            handle->buffer_size * 2, 0);
        if (handle->buffers[channel] == 0) {
            RNAERR_CallErrFunc("E1070306:Can't create SJ.\n");
            AXRNA_Destroy(handle);
            return 0;
        }
        handle->voices[channel] = AXAcquireVoice(31, axrna_voice_drop, 0);
        if (handle->voices[channel] == 0) {
            RNAERR_CallErrFunc("E1070307:Can't acquire voice(AX).\n");
            AXRNA_Destroy(handle);
            return 0;
        }
        GCRNA_LockCs();
        if (handle->voices[channel] != 0) {
            MIXInitChannel(
                handle->voices[channel], 3, handle->output_volume,
                handle->aux_a, handle->aux_b, 64, handle->surround_pan,
                handle->fader);
        }
        GCRNA_UnlockCs();
    }

    AXRNA_SetAdjsfreqFlg(handle, (short)axrna_def_adjsfreq_fg);
    axrna_set_source_type(handle, axrna_def_src_type);
    handle->sample_rate_state = 0;
    AXRNA_SetSfreq(handle, 48000);
    AXRNA_SetBitPerSmpl(handle, 16);
    if (handle->allocated_channels == 2) {
        AXRNA_SetOutPan(handle, 0, -15);
        AXRNA_SetOutPan(handle, 1, 15);
    } else {
        AXRNA_SetOutPan(handle, 0, 0);
    }
    handle->switches = 0;
    handle->used = 1;
    return handle;
}

void axrna_voice_drop(void* voice_data)
{
    AXVPB* voice = (AXVPB*)voice_data;
    int handle_index;
    int channel;

    for (handle_index = 0; handle_index < AXRNA_MAX_HANDLES; handle_index++) {
        for (channel = 0; channel < AXRNA_MAX_CHANNELS; channel++) {
            if (voice == axrna_obj[handle_index].voices[channel]) {
                MIXReleaseChannel(axrna_obj[handle_index].voices[channel]);
                axrna_obj[handle_index].voices[channel] = 0;
                return;
            }
        }
    }
}

void AXRNA_Finish(void)
{
    int i;

    axrna_init_cnt--;
    if (axrna_init_cnt == 0) {
        for (i = 0; i < AXRNA_MAX_HANDLES; i++) {
            if (axrna_obj[i].used == 1) {
                AXRNA_Destroy(&axrna_obj[i]);
            }
        }
        memset(axrna_obj, 0, sizeof(axrna_obj));
        RNARES_Finish();
    }
}

void AXRNA_Init(void)
{
    if (axrna_init_cnt == 0) {
        RNARES_Init(axrna_build);
        memset(axrna_obj, 0, sizeof(axrna_obj));
        axrna_zero_dat = (unsigned char*)
            (((unsigned long)axrna_zero_dat_real + 31) & ~31UL);
    }
    axrna_init_cnt++;
}

void AXRNA_EntryErrFunc(RNAErrorCallback callback, void* object)
{
    RNAERR_EntryErrFunc(callback, object);
}
