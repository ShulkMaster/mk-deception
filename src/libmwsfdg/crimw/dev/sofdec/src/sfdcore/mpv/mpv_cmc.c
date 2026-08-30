#include "sofdec/mpv_mc.h"

static MPVMCFunction mpvcmc_oneref[8];

void MPVCMC_SetCcnt(MPVContext* object) {
    s32 count = 4;

    if (object->condition_state.decoder.field_1A4 == 0) {
        count = -1;
    }
    object->secondary_output_blocks.count = count;
    object->output_blocks.count = count;
}

void MPVCMC_InitMcOiRt(MPVContext* object) {
    MPVOutputBlock* output = object->output_blocks.blocks;
    s32 count = 4;
    s32 first_stride;
    s32 remaining_stride;
    s32 i;

    if (object->condition_state.decoder.field_1A4 == 0) {
        count = -1;
    }
    object->output_blocks.count = count;
    first_stride = object->frame_buffers.backward.chroma_stride;
    for (i = 0; i < 2; i++) {
        output[i].stride = first_stride;
    }
    remaining_stride = object->frame_buffers.backward.luma_stride;
    for (i = 2; i < 6; i++) {
        output[i].stride = remaining_stride;
    }
}

/* Soft ceiling: 92.06% - retail addresses the six secondary output blocks
 * directly from the outer context; clean loops make MWCC retain a +0x158 base.
 * Permutation improved only by adding a redundant pointer-copy lifetime. */
void MPVCMC_InitObj(MPVContext* object) {
    u8* destination;
    s32 count;
    s32 i;

    MPVMC08_Init(object->mc.functions08);
    MPVMC16_Init(&object->mc);
    destination = object->field_D00;
    count = 4;
    if (object->condition_state.decoder.field_1A4 == 0) {
        count = -1;
    }
    object->secondary_output_blocks.count = count;
    for (i = 0; i < 6; i++) {
        object->secondary_output_blocks.blocks[i].destination = destination;
    }
    for (i = 0; i < 6; i++) {
        object->secondary_output_blocks.blocks[i].stride = 8;
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
