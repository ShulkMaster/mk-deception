#include "msl/mslBank.h"

#include "msl/mslsupport.h"
#include "msl/mslWave.h"
#include "msl/mslgcn.h"
#include "msl/mslSound_internal.h"
#include "mw/mwMemHeap.h"
static void mslSoundDeactivate(_mslSound* sound, int immediate);

extern unsigned char g_listPoolSound[];
extern unsigned char g_listPoolAdjust[];
extern mslRuntimeSound* currentUpdateSound;

/* Verified +0x00...+0x278 prefix of the retail mslSound.o @stringBase0 pool. */
static const char stringBase0[] =
    /* +0x00 */ "Error!  Out of sound resources (MSL_MAX_SOUNDS=%d).\n\0"
    /* +0x35 */ "mslSoundGetName %x has no cmd list!\n\0"
    /* +0x5A */ "mslSoundIsPlaying: NULL sound\n\0"
    /* +0x79 */ "Track %d failed to play next in Q; skipping\n\0"
    /* +0xA6 */ "WARNING: Not unusing mbs in mslSoundDeactivate.\n\0"
    /* +0xD7 */ "PlayUnPrep: NULL sound pointer.\n\0"
    /* +0xF8 */ "PlayUnPrep: INVALID sound ID %08x.\n\0"
    /* +0x11C */ "PlayAfterPrep: NULL sound pointer.\n\0"
    /* +0x140 */ "PlayAfterPrep: INVALID sound ID %08x.\n\0"
    /* +0x167 */ "Can't find any free MSL tracks\n\0"
    /* +0x187 */ "sound->track larger than number of tracks in mslInit: %d > %d\n\0"
    /* +0x1C6 */ "SoundKick error: invalid sound pointer\n\0"
    /* +0x1EE */ "SoundKick error: invalid sound ID %08x.\n\0"
    /* +0x217 */ "Kick'd on invalid track %d\n\0"
    /* +0x233 */ "MSL: wave %s not found in sound unload\n\0"
    /* +0x25B */ "Problem!  wave use count < 0\n\0"
    /* +0x279 */ "Can't find wave %s!\n\0"
    /* +0x28E */ "Unable to load wave: [%08x]\n\0"
    /* +0x2AB */ "Unable to load wave: [%s]\n\0"
    /* +0x2C6 */ "mslSoundLoad: Wave Copy Failed\n\0"
    /* +0x2E6 */ "%s failed (%s == %08x).\n\0"
    "mslSoundSetDuckPitch\0"
    /* +0x314 */ "WRAPPED_ARG\0"
    "mslSoundSetDuckPan\0"
    "mslSoundSetDuckVol\0"
    "mslSoundSetPitch\0"
    /* +0x357 */ "mslSoundSetPan\0"
    /* +0x366 */ "mslSoundSetVol\0"
    "mslSoundGetDuckPitch\0"
    "mslSoundGetDuckPan\0"
    "mslSoundGetDuckVol\0"
    "mslSoundGetPitch\0"
    "mslSoundGetPan\0"
    "mslSoundGetVol\0"
    "mslSoundGetName\0"
    "mslSoundUnPause\0"
    "mslSoundPause\0"
    /* +0x40D */ "mslSoundStop\0"
    "mslSoundIsPlaying";

struct mslSoundListNodeSlot {
    unsigned char bytes[0x10];
};

struct mslSoundListPoolView {
    unsigned char pad00[8];
    mslRuntimeSound* sounds;
    mslSoundListNodeSlot* nodes;
};

class mslPlayableView {
public:
    virtual void Slot00(void);
    virtual void Slot04(void);
    virtual void Slot08(void);
    virtual void Slot0C(void);
    virtual int IsReadyToPlay(void);
};

struct mslRuntimeAdjustView {
    float first_time;
    unsigned char pad04[0x0C];
    float second_time;
};

