#ifndef MSL_BANK_H
#define MSL_BANK_H

#include "dolphin/sp.h"
#include "dolphin/ar.h"

#include "mw/mwFile.h"
#include "msl/mslBankLoadAsyncQueue.h"
#include "msl/CriticalSection.h"
#include "msl/listpool.h"
#include "msl/mslqueue.h"
#include "msl/msl_types.h"

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
    unsigned long size;           /* +0x00 */
    unsigned long flags;          /* +0x04 */
    unsigned long track_count;    /* +0x08 */
}; /* 0x0C */

struct mslSysInitParam {
    unsigned long size;           /* +0x00 */
    unsigned long flags;          /* +0x04 */
    unsigned short first_voice;   /* +0x08 */
    unsigned short voice_count;   /* +0x0A */
    unsigned long aram_base;      /* +0x0C */
    unsigned long aram_size;      /* +0x10 */
}; /* 0x14 */

struct mslARQRequest {
    ARQRequest request;          /* +0x00 -- Nintendo DMA queue request */
    /* +0x20 is the free-list link before checkout and the DMA buffer after. */
    union {
        void* stream_buffer;
        mslARQRequest* next_free;
    };                            /* +0x20 */
    void* callback_data;          /* +0x24 */
}; /* 0x28 */

typedef char MslARQRequestSize[
    sizeof(mslARQRequest) == 0x28 ? 1 : -1];

template <class T>
union mslRelocPtr {
    /* Serialized offsets/tokens are rewritten in place to runtime pointers. */
    unsigned long offset;
    long token;
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
    MslCriticalSection critical_section; /* +0x000 */
    unsigned long track_count;    /* +0x0CC */
    struct mslTrack* tracks;      /* +0x0D0 */
    unsigned long reservedD4;     /* +0x0D4 */
    _ListNode* active_sounds;     /* +0x0D8 */
    _ListNode* active_adjustments;/* +0x0DC */
    unsigned char padE0[8];
    _ListNode* reservedE8;         /* +0x0E8 */
    struct mslCallback* callbacks;/* +0x0EC */
    int pending_bank_loads;       /* +0x0F0 */
    float volume;                 /* +0x0F4 */
    float pan;                    /* +0x0F8 */
    float pitch;                  /* +0x0FC */
    float duck_volume;            /* +0x100 */
    float duck_pan;               /* +0x104 */
    float duck_pitch;             /* +0x108 */
    int sound_mode;               /* +0x10C */
    int sound_list_guard;         /* +0x110 */
    char bank_path[0x100];         /* +0x114 */
    unsigned long flags;          /* +0x214 */
}; /* 0x218 */

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
    _mslSound* sound;             /* +0x00 */
    mslQueue* queue;              /* +0x04 */
}; /* 0x08 */

struct mslAsyncBank {
    char filename[0x100];         /* +0x000 */
    _mwFile* sounds_file;         /* +0x100 */
    _mwFile* waves_file;          /* +0x104 */
    unsigned long sounds_size;    /* +0x108 */
    mslLoadedBank* bank_data;     /* +0x10C */
    _mslAsyncResponse* response;  /* +0x110 */
    _mslSystem* system;           /* +0x114 */
    unsigned long asset_version;  /* +0x118 */
    long entry_count;             /* +0x11C; retail loop bounds are signed */
    unsigned long asset_info_size;/* +0x120 */
    unsigned long string_offset;  /* +0x124 */
    unsigned long wave_offset;    /* +0x128 */
    unsigned long wave_size;      /* +0x12C */
    unsigned long read_state;     /* +0x130 */
}; /* 0x134 */

struct mslBankSoundEntry {
    mslBankSoundDefinition* definition; /* +0x00 */
    _mslSound* sound;             /* +0x04 */
    mslLoadedBank* owner_bank;    /* +0x08 */
    unsigned long flags;          /* +0x0C */
}; /* 0x10 */

struct _mslSound {
    unsigned char pad00[0x30];
    mslLoadedBank* owner_bank;    /* +0x30 */
};

/*
 * Full overlay used by the bank play path. Keep the smaller _mslSound view
 * for ownership-only loops: MWCC 2.7 crashes while optimizing that partial
 * TU when the larger layout is substituted directly.
 */
