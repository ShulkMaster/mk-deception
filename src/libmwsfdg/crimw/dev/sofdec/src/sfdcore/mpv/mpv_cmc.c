#include "sofdec/mpv_mc.h"

static MPVMCFunction mpvcmc_oneref[8];

void MPVCMC_SetCcnt(MPVContext* object) {
    s32 count = 4;

    if (object->condition_state.conditions[5] == 0) {
        count = -1;
    }
    object->secondary_output_blocks.count = count;
    object->output_blocks.count = count;
}

void MPVCMC_InitMcOiRt(MPVContext* object) {
    MPVOutputBlocks* output_blocks = &object->output_blocks;
    MPVOutputBlock* output = output_blocks->blocks;
    s32 count;
    s32 first_stride;
    s32 remaining_stride;
    s32 i;

    if (object->condition_state.conditions[5] == 0) {
        count = -1;
    } else {
        count = 4;
    }
    output_blocks->count = count;
    first_stride = object->frame_buffers.backward.chroma_stride;
    for (i = 0; i < 2; i++) {
        output[i].stride = first_stride;
    }
    remaining_stride = object->frame_buffers.backward.luma_stride;
    for (i = 2; i < 6; i++) {
        output[i].stride = remaining_stride;
    }
}

void MPVCMC_InitObj(MPVContext* object) {
    u8* destination;
    MPVOutputBlocks* output_blocks;
    MPVOutputBlock* blocks;
    s32 count;
    s32 i;

    MPVMC08_Init(object->mc.functions08);
    MPVMC16_Init(&object->mc);
    output_blocks = &object->secondary_output_blocks;
    blocks = output_blocks->blocks;
    destination = object->field_D00;
    if (object->condition_state.conditions[5] == 0) {
        count = -1;
    } else {
        count = 4;
    }
    output_blocks->count = count;
    for (i = 0; i < 6; i++) {
        blocks[i].destination = destination;
    }
    for (i = 0; i < 6; i++) {
        blocks[i].stride = 8;
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