extern "C" int mslSoundEnd(_mslSound* sound) {
    mslRuntimeSound* runtime_sound = (mslRuntimeSound*)sound;

    mslSoundDeactivate(sound, runtime_sound->flags & 1);
    return 0;
}

void _mslSoundStop(_mslSound* sound) {
    mslSoundDeactivate(sound, 1);
}

/*
 * Soft ceiling: mslSoundDeactivate ~99.27% -- exact retail lifecycle, size,
 * and diagnostic relocation; remaining deltas are pure NV/zero coloring.
 */
static void mslSoundDeactivate(_mslSound* sound, int immediate) {
    mslRuntimeSound* runtime_sound = (mslRuntimeSound*)sound;
    _ListNode* sound_node = 0;
    mslRuntimeWave* wave = runtime_sound->waves;
    _ListNode* adjustment;

    while (wave != 0) {
        mslRuntimeWave* next = wave->next;

        mslWaveStop(runtime_sound->system, wave);
        wave = next;
    }
    runtime_sound->waves = 0;

    adjustment = runtime_sound->adjustments;
    while (adjustment != 0) {
        ListNodeFree(
            (ListPool*)g_listPoolAdjust, ListRemove(&adjustment));
    }
    runtime_sound->adjustments = 0;

    if (immediate != 0) {
        if (runtime_sound->track >= 0) {
            runtime_sound->system->tracks[
                runtime_sound->track].sound = 0;
            runtime_sound->track = -1;
        }

        {
            mslSoundListPoolView* pool =
                (mslSoundListPoolView*)g_listPoolSound;
            int index = runtime_sound - pool->sounds;

            sound_node = (_ListNode*)&pool->nodes[index];
            sound_node = ListRemove(&sound_node);
        }
    }

    runtime_sound->flags &= ~0x8000;
    if (runtime_sound->system->sound_list_guard != 0) {
        mslUpdate(runtime_sound->system);
    }

    if (immediate != 0) {
        mslBankSoundEntry* bank_sound =
            runtime_sound->bank_sound_entry;
        _ListNode* original_node = sound_node;
        mslRuntimeSound* copied_sound =
            (mslRuntimeSound*)ListNodeData(0, original_node);

        if (copied_sound == currentUpdateSound) {
            currentUpdateSound = 0;
        }

        if (copied_sound->definition != 0) {
            int i;
            mslCmdItem* command =
                copied_sound->definition->commands;

            if (command != 0) {
                for (i = 0;
                     i < copied_sound->definition->command_count;
                     i++, command++) {
                    if (command->type == 1 &&
                        command->attached_wave != 0) {
                        mslWaveUnCopy(
                            copied_sound->system,
                            command->attached_wave);
                        command->attached_wave = 0;
                    }
                }
                _mwMemFree(
                    copied_sound->definition->commands, 0, 0);
                copied_sound->definition->commands = 0;
            }
            _mwMemFree(copied_sound->definition, 0, 0);
            copied_sound->definition = 0;
        }

        copied_sound->bank_sound_entry = 0;
        {
            _ListNode* list = original_node;

            ListNodeFree(
                (ListPool*)g_listPoolSound, ListRemove(&list));
        }

        if (bank_sound != 0) {
            mslBankSoundUnUse(bank_sound);
        } else {
            mslDebugPrintf(&stringBase0[0xA6]);
        }
    }
}

