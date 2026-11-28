#include "msl/mslBank.h"
#include "msl/mslPlayable.h"
#include "dolphin/ax.h"
#include "dolphin/os.h"
#include "dolphin/cache.h"
#include "dolphin/arq.h"
#include "msl/mslsupport.h"
#include "msl/mslStreamCache.h"
#include "msl/mslStreamFile.h"
#include "msl/mslARam.h"
#include "msl/mslgcn.h"
#include "msl/mslgcn_globals.h"
#include "msl/mslWave_internal.h"
#include "mw/mwMemNewDelete.h"
#include "runtime/cmath.h"
#include "mw/mwMemHeap.h"

struct _mslBank {
    unsigned char pad00[0x3C];
    _mwFile* stream_context;
    unsigned char pad40[4];
    MSLGCN_ARamBlock* resident_aram_block;
};

struct _GameCubeFileEntry {
    u32 name;
    u32 primary_aram_offset;
    s32 aram_size;
    SPSoundTable* sound_table;
    unsigned char pad10[8];
    int has_secondary;
    u32 secondary_name;
    u32 secondary_aram_offset;
    u32 unknown24;
    SPSoundTable* secondary_sound_table;
    u32 unknown2C;
};

struct mslStreamFileRequest;

class SoundBuffer_Playable;

struct SoundBufferUpdateList {
    SoundBuffer_Playable* first;
    SoundBuffer_Playable* last;
};

typedef char IRefCntResSize[sizeof(IRefCntRes) == 0x08 ? 1 : -1];
typedef char SoundBufferDataSize[sizeof(SoundBuffer_Data) == 0x14 ? 1 : -1];
typedef char SoundBufferPlayableSize[sizeof(SoundBuffer_Playable) == 0x38 ? 1 : -1];

class SBPlayable_Stream : public SoundBuffer_Playable {
public:
    static void StreamFileRead_CallBack(
        void* buffer, u32 offset, int size, int error,
        int final_chunk, void* callback_data);
    static void i_ARQCALLBACK_ArqComplete_ReturnBuffer(
        u32 request_address);
    static void i_ARQCALLBACK_ArqComplete(
        u32 request_address);
    static void i_ARQCALLBACK_ArqComplete_LastRead(
        u32 request_address);
    static SBPlayable_Stream* GetObject(void);
    SBPlayable_Stream();
    virtual ~SBPlayable_Stream();
    virtual void FreeObject(void);
    virtual void FreeResources(void);
    virtual void PrepForPlay(void);
    virtual int IsReadyToPlay(void);
    virtual int Play(u32 flags);
    virtual int Stop(void);
    virtual int Pause(void);
    virtual void iUpdate_AXUser(void);
    virtual void iUpdate_MslTick(void);
    virtual void StopIfDonePlaying(void);
    void ResetValues(void);
    void UnPrep(void);
    void iPrepForPlay(void);
    int iPlayPrepped(void);
    void iAX_FindNewEndBlock(int block);
    inline void iAX_GetVoiceBlock(
        _AXVPB* voice, int channel, int& block,
        u32* raw_address);

    unsigned char ready_to_play;
    unsigned char last_read_pending;
    unsigned char voices_started;
    unsigned char play_when_ready;
    u32 play_flags;
    u32 cache_buffers[2];
    s32 cache_buffer_size;
    s32 cache_buffer1_size;
    _mwFile* stream_context;
    u32 primary_offset;
    u32 aligned_size;
    u32 segment_size;
    int segment_shift;
    u32 source_read_offset;
    u32 source_bytes_remaining;
    mslStreamFileRequest* pending_file_request;
    u32 queued_read_offset;
    u32 queued_read_size;
    u32 queued_block_count;
    unsigned char partial_read;
    unsigned char at_zero_buffer;
    unsigned char crossed_stream_end;
    unsigned char end_of_stream;
    volatile s32 pending_arq_count;
    s32 ring_block_count;
    s32 source_blocks_remaining;
    s32 ring_write_block;
    s32 ring_play_block;
    s32 pending_ax_block;
    s32 ax_end_block;
    s32 end_pass_count;
    s32 stream_end_block;
    u32 initial_position;
    s32 ring_scan_stop_block;
    unsigned char block_headers[2][0x10];
};

