#include "msl/mslBank.h"
#include "msl/mslWave.h"
#include "msl/CriticalSection.h"
#include "msl/ExtHeapMgr.h"
#include "dolphin/os.h"
#include "dolphin/ax.h"
#include "dolphin/arq.h"
#include "dolphin/ai.h"
#include "msl/mslsupport.h"
#include "msl/mslStreamCache.h"
#include "msl/mslStreamFile.h"
#include "msl/mslARam.h"
#include "msl/mslgcn.h"
#include "mw/mwMemHeap.h"
#include "runtime/cstring.h"

struct mslGCNPlayable;
struct _mslBank;
struct _GameCubeFileEntry;

typedef int (*mslPlayablePlay)(
    mslGCNPlayable* playable, int loop);
typedef void (*mslPlayableStop)(mslGCNPlayable* playable);
typedef int (*mslPlayablePause)(mslGCNPlayable* playable);
typedef void (*mslPlayableFreeObject)(mslGCNPlayable* playable);

struct mslGCNPlayableVTable {
    void* reserved00[3];          /* +0x00 */
    mslPlayableFreeObject FreeObject; /* +0x0C */
    void* reserved10[12];         /* +0x10 */
    mslPlayablePlay Play;         /* +0x40 */
    mslPlayableStop Stop;         /* +0x44 */
    mslPlayablePause Pause;       /* +0x48 */
    mslPlayablePause UnPause;     /* +0x4C */
};

struct mslGCNPlayable {
    mslGCNPlayableVTable* vtable;
    int reference_count;
};

static inline mslGCNPlayable* MslGCNPlayableFromWave(
    mslRuntimeWave* wave) {
    return (mslGCNPlayable*)wave->playable;
}

extern int SoundBufferCount;
extern int SoundBufferCountStatic;
extern int SoundBufferCountStream;

struct mslTickCallback {
    void (*callback)(void*);
    void* data;
};

static void* msl_SystemMalloc(unsigned int);
static void msl_SystemFree(void*);
static void msl_SystemEnterMutex(void*);
static void msl_SystemExitMutex(void*);
extern unsigned long g_MSL_GCN_ARAM_ZeroBase;
extern unsigned long g_MSL_GCN_ARAM_ZeroBase_ADPCM_Start;
extern unsigned long g_MSL_GCN_ARAM_ZeroBase_ADPCM_End;
extern ExternalHeap* g_MSL_GCN_ARAM_Heap;
extern MslCriticalSection g_MSL_GCN_ARAM_CriticalSection;

class SoundBuffer {
public:
    static mslPlayable* CreatePlayableStaticBuffer(
        _mslBank* bank, _GameCubeFileEntry* entry);
    static mslPlayable* CreatePlayableStreamBuffer(
        _mslBank* bank, _GameCubeFileEntry* entry);
    static void SB_MslTickCallback(void);
    static void SB_AXUserCallback(void);
};

ListPool g_listPoolSound;
ListPool g_listPoolAdjust;
unsigned char g_listMemSound[0x2BF20];
unsigned char g_listMemAdjust[0x1100];
_mslSystem* gMsi;
int SoundBufferCount;
int SoundBufferCountStream;
int SoundBufferCountStatic;
unsigned long mslGCN_AXCallback_Ticks;
ExternalHeap* g_MSL_GCN_ARAM_Heap;
unsigned long g_MSL_GCN_ARAM_ZeroBase;
unsigned long g_MSL_volatile_flag;
unsigned long g_MSL_GCN_ARAM_ZeroBase_ADPCM_Start;
unsigned long g_MSL_GCN_ARAM_ZeroBase_ADPCM_End;
int debugger_mbo1;
int debugger_at1;
int debugger_snd;

static mslInitParam s_initDefault = {
    sizeof(mslInitParam), 1, 10
};

int g_bMSL_GCN_BREAK = 1;
MslCriticalSection g_MSL_GCN_ARAM_CriticalSection;
mslTickCallback g_mslTickCB_Queue[20];
int g_mslTickCB_Head;
int g_mslTickCB_Tail;
int g_mslTickCB_NumberItems;

