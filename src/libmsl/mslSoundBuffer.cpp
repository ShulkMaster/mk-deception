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
#include "msl/mslWave.h"
#include "msl/mslgcn_globals.h"
#include "msl/mslWave_internal.h"
#include "mw/mwMemNewDelete.h"
#include "runtime/cmath.h"

/*
 * Retail C++ symbols name these as _mslBank and _GameCubeFileEntry. They are
 * narrow SoundBuffer ABI views, not aliases: merging them with mslLoadedBank
 * and mslAssetWave changes MWCC alias analysis in otherwise exact callers.
 */
struct _mslBank {
    unsigned char pad00[0x3C];
    _mwFile* stream_context;      /* +0x3C */
    unsigned char pad40[4];
    MSLGCN_ARamBlock* resident_aram_block; /* +0x44 */
};

struct _GameCubeFileEntry {
    unsigned long name;           /* +0x00 */
    unsigned long primary_aram_offset; /* +0x04 */
    long aram_size;               /* +0x08 */
    SPSoundTable* sound_table;    /* +0x0C */
    unsigned char pad10[8];
    int has_secondary;            /* +0x18 */
    unsigned long secondary_name; /* +0x1C */
    unsigned long secondary_aram_offset; /* +0x20 */
    unsigned long unknown24;
    SPSoundTable* secondary_sound_table; /* +0x28 */
    unsigned long unknown2C;
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
        void* buffer, unsigned long offset, int size, int error,
        int final_chunk, void* callback_data);
    static void i_ARQCALLBACK_ArqComplete_ReturnBuffer(
        unsigned long request_address);
    static void i_ARQCALLBACK_ArqComplete(
        unsigned long request_address);
    static void i_ARQCALLBACK_ArqComplete_LastRead(
        unsigned long request_address);
    virtual ~SBPlayable_Stream();
    virtual void FreeObject(void);
    virtual void FreeResources(void);
    virtual void PrepForPlay(void);
    virtual int IsReadyToPlay(void);
    virtual int Play(unsigned long flags);
    virtual int Stop(void);
    virtual int Pause(void);
    virtual void iUpdate_AXUser(void);
    virtual void iUpdate_MslTick(void);
    virtual void StopIfDonePlaying(void);
    void ResetValues(void);
    int iPlayPrepped(void);
    void iAX_FindNewEndBlock(int block);
    inline int iAX_GetVoiceBlock(
        _AXVPB* voice, int channel,
        unsigned long* raw_address);

    unsigned char ready_to_play;  /* +0x38 */
    unsigned char last_read_pending; /* +0x39 */
    unsigned char voices_started; /* +0x3A */
    unsigned char play_when_ready; /* +0x3B */
    unsigned long play_flags;      /* +0x3C */
    unsigned long cache_buffers[2]; /* +0x40 -- primary, secondary */
    long cache_buffer_size;       /* +0x48 */
    long cache_buffer1_size;      /* +0x4C */
    _mwFile* stream_context;       /* +0x50 */
    unsigned long primary_offset;  /* +0x54 */
    unsigned long aligned_size;    /* +0x58 */
    unsigned long segment_size;    /* +0x5C */
    int segment_shift;             /* +0x60 */
    unsigned long source_read_offset; /* +0x64 */
    unsigned long source_bytes_remaining; /* +0x68 */
    mslStreamFileRequest* pending_file_request; /* +0x6C */
    unsigned long queued_read_offset; /* +0x70 */
    unsigned long queued_read_size;   /* +0x74 */
    unsigned long queued_block_count; /* +0x78 */
    unsigned char partial_read;   /* +0x7C */
    unsigned char at_zero_buffer; /* +0x7D */
    unsigned char crossed_stream_end; /* +0x7E */
    unsigned char end_of_stream;  /* +0x7F */
    volatile long pending_arq_count; /* +0x80 */
    long ring_block_count;            /* +0x84 */
    long source_blocks_remaining;     /* +0x88; -1 sentinel */
    long ring_write_block;            /* +0x8C */
    long ring_play_block;
    long pending_ax_block;         /* +0x94 */
    long ax_end_block;             /* +0x98 */
    long end_pass_count;           /* +0x9C */
    long stream_end_block;         /* +0xA0 */
    unsigned long initial_position;/* +0xA4 */
    long ring_scan_stop_block;     /* +0xA8 */
    unsigned char block_headers[2][0x10]; /* +0xAC -- first byte per ring block */
};

typedef char SBPlayableStreamSize[sizeof(SBPlayable_Stream) == 0xCC ? 1 : -1];

typedef void (*SoundBufferMethod)(SoundBuffer_Playable*);
typedef int (*SoundBufferIntMethod)(SoundBuffer_Playable*);
typedef void (*SoundBufferLostVoiceMethod)(
    SoundBuffer_Playable*, _AXVPB*);
typedef int (*SoundBufferLongMethod)(
    SoundBuffer_Playable*, long);
typedef int (*SoundBufferByteMethod)(
    SoundBuffer_Playable*, unsigned char);
typedef SoundBuffer_Playable* (*SoundBufferDestroyMethod)(
    SoundBuffer_Playable*, short);

struct SoundBufferPlayableVTable {
    unsigned char pad00[8];
    SoundBufferDestroyMethod Destroy; /* +0x08 */
    SoundBufferMethod FreeObject; /* +0x0C */
    SoundBufferMethod FreeResources; /* +0x10 */
    unsigned char pad14[0x18];
    SoundBufferLongMethod SetVolume; /* +0x2C */
    unsigned char pad30[4];
    SoundBufferByteMethod SetPan; /* +0x34 */
    SoundBufferByteMethod SetSurroundPan; /* +0x38 */
    unsigned char pad3C[8];
    SoundBufferIntMethod Stop;    /* +0x44 */
    unsigned char pad48[0x0C];
    SoundBufferLostVoiceMethod LostVoice; /* +0x54 */
    unsigned char pad58[8];
    SoundBufferMethod StopIfDonePlaying;  /* +0x60 */
};

