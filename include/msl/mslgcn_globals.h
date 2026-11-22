#ifndef MSL_MSLGCN_GLOBALS_H
#define MSL_MSLGCN_GLOBALS_H

#include "dolphin/types.h"

struct ExternalHeap;

extern int SoundBufferCount;
extern int SoundBufferCountStream;
extern int SoundBufferCountStatic;

extern struct ExternalHeap* g_MSL_GCN_ARAM_Heap;
extern u32 g_MSL_GCN_ARAM_ZeroBase;
extern u32 g_MSL_GCN_ARAM_ZeroBase_ADPCM_Start;
extern u32 g_MSL_GCN_ARAM_ZeroBase_ADPCM_End;

#endif
