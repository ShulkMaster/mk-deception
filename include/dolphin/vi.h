#ifndef DOLPHIN_VI_H
#define DOLPHIN_VI_H

#include "dolphin/gx.h"

#define VI_TVMODE(format, scan) (((format) << 2) + (scan))
#define VI_INTERLACE 0
#define VI_NON_INTERLACE 1
#define VI_PROGRESSIVE 2
#define VI_DEBUG 3
#define VI_DEBUG_PAL 4

typedef enum VITVMode {
    VI_TVMODE_NTSC_INT = VI_TVMODE(0, VI_INTERLACE),
    VI_TVMODE_NTSC_DS = VI_TVMODE(0, VI_NON_INTERLACE),
    VI_TVMODE_NTSC_PROG = VI_TVMODE(0, VI_PROGRESSIVE),
    VI_TVMODE_PAL_INT = VI_TVMODE(1, VI_INTERLACE),
    VI_TVMODE_PAL_DS = VI_TVMODE(1, VI_NON_INTERLACE),
    VI_TVMODE_EURGB60_INT = VI_TVMODE(5, VI_INTERLACE),
    VI_TVMODE_EURGB60_DS = VI_TVMODE(5, VI_NON_INTERLACE),
    VI_TVMODE_MPAL_INT = VI_TVMODE(2, VI_INTERLACE),
    VI_TVMODE_MPAL_DS = VI_TVMODE(2, VI_NON_INTERLACE),
    VI_TVMODE_DEBUG_INT = VI_TVMODE(3, VI_INTERLACE),
    VI_TVMODE_DEBUG_PAL_INT = VI_TVMODE(4, VI_INTERLACE),
    VI_TVMODE_DEBUG_PAL_DS = VI_TVMODE(4, VI_NON_INTERLACE)
} VITVMode;

typedef enum VIXFBMode { VI_XFBMODE_SF, VI_XFBMODE_DF } VIXFBMode;
typedef void (*VIRetraceCallback)(unsigned long retrace_count);

#ifdef __cplusplus
extern "C" {
#endif

void VISetBlack(int black);
void VIInit(void);
unsigned long VIGetTvFormat(void);
void VIConfigure(const GXRenderModeObj* mode);
void VIFlush(void);
void VIWaitForRetrace(void);
unsigned long VIGetDTVStatus(void);
unsigned long VIGetNextField(void);
void VISetNextFrameBuffer(void* framebuffer);
VIRetraceCallback VISetPreRetraceCallback(VIRetraceCallback callback);
VIRetraceCallback VISetPostRetraceCallback(VIRetraceCallback callback);
void VIConfigurePan(unsigned short x, unsigned short y,
                    unsigned short width, unsigned short height);
void VISetNextRightFrameBuffer(void* framebuffer);
void VISet3D(int enabled);
unsigned long VIGetRetraceCount(void);
unsigned long VIGetCurrentLine(void);
void* VIGetNextFrameBuffer(void);
void* VIGetCurrentFrameBuffer(void);
unsigned long VIGetScanMode(void);
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