struct SoundBufferPlayableLayout {
    SoundBufferPlayableVTable* vtable; /* +0x00 */
    int reference_count;          /* +0x04 */
    MSLGCN_ARamBlock* aram_block; /* +0x08 */
    _mslBank* bank;               /* +0x0C */
    _GameCubeFileEntry* file_entry; /* +0x10 */
    SoundBufferPlayableLayout* next; /* +0x14 */
    SoundBufferPlayableLayout* previous; /* +0x18 */
    unsigned long current_position; /* +0x1C */
    unsigned long mix_fader;      /* +0x20 */
    unsigned char pan;            /* +0x24 */
    unsigned char volume;         /* +0x25 */
    unsigned char pad26[2];
    unsigned long frequency;      /* +0x28 */
    long state;                   /* +0x2C -- playable state flags */
    _AXVPB* voices[2];             /* +0x30 -- primary, secondary */
};

typedef char SoundBufferPlayableLayoutSize[
    sizeof(SoundBufferPlayableLayout) == 0x38 ? 1 : -1];

struct SBPlayableStreamLayout {
    SoundBufferPlayableLayout playable; /* +0x00 */
    unsigned char ready_to_play;  /* +0x38 */
    unsigned char last_read_pending; /* +0x39 */
    unsigned char voices_started; /* +0x3A */
    unsigned char play_when_ready; /* +0x3B */
    unsigned long play_flags;      /* +0x3C */
    unsigned long cache_buffers[2]; /* +0x40 -- primary, secondary */
    long cache_buffer_size;       /* +0x48 */
    long cache_buffer1_size;      /* +0x4C */
    _mwFile* stream_context;       /* +0x50 */
    unsigned long primary_offset;  /* +0x54 */
    unsigned long aligned_size;    /* +0x58 */
    unsigned long segment_size;    /* +0x5C */
    int segment_shift;             /* +0x60 */
    unsigned long source_read_offset; /* +0x64 */
    unsigned long source_bytes_remaining; /* +0x68 */
    mslStreamFileRequest* pending_file_request; /* +0x6C */
    unsigned long queued_read_offset; /* +0x70 */
    unsigned long queued_read_size;   /* +0x74 */
    unsigned long queued_block_count; /* +0x78 */
    unsigned char partial_read;   /* +0x7C */
    unsigned char at_zero_buffer; /* +0x7D */
    unsigned char crossed_stream_end; /* +0x7E */
    unsigned char end_of_stream;  /* +0x7F */
    volatile long pending_arq_count; /* +0x80 */
    long ring_block_count;            /* +0x84 */
    long source_blocks_remaining;     /* +0x88; -1 sentinel */
    long ring_write_block;            /* +0x8C */
    long ring_play_block;
    long pending_ax_block;         /* +0x94 */
    long ax_end_block;             /* +0x98 */
    long end_pass_count;           /* +0x9C */
    long stream_end_block;         /* +0xA0 */
    unsigned long initial_position;/* +0xA4 */
    long ring_scan_stop_block;     /* +0xA8 */
    unsigned char block_headers[2][0x10]; /* +0xAC -- first byte per ring block */
};

typedef char SBPlayableStreamLayoutSize[
    sizeof(SBPlayableStreamLayout) == 0xCC ? 1 : -1];

static const unsigned char SurroundPanTable[] = {
    0x40, 0x10, 0x10, 0x40, 0x08, 0x78,
    0x20, 0x7C, 0x40, 0x7C, 0x60, 0x7C,
    0x77, 0x78, 0x70, 0x40, 0x40, 0x10
};

static const char stringBase0[] =
    "SBPlayable_Stream\0SoundBuffer_Playable\0SoundBuffer_Data";

extern void* MWSOUND_HEAP;
SoundBufferUpdateList ms_UpdateList__20SoundBuffer_Playable = {0, 0};
extern unsigned char __vt__10IRefCntRes[];
extern unsigned char __vt__11SoundBuffer[];
extern unsigned char __vt__16SoundBuffer_Data[];
extern unsigned char __vt__20SoundBuffer_Playable[];
extern unsigned char __vt__17SBPlayable_Stream[];

/* Matched: 100% report-exact; canonical tick dispatch preserves +0x5C. */
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

/* Matched: 100% report-exact; canonical AX dispatch preserves +0x58. */
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

/* Matched: 100% report-exact; constructor lifetime remains open. Retail MAP
 * separates base/weak/stream code contributions; simple partial linking does not. */