typedef char SBPlayableStreamSize[sizeof(SBPlayable_Stream) == 0xCC ? 1 : -1];

SoundBufferUpdateList SoundBuffer_Playable::ms_UpdateList = {0, 0};

void AXVPB_ComputeVoiceRatio(
    u32 frequency, unsigned short& ratio_hi,
    unsigned short& ratio_lo);
void i_AXVPB_SetVoiceRatioOnRunningVoice(
    _AXVPB* voice, unsigned short ratio_hi, unsigned short ratio_lo);
u32 i_AXVPB_GetVoice_CurrentPosition(_AXVPB* voice);
u32 i_AXVPB_GetVoice_LoopPosition(_AXVPB* voice);
u32 i_AXVPB_GetVoice_EndPosition(_AXVPB* voice);
void i_AXVPB_SetVoice_Addr(_AXVPB* voice, AXPBADDR* address);
void i_AXVPB_SetVoice_CurrentPosition(_AXVPB* voice, u32 position);
void i_AXVPB_SetVoice_LoopPosition(_AXVPB* voice, u32 position);
void i_AXVPB_SetVoice_EndPosition(_AXVPB* voice, u32 position);
void i_AXVPB_SetVoice_Looping(_AXVPB* voice, u32 looping);
void i_AXVPB_SetVoice_ADPCM_LoopPredScale(_AXVPB* voice, u32 pred_scale);
void i_AXVPB_SetVoice_ADPCM_StartPredScale(_AXVPB* voice, u32 pred_scale);
void i_AXVPB_SetVoice_Type(_AXVPB* voice, u32 type);
void i_AXVPB_SetVoice_LoopPosition_PredScale(
    _AXVPB* voice, u32 position, u32 pred_scale,
    bool at_stream_end);
void i_AXVPB_SetVoice_Loop_To_Zero(_AXVPB* voice);

static inline int IsPlaybackLoopRequested(u32 flags) {
    int requested = 0;
    if ((flags & 1) != 0) {
        requested = 1;
    }
    return requested;
}

void AXVPB_ComputeVoiceRatio(
    u32 frequency, unsigned short& ratio_hi,
    unsigned short& ratio_lo) {
    if (frequency < 63) {
        frequency = 63;
    } else if (frequency > 128000) {
        frequency = 128000;
    }
    ratio_hi = frequency / 32000;
    ratio_lo = (frequency << 8) / 125;
}

void i_AXVPB_SetVoiceRatioOnRunningVoice(
    _AXVPB* voice, unsigned short ratio_hi, unsigned short ratio_lo) {
    u32 sync = voice->sync;

    voice->pb.src.ratioHi = ratio_hi;
    sync |= 0x80000;
    voice->pb.src.ratioLo = ratio_lo;
    voice->sync = sync;
}

u32 i_AXVPB_GetVoice_CurrentPosition(_AXVPB* voice) {
    u32 position = (u32)voice->pb.addr.currentAddressHi << 16;

    position += voice->pb.addr.currentAddressLo;
    return position;
}

u32 i_AXVPB_GetVoice_LoopPosition(_AXVPB* voice) {
    u32 position = (u32)voice->pb.addr.loopAddressHi << 16;

    position += voice->pb.addr.loopAddressLo;
    return position;
}

u32 i_AXVPB_GetVoice_EndPosition(_AXVPB* voice) {
    u32 position = (u32)voice->pb.addr.endAddressHi << 16;

    position += voice->pb.addr.endAddressLo;
    return position;
}

void i_AXVPB_SetVoice_Addr(_AXVPB* voice, AXPBADDR* address) {
    u32 sync = voice->sync;

    voice->pb.addr.loopFlag = address->loopFlag;
    voice->pb.addr.format = address->format;
    voice->pb.addr.loopAddressHi = address->loopAddressHi;
    voice->pb.addr.loopAddressLo = address->loopAddressLo;
    voice->pb.addr.endAddressHi = address->endAddressHi;
    voice->pb.addr.endAddressLo = address->endAddressLo;
    voice->pb.addr.currentAddressHi = address->currentAddressHi;
    voice->pb.addr.currentAddressLo = address->currentAddressLo;
    voice->sync = (sync | 0x1000) & 0xFFFE1FFF;
}

