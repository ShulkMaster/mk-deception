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

typedef void (*SoundBufferMethod)(SoundBuffer_Playable*);
typedef int (*SoundBufferIntMethod)(SoundBuffer_Playable*);
typedef void (*SoundBufferLostVoiceMethod)(
    SoundBuffer_Playable*, _AXVPB*);
typedef int (*SoundBufferLongMethod)(
    SoundBuffer_Playable*, s32);
typedef int (*SoundBufferByteMethod)(
    SoundBuffer_Playable*, unsigned char);
typedef SoundBuffer_Playable* (*SoundBufferDestroyMethod)(
    SoundBuffer_Playable*, short);

struct SoundBufferPlayableVTable {
    unsigned char pad00[8];
    SoundBufferDestroyMethod Destroy;
    SoundBufferMethod FreeObject;
    SoundBufferMethod FreeResources;
    unsigned char pad14[0x18];
    SoundBufferLongMethod SetVolume;
    unsigned char pad30[4];
    SoundBufferByteMethod SetPan;
    SoundBufferByteMethod SetSurroundPan;
    unsigned char pad3C[8];
    SoundBufferIntMethod Stop;
    unsigned char pad48[0x0C];
    SoundBufferLostVoiceMethod LostVoice;
    unsigned char pad58[8];
    SoundBufferMethod StopIfDonePlaying;
};

struct SoundBufferPlayableLayout {
    SoundBufferPlayableVTable* vtable;
    int reference_count;
    MSLGCN_ARamBlock* aram_block;
    _mslBank* bank;
    _GameCubeFileEntry* file_entry;
    SoundBufferPlayableLayout* next;
    SoundBufferPlayableLayout* previous;
    u32 current_position;
    u32 mix_fader;
    unsigned char pan;
    unsigned char volume;
    unsigned char pad26[2];
    u32 frequency;
    s32 state;
    _AXVPB* voices[2];
};

typedef char SoundBufferPlayableLayoutSize[
    sizeof(SoundBufferPlayableLayout) == 0x38 ? 1 : -1];

