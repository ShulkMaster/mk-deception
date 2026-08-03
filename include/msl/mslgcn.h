#ifndef MSL_MSLGCN_H
#define MSL_MSLGCN_H

#ifdef __cplusplus
extern "C" {
#endif

int mslTick(void);
unsigned long mslMainRamUsed(void);
void mslCreateLogTable(void);
void _MSL_GCN_BREAK(void);
void MSL_GCN_AXUserCallback(void);
void MSL_ClearVolatileFlag(unsigned long request_address);

extern int g_bMSL_GCN_BREAK;
extern int g_MSL_volatile_flag;
extern unsigned long mslGCN_AXCallback_Ticks;
extern int debugger_mbo1;
extern int debugger_at1;
extern int debugger_snd;

#ifdef __cplusplus
}
#endif

#endif