void i_AXVPB_SetVoice_CurrentPosition(_AXVPB* voice, u32 position) {
    u32 sync = voice->sync | 0x10000;

    voice->pb.addr.currentAddressLo = position;
    voice->pb.addr.currentAddressHi = position >> 16;
    if ((sync & 0x1000) == 0) {
        voice->sync = sync;
    }
}

void i_AXVPB_SetVoice_LoopPosition(_AXVPB* voice, u32 position) {
    u32 sync = voice->sync | 0x4000;

    voice->pb.addr.loopAddressLo = position;
    voice->pb.addr.loopAddressHi = position >> 16;
    if ((sync & 0x1000) == 0) {
        voice->sync = sync;
    }
}

void i_AXVPB_SetVoice_EndPosition(_AXVPB* voice, u32 position) {
    u32 sync = voice->sync | 0x8000;

    voice->pb.addr.endAddressLo = position;
    voice->pb.addr.endAddressHi = position >> 16;
    if ((sync & 0x1000) == 0) {
        voice->sync = sync;
    }
}

void i_AXVPB_SetVoice_Looping(_AXVPB* voice, u32 looping) {
    u32 sync = voice->sync;

    sync |= 0x2000;
    voice->pb.addr.loopFlag = looping != 0;
    if ((sync & 0x1000) == 0) {
        voice->sync = sync;
    }
}

void i_AXVPB_SetVoice_ADPCM_LoopPredScale(_AXVPB* voice, u32 pred_scale) {
    u32 sync = voice->sync;

    voice->pb.adpcmLoop.loop_pred_scale = pred_scale;
    sync |= 0x100000;
    voice->pb.adpcmLoop.loop_yn1 = 0;
    voice->pb.adpcmLoop.loop_yn2 = 0;
    voice->sync = sync;
}

void i_AXVPB_SetVoice_ADPCM_StartPredScale(_AXVPB* voice, u32 pred_scale) {
    u32 sync = voice->sync | 0x20000;

    voice->pb.adpcm.pred_scale = pred_scale;
    voice->pb.adpcm.yn1 = 0;
    voice->pb.adpcm.yn2 = 0;
    voice->sync = sync;
}

void i_AXVPB_SetVoice_Type(_AXVPB* voice, u32 type) {
    u32 sync = voice->sync;

    voice->pb.type = type;
    sync |= 8;
    voice->sync = sync;
}

void i_AXVPB_SetVoice_LoopPosition_PredScale(
    _AXVPB* voice, u32 position, u32 pred_scale,
    bool at_stream_end) {
    u32 sync;

    i_AXVPB_SetVoice_LoopPosition(voice, position);
    sync = voice->sync;
    voice->pb.adpcmLoop.loop_pred_scale = pred_scale;
    sync |= 0x100000;
    voice->pb.adpcmLoop.loop_yn1 = 0;
    voice->pb.adpcmLoop.loop_yn2 = 0;
    voice->sync = sync;
    if (at_stream_end) {
        i_AXVPB_SetVoice_Type(voice, 0);
    } else {
        i_AXVPB_SetVoice_Type(voice, 1);
    }
    i_AXVPB_SetVoice_Looping(voice, 1);
}

void i_AXVPB_SetVoice_Loop_To_Zero(_AXVPB* voice) {
    i_AXVPB_SetVoice_LoopPosition(voice, g_MSL_GCN_ARAM_ZeroBase_ADPCM_Start);
    i_AXVPB_SetVoice_ADPCM_LoopPredScale(voice, 0);
    i_AXVPB_SetVoice_Type(voice, 0);
    i_AXVPB_SetVoice_Looping(voice, 0);
}

SoundBuffer::SoundBuffer() {
}

SoundBuffer::~SoundBuffer() {
}

void SoundBuffer::PrepForPlay(void) {
}

int SoundBuffer::IsReadyToPlay(void) {
    return 0;
}

int SoundBuffer::GetNumChannels(void) {
    _MSL_GCN_BREAK();
    return 0;
}

int SoundBuffer::GetStatus(u32* status) {
    _MSL_GCN_BREAK();
    return -1;
}

int SoundBuffer::SetFrequency(u32 frequency) {
    _MSL_GCN_BREAK();
    return -1;
}

int SoundBuffer::SetRelativeFrequency(float frequency) {
    _MSL_GCN_BREAK();
    return -1;
}

