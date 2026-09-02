#include "sofdec/mpv_mc.h"

static MPVMCFunction mpvumc_oneref_y[8];
static MPVMCFunction mpvumc_oneref[8];

void MPVUMC_BpicSkipped(MPVContext* context, s32 count)
{
    s32 end = context->macroblock_index;
    s32 amount = count - 1;
    void (*decode_macroblock)(MPVContext*) = context->motion_skipped;
    s32 zero = 0;

    context->cbp_mask = zero;
    context->macroblock_index -= amount;
    context->macroblock_column -= amount;
    while (context->macroblock_column < 0) {
        context->macroblock_column +=
            context->condition_state.decoder.picture.macroblocks_per_row;
        context->macroblock_row--;
    }
    while (context->macroblock_index < end) {
        decode_macroblock(context);
        context->macroblock_column++;
        if (context->macroblock_column >=
            context->condition_state.decoder.picture.macroblocks_per_row) {
            context->macroblock_column = zero;
            context->macroblock_row++;
        }
        context->macroblock_index++;
    }
}

static void mpvumc_PpicSkipMb(const MPVBlockOffsets* offsets,
                              const MPVPlaneSet* source,
                              MPVPlaneSet* destination)
{
    s32 plane;
    s32 row;

    for (plane = 0; plane < 2; plane++) {
        const u8* src = source->planes[plane] + offsets->chroma;
        u8* dst = destination->planes[plane] + offsets->chroma;
        for (row = 0; row < 8; row++) {
            *(u64*)dst = *(const u64*)src;
            src += destination->chroma_stride;
            dst += destination->chroma_stride;
        }
    }

    {
        const u8* src = source->planes[2] + offsets->luma;
        u8* dst = destination->planes[2] + offsets->luma;
        for (row = 0; row < 16; row++) {
            ((u64*)dst)[0] = ((const u64*)src)[0];
            ((u64*)dst)[1] = ((const u64*)src)[1];
            src += destination->luma_stride;
            dst += destination->luma_stride;
        }
    }
}

void MPVUMC_PpicSkipped(MPVContext* context, s32 count)
{
    s32 end = context->macroblock_index;
    s32 amount = count - 1;

    context->macroblock_index -= amount;
    context->macroblock_column -= amount;
    while (context->macroblock_column < 0) {
        context->macroblock_column +=
            context->condition_state.decoder.picture.macroblocks_per_row;
        context->macroblock_row--;
    }
    while (context->macroblock_index < end) {
        MPVBlockOffsets offsets;
        offsets.chroma = context->macroblock_column * 8 +
                         context->macroblock_row * 8 *
                             context->frame_buffers.forward.chroma_stride;
        offsets.luma = context->macroblock_column * 16 +
                       context->macroblock_row * 16 *
                           context->frame_buffers.forward.luma_stride;
        mpvumc_PpicSkipMb(&offsets, &context->frame_buffers.forward,
                          &context->frame_buffers.backward);
        context->macroblock_column++;
        if (context->macroblock_column >=
            context->condition_state.decoder.picture.macroblocks_per_row) {
            context->macroblock_column = 0;
            context->macroblock_row++;
        }
        context->macroblock_index++;
    }
}

static void mpvumc_BiMakeMb(MPVMacroblockSources* sources,
                            MPVOutputBlocks* output, s32 cbp_mask)
{
    s16* residual = sources->residual;
    u8* prediction0 = sources->prediction0;
    u8* prediction1 = sources->prediction1;
    s32 block;
    s32 row;
    s32 column;

    for (block = 0; block < 6; block++, cbp_mask <<= 1) {
        u8* destination = output->blocks[block].destination;
        s32 stride = output->blocks[block].stride;
        for (row = 0; row < 8; row++) {
            for (column = 0; column < 8; column++) {
                s32 value =
                    (prediction0[row * 8 + column] +
                     prediction1[row * 8 + column] + 1) /
                    2;
                if (cbp_mask < 0) {
                    value += residual[row * 8 + column];
                }
                if (value < 0) {
                    value = 0;
                } else if (value > 255) {
                    value = 255;
                }
                destination[column] = (u8)value;
            }
            destination += stride;
        }
        residual += 64;
        prediction0 += 64;
        prediction1 += 64;
    }
}

