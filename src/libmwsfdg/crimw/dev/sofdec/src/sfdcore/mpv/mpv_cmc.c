#include "sofdec/mpv_mc.h"

typedef struct MPVCMCOutputBlock {
    u8* destination;
    s32 stride;
} MPVCMCOutputBlock;

typedef struct MPVCMCObject {
    u8 field_000[0xCC];
    MPVMCContext mc;
    u8 field_110[0x10];
    s32 ccnt_120;
    MPVCMCOutputBlock output_124[6];
    s32 ccnt_154;
    MPVCMCOutputBlock output_158[6];
    u8 field_188[0x1C];
    s32 field_1A4;
    u8 field_1A8[0xD8];
    s16 field_280;
    s16 field_282;
    u8 field_284[0xA7C];
    u8 field_D00[1];
} MPVCMCObject;

static MPVMCFunction mpvcmc_oneref[8];

void MPVCMC_SetCcnt(MPVCMCObject* object) {
    s32 count = 4;

    if (object->field_1A4 == 0) {
        count = -1;
    }
    object->ccnt_154 = count;
    object->ccnt_120 = count;
}

void MPVCMC_InitMcOiRt(MPVCMCObject* object) {
    MPVCMCOutputBlock* output = object->output_124;
    s32 count = 4;
    s32 first_stride;
    s32 remaining_stride;
    s32 i;

    if (object->field_1A4 == 0) {
        count = -1;
    }
    object->ccnt_120 = count;
    first_stride = object->field_280;
    for (i = 0; i < 2; i++) {
        output[i].stride = first_stride;
    }
    remaining_stride = object->field_282;
    for (i = 2; i < 6; i++) {
        output[i].stride = remaining_stride;
    }
}

void MPVCMC_InitObj(MPVCMCObject* object) {
    MPVCMCOutputBlock* output;
    u8* destination;
    s32 count;
    s32 i;

    MPVMC08_Init(object->mc.functions08);
    MPVMC16_Init(&object->mc);
    output = object->output_158;
    destination = object->field_D00;
    count = 4;
    if (object->field_1A4 == 0) {
        count = -1;
    }
    object->ccnt_154 = count;
    for (i = 0; i < 6; i++) {
        output[i].destination = destination;
    }
    for (i = 0; i < 6; i++) {
        output[i].stride = 8;
    }
}

void MPVCMC_Init(void) {
    mpvcmc_oneref[0] = MPVMC08_OneRef1p_TuneC;
    mpvcmc_oneref[1] = MPVMC08_OneRefH2_TuneC;
    mpvcmc_oneref[2] = MPVMC08_OneRefV2_TuneC;
    mpvcmc_oneref[3] = MPVMC08_OneRef4p_TuneC;
    mpvcmc_oneref[4] = MPVMC08_OneRef1p_TuneC;
    mpvcmc_oneref[5] = MPVMC08_OneRefH2_TuneC;
    mpvcmc_oneref[6] = MPVMC08_OneRefV2_TuneC;
    mpvcmc_oneref[7] = MPVMC08_OneRefV2_TuneC;
}