int SoundBuffer::SetVolume(s32 volume) {
    _MSL_GCN_BREAK();
    return -1;
}

int SoundBuffer::SetRelativeVolume(float volume) {
    _MSL_GCN_BREAK();
    return -1;
}

int SoundBuffer::SetPan(unsigned char pan) {
    _MSL_GCN_BREAK();
    return -1;
}

int SoundBuffer::SetSurroundPan(unsigned char pan) {
    _MSL_GCN_BREAK();
    return -1;
}

int SoundBuffer::SetRelativePan(float pan) {
    _MSL_GCN_BREAK();
    return -1;
}

int SoundBuffer::Play(u32 flags) {
    _MSL_GCN_BREAK();
    return -1;
}

int SoundBuffer::Stop(void) {
    return -1;
}

int SoundBuffer::Pause(void) {
    _MSL_GCN_BREAK();
    return -1;
}

int SoundBuffer::UnPause(void) {
    _MSL_GCN_BREAK();
    return -1;
}

int SoundBuffer::SetCurrentPosition(u32 position) {
    _MSL_GCN_BREAK();
    return -1;
}

SoundBuffer_Data::SoundBuffer_Data() {
    ResetValues();
}

SoundBuffer_Data::~SoundBuffer_Data() {
    FreeResources();
}

/* TODO: [provisional] linker-stripped (MAP UNUSED 0x84); body from sibling GetObject, compiles to 0x84. */
SoundBuffer_Data* SoundBuffer_Data::GetObject(void) {
    return new (MWSOUND_HEAP, (mwMemFlags)0x10, "SoundBuffer_Data", 0, 0)
        SoundBuffer_Data;
}

void SoundBuffer_Data::FreeObject(void) {
    this->FreeResources();
    delete this;
}

void SoundBuffer_Data::ResetValues(void) {
    aram_block = 0;
    bank = 0;
    file_entry = 0;
}

void SoundBuffer_Data::FreeResources(void) {
    if (aram_block != 0) {
        aram_block->Release();
        aram_block = 0;
    }
}

int SoundBuffer_Data::GetNumChannels(void) {
    return (file_entry->has_secondary != 0) + 1;
}

SPSoundEntry* SoundBuffer_Data::GetSoundEntryLeft(void) {
    SPSoundEntry* sound = 0;

    if (file_entry->sound_table != 0) {
        sound = SPGetSoundEntry(file_entry->sound_table, 0);
    }
    return sound;
}

SPSoundEntry* SoundBuffer_Data::GetSoundEntryRight(void) {
    SPSoundEntry* sound = 0;

    if (file_entry->secondary_sound_table != 0) {
        sound = SPGetSoundEntry(file_entry->secondary_sound_table, 0);
    }
    return sound;
}

void SoundBuffer_Playable::ListUpdate_AXUser(void) {
    SoundBuffer_Playable* playable;
    BOOL enabled = OSDisableInterrupts();

    playable = ms_UpdateList.first;
    while (playable != 0) {
        SoundBuffer_Playable* next = playable->next;
        playable->iUpdate_AXUser();
        playable = next;
    }
    OSRestoreInterrupts(enabled);
}

void SoundBuffer_Playable::ListUpdate_MslTick(void) {
    SoundBuffer_Playable* playable;
    BOOL enabled = OSDisableInterrupts();

    playable = ms_UpdateList.first;
    while (playable != 0) {
        SoundBuffer_Playable* next = playable->next;
        playable->iUpdate_MslTick();
        playable = next;
    }
    OSRestoreInterrupts(enabled);
}

void SoundBuffer_Playable::UpdateList_Insert(void) {
    BOOL enabled = OSDisableInterrupts();

    if ((previous = ms_UpdateList.last) != 0) {
        ms_UpdateList.last->next = this;
    } else {
        ms_UpdateList.first = this;
    }
    ms_UpdateList.last = this;
    next = 0;
    OSRestoreInterrupts(enabled);
}

void SoundBuffer_Playable::UpdateList_Remove(void) {
    BOOL enabled = OSDisableInterrupts();

    if (previous != 0) {
        if ((previous->next = next) != 0) {
            next->previous = previous;
        } else {
            ms_UpdateList.last = previous;
        }
    } else {
        ms_UpdateList.first = next;
        if (ms_UpdateList.first != 0) {
            next->previous = 0;
        } else {
            ms_UpdateList.last = 0;
        }
    }
    OSRestoreInterrupts(enabled);
}