void _MSL_GCN_BREAK(void) {
    mslDebugPrintf("MSL DID SOMETHING BAD!!!\n");
    while (g_bMSL_GCN_BREAK != 0) {
    }
    debugger_mbo1 = 0;
    debugger_at1 = 0;
    debugger_snd = 0;
    OSPanic("mslgcn.cpp", 0x66D, "UNSUPPORTED FUNCTION");
}

extern "C" int ContinueStream(
    _mslSystem* system, mslRuntimeWave* wave) {
    mslGCNPlayable* playable =
        MslGCNPlayableFromWave(wave);
    int result;

    if (playable == 0) {
        return -1;
    }

    result = playable->vtable->UnPause(playable);
    if (result < 0) {
        mslDebugPrintf(
            "ContinueStream failed DS->UnPause.  HR=%08x\n",
            result);
    }
    return result;
}

extern "C" int PauseStream(
    _mslSystem* system, mslRuntimeWave* wave) {
    mslGCNPlayable* playable =
        MslGCNPlayableFromWave(wave);

    if (playable == 0) {
        return -1;
    }
    return playable->vtable->Pause(playable);
}

extern "C" void StopStream(
    _mslSystem* system, mslRuntimeWave* wave) {
    mslGCNPlayable* playable =
        MslGCNPlayableFromWave(wave);

    if (playable != 0) {
        playable->vtable->Stop(playable);
    }
}

/* Soft ceiling: ~96.76% -- retail failure-region return is recovered;
 * remaining differences are virtual-table scratch allocation and the
 * partial-TU string pool.
 */
extern "C" int PlayStream(
    _mslSystem* system, mslRuntimeSound* sound,
    mslRuntimeWave* wave, int allow_voice) {
    int result = -1;
    int loop;
    mslGCNPlayable* playable =
        MslGCNPlayableFromWave(wave);

    if (playable == 0) {
        return -1;
    }
    if (allow_voice == 1) {
        loop = 0;
        if ((wave->flags & 1) != 0) {
            loop = 1;
        }
        result = playable->vtable->Play(playable, loop);
        if (result < 0) {
            mslDebugPrintf(
                "PlayStream failed DS->Play.  HR=%08x\n",
                result);
            return result;
        }
    }
    return result;
}

extern "C" void UnCopyStreamWave(
    _mslSystem* system, mslRuntimeWave* wave) {
    if (wave->playable != 0) {
        mslGCNPlayable* playable;

        SoundBufferCountStream--;
        SoundBufferCount--;
        playable = MslGCNPlayableFromWave(wave);
        if (--playable->reference_count == 0) {
            playable->vtable->FreeObject(playable);
        }
        wave->playable = 0;
    }
    _mwMemFree(wave, 0, 0);
}

/*
 * Clone the complete retail runtime-wave overlay, optionally attach a new
 * streamed playable, then replace the copied playable owner.
 * Soft ceiling: ~92.74% -- all field accesses and control flow are recovered;
 * the residue is alternating-load scheduling and GPR coloring around the
 * factory call and counters.
 */
extern "C" mslRuntimeWave* CopyStreamWave(
    _mslSystem* system, mslLoadedBank* bank, const char* name,
    const mslRuntimeWave* source, int create_playable) {
    mslPlayable* playable = 0;
    mslRuntimeWave* copy =
        (mslRuntimeWave*)_mwMemMalloc(
            MWSOUND_HEAP, sizeof(mslRuntimeWave), 3, 0, 0, 0);

    if (copy == 0) {
        return 0;
    }

    copy->flags = source->flags;
    copy->playable = source->playable;
    copy->file_entry = source->file_entry;
    copy->use_count = source->use_count;
    copy->unknown10 = source->unknown10;
    copy->volume = source->volume;
    copy->pan = source->pan;
    copy->pitch = source->pitch;
    copy->sound = source->sound;
    copy->previous_link = source->previous_link;
    copy->next = source->next;
    copy->command_value = source->command_value;
    copy->play_state = source->play_state;
    copy->unknown34 = source->unknown34;

    if (create_playable != 0) {
        playable = SoundBuffer::CreatePlayableStreamBuffer(
            (_mslBank*)bank,
            (_GameCubeFileEntry*)copy->file_entry);
        if (playable == 0) {
            _mwMemFree(copy, 0, 0);
            return 0;
        }
        SoundBufferCount++;
        SoundBufferCountStream++;
    }
    copy->playable = playable;
    return copy;
}

