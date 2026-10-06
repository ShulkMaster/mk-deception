#include "msl/mslBank.h"

#include "msl/mslsupport.h"
#include "msl/mslWave.h"
#include "msl/mslgcn.h"
#include "msl/mslSound_internal.h"
#include "msl/mslPlayable.h"
#include "mw/mwMemHeap.h"
static void mslSoundDeactivate(_mslSound* sound, int immediate);

extern mslRuntimeSound* currentUpdateSound;


static const char stringBase0[] =
     "Error!  Out of sound resources (MSL_MAX_SOUNDS=%d).\n\0"
     "mslSoundGetName %x has no cmd list!\n\0"
     "mslSoundIsPlaying: NULL sound\n\0"
     "Track %d failed to play next in Q; skipping\n\0"
     "WARNING: Not unusing mbs in mslSoundDeactivate.\n\0"
     "PlayUnPrep: NULL sound pointer.\n\0"
     "PlayUnPrep: INVALID sound ID %08x.\n\0"
     "PlayAfterPrep: NULL sound pointer.\n\0"
     "PlayAfterPrep: INVALID sound ID %08x.\n\0"
     "Can't find any free MSL tracks\n\0"
     "sound->track larger than number of tracks in mslInit: %d > %d\n\0"
     "SoundKick error: invalid sound pointer\n\0"
     "SoundKick error: invalid sound ID %08x.\n\0"
     "Kick'd on invalid track %d\n\0"
     "MSL: wave %s not found in sound unload\n\0"
     "Problem!  wave use count < 0\n\0"
     "Can't find wave %s!\n\0"
     "Unable to load wave: [%08x]\n\0"
     "Unable to load wave: [%s]\n\0"
     "mslSoundLoad: Wave Copy Failed\n\0"
     "%s failed (%s == %08x).\n\0"
    "mslSoundSetDuckPitch\0"
     "WRAPPED_ARG\0"
    "mslSoundSetDuckPan\0"
    "mslSoundSetDuckVol\0"
    "mslSoundSetPitch\0"
     "mslSoundSetPan\0"
     "mslSoundSetVol\0"
    "mslSoundGetDuckPitch\0"
    "mslSoundGetDuckPan\0"
    "mslSoundGetDuckVol\0"
    "mslSoundGetPitch\0"
    "mslSoundGetPan\0"
    "mslSoundGetVol\0"
    "mslSoundGetName\0"
    "mslSoundUnPause\0"
    "mslSoundPause\0"
     "mslSoundStop\0"
    "mslSoundIsPlaying";