SoundBuffer_Playable::SoundBuffer_Playable() {
    ResetValues();
}

SoundBuffer_Playable::~SoundBuffer_Playable() {
    FreeResources();
}

SoundBuffer_Playable* SoundBuffer_Playable::GetObject(void) {
    SoundBuffer_Playable* playable =
        new (MWSOUND_HEAP, (mwMemFlags)0x10, "SoundBuffer_Playable", 0, 0)
            SoundBuffer_Playable;

    playable->UpdateList_Insert();
    return playable;
}

void SoundBuffer_Playable::FreeObject(void) {
    FreeResources();
    UpdateList_Remove();
    delete this;
}

void SoundBuffer_Playable::ResetValues(void) {
    current_position = 0;
    mix_fader = 0;
    pan = 0x40;
    volume = 0x7F;
    frequency = 0;
    state = 8;
    voices[0] = 0;
    voices[1] = 0;
}

void SoundBuffer_Playable::FreeResources(void) {
    SoundBuffer_Playable* self = this;
    this->Stop();
    if (self->aram_block != 0) {
        self->aram_block->Release();
        self->aram_block = 0;
    }
}

int SoundBuffer_Playable::IsReadyToPlay(void) {
    return 1;
}

int SoundBuffer_Playable::GetStatus(u32* status) {
    SoundBuffer_Playable* self = this;
    int result = -1;
    u32 result_status = 0;

    if (self->state == 8) {
        result_status = 8;
    } else {
        if ((self->state & 2) != 0) {
            result_status |= 2;
            result = 0;
        }
        if ((self->state & 4) != 0) {
            result_status |= 4;
            result = 0;
        }
        if ((self->state & 1) != 0) {
            result_status |= 1;
        }
    }
    if (status == 0) {
        return result;
    }
    *status = result_status;
    return result;
}

int SoundBuffer_Playable::SetFrequency(u32 frequency) {
    SoundBuffer_Playable* self = this;
    BOOL enabled;
    _AXVPB* primary;
    unsigned short ratio_hi;
    unsigned short ratio_lo;

    self->frequency = frequency;
    enabled = OSDisableInterrupts();
    primary = self->voices[0];
    if (primary != 0 ||
        self->voices[1] != 0) {
        AXVPB_ComputeVoiceRatio(frequency, ratio_hi, ratio_lo);
        if (primary != 0) {
            i_AXVPB_SetVoiceRatioOnRunningVoice(primary, ratio_hi, ratio_lo);
        }
        if (self->voices[1] != 0) {
            i_AXVPB_SetVoiceRatioOnRunningVoice(
                self->voices[1], ratio_hi, ratio_lo);
        }
    }
    OSRestoreInterrupts(enabled);
    return 0;
}

int SoundBuffer_Playable::SetRelativeFrequency(float frequency) {
    u32 scaled_frequency = 0;
    SPSoundEntry* sound = GetSoundEntryLeft();

    if (sound != 0) {
        scaled_frequency = sound->sample_rate * frequency;
    }
    return SetFrequency(scaled_frequency);
}

int SoundBuffer_Playable::SetVolume(s32 volume) {
    SoundBuffer_Playable* self = this;
    self->mix_fader = volume;
    if (self->voices[0] != 0) {
        MIXSetFader(self->voices[0], volume);
    }
    if (self->voices[1] != 0) {
        MIXSetFader(self->voices[1], volume);
    }
    return 0;
}

int SoundBuffer_Playable::SetRelativeVolume(float volume) {
    s32 db_volume =
        mslWaveGetDbMapEntryRelative(volume);

    return this->SetVolume(db_volume);
}

int SoundBuffer_Playable::SetPan(unsigned char pan) {
    SoundBuffer_Playable* self = this;
    int result = -1;

    if (pan < 0x80) {
        self->pan = pan;
        if (self->state != 8 &&
            self->file_entry->has_secondary == 0 &&
            self->voices[0] != 0) {
            MIXSetPan(self->voices[0], self->pan);
            result = 0;
        }
    }
    return result;
}