/* Exact: verified @stringBase0 prefix restores the retail diagnostic offset. */
void _mslSoundUnPause(_mslSound* sound) {
    mslRuntimeSound* runtime_sound = (mslRuntimeSound*)sound;
    int is_playing;

    if (runtime_sound == 0) {
        mslDebugPrintf(&stringBase0[0x5A]);
        is_playing = 0;
    } else if ((runtime_sound->flags & 0x8000) != 0) {
        is_playing = 1;
    } else {
        is_playing = 0;
    }

    if (is_playing != 0 &&
        runtime_sound->update_time != 0.0f) {
        float current_time = mslGetTime();
        mslRuntimeWave* wave = runtime_sound->waves;
        _ListNode* adjustment;

        while (wave != 0) {
            mslWaveUnPause(runtime_sound->system, wave);
            wave = wave->next;
        }

        runtime_sound->end_time +=
            current_time - runtime_sound->update_time;

        adjustment = runtime_sound->adjustments;
        while (adjustment != 0) {
            mslRuntimeAdjustView* adjust =
                (mslRuntimeAdjustView*)ListNodeData(0, adjustment);

            adjust->first_time +=
                current_time - runtime_sound->update_time;
            adjust->second_time +=
                current_time - runtime_sound->update_time;
            ListNext(&adjustment);
        }

        runtime_sound->update_time = 0.0f;
    }
}

/* Exact: verified @stringBase0 prefix restores the retail diagnostic offset. */
void _mslSoundPause(_mslSound* sound) {
    mslRuntimeSound* runtime_sound = (mslRuntimeSound*)sound;
    int is_playing;

    if (runtime_sound == 0) {
        mslDebugPrintf(&stringBase0[0x5A]);
        is_playing = 0;
    } else if ((runtime_sound->flags & 0x8000) != 0) {
        is_playing = 1;
    } else {
        is_playing = 0;
    }

    if (is_playing != 0) {
        float update_time = runtime_sound->update_time;

        if (!update_time) {
            mslRuntimeWave* wave;

            runtime_sound->update_time = mslGetTime();
            wave = runtime_sound->waves;
            while (wave != 0) {
                mslWavePause(runtime_sound->system, wave);
                wave = wave->next;
            }
        }
    }
}

/*
 * Soft ceiling: mslSoundPlayNow ~96.59% -- exact retail size, operations, and
 * control flow; remaining differences are track-scan GPR coloring, result
 * scheduling, and two pooled-string address instructions.
 */
extern "C" int mslSoundPlayNow(_ListNode* node) {
    mslRuntimeSound* sound =
        (mslRuntimeSound*)ListNodeData(0, node);
    unsigned long play_flags = sound->flags & ~8;
    int track_result;

    if (sound->track == -1) {
        int track;

        for (track = sound->system->track_count;
             track < 0x40; track++) {
            if (sound->system->tracks[track].sound == 0) {
                sound->track = track;
                break;
            }
        }

        if (sound->track == -1) {
            mslDebugPrintf("Can't find any free MSL tracks\n");
            track_result = -1;
        } else {
            track_result = sound->track;
        }
    } else if (sound->system->track_count <=
               (unsigned long)sound->track) {
        mslDebugPrintf(
            "sound->track larger than number of tracks in mslInit: "
            "%d > %d\n",
            sound->track, sound->system->track_count);
        track_result = -1;
    } else {
        if ((play_flags & 8) == 0) {
            int replace_result;
            mslRuntimeSound* current =
                (mslRuntimeSound*)sound->system->tracks[
                    sound->track].sound;

            if (current != 0) {
                if (current->priority > sound->priority) {
                    replace_result = -1;
                } else {
                    mslSoundDeactivate(
                        (_mslSound*)current, current->flags & 1);
                    sound->system->tracks[sound->track].sound = 0;
                    replace_result = 1;
                }
            } else {
                replace_result = 0;
            }

            if (replace_result < 0) {
                track_result = -1;
            } else {
                track_result = sound->track;
            }
        } else {
            track_result = sound->track;
        }
    }

    if (track_result < 0) {
        return 0;
    }

    sound->current_command = sound->definition->commands;
    while (sound->current_command->type != 7) {
        if (sound->current_command->type == 6) {
            sound->current_command->command_state = 0;
        }
        sound->current_command++;
    }
    sound->current_command = sound->definition->commands;

    if ((sound->flags & 8) == 0) {
        _ListNode* active_node = node;
        mslRuntimeSound* active_sound =
            (mslRuntimeSound*)ListNodeData(0, node);

        active_sound->end_time =
            active_sound->current_command->value + mslGetTime();
        active_sound->update_time = 0.0f;
        active_node = ListRemove(&active_node);
        ListInsert(
            &active_sound->system->active_sounds, active_node);
        active_sound->system->tracks[
            active_sound->track].sound =
            (_mslSound*)active_sound;
        active_sound->flags |= 0x8000;
    } else {
        mslWavePlay(
            sound->system, sound,
            sound->current_command->attached_wave, 0);
    }

    if (sound->system->sound_list_guard == 1) {
        mslUpdate(sound->system);
    }
    return 1;
}