extern "C" int mslSoundIsValid(unsigned long handle) {
    _ListNode* node = ListNodeFind(&g_listPoolSound, handle);

    if (node == 0) {
        return 0;
    }

    mslRuntimeSound* sound =
        (mslRuntimeSound*)ListNodeData(0, node);

    if (sound == 0 || sound->definition == 0 ||
        sound->definition->command_count == 0) {
        return 0;
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

/* TODO: [near miss] 97.47%; body agrees; two decoded-equal diagnostic pool-address addis remain. */
extern "C" void mslUpdateTracks(_mslSystem* system) {
    unsigned long track_index;
    int saved_guard;
    int priority;

    priority = 0;
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
    system->sound_list_guard = saved_guard;
}

extern "C" int mslSoundEnd(_mslSound* sound) {
    mslRuntimeSound* runtime_sound = (mslRuntimeSound*)sound;

    mslSoundDeactivate(sound, runtime_sound->flags & 1);
    return 0;
}

void _mslSoundStop(_mslSound* sound) {
    mslSoundDeactivate(sound, 1);
}

static inline void mslSoundReleaseDefinition(mslRuntimeSound* sound) {
    if (sound->definition != 0) {
        mslCmdItem* command =
            sound->definition->commands;
        int i;

        if (command != 0) {
            for (i = 0;
                 i < sound->definition->command_count;
                 i++, command++) {
                if (command->type == 1 &&
                    command->attached_wave != 0) {
                    mslWaveUnCopy(
                        sound->system,
                        command->attached_wave);
                    command->attached_wave = 0;
                }
            }
            _mwMemFree(
                sound->definition->commands, 0, 0);
            sound->definition->commands = 0;
        }
        _mwMemFree(sound->definition, 0, 0);
        sound->definition = 0;
    }
}

/* TODO: [near miss] 99.65%; definition teardown matches; retained node and
 * command cursor remain a nonvolatile register pair after honest form checks. */
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
            &g_listPoolAdjust, ListRemove(&adjustment));
    }
    runtime_sound->adjustments = 0;

    if (immediate != 0) {
        if (runtime_sound->track >= 0) {
            runtime_sound->system->tracks[
                runtime_sound->track].sound = 0;
            runtime_sound->track = -1;
        }

        ListPool* pool = &g_listPoolSound;
        int index = runtime_sound - (mslRuntimeSound*)pool->elements;

        sound_node = &pool->nodes[index];
        sound_node = ListRemove(&sound_node);
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

        mslSoundReleaseDefinition(copied_sound);

        copied_sound->bank_sound_entry = 0;
        _ListNode* list = original_node;

        ListNodeFree(
            &g_listPoolSound, ListRemove(&list));

        if (bank_sound != 0) {
            mslBankSoundUnUse(bank_sound);
        } else {
            mslDebugPrintf(&stringBase0[0xA6]);
        }
    }
}


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
            mslAdjustment* adjust =
                (mslAdjustment*)ListNodeData(0, adjustment);

            adjust->start_time +=
                current_time - runtime_sound->update_time;
            adjust->end_time +=
                current_time - runtime_sound->update_time;
            ListNext(&adjustment);
        }

        runtime_sound->update_time = 0.0f;
    }
}


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

static inline int select_sound_track(mslRuntimeSound* sound, unsigned long play_flags) {
    if (sound->track == -1) {
        int track;

        for (track = sound->system->track_count; track < 0x40; track++) {
            if (sound->system->tracks[track].sound == 0) {
                sound->track = track;
                break;
            }
        }
        if (sound->track == -1) {
            mslDebugPrintf(&stringBase0[0x167]);
            return -1;
        }
    } else {
        if (sound->system->track_count <= (unsigned long)sound->track) {
            mslDebugPrintf(&stringBase0[0x187],
                sound->track, sound->system->track_count);
            return -1;
        }
        if ((play_flags & 8) == 0) {
            int replace_result;
            mslRuntimeSound* current =
                (mslRuntimeSound*)sound->system->tracks[sound->track].sound;

            if (current != 0) {
                if (current->priority > sound->priority) {
                    replace_result = -1;
                } else {
                    mslSoundDeactivate((_mslSound*)current, current->flags & 1);
                    sound->system->tracks[sound->track].sound = 0;
                    replace_result = 1;
                }
            } else {
                replace_result = 0;
            }
            if (replace_result < 0) {
                return -1;
            }
        }
    }
    return sound->track;
}

