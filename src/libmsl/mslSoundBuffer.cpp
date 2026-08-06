#include "msl/mslBank.h"
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

typedef unsigned long BOOL;

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

class SoundBufferPlayableInterface {
public:
    virtual void Destroy(short flags);       /* +0x08 */
    virtual void FreeObject(void);           /* +0x0C */
    virtual void FreeResources(void);        /* +0x10 */
    virtual void Slot14(void);
    virtual void Slot18(void);
    virtual void Slot1C(void);
    virtual int GetStatus(unsigned long* status); /* +0x20 */
    virtual int SetFrequency(unsigned long frequency); /* +0x24 */
    virtual int SetRelativeFrequency(float frequency); /* +0x28 */
    virtual int SetVolume(long volume);       /* +0x2C */
    virtual int SetRelativeVolume(float volume); /* +0x30 */
    virtual int SetPan(int pan);              /* +0x34 ABI call view */
    virtual int SetSurroundPan(int pan);      /* +0x38 ABI call view */
    virtual int SetRelativePan(float pan);    /* +0x3C */
    virtual int Play(unsigned long flags);    /* +0x40 */
    virtual int Stop(void);                   /* +0x44 */
    virtual void Slot48(void);
    virtual void Slot4C(void);
    virtual void Slot50(void);
    virtual void LostVoice(_AXVPB* voice);    /* +0x54 */
    virtual void Slot58(void);
    virtual void Slot5C(void);
    virtual void StopIfDonePlaying(void);     /* +0x60 */
};

struct SoundBufferUpdateList {
    SoundBuffer_Playable* first;
    SoundBuffer_Playable* last;
};

class IRefCntRes {
public:
    virtual ~IRefCntRes() {}
    virtual void FreeObject(void) = 0;
    virtual void FreeResources(void) {}
};

class SoundBuffer : public IRefCntRes {
public:
    static void SB_MslTickCallback(void);
    static void SB_AXUserCallback(void);
    static SoundBuffer_Playable* CreatePlayableStreamBuffer(
        _mslBank* bank, _GameCubeFileEntry* entry);
    static SoundBuffer_Playable* CreatePlayableStaticBuffer(
        _mslBank* bank, _GameCubeFileEntry* entry);
    virtual ~SoundBuffer() {}
    virtual void PrepForPlay(void);
    virtual int IsReadyToPlay(void);
    virtual int GetNumChannels(void);
    virtual int GetStatus(unsigned long* status);
    virtual int SetFrequency(unsigned long frequency);
    virtual int SetRelativeFrequency(float frequency);
    virtual int SetVolume(long volume);
    virtual int SetRelativeVolume(float volume);
    virtual int SetPan(unsigned char pan);
    virtual int SetSurroundPan(unsigned char pan);
    virtual int SetRelativePan(float pan);
    virtual int Play(unsigned long flags);
    virtual int Stop(void);
    virtual int Pause(void);
    virtual int UnPause(void);
    virtual int SetCurrentPosition(unsigned long position);
};

class SoundBuffer_Data : public SoundBuffer {
public:
    virtual ~SoundBuffer_Data() { FreeResources(); }
    virtual void FreeObject(void);
    virtual void FreeResources(void);
    virtual int GetNumChannels(void);
};

class SoundBuffer_Playable : public SoundBuffer_Data {
public:
    virtual ~SoundBuffer_Playable();
    virtual void FreeObject(void);
    virtual void FreeResources(void);
    virtual int IsReadyToPlay(void);
    virtual int GetStatus(unsigned long* status);
    virtual int SetFrequency(unsigned long frequency);
    virtual int SetRelativeFrequency(float frequency);
    virtual int SetVolume(long volume);
    virtual int SetRelativeVolume(float volume);
    virtual int SetPan(unsigned char pan);
    virtual int SetSurroundPan(unsigned char pan);
    virtual int SetRelativePan(float pan);
    virtual int Play(unsigned long flags);
    virtual int Stop(void);
    virtual int Pause(void);
    virtual int UnPause(void);
    virtual int SetCurrentPosition(unsigned long position);
    virtual void LostVoice(_AXVPB* voice);
    virtual void iUpdate_AXUser(void);
    virtual void iUpdate_MslTick(void);
    virtual void StopIfDonePlaying(void);
    static void AcquireVoiceCallback(void* voice);
    int iPlay(unsigned long flags, unsigned long acquire_priority,
              unsigned long active_priority);
};

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
};

typedef void (*SoundBufferMethod)(SoundBuffer_Playable*);
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
    SoundBufferMethod Stop;       /* +0x44 */
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
    _AXVPB* primary_voice;         /* +0x30 */
    _AXVPB* secondary_voice;       /* +0x34 */
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
    unsigned long cache_buffer0;   /* +0x40 */
    unsigned long cache_buffer1;   /* +0x44 */
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
    unsigned long end_pass_count;  /* +0x9C */
    long stream_end_block;         /* +0xA0 */
    unsigned long initial_position;/* +0xA4 */
    long ring_scan_stop_block;     /* +0xA8 */
    unsigned char block_headers[2][0x10]; /* +0xAC -- first byte per ring block */
};

typedef char SBPlayableStreamLayoutSize[
    sizeof(SBPlayableStreamLayout) == 0xCC ? 1 : -1];

static inline SoundBufferPlayableLayout* PlayableState(
    SoundBuffer_Playable* playable) {
    return (SoundBufferPlayableLayout*)playable;
}

static inline SoundBufferPlayableLayout* PlayableState(
    SoundBuffer_Data* data) {
    return (SoundBufferPlayableLayout*)data;
}

static inline SBPlayableStreamLayout* StreamState(
    SBPlayable_Stream* stream) {
    return (SBPlayableStreamLayout*)stream;
}

struct AXVoiceCallbackData {
    unsigned char pad00[0x14];
    SoundBuffer_Playable* owner;   /* +0x14 */
};

