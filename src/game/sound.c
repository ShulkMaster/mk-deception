#include "runtime/sound_tracker.h"

#include "game/game_info.h"
#include "math/mk_math.h"
#include "platform/gcARam.h"
#include "runtime/cam.h"
#include "runtime/mk_pdata.h"
#include "runtime/mk_proc.h"
#include "runtime/utils.h"

typedef struct SoundEntry {
    int bank_index;
    int bank;
    int sound;
    int field_0c;
    float base_volume;
    unsigned char subgroup;
    char pad15[3];
    int field_18;
} SoundEntry;

typedef struct SoundSubgroupVolume {
    float volume;
    unsigned char setting_index;
    char pad05[3];
} SoundSubgroupVolume;

typedef struct SoundRequest {
    int sound_id;
    int apply_group_volume;
    float volume;
    mslLoadedBank* bank;
} SoundRequest;

typedef struct SoundCallTable {
    int* sounds;
    int count;
} SoundCallTable;

typedef struct RandomSoundRequest {
    int* sounds;
    int count;
    unsigned char previous;
    char pad09[3];
} RandomSoundRequest;

typedef struct SoundBankData {
    mslLoadedBank* handle; /* +0x00 */
    int async_state;    /* +0x04 */
    unsigned char active; /* +0x08 */
    char pad09[3];
    char* name;         /* +0x0C */
    void (*callback)(void*); /* +0x10 */
    unsigned char callback_state; /* +0x14 */
    char pad15[3];
    int callback_field_18;   /* +0x18 */
    int* callback_bank;      /* +0x1C */
    int bank_index;          /* +0x20 */
} SoundBankData; /* 0x24 */

typedef struct SoundBankCallback {
    int field_00;
    int status;
    mslLoadedBank* handle;
    int* bank_index;
} SoundBankCallback;

typedef struct LoadedSoundBank {
    int bank_index;       /* +0x00 */
    unsigned char active; /* +0x04 */
    char pad05[3];
} LoadedSoundBank; /* 0x08 */

typedef struct SoundBankLoadMode {
    unsigned int* banks; /* +0x00 - bank/slot pairs */
    int count;           /* +0x04 */
    int transition_mode; /* +0x08 */
    int slot_list;       /* +0x0C */
    char* name;          /* +0x10 */
} SoundBankLoadMode; /* 0x14 */

typedef struct MslInitParam {
    int size;
    int flags;
    int track_count;
} MslInitParam;

typedef struct MslSystemInit {
    int size;
    int field_04;
    int field_08;
    unsigned int aram_base;
    unsigned int aram_size;
} MslSystemInit;

typedef struct CameraSoundFrame {
    char pad00[0x40];
    float position[3];
} CameraSoundFrame;

typedef struct CameraSoundObj {
    char pad00[0x20];
    CameraSoundFrame* frame;
} CameraSoundObj;

extern SoundEntry mk_sound_table[];
MslInitParam g_initDefault = {12, 1, 3};
MslSystemInit g_sysinitDefault = {20, 0, 64, 0, 0};
extern SoundBankData sbank_data[];
extern LoadedSoundBank loaded_sbank_data[];
extern SoundBankLoadMode bank_load_table[];
int last_sound_list_issued;
extern SoundSubgroupVolume subgroup_volume[];
extern float game_settings[];
extern int mode_of_play;
extern int mk_plyr_sound_table[];
extern int mk_foot_sound_table[7][4];
int p1_char_fvoice;
int p2_char_fvoice;
extern SoundCallTable foot_call_table[];
extern SoundCallTable voice_call_table[];
extern SoundCallTable hit_call_table[];
extern SoundCallTable pf_hit_call_table[];
extern RandomSoundRequest random_sound_request[];

#include "src/game/sound_call_tables.inc"
#include "src/game/sound_bank_data.inc"

int can_big_boss_make_special_vo_call(unsigned int cooldown_ticks);
int snd_calculate_volume(SoundRequest* request);
float get_pan_value(const Vec* position);

MslSoundHandle mslBankPlayVol(
    mslLoadedBank* bank, int sound, int field_0c, int priority, float volume,
    int flags);
MslSoundHandle mslBankPlayVolPanPitch(
    mslLoadedBank* bank, int sound, int field_0c, int priority, float volume,
    float pan, float pitch,
    int field_18);
int mslSoundIsValid(MslSoundHandle handle);
void mslSoundSetVol(MslSoundHandle handle, float volume);
void mslSoundSetPan(MslSoundHandle handle, float pan);
void mslSoundStop(MslSoundHandle handle);
void mslSetDuckVol(_mslSystem* system, float volume);
void mslUnPauseAll(_mslSystem* system);
void mslPauseAll(_mslSystem* system);

_mslSystem* msi;
int current_sound_shuffle_state;
int current_konq_sound_shuffle_state;
int sounds_muted;
MslSoundHandle bgnd_music_ptr1;
MslSoundHandle bgnd_music_ptr2;
int gap_08_80510D94_sbss;

void yinyang_finish_music(void);
void yinyang_stop_music(void);
void yinyang_start_music(void);

int get_indirect_bank(unsigned int bank);
void unload_banks_not_on_list(int mode);
void load_banks_on_list_async(int mode);
void unload_slots_not_on_list(int slot_list);
int mslBankLoadAsyncCancelNamed(char* name);
void mslBankUnLoad(mslLoadedBank* bank);
float mslGetVol(_mslSystem* system);
void mslSetVol(_mslSystem* system, float volume);
void mslStopAll(_mslSystem* system);
_mslSystem* mslInit(MslInitParam* init, MslSystemInit* system_init);
void mslSetWavePath(_mslSystem* system, const char* path);
char* strcat(char* dest, const char* src);
void mslBankLoadAsync(void* system, int flags, char* name, void* callback);
void check_and_load_sound_bank_async(int bank, int slot);
void* get_konquest_region_table(void);
void lsba_callbank(SoundBankCallback* callback);

/* Soft ceiling: dk_voice_call ~92% - branch emission and request stack-slot ordering. */
void dk_voice_call(int voice_group, unsigned int cooldown_ticks) {
    int group2[3] = {0x1C4, 0x1C4, 0x1C5};
    int group1[3] = {0x1B4, 0x1B5, 0x1B4};
    int group0[3] = {0x1C0, 0x1BF, 0x1C0};
    SoundRequest request0;
    SoundRequest request1;
    SoundRequest request2;
    unsigned int choice;
    int sound_id;

    choice = randu0(3) & 0xFFFF;
    if (!can_big_boss_make_special_vo_call(cooldown_ticks)) {
        return;
    }

    if (voice_group == 1) {
        sound_id = group1[choice];
        if (sound_id != -1 && sound_id >= 0 && sound_id < 0x1C0C) {
            if (sound_id != -1 && sound_id >= 0 && sound_id < 0x1C0C) {
                request1.sound_id = sound_id;
                request1.volume = 1.0f;
                request1.apply_group_volume = 1;
                if (snd_calculate_volume(&request1)) {
                    SoundEntry* entry = &mk_sound_table[sound_id];
                    mslBankPlayVol(
                        request1.bank, entry->bank, entry->sound, entry->field_0c,
                        request1.volume, entry->field_18);
                }
            }
        }
    } else if (voice_group < 1) {
        if (voice_group >= 0) {
            sound_id = group0[choice];
            if (sound_id != -1 && sound_id >= 0 && sound_id < 0x1C0C) {
                if (sound_id != -1 && sound_id >= 0 && sound_id < 0x1C0C) {
                    request0.sound_id = sound_id;
                    request0.volume = 1.0f;
                    request0.apply_group_volume = 1;
                    if (snd_calculate_volume(&request0)) {
                        SoundEntry* entry = &mk_sound_table[sound_id];
                        mslBankPlayVol(
                            request0.bank, entry->bank, entry->sound, entry->field_0c,
                            request0.volume, entry->field_18);
                    }
                }
            }
        }
    } else if (voice_group < 3) {
        sound_id = group2[choice];
        if (sound_id != -1 && sound_id >= 0 && sound_id < 0x1C0C) {
            if (sound_id != -1 && sound_id >= 0 && sound_id < 0x1C0C) {
                request2.sound_id = sound_id;
                request2.volume = 1.0f;
                request2.apply_group_volume = 1;
                if (snd_calculate_volume(&request2)) {
                    SoundEntry* entry = &mk_sound_table[sound_id];
                    mslBankPlayVol(
                        request2.bank, entry->bank, entry->sound, entry->field_0c,
                        request2.volume, entry->field_18);
                }
            }
        }
    }
}