struct mslRuntimeSound {
    mslBankSoundDefinition* definition; /* +0x00 */
    mslCmdItem* current_command;  /* +0x04 */
    float end_time;               /* +0x08 */
    int track;                    /* +0x0C */
    int priority;                 /* +0x10 */
    float volume;                 /* +0x14 */
    float pan;                    /* +0x18 */
    float pitch;                  /* +0x1C */
    float volume_scale;           /* +0x20 */
    float pan_offset;             /* +0x24 */
    float pitch_scale;            /* +0x28 */
    mslRuntimeWave* waves;        /* +0x2C */
    mslLoadedBank* owner_bank;    /* +0x30 */
    mslBankSoundEntry* bank_sound_entry; /* +0x34 */
    _ListNode* adjustments;       /* +0x38 */
    _mslSystem* system;           /* +0x3C */
    float update_time;            /* +0x40 */
    unsigned long flags;          /* +0x44 */
    int bank_ref_count;           /* +0x48 */
    unsigned char pad4C[4];
    void* callback_data;          /* +0x50 */
};

struct mslWave {
    unsigned long flags;          /* +0x00 */
};

struct mslRuntimeWave {
    unsigned long flags;          /* +0x00 */
    mslPlayable* playable;        /* +0x04 */
    mslAssetWave* file_entry;     /* +0x08 */
    int use_count;                /* +0x0C */
    unsigned long unknown10;      /* +0x10 */
    float volume;                 /* +0x14 */
    float pan;                    /* +0x18 */
    float pitch;                  /* +0x1C */
    mslRuntimeSound* sound;        /* +0x20 */
    mslRuntimeWave** previous_link;/* +0x24 */
    mslRuntimeWave* next;          /* +0x28 */
    int command_value;            /* +0x2C */
    int play_state;               /* +0x30 */
    unsigned long unknown34;      /* +0x34 */
};

class MSLGCN_ARamBlock {
public:
    int reference_count;          /* +0x00 */
    MSLGCN_ARamBlock* parent;      /* +0x04 */
    unsigned char channel_count;  /* +0x08 */
    unsigned char unknown09;      /* +0x09 */
    unsigned char allocation_kind;/* +0x0A */
    unsigned char owns_buffers;   /* +0x0B */
    int buffer_size;              /* +0x0C */
    int base;                     /* +0x10 -- primary ARAM buffer */
    int secondary_base;           /* +0x14 */

    static MSLGCN_ARamBlock* CreateBankBlock(int size);
    static MSLGCN_ARamBlock* GetObject(void);
    static void FreeObject(MSLGCN_ARamBlock* block);
    ~MSLGCN_ARamBlock();
    void FreeResources(void);
    void Release(void);
    void SetParent(MSLGCN_ARamBlock* parent);
    void SetARamBuffers(int primary, int secondary, int size);
    void SetNumChannels(int channels);
};

struct mslAssetWave {
    mslRelocPtr<char> name;       /* +0x00 */
    unsigned long primary_aram_offset; /* +0x04 */
    unsigned long aram_size;      /* +0x08 */
    SPSoundTable* sound_table;    /* +0x0C */
    unsigned char pad10[4];
    int resident;                 /* +0x14 */
    int has_secondary;            /* +0x18 */
    mslRelocPtr<char> secondary_name; /* +0x1C */
    unsigned long secondary_aram_offset; /* +0x20 */
    unsigned char pad24[4];
    SPSoundTable* secondary_sound_table; /* +0x28 */
    unsigned char pad2C[4];
}; /* 0x30 */

struct mslAssetInfo {
    unsigned char pad00[0x18];
    void* temporary_names;        /* +0x18 */
    mslAssetWave waves[1];        /* +0x1C */

    char* At(unsigned long offset) {
        return (char*)this + offset;
    }
};

struct mslBankWaveEntry {
    mslRuntimeWave* wave;         /* +0x00 */
    mslLoadedBank* owner_bank;    /* +0x04 */
    mslRelocPtr<char> name;       /* +0x08 */
    unsigned char pad0C[0x14];
    unsigned long flags;          /* +0x20 */
    unsigned char pad24[0x24];
}; /* 0x48 */

struct mslCmdItem {
    mslRelocPtr<void> source;     /* +0x00 */
    mslRelocPtr<void> target;     /* +0x04 */
    signed char type;             /* +0x08 */
    unsigned char pad09;
    short wave_value;             /* +0x0A */
    int unknown0C;                /* +0x0C */
    int command_state;            /* +0x10 */
    mslRuntimeWave* attached_wave;/* +0x14 */
    float value;                  /* +0x18 */
    float unknown1C;              /* +0x1C */
    float unknown20;              /* +0x20 */
    float unknown24;              /* +0x24 */
    float unknown28;              /* +0x28 */
    unsigned long unknown2C;      /* +0x2C */
}; /* 0x30 */