int SoundBuffer_Playable::SetSurroundPan(unsigned char pan) {
    SoundBuffer_Playable* self = this;
    int result = -1;

    if (pan < 0x80) {
        self->volume = pan;
        if (self->state != 8 &&
            self->file_entry->has_secondary == 0 &&
            self->voices[0] != 0) {
            MIXSetSPan(self->voices[0], self->volume);
            result = 0;
        }
    }
    return result;
}

int SoundBuffer_Playable::SetRelativePan(float pan) {
    static const unsigned char SurroundPanTable[] = {
        0x40, 0x10, 0x10, 0x40, 0x08, 0x78,
        0x20, 0x7C, 0x40, 0x7C, 0x60, 0x7C,
        0x77, 0x78, 0x70, 0x40, 0x40, 0x10
    };
    float scaled = 2.0f * (pan - -2.0f);
    int index = (float)floor(scaled);
    float fraction;
    float inverse;

    if (index < 0) {
        index = 0;
    } else if (index > 7) {
        index = 7;
    }

    fraction = scaled - (float)index;
    if (fraction < 0.0f) {
        fraction = 0.0f;
    } else if (fraction > 1.0f) {
        fraction = 1.0f;
    }
    inverse = 1.0f - fraction;

    int offset = index * 2;
    unsigned char output_pan = (
        inverse * SurroundPanTable[offset] +
        fraction * SurroundPanTable[offset + 2]);
    int result = 0;
    unsigned char output_surround = (
        inverse * SurroundPanTable[offset + 1] +
        fraction * SurroundPanTable[offset + 3]);

    if (this->SetPan(output_pan) == -1) {
        result = -1;
    }
    if (this->SetSurroundPan(output_surround) == -1) {
        result = -1;
    }
    return result;
}

int SoundBuffer_Playable::Play(u32 flags) {
    BOOL enabled = OSDisableInterrupts();
    int result = iPlay(flags, 15, 10);
    OSRestoreInterrupts(enabled);
    return result;
}

int SoundBuffer_Playable::Stop(void) {
    SoundBuffer_Playable* self = this;
    int result = -1;
    BOOL enabled = OSDisableInterrupts();

    if (self->voices[0] != 0) {
        AXSetVoiceState(self->voices[0], 0);
        MIXReleaseChannel(self->voices[0]);
        AXFreeVoice(self->voices[0]);
        self->voices[0] = 0;
        result = 0;
    }
    if (self->voices[1] != 0) {
        AXSetVoiceState(self->voices[1], 0);
        MIXReleaseChannel(self->voices[1]);
        AXFreeVoice(self->voices[1]);
        self->voices[1] = 0;
        result = 0;
    }
    OSRestoreInterrupts(enabled);
    self->state = 8;
    return result;
}

int SoundBuffer_Playable::Pause(void) {
    SoundBuffer_Playable* self = this;
    int result = -1;

    this->StopIfDonePlaying();
    if ((self->state & 2) != 0) {
        BOOL enabled = OSDisableInterrupts();
        if (self->voices[0] != 0) {
            AXSetVoiceState(self->voices[0], 0);
            result = 0;
        }
        if (self->voices[1] != 0) {
            AXSetVoiceState(self->voices[1], 0);
            result = 0;
        }
        OSRestoreInterrupts(enabled);
        self->state = (self->state & 1) | 4;
    }
    return result;
}

int SoundBuffer_Playable::UnPause(void) {
    SoundBuffer_Playable* self = this;
    int result = -1;

    if ((self->state & 4) != 0) {
        result = this->Play(self->state & 1);
    }
    return result;
}

int SoundBuffer_Playable::SetCurrentPosition(
    u32 position) {
    SoundBuffer_Playable* self = this;

    if (position != 0) {
        _MSL_GCN_BREAK();
    }
    if (self->state != 8) {
        this->Stop();
        self->current_position = position;
    }
    return -1;
}

