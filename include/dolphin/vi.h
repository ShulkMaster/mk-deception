#ifndef DOLPHIN_VI_H
#define DOLPHIN_VI_H

#include "dolphin/gx.h"

#ifdef __cplusplus
extern "C" {
#endif

void VISetBlack(int black);
void VIInit(void);
int VIGetTvFormat(void);
void VIConfigure(GXRenderModeObj* mode);
void VIFlush(void);
void VIWaitForRetrace(void);
int VIGetDTVStatus(void);
int VIGetNextField(void);
void VISetNextFrameBuffer(void* framebuffer);
extern GXRenderModeObj GXNtsc480IntDf;

#ifdef __cplusplus
}
#endif

#endif