struct mslCallback {
    const char* name;             /* +0x00 */
    void (*function)(const char*);/* +0x04 */
    unsigned char pad08[4];
    mslCallback* next;            /* +0x0C */
};

struct mslAdjustment {
    float start_time;             /* +0x00 */
    float start_volume;           /* +0x04 */
    float start_pan;              /* +0x08 */
    float start_pitch;            /* +0x0C */
    float end_time;               /* +0x10 */
    float end_volume;             /* +0x14 */
    float end_pan;                /* +0x18 */
    float end_pitch;              /* +0x1C */
    unsigned long flags;          /* +0x20 */
    mslRuntimeWave* wave;         /* +0x24 */
    mslRuntimeSound* sound;       /* +0x28 */
}; /* 0x2C */

struct mslBankSoundDefinition {
    unsigned char pad00[8];
    int command_count;            /* +0x08 */
    mslCmdItem* commands;         /* +0x0C */
}; /* 0x10 */

struct mslLoadedBank {
    int version;                  /* +0x00 -- retail requires 11 */
    unsigned int flags;           /* +0x04 -- format feature flags */
    short wave_count;             /* +0x08 */
    short sound_count;            /* +0x0A */
    short sound_id_count;          /* +0x0C */
    unsigned char pad0E[2];
    mslRelocPtr<mslBankWaveEntry> waves; /* +0x10 */
    mslRelocPtr<mslBankSoundEntry> sounds; /* +0x14 */
    mslRelocPtr<mslBankSoundDefinition> definitions; /* +0x18 */
    mslRelocPtr<mslCmdItem> command_items; /* +0x1C */
    mslRelocPtr<short> sound_ids; /* +0x20 -- external ID -> sound index + 1 */
    mslRelocPtr<void> unknown24;  /* +0x24 */
    mslRelocPtr<char> string_table; /* +0x28 */
    unsigned char pad2C[4];
    _mslSystem* system;           /* +0x30 */
    mslLoadedBank* next;          /* +0x34 */
    mslLoadedBank* previous;      /* +0x38 */
    _mwFile* waves_file;          /* +0x3C */
    mslAssetInfo* asset_info;     /* +0x40 */
    MSLGCN_ARamBlock* resident_aram_block; /* +0x44 */

    char* At(unsigned long offset) {
        return (char*)this + offset;
    }
};

typedef void (*mslAsyncSoundCallback)(
    bool loaded, mslBankSoundEntry* bank_sound, _ListNode* node);

#ifdef __cplusplus
extern "C" {
#endif

int mslBankSoundUnUse(mslBankSoundEntry* bank_sound);
_ListNode* mslBankSoundUse(
    mslBankSoundEntry* bank_sound, _mslSystem* system);
void callbackPlay(
    bool loaded, mslBankSoundEntry* bank_sound, _ListNode* node);
void asyncLoadSound(
    _mslSystem* system, mslLoadedBank* bank,
    mslBankSoundEntry* bank_sound, mslAsyncSoundCallback callback,
    _ListNode* node);
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
    mslBankSoundDefinition* definition, unsigned long flags);
int mslSoundAttach(
    mslRuntimeSound* sound, mslBankSoundEntry* bank_sound);
int mslSoundPlayNow(_ListNode* node);
int mslSoundEnd(_mslSound* sound);
int mslSoundIsReady(_mslSound* sound);
int mslCmdsLoad(
    _mslSystem* system, mslLoadedBank* bank,
    mslBankSoundDefinition* definition, unsigned long flags);
void mslSoundUnCopy(_ListNode* node);
void mslSoundUncommit(_mslSound* sound);
int mslSoundUnLoad(_mslSound* sound);
void mslUpdateTracks(_mslSystem* system);
int mslUpdate(_mslSystem* system);

#ifdef __cplusplus
}
#endif

void mslBankLoadAsyncInternal(
    _mslSystem* system, unsigned long flags, char* filename,
    _mslAsyncResponse* response);
void mslBankOpenWavesComplete(
    mwFileCommand* command, _mwFileAsyncResult result, void* callback_data);
void mslBankOpenSoundsComplete(
    mwFileCommand* command, _mwFileAsyncResult result, void* callback_data);

#endif