static void mpvumc_OneMakeMb(MPVMacroblockSources* sources,
                             MPVOutputBlocks* output, s32 cbp_mask)
{
    s16* residual = sources->residual;
    u8* prediction = sources->prediction0;
    s32 block;
    s32 row;
    s32 column;

    for (block = 0; block < 6; block++, cbp_mask <<= 1) {
        u8* destination = output->blocks[block].destination;
        s32 stride = output->blocks[block].stride;
        for (row = 0; row < 8; row++) {
            for (column = 0; column < 8; column++) {
                s32 value = prediction[row * 8 + column];
                if (cbp_mask < 0) {
                    value += residual[row * 8 + column];
                }
                if (value < 0) {
                    value = 0;
                } else if (value > 255) {
                    value = 255;
                }
                destination[column] = (u8)value;
            }
            destination += stride;
        }
        residual += 64;
        prediction += 64;
    }
}

static void mpvumc_OneReadMb(MPVContext* context, u8* destination,
                             MPVBlockOffsets* offsets,
                             const MPVPlaneSet* planes,
                             const MPVMotionInfo* motion)
{
    MPVMCContext* mc = &context->mc;
    s32 horizontal = motion->horizontal;
    s32 vertical = motion->vertical;
    s32 chroma_stride = planes->chroma_stride;
    s32 luma_stride = planes->luma_stride;
    s32 chroma_horizontal = horizontal / 2;
    s32 chroma_vertical = vertical / 2;
    s32 luma_offset;
    s32 chroma_offset;
    s32 luma_extra;
    s32 chroma_extra;
    const u8* reference;
    MPVMCFunction luma_function;
    MPVMCFunction chroma_function;

    offsets->chroma = context->macroblock_column * 8 +
                      context->macroblock_row * 8 * chroma_stride;
    offsets->luma = context->macroblock_column * 16 +
                    context->macroblock_row * 16 * luma_stride;
    luma_function = mpvumc_oneref_y[context->condition_state.decoder.mc_table * 4 +
                                    (((u32)vertical << 1) & 2) +
                                    (horizontal & 1)];
    chroma_function =
        mpvumc_oneref[context->condition_state.decoder.mc_table * 4 +
                      (((u32)chroma_vertical << 1) & 2) +
                      (chroma_horizontal & 1)];
    luma_offset = offsets->luma + (horizontal >> 1) +
                  (vertical >> 1) * luma_stride;
    chroma_offset = offsets->chroma + (chroma_horizontal >> 1) +
                    (chroma_vertical >> 1) * chroma_stride;
    luma_extra = (horizontal & 1) & context->condition_state.decoder.mc_table;
    chroma_extra = (chroma_horizontal & 1) & context->condition_state.decoder.mc_table;

    mc->reference_stride = chroma_stride;
    mc->destination = destination;
    reference = planes->planes[0] + chroma_offset;
    mc->reference0 = reference;
    mc->reference1 = reference + chroma_stride + chroma_extra;
    chroma_function(mc);
    mc->destination = destination + 0x40;
    reference = planes->planes[1] + chroma_offset;
    mc->reference0 = reference;
    mc->reference1 = reference + chroma_stride + chroma_extra;
    chroma_function(mc);
    mc->reference_stride = luma_stride;
    mc->destination = destination + 0x80;
    reference = planes->planes[2] + luma_offset;
    mc->reference0 = reference;
    mc->reference1 = reference + luma_stride + luma_extra;
    luma_function(mc);
}

static inline void mpvumc_SetOutputBlocks(MPVContext* context,
                                          const MPVBlockOffsets* offsets)
{
    MPVOutputBlock* block = context->output_blocks.blocks;
    block[0].destination = context->output.chroma0 + offsets->chroma;
    block[1].destination = context->output.chroma1 + offsets->chroma;
    block[2].destination = context->output.luma + offsets->luma;
    block[3].destination = block[2].destination + 8;
    block[4].destination =
        block[2].destination + context->output.luma_stride * 8;
    block[5].destination = block[4].destination + 8;
}

void MPVUMC_BiDirect(MPVContext* context)
{
    MPVBlockOffsets offsets;
    mpvumc_OneReadMb(context, context->sources.prediction0, &offsets,
                     &context->frame_buffers.forward, &context->forward_motion);
    mpvumc_OneReadMb(context, context->sources.prediction1, &offsets,
                     &context->frame_buffers.backward, &context->backward_motion);
    mpvumc_SetOutputBlocks(context, &offsets);
    mpvumc_BiMakeMb(&context->sources, &context->output_blocks,
                    context->cbp_mask);
}

