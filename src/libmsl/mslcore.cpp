#include "msl/mslBank.h"
#include "msl/mslsupport.h"
#include "msl/mslWave.h"
#include "msl/mslSound_internal.h"
#include "runtime/cstring.h"

extern unsigned char g_listPoolAdjust[];
mslRuntimeSound* currentUpdateSound;

static const char stringBase0[] =
    "NULL Callback: %s(%s)\n\0"
    "Unregistered Callback: %s(%s)\n\0"
    "mslcore.cpp\0"
    "mslDoPlay: Sound %x had NULL wave (mi->mw), ignored\n";

extern "C" float mslGetVol(_mslSystem* system) {
    return system->volume;
}

extern "C" void mslSetDuckVol(_mslSystem* system, float volume) {
    system->duck_volume = volume;
}

extern "C" void mslSetVol(_mslSystem* system, float volume) {
    system->volume = volume;
}

extern "C" void mslUnDuckAll(_mslSystem* system) {
    system->duck_volume = 1.0f;
    system->duck_pan = 0.0f;
    system->duck_pitch = 1.0f;
}

extern "C" void mslUnPauseAll(_mslSystem* system) {
    _ListNode* node;

    system->sound_list_guard = 0;
    node = system->active_sounds;
    while (node != 0) {
        _mslSound* sound = (_mslSound*)ListNodeData(0, node);
        ListNext(&node);
        _mslSoundUnPause(sound);
    }
    system->sound_list_guard = 1;
}

extern "C" void mslPauseAll(_mslSystem* system) {
    _ListNode* node;

    system->sound_list_guard = 0;
    node = system->active_sounds;
    while (node != 0) {
        _mslSound* sound = (_mslSound*)ListNodeData(0, node);
        ListNext(&node);
        _mslSoundPause(sound);
    }
    system->sound_list_guard = 1;
}

extern "C" void mslStopAll(_mslSystem* system) {
    _ListNode* node;

    system->sound_list_guard = 0;
    node = system->active_sounds;
    while (node != 0) {
        _mslSound* sound = (_mslSound*)ListNodeData(0, node);
        if (node != 0) {
            ListNext(&node);
            _mslSoundStop(sound);
        } else {
            ListNext(&node);
        }
    }
    system->sound_list_guard = 1;
}

_ListNode* mslUpdateSound(
    _mslSystem* system, _ListNode* node, float now);
/* Soft ceiling: mslUpdateAdjust ~99.28% -- float register scheduling; stop. */
_ListNode* mslUpdateAdjust(
    _mslSystem* system, _ListNode* node, float now);
struct _mslCmdItem;
int mslDoAdjust(
    _mslSound* sound, _mslCmdItem* command, float now);

static inline void updateWaveValues(
    _mslSystem* system, mslRuntimeSound* sound, mslRuntimeWave* wave) {
    float volume;
    float pan;
    float pitch;

    if (sound != 0) {
        float sound_volume = system->volume * sound->volume;
        float sound_pan = system->pan + sound->pan;
        float sound_pitch = system->pitch * sound->pitch;

        volume = wave->volume * sound_volume;
        pan = wave->pan + sound_pan;
        pitch = wave->pitch * sound_pitch;
        if (pan > 2.0f)
            pan = 2.0f;
        else if (pan < -2.0f)
            pan = -2.0f;
        if (wave->flags & 4) {
            volume *= system->duck_volume * sound->volume_scale;
            pan += system->duck_pan + sound->pan_offset;
            pitch *= system->duck_pitch * sound->pitch_scale;
        }
    } else {
        volume = system->volume * wave->volume;
        pan = system->pan + wave->pan;
        pitch = system->pitch * wave->pitch;
        if (wave->flags & 4) {
            volume *= system->duck_volume;
            pan += system->duck_pan;
            pitch *= system->duck_pitch;
        }
    }
    mslWaveSetVol(wave, system, volume);
    mslWaveSetPan(wave, pan);
    mslWaveSetPitch(wave, pitch);
}

/* Soft ceiling: mslUpdate ~97.03% -- iterator GPR coloring; stop. */
extern "C" int mslUpdate(_mslSystem* system) {
    float now = mslGetTime();
    _ListNode* node;

    EnterCriticalCodeSection_DEBUG(
        &system->critical_section, "mslcore.cpp", 701);
    node = system->active_sounds;
    while (node != 0)
        node = mslUpdateSound(system, node, now);
    node = system->active_adjustments;
    while (node != 0)
        node = mslUpdateAdjust(system, node, now);

    node = system->active_sounds;
    while (node != 0) {
        mslRuntimeSound* sound =
            (mslRuntimeSound*)ListNodeData(0, node);
        if (sound->update_time == 0.0f) {
            _ListNode* adjustment = sound->adjustments;
            mslRuntimeWave* wave;
            while (adjustment != 0)
                adjustment = mslUpdateAdjust(system, adjustment, now);
            wave = sound->waves;
            while (wave != 0) {
                mslRuntimeWave* next = wave->next;
                mslWaveUpdateStatus(wave);
                if (wave->flags & 8)
                    updateWaveValues(system, sound, wave);
                wave = next;
            }
        }
        ListNext(&node);
    }
    LeaveCriticalCodeSection_DEBUG(
        &system->critical_section, "mslcore.cpp", 721);
    mslUpdateTracks(system);
    return 0;
}

