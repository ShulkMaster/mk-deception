#ifndef CRI_ADX_TLK_H
#define CRI_ADX_TLK_H

#include "cri/adxt_internal.h"

#ifdef __cplusplus
extern "C" {
#endif

void ADXT_StartSj(ADXTHandle* handle, SJ* input);
extern s32 ADXT_GetNumChan(ADXTHandle* handle);
extern s32 ADXT_GetStat(ADXTHandle* handle);
extern void ADXT_Stop(ADXTHandle* handle);
extern void ADXT_Destroy(ADXTHandle* decoder);
extern void ADXT_TermSupply(ADXTHandle* decoder);
extern void ADXT_GetTime(ADXTHandle* decoder, s32* value, s32* scale);
extern void ADXT_ExecServer(void);
extern void ADXT_DestroyAll(void);
extern void ADXT_SetLnkSw(ADXTHandle* handle, s32 enabled);

#ifdef __cplusplus
}
#endif

#endif
