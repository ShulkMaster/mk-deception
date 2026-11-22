#ifndef MSL_MSLGCN_H
#define MSL_MSLGCN_H

#include "dolphin/types.h"
#include "msl/mslgcn_break.h"

#ifdef __cplusplus
void MSL_GCN_AXUserCallback(void);
extern "C" {
#else
void MSL_GCN_AXUserCallback(void);
#endif

u32 mslMainRamUsed(void);
void MSL_ClearVolatileFlag(u32 request_address);

extern volatile int g_bMSL_GCN_BREAK;
extern volatile u32 g_MSL_volatile_flag;
extern u32 mslGCN_AXCallback_Ticks;
extern int debugger_mbo1;
extern int debugger_at1;
extern int debugger_snd;

#ifdef __cplusplus
}
#endif

#endif