void MPVUMC_Backward(MPVContext* context)
{
    MPVBlockOffsets offsets;
    mpvumc_OneReadMb(context, context->sources.prediction0, &offsets,
                     &context->frame_buffers.backward, &context->backward_motion);
    mpvumc_SetOutputBlocks(context, &offsets);
    mpvumc_OneMakeMb(&context->sources, &context->output_blocks,
                     context->cbp_mask);
}

void MPVUMC_Forward(MPVContext* context)
{
    MPVBlockOffsets offsets;
    mpvumc_OneReadMb(context, context->sources.prediction0, &offsets,
                     &context->frame_buffers.forward, &context->forward_motion);
    mpvumc_SetOutputBlocks(context, &offsets);
    mpvumc_OneMakeMb(&context->sources, &context->output_blocks,
                     context->cbp_mask);
}

static void mpvumc_OutputIntra6blk(const DctFsriBlock blocks[6],
                                   MPVOutputBlocks* output)
{
    s32 block;
    s32 row;
    s32 column;

    for (block = 0; block < 6; block++) {
        u8* destination = output->blocks[block].destination;
        s32 stride = output->blocks[block].stride;
        for (row = 0; row < 8; row++) {
            for (column = 0; column < 8; column++) {
                int sample = blocks[block].samples[row * 8 + column];
                if (sample < 0) {
                    sample = 0;
                } else if (sample > 255) {
                    sample = 255;
                }
                destination[column] = (u8)sample;
            }
            destination += stride;
        }
    }
}

void MPVUMC_Intra(MPVContext* context)
{
    MPVBlockOffsets offsets;
    offsets.chroma = context->macroblock_column * 8 +
                     context->macroblock_row * 8 * context->output.chroma_stride;
    offsets.luma = context->macroblock_column * 16 +
                   context->macroblock_row * 16 * context->output.luma_stride;
    mpvumc_SetOutputBlocks(context, &offsets);
    mpvumc_OutputIntra6blk(context->transform.blocks,
                           &context->output_blocks);
}

void MPVUMC_SetGqr(void)
{
    /* Retail programs GQR3, GQR4, and GQR5 for the paired-single kernels. */
}

void MPVUMC_EndOfFrame(void) {}

void MPVUMC_InitOutRfb(MPVContext* context)
{
    s32 width_plus = context->condition_state.decoder.picture.width + 15;
    s32 height_plus = context->condition_state.decoder.picture.height + 15;
    u8* output_rfb = context->frame_buffers.output_rfb;
    s32 rounded_width = (width_plus / 16) * 16;
    s32 luma_stride = ((rounded_width + 31) / 32) << 5;
    s32 chroma_stride = (((rounded_width / 2) + 31) / 32) << 5;
    s32 rounded_height;

    context->output.luma_stride = luma_stride;
    context->output.chroma_stride = chroma_stride;
    rounded_height = (height_plus / 16) * 16;
    context->output.luma = output_rfb;
    context->output.chroma0 =
        context->output.luma + rounded_height * luma_stride;
    context->output.chroma1 =
        context->output.chroma0 + (rounded_height / 2) * chroma_stride;
}

void MPVUMC_Finish(void) {}

void MPVUMC_Init(void)
{
    mpvumc_oneref[0] = MPVMC08_OneRef1p_TuneC;
    mpvumc_oneref_y[0] = MPVMC16_OneRef1p_TuneC;
    mpvumc_oneref[1] = MPVMC08_OneRefH2_TuneC;
    mpvumc_oneref[2] = MPVMC08_OneRefV2_TuneC;
    mpvumc_oneref[3] = MPVMC08_OneRef4p_TuneC;
    mpvumc_oneref[4] = MPVMC08_OneRef1p_TuneC;
    mpvumc_oneref[5] = MPVMC08_OneRefH2_TuneC;
    mpvumc_oneref[6] = MPVMC08_OneRefV2_TuneC;
    mpvumc_oneref[7] = MPVMC08_OneRefV2_TuneC;
    mpvumc_oneref_y[1] = MPVMC16_OneRefH2_TuneC;
    mpvumc_oneref_y[2] = MPVMC16_OneRefV2_TuneC;
    mpvumc_oneref_y[3] = MPVMC16_OneRef4p_TuneC;
    mpvumc_oneref_y[4] = MPVMC16_OneRef1p_TuneC;
    mpvumc_oneref_y[5] = MPVMC16_OneRefH2_TuneC;
    mpvumc_oneref_y[6] = MPVMC16_OneRefV2_TuneC;
    mpvumc_oneref_y[7] = MPVMC16_OneRefV2_TuneC;
}
