#ifndef MKD_SOFDEC_MWS_SOUND_H
#define MKD_SOFDEC_MWS_SOUND_H

#include "cri/sj.h"

typedef struct MwsStHandle {
    int active;
    unsigned char reserved_004[8];
    SJ* stream;
    int element_id;
    void* backend;
} MwsStHandle;

typedef char MwsStHandleSizeCheck[sizeof(MwsStHandle) == 0x18 ? 1 : -1];

void MWSST_Destroy(MwsStHandle* handle);
void MWSST_Reset(void* object);
void MWSST_Pause(MwsStHandle* handle, int paused);
void MWSST_StartSj(MwsStHandle* handle);
void MWSST_Stop(MwsStHandle* handle);
int MWSST_GetStat(MwsStHandle* handle);
int MWSST_GetOutVol(MwsStHandle* handle);
void MWSST_SetOutVol(MwsStHandle* handle, int volume);

#endif
