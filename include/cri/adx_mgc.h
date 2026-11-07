#ifndef CRI_ADX_MGC_H
#define CRI_ADX_MGC_H

typedef struct ADXMThreadParams {
    int lock_priority;
    int safe_priority;
    int vsync_priority;
    int fs_priority;
    int field_10;
    int mwidle_priority;
} ADXMThreadParams;

#ifdef __cplusplus
extern "C" {
#endif

void ADXM_SetupThrd(ADXMThreadParams* params);
void ADXM_SetCbErr(void (*callback)(void*, char*), void* object);
void ADXM_ShutdownThrd(void);
void ADXM_ExecMain(void);
extern int ADXM_IsSetupThrd(void);
extern void ADXM_WaitVsync(void);

#ifdef __cplusplus
}
#endif

#endif