/*
 * Track positional sounds, update valid live handles, and restart expired
 * handles with the same attenuation/pan state.
 * Soft ceiling: ~88.50% -- nested retail ID validation and shared attenuation
 * lifetime are recovered; remaining differences are loop-wide GPR/FPR
 * coloring and camera pointer-wrapper branch emission.
 */
float p_track_sound(void) {
    SoundTrackerPdata* pdata;
    CameraObj* camera;
    CameraSoundFrame* camera_frame;
    MkPtr* node;

    pdata = (SoundTrackerPdata*)pdata_of_proc(aproc);
    if (pdata == 0) {
        return -1.0f;
    }

    camera = camera_item.node;
    if (camera == 0 || camera->hdr.instance != camera_item.instance) {
        return -1.0f;
    }
    camera_frame = ((CameraSoundObj*)camera)->frame;

    node = *pdata->sound_list;
    while (node != 0) {
        TrackedSound* sound;
        MkPtr* next;
        float pan;
        float distance;
        float volume;
        SoundRequest update_request;
        SoundRequest play_request;

        sound = (TrackedSound*)node->hdr;
        if (node->instance != sound->hdr.instance) {
            next = node->next;
            node->hdr = 0;
            destroy_mkptr(node);
            node = next;
            continue;
        }

        if (sound->tracking_enabled != 0) {
            distance = dist_v3_to_v3((Vec*)&sound->pos_x, (Vec*)camera_frame->position);
            if (distance < sound->max_dist) {
                if (sound->out_of_range != 0) {
                    sound->out_of_range = 0;
                }
                if (sound->positional_pan != 0) {
                    pan = get_pan_value(&sound->pos);
                } else {
                    pan = 0.0f;
                }
                volume =
                    1.0f - ((distance - sound->min_dist) / (sound->max_dist - sound->min_dist));

                if (mslSoundIsValid(sound->sound_handle)) {
                    MslSoundHandle handle;

                    handle = sound->sound_handle;
                    update_request.sound_id = sound->sound_id;
                    if (handle != 0 && update_request.sound_id != -1 &&
                        update_request.sound_id >= 0 && update_request.sound_id < 0x1C0C) {
                        update_request.volume = volume;
                        update_request.apply_group_volume = 0;
                        if (snd_calculate_volume(&update_request)) {
                            if (pan < -2.0f) {
                                pan = -2.0f;
                            }
                            if (pan > 2.0f) {
                                pan = 2.0f;
                            }
                            if (mslSoundIsValid(handle)) {
                                mslSoundSetVol(handle, update_request.volume);
                                mslSoundSetPan(handle, pan);
                            }
                        }
                    }
                } else {
                    int sound_id;
                    MslSoundHandle handle;

                    sound_id = sound->sound_id;
                    if (sound_id != -1 && sound_id >= 0 && sound_id < 0x1C0C) {
                        handle = 0;
                        if (sound_id != -1 && sound_id >= 0 &&
                            sound_id < 0x1C0C) {
                            SoundEntry* entry;

                            play_request.sound_id = sound_id;
                            play_request.apply_group_volume = 1;
                            play_request.volume = volume;
                            if (snd_calculate_volume(&play_request)) {
                                if (pan < -2.0f) {
                                    pan = -2.0f;
                                }
                                if (pan > 2.0f) {
                                    pan = 2.0f;
                                }
                                entry = &mk_sound_table[sound_id];
                                handle = mslBankPlayVolPanPitch(
                                    play_request.bank, entry->bank,
                                    entry->sound, entry->field_0c,
                                    play_request.volume, pan, 1.0f,
                                    entry->field_18);
                            }
                        }
                    } else {
                        handle = 0;
                    }
                    sound->sound_handle = handle;
                }
            } else if (sound->out_of_range == 0) {
                MslSoundHandle handle;

                handle = sound->sound_handle;
                if (handle != 0 && mslSoundIsValid(handle)) {
                    mslSoundStop(handle);
                }
                sound->out_of_range = 1;
            }
        } else if (mslSoundIsValid(sound->sound_handle)) {
            MslSoundHandle handle;

            handle = sound->sound_handle;
            if (handle != 0 && mslSoundIsValid(handle)) {
                mslSoundStop(handle);
            }
            sound->out_of_range = 1;
        }
        node = node->next;
    }
    return 1.0f;
}

void stop_tracked_sound(MkPtr** list_head, TrackedSound* sound) {
    MslSoundHandle handle;

    if (sound != 0) {
        handle = sound->sound_handle;
        if (handle != 0 && mslSoundIsValid(handle)) {
            mslSoundStop(handle);
        }
        sound->sound_handle = 0;
        mk_pull_discard(&sound->hdr, list_head);
    }
}

void make_new_tracked_sound(MkPtr** list_head, TrackedSound* sound) {
    if (sound != 0) {
        mk_insert(&sound->hdr, list_head);
    }
}

TrackedSound* get_sound_tracker_data(void) {
    TrackedSound* sound;

    sound = (TrackedSound*)get_mkhdr_generic(sizeof(TrackedSound));
    if (sound != 0) {
        sound->sound_handle = 0;
        sound->tracking_enabled = 0;
    }
    return sound;
}

void stop_sound_tracking_process(MkPtr** sound_list) {
    destroy_list(sound_list);
    destroy_mkprocs_pid(0x8227);
}

void start_sound_tracking_process(MkPtr** sound_list) {
    typedef MkProc* (*CreateTrackerProcFn)(
        int, int, MkProcEntryFn, int, MkHdr**);
    SoundTrackerPdata* pdata;
    MkProc* proc;

    proc = ((CreateTrackerProcFn)_create_mkproc_generic_tinystack)(
        0x8227, 0x1F, p_track_sound, sizeof(SoundTrackerPdata), (MkHdr**)&pdata);
    if (proc != 0 && pdata != 0) {
        pdata->sound_list = sound_list;
    }
}

void duck_sounds(float volume) {
    if (volume < 0.0f) {
        volume = 0.0f;
    }
    if (volume > 1.0f) {
        volume = 1.0f;
    }
    mslSetDuckVol(msi, volume);
}

void unpause_all_game_sounds(void) {
    if (msi != 0) {
        mslUnPauseAll(msi);
    }
}

void pause_all_game_sounds(void) {
    if (msi != 0) {
        mslPauseAll(msi);
    }
}

void unmute_all_game_sounds(void) {
    if (msi != 0 && sounds_muted != 0) {
        mslSetDuckVol(msi, 1.0f);
        sounds_muted = 0;
    }
}

void mute_all_game_sounds(void) {
    if (msi != 0 && sounds_muted == 0) {
        mslSetDuckVol(msi, 0.0f);
        sounds_muted = 1;
    }
}

/* Soft ceiling: finish_music ~96% - range-branch sense and one handle reload. */
void finish_music(void) {
    MslSoundHandle handle;
    int sound_id;
    SoundRequest request;

    if (g_game_info.bgnd_id >= 0 && g_game_info.bgnd_id < 0x23) {
        if (g_game_info.bgnd_id == 0x15) {
            yinyang_finish_music();
        } else {
            if (bgnd_music_ptr1 != 0) {
                handle = bgnd_music_ptr1;
                if (handle != 0 && mslSoundIsValid(handle)) {
                    mslSoundStop(handle);
                }
                bgnd_music_ptr1 = 0;
            }

            sound_id = g_game_info.section->finish_music_id;
            handle = bgnd_music_ptr1;
            if (sound_id > -1) {
                if (sound_id == -1) {
                    handle = 0;
                } else if (sound_id < 0 || sound_id >= 0x1C0C) {
                    handle = 0;
                } else {
                    handle = 0;
                    if (sound_id != -1) {
                        if (sound_id < 0 || sound_id >= 0x1C0C) {
                            handle = 0;
                        } else {
                            SoundEntry* entry;

                            request.sound_id = sound_id;
                            request.volume = 1.0f;
                            request.apply_group_volume = 1;
                            if (snd_calculate_volume(&request)) {
                                entry = &mk_sound_table[sound_id];
                                handle = mslBankPlayVol(
                                    request.bank, entry->bank, entry->sound, entry->field_0c,
                                    request.volume, entry->field_18);
                            }
                        }
                    }
                }
            }
            bgnd_music_ptr1 = handle;
        }
    }
}