void SoundBuffer_Playable::InitializeVoice(
    _AXVPB* voice, SPSoundEntry* sound, int stream_loop,
    int use_loop_address) {
    AXVoiceSrc source;
    AXPBADDR address;
    u32 frequency = this->frequency;

    if (frequency == 0) {
        frequency = sound->sample_rate;
    }
    AXVPB_ComputeVoiceRatio(frequency, source.ratioHi, source.ratioLo);
    source.currentAddressFrac = 0;
    source.last_samples[0] = 0;
    source.last_samples[1] = 0;
    source.last_samples[2] = 0;
    source.last_samples[3] = 0;
    AXSetVoiceSrc(voice, &source);
    AXSetVoiceAdpcm(voice, &sound->adpcm->adpcm);

    if (stream_loop != 0) {
        i_AXVPB_SetVoice_Type(voice, 1);
    } else {
        i_AXVPB_SetVoice_Type(voice, 0);
    }
    if (use_loop_address != 0) {
        address.loopFlag = 1;
        address.loopAddressHi = sound->current_address >> 16;
        address.loopAddressLo = sound->current_address;
    } else {
        address.loopFlag = 0;
        address.loopAddressHi = sound->loop_address >> 16;
        address.loopAddressLo = sound->loop_address;
    }
    address.endAddressHi = sound->end_address >> 16;
    address.endAddressLo = sound->end_address;
    address.currentAddressHi = sound->current_address >> 16;
    address.currentAddressLo = sound->current_address;
    address.format = 0;
    i_AXVPB_SetVoice_Addr(voice, &address);
}

int SoundBuffer_Playable::iPlay(
    u32 flags, u32 acquire_priority,
    u32 active_priority) {
    int result = -1;

    if ((this->state & 4) != 0) {
        this->state = (this->state & 1) | 2;
        if (this->voices[0] != 0) {
            AXSetVoiceState(this->voices[0], 1);
            result = 0;
        }
        if (this->voices[1] != 0) {
            AXSetVoiceState(this->voices[1], 1);
            result = 0;
        }
    } else if ((this->state & 8) != 0) {
        this->voices[0] = AXAcquireVoice(
            acquire_priority, AcquireVoiceCallback, (uptr)this);
        if (this->voices[0] == 0) {
            return -1;
        }
        if (active_priority != acquire_priority) {
            AXSetVoicePriority(this->voices[0], active_priority);
        }

        if (this->file_entry->has_secondary != 0) {
            this->voices[1] = AXAcquireVoice(
                acquire_priority, AcquireVoiceCallback, (uptr)this);
            if (this->voices[1] == 0) {
                AXFreeVoice(this->voices[0]);
                this->voices[0] = 0;
                return -1;
            }
            if (active_priority != acquire_priority) {
                AXSetVoicePriority(this->voices[1], active_priority);
            }
        }

        int loop = IsPlaybackLoopRequested(flags);
        SPSoundEntry* sound = this->GetSoundEntryLeft();
        this->InitializeVoice(this->voices[0], sound, 0, loop);

        if (this->file_entry->has_secondary != 0) {
            sound = this->GetSoundEntryRight();
            this->InitializeVoice(this->voices[1], sound, 0, loop);
            MIXInitChannel(
                this->voices[0], 0, 0, -960, -960,
                0, 0x7F, this->mix_fader);
            MIXInitChannel(
                this->voices[1], 0, 0, -960, -960,
                0x7F, 0x7F, this->mix_fader);
            AXSetVoiceState(this->voices[0], 1);
            AXSetVoiceState(this->voices[1], 1);
        } else {
            MIXInitChannel(
                this->voices[0], 0, 0, -960, -960,
                this->pan, this->volume, this->mix_fader);
            AXSetVoiceState(this->voices[0], 1);
        }

        this->state = 2;
        if ((flags & 1) != 0) {
            this->state |= 1;
        }
        result = 0;
    }
    return result;
}

void SoundBuffer_Playable::AcquireVoiceCallback(void* voice) {
    AXVPB* callback_voice = (AXVPB*)voice;
    SoundBuffer_Playable* owner =
        (SoundBuffer_Playable*)callback_voice->user_context;

    owner->LostVoice(callback_voice);
}

void SoundBuffer_Playable::LostVoice(_AXVPB* voice) {
    SoundBuffer_Playable* self = this;
    BOOL enabled = OSDisableInterrupts();

    if (voice == self->voices[0]) {
        self->voices[0] = 0;
        this->Stop();
    } else if (voice == self->voices[1]) {
        self->voices[1] = 0;
        this->Stop();
    }

    OSRestoreInterrupts(enabled);
}