static const unsigned char SurroundPanTable[] = {
    0x40, 0x10, 0x10, 0x40, 0x08, 0x78,
    0x20, 0x7C, 0x40, 0x7C, 0x60, 0x7C,
    0x77, 0x78, 0x70, 0x40, 0x40, 0x10
};

static const char stringBase0[] =
    "SBPlayable_Stream\0SoundBuffer_Playable";

extern void* MWSOUND_HEAP;
SoundBufferUpdateList ms_UpdateList__20SoundBuffer_Playable = {0, 0};
extern unsigned char __vt__10IRefCntRes[];
extern unsigned char __vt__11SoundBuffer[];
extern unsigned char __vt__16SoundBuffer_Data[];
extern unsigned char __vt__20SoundBuffer_Playable[];
extern unsigned char __vt__17SBPlayable_Stream[];

void SoundBuffer::SB_MslTickCallback(void) {
    BOOL enabled = OSDisableInterrupts();
    SoundBufferPlayableLayout* playable =
        (SoundBufferPlayableLayout*)
            ms_UpdateList__20SoundBuffer_Playable.first;

    while (playable != 0) {
        SoundBufferPlayableLayout* current = playable;
        playable = playable->next;
        ((SoundBufferPlayableInterface*)current)->Slot5C();
    }
    OSRestoreInterrupts(enabled);
}

void SoundBuffer::SB_AXUserCallback(void) {
    BOOL enabled = OSDisableInterrupts();
    SoundBufferPlayableLayout* playable =
        (SoundBufferPlayableLayout*)
            ms_UpdateList__20SoundBuffer_Playable.first;

    while (playable != 0) {
        SoundBufferPlayableLayout* current = playable;
        playable = playable->next;
        ((SoundBufferPlayableInterface*)current)->Slot58();
    }
    OSRestoreInterrupts(enabled);
}

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
        playable->primary_voice = 0;
        playable->secondary_voice = 0;
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
        ((SoundBufferPlayableInterface*)stream)->Slot14();
    }

    OSRestoreInterrupts(enabled);
    return (SoundBuffer_Playable*)stream;
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
        playable->primary_voice = 0;
        playable->secondary_voice = 0;
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
                ((SoundBufferPlayableInterface*)playable)->FreeObject();
            }
            playable = 0;
        }
    }

    OSRestoreInterrupts(enabled);
    return (SoundBuffer_Playable*)playable;
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

    source.ratio_hi = frequency / 32000;
    source.ratio_lo = (frequency << 8) / 125;
    source.current_fraction = 0;
    source.last_samples[0] = 0;
    source.last_samples[1] = 0;
    source.last_samples[2] = 0;
    source.last_samples[3] = 0;
    AXSetVoiceSrc(voice, &source);
    AXSetVoiceAdpcm(voice, sound->adpcm);

    sync = voice->sync;
    voice->depop = 0;
    sync |= 8;
    voice->sync = sync;
    current =
        loop != 0 ? sound->current_address : sound->loop_address;
    end = sound->end_address;
    loop_address = sound->current_address;
    sync = voice->sync;
    voice->loop = loop;
    voice->format = 0;
    voice->current_hi = current >> 16;
    voice->current_lo = current;
    voice->end_hi = end >> 16;
    voice->end_lo = end;
    voice->loop_hi = loop_address >> 16;
    voice->loop_lo = loop_address;
    voice->sync = (sync & 0xFFFE1FFF) | 0x1000;
}

/*
 * Soft ceiling: iPlay ~91.26% with the exact retail 0x450-byte size.
 * The AX voice, priority, mono/stereo source, mixer, loop, and state setup
 * are recovered. Hoisting both sound-entry declarations to the shared play
 * region restores retail's six-register save range; remaining differences
 * are register coloring and inlined source-address scheduling. The paused
 * resume path intentionally returns -1 when neither voice exists.
 */