/* Soft ceiling: end_music ~96% - range-branch sense and one handle reload. */
void end_music(void) {
    MslSoundHandle handle;
    int sound_id;
    SoundRequest request;

    if (g_game_info.bgnd_id >= 0 && g_game_info.bgnd_id < 0x23) {
        if (g_game_info.bgnd_id == 0x15) {
            yinyang_stop_music();
        } else {
            if (bgnd_music_ptr1 != 0) {
                handle = bgnd_music_ptr1;
                if (handle != 0 && mslSoundIsValid(handle)) {
                    mslSoundStop(handle);
                }
                bgnd_music_ptr1 = 0;
            }

            if (g_game_info.section->start_music_callback != 0) {
                g_game_info.section->end_music_callback();
            } else {
                sound_id = g_game_info.section->end_music_id;
                handle = bgnd_music_ptr1;
                if (sound_id > -1) {
                    if (sound_id == -1) {
                        handle = 0;
                    } else if (sound_id < 0 || sound_id >= 0x1C0C) {
                        handle = 0;
                    } else {
                        handle = 0;
                        if (sound_id != -1) {
                            if (sound_id < 0 || sound_id >= 0x1C0C) {
                                handle = 0;
                            } else {
                                SoundEntry* entry;

                                request.sound_id = sound_id;
                                request.volume = 1.0f;
                                request.apply_group_volume = 1;
                                if (snd_calculate_volume(&request)) {
                                    entry = &mk_sound_table[sound_id];
                                    handle = mslBankPlayVol(
                                        request.bank, entry->bank, entry->sound,
                                        entry->field_0c, request.volume, entry->field_18);
                                }
                            }
                        }
                    }
                }
                bgnd_music_ptr1 = handle;
            }
        }
    }
}

void stop_tunes(void) {
    MslSoundHandle handle;

    if (bgnd_music_ptr1 != 0) {
        handle = bgnd_music_ptr1;
        if (handle != 0 && mslSoundIsValid(handle)) {
            mslSoundStop(handle);
        }
        bgnd_music_ptr1 = 0;
    }

    if (bgnd_music_ptr2 != 0) {
        handle = bgnd_music_ptr2;
        if (handle != 0 && mslSoundIsValid(handle)) {
            mslSoundStop(handle);
        }
        bgnd_music_ptr2 = 0;
    }
}

/* Soft ceiling: start_tunes ~96% - range branches and GPR coloring. */
void start_tunes(void) {
    SoundRequest secondary_request;
    SoundRequest primary_request;
    MslSoundHandle handle;
    int sound_id;

    if (g_game_info.bgnd_id >= 0 && g_game_info.bgnd_id < 0x23) {
        if (g_game_info.bgnd_id == 0x15) {
            yinyang_start_music();
        } else {
            if (g_game_info.pselect.field_1f4 < 2) {
                sound_id = g_game_info.section->music_id_round_0_1;
            } else {
                sound_id = g_game_info.section->music_id_round_2_plus;
            }

            if (g_game_info.section->start_music_callback != 0) {
                g_game_info.section->start_music_callback();
            } else if (sound_id > -1) {
                handle = bgnd_music_ptr1;
                if (handle != 0) {
                    if (handle != 0 && mslSoundIsValid(handle)) {
                        mslSoundStop(handle);
                    }
                    bgnd_music_ptr1 = 0;
                }

                if (sound_id == -1) {
                    handle = 0;
                } else if (sound_id < 0 || sound_id >= 0x1C0C) {
                    handle = 0;
                } else {
                    handle = 0;
                    if (sound_id != -1) {
                        if (sound_id < 0 || sound_id >= 0x1C0C) {
                            handle = 0;
                        } else {
                            SoundEntry* entry;

                            primary_request.sound_id = sound_id;
                            primary_request.volume = 1.0f;
                            primary_request.apply_group_volume = 1;
                            if (snd_calculate_volume(&primary_request)) {
                                entry = &mk_sound_table[sound_id];
                                handle = mslBankPlayVol(
                                    primary_request.bank, entry->bank, entry->sound,
                                    entry->field_0c, primary_request.volume, entry->field_18);
                            }
                        }
                    }
                }
                bgnd_music_ptr1 = handle;
            }

            if (!mslSoundIsValid(bgnd_music_ptr2)) {
                sound_id = g_game_info.section->secondary_music_id;
                if (sound_id > -1) {
                    if (sound_id == -1) {
                        handle = 0;
                    } else if (sound_id < 0 || sound_id >= 0x1C0C) {
                        handle = 0;
                    } else {
                        handle = 0;
                        if (sound_id != -1) {
                            if (sound_id < 0 || sound_id >= 0x1C0C) {
                                handle = 0;
                            } else {
                                SoundEntry* entry;

                                secondary_request.sound_id = sound_id;
                                secondary_request.volume = 1.0f;
                                secondary_request.apply_group_volume = 1;
                                if (snd_calculate_volume(&secondary_request)) {
                                    entry = &mk_sound_table[sound_id];
                                    handle = mslBankPlayVol(
                                        secondary_request.bank, entry->bank, entry->sound,
                                        entry->field_0c, secondary_request.volume,
                                        entry->field_18);
                                }
                            }
                        }
                    }
                    bgnd_music_ptr2 = handle;
                }
            }
        }
    }
}

float snd_get_game_vol(void) {
    return mslGetVol(msi);
}

void snd_set_game_vol(float volume) {
    if (volume >= 0.0f && volume <= 1.0f) {
        mslSetVol(msi, volume);
    }
}

#define UNLOAD_BANK_LIST(filename)                                                     \
    do {                                                                               \
        SoundBankLoadMode* load_mode = &bank_load_table[0];                        \
        int list_index;                                                                \
                                                                                       \
        for (list_index = 0; list_index < load_mode->count; list_index++) {             \
            unsigned int raw_bank = load_mode->banks[list_index * 2];                  \
            int bank;                                                                  \
                                                                                       \
            switch (raw_bank >> 24) {                                                  \
            case 0:                                                                    \
                bank = raw_bank;                                                       \
                break;                                                                 \
            case 1:                                                                    \
                bank = get_indirect_bank(raw_bank);                                    \
                break;                                                                 \
            case 0xFF:                                                                 \
            default:                                                                   \
                bank = -1;                                                             \
                break;                                                                 \
            }                                                                          \
                                                                                       \
            if (bank != -1) {                                                          \
                int use_count = 0;                                                     \
                int loaded_index;                                                      \
                                                                                       \
                for (loaded_index = 0; loaded_index < 0x1D; loaded_index++) {           \
                    if (loaded_sbank_data[loaded_index].bank_index == bank) {       \
                        use_count++;                                                   \
                    }                                                                  \
                }                                                                      \
                                                                                       \
                if (use_count == 1 && sbank_data[bank].handle != 0 &&               \
                    bank != -1 && bank >= 0 && bank < 0x88) {                          \
                    SoundBankData* bank_data = &sbank_data[bank];                   \
                                                                                       \
                    (filename)[0] = '\0';                                              \
                    strcat((filename), bank_data->name);                               \
                    strcat((filename), ".msb");                                        \
                    if (bank_data->handle != 0) {                                      \
                        mslBankUnLoad(bank_data->handle);                              \
                        bank_data->handle = 0;                                         \
                    } else if (bank_data->async_state == 1) {                          \
                        int cancel_status = mslBankLoadAsyncCancelNamed((filename));    \
                                                                                       \
                        switch (cancel_status) {                                       \
                        case 1:                                                        \
                            bank_data->handle = 0;                                     \
                            break;                                                     \
                        case 0:                                                        \
                            if (bank_data->handle != 0) {                              \
                                mslBankUnLoad(bank_data->handle);                      \
                            }                                                          \
                            bank_data->handle = 0;                                     \
                            break;                                                     \
                        default:                                                       \
                            bank_data->handle = 0;                                     \
                            break;                                                     \
                        }                                                              \
                    }                                                                  \
                }                                                                      \
                loaded_sbank_data[bank].bank_index = -1;                           \
                loaded_sbank_data[bank].active = 0;                                \
            }                                                                          \
        }                                                                              \
    } while (0)