SoundBuffer_Playable* SoundBuffer::CreatePlayableStreamBuffer(
    _mslBank* bank, _GameCubeFileEntry* entry) {
    /* Exact: named TU-local class metadata owns the retail pool label. */
    SBPlayableStreamLayout* stream;
    BOOL enabled = OSDisableInterrupts();
    stream =
        (SBPlayableStreamLayout*)
            operator new(
                sizeof(SBPlayableStreamLayout), (_mwMemHeap*)MWSOUND_HEAP,
                (mwMemFlags)0x10,
                stringBase0, 0, 0);

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

/* Matched: 100% report-exact; typed new still moves destructors/vtables.
 * ARAM helper extraction changes register allocation; retain exact baseline. */
SoundBuffer_Playable* SoundBuffer::CreatePlayableStaticBuffer(
    _mslBank* bank, _GameCubeFileEntry* entry) {
    SoundBufferPlayableLayout* playable;
    BOOL enabled = OSDisableInterrupts();
    playable =
        (SoundBufferPlayableLayout*)
            operator new(
                sizeof(SoundBufferPlayableLayout), (_mwMemHeap*)MWSOUND_HEAP,
                (mwMemFlags)0x10,
                stringBase0 + 0x12, 0, 0);

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
            unsigned long aram_size;
            unsigned long primary_offset;
            unsigned long secondary_offset;
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

/* Matched: 100% report-exact; canonical Stop dispatch. */
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

/* Matched: 100% report-exact; canonical StopIfDonePlaying dispatch. */
void SoundBuffer_Playable::iUpdate_MslTick(void) {
    this->StopIfDonePlaying();
}

void SoundBuffer_Playable::iUpdate_AXUser(void) {
}

/* Matched: 100% report-exact; canonical Stop dispatch for both voices. */
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

/* Matched: 100% report-exact; canonical LostVoice dispatch. */
void SoundBuffer_Playable::AcquireVoiceCallback(void* voice) {
    AXVPB* callback_voice = (AXVPB*)voice;
    SoundBuffer_Playable* owner =
        (SoundBuffer_Playable*)callback_voice->user_context;

    owner->LostVoice(callback_voice);
}

static inline void SetVoiceSource(
    _AXVPB* voice, SPSoundEntry* sound, unsigned long frequency,
    int loop) {
    AXVoiceSrc source;
    unsigned long current;
    unsigned long end;
    unsigned long loop_address;
    unsigned long sync;

    if (frequency == 0) {
        frequency = sound->sample_rate;
    }
    if (frequency < 63) {
        frequency = 63;
    } else if (frequency > 128000) {
        frequency = 128000;
    }

    source.ratioHi = frequency / 32000;
    source.ratioLo = (frequency << 8) / 125;
    source.currentAddressFrac = 0;
    source.last_samples[0] = 0;
    source.last_samples[1] = 0;
    source.last_samples[2] = 0;
    source.last_samples[3] = 0;
    AXSetVoiceSrc(voice, &source);
    AXSetVoiceAdpcm(voice, &sound->adpcm->adpcm);

    sync = voice->sync;
    voice->pb.type = 0;
    sync |= 8;
    voice->sync = sync;
    current =
        loop != 0 ? sound->current_address : sound->loop_address;
    end = sound->end_address;
    loop_address = sound->current_address;
    sync = voice->sync;
    voice->pb.addr.loopFlag = loop;
    voice->pb.addr.format = 0;
    voice->pb.addr.loopAddressHi = current >> 16;
    voice->pb.addr.loopAddressLo = current;
    voice->pb.addr.endAddressHi = end >> 16;
    voice->pb.addr.endAddressLo = end;
    voice->pb.addr.currentAddressHi = loop_address >> 16;
    voice->pb.addr.currentAddressLo = loop_address;
    voice->sync = (sync & 0xFFFE1FFF) | 0x1000;
}

static inline void SetStreamVoiceSource(
    _AXVPB* voice, SPSoundEntry* sound, unsigned long frequency,
    int stream_loop, int use_loop_address) {
    AXVoiceSrc source;
    unsigned long end;
    unsigned long current;
    unsigned long sync;
    unsigned short loop_flag;
    unsigned short loop_address_hi;
    unsigned short loop_address_lo;

    if (frequency == 0) {
        frequency = sound->sample_rate;
    }
    if (frequency < 63) {
        frequency = 63;
    } else if (frequency > 128000) {
        frequency = 128000;
    }

    source.ratioHi = frequency / 32000;
    source.ratioLo = (frequency << 8) / 125;
    source.currentAddressFrac = 0;
    source.last_samples[0] = 0;
    source.last_samples[1] = 0;
    source.last_samples[2] = 0;
    source.last_samples[3] = 0;
    AXSetVoiceSrc(voice, &source);
    AXSetVoiceAdpcm(voice, &sound->adpcm->adpcm);

    if (stream_loop != 0) {
        sync = voice->sync;
        voice->pb.type = 1;
        sync |= 8;
        voice->sync = sync;
    } else {
        sync = voice->sync;
        voice->pb.type = 0;
        sync |= 8;
        voice->sync = sync;
    }
    if (use_loop_address != 0) {
        loop_flag = 1;
        loop_address_hi = sound->current_address >> 16;
        loop_address_lo = sound->current_address;
    } else {
        loop_flag = 0;
        loop_address_hi = sound->loop_address >> 16;
        loop_address_lo = sound->loop_address;
    }
    end = sound->end_address;
    current = sound->current_address;
    sync = voice->sync;
    voice->pb.addr.loopFlag = loop_flag;
    voice->pb.addr.format = 0;
    voice->pb.addr.loopAddressHi = loop_address_hi;
    voice->pb.addr.loopAddressLo = loop_address_lo;
    voice->pb.addr.endAddressHi = end >> 16;
    voice->pb.addr.endAddressLo = end;
    voice->pb.addr.currentAddressHi = current >> 16;
    voice->pb.addr.currentAddressLo = current;
    voice->sync = (sync | 0x1000) & 0xFFFE1FFF;
}

/* TODO: [near miss] 91.26%; source-address halfword extraction, temporary
 * lifetimes and GPR coloring remain; paused resume without voices returns -1. */
int SoundBuffer_Playable::iPlay(
    unsigned long flags, unsigned long acquire_priority,
    unsigned long active_priority) {
    SoundBuffer_Playable* self = this;
    int result = -1;

    if ((self->state & 4) != 0) {
        self->state = (self->state & 1) | 2;
        if (self->voices[0] != 0) {
            AXSetVoiceState(self->voices[0], 1);
            result = 0;
        }
        if (self->voices[1] != 0) {
            AXSetVoiceState(self->voices[1], 1);
            result = 0;
        }
    } else if ((self->state & 8) != 0) {
        self->voices[0] = AXAcquireVoice(
            acquire_priority, AcquireVoiceCallback, (unsigned long)this);
        if (self->voices[0] == 0) {
            return -1;
        }
        if (active_priority != acquire_priority) {
            AXSetVoicePriority(self->voices[0], active_priority);
        }

        if (self->file_entry->has_secondary != 0) {
            self->voices[1] = AXAcquireVoice(
                acquire_priority, AcquireVoiceCallback, (unsigned long)this);
            if (self->voices[1] == 0) {
                AXFreeVoice(self->voices[0]);
                self->voices[0] = 0;
                return -1;
            }
            if (active_priority != acquire_priority) {
                AXSetVoicePriority(self->voices[1], active_priority);
            }
        }

        int loop = 0;
        if ((flags & 1) != 0) {
            loop = 1;
        }
        SPSoundEntry* primary_sound = 0;
        SPSoundEntry* secondary_sound = 0;
        if (self->file_entry->sound_table != 0) {
            primary_sound =
                SPGetSoundEntry(self->file_entry->sound_table, 0);
        }
        SetVoiceSource(
            self->voices[0], primary_sound, self->frequency, loop);

        if (self->file_entry->has_secondary != 0) {
            if (self->file_entry->secondary_sound_table != 0) {
                secondary_sound = SPGetSoundEntry(
                    self->file_entry->secondary_sound_table, 0);
            }
            SetVoiceSource(
                self->voices[1], secondary_sound,
                self->frequency, loop);
            MIXInitChannel(
                self->voices[0], 0, 0, -960, -960,
                0, 0x7F, self->mix_fader);
            MIXInitChannel(
                self->voices[1], 0, 0, -960, -960,
                0x7F, 0x7F, self->mix_fader);
            AXSetVoiceState(self->voices[0], 1);
            AXSetVoiceState(self->voices[1], 1);
        } else {
            MIXInitChannel(
                self->voices[0], 0, 0, -960, -960,
                self->pan, self->volume, self->mix_fader);
            AXSetVoiceState(self->voices[0], 1);
        }

        self->state = 2;
        if ((flags & 1) != 0) {
            self->state |= 1;
        }
        result = 0;
    }
    return result;
}

/* Matched: 100% report-exact; canonical Stop dispatch. */
int SoundBuffer_Playable::SetCurrentPosition(
    unsigned long position) {
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

/* Matched: 100% report-exact; canonical Play dispatch. */
int SoundBuffer_Playable::UnPause(void) {
    SoundBuffer_Playable* self = this;
    int result = -1;

    if ((self->state & 4) != 0) {
        result = this->Play(self->state & 1);
    }
    return result;
}

/* Matched: 100% report-exact; canonical StopIfDonePlaying dispatch. */
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

int SoundBuffer_Playable::Stop(void) {
    SoundBuffer_Playable* self = this;
    int result = -1;
    BOOL enabled = OSDisableInterrupts();

    if (self->voices[0] != 0) {
        AXSetVoiceState(self->voices[0], 0);
        MIXReleaseChannel(self->voices[0]);
        AXFreeVoice(self->voices[0]);
        result = 0;
        self->voices[0] = (_AXVPB*)result;
    }
    if (self->voices[1] != 0) {
        AXSetVoiceState(self->voices[1], 0);
        MIXReleaseChannel(self->voices[1]);
        AXFreeVoice(self->voices[1]);
        result = 0;
        self->voices[1] = (_AXVPB*)result;
    }
    OSRestoreInterrupts(enabled);
    self->state = 8;
    return result;
}

int SoundBuffer_Playable::Play(unsigned long flags) {
    BOOL enabled = OSDisableInterrupts();
    int result = iPlay(flags, 15, 10);
    OSRestoreInterrupts(enabled);
    return result;
}

/* Matched: 100% report-exact; direct float-to-byte conversion preserves the
 * retail arguments while using the canonical unsigned-char virtual methods. */
int SoundBuffer_Playable::SetRelativePan(float pan) {
    float scaled = 2.0f * (pan - -2.0f);
    int index = (int)(float)floor(scaled);
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
    unsigned char output_pan = (unsigned char)(
        inverse * SurroundPanTable[offset] +
        fraction * SurroundPanTable[offset + 2]);
    int result = 0;
    unsigned char output_surround = (unsigned char)(
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

/* Matched: 100% report-exact; canonical SetVolume dispatch. */
int SoundBuffer_Playable::SetRelativeVolume(float volume) {
    long db_volume =
        mslWaveGetDbMapEntryRelative(volume);

    return this->SetVolume(db_volume);
}

int SoundBuffer_Playable::SetVolume(long volume) {
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

/* Matched: 100% report-exact; canonical SetFrequency dispatch. */
int SoundBuffer_Playable::SetRelativeFrequency(float frequency) {
    /* Soft ceiling: ~99.89% -- only the partial-TU floating-constant
       relocation label differs from retail. */
    SoundBuffer_Playable* self = this;
    unsigned long scaled_frequency = 0;
    SPSoundEntry* sound = 0;

    if (self->file_entry->sound_table != 0) {
        sound =
            SPGetSoundEntry(self->file_entry->sound_table, 0);
    }
    if (sound != 0) {
        scaled_frequency =
            (unsigned long)(sound->sample_rate * frequency);
    }
    return this->SetFrequency(scaled_frequency);
}

int SoundBuffer_Playable::SetFrequency(unsigned long frequency) {
    SoundBuffer_Playable* self = this;
    BOOL enabled;
    _AXVPB* primary;
    unsigned short ratio_hi;
    unsigned short ratio_lo;
    unsigned long sync;

    self->frequency = frequency;
    enabled = OSDisableInterrupts();
    primary = self->voices[0];
    if (primary != 0 ||
        self->voices[1] != 0) {
        unsigned long adjusted_frequency = frequency;

        if (adjusted_frequency < 63) {
            adjusted_frequency = 63;
        } else if (adjusted_frequency > 128000) {
            adjusted_frequency = 128000;
        }
        ratio_hi = adjusted_frequency / 32000;
        ratio_lo = (adjusted_frequency << 8) / 125;
        if (primary != 0) {
            sync = primary->sync;
            primary->pb.src.ratioHi = ratio_hi;
            sync |= 0x80000;
            primary->pb.src.ratioLo = ratio_lo;
            primary->sync = sync;
        }
        if (self->voices[1] != 0) {
            _AXVPB* secondary = self->voices[1];

            sync = secondary->sync;
            secondary->pb.src.ratioHi = ratio_hi;
            sync |= 0x80000;
            secondary->pb.src.ratioLo = ratio_lo;
            secondary->sync = sync;
        }
    }
    OSRestoreInterrupts(enabled);
    return 0;
}

int SoundBuffer_Playable::GetStatus(unsigned long* status) {
    SoundBuffer_Playable* self = this;
    int result = -1;
    unsigned long result_status = 0;

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

int SoundBuffer_Playable::IsReadyToPlay(void) {
    return 1;
}

/* Matched: 100% report-exact; canonical Stop dispatch. */
void SoundBuffer_Playable::FreeResources(void) {
    SoundBuffer_Playable* self = this;
    this->Stop();
    if (self->aram_block != 0) {
        self->aram_block->Release();
        self->aram_block = 0;
    }
}

/* Matched: 100% report-exact; standard delete supplies the retail null guard
 * and virtual deleting-destructor call after resource release. */
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

SoundBuffer_Playable::~SoundBuffer_Playable() {
    FreeResources();
}

/* Matched: 100% report-exact; access the data owner's canonical file entry. */
int SoundBuffer_Data::GetNumChannels(void) {
    return (file_entry->has_secondary != 0) + 1;
}

/* Matched: 100% report-exact; release the data owner's canonical ARAM block. */
void SoundBuffer_Data::FreeResources(void) {
    if (aram_block != 0) {
        aram_block->Release();
        aram_block = 0;
    }
}

/* Matched: 100% report-exact; standard delete supplies the retail null guard
 * and virtual deleting-destructor call after resource release. */
void SoundBuffer_Data::FreeObject(void) {
    this->FreeResources();
    delete this;
}

int SoundBuffer::SetCurrentPosition(unsigned long position) {
    _MSL_GCN_BREAK();
    return -1;
}

int SoundBuffer::UnPause(void) {
    _MSL_GCN_BREAK();
    return -1;
}

int SoundBuffer::Pause(void) {
    _MSL_GCN_BREAK();
    return -1;
}

int SoundBuffer::Stop(void) {
    return -1;
}

int SoundBuffer::Play(unsigned long flags) {
    _MSL_GCN_BREAK();
    return -1;
}

int SoundBuffer::SetRelativePan(float pan) {
    _MSL_GCN_BREAK();
    return -1;
}

int SoundBuffer::SetSurroundPan(unsigned char pan) {
    _MSL_GCN_BREAK();
    return -1;
}

int SoundBuffer::SetPan(unsigned char pan) {
    _MSL_GCN_BREAK();
    return -1;
}

int SoundBuffer::SetRelativeVolume(float volume) {
    _MSL_GCN_BREAK();
    return -1;
}

int SoundBuffer::SetVolume(long volume) {
    _MSL_GCN_BREAK();
    return -1;
}

int SoundBuffer::SetRelativeFrequency(float frequency) {
    _MSL_GCN_BREAK();
    return -1;
}

int SoundBuffer::SetFrequency(unsigned long frequency) {
    _MSL_GCN_BREAK();
    return -1;
}

int SoundBuffer::GetStatus(unsigned long* status) {
    _MSL_GCN_BREAK();
    return -1;
}

int SoundBuffer::GetNumChannels(void) {
    _MSL_GCN_BREAK();
    return 0;
}

int SoundBuffer::IsReadyToPlay(void) {
    return 0;
}

void SoundBuffer::PrepForPlay(void) {
}

/*
 * Exact: retail file-buffer splitting, ARQ request ownership, mono/stereo
 * ring advancement, final-read callback selection, and cleanup.
 */
/* Matched: 100% report-exact; canonical Stop dispatch on read failure. */
void SBPlayable_Stream::StreamFileRead_CallBack(
    void* buffer, unsigned long offset, int size, int error,
    int final_chunk, void* callback_data) {
    SBPlayable_Stream* stream;
    void (*callback)(unsigned long);
    void (*last_callback)(unsigned long);
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
            long segment = (long)offset >> stream->segment_shift;
            long channel;
            long block;
            long segment_size = stream->segment_size;
            long in_segment;
            long segment_remaining;

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
                    unsigned long destination;

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
                                (segment_remaining + 0x1F) & ~0x1FUL;
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
                        &request->request, 0, 0, 1, (unsigned long)cursor,
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

void SBPlayable_Stream::i_ARQCALLBACK_ArqComplete_LastRead(
    unsigned long request_address) {
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

void SBPlayable_Stream::i_ARQCALLBACK_ArqComplete_ReturnBuffer(
    unsigned long request_address) {
    mslARQRequest* request = (mslARQRequest*)request_address;
    SBPlayable_Stream* stream =
        (SBPlayable_Stream*)request->callback_data;

    stream->pending_arq_count--;
    i_ARQCALLBACK_ReturnArqAndUserStreamBuffer(request_address);
}

void SBPlayable_Stream::i_ARQCALLBACK_ArqComplete(
    unsigned long request_address) {
    mslARQRequest* request = (mslARQRequest*)request_address;
    SBPlayable_Stream* stream =
        (SBPlayable_Stream*)request->callback_data;

    stream->pending_arq_count--;
    i_ARQCALLBACK_ReturnArq(request_address);
}

/* TODO: [near miss] 97.974655%; narrower secondary lifetime regressed to
 * 97.88499%; retain one-owner baseline; stop at extra zero and coloring. */
int SBPlayable_Stream::iPlayPrepped(void) {
    SBPlayable_Stream* stream =
        this;
    int result = -1;

    if (stream->ready_to_play == 0 ||
        stream->voices_started != 0) {
        return result;
    }

    if (stream->voices[0] == 0) {
        int use_stream_loop;
        int use_loop_address;

        stream->ring_play_block = 0;
        stream->at_zero_buffer = 0;
        stream->voices[0] = AXAcquireVoice(
            0x14, AcquireVoiceCallback, (unsigned long)this);
        if (stream->voices[0] == 0) {
            return result;
        }
        AXSetVoicePriority(stream->voices[0], 0x19);

        use_stream_loop = 1;
        use_loop_address = 1;
        if (stream->source_blocks_remaining == -1) {
            use_stream_loop = 0;
            if ((stream->state & 1) == 0) {
                use_loop_address = 0;
            }
        }

        if (stream->file_entry->has_secondary != 0) {
            stream->voices[1] = AXAcquireVoice(
                0x14, AcquireVoiceCallback, (unsigned long)this);
            if (stream->voices[1] == 0) {
                AXFreeVoice(stream->voices[0]);
                stream->voices[0] = 0;
                return result;
            }
            AXSetVoicePriority(stream->voices[1], 0x19);
            SPSoundEntry* primary_sound = 0;
            SPSoundEntry* secondary_sound = 0;

            if (stream->file_entry->sound_table != 0) {
                primary_sound = SPGetSoundEntry(
                    stream->file_entry->sound_table, 0);
            }
            SetStreamVoiceSource(
                stream->voices[0], primary_sound,
                stream->frequency, use_stream_loop,
                use_loop_address);
            if (stream->file_entry->secondary_sound_table != 0) {
                secondary_sound = SPGetSoundEntry(
                    stream->file_entry->secondary_sound_table, 0);
            }
            SetStreamVoiceSource(
                stream->voices[1], secondary_sound,
                stream->frequency, use_stream_loop,
                use_loop_address);
            MIXInitChannel(
                stream->voices[0], 0, 0, -960, -960,
                0, 0x7F, stream->mix_fader);
            MIXInitChannel(
                stream->voices[1], 0, 0, -960, -960,
                0x7F, 0x7F, stream->mix_fader);
        } else {
            SPSoundEntry* primary_sound = 0;

            if (stream->file_entry->sound_table != 0) {
                primary_sound = SPGetSoundEntry(
                    stream->file_entry->sound_table, 0);
            }
            SetStreamVoiceSource(
                stream->voices[0], primary_sound,
                stream->frequency, use_stream_loop,
                use_loop_address);
            MIXInitChannel(
                stream->voices[0], 0, 0, -960, -960,
                stream->pan, stream->volume, stream->mix_fader);
        }

        if (stream->source_blocks_remaining == -1) {
            int channel;

            stream->ring_play_block = 0;
            if (stream->ring_write_block != 0) {
                stream->ax_end_block =
                    stream->ring_write_block - 1;
            } else {
                stream->ax_end_block =
                    stream->ring_block_count - 1;
            }
            if (use_loop_address != 0) {
                stream->pending_ax_block = 0;
            } else {
                stream->pending_ax_block = -1;
            }

            for (channel = 0;
                 channel < GetNumChannels(); channel++) {
                _AXVPB* voice =
                    stream->voices[channel];
                unsigned long base =
                    stream->cache_buffers[channel] << 1;
                unsigned long address;
                unsigned long sync;

                address =
                    ((unsigned long)voice->pb.addr.currentAddressHi << 16) +
                    voice->pb.addr.currentAddressLo;
                address += base;
                sync = voice->sync | 0x10000;
                voice->pb.addr.currentAddressLo = address;
                voice->pb.addr.currentAddressHi = address >> 16;
                if ((sync & 0x1000) == 0) {
                    voice->sync = sync;
                }

                address =
                    ((unsigned long)voice->pb.addr.endAddressHi << 16) +
                    voice->pb.addr.endAddressLo;
                address += base;
                sync = voice->sync | 0x8000;
                voice->pb.addr.endAddressLo = address;
                voice->pb.addr.endAddressHi = address >> 16;
                if ((sync & 0x1000) == 0) {
                    voice->sync = sync;
                }

                if (use_loop_address) {
                    address =
                        ((unsigned long)voice->pb.addr.loopAddressHi << 16) +
                        voice->pb.addr.loopAddressLo;
                    address += base;
                    sync = voice->sync | 0x4000;
                    voice->pb.addr.loopAddressLo = address;
                    voice->pb.addr.loopAddressHi = address >> 16;
                    if ((sync & 0x1000) == 0) {
                        voice->sync = sync;
                    }
                }
            }
        } else {
            int channel;

            stream->ax_end_block =
                stream->ring_play_block;
            stream->pending_ax_block = -1;
            for (channel = 0;
                 channel < GetNumChannels(); channel++) {
                _AXVPB* voice =
                    stream->voices[channel];
                unsigned long base =
                    stream->cache_buffers[channel] << 1;
                unsigned long zero;
                unsigned long address;
                unsigned long sync;

                address = base + 2;
                sync = voice->sync | 0x10000;
                voice->pb.addr.currentAddressLo = address;
                voice->pb.addr.currentAddressHi = address >> 16;
                if ((sync & 0x1000) == 0) {
                    voice->sync = sync;
                }

                address =
                    base + (stream->segment_size << 1) - 1;
                sync = voice->sync | 0x8000;
                voice->pb.addr.endAddressLo = address;
                voice->pb.addr.endAddressHi = address >> 16;
                if ((sync & 0x1000) == 0) {
                    voice->sync = sync;
                }

                sync = voice->sync;
                zero = g_MSL_GCN_ARAM_ZeroBase_ADPCM_Start;
                sync |= 0x4000;
                voice->pb.addr.loopAddressLo = zero;
                voice->pb.addr.loopAddressHi = zero >> 16;
                if ((sync & 0x1000) == 0) {
                    voice->sync = sync;
                }
                sync = voice->sync;
                voice->pb.adpcmLoop.loop_pred_scale = 0;
                sync |= 0x100000;
                voice->pb.adpcmLoop.loop_yn1 = 0;
                voice->pb.adpcmLoop.loop_yn2 = 0;
                voice->sync = sync;
                sync = voice->sync;
                voice->pb.type = 0;
                sync |= 8;
                voice->sync = sync;
                sync = voice->sync;
                sync |= 0x2000;
                voice->pb.addr.loopFlag = 0;
                if ((sync & 0x1000) == 0) {
                    voice->sync = sync;
                }
            }
        }
    }

    if (stream->voices[0] != 0) {
        stream->voices_started = 1;
        stream->crossed_stream_end = 0;
        stream->end_of_stream = 0;
        if (stream->at_zero_buffer == 0) {
            AXSetVoiceState(stream->voices[0], 1);
            if (stream->voices[1] != 0) {
                AXSetVoiceState(stream->voices[1], 1);
            }
        }
        result = 0;
    }
    return result;
}

/* Matched: 100% report-exact; canonical Stop dispatch at both end gates. */
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

/* Matched: 100% report-exact; queue priority is always 0x10 and its callee
 * overwrites the dead comparison. Identical-arm source provenance remains open. */
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
            long block_count;
            long read_size;
            long bytes_remaining;
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
                    ((long)read_size + 0x3FFF) >> 14;
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

/*
 * Exact: typed mono/stereo AX end/current/loop publication, including
 * separate raw and updated sync-word lifetimes for both voices.
 */
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

    unsigned long end_address;
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
    unsigned long sync;
    unsigned long end_sync;
    unsigned long address = stream->cache_buffers[0];
    end_sync = voice->sync;
    address = end_address + (address << 1);

    sync = end_sync | 0x8000;
    voice->pb.addr.endAddressLo = address;
    voice->pb.addr.endAddressHi = address >> 16;
    if ((sync & 0x1000) == 0) {
        voice->sync = sync;
    }
    sync = voice->sync;
    address = g_MSL_GCN_ARAM_ZeroBase_ADPCM_Start;
    sync |= 0x4000;
    voice->pb.addr.loopAddressLo = address;
    voice->pb.addr.loopAddressHi = address >> 16;
    if ((sync & 0x1000) == 0) {
        voice->sync = sync;
    }
    sync = voice->sync;
    voice->pb.adpcmLoop.loop_pred_scale = 0;
    sync |= 0x100000;
    voice->pb.adpcmLoop.loop_yn1 = 0;
    voice->pb.adpcmLoop.loop_yn2 = 0;
    voice->sync = sync;
    sync = voice->sync;
    voice->pb.type = 0;
    sync |= 8;
    voice->sync = sync;
    sync = voice->sync;
    sync |= 0x2000;
    voice->pb.addr.loopFlag = 0;
    if ((sync & 0x1000) == 0) {
        voice->sync = sync;
    }

    voice = stream->voices[1];
    if (voice == 0) {
        return;
    }
    address = stream->cache_buffers[1];
    end_sync = voice->sync;
    address = end_address + (address << 1);
    sync = end_sync | 0x8000;
    voice->pb.addr.endAddressLo = address;
    voice->pb.addr.endAddressHi = address >> 16;
    if ((sync & 0x1000) == 0) {
        voice->sync = sync;
    }
    sync = voice->sync;
    address = g_MSL_GCN_ARAM_ZeroBase_ADPCM_Start;
    sync |= 0x4000;
    voice->pb.addr.loopAddressLo = address;
    voice->pb.addr.loopAddressHi = address >> 16;
    if ((sync & 0x1000) == 0) {
        voice->sync = sync;
    }
    sync = voice->sync;
    voice->pb.adpcmLoop.loop_pred_scale = 0;
    sync |= 0x100000;
    voice->pb.adpcmLoop.loop_yn1 = 0;
    voice->pb.adpcmLoop.loop_yn2 = 0;
    voice->sync = sync;
    sync = voice->sync;
    voice->pb.type = 0;
    sync |= 8;
    voice->sync = sync;
    sync = voice->sync;
    sync |= 0x2000;
    voice->pb.addr.loopFlag = 0;
    if ((sync & 0x1000) == 0) {
        voice->sync = sync;
    }
}

/* Typed stream-voice block lookup with the retail optional raw-address out. */
inline int SBPlayable_Stream::iAX_GetVoiceBlock(
    _AXVPB* voice,
    int channel,
    unsigned long* raw_address) {
    SBPlayable_Stream* stream =
        this;
    unsigned long address =
        ((unsigned long)voice->pb.addr.currentAddressHi << 16) +
        voice->pb.addr.currentAddressLo;
    int block;

    if (address >= g_MSL_GCN_ARAM_ZeroBase_ADPCM_Start &&
        address <= g_MSL_GCN_ARAM_ZeroBase_ADPCM_End) {
        block = -1;
    } else {
        address = ((long)address >> 1) -
            (channel == 0 ? stream->cache_buffers[0] :
                            stream->cache_buffers[1]);
        block = (long)address >> stream->segment_shift;
        if (block < 0 || block >= stream->ring_block_count) {
            _MSL_GCN_BREAK();
        }
    }
    if (raw_address != 0) {
        *raw_address = address;
    }
    return block;
}

/* TODO: [near miss] 92.93%; nested end-pass gate and restart ADPCM fields
 * are corrected; remaining differences are GPRs and AX load/store scheduling. */
void SBPlayable_Stream::iUpdate_AXUser(void) {
    SBPlayable_Stream* stream =
        this;
    int primary_block;
    int secondary_block;
    int channels;
    int next_block;
    int crossed_scan_stop;
    unsigned long primary_raw_address;

    if (stream->voices_started == 0) {
        return;
    }
    if (stream->source_blocks_remaining == -1) {
        return;
    }

    primary_block = iAX_GetVoiceBlock(
        stream->voices[0], 0,
        &primary_raw_address);

    if (stream->file_entry->has_secondary != 0) {
        unsigned long secondary_raw_address;

        channels = 2;
        secondary_block = iAX_GetVoiceBlock(
            stream->voices[1], 1,
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

                unsigned long block_address =
                    (stream->ring_play_block <<
                     (stream->segment_shift + 1)) + 2;

                {
                    _AXVPB* voice =
                        stream->voices[0];
                    unsigned long address;
                    unsigned long sync;

                    sync = voice->sync;
                    address =
                        block_address +
                        (stream->cache_buffers[0] << 1);
                    sync |= 0x10000;
                    voice->pb.addr.currentAddressLo = address;
                    voice->pb.addr.currentAddressHi = address >> 16;
                    if ((sync & 0x1000) == 0) {
                        voice->sync = sync;
                    }
                    sync = voice->sync;
                    voice->pb.adpcm.pred_scale =
                        stream->block_headers[0]
                            [stream->ring_play_block];
                    sync |= 0x20000;
                    voice->pb.adpcm.yn1 = 0;
                    voice->pb.adpcm.yn2 = 0;
                    voice->sync = sync;
                }

                if (stream->voices[1] != 0) {
                    _AXVPB* voice =
                        stream->voices[1];
                    unsigned long address;
                    unsigned long sync;
                    sync = voice->sync;
                    address =
                        block_address +
                        (stream->cache_buffers[1] << 1);
                    sync |= 0x10000;
                    voice->pb.addr.currentAddressLo = address;
                    voice->pb.addr.currentAddressHi = address >> 16;
                    if ((sync & 0x1000) == 0) {
                        voice->sync = sync;
                    }
                    sync = voice->sync;
                    voice->pb.adpcm.pred_scale =
                        stream->block_headers[1]
                            [stream->ring_play_block];
                    sync |= 0x20000;
                    voice->pb.adpcm.yn1 = 0;
                    voice->pb.adpcm.yn2 = 0;
                    voice->sync = sync;
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

            unsigned long block_address =
                (next_block <<
                 (stream->segment_shift + 1)) + 2;
            unsigned char block_header;
            _AXVPB* voice =
                stream->voices[0];
            unsigned long address;
            unsigned long sync;
            bool at_stream_end =
                stream->ax_end_block ==
                stream->stream_end_block;

            address =
                block_address +
                (stream->cache_buffers[0] << 1);
            sync = voice->sync;
            sync |= 0x4000;
            block_header =
                stream->block_headers[0][next_block];
            voice->pb.addr.loopAddressLo = address;
            voice->pb.addr.loopAddressHi = address >> 16;
            if ((sync & 0x1000) == 0) {
                voice->sync = sync;
            }
            sync = voice->sync;
            voice->pb.adpcmLoop.loop_pred_scale =
                block_header;
            sync |= 0x100000;
            voice->pb.adpcmLoop.loop_yn1 = 0;
            voice->pb.adpcmLoop.loop_yn2 = 0;
            voice->sync = sync;
            if (at_stream_end != 0) {
                sync = voice->sync;
                voice->pb.type = 0;
                sync |= 8;
                voice->sync = sync;
            } else {
                sync = voice->sync;
                voice->pb.type = 1;
                sync |= 8;
                voice->sync = sync;
            }
            sync = voice->sync;
            sync |= 0x2000;
            voice->pb.addr.loopFlag = 1;
            if ((sync & 0x1000) == 0) {
                voice->sync = sync;
            }

            voice = stream->voices[1];
            if (voice != 0) {
                at_stream_end =
                    stream->ax_end_block ==
                    stream->stream_end_block;

                address =
                    block_address +
                    (stream->cache_buffers[1] << 1);
                sync = voice->sync;
                sync |= 0x4000;
                block_header =
                    stream->block_headers[1][next_block];
                voice->pb.addr.loopAddressLo = address;
                voice->pb.addr.loopAddressHi = address >> 16;
                if ((sync & 0x1000) == 0) {
                    voice->sync = sync;
                }
                sync = voice->sync;
                voice->pb.adpcmLoop.loop_pred_scale =
                    block_header;
                sync |= 0x100000;
                voice->pb.adpcmLoop.loop_yn1 = 0;
                voice->pb.adpcmLoop.loop_yn2 = 0;
                voice->sync = sync;
                if (at_stream_end != 0) {
                    sync = voice->sync;
                    voice->pb.type = 0;
                    sync |= 8;
                    voice->sync = sync;
                } else {
                    sync = voice->sync;
                    voice->pb.type = 1;
                    sync |= 8;
                    voice->sync = sync;
                }
                sync = voice->sync;
                sync |= 0x2000;
                voice->pb.addr.loopFlag = 1;
                if ((sync & 0x1000) == 0) {
                    voice->sync = sync;
                }
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

/* Matched: 100% report-exact; canonical StopIfDonePlaying dispatch. */
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

/* TODO: [near miss] 97.00%; interrupt-state/result GPRs and two extra zero
 * loads remain; voice/cache/request teardown and return behavior agree. */
int SBPlayable_Stream::Stop(void) {
    SBPlayable_Stream* stream =
        this;
    SoundBuffer_Playable* self = stream;
    BOOL enabled = OSDisableInterrupts();
    int result = -1;
    BOOL voice_enabled = OSDisableInterrupts();

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
    OSRestoreInterrupts(voice_enabled);
    self->state = 8;
    stream->voices_started = 0;

    voice_enabled = OSDisableInterrupts();
    stream->ready_to_play = 0;
    stream->last_read_pending = 0;
    stream->play_when_ready = 0;
    if (stream->cache_buffers[0] != 0) {
        mslStreamCache_ReleaseBuffer(stream->cache_buffers[0]);
    }
    if (stream->cache_buffers[1] != 0) {
        mslStreamCache_ReleaseBuffer(stream->cache_buffers[1]);
    }
    stream->cache_buffers[0] = 0;
    stream->cache_buffers[1] = 0;
    stream->cache_buffer_size = 0;
    stream->cache_buffer1_size = 0;
    if (stream->pending_file_request != 0) {
        mslStreamFile_CancelRequest(stream->pending_file_request);
        stream->pending_file_request = 0;
    }
    OSRestoreInterrupts(voice_enabled);
    OSRestoreInterrupts(enabled);
    return result;
}

/* Matched: 100% report-exact; canonical PrepForPlay dispatch. */
int SBPlayable_Stream::Play(unsigned long flags) {
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

int SBPlayable_Stream::IsReadyToPlay(void) {
    SBPlayable_Stream* self =
        this;

    return self->ready_to_play;
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

/* Matched: 100% report-exact; both retail Stop calls use the owner type. */
void SBPlayable_Stream::FreeResources(void) {
    SoundBuffer_Playable* self = this;

    this->Stop();
    this->Stop();
    if (self->aram_block != 0) {
        self->aram_block->Release();
        self->aram_block = 0;
    }
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

/* Matched: 100% report-exact; standard delete supplies the retail null guard
 * and virtual deleting-destructor call after resource release. */
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

SBPlayable_Stream::~SBPlayable_Stream() {
    FreeResources();
}