void SoundBuffer_Playable::iUpdate_AXUser(void) {
}

void SoundBuffer_Playable::iUpdate_MslTick(void) {
    this->StopIfDonePlaying();
}

void SoundBuffer_Playable::StopIfDonePlaying(void) {
    SoundBuffer_Playable* self = this;

    if ((self->state & 2) != 0) {
        int done = 0;

        if (self->voices[0] != 0 &&
            (self->voices[0]->pb.state & 1) == 0) {
            done = 1;
        }
        if (self->voices[1] != 0 &&
            (self->voices[1]->pb.state & 1) == 0) {
            done = 1;
        }
        if (done != 0) {
            this->Stop();
        }
    }
}

#include "src/libmsl/SBPlayable_Stream.cpp"

void SetAramFromGCFileEntry(
    MSLGCN_ARamBlock* aram_block, _GameCubeFileEntry* entry) {
    u32 secondary_offset;
    int channel_count;
    u32 primary_offset = entry->primary_aram_offset;
    u32 aram_size = entry->aram_size;

    if (entry->has_secondary != 0) {
        secondary_offset = entry->secondary_aram_offset;
        channel_count = 2;
    } else {
        channel_count = 1;
        secondary_offset = 0;
    }
    aram_block->SetNumChannels(channel_count);
    aram_block->SetARamBuffers(
        primary_offset, secondary_offset, aram_size);
}

SoundBuffer_Data* SoundBuffer_Data::iCreateAramOrDestroyBuffer(
    SoundBuffer_Data* buffer, _mslBank* bank,
    _GameCubeFileEntry* entry) {
    MSLGCN_ARamBlock* aram_block;

    if (buffer == 0) {
        return 0;
    }
    buffer->bank = bank;
    buffer->file_entry = entry;
    aram_block = MSLGCN_ARamBlock::GetObject();
    if (aram_block != 0) {
        buffer->aram_block = aram_block;
        aram_block->SetParent(bank->resident_aram_block);
        SetAramFromGCFileEntry(aram_block, entry);
    } else {
        if (--buffer->reference_count == 0) {
            buffer->FreeObject();
        }
        buffer = 0;
    }
    return buffer;
}

SoundBuffer_Playable* SoundBuffer::CreatePlayableStaticBuffer(
    _mslBank* bank, _GameCubeFileEntry* entry) {
    BOOL enabled = OSDisableInterrupts();
    SoundBuffer_Playable* playable = (SoundBuffer_Playable*)
        SoundBuffer_Data::iCreateAramOrDestroyBuffer(
            SoundBuffer_Playable::GetObject(), bank, entry);

    OSRestoreInterrupts(enabled);
    return playable;
}

SoundBuffer_Playable* SoundBuffer::CreatePlayableStreamBuffer(
    _mslBank* bank, _GameCubeFileEntry* entry) {
    BOOL enabled = OSDisableInterrupts();
    SBPlayable_Stream* stream = SBPlayable_Stream::GetObject();

    if (stream != 0) {
        SPSoundEntry* sound;

        stream->bank = bank;
        stream->file_entry = entry;
        stream->stream_context = bank->stream_context;
        stream->primary_offset = entry->primary_aram_offset;
        if (stream->file_entry->has_secondary != 0) {
            stream->aligned_size =
                (entry->aram_size + 0x1FFF) & ~0x1FFF;
            stream->aligned_size <<= 1;
            stream->segment_size =
                entry->secondary_aram_offset -
                entry->primary_aram_offset;
        } else {
            stream->aligned_size =
                (entry->aram_size + 0x7FF) & ~0x7FF;
            stream->segment_size = 0x4000;
        }
        stream->segment_shift = mslIntLog2(stream->segment_size);
        sound = SPGetSoundEntry(entry->sound_table, 0);
        stream->initial_position =
            sound->end_address & (stream->segment_size * 2 - 1);
        stream->PrepForPlay();
    }

    OSRestoreInterrupts(enabled);
    return stream;
}

void SoundBuffer::SB_AXUserCallback(void) {
    SoundBuffer_Playable::ListUpdate_AXUser();
}

void SoundBuffer::SB_MslTickCallback(void) {
    SoundBuffer_Playable::ListUpdate_MslTick();
}
