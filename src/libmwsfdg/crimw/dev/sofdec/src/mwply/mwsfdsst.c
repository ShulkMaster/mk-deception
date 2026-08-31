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

void MWSST_Destroy(MwsStHandle* handle)
{
    MwsStHandle* sound;
    SJ* stream;

    if (mwsst_IsValid(handle) != 1) {
        return;
    }
    sound = handle->backend;
    stream = handle->stream;
    if (sound == 0) {
        return;
    }

    MWSFSVM_GotoIdleBorder();
    if (mwsst_IsValid(sound) == 1 && mwsstmng.interface != 0 &&
        mwsstmng.interface->stop != 0) {
        mwsstmng.interface->stop(sound->backend);
    }
    handle->active = 0;
    if (sound != 0 && mwsstmng.interface != 0 &&
        mwsstmng.interface->destroy != 0) {
        mwsstmng.interface->destroy(sound);
    }
    stream->interface->destroy(stream);
    handle->backend = 0;
    if (mwsstmng.interface != 0 && mwsstmng.active_count != 0) {
        mwsstmng.active_count--;
        if (mwsstmng.active_count == 0 &&
            mwsstmng.interface->finish != 0) {
            mwsstmng.interface->finish();
        }
    }
}

void MWSST_Reset(MwsStPlayerPrefix* wrapper)
{
    MwsStHandle* sound = &wrapper->sound;
    SfdHandle* player = wrapper->player;
    SJ* stream = sound->stream;
    int element_id = sound->element_id;
    MwsStHandle* backend = sound->backend;

    if (mwsst_IsValid(sound) != 1) {
        return;
    }
    if (backend != 0 && mwsst_IsValid(backend) == 1 &&
        mwsstmng.interface != 0 && mwsstmng.interface->stop != 0) {
        mwsstmng.interface->stop(backend->backend);
    }
    stream->interface->reset(stream);
    SFD_SetElementOutSj(player, element_id + 0xC0, stream, 0, 0);
}

int MWSST_GetOutVol(MwsStHandle* handle)
{
    int volume = 0;

    if (mwsst_IsValid(handle) != 1) {
        return 0;
    }
    if (mwsstmng.interface != 0 && mwsstmng.interface->get_volume != 0) {
        volume = mwsstmng.interface->get_volume(handle->backend);
    }
    return volume;
}

void MWSST_SetOutVol(MwsStHandle* handle, int volume)
{
    if (mwsst_IsValid(handle) == 1 && mwsstmng.interface != 0 &&
        mwsstmng.interface->set_volume != 0) {
        mwsstmng.interface->set_volume(handle->backend, volume);
    }
}

void MWSST_Pause(MwsStHandle* handle, int paused)
{
    if (mwsst_IsValid(handle) == 1 && mwsstmng.interface != 0 &&
        mwsstmng.interface->pause != 0) {
        mwsstmng.interface->pause(handle->backend, paused);
    }
}

int MWSST_GetStat(MwsStHandle* handle)
{
    int status = 0;

    if (mwsst_IsValid(handle) != 1) {
        return 0;
    }
    if (mwsstmng.interface != 0 && mwsstmng.interface->get_status != 0) {
        status = mwsstmng.interface->get_status(handle->backend);
    }
    return status;
}

void MWSST_Stop(MwsStHandle* handle)
{
    if (mwsst_IsValid(handle) == 1 && mwsstmng.interface != 0 &&
        mwsstmng.interface->stop != 0) {
        mwsstmng.interface->stop(handle->backend);
    }
}

void MWSST_StartSj(MwsStHandle* handle)
{
    if (mwsst_IsValid(handle) == 1 && mwsstmng.interface != 0 &&
        mwsstmng.interface->start_sj != 0) {
        mwsstmng.interface->start_sj(handle->backend, handle->stream);
    }
}