/*
 * Breakthrough needed: setup_sound_bank_list_by_mode ~77.93%. Retail keeps a
 * three-way async-cancel diamond and bottom-tested 29-entry walks. Each
 * natural structured spelling can crash MWCC 2.7 (exit 159) in this
 * duplicated 1 KB body. The explicit cancel-result switch and lower filename
 * buffer recover the retail frame and diamonds; current code is 0x3D0 versus
 * retail 0x404. Retain the readable equivalent until a new source shape
 * closes the register/loop gap.
 */
void setup_sound_bank_list_by_mode(int mode, int transition_mode) {
    char mode2_filename[0x98];
    char mode1_filename[0xA0];

    if (mode < 0 || mode >= 0xF || transition_mode < 0 || transition_mode >= 3) {
        return;
    }

    last_sound_list_issued = mode;
    switch (transition_mode) {
    case 0:
        unload_banks_not_on_list(mode);
        load_banks_on_list_async(mode);
        break;
    case 1:
        UNLOAD_BANK_LIST(mode1_filename);
        load_banks_on_list_async(mode);
        break;
    case 2:
        UNLOAD_BANK_LIST(mode2_filename);
        if (bank_load_table[mode].slot_list != -1) {
            unload_slots_not_on_list(bank_load_table[mode].slot_list);
        }
        load_banks_on_list_async(mode);
        break;
    }
}

#undef UNLOAD_BANK_LIST

void setup_sound_banks(int mode) {
    int transition_mode;

    if (mode < 0 || mode >= 0xF) {
        return;
    }

    transition_mode = bank_load_table[mode].transition_mode;
    if (transition_mode < 0 || transition_mode >= 3) {
        return;
    }
    setup_sound_bank_list_by_mode(mode, transition_mode);
}

/* Soft ceiling: load_banks_on_list_async ~76% - loop GPR allocation. */
void load_banks_on_list_async(int mode) {
    SoundBankLoadMode* load_mode = &bank_load_table[mode];
    char filename[0x98];
    int i;

    for (i = 0; i < load_mode->count; i++) {
        unsigned int raw_bank = load_mode->banks[i * 2];
        unsigned int slot = load_mode->banks[i * 2 + 1];
        int bank;

        switch (raw_bank >> 24) {
        case 0:
            bank = raw_bank;
            break;
        case 1:
            bank = get_indirect_bank(raw_bank);
            break;
        case 0xFF:
        default:
            bank = -1;
            break;
        }

        if (bank != -1) {
            SoundBankData* bank_data = &sbank_data[bank];

            if (bank_data->handle == 0 && bank != -1) {
                filename[0] = '\0';
                strcat(filename, bank_data->name);
                strcat(filename, ".msb");
                if (bank_data->handle == 0 && bank_data->async_state == 0) {
                    bank_data->handle = 0;
                    bank_data->async_state = 1;
                    bank_data->callback = (void (*)(void*))lsba_callbank;
                    bank_data->callback_field_18 = 0;
                    bank_data->callback_bank = &bank_data->bank_index;
                    bank_data->callback_state = 0;
                    mslBankLoadAsync(msi, 0, filename, &bank_data->callback);
                }
            }
            loaded_sbank_data[slot].bank_index = bank;
            loaded_sbank_data[slot].active = bank_data->active;
        }
    }
}

#define CANCEL_OR_UNLOAD_BANK(bank_data, filename)                               \
    do {                                                                          \
        if ((bank_data)->handle != 0) {                                           \
            mslBankUnLoad((bank_data)->handle);                                   \
            (bank_data)->handle = 0;                                              \
        } else if ((bank_data)->async_state == 1) {                               \
            int cancel_status = mslBankLoadAsyncCancelNamed((filename));          \
                                                                                  \
            if (cancel_status == 0 && (bank_data)->handle != 0) {                 \
                mslBankUnLoad((bank_data)->handle);                               \
            }                                                                     \
            (bank_data)->handle = 0;                                              \
        }                                                                         \
    } while (0)

/* Soft ceiling: unload_banks_not_on_list ~72% - nested-loop GPR allocation. */
void unload_banks_not_on_list(int mode) {
    SoundBankLoadMode* load_mode = &bank_load_table[mode];
    char filename[0xA0];
    int bank;

    for (bank = 0; bank < 0x88; bank++) {
        SoundBankData* bank_data = &sbank_data[bank];

        if (bank_data->handle != 0 || bank_data->async_state == 1) {
            int present = 0;
            int i;

            for (i = 0; i < load_mode->count; i++) {
                unsigned int raw_bank = load_mode->banks[i * 2];
                int listed_bank;

                switch (raw_bank >> 24) {
                case 0:
                    listed_bank = raw_bank;
                    break;
                case 1:
                    listed_bank = get_indirect_bank(raw_bank);
                    break;
                case 0xFF:
                default:
                    listed_bank = -1;
                    break;
                }
                if (listed_bank == bank) {
                    present = 1;
                    break;
                }
            }

            if (!present && bank != -1 && bank >= 0 && bank < 0x88) {
                filename[0] = '\0';
                strcat(filename, bank_data->name);
                strcat(filename, ".msb");
                CANCEL_OR_UNLOAD_BANK(bank_data, filename);
            }
        }
    }
}

/* Soft ceiling: unload_slots_not_on_list ~72% - nested-loop GPR allocation. */
void unload_slots_not_on_list(int mode) {
    SoundBankLoadMode* load_mode = &bank_load_table[mode];
    char filename[0x9C];
    int slot;

    for (slot = 0; slot < 0x1D; slot++) {
        LoadedSoundBank* loaded = &loaded_sbank_data[slot];

        if (loaded->bank_index != -1) {
            int present = 0;
            int i;

            for (i = 0; i < load_mode->count; i++) {
                if ((int)load_mode->banks[i * 2 + 1] == slot) {
                    present = 1;
                    break;
                }
            }

            if (!present && slot >= 0 && slot < 0x1D) {
                int bank = loaded->bank_index;
                int use_count = 0;

                for (i = 0; i < 0x1D; i++) {
                    if (loaded_sbank_data[i].bank_index == bank) {
                        use_count++;
                    }
                }
                if (use_count == 1 && bank != -1 && bank >= 0 && bank < 0x88) {
                    SoundBankData* bank_data = &sbank_data[bank];

                    filename[0] = '\0';
                    strcat(filename, bank_data->name);
                    strcat(filename, ".msb");
                    CANCEL_OR_UNLOAD_BANK(bank_data, filename);
                }
                loaded->bank_index = -1;
                loaded->active = 0;
            }
        }
    }
}

#define SOUND_BANK_WAIT_SLEEP()                  \
    do {                                         \
        typedef void (*SleepFn)(void*);          \
        void* vtable;                            \
                                                 \
        _mkproc_sleep_ticks = 1.0f;              \
        vtable = aproc->vtbl;                    \
        ((SleepFn*)vtable)[6](vtable);            \
    } while (0)

/* Soft ceiling: wait_for_a_sound_bank_to_load ~86% - loop branch emission. */
void wait_for_a_sound_bank_to_load(int bank) {
    int* async_state = &sbank_data[bank].async_state;
    int timeout = 0x708;

    while ((bank == -1
                ? 1
                : ((bank < 0 || bank >= 0x88) ? 0 : (*async_state == 1 ? 0 : 1))) == 0 &&
           --timeout != 0) {
        SOUND_BANK_WAIT_SLEEP();
    }
}