/*
 * Soft ceiling: mslSoundAttach ~96.57% -- typed retail ownership and
 * rollback; remaining differences are loop NV coloring and tail scheduling.
 */
extern "C" int mslSoundAttach(
    mslRuntimeSound* sound, mslBankSoundEntry* bank_sound) {
    mslRuntimeSound* base_sound;
    mslBankSoundDefinition* definition;
    mslCmdItem* command;
    mslCmdItem* source_command;
    int i;

    if (sound->bank_sound_entry == bank_sound) {
        return 0;
    }

    base_sound = (mslRuntimeSound*)bank_sound->sound;
    sound->end_time = 0.0f;
    sound->bank_sound_entry = bank_sound;
    sound->adjustments = 0;
    sound->waves = 0;
    sound->update_time = 0.0f;
    sound->bank_ref_count = 0;
    sound->callback_data = (void*)1;

    definition = (mslBankSoundDefinition*)_mwMemMalloc(
        MWSOUND_HEAP, sizeof(mslBankSoundDefinition), 3, 0, 0, 0);
    sound->definition = definition;
    if (definition == 0) {
        return 1;
    }

    definition->command_count =
        base_sound->definition->command_count;
    definition->commands = (mslCmdItem*)_mwMemMalloc(
        MWSOUND_HEAP,
        definition->command_count * sizeof(mslCmdItem),
        3, 0, 0, 0);
    if (definition->commands != 0) {
        source_command = base_sound->definition->commands;
        command = definition->commands;
        for (i = 0; i < definition->command_count;
             i++, source_command++, command++) {
            command->source.offset = source_command->source.offset;
            command->target.offset = source_command->target.offset;
            command->type = source_command->type;
            command->pad09 = source_command->pad09;
            command->wave_value = source_command->wave_value;
            command->unknown0C = source_command->unknown0C;
            command->command_state = source_command->command_state;
            command->attached_wave = source_command->attached_wave;
            command->value = source_command->value;
            command->unknown1C = source_command->unknown1C;
            command->unknown20 = source_command->unknown20;
            command->unknown24 = source_command->unknown24;
            command->unknown28 = source_command->unknown28;
            command->unknown2C = source_command->unknown2C;
            if (command->source.pointer != 0) {
                command->source.pointer =
                    source_command->source.pointer;
            } else {
                command->source.pointer = 0;
            }
            if (command->target.pointer != 0) {
                command->target.pointer =
                    source_command->target.pointer;
            } else {
                command->target.pointer = 0;
            }
        }

        sound->current_command = definition->commands;
        command = sound->current_command;
        for (i = 0; i < definition->command_count; i++, command++) {
            if (command->type == 1) {
                command->attached_wave = mslWaveCopy(
                    sound->system, command->attached_wave,
                    bank_sound->owner_bank,
                    (const char*)command->source.pointer, 1);
                if (command->attached_wave == 0) {
                    mslCmdItem* rollback = sound->current_command;
                    int j;

                    for (j = 0; j < i; j++, rollback++) {
                        if (rollback->type == 1) {
                            mslWaveUnCopy(
                                sound->system,
                                rollback->attached_wave);
                        }
                        rollback->attached_wave = 0;
                    }
                    break;
                }
            }
        }

        if (i >= definition->command_count) {
            return 0;
        }
    }

    definition = sound->definition;
    if (definition != 0) {
        _mwMemFree(definition->commands, 0, 0);
        definition->commands = 0;
        _mwMemFree(definition, 0, 0);
        sound->definition = 0;
    }
    sound->bank_sound_entry = 0;
    return 1;
}