int SoundBuffer_Playable::iPlay(
    unsigned long flags, unsigned long acquire_priority,
    unsigned long active_priority) {
    SoundBufferPlayableLayout* self =
        PlayableState(this);
    int result = -1;

    if ((self->state & 4) != 0) {
        self->state = (self->state & 1) | 2;
        if (self->primary_voice != 0) {
            AXSetVoiceState(self->primary_voice, 1);
            result = 0;
        }
        if (self->secondary_voice != 0) {
            AXSetVoiceState(self->secondary_voice, 1);
            result = 0;
        }
    } else if ((self->state & 8) != 0) {
        self->primary_voice = AXAcquireVoice(
            acquire_priority, AcquireVoiceCallback, this);
        if (self->primary_voice == 0) {
            return -1;
        }
        if (active_priority != acquire_priority) {
            AXSetVoicePriority(self->primary_voice, active_priority);
        }

        if (self->file_entry->has_secondary != 0) {
            self->secondary_voice = AXAcquireVoice(
                acquire_priority, AcquireVoiceCallback, this);
            if (self->secondary_voice == 0) {
                AXFreeVoice(self->primary_voice);
                self->primary_voice = 0;
                return -1;
            }
            if (active_priority != acquire_priority) {
                AXSetVoicePriority(
                    self->secondary_voice, active_priority);
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
            self->primary_voice, primary_sound, self->frequency, loop);

        if (self->file_entry->has_secondary != 0) {
            if (self->file_entry->secondary_sound_table != 0) {
                secondary_sound = SPGetSoundEntry(
                    self->file_entry->secondary_sound_table, 0);
            }
            SetVoiceSource(
                self->secondary_voice, secondary_sound,
                self->frequency, loop);
            MIXInitChannel(
                self->primary_voice, 0, 0, -960, -960,
                0, 0x7F, self->mix_fader);
            MIXInitChannel(
                self->secondary_voice, 0, 0, -960, -960,
                0x7F, 0x7F, self->mix_fader);
            AXSetVoiceState(self->primary_voice, 1);
            AXSetVoiceState(self->secondary_voice, 1);
        } else {
            MIXInitChannel(
                self->primary_voice, 0, 0, -960, -960,
                self->pan, self->volume, self->mix_fader);
            AXSetVoiceState(self->primary_voice, 1);
        }

        self->state = 2;
        if ((flags & 1) != 0) {
            self->state |= 1;
        }
        result = 0;
    }
    return result;
}

int SoundBuffer_Playable::SetCurrentPosition(
    unsigned long position) {
    SoundBufferPlayableLayout* self =
        PlayableState(this);

    if (position != 0) {
        _MSL_GCN_BREAK();
    }
    if (self->state != 8) {
        ((SoundBufferPlayableInterface*)this)->Stop();
        self->current_position = position;
    }
    return -1;
}

int SoundBuffer_Playable::UnPause(void) {
    SoundBufferPlayableLayout* self =
        PlayableState(this);
    int result = -1;

    if ((self->state & 4) != 0) {
        result =
            ((SoundBufferPlayableInterface*)this)
                ->Play(self->state & 1);
    }
    return result;
}

int SoundBuffer_Playable::Pause(void) {
    SoundBufferPlayableLayout* self =
        PlayableState(this);
    int result = -1;

    ((SoundBufferPlayableInterface*)this)
        ->StopIfDonePlaying();
    if ((self->state & 2) != 0) {
        BOOL enabled = OSDisableInterrupts();
        if (self->primary_voice != 0) {
            AXSetVoiceState(self->primary_voice, 0);
            result = 0;
        }
        if (self->secondary_voice != 0) {
            AXSetVoiceState(self->secondary_voice, 0);
            result = 0;
        }
        OSRestoreInterrupts(enabled);
        self->state = (self->state & 1) | 4;
    }
    return result;
}

int SoundBuffer_Playable::Stop(void) {
    SoundBufferPlayableLayout* self =
        PlayableState(this);
    int result = -1;
    BOOL enabled = OSDisableInterrupts();

    if (self->primary_voice != 0) {
        AXSetVoiceState(self->primary_voice, 0);
        MIXReleaseChannel(self->primary_voice);
        AXFreeVoice(self->primary_voice);
        result = 0;
        self->primary_voice = (_AXVPB*)result;
    }
    if (self->secondary_voice != 0) {
        AXSetVoiceState(self->secondary_voice, 0);
        MIXReleaseChannel(self->secondary_voice);
        AXFreeVoice(self->secondary_voice);
        result = 0;
        self->secondary_voice = (_AXVPB*)result;
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
    int output_pan = (int)(
        inverse * SurroundPanTable[offset] +
        fraction * SurroundPanTable[offset + 2]);
    int result = 0;
    int output_surround = (int)(
        inverse * SurroundPanTable[offset + 1] +
        fraction * SurroundPanTable[offset + 3]);

    if (((SoundBufferPlayableInterface*)this)
            ->SetPan(output_pan) == -1) {
        result = -1;
    }
    if (((SoundBufferPlayableInterface*)this)
            ->SetSurroundPan(output_surround) == -1) {
        result = -1;
    }
    return result;
}

int SoundBuffer_Playable::SetPan(unsigned char pan) {
    SoundBufferPlayableLayout* self =
        PlayableState(this);
    int result = -1;

    if (pan < 0x80) {
        self->pan = pan;
        if (self->state != 8 &&
            self->file_entry->has_secondary == 0 &&
            self->primary_voice != 0) {
            MIXSetPan(self->primary_voice, self->pan);
            result = 0;
        }
    }
    return result;
}

int SoundBuffer_Playable::SetSurroundPan(unsigned char pan) {
    SoundBufferPlayableLayout* self =
        PlayableState(this);
    int result = -1;

    if (pan < 0x80) {
        self->volume = pan;
        if (self->state != 8 &&
            self->file_entry->has_secondary == 0 &&
            self->primary_voice != 0) {
            MIXSetSPan(self->primary_voice, self->volume);
            result = 0;
        }
    }
    return result;
}

int SoundBuffer_Playable::SetRelativeVolume(float volume) {
    long db_volume =
        mslWaveGetDbMapEntryRelative(volume);

    return ((SoundBufferPlayableInterface*)this)
        ->SetVolume(db_volume);
}

int SoundBuffer_Playable::SetVolume(long volume) {
    SoundBufferPlayableLayout* self =
        PlayableState(this);
    self->mix_fader = volume;
    if (self->primary_voice != 0) {
        MIXSetFader(self->primary_voice, volume);
    }
    if (self->secondary_voice != 0) {
        MIXSetFader(self->secondary_voice, volume);
    }
    return 0;
}

int SoundBuffer_Playable::SetRelativeFrequency(float frequency) {
    /* Soft ceiling: ~99.89% -- only the partial-TU floating-constant
       relocation label differs from retail. */
    SoundBufferPlayableLayout* self =
        PlayableState(this);
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
    return ((SoundBufferPlayableInterface*)this)
        ->SetFrequency(scaled_frequency);
}

int SoundBuffer_Playable::SetFrequency(unsigned long frequency) {
    SoundBufferPlayableLayout* self =
        PlayableState(this);
    BOOL enabled;
    _AXVPB* primary;
    unsigned short ratio_hi;
    unsigned short ratio_lo;
    unsigned long sync;

    self->frequency = frequency;
    enabled = OSDisableInterrupts();
    primary = self->primary_voice;
    if (primary != 0 ||
        self->secondary_voice != 0) {
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
            primary->ratio_hi = ratio_hi;
            sync |= 0x80000;
            primary->ratio_lo = ratio_lo;
            primary->sync = sync;
        }
        if (self->secondary_voice != 0) {
            _AXVPB* secondary = self->secondary_voice;

            sync = secondary->sync;
            secondary->ratio_hi = ratio_hi;
            sync |= 0x80000;
            secondary->ratio_lo = ratio_lo;
            secondary->sync = sync;
        }
    }
    OSRestoreInterrupts(enabled);
    return 0;
}

int SoundBuffer_Playable::GetStatus(unsigned long* status) {
    SoundBufferPlayableLayout* self =
        PlayableState(this);
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

void SoundBuffer_Playable::FreeResources(void) {
    SoundBufferPlayableLayout* self =
        PlayableState(this);
    ((SoundBufferPlayableInterface*)this)->Stop();
    if (self->aram_block != 0) {
        self->aram_block->Release();
        self->aram_block = 0;
    }
}

void SoundBuffer_Playable::FreeObject(void) {
    SoundBufferPlayableLayout* self =
        PlayableState(this);
    BOOL enabled;

    ((SoundBufferPlayableInterface*)this)
        ->FreeResources();
    enabled = OSDisableInterrupts();
    if (self->previous != 0) {
        if ((self->previous->next = self->next) != 0) {
            self->next->previous = self->previous;
        } else {
            ms_UpdateList__20SoundBuffer_Playable.last =
                (SoundBuffer_Playable*)self->previous;
        }
    } else {
        ms_UpdateList__20SoundBuffer_Playable.first =
            (SoundBuffer_Playable*)self->next;
        if (ms_UpdateList__20SoundBuffer_Playable.first != 0) {
            self->next->previous = 0;
        } else {
            ms_UpdateList__20SoundBuffer_Playable.last = 0;
        }
    }
    OSRestoreInterrupts(enabled);
    if (self != 0) {
        ((SoundBufferPlayableInterface*)this)->Destroy(1);
    }
}

SoundBuffer_Playable::~SoundBuffer_Playable() {
    FreeResources();
}

void SoundBuffer_Playable::StopIfDonePlaying(void) {
    SoundBufferPlayableLayout* self =
        PlayableState(this);

    if ((self->state & 2) != 0) {
        int done = 0;

        if (self->primary_voice != 0 &&
            (self->primary_voice->state & 1) == 0) {
            done = 1;
        }
        if (self->secondary_voice != 0 &&
            (self->secondary_voice->state & 1) == 0) {
            done = 1;
        }
        if (done != 0) {
            ((SoundBufferPlayableInterface*)this)->Stop();
        }
    }
}

void SoundBuffer_Playable::iUpdate_MslTick(void) {
    ((SoundBufferPlayableInterface*)this)
        ->StopIfDonePlaying();
}

void SoundBuffer_Playable::iUpdate_AXUser(void) {
}

void SoundBuffer_Playable::LostVoice(_AXVPB* voice) {
    SoundBufferPlayableLayout* self =
        PlayableState(this);
    BOOL enabled = OSDisableInterrupts();

    if (voice == self->primary_voice) {
        self->primary_voice = 0;
        ((SoundBufferPlayableInterface*)this)->Stop();
    } else if (voice == self->secondary_voice) {
        self->secondary_voice = 0;
        ((SoundBufferPlayableInterface*)this)->Stop();
    }

    OSRestoreInterrupts(enabled);
}

void SoundBuffer_Playable::AcquireVoiceCallback(void* voice) {
    AXVoiceCallbackData* callback = (AXVoiceCallbackData*)voice;
    ((SoundBufferPlayableInterface*)callback->owner)
        ->LostVoice((_AXVPB*)voice);
}

int SoundBuffer_Data::GetNumChannels(void) {
    SoundBufferPlayableLayout* self =
        PlayableState(this);
    return (self->file_entry->has_secondary != 0) + 1;
}

void SoundBuffer_Data::FreeResources(void) {
    SoundBufferPlayableLayout* self =
        PlayableState(this);

    if (self->aram_block != 0) {
        self->aram_block->Release();
        self->aram_block = 0;
    }
}

void SoundBuffer_Data::FreeObject(void) {
    ((SoundBufferPlayableInterface*)this)->FreeResources();
    if (this != 0) {
        ((SoundBufferPlayableInterface*)this)->Destroy(1);
    }
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

void SBPlayable_Stream::FreeResources(void) {
    SoundBufferPlayableLayout* self =
        PlayableState(this);

    ((SoundBufferPlayableInterface*)this)->Stop();
    ((SoundBufferPlayableInterface*)this)->Stop();
    if (self->aram_block != 0) {
        self->aram_block->Release();
        self->aram_block = 0;
    }
}

void SBPlayable_Stream::ResetValues(void) {
    SBPlayableStreamLayout* self =
        StreamState(this);

    self->ready_to_play = 0;
    self->last_read_pending = 0;
    self->voices_started = 0;
    self->play_when_ready = 0;
    self->play_flags = 0;
    self->cache_buffer0 = 0;
    self->cache_buffer1 = 0;
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

/*
 * Exact: retail file-buffer splitting, ARQ request ownership, mono/stereo
 * ring advancement, final-read callback selection, and cleanup.
 */
void SBPlayable_Stream::StreamFileRead_CallBack(
    void* buffer, unsigned long offset, int size, int error,
    int final_chunk, void* callback_data) {
    SBPlayableStreamLayout* stream;
    void (*callback)(unsigned long);
    void (*last_callback)(unsigned long);
    BOOL enabled;

    enabled = OSDisableInterrupts();
    stream = (SBPlayableStreamLayout*)callback_data;

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
            if (stream->playable.file_entry->has_secondary != 0) {
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
                    ((SoundBufferPlayableInterface*)stream)->Stop();
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
                        (&stream->cache_buffer0)[channel] +
                        block * segment_size + in_segment;
                    DCFlushRange(cursor, segment_remaining);
                    ARQPostRequest(
                        request, 0, 0, 1, (unsigned long)cursor,
                        destination, segment_remaining, callback);
                    cursor += segment_remaining;
                    if (remaining != 0) {
                        if (stream->playable.file_entry->has_secondary != 0) {
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

void SBPlayable_Stream::i_ARQCALLBACK_ArqComplete_ReturnBuffer(
    unsigned long request_address) {
    mslARQRequest* request = (mslARQRequest*)request_address;
    SBPlayableStreamLayout* stream =
        (SBPlayableStreamLayout*)request->callback_data;

    stream->pending_arq_count--;
    i_ARQCALLBACK_ReturnArqAndUserStreamBuffer(request_address);
}

void SBPlayable_Stream::i_ARQCALLBACK_ArqComplete(
    unsigned long request_address) {
    mslARQRequest* request = (mslARQRequest*)request_address;
    SBPlayableStreamLayout* stream =
        (SBPlayableStreamLayout*)request->callback_data;

    stream->pending_arq_count--;
    i_ARQCALLBACK_ReturnArq(request_address);
}

void SBPlayable_Stream::i_ARQCALLBACK_ArqComplete_LastRead(
    unsigned long request_address) {
    mslARQRequest* request = (mslARQRequest*)request_address;
    SBPlayableStreamLayout* stream =
        (SBPlayableStreamLayout*)request->callback_data;

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

int SBPlayable_Stream::IsReadyToPlay(void) {
    SBPlayableStreamLayout* self =
        StreamState(this);

    return self->ready_to_play;
}

int SBPlayable_Stream::Pause(void) {
    SBPlayableStreamLayout* stream =
        StreamState(this);
    SoundBufferPlayableLayout* self = &stream->playable;
    BOOL voice_enabled;
    int result;
    BOOL enabled = OSDisableInterrupts();
    result = -1;

    ((SoundBufferPlayableInterface*)this)
        ->StopIfDonePlaying();
    if ((self->state & 2) != 0) {
        voice_enabled = OSDisableInterrupts();

        if (self->primary_voice != 0) {
            AXSetVoiceState(self->primary_voice, 0);
            result = 0;
        }
        if (self->secondary_voice != 0) {
            AXSetVoiceState(self->secondary_voice, 0);
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

int SBPlayable_Stream::Play(unsigned long flags) {
    SBPlayableStreamLayout* stream =
        StreamState(this);
    SoundBufferPlayableLayout* self = &stream->playable;
    int result = -1;

    if ((self->state & 2) == 0) {
        BOOL enabled;

        stream->play_flags = flags;
        ((SoundBufferPlayableInterface*)this)->Slot14();
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

int SBPlayable_Stream::iPlayPrepped(void) {
    SBPlayableStreamLayout* stream =
        StreamState(this);
    SoundBufferPlayableLayout* self = &stream->playable;
    int result = -1;

    if (stream->ready_to_play == 0 ||
        stream->voices_started != 0) {
        return result;
    }

    if (self->primary_voice == 0) {
        int use_stream_loop;
        int use_loop_address;
        SPSoundEntry* primary_sound = 0;

        stream->ring_play_block = 0;
        stream->at_zero_buffer = 0;
        self->primary_voice = AXAcquireVoice(
            0x14, AcquireVoiceCallback, this);
        if (self->primary_voice == 0) {
            return result;
        }
        AXSetVoicePriority(self->primary_voice, 0x19);

        use_stream_loop = 1;
        use_loop_address = 1;
        if (stream->source_blocks_remaining == -1) {
            use_stream_loop = 0;
            if ((self->state & 1) == 0) {
                use_loop_address = 0;
            }
        }

        if (self->file_entry->has_secondary != 0) {
            self->secondary_voice = AXAcquireVoice(
                0x14, AcquireVoiceCallback, this);
            if (self->secondary_voice == 0) {
                AXFreeVoice(self->primary_voice);
                self->primary_voice = 0;
                return result;
            }
            AXSetVoicePriority(self->secondary_voice, 0x19);
        }

        if (self->file_entry->has_secondary != 0) {
            SPSoundEntry* secondary_sound = 0;

            if (self->file_entry->sound_table != 0) {
                primary_sound = SPGetSoundEntry(
                    self->file_entry->sound_table, 0);
            }
            SetVoiceSource(
                self->primary_voice, primary_sound,
                self->frequency, use_stream_loop);
            if (use_loop_address) {
                self->primary_voice->current_hi =
                    primary_sound->loop_address >> 16;
                self->primary_voice->current_lo =
                    primary_sound->loop_address;
            }
            if (self->file_entry->secondary_sound_table != 0) {
                secondary_sound = SPGetSoundEntry(
                    self->file_entry->secondary_sound_table, 0);
            }
            SetVoiceSource(
                self->secondary_voice, secondary_sound,
                self->frequency, use_stream_loop);
            if (use_loop_address) {
                self->secondary_voice->current_hi =
                    secondary_sound->loop_address >> 16;
                self->secondary_voice->current_lo =
                    secondary_sound->loop_address;
            }
            MIXInitChannel(
                self->primary_voice, 0, 0, -960, -960,
                0, 0x7F, self->mix_fader);
            MIXInitChannel(
                self->secondary_voice, 0, 0, -960, -960,
                0x7F, 0x7F, self->mix_fader);
        } else {
            if (self->file_entry->sound_table != 0) {
                primary_sound = SPGetSoundEntry(
                    self->file_entry->sound_table, 0);
            }
            SetVoiceSource(
                self->primary_voice, primary_sound,
                self->frequency, use_stream_loop);
            if (use_loop_address) {
                self->primary_voice->current_hi =
                    primary_sound->loop_address >> 16;
                self->primary_voice->current_lo =
                    primary_sound->loop_address;
            }
            MIXInitChannel(
                self->primary_voice, 0, 0, -960, -960,
                self->pan, self->volume, self->mix_fader);
        }

        if (stream->source_blocks_remaining == -1) {
            int channel;
            int channel_count = GetNumChannels();

            stream->ring_play_block = 0;
            if (stream->ring_write_block == 0) {
                stream->ax_end_block =
                    stream->ring_block_count - 1;
            } else {
                stream->ax_end_block =
                    stream->ring_write_block - 1;
            }
            stream->pending_ax_block =
                use_loop_address ? 0 : -1;

            for (channel = 0; channel < channel_count; channel++) {
                _AXVPB* voice =
                    (&self->primary_voice)[channel];
                unsigned long base =
                    (&stream->cache_buffer0)[channel] << 1;
                unsigned long loop =
                    ((unsigned long)voice->loop_hi << 16) |
                    voice->loop_lo;
                unsigned long end =
                    ((unsigned long)voice->end_hi << 16) |
                    voice->end_lo;
                unsigned long current =
                    ((unsigned long)voice->current_hi << 16) |
                    voice->current_lo;
                unsigned long sync;

                loop += base;
                sync = voice->sync | 0x10000;
                voice->loop_lo = loop;
                voice->loop_hi = loop >> 16;
                if ((sync & 0x1000) == 0) {
                    voice->sync = sync;
                }

                end += base;
                sync = voice->sync | 0x8000;
                voice->end_lo = end;
                voice->end_hi = end >> 16;
                if ((sync & 0x1000) == 0) {
                    voice->sync = sync;
                }

                if (use_loop_address) {
                    current += base;
                    sync = voice->sync | 0x4000;
                    voice->current_lo = current;
                    voice->current_hi = current >> 16;
                    if ((sync & 0x1000) == 0) {
                        voice->sync = sync;
                    }
                }
            }
        } else {
            int channel;
            int channel_count = GetNumChannels();

            stream->ax_end_block =
                stream->ring_play_block;
            stream->pending_ax_block = -1;
            for (channel = 0; channel < channel_count; channel++) {
                _AXVPB* voice =
                    (&self->primary_voice)[channel];
                unsigned long base =
                    (&stream->cache_buffer0)[channel] << 1;
                unsigned long zero =
                    g_MSL_GCN_ARAM_ZeroBase_ADPCM_Start;
                unsigned long address;
                unsigned long sync;

                address = base + 2;
                sync = voice->sync | 0x10000;
                voice->loop_lo = address;
                voice->loop_hi = address >> 16;
                if ((sync & 0x1000) == 0) {
                    voice->sync = sync;
                }

                address =
                    base + (stream->segment_size << 1) - 1;
                sync = voice->sync | 0x8000;
                voice->end_lo = address;
                voice->end_hi = address >> 16;
                if ((sync & 0x1000) == 0) {
                    voice->sync = sync;
                }

                sync = voice->sync | 0x4000;
                voice->current_hi = zero >> 16;
                voice->current_lo = zero;
                if ((sync & 0x1000) == 0) {
                    voice->sync = sync;
                }
                voice->adpcm_loop_pred_scale = 0;
                voice->adpcm_loop_yn1 = 0;
                voice->adpcm_loop_yn2 = 0;
                voice->sync |= 0x100000;
                voice->depop = 0;
                voice->sync |= 8;
                voice->loop = 0;
                sync = voice->sync | 0x2000;
                if ((sync & 0x1000) == 0) {
                    voice->sync = sync;
                }
            }
        }
    }

    if (self->primary_voice != 0) {
        stream->voices_started = 1;
        stream->crossed_stream_end = 0;
        stream->end_of_stream = 0;
        if (stream->at_zero_buffer == 0) {
            AXSetVoiceState(self->primary_voice, 1);
            if (self->secondary_voice != 0) {
                AXSetVoiceState(self->secondary_voice, 1);
            }
        }
        result = 0;
    }
    return result;
}

/*
 * Soft ceiling: Stop 97.00% -- the dual interrupt scopes, voice teardown,
 * cache release, request cancellation, state reset, and return contract are
 * retail-correct. Declaration/initializer reordering recovered retail size
 * but rotated the full NV map and regressed to 90.50%/92.94%; stop.
 */
int SBPlayable_Stream::Stop(void) {
    SBPlayableStreamLayout* stream =
        StreamState(this);
    SoundBufferPlayableLayout* self = &stream->playable;
    BOOL enabled = OSDisableInterrupts();
    int result = -1;
    BOOL voice_enabled = OSDisableInterrupts();

    if (self->primary_voice != 0) {
        AXSetVoiceState(self->primary_voice, 0);
        MIXReleaseChannel(self->primary_voice);
        AXFreeVoice(self->primary_voice);
        result = 0;
        self->primary_voice = 0;
    }
    if (self->secondary_voice != 0) {
        AXSetVoiceState(self->secondary_voice, 0);
        MIXReleaseChannel(self->secondary_voice);
        AXFreeVoice(self->secondary_voice);
        result = 0;
        self->secondary_voice = 0;
    }
    OSRestoreInterrupts(voice_enabled);
    self->state = 8;
    stream->voices_started = 0;

    voice_enabled = OSDisableInterrupts();
    stream->ready_to_play = 0;
    stream->last_read_pending = 0;
    stream->play_when_ready = 0;
    if (stream->cache_buffer0 != 0) {
        mslStreamCache_ReleaseBuffer(stream->cache_buffer0);
    }
    if (stream->cache_buffer1 != 0) {
        mslStreamCache_ReleaseBuffer(stream->cache_buffer1);
    }
    stream->cache_buffer0 = 0;
    stream->cache_buffer1 = 0;
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

void SBPlayable_Stream::StopIfDonePlaying(void) {
    SBPlayableStreamLayout* stream =
        StreamState(this);
    SoundBufferPlayableLayout* self = &stream->playable;

    if ((self->state & 2) != 0) {
        if (stream->source_blocks_remaining == -1) {
            if (stream->voices_started != 0 &&
                (self->state & 1) == 0) {
                int done = 0;

                if (self->primary_voice != 0 &&
                    (self->primary_voice->state & 1) == 0) {
                    done = 1;
                } else if (self->secondary_voice != 0 &&
                           (self->secondary_voice->state & 1) == 0) {
                    done = 1;
                }
                if (done != 0) {
                    ((SoundBufferPlayableInterface*)this)->Stop();
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
            ((SoundBufferPlayableInterface*)this)->Stop();
        }
    }
}

/*
 * Exact: retail refill, wrap, final-chunk publication, and play-when-ready
 * scheduling. The read-size local is deliberately reused first as the
 * available-block ceiling.
 */
void SBPlayable_Stream::iUpdate_MslTick(void) {
    SBPlayableStreamLayout* stream =
        StreamState(this);
    SoundBufferPlayableLayout* self = &stream->playable;

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

        ((SoundBufferPlayableInterface*)this)
            ->StopIfDonePlaying();

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
    SBPlayableStreamLayout* stream =
        StreamState(this);
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

    _AXVPB* voice = stream->playable.primary_voice;
    unsigned long sync;
    unsigned long end_sync;
    unsigned long address = stream->cache_buffer0;
    end_sync = voice->sync;
    address = end_address + (address << 1);

    sync = end_sync | 0x8000;
    voice->end_lo = address;
    voice->end_hi = address >> 16;
    if ((sync & 0x1000) == 0) {
        voice->sync = sync;
    }
    sync = voice->sync;
    address = g_MSL_GCN_ARAM_ZeroBase_ADPCM_Start;
    sync |= 0x4000;
    voice->current_lo = address;
    voice->current_hi = address >> 16;
    if ((sync & 0x1000) == 0) {
        voice->sync = sync;
    }
    sync = voice->sync;
    voice->adpcm_loop_pred_scale = 0;
    sync |= 0x100000;
    voice->adpcm_loop_yn1 = 0;
    voice->adpcm_loop_yn2 = 0;
    voice->sync = sync;
    sync = voice->sync;
    voice->depop = 0;
    sync |= 8;
    voice->sync = sync;
    sync = voice->sync;
    sync |= 0x2000;
    voice->loop = 0;
    if ((sync & 0x1000) == 0) {
        voice->sync = sync;
    }

    voice = stream->playable.secondary_voice;
    if (voice == 0) {
        return;
    }
    address = stream->cache_buffer1;
    end_sync = voice->sync;
    address = end_address + (address << 1);
    sync = end_sync | 0x8000;
    voice->end_lo = address;
    voice->end_hi = address >> 16;
    if ((sync & 0x1000) == 0) {
        voice->sync = sync;
    }
    sync = voice->sync;
    address = g_MSL_GCN_ARAM_ZeroBase_ADPCM_Start;
    sync |= 0x4000;
    voice->current_lo = address;
    voice->current_hi = address >> 16;
    if ((sync & 0x1000) == 0) {
        voice->sync = sync;
    }
    sync = voice->sync;
    voice->adpcm_loop_pred_scale = 0;
    sync |= 0x100000;
    voice->adpcm_loop_yn1 = 0;
    voice->adpcm_loop_yn2 = 0;
    voice->sync = sync;
    sync = voice->sync;
    voice->depop = 0;
    sync |= 8;
    voice->sync = sync;
    sync = voice->sync;
    sync |= 0x2000;
    voice->loop = 0;
    if ((sync & 0x1000) == 0) {
        voice->sync = sync;
    }
}

/*
 * Soft ceiling: iUpdate_AXUser ~75.87% -- the full retail dual-voice stream
 * ring/AX transition algorithm is recovered; current size is 0x5AC versus
 * retail 0x5B4. The typed inline voice-block helper restores the retail
 * optional raw-address stores and delayed per-channel cache load. MWCC 2.7
 * crashes at optimization levels 2 and 4; level 3 is the highest stable
 * scoped setting and retains readable structured C.
 */
inline int SBPlayable_Stream::iAX_GetVoiceBlock(
    _AXVPB* voice,
    int channel,
    unsigned long* raw_address) {
    SBPlayableStreamLayout* stream =
        StreamState(this);
    unsigned long address =
        ((unsigned long)voice->loop_hi << 16) +
        voice->loop_lo;
    int block;

    if (address >= g_MSL_GCN_ARAM_ZeroBase_ADPCM_Start &&
        address <= g_MSL_GCN_ARAM_ZeroBase_ADPCM_End) {
        block = -1;
    } else {
        address = (address >> 1) -
            (channel == 0 ? stream->cache_buffer0 :
                            stream->cache_buffer1);
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

void SBPlayable_Stream::iUpdate_AXUser(void) {
    SBPlayableStreamLayout* stream =
        StreamState(this);
    _AXVPB* primary;
    _AXVPB* secondary;
    unsigned long address;
    unsigned long sync;
    int primary_block;
    int secondary_block;
    int channels;
    int next_block;
    int crossed_scan_stop;
    unsigned long primary_raw_address;
    unsigned long secondary_raw_address;

    if (stream->voices_started == 0) {
        return;
    }
    if (stream->source_blocks_remaining == -1) {
        return;
    }

    primary = stream->playable.primary_voice;
    primary_block = iAX_GetVoiceBlock(
        primary, 0, &primary_raw_address);

    if (stream->playable.file_entry->has_secondary != 0) {
        channels = 2;
        secondary = stream->playable.secondary_voice;
        secondary_block = iAX_GetVoiceBlock(
            secondary, 1, &secondary_raw_address);
    } else {
        channels = 1;
    }
    if (channels != 1 && secondary_block != primary_block) {
        return;
    }

    if (primary_block == -1) {
        if (stream->end_of_stream != 0 &&
            stream->stream_end_block == stream->ax_end_block &&
            stream->end_pass_count == 0) {
            stream->end_pass_count = 1;
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

                sync = primary->sync;
                address =
                    (next_block <<
                     (stream->segment_shift + 1)) +
                    2 + (stream->cache_buffer0 << 1);
                sync |= 0x10000;
                primary->loop_lo = address;
                primary->loop_hi = address >> 16;
                if ((sync & 0x1000) == 0) {
                    primary->sync = sync;
                }
                primary->adpcm_loop_pred_scale =
                    stream->block_headers[0][next_block];
                primary->adpcm_loop_yn1 = 0;
                primary->adpcm_loop_yn2 = 0;
                primary->sync |= 0x20000;

                if (stream->playable.secondary_voice != 0) {
                    secondary =
                        stream->playable.secondary_voice;
                    sync = secondary->sync;
                    address =
                        (next_block <<
                         (stream->segment_shift + 1)) +
                        2 + (stream->cache_buffer1 << 1);
                    sync |= 0x10000;
                    secondary->loop_lo = address;
                    secondary->loop_hi = address >> 16;
                    if ((sync & 0x1000) == 0) {
                        secondary->sync = sync;
                    }
                    secondary->adpcm_loop_pred_scale =
                        stream->block_headers[1][next_block];
                    secondary->adpcm_loop_yn1 = 0;
                    secondary->adpcm_loop_yn2 = 0;
                    secondary->sync |= 0x20000;
                }

                iAX_FindNewEndBlock(primary_block);
                AXSetVoiceState(
                    stream->playable.primary_voice, 1);
                if (stream->playable.secondary_voice != 0) {
                    AXSetVoiceState(
                        stream->playable.secondary_voice, 1);
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

            primary = stream->playable.primary_voice;
            sync = primary->sync;
            address =
                (next_block << (stream->segment_shift + 1)) +
                2 + (stream->cache_buffer0 << 1);
            sync |= 0x4000;
            primary->current_lo = address;
            primary->current_hi = address >> 16;
            if ((sync & 0x1000) == 0) {
                primary->sync = sync;
            }
            sync = primary->sync;
            primary->adpcm_loop_pred_scale =
                stream->block_headers[0][next_block];
            sync |= 0x100000;
            primary->adpcm_loop_yn1 = 0;
            primary->adpcm_loop_yn2 = 0;
            primary->sync = sync;
            if (stream->stream_end_block ==
                stream->ax_end_block) {
                primary->depop = 0;
                primary->sync |= 8;
            } else {
                primary->depop = 1;
                primary->sync |= 8;
            }
            sync = primary->sync;
            sync |= 0x2000;
            primary->loop = 1;
            if ((sync & 0x1000) == 0) {
                primary->sync = sync;
            }

            secondary = stream->playable.secondary_voice;
            if (secondary != 0) {
                sync = secondary->sync;
                address =
                    (next_block <<
                     (stream->segment_shift + 1)) +
                    2 + (stream->cache_buffer1 << 1);
                sync |= 0x4000;
                secondary->current_lo = address;
                secondary->current_hi = address >> 16;
                if ((sync & 0x1000) == 0) {
                    secondary->sync = sync;
                }
                sync = secondary->sync;
                secondary->adpcm_loop_pred_scale =
                    stream->block_headers[1][next_block];
                sync |= 0x100000;
                secondary->adpcm_loop_yn1 = 0;
                secondary->adpcm_loop_yn2 = 0;
                secondary->sync = sync;
                if (stream->stream_end_block ==
                    stream->ax_end_block) {
                    secondary->depop = 0;
                    secondary->sync |= 8;
                } else {
                    secondary->depop = 1;
                    secondary->sync |= 8;
                }
                sync = secondary->sync;
                sync |= 0x2000;
                secondary->loop = 1;
                if ((sync & 0x1000) == 0) {
                    secondary->sync = sync;
                }
            }
        }
    }

    if (primary_block < stream->ring_write_block ||
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

void SBPlayable_Stream::PrepForPlay(void) {
    SBPlayableStreamLayout* stream =
        StreamState(this);

    while (stream->pending_arq_count != 0) {
    }

    BOOL enabled = OSDisableInterrupts();
    if (stream->ready_to_play == 0 &&
        stream->last_read_pending == 0) {
        do {
        stream->cache_buffer0 =
            mslStreamCache_GetStreamBuffer();
        if (stream->cache_buffer0 == 0) {
            BOOL cleanup_enabled = OSDisableInterrupts();

            stream->ready_to_play = 0;
            stream->last_read_pending = 0;
            stream->play_when_ready = 0;
            if (stream->cache_buffer0 != 0) {
                mslStreamCache_ReleaseBuffer(
                    stream->cache_buffer0);
            }
            if (stream->cache_buffer1 != 0) {
                mslStreamCache_ReleaseBuffer(
                    stream->cache_buffer1);
            }
            stream->cache_buffer0 = 0;
            stream->cache_buffer1 = 0;
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
            if (stream->playable.file_entry->has_secondary != 0) {
                stream->ring_block_count =
                    stream->cache_buffer_size / 0x2000;
                stream->cache_buffer1 =
                    mslStreamCache_GetStreamBuffer();
                if (stream->cache_buffer1 == 0) {
                    BOOL cleanup_enabled =
                        OSDisableInterrupts();

                    stream->ready_to_play = 0;
                    stream->last_read_pending = 0;
                    stream->play_when_ready = 0;
                    if (stream->cache_buffer0 != 0) {
                        mslStreamCache_ReleaseBuffer(
                            stream->cache_buffer0);
                    }
                    if (stream->cache_buffer1 != 0) {
                        mslStreamCache_ReleaseBuffer(
                            stream->cache_buffer1);
                    }
                    stream->cache_buffer0 = 0;
                    stream->cache_buffer1 = 0;
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
                stream->playable.file_entry->aram_size) {
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

void SBPlayable_Stream::FreeObject(void) {
    SoundBufferPlayableLayout* self =
        PlayableState(this);
    BOOL enabled;

    ((SoundBufferPlayableInterface*)this)->FreeResources();
    enabled = OSDisableInterrupts();
    if (self->previous != 0) {
        if ((self->previous->next = self->next) != 0) {
            self->next->previous = self->previous;
        } else {
            ms_UpdateList__20SoundBuffer_Playable.last =
                (SoundBuffer_Playable*)self->previous;
        }
    } else {
        ms_UpdateList__20SoundBuffer_Playable.first =
            (SoundBuffer_Playable*)self->next;
        if (ms_UpdateList__20SoundBuffer_Playable.first != 0) {
            self->next->previous = 0;
        } else {
            ms_UpdateList__20SoundBuffer_Playable.last = 0;
        }
    }
    OSRestoreInterrupts(enabled);
    if (self != 0) {
        ((SoundBufferPlayableInterface*)this)->Destroy(1);
    }
}

SBPlayable_Stream::~SBPlayable_Stream() {
    FreeResources();
}