struct SBPlayableStreamLayout {
    SoundBufferPlayableLayout playable;
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

typedef char SBPlayableStreamLayoutSize[
    sizeof(SBPlayableStreamLayout) == 0xCC ? 1 : -1];

static const unsigned char SurroundPanTable[] = {
    0x40, 0x10, 0x10, 0x40, 0x08, 0x78,
    0x20, 0x7C, 0x40, 0x7C, 0x60, 0x7C,
    0x77, 0x78, 0x70, 0x40, 0x40, 0x10
};

SoundBufferUpdateList ms_UpdateList__20SoundBuffer_Playable = {0, 0};
extern unsigned char __vt__10IRefCntRes[];
extern unsigned char __vt__11SoundBuffer[];
extern unsigned char __vt__16SoundBuffer_Data[];
extern unsigned char __vt__20SoundBuffer_Playable[];
extern unsigned char __vt__17SBPlayable_Stream[];

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

inline void SBPlayable_Stream::iAX_GetVoiceBlock(
    _AXVPB* voice,
    int channel,
    int& block,
    u32* raw_address) {
    SBPlayable_Stream* stream =
        this;
    u32 address = i_AXVPB_GetVoice_CurrentPosition(voice);

    if (address >= g_MSL_GCN_ARAM_ZeroBase_ADPCM_Start &&
        address <= g_MSL_GCN_ARAM_ZeroBase_ADPCM_End) {
        block = -1;
    } else {
        address = ((s32)address >> 1) -
            stream->cache_buffers[channel];
        block = (s32)address >> stream->segment_shift;
        if (block < 0 || block >= stream->ring_block_count) {
            _MSL_GCN_BREAK();
        }
    }
    if (raw_address != 0) {
        *raw_address = address;
    }
}

SBPlayable_Stream::~SBPlayable_Stream() {
    FreeResources();
}

void SBPlayable_Stream::FreeObject(void) {
    SoundBuffer_Playable* self = this;
    BOOL enabled;

    this->FreeResources();
    enabled = OSDisableInterrupts();
    if (self->previous != 0) {
        if ((self->previous->next = self->next) != 0) {
            self->next->previous = self->previous;
        } else {
            ms_UpdateList__20SoundBuffer_Playable.last =
                self->previous;
        }
    } else {
        ms_UpdateList__20SoundBuffer_Playable.first =
            self->next;
        if (ms_UpdateList__20SoundBuffer_Playable.first != 0) {
            self->next->previous = 0;
        } else {
            ms_UpdateList__20SoundBuffer_Playable.last = 0;
        }
    }
    OSRestoreInterrupts(enabled);
    delete this;
}

void SBPlayable_Stream::ResetValues(void) {
    SBPlayable_Stream* self =
        this;

    self->ready_to_play = 0;
    self->last_read_pending = 0;
    self->voices_started = 0;
    self->play_when_ready = 0;
    self->play_flags = 0;
    self->cache_buffers[0] = 0;
    self->cache_buffers[1] = 0;
    self->cache_buffer_size = 0;
    self->cache_buffer1_size = 0;
    self->stream_context = 0;
    self->aligned_size = 0;
    self->primary_offset = 0;
    self->segment_size = 0;
    self->segment_shift = 0;
    self->source_read_offset = 0;
    self->source_bytes_remaining = 0;
    self->pending_file_request = 0;
    self->queued_read_offset = 0;
    self->queued_read_size = 0;
    self->queued_block_count = 0;
    self->partial_read = 0;
    self->at_zero_buffer = 0;
    self->crossed_stream_end = 0;
    self->end_of_stream = 0;
    self->pending_arq_count = 0;
    self->ring_block_count = 0;
    self->source_blocks_remaining = 0;
    self->ring_write_block = 0;
    self->ring_play_block = 0;
    self->pending_ax_block = -1;
    self->ax_end_block = -1;
    self->end_pass_count = 0;
    self->stream_end_block = -1;
    self->initial_position = 0;
    self->ring_scan_stop_block = -1;
}

void SBPlayable_Stream::FreeResources(void) {
    SoundBuffer_Playable* self = this;

    this->Stop();
    this->Stop();
    if (self->aram_block != 0) {
        self->aram_block->Release();
        self->aram_block = 0;
    }
}

void SBPlayable_Stream::PrepForPlay(void) {
    SBPlayable_Stream* stream =
        this;

    while (stream->pending_arq_count != 0) {
    }

    BOOL enabled = OSDisableInterrupts();
    if (stream->ready_to_play == 0 &&
        stream->last_read_pending == 0) {
        do {
        stream->cache_buffers[0] =
            mslStreamCache_GetStreamBuffer();
        if (stream->cache_buffers[0] == 0) {
            BOOL cleanup_enabled = OSDisableInterrupts();

            stream->ready_to_play = 0;
            stream->last_read_pending = 0;
            stream->play_when_ready = 0;
            if (stream->cache_buffers[0] != 0) {
                mslStreamCache_ReleaseBuffer(
                    stream->cache_buffers[0]);
            }
            if (stream->cache_buffers[1] != 0) {
                mslStreamCache_ReleaseBuffer(
                    stream->cache_buffers[1]);
            }
            stream->cache_buffers[0] = 0;
            stream->cache_buffers[1] = 0;
            stream->cache_buffer_size = 0;
            stream->cache_buffer1_size = 0;
            if (stream->pending_file_request != 0) {
                mslStreamFile_CancelRequest(
                    stream->pending_file_request);
                stream->pending_file_request = 0;
            }
            OSRestoreInterrupts(cleanup_enabled);
            break;
        } else {
            stream->cache_buffer_size =
                mslStreamCache_GetSizeBuffer();
            if (stream->file_entry->has_secondary != 0) {
                stream->ring_block_count =
                    stream->cache_buffer_size / 0x2000;
                stream->cache_buffers[1] =
                    mslStreamCache_GetStreamBuffer();
                if (stream->cache_buffers[1] == 0) {
                    BOOL cleanup_enabled =
                        OSDisableInterrupts();

                    stream->ready_to_play = 0;
                    stream->last_read_pending = 0;
                    stream->play_when_ready = 0;
                    if (stream->cache_buffers[0] != 0) {
                        mslStreamCache_ReleaseBuffer(
                            stream->cache_buffers[0]);
                    }
                    if (stream->cache_buffers[1] != 0) {
                        mslStreamCache_ReleaseBuffer(
                            stream->cache_buffers[1]);
                    }
                    stream->cache_buffers[0] = 0;
                    stream->cache_buffers[1] = 0;
                    stream->cache_buffer_size = 0;
                    stream->cache_buffer1_size = 0;
                    if (stream->pending_file_request != 0) {
                        mslStreamFile_CancelRequest(
                            stream->pending_file_request);
                        stream->pending_file_request = 0;
                    }
                    OSRestoreInterrupts(cleanup_enabled);
                    break;
                } else {
                    stream->cache_buffer1_size =
                        stream->cache_buffer_size;
                }
            } else {
                stream->ring_block_count =
                    stream->cache_buffer_size / 0x4000;
            }

            if (stream->cache_buffer_size >=
                stream->file_entry->aram_size) {
                stream->source_blocks_remaining = -1;
            } else {
                stream->source_blocks_remaining =
                    stream->ring_block_count;
            }
            stream->ring_write_block = 0;
            stream->ring_play_block = 0;
            stream->source_read_offset = 0;
            stream->source_bytes_remaining =
                stream->aligned_size;
            stream->last_read_pending = 1;
        }
        } while (0);
    }
    OSRestoreInterrupts(enabled);
}

int SBPlayable_Stream::IsReadyToPlay(void) {
    SBPlayable_Stream* self =
        this;

    return self->ready_to_play;
}

int SBPlayable_Stream::Play(u32 flags) {
    SBPlayable_Stream* stream =
        this;
    SoundBuffer_Playable* self = stream;
    int result = -1;

    if ((self->state & 2) == 0) {
        BOOL enabled;

        stream->play_flags = flags;
        this->PrepForPlay();
        enabled = OSDisableInterrupts();
        if ((flags & 1) != 0) {
            self->state = 3;
        } else {
            self->state = 2;
        }
        stream->play_flags = flags;
        if (stream->ready_to_play != 0) {
            result = iPlayPrepped();
        } else if (stream->last_read_pending != 0) {
            stream->play_when_ready = 1;
            result = 0;
        }
        OSRestoreInterrupts(enabled);
    }
    return result;
}

/* TODO: [near miss] 97.00%; named result blocks two shared null zeros;
 * recover result/outer critical-section ownership without changing voice teardown. */
int SBPlayable_Stream::Stop(void) {
    BOOL enabled = OSDisableInterrupts();
    int result = -1;
    BOOL voice_enabled = OSDisableInterrupts();

    if (this->voices[0] != 0) {
        AXSetVoiceState(this->voices[0], 0);
        MIXReleaseChannel(this->voices[0]);
        AXFreeVoice(this->voices[0]);
        result = 0;
        this->voices[0] = 0;
    }
    if (this->voices[1] != 0) {
        AXSetVoiceState(this->voices[1], 0);
        MIXReleaseChannel(this->voices[1]);
        AXFreeVoice(this->voices[1]);
        result = 0;
        this->voices[1] = 0;
    }
    OSRestoreInterrupts(voice_enabled);
    this->state = 8;
    this->voices_started = 0;

    voice_enabled = OSDisableInterrupts();
    this->ready_to_play = 0;
    this->last_read_pending = 0;
    this->play_when_ready = 0;
    if (this->cache_buffers[0] != 0) {
        mslStreamCache_ReleaseBuffer(this->cache_buffers[0]);
    }
    if (this->cache_buffers[1] != 0) {
        mslStreamCache_ReleaseBuffer(this->cache_buffers[1]);
    }
    this->cache_buffers[0] = 0;
    this->cache_buffers[1] = 0;
    this->cache_buffer_size = 0;
    this->cache_buffer1_size = 0;
    if (this->pending_file_request != 0) {
        mslStreamFile_CancelRequest(this->pending_file_request);
        this->pending_file_request = 0;
    }
    OSRestoreInterrupts(voice_enabled);
    OSRestoreInterrupts(enabled);
    return result;
}

int SBPlayable_Stream::Pause(void) {
    SBPlayable_Stream* stream =
        this;
    SoundBuffer_Playable* self = stream;
    BOOL voice_enabled;
    int result;
    BOOL enabled = OSDisableInterrupts();
    result = -1;

    this->StopIfDonePlaying();
    if ((self->state & 2) != 0) {
        voice_enabled = OSDisableInterrupts();

        if (self->voices[0] != 0) {
            AXSetVoiceState(self->voices[0], 0);
            result = 0;
        }
        if (self->voices[1] != 0) {
            AXSetVoiceState(self->voices[1], 0);
            result = 0;
        }
        OSRestoreInterrupts(voice_enabled);
        self->state = (self->state & 1) | 4;
    }
    stream->voices_started = 0;
    stream->play_when_ready = 0;
    OSRestoreInterrupts(enabled);
    return result;
}

void SBPlayable_Stream::iUpdate_AXUser(void) {
    SBPlayable_Stream* stream = this;
    int primary_block;
    int secondary_block;
    int channels;
    int crossed_scan_stop;
    s32 next_block;
    u32 primary_raw_address;

    if (stream->voices_started == 0) {
        return;
    }
    if (stream->source_blocks_remaining == -1) {
        return;
    }

    iAX_GetVoiceBlock(
        stream->voices[0], 0, primary_block,
        &primary_raw_address);

    if (stream->file_entry->has_secondary != 0) {
        u32 secondary_raw_address;

        channels = 2;
        iAX_GetVoiceBlock(
            stream->voices[1], 1, secondary_block,
            &secondary_raw_address);
    } else {
        channels = 1;
    }
    if (channels != 1 && secondary_block != primary_block) {
        return;
    }

    if (primary_block == -1) {
        if (stream->end_of_stream != 0 &&
            stream->stream_end_block == stream->ax_end_block) {
            if (stream->end_pass_count == 0) {
                stream->end_pass_count = 1;
            }
        } else {
            stream->crossed_stream_end = 1;
            if (stream->ring_play_block ==
                stream->stream_end_block - 1) {
                stream->ring_play_block =
                    stream->stream_end_block;
            }

            next_block = stream->ax_end_block + 1;
            if (next_block >= stream->ring_block_count) {
                next_block = 0;
            }
            crossed_scan_stop = 0;
            if (stream->ring_scan_stop_block == next_block) {
                next_block++;
                crossed_scan_stop = 1;
                if (next_block >= stream->ring_block_count) {
                    next_block = 0;
                }
            }
            if (next_block != stream->ring_write_block) {
                if (crossed_scan_stop != 0) {
                    stream->ring_scan_stop_block = -1;
                }
                while (stream->ring_play_block != next_block) {
                    stream->source_blocks_remaining++;
                    stream->ring_play_block++;
                    if (stream->ring_play_block >=
                        stream->ring_block_count) {
                        stream->ring_play_block = 0;
                    }
                }

                u32 block_address =
                    (stream->ring_play_block <<
                     (stream->segment_shift + 1)) + 2;

                i_AXVPB_SetVoice_CurrentPosition(stream->voices[0],
                    block_address + (stream->cache_buffers[0] << 1));
                i_AXVPB_SetVoice_ADPCM_StartPredScale(stream->voices[0],
                    stream->block_headers[0][stream->ring_play_block]);

                if (stream->voices[1] != 0) {
                    i_AXVPB_SetVoice_CurrentPosition(stream->voices[1],
                        block_address + (stream->cache_buffers[1] << 1));
                    i_AXVPB_SetVoice_ADPCM_StartPredScale(stream->voices[1],
                        stream->block_headers[1][stream->ring_play_block]);
                }

                iAX_FindNewEndBlock(next_block);
                AXSetVoiceState(
                    stream->voices[0], 1);
                if (stream->voices[1] != 0) {
                    AXSetVoiceState(
                        stream->voices[1], 1);
                }
            }
        }
    }

    if (primary_block == -1) {
        stream->at_zero_buffer = 1;
        return;
    }
    stream->at_zero_buffer = 0;

    if (stream->pending_ax_block == -1) {
        next_block = stream->ax_end_block + 1;
        if (next_block >= stream->ring_block_count) {
            next_block = 0;
        }
        crossed_scan_stop = 0;
        if (stream->ring_scan_stop_block == next_block) {
            next_block++;
            crossed_scan_stop = 1;
            if (next_block >= stream->ring_block_count) {
                next_block = 0;
            }
        }
        if (next_block != stream->ring_write_block) {
            if (crossed_scan_stop != 0) {
                stream->ring_scan_stop_block = -1;
            }
            stream->pending_ax_block = next_block;

            u32 block_address =
                (next_block <<
                 (stream->segment_shift + 1)) + 2;
            i_AXVPB_SetVoice_LoopPosition_PredScale(stream->voices[0],
                block_address + (stream->cache_buffers[0] << 1),
                stream->block_headers[0][next_block],
                stream->ax_end_block == stream->stream_end_block);
            if (stream->voices[1] != 0) {
                i_AXVPB_SetVoice_LoopPosition_PredScale(stream->voices[1],
                    block_address + (stream->cache_buffers[1] << 1),
                    stream->block_headers[1][next_block],
                    stream->ax_end_block == stream->stream_end_block);
            }
        }
    }

    if (primary_block < stream->ring_play_block ||
        primary_block > stream->ax_end_block) {
        if (stream->ax_end_block ==
            stream->stream_end_block) {
            stream->end_pass_count++;
            stream->stream_end_block = -1;
        }
        iAX_FindNewEndBlock(primary_block);
    }

    while (stream->ring_play_block != primary_block) {
        stream->source_blocks_remaining++;
        stream->ring_play_block++;
        if (stream->ring_play_block >=
            stream->ring_block_count) {
            stream->ring_play_block = 0;
        }
    }
}

void SBPlayable_Stream::iAX_FindNewEndBlock(int block) {
    SBPlayable_Stream* stream =
        this;
    int scan_stop = stream->ring_scan_stop_block;
    int end_block;

    do {
        end_block = block;
        block++;
    } while (block != scan_stop &&
             block < stream->ring_block_count &&
             block != stream->ring_write_block &&
             end_block != stream->stream_end_block);

    if (stream->ax_end_block == end_block) {
        return;
    }
    stream->ax_end_block = end_block;

    u32 end_address;
    if (end_block == stream->stream_end_block) {
        end_address =
            stream->initial_position +
            (end_block << (stream->segment_shift + 1));
    } else {
        end_address =
            ((end_block + 1) <<
             (stream->segment_shift + 1)) - 1;
    }
    stream->pending_ax_block = -1;

    _AXVPB* voice = stream->voices[0];

    i_AXVPB_SetVoice_EndPosition(
        voice, end_address + (stream->cache_buffers[0] << 1));
    i_AXVPB_SetVoice_Loop_To_Zero(voice);

    voice = stream->voices[1];
    if (voice == 0) {
        return;
    }
    i_AXVPB_SetVoice_EndPosition(
        voice, end_address + (stream->cache_buffers[1] << 1));
    i_AXVPB_SetVoice_Loop_To_Zero(voice);
}

void SBPlayable_Stream::iUpdate_MslTick(void) {
    SBPlayable_Stream* stream =
        this;
    SoundBuffer_Playable* self = stream;

    if (stream->ready_to_play != 0 ||
        stream->last_read_pending != 0) {
        if (stream->source_bytes_remaining == 0 &&
            stream->source_blocks_remaining != -1) {
            if ((self->state & 1) != 0) {
                stream->source_read_offset = 0;
                stream->source_bytes_remaining =
                    stream->aligned_size;
            } else {
                stream->end_of_stream = 1;
            }
        }

        this->StopIfDonePlaying();

        if ((stream->source_blocks_remaining > 0 ||
             stream->source_blocks_remaining == -1) &&
            stream->pending_arq_count == 0 &&
            stream->source_bytes_remaining != 0 &&
            stream->pending_file_request == 0) {
            s32 block_count;
            s32 read_size;
            s32 bytes_remaining;
            int cannot_read = 0;

            stream->partial_read = 0;
            if (stream->source_blocks_remaining == -1) {
                read_size = stream->source_bytes_remaining;
            } else {
                bytes_remaining =
                    stream->source_bytes_remaining;
                read_size =
                    ((bytes_remaining + 0x3FFF) >> 14) - 2;
                block_count = stream->source_blocks_remaining;
                if (block_count > read_size) {
                    block_count = read_size;
                }
                if (block_count > 0) {
                    if ((stream->last_read_pending != 0 ||
                         stream->at_zero_buffer != 0) &&
                        block_count > 3) {
                        block_count = 3;
                    }
                    read_size = block_count << 14;
                } else if (block_count == 0) {
                    if (stream->ring_write_block ==
                        stream->ring_block_count - 1) {
                        stream->ring_write_block = 0;
                        stream->ring_scan_stop_block =
                            stream->ring_block_count - 1;
                        stream->source_blocks_remaining--;
                    }
                    if (stream->source_blocks_remaining < 2) {
                        cannot_read = 1;
                    } else {
                        read_size = 0x8000;
                        if (bytes_remaining <= read_size) {
                            read_size = bytes_remaining;
                        }
                        stream->stream_end_block =
                            stream->ring_write_block + 1;
                    }
                } else {
                    cannot_read = 1;
                }
            }

            if (cannot_read == 0) {
                block_count =
                    (read_size + 0x3FFF) >> 14;
                if (read_size != block_count << 14) {
                    stream->partial_read = 1;
                }
                stream->queued_read_offset =
                    stream->source_read_offset;
                stream->queued_read_size = read_size;
                stream->queued_block_count = block_count;
                stream->pending_file_request =
                    mslStreamFile_QueueRequest(
                        stream->stream_context,
                        stream->primary_offset +
                            stream->queued_read_offset,
                        stream->queued_read_size,
                        stream->last_read_pending != 0
                            ? 0x10
                            : 0x10,
                        StreamFileRead_CallBack, stream);
            }
        }
    }

    if (stream->ready_to_play != 0 &&
        stream->play_when_ready != 0) {
        stream->play_when_ready = 0;
        iPlayPrepped();
    }
}

void SBPlayable_Stream::StopIfDonePlaying(void) {
    SBPlayable_Stream* stream =
        this;
    SoundBuffer_Playable* self = stream;

    if ((self->state & 2) != 0) {
        if (stream->source_blocks_remaining == -1) {
            if (stream->voices_started != 0 &&
                (self->state & 1) == 0) {
                int done = 0;

                if (self->voices[0] != 0 &&
                    (self->voices[0]->pb.state & 1) == 0) {
                    done = 1;
                } else if (self->voices[1] != 0 &&
                           (self->voices[1]->pb.state & 1) == 0) {
                    done = 1;
                }
                if (done != 0) {
                    this->Stop();
                }
            }
        } else if (stream->end_of_stream != 0 &&
                   stream->at_zero_buffer != 0 &&
                   stream->stream_end_block ==
                       stream->ax_end_block &&
                   (stream->ring_play_block ==
                        stream->stream_end_block ||
                    stream->ring_play_block ==
                        stream->stream_end_block - 1)) {
            this->Stop();
        }
    }
}

int SBPlayable_Stream::iPlayPrepped(void) {
    int result = -1;

    if (ready_to_play == 0 ||
        voices_started != 0) {
        return result;
    }

    if (voices[0] == 0) {
        int use_stream_loop;
        int use_loop_address;

        ring_play_block = 0;
        at_zero_buffer = 0;
        voices[0] = AXAcquireVoice(
            0x14, AcquireVoiceCallback, (uptr)this);
        if (voices[0] == 0) {
            return result;
        }
        AXSetVoicePriority(voices[0], 0x19);

        use_stream_loop = 1;
        use_loop_address = 1;
        if (source_blocks_remaining == -1) {
            use_stream_loop = 0;
            if ((state & 1) == 0) {
                use_loop_address = 0;
            }
        }

        if (file_entry->has_secondary != 0) {
            voices[1] = AXAcquireVoice(
                0x14, AcquireVoiceCallback, (uptr)this);
            if (voices[1] == 0) {
                AXFreeVoice(voices[0]);
                voices[0] = 0;
                return result;
            }
            AXSetVoicePriority(voices[1], 0x19);
            SPSoundEntry* sound = GetSoundEntryLeft();
            InitializeVoice(
                voices[0], sound, use_stream_loop,
                use_loop_address);
            sound = GetSoundEntryRight();
            InitializeVoice(
                voices[1], sound, use_stream_loop,
                use_loop_address);
            MIXInitChannel(
                voices[0], 0, 0, -960, -960,
                0, 0x7F, mix_fader);
            MIXInitChannel(
                voices[1], 0, 0, -960, -960,
                0x7F, 0x7F, mix_fader);
        } else {
            SPSoundEntry* sound = GetSoundEntryLeft();
            InitializeVoice(
                voices[0], sound, use_stream_loop,
                use_loop_address);
            MIXInitChannel(
                voices[0], 0, 0, -960, -960,
                pan, volume, mix_fader);
        }

        if (source_blocks_remaining == -1) {
            int channel;

            ring_play_block = 0;
            if (ring_write_block != 0) {
                ax_end_block =
                    ring_write_block - 1;
            } else {
                ax_end_block =
                    ring_block_count - 1;
            }
            if (use_loop_address != 0) {
                pending_ax_block = 0;
            } else {
                pending_ax_block = -1;
            }

            for (channel = 0;
                 channel < GetNumChannels(); channel++) {
                _AXVPB* voice =
                    voices[channel];
                u32 base =
                    cache_buffers[channel] << 1;

                i_AXVPB_SetVoice_CurrentPosition(
                    voice, base + i_AXVPB_GetVoice_CurrentPosition(voice));
                i_AXVPB_SetVoice_EndPosition(
                    voice, base + i_AXVPB_GetVoice_EndPosition(voice));
                if (use_loop_address) {
                    i_AXVPB_SetVoice_LoopPosition(
                        voice, base + i_AXVPB_GetVoice_LoopPosition(voice));
                }
            }
        } else {
            int channel;

            ax_end_block =
                ring_play_block;
            pending_ax_block = -1;
            for (channel = 0;
                 channel < GetNumChannels(); channel++) {
                _AXVPB* voice =
                    voices[channel];
                u32 base =
                    cache_buffers[channel] << 1;

                i_AXVPB_SetVoice_CurrentPosition(voice, base + 2);
                i_AXVPB_SetVoice_EndPosition(
                    voice, base + (segment_size << 1) - 1);
                i_AXVPB_SetVoice_Loop_To_Zero(voice);
            }
        }
    }

    if (voices[0] != 0) {
        voices_started = 1;
        crossed_stream_end = 0;
        end_of_stream = 0;
        if (at_zero_buffer == 0) {
            AXSetVoiceState(voices[0], 1);
            if (voices[1] != 0) {
                AXSetVoiceState(voices[1], 1);
            }
        }
        result = 0;
    }
    return result;
}

void SBPlayable_Stream::i_ARQCALLBACK_ArqComplete(
    u32 request_address) {
    mslARQRequest* request = (mslARQRequest*)request_address;
    SBPlayable_Stream* stream =
        (SBPlayable_Stream*)request->callback_data;

    stream->pending_arq_count--;
    i_ARQCALLBACK_ReturnArq(request_address);
}

void SBPlayable_Stream::i_ARQCALLBACK_ArqComplete_ReturnBuffer(
    u32 request_address) {
    mslARQRequest* request = (mslARQRequest*)request_address;
    SBPlayable_Stream* stream =
        (SBPlayable_Stream*)request->callback_data;

    stream->pending_arq_count--;
    i_ARQCALLBACK_ReturnArqAndUserStreamBuffer(request_address);
}

void SBPlayable_Stream::i_ARQCALLBACK_ArqComplete_LastRead(
    u32 request_address) {
    mslARQRequest* request = (mslARQRequest*)request_address;
    SBPlayable_Stream* stream =
        (SBPlayable_Stream*)request->callback_data;

    stream->pending_arq_count--;
    i_ARQCALLBACK_ReturnArqAndUserStreamBuffer(request_address);
    if (stream->last_read_pending != 0) {
        stream->last_read_pending = 0;
        stream->ready_to_play = 1;
    }
    if (stream->ready_to_play != 0) {
        stream->source_read_offset += stream->queued_read_size;
        stream->source_bytes_remaining -= stream->queued_read_size;
        if (stream->source_blocks_remaining > 0) {
            stream->source_blocks_remaining -=
                stream->queued_block_count;
        }
        stream->ring_write_block += stream->queued_block_count;
        if (stream->ring_write_block >= stream->ring_block_count) {
            stream->ring_write_block -= stream->ring_block_count;
        }
    }
}

void SBPlayable_Stream::StreamFileRead_CallBack(
    void* buffer, u32 offset, int size, int error,
    int final_chunk, void* callback_data) {
    SBPlayable_Stream* stream;
    ARQCallback callback;
    ARQCallback last_callback;
    BOOL enabled;

    enabled = OSDisableInterrupts();
    stream = (SBPlayable_Stream*)callback_data;

    if (error == 0) {
        if (final_chunk != 0) {
            stream->pending_file_request = 0;
            last_callback = i_ARQCALLBACK_ArqComplete_LastRead;
        } else {
            last_callback = i_ARQCALLBACK_ArqComplete_ReturnBuffer;
        }
        callback = i_ARQCALLBACK_ArqComplete;

        if (size == 0) {
            mslStreamFile_ReturnBuffer(buffer);
        } else {
            unsigned char* cursor;
            mslARQRequest* request;
            int remaining;
            s32 segment = (s32)offset >> stream->segment_shift;
            s32 channel;
            s32 block;
            s32 segment_size = stream->segment_size;
            s32 in_segment;
            s32 segment_remaining;

            remaining = size;
            cursor = (unsigned char*)buffer;
            if (stream->file_entry->has_secondary != 0) {
                channel = segment & 1;
                block = stream->ring_write_block + (segment >> 1);
            } else {
                channel = 0;
                block = stream->ring_write_block + segment;
            }
            in_segment = offset - segment * segment_size;
            segment_remaining = segment_size - in_segment;

            while (remaining != 0) {
                request = mslGetArqRequest();

                if (request == 0) {
                    _MSL_GCN_BREAK();
                    mslStreamFile_ReturnBuffer(buffer);
                    remaining = 0;
                    ((SoundBuffer_Playable*)stream)->Stop();
                } else {
                    u32 destination;

                    stream->pending_arq_count++;
                    request->callback_data = stream;
                    request->stream_buffer = 0;
                    if (block >= stream->ring_block_count) {
                        block -= stream->ring_block_count;
                    }
                    if (segment_remaining > remaining) {
                        segment_remaining = remaining;
                    }
                    remaining -= segment_remaining;
                    if (remaining == 0) {
                        request->stream_buffer = buffer;
                        callback = last_callback;
                        if (stream->partial_read != 0) {
                            segment_remaining =
                                (segment_remaining + 0x1F) & ~0x1FU;
                        }
                    }
                    if (in_segment == 0) {
                        stream->block_headers[channel][block] = *cursor;
                    }
                    destination =
                        stream->cache_buffers[channel] +
                        block * segment_size + in_segment;
                    DCFlushRange(cursor, segment_remaining);
                    ARQPostRequest(
                        &request->request, 0, 0, 1, (uptr)cursor,
                        destination, segment_remaining, callback);
                    cursor += segment_remaining;
                    if (remaining != 0) {
                        if (stream->file_entry->has_secondary != 0) {
                            channel ^= 1;
                            if (channel == 0) {
                                block++;
                            }
                        } else {
                            block++;
                        }
                        in_segment = 0;
                        segment_remaining = segment_size;
                    }
                }
            }
        }
    } else {
        if (error == 0x44DEAD0F) {
            stream->pending_file_request = 0;
        } else {
            _MSL_GCN_BREAK();
            stream->pending_file_request = 0;
        }
    }
    OSRestoreInterrupts(enabled);
}

__declspec(weak) void IRefCntRes::FreeResources(void) {
}

__declspec(weak) IRefCntRes::~IRefCntRes() {
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

SoundBuffer_Data::~SoundBuffer_Data() {
    FreeResources();
}

void SoundBuffer_Data::FreeObject(void) {
    this->FreeResources();
    delete this;
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

SoundBuffer_Playable::~SoundBuffer_Playable() {
    FreeResources();
}

void SoundBuffer_Playable::FreeObject(void) {
    SoundBuffer_Playable* self = this;
    BOOL enabled;

    this->FreeResources();
    enabled = OSDisableInterrupts();
    if (self->previous != 0) {
        if ((self->previous->next = self->next) != 0) {
            self->next->previous = self->previous;
        } else {
            ms_UpdateList__20SoundBuffer_Playable.last =
                self->previous;
        }
    } else {
        ms_UpdateList__20SoundBuffer_Playable.first =
            self->next;
        if (ms_UpdateList__20SoundBuffer_Playable.first != 0) {
            self->next->previous = 0;
        } else {
            ms_UpdateList__20SoundBuffer_Playable.last = 0;
        }
    }
    OSRestoreInterrupts(enabled);
    delete this;
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
        result = 0;
        self->voices[0] = 0;
    }
    if (self->voices[1] != 0) {
        AXSetVoiceState(self->voices[1], 0);
        MIXReleaseChannel(self->voices[1]);
        AXFreeVoice(self->voices[1]);
        result = 0;
        self->voices[1] = 0;
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

static SoundBuffer_Data* fn_stub1(void) {
    return (SoundBuffer_Data*)operator new(
        sizeof(SoundBuffer_Data), (_mwMemHeap*)MWSOUND_HEAP, (mwMemFlags)0x10,
        "SoundBuffer_Data", 0, 0);
}

SoundBuffer_Playable* SoundBuffer::CreatePlayableStaticBuffer(
    _mslBank* bank, _GameCubeFileEntry* entry) {
    SoundBufferPlayableLayout* playable;
    BOOL enabled = OSDisableInterrupts();
    playable =
        (SoundBufferPlayableLayout*)
            operator new(
                sizeof(SoundBufferPlayableLayout), (_mwMemHeap*)MWSOUND_HEAP,
                (mwMemFlags)0x10,
                "SoundBuffer_Playable", 0, 0);

    if (playable != 0) {
        playable->vtable =
            (SoundBufferPlayableVTable*)__vt__10IRefCntRes;
        playable->reference_count = 1;
        playable->vtable =
            (SoundBufferPlayableVTable*)__vt__11SoundBuffer;
        playable->vtable =
            (SoundBufferPlayableVTable*)__vt__16SoundBuffer_Data;
        playable->aram_block = 0;
        playable->bank = 0;
        playable->file_entry = 0;
        playable->vtable =
            (SoundBufferPlayableVTable*)__vt__20SoundBuffer_Playable;
        playable->current_position = 0;
        playable->mix_fader = 0;
        playable->pan = 0x40;
        playable->volume = 0x7F;
        playable->frequency = 0;
        playable->state = 8;
        playable->voices[0] = 0;
        playable->voices[1] = 0;
    }

    BOOL list_enabled = OSDisableInterrupts();
    if ((playable->previous =
             (SoundBufferPlayableLayout*)
                 ms_UpdateList__20SoundBuffer_Playable.last) != 0) {
        ((SoundBufferPlayableLayout*)
            ms_UpdateList__20SoundBuffer_Playable.last)->next = playable;
    } else {
        ms_UpdateList__20SoundBuffer_Playable.first =
            (SoundBuffer_Playable*)playable;
    }
    ms_UpdateList__20SoundBuffer_Playable.last =
        (SoundBuffer_Playable*)playable;
    playable->next = 0;
    OSRestoreInterrupts(list_enabled);

    if (playable == 0) {
        playable = 0;
    } else {
        MSLGCN_ARamBlock* aram_block;

        playable->bank = bank;
        playable->file_entry = entry;
        aram_block = MSLGCN_ARamBlock::GetObject();
        if (aram_block != 0) {
            playable->aram_block = aram_block;
            u32 aram_size;
            u32 primary_offset;
            u32 secondary_offset;
            int channel_count;

            aram_block->SetParent(bank->resident_aram_block);
            primary_offset = entry->primary_aram_offset;
            aram_size = entry->aram_size;
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
        } else {
            if (--playable->reference_count == 0) {
                ((SoundBuffer_Playable*)playable)->FreeObject();
            }
            playable = 0;
        }
    }

    OSRestoreInterrupts(enabled);
    return (SoundBuffer_Playable*)playable;
}

SoundBuffer_Playable* SoundBuffer::CreatePlayableStreamBuffer(
    _mslBank* bank, _GameCubeFileEntry* entry) {
    SBPlayableStreamLayout* stream;
    BOOL enabled = OSDisableInterrupts();
    stream =
        (SBPlayableStreamLayout*)
            operator new(
                sizeof(SBPlayableStreamLayout), (_mwMemHeap*)MWSOUND_HEAP,
                (mwMemFlags)0x10,
                "SBPlayable_Stream", 0, 0);

    if (stream != 0) {
        SoundBufferPlayableLayout* playable = &stream->playable;
        playable->vtable =
            (SoundBufferPlayableVTable*)__vt__10IRefCntRes;
        playable->reference_count = 1;
        playable->vtable =
            (SoundBufferPlayableVTable*)__vt__11SoundBuffer;
        playable->vtable =
            (SoundBufferPlayableVTable*)__vt__16SoundBuffer_Data;
        playable->aram_block = 0;
        playable->bank = 0;
        playable->file_entry = 0;
        playable->vtable =
            (SoundBufferPlayableVTable*)__vt__20SoundBuffer_Playable;
        playable->current_position = 0;
        playable->mix_fader = 0;
        playable->pan = 0x40;
        playable->volume = 0x7F;
        playable->frequency = 0;
        playable->state = 8;
        playable->voices[0] = 0;
        playable->voices[1] = 0;
        playable->vtable =
            (SoundBufferPlayableVTable*)__vt__17SBPlayable_Stream;
        ((SBPlayable_Stream*)stream)->ResetValues();
    }

    BOOL list_enabled = OSDisableInterrupts();
    if ((stream->playable.previous =
             (SoundBufferPlayableLayout*)
                 ms_UpdateList__20SoundBuffer_Playable.last) != 0) {
        ((SoundBufferPlayableLayout*)
            ms_UpdateList__20SoundBuffer_Playable.last)->next =
                &stream->playable;
    } else {
        ms_UpdateList__20SoundBuffer_Playable.first =
            (SoundBuffer_Playable*)stream;
    }
    ms_UpdateList__20SoundBuffer_Playable.last =
        (SoundBuffer_Playable*)stream;
    stream->playable.next = 0;
    OSRestoreInterrupts(list_enabled);

    if (stream != 0) {
        SPSoundEntry* sound;

        stream->playable.bank = bank;
        stream->playable.file_entry = entry;
        stream->stream_context = bank->stream_context;
        stream->primary_offset = entry->primary_aram_offset;
        if (stream->playable.file_entry->has_secondary != 0) {
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
        ((SoundBuffer_Playable*)stream)->PrepForPlay();
    }

    OSRestoreInterrupts(enabled);
    return (SoundBuffer_Playable*)stream;
}

void SoundBuffer::SB_AXUserCallback(void) {
    BOOL enabled = OSDisableInterrupts();
    SoundBuffer_Playable* playable =
        ms_UpdateList__20SoundBuffer_Playable.first;

    while (playable != 0) {
        SoundBuffer_Playable* current = playable;
        playable = playable->next;
        current->iUpdate_AXUser();
    }
    OSRestoreInterrupts(enabled);
}

void SoundBuffer::SB_MslTickCallback(void) {
    BOOL enabled = OSDisableInterrupts();
    SoundBuffer_Playable* playable =
        ms_UpdateList__20SoundBuffer_Playable.first;

    while (playable != 0) {
        SoundBuffer_Playable* current = playable;
        playable = playable->next;
        current->iUpdate_MslTick();
    }
    OSRestoreInterrupts(enabled);
}