static inline int are_sound_banks_loaded(void) {
    int bank;

    for (bank = 0; bank < 0x88; bank++) {
        if (sbank_data[bank].async_state == 1) {
            return 0;
        }
    }
    return 1;
}

void wait_for_sound_banks_to_load(void) {
    int timeout = 0x708;

    while (!are_sound_banks_loaded() && --timeout != 0) {
        SOUND_BANK_WAIT_SLEEP();
    }
}

/* Soft ceiling: unload_pz_fighter_fatality_banks ~73% - unload GPR allocation. */
void unload_pz_fighter_fatality_banks(void) {
    LoadedSoundBank* loaded = &loaded_sbank_data[0x15];
    int bank = loaded->bank_index;
    int use_count = 0;
    char filename[0x9C];
    int i;

    if (bank != -1) {
        for (i = 0; i < 0x1D; i++) {
            if (loaded_sbank_data[i].bank_index == bank) {
                use_count++;
            }
        }
    }
    if (use_count == 1 && bank != -1 && bank >= 0 && bank < 0x88) {
        SoundBankData* bank_data = &sbank_data[bank];

        filename[0] = '\0';
        strcat(filename, bank_data->name);
        strcat(filename, ".msb");
        CANCEL_OR_UNLOAD_BANK(bank_data, filename);
    }
    loaded->bank_index = -1;
    loaded->active = 0;
}

#undef SOUND_BANK_WAIT_SLEEP

/* Soft ceiling: load_pz_fighter_fatality_bank ~86% - inlined wait branches. */
void load_pz_fighter_fatality_bank(int bank) {
    check_and_load_sound_bank_async(bank, 0x15);
    wait_for_a_sound_bank_to_load(bank);
}

typedef struct SoundFighterBanks {
    char pad00[0x60];
    int standard_0;
    int standard_1;
    char pad68[0x10];
    int alternate_0;
    int alternate_1;
} SoundFighterBanks;

typedef struct SoundFighter {
    char pad00[0x6F8];
    SoundFighterBanks* sound_banks;
} SoundFighter;

typedef struct KonquestSoundBanks {
    char pad00[0x58];
    int bank_0;
    int bank_1;
} KonquestSoundBanks;

/* Soft ceiling: get_indirect_bank ~98% - jump-table relocation labels. */
int get_indirect_bank(unsigned int indirect_bank) {
    SoundFighter* fighter;
    KonquestSoundBanks* konquest;
    int bank = -1;

    switch (indirect_bank) {
    case 0x01000000:
        fighter = (SoundFighter*)g_game_info.plyr0.slot.fighter;
        if (fighter != 0 && fighter->sound_banks != 0) bank = fighter->sound_banks->standard_0;
        break;
    case 0x01000001:
        fighter = (SoundFighter*)g_game_info.plyr0.slot.fighter;
        if (fighter != 0 && fighter->sound_banks != 0) bank = fighter->sound_banks->standard_1;
        break;
    case 0x01000002:
        fighter = (SoundFighter*)g_game_info.plyr1.slot.fighter;
        if (fighter != 0 && fighter->sound_banks != 0) bank = fighter->sound_banks->standard_0;
        break;
    case 0x01000003:
        fighter = (SoundFighter*)g_game_info.plyr1.slot.fighter;
        if (fighter != 0 && fighter->sound_banks != 0) bank = fighter->sound_banks->standard_1;
        break;
    case 0x01000004:
        if (g_game_info.section != 0) bank = g_game_info.section->sound_bank_0;
        break;
    case 0x01000005:
        if (g_game_info.section != 0) bank = g_game_info.section->sound_bank_1;
        break;
    case 0x01000006:
        if (g_game_info.section != 0) bank = g_game_info.section->sound_bank_2;
        break;
    case 0x01000007:
        if (g_game_info.section != 0) bank = g_game_info.section->sound_bank_3;
        break;
    case 0x01000008:
        fighter = (SoundFighter*)g_game_info.plyr0.slot.fighter;
        if (fighter != 0 && fighter->sound_banks != 0) bank = fighter->sound_banks->alternate_0;
        break;
    case 0x01000009:
        fighter = (SoundFighter*)g_game_info.plyr0.slot.fighter;
        if (fighter != 0 && fighter->sound_banks != 0) bank = fighter->sound_banks->alternate_1;
        break;
    case 0x0100000A:
        fighter = (SoundFighter*)g_game_info.plyr1.slot.fighter;
        if (fighter != 0 && fighter->sound_banks != 0) bank = fighter->sound_banks->alternate_0;
        break;
    case 0x0100000B:
        fighter = (SoundFighter*)g_game_info.plyr1.slot.fighter;
        if (fighter != 0 && fighter->sound_banks != 0) bank = fighter->sound_banks->alternate_1;
        break;
    case 0x0100000C:
        konquest = (KonquestSoundBanks*)get_konquest_region_table();
        bank = konquest->bank_0;
        break;
    case 0x0100000D:
        konquest = (KonquestSoundBanks*)get_konquest_region_table();
        bank = konquest->bank_1;
        break;
    }

    if ((bank < 0 || bank >= 0x88) && bank != -1) {
        bank = -1;
    }
    return bank;
}

/* Soft ceiling: lsba_callbank ~74% - repeated switch-case GPR allocation. */
void lsba_callbank(SoundBankCallback* callback) {
    int bank;

    if (callback == 0 || callback->bank_index == 0) {
        return;
    }

    bank = *callback->bank_index;
    switch (callback->status) {
    case 0:
        sbank_data[bank].handle = callback->handle;
        sbank_data[bank].async_state = 0;
        break;
    case 1:
        sbank_data[bank].handle = 0;
        sbank_data[bank].async_state = 0;
        sbank_data[bank].callback_state = 0;
        break;
    case 2:
        sbank_data[bank].handle = 0;
        sbank_data[bank].async_state = 0;
        sbank_data[bank].callback_state = 0;
        break;
    case 3:
        sbank_data[bank].handle = 0;
        sbank_data[bank].async_state = 0;
        sbank_data[bank].callback_state = 0;
        break;
    case 4:
        sbank_data[bank].handle = 0;
        sbank_data[bank].async_state = 0;
        sbank_data[bank].callback_state = 0;
        break;
    }
}

/* Soft ceiling: check_and_load_sound_bank_async ~65% - dual-buffer GPR allocation. */
void check_and_load_sound_bank_async(int bank, int slot) {
    char load_filename[0x98];
    char unload_filename[0x9C];

    if (bank != -1 && bank >= 0 && bank < 0x88 && slot >= 0 && slot < 0x1D) {
        LoadedSoundBank* loaded = &loaded_sbank_data[slot];

        if (loaded->bank_index != -1 && slot >= 0 && slot < 0x1D) {
            int old_bank = loaded->bank_index;
            int use_count = 0;
            int i;

            if (old_bank != -1) {
                for (i = 0; i < 0x1D; i++) {
                    if (loaded_sbank_data[i].bank_index == old_bank) {
                        use_count++;
                    }
                }
            }
            if (use_count == 1 && old_bank != -1 && old_bank >= 0 && old_bank < 0x88) {
                SoundBankData* old_data = &sbank_data[old_bank];

                unload_filename[0] = '\0';
                strcat(unload_filename, old_data->name);
                strcat(unload_filename, ".msb");
                CANCEL_OR_UNLOAD_BANK(old_data, unload_filename);
            }
            loaded->bank_index = -1;
            loaded->active = 0;
        }

        if (bank != -1) {
            SoundBankData* bank_data = &sbank_data[bank];

            if (bank_data->handle == 0 && bank != -1) {
                load_filename[0] = '\0';
                strcat(load_filename, bank_data->name);
                strcat(load_filename, ".msb");
                if (bank_data->handle == 0 && bank_data->async_state == 0) {
                    bank_data->handle = 0;
                    bank_data->async_state = 1;
                    bank_data->callback = (void (*)(void*))lsba_callbank;
                    bank_data->callback_field_18 = 0;
                    bank_data->callback_bank = &bank_data->bank_index;
                    bank_data->callback_state = 0;
                    mslBankLoadAsync(msi, 0, load_filename, &bank_data->callback);
                }
            }
            loaded->bank_index = bank;
            loaded->active = bank_data->active;
        }
    }
}

