#include "cri/sj.h"

typedef struct SfdHandle SfdHandle;
typedef struct MwsStHandle MwsStHandle;

typedef struct MwsStManagerInterface {
    void* reserved_00;
    void (*finish)(void);
    void* reserved_08[2];
    void (*destroy)(MwsStHandle* handle);
    void (*start_sj)(void* backend, SJ* stream);
    void (*stop)(void* backend);
    int (*get_status)(void* backend);
    void* reserved_20;
    void (*pause)(void* backend, int paused);
    void (*set_volume)(void* backend, int volume);
    int (*get_volume)(void* backend);
} MwsStManagerInterface;

struct MwsStHandle {
    int active;
    unsigned char reserved_04[8];
    SJ* stream;
    int element_id;
    void* backend;
};

typedef struct MwsStPlayerPrefix {
    unsigned char reserved_000[0x40];
    SfdHandle* player;
    unsigned char reserved_044[0x250];
    MwsStHandle sound;
} MwsStPlayerPrefix;

typedef struct MwsStManager {
    MwsStManagerInterface* interface;
    int active_count;
} MwsStManager;

extern void MWSFSVM_GotoIdleBorder(void);
extern int SFD_SetElementOutSj(SfdHandle* handle, int element_id, SJ* stream,
                               int channel, int flags);

MwsStManager mwsstmng = {0, 0};

static inline int mwsst_IsValid(const MwsStHandle* handle)
{
    if (mwsstmng.interface == 0) {
        return 0;
    }
    if (handle->active != 1) {
        return 0;
    }
    if (handle->backend == 0) {
        return 0;
    }
    return 1;
}

static inline int mwsst_IsValidWithBackend(const MwsStHandle* handle,
                                           const MwsStHandle* backend)
{
    if (mwsstmng.interface == 0) {
        return 0;
    }
    if (handle->active != 1) {
        return 0;
    }
    if (backend == 0) {
        return 0;
    }
    return 1;
}

static void mwsst_Stop(MwsStHandle* handle)
{
    if (mwsst_IsValid(handle) == 1) {
        void* backend = handle->backend;
        if (mwsstmng.interface != 0 && mwsstmng.interface->stop != 0) {
            mwsstmng.interface->stop(backend);
        }
    }
}

static void mwsst_DestroyHn(MwsStHandle* handle)
{
    MwsStManagerInterface* interface = mwsstmng.interface;

    if (handle != 0 && interface != 0 && interface->destroy != 0) {
        interface->destroy(handle);
    }
}

static void mwsst_ReleaseLib(void)
{
    MwsStManagerInterface* interface = mwsstmng.interface;

    if (interface != 0 && mwsstmng.active_count != 0) {
        mwsstmng.active_count--;
        if (mwsstmng.active_count == 0 && interface->finish != 0) {
            interface->finish();
        }
    }
}

void MWSST_Destroy(MwsStHandle* handle)
{
    MwsStHandle* sound;
    SJ* stream;

    if (mwsst_IsValid(handle) == 1) {
        sound = handle->backend;
        stream = handle->stream;
        if (sound != 0) {
            MWSFSVM_GotoIdleBorder();
            mwsst_Stop(sound);
            handle->active = 0;
            mwsst_DestroyHn(sound);
            stream->interface->destroy(stream);
            handle->backend = 0;
            mwsst_ReleaseLib();
        }
    }
}

void MWSST_Reset(MwsStPlayerPrefix* wrapper)
{
    MwsStHandle* sound = &wrapper->sound;
    SfdHandle* player = wrapper->player;
    MwsStHandle* backend = sound->backend;
    SJ* stream = sound->stream;
    int element_id = sound->element_id;

    if (mwsst_IsValidWithBackend(sound, backend) != 1) {
        return;
    }
    if (backend != 0 && mwsst_IsValid(backend) == 1) {
        void* playback = backend->backend;
        if (mwsstmng.interface != 0 && mwsstmng.interface->stop != 0) {
            mwsstmng.interface->stop(playback);
        }
    }
    stream->interface->reset(stream);
    SFD_SetElementOutSj(player, element_id + 0xC0, stream, 0, 0);
}

int MWSST_GetOutVol(MwsStHandle* handle)
{
    int volume = 0;
    void* backend;

    if (mwsst_IsValid(handle) != 1) {
        return 0;
    }
    backend = handle->backend;
    if (mwsstmng.interface != 0 && mwsstmng.interface->get_volume != 0) {
        volume = mwsstmng.interface->get_volume(backend);
    }
    return volume;
}

void MWSST_SetOutVol(MwsStHandle* handle, int volume)
{
    if (mwsst_IsValid(handle) == 1) {
        void* backend = handle->backend;
        if (mwsstmng.interface != 0 && mwsstmng.interface->set_volume != 0) {
            mwsstmng.interface->set_volume(backend, volume);
        }
    }
}

void MWSST_Pause(MwsStHandle* handle, int paused)
{
    if (mwsst_IsValid(handle) == 1) {
        void* backend = handle->backend;
        if (mwsstmng.interface != 0 && mwsstmng.interface->pause != 0) {
            mwsstmng.interface->pause(backend, paused);
        }
    }
}

int MWSST_GetStat(MwsStHandle* handle)
{
    int status = 0;
    void* backend;

    if (mwsst_IsValid(handle) != 1) {
        return 0;
    }
    backend = handle->backend;
    if (mwsstmng.interface != 0 && mwsstmng.interface->get_status != 0) {
        status = mwsstmng.interface->get_status(backend);
    }
    return status;
}

void MWSST_Stop(MwsStHandle* handle)
{
    if (mwsst_IsValid(handle) == 1) {
        void* backend = handle->backend;
        if (mwsstmng.interface != 0 && mwsstmng.interface->stop != 0) {
            mwsstmng.interface->stop(backend);
        }
    }
}

void MWSST_StartSj(MwsStHandle* handle)
{
    if (mwsst_IsValid(handle) == 1) {
        void* backend = handle->backend;
        SJ* stream = handle->stream;
        if (mwsstmng.interface != 0 && mwsstmng.interface->start_sj != 0) {
            mwsstmng.interface->start_sj(backend, stream);
        }
    }
}