extern "C" int mslSoundIsReady(_mslSound* sound) {
    mslRuntimeSound* runtime_sound = (mslRuntimeSound*)sound;
    mslBankSoundDefinition* definition;
    int command_count;
    mslCmdItem* command;

    if (runtime_sound == 0 ||
        runtime_sound->bank_sound_entry == 0 ||
        (definition = runtime_sound->definition) == 0 ||
        definition->commands == 0) {
        return 0;
    }

    command_count = definition->command_count;
    command = definition->commands;
    while (command_count != 0) {
        if (command->type == 1) {
            mslRuntimeWave* wave = command->attached_wave;

            if (wave == 0) {
                return 0;
            }
            if ((wave->flags & 2) != 0) {
                mslPlayableView* playable =
                    (mslPlayableView*)wave->playable;

                if (playable == 0) {
                    return 0;
                }
                if (playable->IsReadyToPlay() == 0) {
                    return 0;
                }
            }
        }
        command++;
        command_count--;
    }
    return 1;
}

extern "C" void mslSoundUncommit(_mslSound* sound) {
    mslRuntimeSound* runtime_sound = (mslRuntimeSound*)sound;
    int i;
    mslCmdItem* command = runtime_sound->definition->commands;

    for (i = 0; i < runtime_sound->definition->command_count;
         i++, command++) {
        if (command->type == 1 && command->attached_wave != 0) {
            command->attached_wave->flags &= ~0x40;
        }
    }
}

/*
 * Soft ceiling: mslSoundUnCopy ~99.24% -- exact retail size and operations;
 * remaining differences are pure NV register coloring.
 */
extern "C" void mslSoundUnCopy(_ListNode* node) {
    mslCmdItem* command;
    int i;
    mslRuntimeSound* sound;

    sound = (mslRuntimeSound*)ListNodeData(0, node);

    if (sound == currentUpdateSound) {
        currentUpdateSound = 0;
    }

    if (sound->definition != 0) {
        if (sound->definition->commands != 0) {
            command = sound->definition->commands;
            for (i = 0; i < sound->definition->command_count;
                 i++, command++) {
                if (command->type == 1 &&
                    command->attached_wave != 0) {
                    mslWaveUnCopy(
                        sound->system, command->attached_wave);
                    command->attached_wave = 0;
                }
            }
            _mwMemFree(sound->definition->commands, 0, 0);
            sound->definition->commands = 0;
        }
        _mwMemFree(sound->definition, 0, 0);
        sound->definition = 0;
    }

    sound->bank_sound_entry = 0;
    {
        _ListNode* list = node;
        ListNodeFree(
            (ListPool*)g_listPoolSound, ListRemove(&list));
    }
}

/*
 * Soft ceiling: mslSoundUnLoad ~97.04% -- retail ownership and reload order;
 * remaining differences are two string-pool relocations. Direct use of
 * either verified pool offset retains the base and regresses NV coloring.
 */