extern "C" mslRuntimeWave* LoadStreamWaveFile(
    _mslSystem* system, mslLoadedBank* bank, const char* name,
    unsigned long flags) {
    mslAssetWave* entry = mslBankFileEntryFind(bank, name);
    mslRuntimeWave* wave;

    if (entry == 0) {
        return 0;
    }
    wave = (mslRuntimeWave*)_mwMemCalloc(
        MWSOUND_HEAP, 1, sizeof(mslRuntimeWave), 3, 0, 0, 0);
    if (wave == 0) {
        return 0;
    }

    wave->file_entry = entry;
    wave->volume = 1.0f;
    wave->pan = 0.0f;
    wave->pitch = 1.0f;
    wave->previous_link = 0;
    wave->next = 0;
    return wave;
}

extern "C" int ContinueStatic(
    _mslSystem* system, mslRuntimeWave* wave) {
    mslGCNPlayable* playable = MslGCNPlayableFromWave(wave);
    int result;

    if (playable == 0) {
        return -1;
    }
    result = playable->vtable->UnPause(playable);
    if (result < 0) {
        mslDebugPrintf(
            "ContinueStatic failed DS->UnPause.  HR=%08x\n", result);
        return result;
    }
    return 0;
}

extern "C" int PauseStatic(
    _mslSystem* system, mslRuntimeWave* wave) {
    mslGCNPlayable* playable = MslGCNPlayableFromWave(wave);

    if (playable != 0) {
        playable->vtable->Pause(playable);
    }
    return 0;
}

/* Soft ceiling: ~99.33% -- virtual-table scratch GPR only. */
extern "C" int StopStatic(
    _mslSystem* system, mslRuntimeWave* wave) {
    mslGCNPlayable* playable =
        MslGCNPlayableFromWave(wave);

    if (playable != 0) {
        playable->vtable->Stop(playable);
    }
    return 0;
}

/*
 * Dispatch a prepared resident wave to the GameCube static playable. A
 * caller may suppress platform playback while still allowing the higher
 * MSL state machine to complete; loop polarity comes directly from bit 0.
 * Soft ceiling: ~99.6% -- exact retail size/control flow and string order;
 * only the virtual-table scratch register differs.
 */
extern "C" int PlayStatic(
    _mslSystem* system, mslRuntimeWave* wave, int allow_voice) {
    mslGCNPlayable* playable =
        MslGCNPlayableFromWave(wave);
    int result;

    if (playable == 0) {
        return -1;
    }
    if (allow_voice == 1) {
        if ((wave->flags & 1) != 0) {
            result = playable->vtable->Play(playable, 1);
            if (result < 0) {
                mslDebugPrintf(
                    "PlayStatic failed DS->Play(LOOP).  HR=%08x\n",
                    result);
                return result;
            }
        } else {
            result = playable->vtable->Play(playable, 0);
            if (result < 0) {
                mslDebugPrintf(
                    "PlayStatic failed DS->Play.  HR=%08x\n", result);
                return result;
            }
        }
    }
    return 0;
}

extern "C" void UnCopyStaticWave(
    _mslSystem* system, mslRuntimeWave* wave) {
    if (wave->playable != 0) {
        mslGCNPlayable* playable;

        SoundBufferCountStatic--;
        SoundBufferCount--;
        playable = MslGCNPlayableFromWave(wave);
        if (--playable->reference_count == 0) {
            playable->vtable->FreeObject(playable);
        }
        wave->playable = 0;
    }
    _mwMemFree(wave, 0, 0);
}