extern "C" int mslSoundPlayNow(_ListNode* node) {
    mslRuntimeSound* sound =
        (mslRuntimeSound*)ListNodeData(0, node);

    if (select_sound_track(sound, sound->flags & ~8) < 0) {
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

/* TODO: [near miss] 99.61%; shared failure cleanup CFG recovered; base/rollback owner allocation remains. */
extern "C" int mslSoundAttach(
    mslRuntimeSound* sound, mslBankSoundEntry* bank_sound) {
    const mslRuntimeSound* base_sound;
    mslBankSoundDefinition* definition;
    mslCmdItem* source_command;
    int i;

    if (sound->bank_sound_entry == bank_sound) {
        return 0;
    }

    base_sound = (const mslRuntimeSound*)bank_sound->sound;
    sound->end_time = 0.0f;
    sound->bank_sound_entry = bank_sound;
    sound->adjustments = 0;
    sound->waves = 0;
    sound->update_time = 0.0f;
    sound->bank_ref_count = 0;
    sound->callback_data = 1;

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
        mslCmdItem* command;
        mslCmdItem* wave_command;
        source_command = base_sound->definition->commands;
        command = definition->commands;
        for (i = 0; i < definition->command_count;
             i++, command++, source_command++) {
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
        wave_command = sound->current_command;
        for (i = 0; i < definition->command_count; i++, wave_command++) {
            if (wave_command->type == 1) {
                wave_command->attached_wave = mslWaveCopy(
                    sound->system, wave_command->attached_wave,
                    bank_sound->owner_bank,
                    (const char*)wave_command->source.pointer, 1);
                if (wave_command->attached_wave == 0) {
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
                    goto failure_cleanup;
                }
            }
        }

        return 0;
    }

failure_cleanup:
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
                mslPlayable* playable = wave->playable;

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

extern "C" void mslSoundUnCopy(_ListNode* node) {
    int i;
    mslCmdItem* command;
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
    _ListNode* list = node;
    ListNodeFree(
        &g_listPoolSound, ListRemove(&list));
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

/* TODO: [breakthrough needed] 97.85%; string pooling restores diagnostic
 * instructions; reconcile mixed TU pool layout before changing its mode. */
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

    ListPool* pool = &g_listPoolSound;
    int index = runtime_sound - (mslRuntimeSound*)pool->elements;
    _ListNode* node = &pool->nodes[index];

    ListNodeFree(
        &g_listPoolSound, ListRemove(&node));
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
                    bank_wave->wave->use_count--;
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


extern "C" _ListNode* mslSoundNew(_mslSystem* system, int unused) {
    _ListNode* node = ListNodeAlloc(&g_listPoolSound);

    if (node == 0) {
        mslDebugPrintf(
            &stringBase0[0x00],
            0x708);
    }
    mslSoundInit(node, system);
    return node;
}


/* TODO: [near miss] 97.78%; rollback diagnostics need pooled stringBase0
 * literals (TU data layout); release_base colors r25 vs retail r26. */
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


extern "C" _mslSound* mslSoundLoad(
    _mslSystem* system, mslLoadedBank* bank,
    mslBankSoundDefinition* definition, unsigned long flags) {
    mslRuntimeSound* result = 0;

    if (mslCmdsLoad(system, bank, definition, flags) == 0) {
        return 0;
    }

    _ListNode* node = mslSoundNew(system, 0);

    if (node != 0) {
        result = (mslRuntimeSound*)ListNodeData(0, node);
        result->definition = definition;
        result->current_command = result->definition->commands;
        result->flags = flags;
    }
    return (_mslSound*)result;
}

extern "C" void mslSoundSetPan(unsigned long handle, float pan) {
    _ListNode* node = ListNodeFind(&g_listPoolSound, handle);

    if (node == 0) {
        mslDebugPrintf(
            &stringBase0[0x2E6], &stringBase0[0x357],
            &stringBase0[0x314], handle);
    } else {
        ((mslRuntimeSound*)ListNodeData(0, node))->pan = pan;
    }
}

extern "C" void mslSoundSetVol(unsigned long handle, float volume) {
    _ListNode* node = ListNodeFind(&g_listPoolSound, handle);

    if (node == 0) {
        mslDebugPrintf(
            &stringBase0[0x2E6], &stringBase0[0x366],
            &stringBase0[0x314], handle);
    } else {
        ((mslRuntimeSound*)ListNodeData(0, node))->volume = volume;
    }
}

extern "C" void mslSoundStop(unsigned long handle) {
    _ListNode* node = ListNodeFind(&g_listPoolSound, handle);

    if (node == 0) {
        mslDebugPrintf(
            &stringBase0[0x2E6], &stringBase0[0x40D],
            &stringBase0[0x314], handle);
    } else {
        mslSoundDeactivate(
            (_mslSound*)ListNodeData(0, node), 1);
    }
}
