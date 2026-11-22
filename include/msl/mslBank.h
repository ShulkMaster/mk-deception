#ifndef MSL_BANK_H
#define MSL_BANK_H

#include "dolphin/sp.h"
#include "msl/mslARam.h"

#include "mw/mwFile.h"
#include "msl/mslBankLoadAsyncQueue.h"
#include "msl/CriticalSection.h"
#include "msl/listpool.h"
#include "msl/mslqueue.h"

struct _mslSound;
struct mslLoadedBank;
struct mslBankWaveEntry;
struct mslBankSoundDefinition;
struct mslCmdItem;
struct mslWave;
struct mslRuntimeWave;
struct mslAssetWave;
class SoundBuffer_Playable;
typedef SoundBuffer_Playable mslPlayable;

struct mslInitParam {
    u32 size;
    u32 flags;
    u32 track_count;
};

struct mslSysInitParam {
    u32 size;
    u32 flags;
    unsigned short first_voice;
    unsigned short voice_count;
    u32 aram_base;
    u32 aram_size;
};

template <class T>
union mslRelocPtr {
    u32 offset;
    s32 token;
    T* pointer;
};

enum _mslError_e {
    MSL_ERROR_NONE = 0,
    MSL_ERROR_MEMORY = 1,
    MSL_ERROR_FILE_OPEN = 2,
    MSL_ERROR_BANK_FORMAT = 3,
    MSL_ERROR_WAVES_OPEN = 5,
    MSL_ERROR_ASYNC_READ = 9,
    MSL_ERROR_SYSTEM = 10
};

struct _mslSystem {
    MslCriticalSection critical_section;
    u32 track_count;
    struct mslTrack* tracks;
    u32 reservedD4;
    _ListNode* active_sounds;
    _ListNode* active_adjustments;
    unsigned char padE0[8];
    _ListNode* reservedE8;
    struct mslCallback* callbacks;
    int pending_bank_loads;
    float volume;
    float pan;
    float pitch;
    float duck_volume;
    float duck_pan;
    float duck_pitch;
    int sound_mode;
    int sound_list_guard;
    char bank_path[0x100];
    u32 flags;
};

typedef char MslSystemSize[
    sizeof(_mslSystem) == 0x218 ? 1 : -1];

#ifdef __cplusplus
extern "C" {
#endif

_mslSystem* mslInit(
    mslInitParam* init, mslSysInitParam* system_init);
int mslSetWavePath(_mslSystem* system, const char* path);
void mslTickCallBack_Queue(
    void (*callback)(void*), void* callback_data);

#ifdef __cplusplus
}
#endif

struct mslTrack {
    _mslSound* sound;
    mslQueue* queue;
};

struct mslAsyncBank {
    char filename[0x100];
    _mwFile* sounds_file;
    _mwFile* waves_file;
    u32 sounds_size;
    mslLoadedBank* bank_data;
    _mslAsyncResponse* response;
    _mslSystem* system;
    u32 asset_version;
    s32 entry_count;
    u32 asset_info_size;
    u32 string_offset;
    u32 wave_offset;
    u32 wave_size;
    u32 read_state;
};

struct mslBankSoundEntry {
    mslBankSoundDefinition* definition;
    _mslSound* sound;
    mslLoadedBank* owner_bank;
    u32 flags;
};

struct _mslSound {
    unsigned char pad00[0x30];
    mslLoadedBank* owner_bank;
};

struct mslRuntimeSound {
    mslBankSoundDefinition* definition;
    mslCmdItem* current_command;
    float end_time;
    int track;
    int priority;
    float volume;
    float pan;
    float pitch;
    float volume_scale;
    float pan_offset;
    float pitch_scale;
    mslRuntimeWave* waves;
    mslLoadedBank* owner_bank;
    mslBankSoundEntry* bank_sound_entry;
    _ListNode* adjustments;
    _mslSystem* system;
    float update_time;
    u32 flags;
    int bank_ref_count;
    unsigned char pad4C[4];
    int callback_data;
};

struct mslWave {
    u32 flags;
};

struct mslRuntimeWave {
    u32 flags;
    mslPlayable* playable;
    mslAssetWave* file_entry;
    int use_count;
    u32 unknown10;
    float volume;
    float pan;
    float pitch;
    mslRuntimeSound* sound;
    mslRuntimeWave** previous_link;
    mslRuntimeWave* next;
    int command_value;
    int play_state;
    u32 unknown34;
};

struct mslAssetWave {
    mslRelocPtr<char> name;
    u32 primary_aram_offset;
    u32 aram_size;
    mslRelocPtr<SPSoundTable> sound_table;
    unsigned char pad10[4];
    int resident;
    int has_secondary;
    mslRelocPtr<char> secondary_name;
    u32 secondary_aram_offset;
    unsigned char pad24[4];
    mslRelocPtr<SPSoundTable> secondary_sound_table;
    unsigned char pad2C[4];
};