extern "C" mslRuntimeWave* CopyStaticWave(
    _mslSystem* system, mslLoadedBank* bank,
    const mslRuntimeWave* source, int create_playable) {
    mslPlayable* playable = 0;
    mslRuntimeWave* copy =
        (mslRuntimeWave*)_mwMemMalloc(
            MWSOUND_HEAP, sizeof(mslRuntimeWave), 3, 0, 0, 0);

    /* Soft ceiling: ~92.93% -- retail-exact size, fields, frame, calls, and
     * branches; remaining differences are copy scheduling and GPR coloring. */

    if (copy == 0) {
        return 0;
    }

    copy->flags = source->flags;
    copy->playable = source->playable;
    copy->file_entry = source->file_entry;
    copy->use_count = source->use_count;
    copy->unknown10 = source->unknown10;
    copy->volume = source->volume;
    copy->pan = source->pan;
    copy->pitch = source->pitch;
    copy->sound = source->sound;
    copy->previous_link = source->previous_link;
    copy->next = source->next;
    copy->command_value = source->command_value;
    copy->play_state = source->play_state;
    copy->unknown34 = source->unknown34;

    if (create_playable != 0) {
        playable =
            SoundBuffer::CreatePlayableStaticBuffer(
                (_mslBank*)bank,
                (_GameCubeFileEntry*)copy->file_entry);
        if (playable == 0) {
            _mwMemFree(copy, 0, 0);
            return 0;
        }
    }

    copy->playable = playable;
    if (playable != 0) {
        SoundBufferCount++;
        SoundBufferCountStatic++;
    }
    return copy;
}

extern "C" mslRuntimeWave* LoadStaticWaveFile(
    _mslSystem* system, mslLoadedBank* bank, const char* name,
    unsigned long flags) {
    mslAssetWave* entry = mslBankFileEntryFind(bank, name);
    mslRuntimeWave* wave;

    if (entry == 0) {
        return 0;
    }

    wave = (mslRuntimeWave*)_mwMemCalloc(
        MWSOUND_HEAP, 1, sizeof(mslRuntimeWave), 3, 0, 0, 0);
    if (wave == 0) {
        return 0;
    }

    wave->flags = flags;
    wave->file_entry = entry;
    wave->volume = 1.0f;
    wave->pan = 0.0f;
    wave->pitch = 1.0f;
    wave->sound = 0;
    wave->use_count = 0;
    wave->previous_link = 0;
    wave->next = 0;
    return wave;
}

/* Soft ceiling: mslTick ~99.3% -- remaining callback-loop GPR coloring. */
extern "C" int mslTick(void) {
    int head;
    int item_count;
    int processed;
    unsigned long enabled;

    mwFileTick();
    SoundBuffer::SB_MslTickCallback();
    mslUpdate(gMsi);

    if (g_mslTickCB_NumberItems != 0) {
        enabled = OSDisableInterrupts();
        head = g_mslTickCB_Head;
        item_count = g_mslTickCB_NumberItems;
        OSRestoreInterrupts(enabled);

        for (processed = 0; processed < item_count; processed++) {
            g_mslTickCB_Queue[head].callback(
                g_mslTickCB_Queue[head].data);
            head++;
            if (head >= 20) {
                head = 0;
            }
        }

        enabled = OSDisableInterrupts();
        g_mslTickCB_Head = head;
        g_mslTickCB_NumberItems -= processed;
        OSRestoreInterrupts(enabled);
    }
    return 0;
}

extern "C" int mslSetWavePath(
    _mslSystem* system, const char* path) {
    if (path == 0 || path[0] == '\0') {
        system->bank_path[0] = '\0';
    } else {
        strncpy(system->bank_path, path, 0xFF);
        system->bank_path[0xFF] = '\0';
    }
    return 0;
}

static inline void mslInitCleanup(_mslSystem* system) {
    unsigned long i;
    _ListNode* adjustment;
    mslCallback* callback;

    if (system == 0) {
        return;
    }

    if (system->tracks != 0) {
        for (i = 0; i < system->track_count; i++) {
            if (system->tracks[i].queue != 0) {
                mslQueueDelete(system->tracks[i].queue);
                system->tracks[i].queue = 0;
            }
            system->tracks[i].sound = 0;
        }
        _mwMemFree(system->tracks, 0, 0);
        system->tracks = 0;
    }

    adjustment = system->active_adjustments;
    while (adjustment != 0) {
        ListNodeFree(
            &g_listPoolAdjust, ListRemove(&adjustment));
    }

    callback = system->callbacks;
    while (callback != 0) {
        mslCallback* next = callback->next;
        _mwMemFree(callback, 0, 0);
        callback = next;
    }

    UnInitCriticalSection(&g_MSL_GCN_ARAM_CriticalSection);
    _mwMemFree(system, 0, 0);
}