#undef CANCEL_OR_UNLOAD_BANK

typedef struct DelayedSoundPdata {
    MkHdr hdr;
    int sound_id;
} DelayedSoundPdata;

float p_snd_req_delay(void);

/* Soft ceiling: snd_req_delay ~97% - final range-check branch emission. */
void snd_req_delay(int sound_id, int delay) {
    typedef MkProc* (*CreateDelayedSoundProcFn)(
        int, int, MkProcEntryFn, int, MkHdr**);
    DelayedSoundPdata* pdata;
    MkProc* proc;

    if (sound_id != -1) {
        if (sound_id >= 0) {
            if (sound_id < 0x1C0C) {
                proc = ((CreateDelayedSoundProcFn)_create_mkproc_generic_tinystack)(
                    0x3006, 0x27, p_snd_req_delay, sizeof(DelayedSoundPdata),
                    (MkHdr**)&pdata);
                if (proc != 0) {
                    proc->sleep_ticks = (float)delay;
                    pdata->sound_id = sound_id;
                }
            }
        }
    }
}

/* Soft ceiling: p_snd_req_delay ~99.8% - zero-float pool label only. */
float p_snd_req_delay(void) {
    SoundRequest request;
    int sound_id;

    if (aproc->pid != 0x3006) {
        return 0.0f;
    }
    if (apdata != 0) {
        sound_id = ((DelayedSoundPdata*)apdata)->sound_id;
        if (sound_id != -1 && sound_id >= 0 && sound_id < 0x1C0C) {
            if (sound_id != -1 && sound_id >= 0 && sound_id < 0x1C0C) {
                SoundEntry* entry;

                request.sound_id = sound_id;
                request.volume = 1.0f;
                request.apply_group_volume = 1;
                if (snd_calculate_volume(&request)) {
                    entry = &mk_sound_table[sound_id];
                    mslBankPlayVol(
                        request.bank, entry->bank, entry->sound, entry->field_0c,
                        request.volume, entry->field_18);
                }
            }
        }
    }
    return 0.0f;
}

void snd_stop_all(void) {
    mslStopAll(msi);
}

void snd_stop(MslSoundHandle handle) {
    if (handle != 0 && mslSoundIsValid(handle)) {
        mslSoundStop(handle);
    }
}

MslSoundHandle pan_vol_pitch_snd_req(
    int sound_id, float pan, float volume, float pitch) {
    SoundRequest request;
    MslSoundHandle handle = 0;

    if (sound_id == -1) {
        return 0;
    }
    if (sound_id < 0 || sound_id >= 0x1C0C) {
        return 0;
    }

    request.sound_id = sound_id;
    request.volume = volume;
    request.apply_group_volume = 1;
    if (snd_calculate_volume(&request)) {
        SoundEntry* entry;

        if (pan < -2.0f) {
            pan = -2.0f;
        }
        if (pan > 2.0f) {
            pan = 2.0f;
        }
        if (pitch < 0.25f) {
            pitch = 0.25f;
        }
        if (pitch > 4.0f) {
            pitch = 4.0f;
        }
        entry = &mk_sound_table[sound_id];
        handle = mslBankPlayVolPanPitch(
            request.bank, entry->bank, entry->sound, entry->field_0c, request.volume,
            pan, pitch, entry->field_18);
    }
    return handle;
}

/* Soft ceiling: pan_vol_snd_req ~98% - duplicated validation branch emission. */
MslSoundHandle pan_vol_snd_req(int sound_id, float pan, float volume) {
    SoundRequest request;
    MslSoundHandle handle;

    if (sound_id == -1) {
        return 0;
    }
    if (sound_id < 0 || sound_id >= 0x1C0C) {
        return 0;
    }

    handle = 0;
    if (sound_id != -1) {
        if (sound_id < 0 || sound_id >= 0x1C0C) {
            handle = 0;
        } else {
            SoundEntry* entry;

            request.sound_id = sound_id;
            request.volume = volume;
            request.apply_group_volume = 1;
            if (snd_calculate_volume(&request)) {
                if (pan < -2.0f) {
                    pan = -2.0f;
                }
                if (pan > 2.0f) {
                    pan = 2.0f;
                }
                entry = &mk_sound_table[sound_id];
                handle = mslBankPlayVolPanPitch(
                    request.bank, entry->bank, entry->sound, entry->field_0c,
                    request.volume, pan, 1.0f, entry->field_18);
            }
        }
    }
    return handle;
}

/* Soft ceiling: pan_snd_req ~98% - duplicated validation branch emission. */
MslSoundHandle pan_snd_req(int sound_id, float pan) {
    SoundRequest request;
    MslSoundHandle handle;

    if (sound_id == -1) {
        return 0;
    }
    if (sound_id < 0 || sound_id >= 0x1C0C) {
        return 0;
    }

    handle = 0;
    if (sound_id != -1) {
        if (sound_id < 0 || sound_id >= 0x1C0C) {
            handle = 0;
        } else {
            SoundEntry* entry;

            request.sound_id = sound_id;
            request.volume = 1.0f;
            request.apply_group_volume = 1;
            if (snd_calculate_volume(&request)) {
                if (pan < -2.0f) {
                    pan = -2.0f;
                }
                if (pan > 2.0f) {
                    pan = 2.0f;
                }
                entry = &mk_sound_table[sound_id];
                handle = mslBankPlayVolPanPitch(
                    request.bank, entry->bank, entry->sound, entry->field_0c,
                    request.volume, pan, 1.0f, entry->field_18);
            }
        }
    }
    return handle;
}

void set_snd_vol(MslSoundHandle handle, int sound_id, float volume) {
    SoundRequest request;

    if (handle != 0) {
        request.sound_id = sound_id;
        request.volume = volume;
        request.apply_group_volume = 0;
        if (snd_calculate_volume(&request) && mslSoundIsValid(handle)) {
            mslSoundSetVol(handle, request.volume);
        }
    }
}

MslSoundHandle snd_req_vol(int sound_id, float volume) {
    SoundRequest request;
    MslSoundHandle handle = 0;

    if (sound_id == -1) {
        return 0;
    }
    if (sound_id < 0 || sound_id >= 0x1C0C) {
        return 0;
    }

    request.sound_id = sound_id;
    request.volume = volume;
    request.apply_group_volume = 1;
    if (snd_calculate_volume(&request)) {
        SoundEntry* entry = &mk_sound_table[sound_id];

        handle = mslBankPlayVol(
            request.bank, entry->bank, entry->sound, entry->field_0c, request.volume,
            entry->field_18);
    }
    return handle;
}

/* Soft ceiling: snd_req ~98% - duplicated validation branch emission. */
MslSoundHandle snd_req(int sound_id) {
    SoundRequest request;
    MslSoundHandle handle;

    if (sound_id == -1) {
        return 0;
    }
    if (sound_id < 0 || sound_id >= 0x1C0C) {
        return 0;
    }

    handle = 0;
    if (sound_id != -1) {
        if (sound_id < 0 || sound_id >= 0x1C0C) {
            handle = 0;
        } else {
            SoundEntry* entry;

            request.sound_id = sound_id;
            request.volume = 1.0f;
            request.apply_group_volume = 1;
            if (snd_calculate_volume(&request)) {
                entry = &mk_sound_table[sound_id];
                handle = mslBankPlayVol(
                    request.bank, entry->bank, entry->sound, entry->field_0c,
                    request.volume, entry->field_18);
            }
        }
    }
    return handle;
}