struct mslAssetInfo {
    unsigned char pad00[0x18];
    void* temporary_names;
    mslAssetWave waves[1];

    char* At(u32 offset) {
        return (char*)this + offset;
    }
};

struct mslBankWaveEntry {
    mslRuntimeWave* wave;
    mslLoadedBank* owner_bank;
    mslRelocPtr<char> name;
    unsigned char pad0C[0x14];
    u32 flags;
    unsigned char pad24[0x24];
};

struct mslCmdItem {
    mslRelocPtr<void> source;
    mslRelocPtr<void> target;
    signed char type;
    unsigned char pad09;
    short wave_value;
    int unknown0C;
    int command_state;
    mslRuntimeWave* attached_wave;
    float value;
    float unknown1C;
    float unknown20;
    float unknown24;
    float unknown28;
    u32 unknown2C;
};

struct mslCallback {
    const char* name;
    void (*function)(const char*);
    unsigned char pad08[4];
    mslCallback* next;
};

struct mslAdjustment {
    float start_time;
    float start_volume;
    float start_pan;
    float start_pitch;
    float end_time;
    float end_volume;
    float end_pan;
    float end_pitch;
    u32 flags;
    mslRuntimeWave* wave;
    mslRuntimeSound* sound;
};

struct mslBankSoundDefinition {
    unsigned char pad00[8];
    int command_count;
    mslCmdItem* commands;
};

struct mslLoadedBank {
    int version;
    unsigned int flags;
    short wave_count;
    short sound_count;
    short sound_id_count;
    unsigned char pad0E[2];
    mslRelocPtr<mslBankWaveEntry> waves;
    mslRelocPtr<mslBankSoundEntry> sounds;
    mslRelocPtr<mslBankSoundDefinition> definitions;
    mslRelocPtr<mslCmdItem> command_items;
    mslRelocPtr<short> sound_ids;
    mslRelocPtr<void> unknown24;
    mslRelocPtr<char> string_table;
    unsigned char pad2C[4];
    _mslSystem* system;
    mslLoadedBank* next;
    mslLoadedBank* previous;
    _mwFile* waves_file;
    mslAssetInfo* asset_info;
    MSLGCN_ARamBlock* resident_aram_block;

    char* At(u32 offset) {
        return (char*)this + offset;
    }
};

typedef void (*mslAsyncSoundCallback)(
    bool loaded, mslBankSoundEntry* bank_sound, void* data);

#ifdef __cplusplus
extern "C" {
#endif

int mslBankSoundUnUse(mslBankSoundEntry* bank_sound);
_ListNode* mslBankSoundUse(
    mslBankSoundEntry* bank_sound, _mslSystem* system);
void callbackPlay(
    bool loaded, mslBankSoundEntry* bank_sound, void* data);
void asyncLoadSound(
    _mslSystem* system, mslLoadedBank* bank,
    mslBankSoundEntry* bank_sound, mslAsyncSoundCallback callback,
    void* data);
void* mslBankUnLoad(mslLoadedBank* bank);
void* mslBankUpdatePtrs(mslLoadedBank* bank);
int mslBankUse(_mslSystem* system, mslLoadedBank* bank);
mslAssetWave* mslBankFileEntryFind(
    mslLoadedBank* bank, const char* name);
mslBankWaveEntry* mslBankWavesFind(
    mslLoadedBank* bank, const char* name);

_ListNode* mslSoundNew(_mslSystem* system, int priority);
_mslSound* mslSoundLoad(
    _mslSystem* system, mslLoadedBank* bank,
    mslBankSoundDefinition* definition, u32 flags);
int mslSoundAttach(
    mslRuntimeSound* sound, mslBankSoundEntry* bank_sound);
int mslSoundPlayNow(_ListNode* node);
int mslSoundEnd(_mslSound* sound);
int mslSoundIsReady(_mslSound* sound);
int mslCmdsLoad(
    _mslSystem* system, mslLoadedBank* bank,
    mslBankSoundDefinition* definition, u32 flags);
void mslSoundUnCopy(_ListNode* node);
void mslSoundUncommit(_mslSound* sound);
int mslSoundUnLoad(_mslSound* sound);
void mslUpdateTracks(_mslSystem* system);
int mslUpdate(_mslSystem* system);

#ifdef __cplusplus
}
#endif

void mslBankLoadAsyncInternal(
    _mslSystem* system, u32 flags, char* filename,
    _mslAsyncResponse* response);
void mslBankOpenWavesComplete(
    mwFileCommand* command, _mwFileAsyncResult result, void* callback_data);
void mslBankOpenSoundsComplete(
    mwFileCommand* command, _mwFileAsyncResult result, void* callback_data);

#endif