_ListNode* mslUpdateSound(
    _mslSystem* system, _ListNode* node, float now) {
    _ListNode* next = node;
    mslRuntimeSound* sound;
    int skip_wait = 0;

    if (node->state == 0) {
        ListNext(&next);
        return next;
    }
    sound = (mslRuntimeSound*)ListNodeData(0, node);
    ListNext(&next);
    currentUpdateSound = sound;

    if (sound->update_time == 0.0f && sound->callback_data != 0) {
        if (!mslSoundIsReady((_mslSound*)sound))
            sound->end_time = now + 0.01f;
        else {
            sound->end_time = now;
            sound->callback_data = 0;
        }
    }

    while (sound->current_command != 0 &&
           sound->update_time == 0.0f &&
           (now >= sound->end_time ||
            (sound->current_command->type & 0x80))) {
        mslCmdItem* command = sound->current_command;
        switch (command->type) {
        case 1:
            if (command->attached_wave == 0) {
                mslDebugPrintf(&stringBase0[0x42], sound);
            } else {
                mslRuntimeWave* wave = command->attached_wave;
                wave->volume = command->unknown20;
                wave->pan = command->unknown24;
                wave->pitch = command->unknown28;
                wave->unknown34 = command->command_state;
                wave->command_value = command->wave_value;
                if (command->wave_value != 1)
                    wave->flags |= 1;
                if (command->pad09 & 1)
                    wave->flags |= 4;
                mslWavePlay(
                    sound->system, sound, wave, command->unknown0C);
            }
            break;
        case 8:
            if (command->unknown0C <
                sound->definition->command_count) {
                mslCmdItem* target =
                    &sound->definition->commands[command->unknown0C];
                if (!(target->type & 0x80) &&
                    target->attached_wave != 0)
                    mslWaveStop(sound->system, target->attached_wave);
            }
            break;
        case 2:
            mslDoAdjust(
                (_mslSound*)sound, (_mslCmdItem*)command, now);
            break;
        case 3: {
            const char* name = (const char*)command->source.pointer;
            const char* argument = (const char*)command->target.pointer;
            system->sound_list_guard = 0;
            if (name != 0) {
                mslCallback* callback;
                for (callback = system->callbacks;
                     callback != 0; callback = callback->next) {
                    if (stricmp(name, callback->name) == 0) {
                        if (callback->function == 0)
                            mslDebugPrintf(&stringBase0[0], name, argument);
                        else
                            callback->function(argument);
                        break;
                    }
                }
                if (callback == 0)
                    mslDebugPrintf(&stringBase0[0x18], name, argument);
            }
            system->sound_list_guard = 1;
            break;
        }
        case 6:
            if (command->wave_value == 0 ||
                ++command->command_state < command->wave_value) {
                int search_index =
                    sound->current_command -
                    sound->definition->commands;

                for (;;) {
                    int marker_index = search_index - 1;
                    mslCmdItem* marker =
                        sound->definition->commands + marker_index;

                    while (marker_index >= 0 &&
                           marker->type != 5) {
                        marker_index--;
                        marker--;
                        search_index--;
                    }
                    if (marker_index < 0) {
                        command->command_state = 0;
                        break;
                    }

                    {
                        unsigned long marker_name =
                            marker->source.offset;
                        unsigned long command_name =
                            sound->current_command->source.offset;
                        int same_name;

                        if (marker_name == 0 ||
                            command_name == 0) {
                            same_name = 0;
                        } else if (
                            (marker_name & 0xF0000000) ==
                                0x20000000 ||
                            (command_name & 0xF0000000) ==
                                0x20000000) {
                            same_name = marker_name ==
                                command_name;
                        } else {
                            same_name = stricmp(
                                (const char*)marker_name,
                                (const char*)command_name) == 0;
                        }

                        search_index = marker_index;
                        if (same_name) {
                            if (sound->current_command->value <=
                                    0.0f &&
                                marker->value <= 0.0f) {
                                mslCmdItem* item;

                                skip_wait = 1;
                                for (item = marker;
                                     item !=
                                        sound->current_command;
                                     item++) {
                                    if (item->value >= 0.1f) {
                                        skip_wait = 0;
                                        break;
                                    }
                                }
                            }
                            sound->current_command = marker;
                        }
                    }
                }
            } else {
                command->command_state = 0;
            }
            break;
        case 7:
            system->sound_list_guard = 0;
            sound->current_command = 0;
            mslSoundEnd((_mslSound*)sound);
            sound = 0;
            system->sound_list_guard = 1;
            break;
        }
        if (sound == 0 || currentUpdateSound == 0)
            return next;
        if (sound->current_command != 0) {
            sound->current_command++;
            if (sound->current_command >=
                sound->definition->commands +
                sound->definition->command_count)
                sound->current_command = 0;
            else if (!(sound->current_command->type & 0x80)) {
                if (sound->current_command->value >= 0.0f)
                    sound->end_time += sound->current_command->value;
                else
                    sound->end_time = 9999999.0f;
            }
        }
        if (skip_wait)
            break;
    }
    return next;
}