extern "C" int mslSoundUnLoad(_mslSound* sound) {
    mslRuntimeSound* runtime_sound = (mslRuntimeSound*)sound;
    mslCmdItem* command;
    int i;
    _mslSystem* system;
    mslLoadedBank* bank;
    mslBankSoundDefinition* definition;

    definition = runtime_sound->definition;
    bank = runtime_sound->owner_bank;
    system = runtime_sound->system;
    command = definition->commands;

    for (i = 0; i < definition->command_count; i++, command++) {
        if (command->type == 1 && command->attached_wave != 0) {
            int release_base =
                command->attached_wave != 0 &&
                (command->attached_wave->flags & 0x40) == 0;

            mslWaveUnCopy(system, command->attached_wave);
            command->attached_wave = 0;
            if (release_base) {
                mslBankWaveEntry* bank_wave =
                    mslBankWavesFind(
                        bank, (const char*)command->source.pointer);

                if (bank_wave == 0) {
                    mslDebugPrintf(
                        "MSL: wave %s not found in sound unload\n",
                        command->source.pointer);
                } else if (bank_wave->wave != 0) {
                    mslRuntimeWave* runtime_wave =
                        bank_wave->wave;

                    runtime_wave->use_count--;
                    if (bank_wave->wave->use_count == 0) {
                        mslWaveUnLoad(system, bank_wave->wave);
                        bank_wave->wave = 0;
                    } else if (bank_wave->wave->use_count < 0) {
                        mslDebugPrintf(
                            "Problem!  wave use count < 0\n");
                    }
                }
            }
        }
    }

    {
        mslSoundListPoolView* pool =
            (mslSoundListPoolView*)g_listPoolSound;
        int index = runtime_sound - pool->sounds;
        _ListNode* node =
            (_ListNode*)&pool->nodes[index];

        ListNodeFree(
            (ListPool*)g_listPoolSound, ListRemove(&node));
    }
    return 0;
}

static inline void mslCmdsRollback(
    _mslSystem* system, mslLoadedBank* bank,
    mslBankSoundDefinition* definition) {
    int i;
    mslCmdItem* command = definition->commands;

    for (i = 0; i < definition->command_count; i++, command++) {
        if (command->type == 1 && command->attached_wave != 0) {
            unsigned char release_base = 0;

            if (command->attached_wave != 0) {
                if ((command->attached_wave->flags & 0x40) == 0) {
                    release_base = 1;
                }
            }

            mslWaveUnCopy(system, command->attached_wave);
            command->attached_wave = 0;
            if (release_base) {
                mslBankWaveEntry* bank_wave =
                    mslBankWavesFind(
                        bank, (const char*)command->source.pointer);

                if (bank_wave == 0) {
                    mslDebugPrintf(
                        "MSL: wave %s not found in sound unload\n",
                        command->source.pointer);
                } else if (bank_wave->wave != 0) {
                    mslRuntimeWave* runtime_wave =
                        bank_wave->wave;

                    runtime_wave->use_count--;
                    if (runtime_wave->use_count == 0) {
                        mslWaveUnLoad(system, bank_wave->wave);
                        bank_wave->wave = 0;
                    } else if (runtime_wave->use_count < 0) {
                        mslDebugPrintf(
                            "Problem!  wave use count < 0\n");
                    }
                }
            }
        }
    }
}

static inline void mslSoundInit(_ListNode* node, _mslSystem* system) {
    if (node != 0) {
        mslRuntimeSound* sound =
            (mslRuntimeSound*)ListNodeData(0, node);

        sound->system = system;
        sound->definition = 0;
        sound->volume = 1.0f;
        sound->pan = 0.0f;
        sound->pitch = 1.0f;
        sound->volume_scale = 1.0f;
        sound->pan_offset = 0.0f;
        sound->pitch_scale = 1.0f;
        sound->bank_ref_count = 0;
        sound->callback_data = 0;
    }
}

/*
 * Resolve every wave command against the owning bank, lazily load its base
 * wave, create the per-command runtime copy, and unwind all prior copies on
 * any failure.
 * Soft ceiling: ~94.72%, retail/current size 0x48c/0x474. Reconstructing the
 * contiguous string-pool tail, preserving the rollback flag diamond, ordering
 * the large load-success region before failure cleanup, and aligning loop
 * declarations closed the structural mismatch. Remaining differences are
 * inlined rollback GPR coloring, two load-order islands, and four objdiff
 * replacements whose instructions are identical `addi` operations but whose
 * operands name substring symbols instead of retail `stringBase0` addends.
 */