extern "C" _mslSystem* mslInit(
    mslInitParam* init, mslSysInitParam* system_init) {
    _mslSystem* system;
    unsigned long i;
    int mode;
    int sound_mode;

    /* Soft ceiling: mslInit ~99.24% -- clean nested-loop failure handling
     * retains one redundant post-loop count guard; the remaining arithmetic
     * delta is commutative mullw operand order. */

    if (gMsi != 0) {
        mslDebugPrintf(
            "mslInit: only ONE system supported, already inited.\n");
        return 0;
    }
    if (init == 0) {
        mslDebugPrintf("DEFAULT ");
        init = &s_initDefault;
    }
    mslDebugPrintf(
        "Init: FLAGS=%d, Tracks=%d\n",
        init->flags, init->track_count);

    if (system_init == 0) {
        mslDebugPrintf("DEFAULT ");
        mslDebugPrintf(
            "mslInit: Gamecube requires mslSysInitParam.\n");
        return 0;
    }

    if (system_init->voice_count == 0) {
        system_init->voice_count = 0x40;
    }
    if (system_init->first_voice != 0) {
        mslDebugPrintf(
            "SysInit: UNSUPPORTED 'reserved' voices %d-%d, "
            "starting from 0 instead\n",
            system_init->first_voice, system_init->voice_count);
        system_init->voice_count -= system_init->first_voice;
        system_init->first_voice = 0;
    }

    mslDebugPrintf(
        "SysInit: FLAGS=%d, Voices %d-%d\n",
        system_init->flags, system_init->first_voice,
        system_init->voice_count);
    mslDebugPrintf("MSL heap %d\n", mslMainRamUsed());
    mslCreateLogTable();
    mslDebugPrintf("MSL version %s\n", "1.8.8");

    do {
        system = (_mslSystem*)_mwMemMalloc(
            MWSOUND_HEAP, sizeof(_mslSystem), 3, 0, 0, 0);
        if (system == 0) {
            mslDebugPrintf(
                "mslInit: Out of memory allocating mslSystem.\n");
            break;
        }

        ListPoolAttach(
            &g_listPoolSound, g_listMemSound, 0x708, 0x54);
        ListPoolAttach(
            &g_listPoolAdjust, g_listMemAdjust, 0x40, 0x34);

        gMsi = system;
        system->flags = init->flags | system_init->flags;
        system->bank_path[0] = '\0';

        AIInit(0);
        ARQInit();
        AXInitEx(1);
        MIXInit();
        AXSetMode(2);
        AXRegisterCallback(MSL_GCN_AXUserCallback);

        mode = MIXGetSoundMode();
        switch (mode) {
        case 0:
            sound_mode = 0;
            break;
        case 2:
        case 3:
            sound_mode = 3;
            break;
        default:
            sound_mode = 1;
            break;
        }
        system->sound_mode = sound_mode;

        if (g_MSL_GCN_ARAM_Heap == 0) {
            unsigned char request[0x20];
            unsigned char zero_storage[0x420];
            void* zero_buffer = (void*)(
                ((unsigned long)zero_storage + 0x1F) & ~0x1FUL);
            int cache_size;
            int cache_total;
            unsigned long heap_base;
            unsigned long heap_size;

            g_MSL_GCN_ARAM_ZeroBase = system_init->aram_base;
            g_MSL_GCN_ARAM_ZeroBase_ADPCM_Start =
                g_MSL_GCN_ARAM_ZeroBase * 2 + 2;
            g_MSL_GCN_ARAM_ZeroBase_ADPCM_End =
                (g_MSL_GCN_ARAM_ZeroBase + 0x400) * 2 - 1;
            g_MSL_volatile_flag = 1;
            memset(zero_storage, 0, 0x420);
            ARQPostRequest(
                request, 0, 0, 1, (unsigned long)zero_buffer,
                g_MSL_GCN_ARAM_ZeroBase, 0x400,
                MSL_ClearVolatileFlag);

            heap_base = system_init->aram_base + 0x400;
            heap_size = system_init->aram_size - 0x400;
            mslStreamCache_Initialize_A(heap_base);
            cache_size = mslStreamCache_GetSizeBuffer();
            cache_total =
                mslStreamCache_GetNumBuffers() * cache_size;
            heap_base += cache_total;
            heap_size -= cache_total;

            ExternalHeap_SetSysMemRoutines(
                msl_SystemMalloc, msl_SystemFree);
            g_MSL_GCN_ARAM_Heap =
                ExternalHeap_Create(heap_base, heap_size, 5, 0xB0);
            if (g_MSL_GCN_ARAM_Heap == 0) {
                break;
            }
            InitCriticalCodeSection_DEBUG(
                &g_MSL_GCN_ARAM_CriticalSection,
                "mslgcn.cpp", 0x22E);
            ExternalHeap_SetMutex(
                g_MSL_GCN_ARAM_Heap,
                &g_MSL_GCN_ARAM_CriticalSection);
            ExternalHeap_SetSysMutexRoutines(
                msl_SystemEnterMutex, msl_SystemExitMutex);
            mslArqRequest_Init();
            while (g_MSL_volatile_flag != 0) {
            }
            mslStreamFile_Initialize();
        }

        system->tracks = (mslTrack*)_mwMemMalloc(
            MWSOUND_HEAP, 0x200, 3, 0, 0, 0);
        if (system->tracks == 0) {
            mslDebugPrintf(
                "mslInit: Out of memory allocating tracks.\n");
            break;
        }

        for (i = 0; i < 0x40; i++) {
            system->tracks[i].sound = 0;
        }

        system->track_count = init->track_count;
        for (i = 0; i < system->track_count; i++) {
            system->tracks[i].queue = mslQueueNew(0x20);
            if (system->tracks[i].queue == 0) {
                mslDebugPrintf(
                    "mslInit: Out of memory allocating track queues.\n");
                break;
            }
        }

        if (i == system->track_count) {
            system->volume = 1.0f;
            system->pan = 0.0f;
            system->pitch = 1.0f;
            system->duck_volume = 1.0f;
            system->duck_pan = 0.0f;
            system->duck_pitch = 1.0f;
            system->sound_list_guard = 1;
            system->active_sounds = 0;
            system->active_adjustments = 0;
            system->callbacks = 0;
            system->reservedE8 = 0;
            system->pending_bank_loads = 0;

            InitCriticalCodeSection_DEBUG(
                &system->critical_section,
                "mslgcn.cpp", 0x26E);
            mslDebugPrintf(
                "MSL after init heap %d\n", mslMainRamUsed());
            return system;
        }
    } while (0);

    mslInitCleanup(system);
    return 0;
}

