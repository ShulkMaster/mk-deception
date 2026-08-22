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
unsigned int VIGetDTVStatus(void);
int VIGetNextField(void);
void VISetNextFrameBuffer(void* framebuffer);
extern GXRenderModeObj GXNtsc480IntDf;
extern GXRenderModeObj GXPal528IntDf;
extern GXRenderModeObj GXMpal480IntDf;
extern GXRenderModeObj GXEurgb60Hz480IntDf;

#define VI_NTSC 0
#define VI_PAL 1
#define VI_MPAL 2
#define VI_EURGB60 5

#ifdef __cplusplus
}
#endif

#endif