extern "C" int mslCmdsLoad(
    _mslSystem* system, mslLoadedBank* bank,
    mslBankSoundDefinition* definition, unsigned long flags) {
    int lod_flags = flags & 2;
    int i;
    mslCmdItem* command = definition->commands;

    for (i = 0; i < definition->command_count; i++, command++) {
        if (command->type == 1) {
            mslBankWaveEntry* bank_wave =
                mslBankWavesFind(
                    bank, (const char*)command->source.pointer);

            if (bank_wave == 0) {
                mslDebugPrintf(
                    &stringBase0[0x279], command->source.pointer);
                _MSL_GCN_BREAK();
                mslCmdsRollback(system, bank, definition);
                _MSL_GCN_BREAK();
                return 0;
            }

            if (bank_wave->wave == 0) {
                unsigned long wave_flags = 0;

                if ((bank_wave->flags & 1) != 0) {
                    wave_flags |= 2;
                }
                if (lod_flags == 0) {
                    wave_flags |= 0x40;
                }

                bank_wave->wave = mslWaveLoad(
                    system, bank, bank_wave->name.pointer, wave_flags);
                if (bank_wave->wave != 0) {
                    bank_wave->owner_bank = bank;
                    bank_wave->wave->use_count = 1;
                } else {
                    if ((bank_wave->name.offset & 0xf0000000) ==
                        0x20000000) {
                        mslDebugPrintf(
                            &stringBase0[0x28E],
                            bank_wave->name.offset);
                    } else {
                        mslDebugPrintf(
                            &stringBase0[0x2AB],
                            bank_wave->name.pointer);
                    }
                    _MSL_GCN_BREAK();
                    mslCmdsRollback(system, bank, definition);
                    _MSL_GCN_BREAK();
                    return 0;
                }
            } else {
                if (lod_flags == 0) {
                    bank_wave->wave->flags |= 0x40;
                }
                bank_wave->wave->use_count++;
            }

            command->attached_wave = mslWaveCopy(
                system, bank_wave->wave, bank, bank_wave->name.pointer, 0);
            if (command->attached_wave == 0) {
                _MSL_GCN_BREAK();
                mslCmdsRollback(system, bank, definition);
                _MSL_GCN_BREAK();
                mslDebugPrintf(&stringBase0[0x2C6]);
                return 0;
            }
            command->attached_wave->command_value =
                command->wave_value;
        }
    }
    return 1;
}

/*
 * Allocate and initialize a live sound node. Retail defaults are:
 * track=-1 (from the zeroed pool/list state), unit volume/pitch scales,
 * centered pan offsets, and no bank references or callback payload.
 * Soft ceiling: ~99.76% -- the retail diagnostic-then-inline null recheck is
 * exact; remaining differences are partial-TU branch and literal relocations.
 */
extern "C" _ListNode* mslSoundNew(_mslSystem* system, int unused) {
    _ListNode* node = ListNodeAlloc((ListPool*)g_listPoolSound);

    if (node == 0) {
        mslDebugPrintf(
            "Error!  Out of sound resources (MSL_MAX_SOUNDS=%d).\n",
            0x708);
    }
    mslSoundInit(node, system);
    return node;
}

/*
 * Publish a reusable bank sound only after every wave command has loaded and
 * received its private runtime copy. Retail embeds mslSoundNew here.
 * Soft ceiling: ~99.48% -- the embedded allocator and null recheck are exact;
 * remaining differences are partial-TU relocations and one member-load
 * scratch-register choice.
 */
extern "C" _mslSound* mslSoundLoad(
    _mslSystem* system, mslLoadedBank* bank,
    mslBankSoundDefinition* definition, unsigned long flags) {
    _mslSound* result = 0;

    if (mslCmdsLoad(system, bank, definition, flags) == 0) {
        return 0;
    }

    {
        _ListNode* node = mslSoundNew(system, 0);

        if (node != 0) {
            mslRuntimeSound* sound =
                (mslRuntimeSound*)ListNodeData(0, node);
            sound->definition = definition;
            sound->current_command = sound->definition->commands;
            sound->flags = flags;
            result = (_mslSound*)sound;
        }
    }
    return result;
}

