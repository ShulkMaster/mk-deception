#ifndef MSL_MSLGCN_H
#define MSL_MSLGCN_H

#include "msl/mslgcn_break.h"

#ifdef __cplusplus
void MSL_GCN_AXUserCallback(void);
extern "C" {
#else
void MSL_GCN_AXUserCallback(void);
#endif

unsigned long mslMainRamUsed(void);
void MSL_ClearVolatileFlag(unsigned long request_address);

extern int g_bMSL_GCN_BREAK;
extern unsigned long g_MSL_volatile_flag;
extern unsigned long mslGCN_AXCallback_Ticks;
extern int debugger_mbo1;
extern int debugger_at1;
extern int debugger_snd;

#ifdef __cplusplus
}
#endif

#endif
