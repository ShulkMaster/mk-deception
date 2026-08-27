#ifndef MSL_MSLGCN_H
#define MSL_MSLGCN_H

#ifdef __cplusplus
void _MSL_GCN_BREAK(void);
void MSL_GCN_AXUserCallback(void);
extern "C" {
#else
void _MSL_GCN_BREAK(void);
void MSL_GCN_AXUserCallback(void);
#endif

int mslTick(void);
unsigned long mslMainRamUsed(void);
void mslCreateLogTable(void);
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