extern "C" int mslSoundIsValid(unsigned long handle) {
    _ListNode* node = ListNodeFind((ListPool*)g_listPoolSound, handle);

    if (node == 0) {
        return 0;
    }

    {
        mslRuntimeSound* sound =
            (mslRuntimeSound*)ListNodeData(0, node);

        if (sound == 0 || sound->definition == 0 ||
            sound->definition->command_count == 0) {
            return 0;
        }
    }
    return 1;
}

static inline int mslSoundIsPlaying(mslRuntimeSound* sound) {
    int is_playing;

    if (sound == 0) {
        mslDebugPrintf("mslSoundIsPlaying: NULL sound\n");
        is_playing = 0;
    } else if ((sound->flags & 0x8000) != 0) {
        is_playing = 1;
    } else {
        is_playing = 0;
    }
    return is_playing;
}

/*
 * Soft ceiling: mslUpdateTracks ~97.22% -- exact retail control flow and
 * register homes; both four-byte gaps are string-pool relocations. Direct
 * pool-offset expressions retain the base across the loop and regress code.
 */
extern "C" void mslUpdateTracks(_mslSystem* system) {
    unsigned long track_index;
    int saved_guard;
    int priority;

    priority = 0;
    track_index = 0;
    saved_guard = system->sound_list_guard;
    system->sound_list_guard = 0;
    for (track_index = 0; track_index < system->track_count;
         track_index++) {
        mslRuntimeSound* sound =
            (mslRuntimeSound*)system->tracks[track_index].sound;

        if (sound != 0) {
            if (mslSoundIsPlaying(sound) != 0) {
                continue;
            }
            priority = sound->priority;
            mslSoundDeactivate(
                (_mslSound*)sound, sound->flags & 1);
        }

        {
            mslBankSoundEntry* bank_sound =
                mslQueueGet(system->tracks[track_index].queue);

            if (bank_sound != 0) {
                _ListNode* node =
                    mslBankSoundUse(bank_sound, system);

                if (node != 0) {
                    mslRuntimeSound* next_sound =
                        (mslRuntimeSound*)ListNodeData(0, node);

                    next_sound->flags |= 1;
                    next_sound->priority = priority;
                    next_sound->track = track_index;
                    asyncLoadSound(
                        system, bank_sound->owner_bank,
                        bank_sound, callbackPlay, node);
                } else {
                    mslDebugPrintf(
                        "Track %d failed to play next in Q; "
                        "skipping\n",
                        track_index);
                }
            }
        }
    }
    system->sound_list_guard = saved_guard;
}

extern "C" void mslSoundSetPan(unsigned long handle, float pan) {
    _ListNode* node = ListNodeFind((ListPool*)g_listPoolSound, handle);

    if (node == 0) {
        mslDebugPrintf(
            &stringBase0[0x2E6], &stringBase0[0x357],
            &stringBase0[0x314], handle);
    } else {
        ((mslRuntimeSound*)ListNodeData(0, node))->pan = pan;
    }
}

extern "C" void mslSoundSetVol(unsigned long handle, float volume) {
    _ListNode* node = ListNodeFind((ListPool*)g_listPoolSound, handle);

    if (node == 0) {
        mslDebugPrintf(
            &stringBase0[0x2E6], &stringBase0[0x366],
            &stringBase0[0x314], handle);
    } else {
        ((mslRuntimeSound*)ListNodeData(0, node))->volume = volume;
    }
}

extern "C" void mslSoundStop(unsigned long handle) {
    _ListNode* node = ListNodeFind((ListPool*)g_listPoolSound, handle);

    if (node == 0) {
        mslDebugPrintf(
            &stringBase0[0x2E6], &stringBase0[0x40D],
            &stringBase0[0x314], handle);
    } else {
        mslSoundDeactivate(
            (_mslSound*)ListNodeData(0, node), 1);
    }
}