/* Soft ceiling: snd_calculate_volume ~74% - global-load and arithmetic scheduling. */
int snd_calculate_volume(SoundRequest* request) {
    SoundEntry* entry;
    SoundSubgroupVolume* subgroup;
    int bank_index;
    int setting_index;
    int result = 0;

    if (request->volume < 0.0f) {
        request->volume = 0.0f;
    }
    if (request->volume > 1.0f) {
        request->volume = 1.0f;
    }
    if (request->sound_id >= 0 && (unsigned int)request->sound_id < 0x1C0C) {
        entry = &mk_sound_table[request->sound_id];
        bank_index = entry->bank_index;
        if (mode_of_play == 6 && bank_index == 7) {
            request->bank = sbank_data[0x72].handle;
        } else {
            request->bank = sbank_data[bank_index].handle;
        }
        if (request->bank != 0 && entry->subgroup < 0x0C) {
            subgroup = &subgroup_volume[entry->subgroup];
            setting_index = subgroup->setting_index;
            if ((unsigned int)setting_index < 6) {
                request->volume =
                    game_settings[setting_index] * (request->volume * entry->base_volume) *
                    subgroup->volume;
                if (request->apply_group_volume != 0 && mode_of_play == 6 &&
                    entry->bank_index != 0x71 && entry->bank_index < 0x5F) {
                    request->volume *= 0.65f;
                }
                if (request->volume < 0.0f) {
                    request->volume = 0.0f;
                }
                if (request->volume > 1.0f) {
                    request->volume = 1.0f;
                }
                result = 1;
            }
        }
    }
    return result;
}

/* Soft ceiling: plyr_snd_req_no_plyr_proc ~88% - player-bank GPR allocation. */
MslSoundHandle plyr_snd_req_no_plyr_proc(
    PlyrPdata* fighter, int sound_offset) {
    SoundRequest request;
    int player_bank = -1;
    int voice_offset = 0;
    int sound_id;
    MslSoundHandle handle = 0;

    if (sound_offset < 0 || sound_offset >= 0x5C) {
        return 0;
    }

    if ((void*)fighter == (void*)g_game_info.plyr0.slot.fighter) {
        player_bank = loaded_sbank_data[9].active;
        if (player_bank == 0x13) {
            voice_offset = p1_char_fvoice != 0 ? 0x5C : 0;
        }
    }
    if ((void*)fighter == (void*)g_game_info.plyr1.slot.fighter) {
        player_bank = loaded_sbank_data[10].active;
        if (player_bank == 0x13) {
            voice_offset = p2_char_fvoice != 0 ? 0x5C : 0;
        }
    }
    if (player_bank < 0 || player_bank >= 0x19) {
        return 0;
    }

    sound_id = mk_plyr_sound_table[player_bank] + voice_offset + sound_offset;
    if (sound_id == -1) {
        return 0;
    }
    if (sound_id < 0 || sound_id >= 0x1C0C) {
        return 0;
    }

    if (sound_id != -1) {
        if (sound_id < 0 || sound_id >= 0x1C0C) {
            handle = 0;
        } else {
            SoundEntry* entry;

            request.sound_id = sound_id;
            request.volume = 1.0f;
            request.apply_group_volume = 1;
            if (snd_calculate_volume(&request)) {
                entry = &mk_sound_table[sound_id];
                handle = mslBankPlayVol(
                    request.bank, entry->bank, entry->sound, entry->field_0c,
                    request.volume, entry->field_18);
            }
        }
    }
    return handle;
}

MslSoundHandle plyr_snd_req(int sound_offset) {
    if (plyr_pdata != 0) {
        return plyr_snd_req_no_plyr_proc(plyr_pdata, sound_offset);
    }
    return 0;
}

void select_fighter_voice_in_bank(int player, int alternate_voice) {
    if (player == 0) {
        p1_char_fvoice = alternate_voice;
    }
    if (player != 0) {
        p2_char_fvoice = alternate_voice;
    }
}

/* Soft ceiling: foot_snd_req ~88% - table-index and validation branch emission. */
MslSoundHandle foot_snd_req(int foot_type) {
    SoundRequest request;
    MslSoundHandle handle = 0;
    int sound_id;
    int foot_bank;

    if (mode_of_play == 6) {
        return 0;
    }
    if (foot_type < 0 || foot_type >= 7) {
        return 0;
    }

    foot_bank = loaded_sbank_data[8].active;
    if (foot_bank >= 4) {
        return 0;
    }
    sound_id = mk_foot_sound_table[foot_type][foot_bank];
    if (sound_id == -1) {
        return 0;
    }
    if (sound_id < 0 || sound_id >= 0x1C0C) {
        return 0;
    }

    if (sound_id != -1) {
        if (sound_id < 0 || sound_id >= 0x1C0C) {
            handle = 0;
        } else {
            SoundEntry* entry;

            request.sound_id = sound_id;
            request.volume = 1.0f;
            request.apply_group_volume = 1;
            if (snd_calculate_volume(&request)) {
                entry = &mk_sound_table[sound_id];
                handle = mslBankPlayVol(
                    request.bank, entry->bank, entry->sound, entry->field_0c,
                    request.volume, entry->field_18);
            }
        }
    }
    return handle;
}

#pragma dont_inline on

MslSoundHandle random_foot(int group) {
    MslSoundHandle handle = 0;

    if (group >= 0 && group < 3) {
        int* sounds = foot_call_table[group].sounds;
        int count = foot_call_table[group].count;

        if (sounds != 0 && count > 0) {
            unsigned int choice = randu0((unsigned short)count) & 0xFFFF;

            handle = foot_snd_req(sounds[choice]);
        }
    }
    return handle;
}

MslSoundHandle random_voice(int group) {
    MslSoundHandle handle = 0;

    if (group >= 0 && group < 0x17) {
        int* sounds = voice_call_table[group].sounds;
        int count = voice_call_table[group].count;

        if (sounds != 0 && count > 0) {
            unsigned int choice = randu0((unsigned short)count) & 0xFFFF;

            handle = plyr_snd_req(sounds[choice]);
        }
    }
    return handle;
}

void snd_major_hit_voice(void) {
    random_voice(3);
}

void snd_death_voice(void) {
    random_voice(0xE);
}

/*
 * Soft ceiling: pan_vol_pitch_random_snd_req ~93.01% -- the retail
 * count switch, branch-local result lifetime, and delayed pan/pitch
 * initialization are recovered. Remaining differences are table-base and
 * branch-result register coloring.
 */
