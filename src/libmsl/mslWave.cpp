#include "msl/mslWave.h"
#include "mw/mwMem.h"
#include "runtime/cmath.h"

class mslPlayable {
public:
    virtual void Slot00(void);
    virtual void FreeObject(void);     /* +0x0C */
    virtual void Slot08(void);
    virtual void Slot0C(void);
    virtual void Slot10(void);
    virtual int CanSetPan(void);       /* +0x1C */
    virtual int GetStatus(unsigned long* status); /* +0x20 */
    virtual void Slot1C(void);
    virtual int SetPitch(float pitch); /* +0x28 */
    virtual void Slot2C(void);
    virtual int SetVolume(float volume); /* +0x30 */
    virtual void Slot34(void);
    virtual void Slot38(void);
    virtual int SetPan(float pan);     /* +0x3C */
    virtual void Slot3C(void);
    virtual void Slot40(void);
    virtual void Slot44(void);
    virtual void Slot48(void);
    virtual void Prepare(int state);   /* +0x50 */

    int reference_count;               /* object +0x04 */
};

extern int SoundBufferCount;
extern int SoundBufferCountStream;
extern int SoundBufferCountStatic;
int DBMap[101];
/* DBMap's section alignment reproduces retail's four trailing .bss bytes. */

/*
 * Start one runtime wave and link it into the owning sound. Stream and
 * resident/static voices share the sound's bit-4 voice-steal policy, while
 * static voices first publish current volume/pan/pitch and prepare their
 * platform buffer.
 */
extern "C" int mslWavePlay(
    _mslSystem* system, mslRuntimeSound* sound,
    mslRuntimeWave* wave, int play_state) {
    wave->sound = sound;
    if ((wave->flags & 2) != 0) {
        wave->play_state = play_state;
        if (PlayStream(
                system, sound, wave,
                ((wave->sound->flags >> 3) & 1) ^ 1) != 0) {
            return 1;
        }
    } else {
        mslWaveUpdateVolPanPitch(system, sound, wave, play_state);
        wave->play_state = play_state;
        wave->playable->Prepare(0);
        if (PlayStatic(
                system, wave,
                ((wave->sound->flags >> 3) & 1) ^ 1) != 0) {
            return 2;
        }
    }

    if (sound != 0 && wave->previous_link == 0) {
        mslRuntimeWave* old_head = sound->waves;

        sound->waves = wave;
        wave->previous_link = &sound->waves;
        wave->next = old_head;
        if (old_head != 0) {
            old_head->previous_link = &wave->next;
        }
    }
    return 0;
}

extern "C" int mslWaveUnLoad(
    _mslSystem* system, mslRuntimeWave* wave) {
    mslRuntimeWave** previous_link = wave->previous_link;
    mslRuntimeWave* next = wave->next;

    if (previous_link != 0) {
        *previous_link = next;
        wave->previous_link = 0;
    }
    if (next != 0) {
        next->previous_link = previous_link;
        wave->next = 0;
    }

    if ((wave->flags & 2) != 0) {
        StopStream(system, wave);
    } else {
        StopStatic(system, wave);
    }

    if (wave->playable != 0) {
        SoundBufferCount--;
        if ((wave->flags & 2) != 0) {
            SoundBufferCountStream--;
        } else {
            SoundBufferCountStatic--;
        }

        mslPlayable* playable = wave->playable;
        if (--playable->reference_count == 0) {
            playable->FreeObject();
        }
        wave->playable = 0;
    }

    _mwMemFree(wave, 0, 0);
    return 0;
}

extern "C" int mslWaveUnPause(
    _mslSystem* system, mslRuntimeWave* wave) {
    if ((wave->flags & 2) != 0) {
        ContinueStream(system, wave);
    } else {
        ContinueStatic(system, wave);
    }
    return 0;
}

extern "C" int mslWavePause(
    _mslSystem* system, mslRuntimeWave* wave) {
    if ((wave->flags & 2) != 0) {
        PauseStream(system, wave);
    } else {
        PauseStatic(system, wave);
    }
    return 0;
}