extern "C" void mslWaveUpdateVolPanPitch(
    _mslSystem* system, mslRuntimeSound* sound,
    mslRuntimeWave* wave, int) {
    updateWaveValues(system, sound, wave);
}

_ListNode* mslUpdateAdjust(
    _mslSystem* system, _ListNode* node, float now) {
    _ListNode* next = node;
    mslAdjustment* adjustment =
        (mslAdjustment*)ListNodeData(0, node);
    float elapsed;
    float duration;
    float volume;
    float pan;
    float pitch;

    if (now >= adjustment->end_time)
        now = adjustment->end_time;
    if (now == adjustment->end_time ||
        adjustment->end_time <= adjustment->start_time + 0.01f) {
        volume = adjustment->end_volume;
        pan = adjustment->end_pan;
        pitch = adjustment->end_pitch;
    } else {
        elapsed = now - adjustment->start_time;
        duration = adjustment->end_time - adjustment->start_time;
        volume = adjustment->start_volume +
            elapsed * (adjustment->end_volume -
                       adjustment->start_volume) / duration;
        pan = adjustment->start_pan +
            elapsed * (adjustment->end_pan -
                       adjustment->start_pan) / duration;
        pitch = adjustment->start_pitch +
            elapsed * (adjustment->end_pitch -
                       adjustment->start_pitch) / duration;
    }

    if (adjustment->flags & 1) {
        system->duck_volume = volume;
        system->duck_pan = pan;
        system->duck_pitch = pitch;
    } else if (adjustment->flags & 2) {
        system->volume = volume;
        system->pan = pan;
        system->pitch = pitch;
    } else if (adjustment->wave != 0) {
        adjustment->wave->volume = volume;
        adjustment->wave->pan = pan;
        adjustment->wave->pitch = pitch;
    } else {
        adjustment->sound->volume = volume;
        adjustment->sound->pan = pan;
        adjustment->sound->pitch = pitch;
    }

    if (now >= adjustment->end_time) {
        ListNodeFree(
            (ListPool*)g_listPoolAdjust, ListRemove(&next));
    } else {
        ListNext(&next);
    }
    return next;
}

int mslDoAdjust(
    _mslSound* sound_arg, _mslCmdItem* command_arg, float now) {
    mslRuntimeSound* sound = (mslRuntimeSound*)sound_arg;
    mslCmdItem* command = (mslCmdItem*)command_arg;
    float start_volume;
    float start_pan;
    float start_pitch;
    _ListNode* node =
        ListNodeAlloc((ListPool*)g_listPoolAdjust);
    if (node == 0)
        return 1;

    mslAdjustment* adjustment =
        (mslAdjustment*)ListNodeData(0, node);
    _ListNode** list;
    adjustment->flags = (signed char)command->pad09;
    if (adjustment->flags & 2) {
        start_volume = sound->system->volume;
        start_pan = sound->system->pan;
        start_pitch = sound->system->pitch;
        adjustment->sound = 0;
        list = &sound->system->active_adjustments;
    } else if (adjustment->flags & 1) {
        start_volume = sound->system->duck_volume;
        start_pan = sound->system->duck_pan;
        start_pitch = sound->system->duck_pitch;
        adjustment->sound = 0;
        list = &sound->system->active_adjustments;
    } else {
        start_volume = sound->volume;
        start_pan = sound->pan;
        start_pitch = sound->pitch;
        adjustment->sound = sound;
        list = &sound->adjustments;
        if (command->unknown0C >= 0) {
            if (command->unknown0C <
                sound->definition->command_count) {
                mslCmdItem* wave_command =
                    &sound->definition->commands[command->unknown0C];
                if (wave_command->type & 0x80)
                    return 0;
                adjustment->wave = wave_command->attached_wave;
                if (adjustment->wave != 0) {
                    start_volume = adjustment->wave->volume;
                    start_pan = adjustment->wave->pan;
                    start_pitch = adjustment->wave->pitch;
                }
            } else {
                adjustment->wave = 0;
            }
        }
    }
    ListInsertAtTail(list, node);
    adjustment->start_time = now;
    adjustment->start_volume = start_volume;
    adjustment->start_pan = start_pan;
    adjustment->start_pitch = start_pitch;
    adjustment->end_time = now + command->unknown1C;
    adjustment->end_volume = command->unknown20;
    adjustment->end_pan = command->unknown24;
    adjustment->end_pitch = command->unknown28;
    return 0;
}