MslSoundHandle pan_vol_pitch_random_snd_req(int group, float pan, float volume, float pitch) {
    SoundRequest single_request;
    SoundRequest alternate_request;
    SoundRequest random_request;
    RandomSoundRequest *table;
    unsigned int previous;
    int *sounds;
    unsigned char *previous_ptr;
    int count;
    MslSoundHandle handle = 0;
    unsigned int choice;
    int sound_id;

    if (group >= 0 && group < 0xB5) {
        table = &random_sound_request[group];
        sounds = table->sounds;
        if (sounds != 0) {
            count = table->count;
            previous_ptr = &table->previous;
            previous = *previous_ptr;

            switch (count) {
            case 1: {
                float play_pan;
                float play_pitch;
                MslSoundHandle branch_handle;

                sound_id = sounds[0];
                play_pitch = pitch;
                play_pan = pan;
                branch_handle = 0;
                if (sound_id != -1) {
                    if (sound_id < 0 || sound_id >= 0x1C0C) {
                        branch_handle = 0;
                    } else {
                        SoundEntry *entry;

                        single_request.sound_id = sound_id;
                        single_request.volume = volume;
                        single_request.apply_group_volume = 1;
                        if (snd_calculate_volume(&single_request)) {
                            if (play_pan < -2.0f) {
                                play_pan = -2.0f;
                            }
                            if (play_pan > 2.0f) {
                                play_pan = 2.0f;
                            }
                            if (play_pitch < 0.25f) {
                                play_pitch = 0.25f;
                            }
                            if (play_pitch > 4.0f) {
                                play_pitch = 4.0f;
                            }
                            entry = &mk_sound_table[sound_id];
                            branch_handle = mslBankPlayVolPanPitch(
                                single_request.bank, entry->bank, entry->sound, entry->field_0c,
                                single_request.volume, play_pan, play_pitch, entry->field_18);
                        }
                    }
                }
                handle = branch_handle;
                return handle;
            }
            case 0:
                return 0;
            case 2: {
                float play_pan;
                float play_pitch;
                MslSoundHandle branch_handle;

                if (previous >= (unsigned int)count) {
                    previous = 0;
                    *previous_ptr = 0;
                }
                play_pitch = pitch;
                choice = 1 - previous;
                play_pan = pan;
                sound_id = sounds[choice];
                branch_handle = 0;
                if (sound_id != -1) {
                    if (sound_id < 0 || sound_id >= 0x1C0C) {
                        branch_handle = 0;
                    } else {
                        SoundEntry *entry;

                        alternate_request.sound_id = sound_id;
                        alternate_request.volume = volume;
                        alternate_request.apply_group_volume = 1;
                        if (snd_calculate_volume(&alternate_request)) {
                            if (play_pan < -2.0f) {
                                play_pan = -2.0f;
                            }
                            if (play_pan > 2.0f) {
                                play_pan = 2.0f;
                            }
                            if (play_pitch < 0.25f) {
                                play_pitch = 0.25f;
                            }
                            if (play_pitch > 4.0f) {
                                play_pitch = 4.0f;
                            }
                            entry = &mk_sound_table[sound_id];
                            branch_handle = mslBankPlayVolPanPitch(
                                alternate_request.bank, entry->bank, entry->sound, entry->field_0c,
                                alternate_request.volume, play_pan, play_pitch, entry->field_18);
                        }
                    }
                }
                *previous_ptr = choice;
                handle = branch_handle;
                return handle;
            }
            default:
                break;
            }

            {
                float play_pan;
                float play_pitch;
                MslSoundHandle branch_handle;

                if (previous >= (unsigned int)count) {
                    previous = 0;
                    *previous_ptr = 0;
                }
                choice = randu0((unsigned short)(count - 1)) & 0xFFFF;
                if (choice >= previous) {
                    choice++;
                }
                play_pitch = pitch;
                sound_id = sounds[choice];
                play_pan = pan;
                branch_handle = 0;
                if (sound_id != -1) {
                    if (sound_id < 0 || sound_id >= 0x1C0C) {
                        branch_handle = 0;
                    } else {
                        SoundEntry *entry;

                        random_request.sound_id = sound_id;
                        random_request.volume = volume;
                        random_request.apply_group_volume = 1;
                        if (snd_calculate_volume(&random_request)) {
                            if (play_pan < -2.0f) {
                                play_pan = -2.0f;
                            }
                            if (play_pan > 2.0f) {
                                play_pan = 2.0f;
                            }
                            if (play_pitch < 0.25f) {
                                play_pitch = 0.25f;
                            }
                            if (play_pitch > 4.0f) {
                                play_pitch = 4.0f;
                            }
                            entry = &mk_sound_table[sound_id];
                            branch_handle = mslBankPlayVolPanPitch(
                                random_request.bank, entry->bank, entry->sound, entry->field_0c,
                                random_request.volume, play_pan, play_pitch, entry->field_18);
                        }
                    }
                }
                *previous_ptr = choice;
                handle = branch_handle;
            }
        }
    }
    return handle;
}

MslSoundHandle random_snd_req(int group) {
    return pan_vol_pitch_random_snd_req(group, 0.0f, 1.0f, 1.0f);
}

typedef struct DelayedRandomSoundPdata {
    MkHdr hdr;
    int group;
} DelayedRandomSoundPdata;

float p_random_snd_req_delay(void);

void random_snd_req_delay(int group, int delay) {
    typedef MkProc* (*CreateDelayedRandomSoundProcFn)(
        int, int, MkProcEntryFn, int, MkHdr**);
    DelayedRandomSoundPdata* pdata;
    MkProc* proc;

    proc = ((CreateDelayedRandomSoundProcFn)_create_mkproc_generic_tinystack)(
        0x3010, 0x27, p_random_snd_req_delay,
        sizeof(DelayedRandomSoundPdata), (MkHdr**)&pdata);
    if (proc != 0) {
        proc->sleep_ticks = (float)delay;
        pdata->group = group;
    }
}

/* Soft ceiling: p_random_snd_req_delay ~99.55% - zero-float pool label only. */
float p_random_snd_req_delay(void) {
    if (aproc->pid != 0x3010) {
        return 0.0f;
    }
    if (apdata != 0) {
        pan_vol_pitch_random_snd_req(
            ((DelayedRandomSoundPdata*)apdata)->group, 0.0f, 1.0f, 1.0f);
    }
    return 0.0f;
}

/*
 * MWCC GC/2.7 exits internally on these recovered hit paths at optimization
 * levels 2-4. Level 1 preserves the retail algorithms without match-force C.
 */
#pragma optimization_level 1

/* Soft ceiling: pan_vol_pitch_random_hit ~74.50% - compiler-stable O1 body. */
MslSoundHandle pan_vol_pitch_random_hit(
    int group, float pan, float volume, float pitch) {
    SoundRequest request;
    MslSoundHandle handle = 0;
    SoundEntry* entry;
    int* sounds;
    int count;
    unsigned int choice;
    int sound_id;

    if (group < 0 || group >= 0x13) {
        return 0;
    }

    if (mode_of_play == 6) {
        sounds = pf_hit_call_table[group].sounds;
        count = pf_hit_call_table[group].count;
    } else {
        sounds = hit_call_table[group].sounds;
        count = hit_call_table[group].count;
    }

    if (sounds != 0 && count > 0) {
        choice = randu0((unsigned short)count) & 0xFFFF;
        sound_id = sounds[choice];
        if (sound_id != -1 && sound_id >= 0 && sound_id < 0x1C0C) {
            request.sound_id = sound_id;
            request.volume = volume;
            request.apply_group_volume = 1;
            if (snd_calculate_volume(&request)) {
                if (pan < -2.0f) {
                    pan = -2.0f;
                }
                if (pan > 2.0f) {
                    pan = 2.0f;
                }
                if (pitch < 0.25f) {
                    pitch = 0.25f;
                }
                if (pitch > 4.0f) {
                    pitch = 4.0f;
                }
                entry = &mk_sound_table[sound_id];
                handle = mslBankPlayVolPanPitch(
                    request.bank, entry->bank, entry->sound, entry->field_0c,
                    request.volume, pan, pitch, entry->field_18);
            }
        }
    }
    return handle;
}

/*
 * Soft ceiling: random_hit ~92.50% - duplicate table-index calculation,
 * sentinel-branch emission, and float-constant load scheduling remain.
 */
MslSoundHandle random_hit(int group) {
    SoundRequest request;
    int* sounds;
    MslSoundHandle handle = 0;
    SoundEntry* entry;
    SoundCallTable* table;
    SoundCallTable* call;
    int count;
    unsigned int choice;
    int sound_id;

    if (group >= 0 && group < 0x13) {
        if (mode_of_play == 6) {
            table = pf_hit_call_table;
            sounds = table[group].sounds;
            call = &table[group];
            count = call->count;
        } else {
            table = hit_call_table;
            sounds = table[group].sounds;
            call = &table[group];
            count = call->count;
        }

        if (sounds != 0 && count > 0) {
            choice = randu0((unsigned short)count) & 0xFFFF;
            handle = 0;
            sound_id = sounds[choice];
            if (sound_id != -1) {
                if (sound_id < 0 || sound_id >= 0x1C0C) {
                    handle = 0;
                } else {
                    request.sound_id = sound_id;
                    request.volume = 1.0f;
                    request.apply_group_volume = 1;
                    if (snd_calculate_volume(&request)) {
                        entry = &mk_sound_table[sound_id];
                        handle = mslBankPlayVolPanPitch(
                            request.bank, entry->bank, entry->sound,
                            entry->field_0c, request.volume, 0.0f, 1.0f,
                            entry->field_18);
                    }
                }
            }
        }
    }
    return handle;
}

#pragma optimization_level 4

/* Soft ceiling: init_sounds ~99.97% - pooled-string relocation label only. */
int init_sounds(void) {
    current_sound_shuffle_state = 0;
    current_konq_sound_shuffle_state = 0;
    g_sysinitDefault.aram_base = ARAM_MSL_GetBase();
    g_sysinitDefault.aram_size = ARAM_MSL_GetSize();
    msi = mslInit(&g_initDefault, &g_sysinitDefault);
    if (msi == 0) {
        return 0;
    }
    mslSetWavePath(msi, "/cdrom/sndsgc/");
    return 1;
}

#pragma dont_inline reset
