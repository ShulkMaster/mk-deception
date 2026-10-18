#include "runtime/sound_tracker.h"
#include "runtime/sound_data.h"
#include "runtime/sound.h"

#include "game/game_info.h"
#include "game/settings.h"
#include "math/mk_math.h"
#include "platform/gcARam.h"
#include "runtime/cam.h"
#include "runtime/mk_pdata.h"
#include "runtime/mk_proc.h"
#include "runtime/utils.h"
#include "msl/mslcore.h"
#include "msl/mslBankLoadAsyncQueue.h"
#include "runtime/cstring.h"
#include "platform/io.h"
#include "platform/main.h"
#include "rw/rwframe.h"

struct SoundRequest {
    int sound_id;
    int apply_group_volume;
    float volume;
    mslLoadedBank* bank;
};

typedef struct SoundCallTable {
    int* sounds;
    int count;
} SoundCallTable;

struct SoundBankCallback {
    _mslAsyncResponse response;
    int* bank_index;
};

typedef struct SoundBankData {
    mslLoadedBank* handle;
    int async_state;
    unsigned char active;
    char pad09[3];
    char* name;
    struct SoundBankCallback callback;
    int bank_index;
} SoundBankData;

typedef char SoundBankCallbackSizeCheck[sizeof(struct SoundBankCallback) == 0x10 ? 1 : -1];
typedef char SoundBankDataSizeCheck[sizeof(SoundBankData) == 0x24 ? 1 : -1];

typedef struct LoadedSoundBank {
    int bank_index;
    unsigned char active;
    char pad05[3];
} LoadedSoundBank;

typedef struct SoundBankLoadMode {
    unsigned int* banks;
    int count;
    int transition_mode;
    int slot_list;
    char* name;
} SoundBankLoadMode;

struct MslInitParam {
    int size;
    int flags;
    int track_count;
};

struct MslSystemInit {
    int size;
    int field_04;
    unsigned short first_voice;
    unsigned short voice_count;
    unsigned int aram_base;
    unsigned int aram_size;
};

static struct MslInitParam g_initDefault = {sizeof(struct MslInitParam), 1, 3};
static struct MslSystemInit g_sysinitDefault = {sizeof(struct MslSystemInit), 0, 0, 64, 0, 0};
extern SoundBankData sbank_data[];
extern LoadedSoundBank loaded_sbank_data[];
extern int mk_plyr_sound_table[];
extern int mk_foot_sound_table[7][4];

#include "src/game/sound_call_tables.inc"
#include "src/game/sound_bank_data.inc"

enum { SOUND_BANK_COUNT = sizeof(sbank_data) / sizeof(sbank_data[0]) };

int can_big_boss_make_special_vo_call(unsigned int cooldown_ticks);
static int snd_calculate_volume(struct SoundRequest* request);

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

int gap_08_80510D94_sbss;
static int last_sound_list_issued;
static int p2_char_fvoice;
static int p1_char_fvoice;
_mslSystem* msi;
static int current_konq_sound_shuffle_state;
static int current_sound_shuffle_state;
int sounds_muted;
MslSoundHandle bgnd_music_ptr2;
MslSoundHandle bgnd_music_ptr1;

void yinyang_finish_music(void);
void yinyang_stop_music(void);
void yinyang_start_music(void);

static int get_indirect_bank(unsigned int bank);
static void unload_banks_not_on_list(int mode);
static void load_banks_on_list_async(int mode);
static void unload_slots_not_on_list(int slot_list);
void mslBankUnLoad(mslLoadedBank* bank);
float mslGetVol(_mslSystem* system);
void mslSetVol(_mslSystem* system, float volume);
_mslSystem* mslInit(struct MslInitParam* init, struct MslSystemInit* system_init);
int mslSetWavePath(_mslSystem* system, const char* path);
static void check_and_load_sound_bank_async(int bank, int slot);
void* get_konquest_region_table(void);
static void lsba_callbank(_mslAsyncResponse* response);

