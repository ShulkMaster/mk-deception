#ifndef MKD_SOFDEC_MPV_MC_H
#define MKD_SOFDEC_MPV_MC_H

#include "dolphin/types.h"

typedef struct MPVMCContext MPVMCContext;
typedef void (*MPVMCFunction)(MPVMCContext* context);

struct MPVMCContext {
    MPVMCFunction functions08[4];
    u8 field_10[8];
    u8* destination;
    u32 field_1c;
    s32 reference_stride;
    const u8* reference0;
    const u8* reference1;
    u8 field_2c[8];
    MPVMCFunction functions16[4];
};

void MPVMC08_OneRef1p_TuneC(MPVMCContext* context);
void MPVMC08_OneRefH2_TuneC(MPVMCContext* context);
void MPVMC08_OneRefV2_TuneC(MPVMCContext* context);
void MPVMC08_OneRef4p_TuneC(MPVMCContext* context);
void MPVMC08_Init(MPVMCFunction functions[4]);

void MPVMC16_OneRef1p_TuneC(MPVMCContext* context);
void MPVMC16_OneRefH2_TuneC(MPVMCContext* context);
void MPVMC16_OneRefV2_TuneC(MPVMCContext* context);
void MPVMC16_OneRef4p_TuneC(MPVMCContext* context);
void MPVMC16_Init(MPVMCContext* context);

#endif