extern "C" void mslWaveStop(
    _mslSystem* system, mslRuntimeWave* wave) {
    mslRuntimeWave** previous_link = wave->previous_link;
    mslRuntimeWave* next = wave->next;

    if (previous_link != 0) {
        *previous_link = next;
        wave->previous_link = 0;
    }
    if (next != 0) {
        next->previous_link = previous_link;
        wave->next = 0;
    }

    if ((wave->flags & 2) != 0) {
        StopStream(system, wave);
    } else {
        StopStatic(system, wave);
    }
}

extern "C" void mslWaveUnCopy(
    _mslSystem* system, mslRuntimeWave* wave) {
    if ((wave->flags & 2) == 0) {
        UnCopyStaticWave(system, wave);
    } else {
        UnCopyStreamWave(system, wave);
    }
}

extern "C" mslRuntimeWave* mslWaveCopy(
    _mslSystem* system, mslRuntimeWave* source,
    mslLoadedBank* bank, const char* name, int copy_flags) {
    if (source == 0) {
        return 0;
    }
    if ((source->flags & 2) == 0) {
        return CopyStaticWave(
            system, bank, source, copy_flags);
    }
    return CopyStreamWave(system, bank, name, source, copy_flags);
}

extern "C" mslRuntimeWave* mslWaveLoad(
    _mslSystem* system, mslLoadedBank* bank,
    const char* name, unsigned long flags) {
    mslRuntimeWave* wave;

    if ((flags & 2) != 0) {
        wave = LoadStreamWaveFile(system, bank, name, flags);
    } else {
        unsigned char create_playable =
            ((flags >> 6) & 1) ^ 1;

        wave = LoadStaticWaveFile(
            system, bank, name, create_playable);
    }

    if (wave == 0) {
        return 0;
    }
    wave->flags = flags;
    wave->use_count = 0;
    return wave;
}

extern "C" int mslWaveSetPitch(
    mslRuntimeWave* wave, float pitch) {
    mslPlayable* playable = wave->playable;

    playable->SetPitch(pitch);
    return 0;
}

extern "C" int mslWaveSetPan(
    mslRuntimeWave* wave, float pan) {
    mslPlayable* playable = wave->playable;
    int can_set_pan;

    if (playable != 0) {
        can_set_pan = playable->CanSetPan();
    } else {
        can_set_pan = 1;
    }

    if (can_set_pan == 1) {
        if (pan < -2.0f) {
            pan = -2.0f;
        } else if (pan > 2.0f) {
            pan = 2.0f;
        }
        wave->playable->SetPan(pan);
    }
    return 0;
}

extern "C" int mslWaveSetVol(
    mslRuntimeWave* wave, _mslSystem* system, float volume) {
    mslPlayable* playable;

    if (volume < 0.0f) {
        volume = 0.0f;
    }
    if (volume > 1.0f) {
        volume = 1.0f;
    }

    playable = wave->playable;
    playable->SetVolume(volume);
    return 0;
}

extern "C" int mslWaveUpdateStatus(mslRuntimeWave* wave) {
    unsigned long status;
    unsigned long status_flags = 0;
    mslPlayable* playable;
    int result;

    wave->flags &= ~0x38;
    playable = wave->playable;
    if (playable == 0) {
        wave->flags |= 0x20;
        return -1;
    }

    result = playable->GetStatus(&status);
    if (result < 0) {
        wave->flags |= 0x20;
        return result;
    }

    if ((status & 2) != 0) {
        status_flags |= 8;
    } else {
        status_flags |= 0x20;
    }
    if ((status & 1) != 0) {
        status_flags |= 0x10;
    }
    wave->flags |= status_flags;
    return 0;
}

extern "C" void mslCreateLogTable(void) {
    int i;

    DBMap[0] = -904;
    for (i = 1; i <= 100; i++) {
        float value = (float)i / 100.0f;
        value *= 0.7f;
        value *= value;
        int db = (int)(
            10.0f *
            (10.0f * (float)log10(value)));

        if (db < -904) {
            db = -904;
        }
        DBMap[i] = db;
    }
}

long mslWaveGetDbMapEntryRelative(float volume) {
    int index = (int)(volume * 100.0f);

    if (index < 0) {
        index = 0;
    } else if (index > 100) {
        index = 100;
    }
    return DBMap[index];
}
