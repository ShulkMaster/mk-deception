#include "sofdec/mpv_mc.h"

static MPVMCFunction mpvumc_oneref_y[2][2][2];
static MPVMCFunction mpvumc_oneref[2][2][2];

void MPVUMC_BpicSkipped(MPVContext* context, s32 count)
{
    s32 end;
    s32 amount;
    void (*decode_macroblock)(MPVContext*);

    context->cbp_mask = 0;
    end = context->macroblock_index;
    decode_macroblock = context->motion_skipped;
    amount = count - 1;
    context->macroblock_index -= amount;
    context->macroblock_column -= amount;
    while (context->macroblock_column < 0) {
        context->macroblock_column +=
            context->condition_state.picture.macroblocks_per_row;
        context->macroblock_row--;
    }
    while (context->macroblock_index < end) {
        decode_macroblock(context);
        context->macroblock_column++;
        if (context->macroblock_column >=
            context->condition_state.picture.macroblocks_per_row) {
            context->macroblock_column = 0;
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
    MPVPlaneSet* output = &context->frame_buffers.forward;
    MPVPlaneSet* reference = output + 1;
    s32 y8;
    s32 y16;

    context->macroblock_index -= amount;
    context->macroblock_column -= amount;
    while (context->macroblock_column < 0) {
        context->macroblock_column +=
            context->condition_state.picture.macroblocks_per_row;
        context->macroblock_row--;
    }
    while (context->macroblock_index < end) {
        MPVBlockOffsets offsets;
        y8 = context->macroblock_row * 8;
        y16 = y8 * 2;
        offsets.chroma = context->macroblock_column * 8 +
                         y8 * output->chroma_stride;
        offsets.luma = context->macroblock_column * 16 +
                       y16 * output->luma_stride;
        mpvumc_PpicSkipMb(&offsets, output, reference);
        context->macroblock_column++;
        if (context->macroblock_column >=
            context->condition_state.picture.macroblocks_per_row) {
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

/* TODO: [borked] 29.605770%; donor 3D tables and setup order agree;
 * motion-call argument lifetimes still differ from retail. */
static void mpvumc_OneReadMb(MPVContext* context, u8* destination,
                             MPVBlockOffsets* offsets,
                             const MPVPlaneSet* planes,
                             const MPVMotionInfo* motion)
{
    MPVMCContext* mc = &context->mc;
    s32 horizontal;
    s32 vertical;
    s32 chroma_stride;
    s32 luma_stride;
    s32 mc_flag;
    s32 chroma_horizontal;
    s32 chroma_vertical;
    s32 luma_offset;
    s32 chroma_offset;
    s32 luma_extra;
    s32 chroma_extra;
    s32 macroblock_row;
    s32 macroblock_column;
    s32 row8;
    s32 row16;
    s32 column8;
    const u8* reference;
    MPVMCFunction luma_function;
    MPVMCFunction chroma_function;
    MPVMCFunction (*luma_table)[2];
    MPVMCFunction (*chroma_table)[2];

    macroblock_row = context->macroblock_row;
    chroma_stride = planes->chroma_stride;
    macroblock_column = context->macroblock_column;
    mc_flag = context->condition_state.conditions[3];
    luma_stride = planes->luma_stride;
    row8 = macroblock_row * 8;
    column8 = macroblock_column * 8;
    offsets->chroma = column8 + row8 * chroma_stride;
    row16 = macroblock_row * 16;
    offsets->luma = column8 * 2 + row16 * luma_stride;
    luma_table = mpvumc_oneref_y[mc_flag];
    chroma_table = mpvumc_oneref[mc_flag];
    horizontal = motion->horizontal;
    vertical = motion->vertical;
    luma_offset = offsets->luma + (horizontal >> 1) +
                  (vertical >> 1) * luma_stride;
    luma_extra = (u32)horizontal & 1;
    luma_function = luma_table[vertical & 1][horizontal & 1];
    chroma_horizontal = horizontal / 2;
    chroma_vertical = vertical / 2;
    chroma_offset = offsets->chroma + (chroma_horizontal >> 1) +
                    (chroma_vertical >> 1) * chroma_stride;
    chroma_extra = (u32)chroma_horizontal & 1;
    chroma_function = chroma_table[chroma_vertical & 1]
                                  [chroma_horizontal & 1];
    chroma_extra &= mc_flag;
    luma_extra &= mc_flag;

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

/* Soft ceiling: retail uses GQR3 paired-single quantized loads/stores for the
 * runtime-proven signed 16-bit DCT blocks; portable scalar C cannot emit it. */
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
    s32 width = context->condition_state.picture.width;
    s32 height = context->condition_state.picture.height;
    s32 macroblocks_wide;
    s32 macroblocks_high;
    s32 rounded_width;
    s32 luma_stride;
    s32 chroma_stride;
    s32 rounded_height;
    u8* output_rfb = context->frame_buffers.output_rfb;

    macroblocks_wide = (width + 15) / 16;
    rounded_width = macroblocks_wide * 16;
    luma_stride = (rounded_width + 31) / 32 * 32;
    chroma_stride = (rounded_width / 2 + 31) / 32 * 32;
    context->output.luma_stride = luma_stride;
    context->output.chroma_stride = chroma_stride;
    macroblocks_high = (height + 15) / 16;
    rounded_height = macroblocks_high * 16;
    context->output.luma = output_rfb;
    context->output.chroma0 =
        context->output.luma + rounded_height * luma_stride;
    context->output.chroma1 =
        context->output.chroma0 + (rounded_height / 2) * chroma_stride;
}

void MPVUMC_Finish(void) {}

/* TODO: [near miss] 95.857140%; typed 3D table stores retain the same score;
 * inspect the remaining initializer order and relocation pairing. */
void MPVUMC_Init(void)
{
    mpvumc_oneref[0][0][0] = MPVMC08_OneRef1p_TuneC;
    mpvumc_oneref_y[0][0][0] = MPVMC16_OneRef1p_TuneC;
    mpvumc_oneref[0][0][1] = MPVMC08_OneRefH2_TuneC;
    mpvumc_oneref[0][1][0] = MPVMC08_OneRefV2_TuneC;
    mpvumc_oneref[0][1][1] = MPVMC08_OneRef4p_TuneC;
    mpvumc_oneref[1][0][0] = MPVMC08_OneRef1p_TuneC;
    mpvumc_oneref[1][0][1] = MPVMC08_OneRefH2_TuneC;
    mpvumc_oneref[1][1][0] = MPVMC08_OneRefV2_TuneC;
    mpvumc_oneref[1][1][1] = MPVMC08_OneRefV2_TuneC;
    mpvumc_oneref_y[0][0][1] = MPVMC16_OneRefH2_TuneC;
    mpvumc_oneref_y[0][1][0] = MPVMC16_OneRefV2_TuneC;
    mpvumc_oneref_y[0][1][1] = MPVMC16_OneRef4p_TuneC;
    mpvumc_oneref_y[1][0][0] = MPVMC16_OneRef1p_TuneC;
    mpvumc_oneref_y[1][0][1] = MPVMC16_OneRefH2_TuneC;
    mpvumc_oneref_y[1][1][0] = MPVMC16_OneRefV2_TuneC;
    mpvumc_oneref_y[1][1][1] = MPVMC16_OneRefV2_TuneC;
}
