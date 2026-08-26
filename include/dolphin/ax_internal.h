#ifndef DOLPHIN_AX_INTERNAL_H
#define DOLPHIN_AX_INTERNAL_H

#include "dolphin/ax.h"

#ifdef __cplusplus
extern "C" {
#endif

extern u16 __AXCompressorTable[3360];
extern u16 axDspSlave[];
extern u16 axDspSlaveLength;
extern u32 __AXClMode;
extern AXPROFILE __AXLocalProfile;

void __AXAllocInit(void);
AXVPB* __AXGetStackHead(u32 priority);
void __AXServiceCallbackStack(void);
void __AXPushFreeStack(AXVPB* voice);
void __AXPushCallbackStack(AXVPB* voice);
AXVPB* __AXPopCallbackStack(void);
void __AXRemoveFromStack(AXVPB* voice);

void __AXAuxInit(void);
void __AXGetAuxAInput(u32* address);
void __AXGetAuxAInputDpl2(u32* address);
void __AXGetAuxAOutput(u32* address);
void __AXGetAuxAOutputDpl2R(u32* address);
void __AXGetAuxAOutputDpl2Ls(u32* address);
void __AXGetAuxAOutputDpl2Rs(u32* address);
void __AXGetAuxBInput(u32* address);
void __AXGetAuxBOutput(u32* address);
void __AXGetAuxBForDPL2(u32* address);
void __AXGetAuxBOutputDPL2(u32* address);
void __AXProcessAux(void);

u32 __AXGetCommandListCycles(void);
u32 __AXGetCommandListAddress(void);
void __AXNextFrame(void* sbuffer, void* buffer);
void __AXClInit(void);

void __AXOutNewFrame(u32 less_dsp_cycles);
void __AXOutAiCallback(void);
void __AXOutInitDSP(void);
void __AXOutInit(u32 output_buffer_mode);

AXPROFILE* __AXGetCurrentProfile(void);

u32 __AXGetStudio(void);
void __AXPrintStudio(void);
void __AXSPBInit(void);
void __AXDepopVoice(AXPB* pb);

u32 __AXGetNumVoices(void);
void __AXServiceVPB(AXVPB* voice);
void __AXSyncPBs(u32 less_dsp_cycles);
AXPB* __AXGetPBs(void);
void __AXSetPBDefault(AXVPB* voice);
void __AXVPBInit(void);

#ifdef __cplusplus
}
#endif

#endif