void dk_voice_call(int voice_group, unsigned int cooldown_ticks) {
    int group0[3] = {0x1C4, 0x1C4, 0x1C5};
    int group1[3] = {0x1B4, 0x1B5, 0x1B4};
    int group2[3] = {0x1C0, 0x1BF, 0x1C0};
    struct SoundRequest request0;
    struct SoundRequest request1;
    struct SoundRequest request2;
    unsigned int choice;
    int sound_id;

    choice = randu0(3) & 0xFFFF;
    if (!can_big_boss_make_special_vo_call(cooldown_ticks)) {
        return;
    }

    switch (voice_group) {
    case 0:
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
        break;
    case 1:
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
        break;
    case 2:
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
        break;
    }
}

static inline MslSoundHandle play_tracked_sound(int sound_id, float volume,
                                                float pan)
{
    struct SoundRequest play_request;
    MslSoundHandle handle = 0;

    if (sound_id == -1) {
        return handle;
    }
    if (sound_id < 0 || sound_id >= 0x1C0C) {
        handle = 0;
    } else {
    SoundEntry* entry;

    play_request.sound_id = sound_id;
    play_request.volume = volume;
    play_request.apply_group_volume = 1;
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
    return handle;
}

static inline void update_tracked_sound(TrackedSound* sound,
                                        RwFrame* camera_frame)
{
    float distance;
    float pan;
    float volume;
    struct SoundRequest update_request;

    if (sound->tracking_enabled != 0) {
        distance = dist_v3_to_v3(&sound->pos, &camera_frame->modelling.pos_row.value);
        if (distance < sound->max_dist) {
            if (sound->out_of_range != 0) {
                sound->out_of_range = 0;
            }
            pan = sound->positional_pan != 0 ?
                get_pan_value(&sound->pos) : 0.0f;
            volume =
                1.0f - ((distance - sound->min_dist) / (sound->max_dist - sound->min_dist));

            if (mslSoundIsValid(sound->sound_handle)) {
                int sound_id;
                MslSoundHandle handle;

                handle = sound->sound_handle;
                sound_id = sound->sound_id;
                if (handle != 0 && sound_id != -1 &&
                    sound_id >= 0 && sound_id < 0x1C0C) {
                    update_request.sound_id = sound_id;
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
                MslSoundHandle handle;
                int sound_id;

                sound_id = sound->sound_id;
                if (sound_id == -1) {
                    handle = 0;
                } else if (sound_id < 0 || sound_id >= 0x1C0C) {
                    handle = 0;
                } else {
                    handle = play_tracked_sound(sound_id, volume, pan);
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
}

float p_track_sound(void) {
    SoundTrackerPdata* pdata;
    CameraObj* camera;
    RwFrame* camera_frame;
    TrackedSound* sound;
    MkPtr* node;

    pdata = (SoundTrackerPdata*)pdata_of_proc(aproc);
    if (pdata == 0) {
        return -1.0f;
    }

    camera = MK_HDR_LIVE(camera_item.node, camera_item.instance);
    if (camera == 0) {
        return -1.0f;
    }
    camera_frame = camera->frame;

    if (pdata->sound_list != 0) {
        node = *pdata->sound_list;
        while (node != 0) {
            MkPtr* next;

            sound = (TrackedSound*)node->hdr;
            if (node->instance != sound->hdr.instance) {
                next = node->next;
                discard_stale_mkptr(node);
                node = next;
                continue;
            }

            update_tracked_sound(sound, camera_frame);
            node = node->next;
        }
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
    SoundTrackerPdata* pdata;
    MkProc* proc;

    proc = _create_mkproc_generic_tinystack(
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

static inline MslSoundHandle play_sound_vol(int sound_id, float volume) {
    struct SoundRequest request;
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

static inline MslSoundHandle play_sound(int sound_id) {
    if (sound_id == -1) {
        return 0;
    }
    if (sound_id < 0 || sound_id >= 0x1C0C) {
        return 0;
    }
    return play_sound_vol(sound_id, 1.0f);
}

void finish_music(void) {
    MslSoundHandle handle;
    int sound_id;

    if (g_game_info.bgnd_id < 0 || g_game_info.bgnd_id >= 0x23) {
        return;
    }
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
        if (sound_id > -1) {
            bgnd_music_ptr1 = play_sound(sound_id);
        }
    }
}

void end_music(void) {
    MslSoundHandle handle;
    int sound_id;

    if (g_game_info.bgnd_id < 0 || g_game_info.bgnd_id >= 0x23) {
        return;
    }
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
            if (sound_id > -1) {
                bgnd_music_ptr1 = play_sound(sound_id);
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

void start_tunes(void) {
    MslSoundHandle handle;
    int sound_id;

    if (g_game_info.bgnd_id < 0 || g_game_info.bgnd_id >= 0x23) {
        return;
    }
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
            if (bgnd_music_ptr1 != 0) {
                handle = bgnd_music_ptr1;
                if (handle != 0 && mslSoundIsValid(handle)) {
                    mslSoundStop(handle);
                }
                bgnd_music_ptr1 = 0;
            }
            bgnd_music_ptr1 = play_sound(sound_id);
        }

        if (!mslSoundIsValid(bgnd_music_ptr2)) {
            sound_id = g_game_info.section->secondary_music_id;
            if (sound_id > -1) {
                bgnd_music_ptr2 = play_sound(sound_id);
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



static void debug_print_unload_general_bank(int bank) {
    debug_print_message("+++++++++++++++++++++++++++++++++++++++++\n");
    debug_print_message("++++ unload general: %i  +++++++++++++\n", bank);
    debug_print_message("+++++++++++++++++++++++++++++++++++++++++\n");
}

static inline int get_sound_bank_number(unsigned int bank) {
    switch (bank >> 24) {
    case 0xFF:
        return -1;
    case 0:
        return bank;
    case 1:
        return get_indirect_bank(bank);
    default:
        return -1;
    }
}

static inline int sound_bank_use_count(int bank)
{
    int count = 0;
    int i;

    if (bank == -1) {
        return count;
    }
    for (i = 0; i < 0x1D; i++) {
        if (loaded_sbank_data[i].bank_index == bank) {
            count++;
        }
    }
    return count;
}

static inline void unload_sound_bank(long bank) {
    char filename[0x98];

    if (bank != -1 && bank >= 0 && bank < SOUND_BANK_COUNT) {
        filename[0] = '\0';
        strcat(filename, sbank_data[bank].name);
        strcat(filename, ".msb");
        if (sbank_data[bank].handle != 0) {
            mslBankUnLoad(sbank_data[bank].handle);
            sbank_data[bank].handle = 0;
        } else if (sbank_data[bank].async_state == 1) {
            switch (mslBankLoadAsyncCancelNamed(filename)) {
            case 0:
                if (sbank_data[bank].handle != 0) {
                    mslBankUnLoad(sbank_data[bank].handle);
                }
                sbank_data[bank].handle = 0;
                break;
            case 1:
                sbank_data[bank].handle = 0;
                break;
            default:
                sbank_data[bank].handle = 0;
                break;
            }
        }
    }
}

static inline void unload_mode_bank_list(void)
{
    int list_index;
    unsigned int* banks = bank_load_table[0].banks;
    int bank_count = bank_load_table[0].count;

    for (list_index = 0; list_index < bank_count; list_index++) {
        int bank = get_sound_bank_number(banks[list_index * 2]);

        if (bank != -1) {
            int use_count = sound_bank_use_count(bank);

            if (use_count == 1 && sbank_data[bank].handle != 0) {
                unload_sound_bank(bank);
            }
            loaded_sbank_data[bank].bank_index = -1;
            loaded_sbank_data[bank].active = 0;
        }
    }
}

void setup_sound_bank_list_by_mode(int mode, int transition_mode) {
    if (mode < 0 || mode >= 0xF) {
        return;
    }
    if (transition_mode < 0 || transition_mode >= 3) {
        return;
    }

    last_sound_list_issued = mode;
    switch (transition_mode) {
    case 0:
        unload_banks_not_on_list(mode);
        load_banks_on_list_async(mode);
        break;
    case 1:
        unload_mode_bank_list();
        load_banks_on_list_async(mode);
        break;
    case 2:
        unload_mode_bank_list();
        if (bank_load_table[mode].slot_list != -1) {
            unload_slots_not_on_list(bank_load_table[mode].slot_list);
        }
        load_banks_on_list_async(mode);
        break;
    }
}



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

static void debug_print_load_konq_general_bank(int bank) {
    debug_print_message("++++ load konq general: %i +++++++++++++\n", bank);
    debug_print_message("++++++++++++++++++++++++++++++++++++++++++++\n");
}

static inline void load_sound_bank_and_slot_async(int bank, unsigned int slot) {
    char filename[0x98];
    if (bank != -1) {
        SoundBankData* bank_data = &sbank_data[bank];

        if (bank_data->handle == 0 && bank != -1) {
            filename[0] = '\0';
            strcat(filename, bank_data->name);
            strcat(filename, ".msb");
            if (bank_data->handle == 0 && sbank_data[bank].async_state == 0) {
                bank_data->handle = 0;
                sbank_data[bank].async_state = 1;
                bank_data->callback.response.callback = lsba_callbank;
                bank_data->callback.response.result = 0;
                bank_data->callback.bank_index = &bank_data->bank_index;
                bank_data->callback.response.status = 0;
                mslBankLoadAsync(msi, 0, filename, &bank_data->callback.response);
            }
        }
        loaded_sbank_data[slot].bank_index = bank;
        loaded_sbank_data[slot].active = bank_data->active;
    }
}

static void load_banks_on_list_async(int mode) {
    unsigned int* banks = bank_load_table[mode].banks;
    int count = bank_load_table[mode].count;
    int i;

    for (i = 0; i < count; i++) {
        unsigned int raw_bank = banks[i * 2];
        unsigned int slot;
        int bank;

        bank = get_sound_bank_number(raw_bank);

        slot = banks[i * 2 + 1];
        load_sound_bank_and_slot_async(bank, slot);
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

static inline int is_bank_on_list(int bank, int mode) {
    unsigned int* banks = bank_load_table[mode].banks;
    int count = bank_load_table[mode].count;
    int i;

    for (i = 0; i < count; i++) {
        int listed_bank = get_sound_bank_number(banks[i * 2]);

        if (listed_bank == bank) {
            return 1;
        }
    }
    return 0;
}

static void unload_banks_not_on_list(int mode) {
    long bank;

    for (bank = 0; bank < SOUND_BANK_COUNT; bank++) {
        SoundBankData* bank_data = &sbank_data[bank];

        if (bank_data->handle != 0 || bank_data->async_state == 1) {
            int present = is_bank_on_list(bank, mode);

            if (!present) {
                unload_sound_bank(bank);
            }
        }
    }
}

static inline int is_slot_on_list(int slot, int mode) {
    int count = bank_load_table[mode].count;
    unsigned int* banks = bank_load_table[mode].banks;
    int i;

    for (i = 0; i < count; i++) {
        int listed_slot = banks[i * 2 + 1];

        if (listed_slot == slot) {
            return 1;
        }
    }
    return 0;
}

static inline void unload_sound_bank_slot(int slot) {
    if (slot >= 0 && slot < 0x1D) {
        LoadedSoundBank* loaded = &loaded_sbank_data[slot];
        int bank = loaded->bank_index;
        int use_count = sound_bank_use_count(bank);

        if (use_count == 1) {
            unload_sound_bank(bank);
        }
        loaded->bank_index = -1;
        loaded_sbank_data[slot].active = 0;
    }
}

static void unload_slots_not_on_list(int mode) {
    int slot;

    for (slot = 0; slot < 0x1D; slot++) {
        if (loaded_sbank_data[slot].bank_index != -1) {
            int present = is_slot_on_list(slot, mode);

            if (!present) {
                unload_sound_bank_slot(slot);
            }
        }
    }
}

#define SOUND_BANK_WAIT_SLEEP()                  \
    do {                                         \
        MkVtableMkproc* vtable;                  \
                                                 \
        _mkproc_sleep_ticks = 1.0f;              \
        vtable = aproc->vtbl;                    \
        vtable->sleep();                         \
    } while (0)

static inline int is_sound_bank_ready(int bank) {
    if (bank == -1) {
        return 1;
    }
    if (bank < 0 || bank >= SOUND_BANK_COUNT) {
        return 0;
    }
    if (sbank_data[bank].async_state == 1) {
        return 0;
    }
    return 1;
}

void wait_for_a_sound_bank_to_load(int bank) {
    int timeout = 0x708;

    while (is_sound_bank_ready(bank) == 0 &&
           --timeout != 0) {
        SOUND_BANK_WAIT_SLEEP();
    }
}

static inline int are_sound_banks_loaded(void) {
    int bank;

    for (bank = 0; bank < SOUND_BANK_COUNT; bank++) {
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

void unload_pz_fighter_fatality_banks(void) {
    unload_sound_bank_slot(0x15);
}

#undef SOUND_BANK_WAIT_SLEEP

void load_pz_fighter_fatality_bank(int bank) {
    check_and_load_sound_bank_async(bank, 0x15);
    wait_for_a_sound_bank_to_load(bank);
}

struct KonquestSoundBanks {
    char pad00[0x58];
    int bank_0;
    int bank_1;
};

static int get_indirect_bank(unsigned int indirect_bank) {
    PlyrPdata* fighter;
    struct KonquestSoundBanks* konquest;
    int bank = -1;

    switch (indirect_bank) {
    case 0x01000000:
        fighter = g_game_info.plyr0.slot.pdata;
        if (fighter != 0 && fighter->runtime_data != 0) bank = fighter->runtime_data->fields_60[0];
        break;
    case 0x01000001:
        fighter = g_game_info.plyr0.slot.pdata;
        if (fighter != 0 && fighter->runtime_data != 0) bank = fighter->runtime_data->fields_60[1];
        break;
    case 0x01000002:
        fighter = g_game_info.plyr1.slot.pdata;
        if (fighter != 0 && fighter->runtime_data != 0) bank = fighter->runtime_data->fields_60[0];
        break;
    case 0x01000003:
        fighter = g_game_info.plyr1.slot.pdata;
        if (fighter != 0 && fighter->runtime_data != 0) bank = fighter->runtime_data->fields_60[1];
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
        fighter = g_game_info.plyr0.slot.pdata;
        if (fighter != 0 && fighter->runtime_data != 0) bank = fighter->runtime_data->fields_70[2];
        break;
    case 0x01000009:
        fighter = g_game_info.plyr0.slot.pdata;
        if (fighter != 0 && fighter->runtime_data != 0) bank = fighter->runtime_data->fields_70[3];
        break;
    case 0x0100000A:
        fighter = g_game_info.plyr1.slot.pdata;
        if (fighter != 0 && fighter->runtime_data != 0) bank = fighter->runtime_data->fields_70[2];
        break;
    case 0x0100000B:
        fighter = g_game_info.plyr1.slot.pdata;
        if (fighter != 0 && fighter->runtime_data != 0) bank = fighter->runtime_data->fields_70[3];
        break;
    case 0x0100000C:
        konquest = get_konquest_region_table();
        bank = konquest->bank_0;
        break;
    case 0x0100000D:
        konquest = get_konquest_region_table();
        bank = konquest->bank_1;
        break;
    default:
        bank = -1;
        break;
    }

    if ((bank < 0 || bank >= SOUND_BANK_COUNT) && bank != -1) {
        bank = -1;
    }
    return bank;
}

static inline void sound_publish_bank_response(int bank, _mslAsyncResponse* response) {
    mslLoadedBank* handle;

    handle = response->result;
    switch (response->status) {
    case 0:
        sbank_data[bank].handle = handle;
        sbank_data[bank].async_state = 0;
        break;
    case 1:
        sbank_data[bank].handle = 0;
        sbank_data[bank].async_state = 0;
        sbank_data[bank].callback.response.status = 0;
        break;
    case 2:
        sbank_data[bank].handle = 0;
        sbank_data[bank].async_state = 0;
        sbank_data[bank].callback.response.status = 0;
        break;
    case 3:
        sbank_data[bank].handle = 0;
        sbank_data[bank].async_state = 0;
        sbank_data[bank].callback.response.status = 0;
        break;
    case 4:
        sbank_data[bank].handle = 0;
        sbank_data[bank].async_state = 0;
        sbank_data[bank].callback.response.status = 0;
        break;
    }
}

static void lsba_callbank(_mslAsyncResponse* response) {
    struct SoundBankCallback* callback = (struct SoundBankCallback*)response;
    int bank;

    if (callback == 0) {
        return;
    }
    if (callback->bank_index == 0) {
        return;
    }

    bank = *callback->bank_index;
    sound_publish_bank_response(bank, response);
}

static void check_and_load_sound_bank_async(int bank, int slot) {
    char unload_filename[0x98];
    char load_filename[0x98];

    if (bank == -1) {
        return;
    }
    if (bank < 0 || bank >= SOUND_BANK_COUNT) {
        return;
    }
    if (slot < 0 || slot >= 0x1D) {
        return;
    }

    if (loaded_sbank_data[slot].bank_index != -1 && slot >= 0 && slot < 0x1D) {
        int old_bank = loaded_sbank_data[slot].bank_index;
        int use_count = sound_bank_use_count(old_bank);
        if (use_count == 1 && old_bank != -1 && old_bank >= 0 && old_bank < SOUND_BANK_COUNT) {
            unload_filename[0] = '\0';
            strcat(unload_filename, sbank_data[old_bank].name);
            strcat(unload_filename, ".msb");
            if (sbank_data[old_bank].handle != 0) {
                mslBankUnLoad(sbank_data[old_bank].handle);
                sbank_data[old_bank].handle = 0;
            } else if (sbank_data[old_bank].async_state == 1) {
                switch (mslBankLoadAsyncCancelNamed(unload_filename)) {
                case 0:
                    if (sbank_data[old_bank].handle != 0) {
                        mslBankUnLoad(sbank_data[old_bank].handle);
                    }
                    sbank_data[old_bank].handle = 0;
                    break;
                case 1:
                    sbank_data[old_bank].handle = 0;
                    break;
                default:
                    sbank_data[old_bank].handle = 0;
                    break;
                }
            }
        }
        loaded_sbank_data[slot].bank_index = -1;
        loaded_sbank_data[slot].active = 0;
    }

    if (bank != -1) {
        SoundBankData* bank_data = &sbank_data[bank];

        if (bank_data->handle == 0 && bank != -1) {
            load_filename[0] = '\0';
            strcat(load_filename, bank_data->name);
            strcat(load_filename, ".msb");
            if (bank_data->handle == 0 && sbank_data[bank].async_state == 0) {
                bank_data->handle = 0;
                sbank_data[bank].async_state = 1;
                sbank_data[bank].callback.response.callback = lsba_callbank;
                sbank_data[bank].callback.response.result = 0;
                sbank_data[bank].callback.bank_index = &bank_data->bank_index;
                sbank_data[bank].callback.response.status = 0;
                mslBankLoadAsync(msi, 0, load_filename, &bank_data->callback.response);
            }
        }
        loaded_sbank_data[slot].bank_index = bank;
        loaded_sbank_data[slot].active = sbank_data[bank].active;
    }
}


#undef CANCEL_OR_UNLOAD_BANK

struct DelayedSoundPdata {
    MkHdr hdr;
    int sound_id;
};

static float p_snd_req_delay(void);

void snd_req_delay(int sound_id, int delay) {
    struct DelayedSoundPdata* pdata;
    MkProc* proc;

    if (sound_id == -1 || sound_id < 0 || sound_id >= 0x1C0C) {
        return;
    }
    proc = _create_mkproc_generic_tinystack(
        0x3006, 0x27, p_snd_req_delay, sizeof(struct DelayedSoundPdata),
        (MkHdr**)&pdata);
    if (proc != 0) {
        proc->sleep_ticks = delay;
        pdata->sound_id = sound_id;
    }
}

static float p_snd_req_delay(void) {
    struct SoundRequest request;
    int sound_id;

    if (aproc->pid != 0x3006) {
        return -1.0f;
    }
    if (apdata != 0) {
        sound_id = ((struct DelayedSoundPdata*)apdata)->sound_id;
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
    return -1.0f;
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
    struct SoundRequest request;
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
        if (pitch < 0.5f) {
            pitch = 0.5f;
        }
        if (pitch > 1.5f) {
            pitch = 1.5f;
        }
        entry = &mk_sound_table[sound_id];
        handle = mslBankPlayVolPanPitch(
            request.bank, entry->bank, entry->sound, entry->field_0c, request.volume,
            pan, pitch, entry->field_18);
    }
    return handle;
}

static inline MslSoundHandle snd_play_pan_volume(int sound_id, float pan,
                                                 float volume)
{
    struct SoundRequest request;
    MslSoundHandle handle;

    handle = 0;
    if (sound_id == -1) {
        return handle;
    }
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
    return handle;
}

MslSoundHandle pan_vol_snd_req(int sound_id, float pan, float volume) {
    if (sound_id == -1) {
        return 0;
    }
    if (sound_id < 0 || sound_id >= 0x1C0C) {
        return 0;
    }
    return snd_play_pan_volume(sound_id, pan, volume);
}

static inline MslSoundHandle play_panned_sound(
    int sound_id, float pan, float volume) {
    struct SoundRequest request;
    MslSoundHandle handle = 0;
    SoundEntry* entry;

    if (sound_id == -1) {
        return handle;
    }
    if (sound_id < 0 || sound_id >= 0x1C0C) {
        return 0;
    }
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
    return handle;
}

MslSoundHandle pan_snd_req(int sound_id, float pan) {
    if (sound_id == -1) {
        return 0;
    }
    if (sound_id < 0 || sound_id >= 0x1C0C) {
        return 0;
    }
    return play_panned_sound(sound_id, pan, 1.0f);
}

void set_snd_vol(MslSoundHandle handle, int sound_id, float volume) {
    struct SoundRequest request;

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
    struct SoundRequest request;
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

MslSoundHandle snd_req(int sound_id) {
    if (sound_id == -1) {
        return 0;
    }
    if (sound_id < 0 || sound_id >= 0x1C0C) {
        return 0;
    }
    return play_sound_vol(sound_id, 1.0f);
}

static int snd_calculate_volume(struct SoundRequest* request) {
    int result = 0;
    int sound_id;
    mslLoadedBank* bank;
    unsigned char subgroup_index;
    unsigned char setting_index;
    int bank_index;

    if (request->volume < 0.0f) {
        request->volume = 0.0f;
    }
    if (request->volume > 1.0f) {
        request->volume = 1.0f;
    }
    sound_id = request->sound_id;
    if (sound_id >= 0 && (unsigned int)sound_id < 0x1C0C) {
        if (mode_of_play == 6 && mk_sound_table[sound_id].bank_index == 7) {
            bank = sbank_data[0x72].handle;
        } else {
            bank = sbank_data[mk_sound_table[sound_id].bank_index].handle;
        }
        request->bank = bank;
        if (request->bank != 0) {
            subgroup_index = mk_sound_table[request->sound_id].subgroup;
            if (subgroup_index < 12) {
                setting_index = subgroup_volume[subgroup_index].setting_index;
                if (setting_index < 6) {
                    request->volume =
                        game_settings.volume[setting_index] *
                        (request->volume *
                         mk_sound_table[request->sound_id].base_volume *
                         subgroup_volume[subgroup_index].volume);
                    bank_index = mk_sound_table[request->sound_id].bank_index;
                    if (request->apply_group_volume != 0 && mode_of_play == 6 &&
                        bank_index != 0x71 && bank_index < 0x5F) {
                        request->volume *= 0.55f;
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
    }
    return result;
}

#pragma auto_inline off

MslSoundHandle plyr_snd_req_no_plyr_proc(
    PlyrPdata* fighter, int sound_offset) {
    MslSoundHandle handle = 0;
    int player_bank = -1;
    int voice_offset = 0;

    if (sound_offset >= 0 && sound_offset < 0x5C) {
        if (fighter == g_game_info.plyr0.slot.pdata) {
            player_bank = loaded_sbank_data[9].active;
            if (player_bank == 0x13) {
                voice_offset = p1_char_fvoice ? 0 : 0x5C;
            }
        }
        if (fighter == g_game_info.plyr1.slot.pdata) {
            player_bank = loaded_sbank_data[10].active;
            if (player_bank == 0x13) {
                voice_offset = p2_char_fvoice ? 0 : 0x5C;
            }
        }
        if (player_bank >= 0 && player_bank < 0x19) {
            handle = play_sound(
                sound_offset + mk_plyr_sound_table[player_bank] + voice_offset);
        }
    }
    return handle;
}

#pragma auto_inline reset

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

MslSoundHandle foot_snd_req(int foot_type) {
    MslSoundHandle handle = 0;
    int foot_bank;

    if (mode_of_play == 6) {
        return 0;
    }
    if (foot_type >= 0 && foot_type < 7) {
        foot_bank = loaded_sbank_data[8].active;
        if (foot_bank >= 0 && foot_bank < 4) {
            handle = play_sound(mk_foot_sound_table[foot_type][foot_bank]);
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

#pragma dont_inline reset

static inline MslSoundHandle play_random_sound(
    int sound_id, float pan, float volume, float pitch) {
    struct SoundRequest request;
    MslSoundHandle handle = 0;

    if (sound_id == -1) {
        return handle;
    }
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
            if (pitch < 0.5f) {
                pitch = 0.5f;
            }
            if (pitch > 1.5f) {
                pitch = 1.5f;
            }
            entry = &mk_sound_table[sound_id];
            handle = mslBankPlayVolPanPitch(
                request.bank, entry->bank, entry->sound, entry->field_0c,
                request.volume, pan, pitch, entry->field_18);
        }
    }
    return handle;
}

MslSoundHandle pan_vol_pitch_random_snd_req(int group, float pan, float volume, float pitch) {
    int* sounds;
    unsigned char previous;
    int count;
    MslSoundHandle handle = 0;
    int choice;
    int sound_id;

    if (group >= 0 && group < 0xB5) {
        sounds = random_sound_request[group].sounds;
        if (sounds != 0) {
            count = random_sound_request[group].count;
            previous = random_sound_request[group].previous;
            switch (count) {
            case 1:
                sound_id = sounds[0];
                handle = play_random_sound(sound_id, pan, volume, pitch);
                break;
            case 0:
                break;
            case 2: {
                int alternate;

                if (previous >= count) {
                    previous = 0;
                    random_sound_request[group].previous = 0;
                }
                alternate = 1 - previous;
                sound_id = sounds[alternate];
                handle = play_random_sound(sound_id, pan, volume, pitch);
                random_sound_request[group].previous = alternate;
                break;
            }
            default:
                if (previous >= count) {
                    previous = 0;
                    random_sound_request[group].previous = 0;
                }
                choice = randu0((unsigned short)(count - 1)) & 0xFFFF;
                if (choice >= previous) {
                    choice++;
                }
                sound_id = sounds[choice];
                handle = play_random_sound(sound_id, pan, volume, pitch);
                random_sound_request[group].previous = choice;
                break;
            }
        }
    }
    return handle;
}

MslSoundHandle random_snd_req(int group) {
    return pan_vol_pitch_random_snd_req(group, 0.0f, 1.0f, 1.0f);
}

struct DelayedRandomSoundPdata {
    MkHdr hdr;
    int group;
};

static float p_random_snd_req_delay(void);

void random_snd_req_delay(int group, int delay) {
    struct DelayedRandomSoundPdata* pdata;
    MkProc* proc;

    proc = _create_mkproc_generic_tinystack(
        0x3010, 0x27, p_random_snd_req_delay,
        sizeof(struct DelayedRandomSoundPdata), (MkHdr**)&pdata);
    if (proc != 0) {
        proc->sleep_ticks = delay;
        pdata->group = group;
    }
}

static float p_random_snd_req_delay(void) {
    if (aproc->pid != 0x3010) {
        return -1.0f;
    }
    if (apdata != 0) {
        pan_vol_pitch_random_snd_req(
            ((struct DelayedRandomSoundPdata*)apdata)->group, 0.0f, 1.0f, 1.0f);
    }
    return -1.0f;
}

MslSoundHandle pan_vol_pitch_random_hit(
    int group, float pan, float volume, float pitch) {
    MslSoundHandle result = 0;
    int sound_id;
    int* sounds;
    int count;
    SoundCallTable* table;
    unsigned int choice;

    if (group >= 0 && group < 0x13) {
        if (mode_of_play == 6) {
            table = pf_hit_call_table;
            sounds = table[group].sounds;
            count = table[group].count;
        } else {
            table = hit_call_table;
            sounds = table[group].sounds;
            count = table[group].count;
        }

        if (sounds != 0 && count > 0) {
            MslSoundHandle handle;
            choice = randu0((unsigned short)count) & 0xFFFF;
            sound_id = sounds[choice];
            handle = play_random_sound(sound_id, pan, volume, pitch);
            result = handle;
        }
    }
    return result;
}

MslSoundHandle random_hit(int group) {
    return pan_vol_pitch_random_hit(group, 0.0f, 1.0f, 1.0f);
}


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