extern "C" void MSL_ClearVolatileFlag(unsigned long request_address) {
    (void)request_address;
    g_MSL_volatile_flag = 0;
}

/*
 * Soft ceiling: mslTickCallBack_Queue ~99.91% -- the retail overflow path is
 * exact; only one diagnostic relocation argument differs.
 */
extern "C" void mslTickCallBack_Queue(
    void (*callback)(void*), void* callback_data) {
    unsigned long enabled = OSDisableInterrupts();

    if (g_mslTickCB_NumberItems < 20) {
        int tail = g_mslTickCB_Tail;

        g_mslTickCB_Queue[tail].callback = callback;
        g_mslTickCB_Queue[tail].data = callback_data;
        g_mslTickCB_NumberItems++;
        tail++;
        if (tail >= 20) {
            tail = 0;
        }
        g_mslTickCB_Tail = tail;
    } else {
        mslDebugPrintf("MSL DID SOMETHING BAD!!!\n");
        do {
        } while (g_bMSL_GCN_BREAK != 0);
        debugger_mbo1 = 0;
        debugger_at1 = 0;
        debugger_snd = 0;
        OSPanic("mslgcn.cpp", 0x66D, "UNSUPPORTED FUNCTION");
    }

    OSRestoreInterrupts(enabled);
}

static void msl_SystemExitMutex(void* section) {
    LeaveCriticalCodeSection_DEBUG(
        (MslCriticalSection*)section,
        "mslgcn.cpp", 0x109);
}

static void msl_SystemEnterMutex(void* section) {
    EnterCriticalCodeSection_DEBUG(
        (MslCriticalSection*)section,
        "mslgcn.cpp", 0x102);
}

static void msl_SystemFree(void* allocation) {
    _mwMemFree(allocation, 0, 0);
}

static void* msl_SystemMalloc(unsigned int size) {
    return _mwMemMalloc(
        MWSOUND_HEAP, size, 3, 0, 0, 0);
}

void MSL_GCN_AXUserCallback(void) {
    mslGCN_AXCallback_Ticks++;
    MIXUpdateSettings();
    SoundBuffer::SB_AXUserCallback();
}
