#include "sofdec/mpv_mc.h"

#include "runtime/asm_sequences.inc"

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

/* Two chroma rows (one double each) or one luma row (two doubles), with and without
 * clearing the destination cache lines first. */
#define MPVUMC_COPY_CHROMA_CLEAR(dst, src, pitch, first, second) \
    __dcbz(dst, 0);                                               \
    first = (src)[0];                                             \
    __dcbz(dst, (pitch) * 8);                                     \
    second = (src)[pitch];                                        \
    (src) += (pitch) * 2;                                         \
    (dst)[0] = first;                                             \
    (dst)[pitch] = second;                                        \
    (dst) += (pitch) * 2
#define MPVUMC_COPY_CHROMA(dst, src, pitch, first, second)       \
    first = (src)[0];                                             \
    second = (src)[pitch];                                        \
    (src) += (pitch) * 2;                                         \
    (dst)[0] = first;                                             \
    (dst)[pitch] = second;                                        \
    (dst) += (pitch) * 2
#define MPVUMC_COPY_LUMA_CLEAR(dst, src, pitch, first, second)   \
    __dcbz(dst, 0);                                               \
    first = (src)[0];                                             \
    second = (src)[1];                                            \
    (src) += (pitch);                                             \
    (dst)[0] = first;                                             \
    (dst)[1] = second;                                            \
    (dst) += (pitch)
#define MPVUMC_COPY_LUMA(dst, src, pitch, first, second)         \
    first = (src)[0];                                             \
    second = (src)[1];                                            \
    (src) += (pitch);                                             \
    (dst)[0] = first;                                             \
    (dst)[1] = second;                                            \
    (dst) += (pitch)

static void mpvumc_PpicSkipMb(MPVBlockOffsets* offsets,
                              MPVPlaneSet* source,
                              MPVPlaneSet* destination)
{
    s32 pitch;
    f64* src;
    f64* dst;
    f64 first;
    f64 second;

    pitch = destination->chroma_stride / 8;
    if ((offsets->chroma & 0x1F) == 0) {
        src = (f64*)(source->planes[0] + offsets->chroma);
        dst = (f64*)(destination->planes[0] + offsets->chroma);
        MPVUMC_COPY_CHROMA_CLEAR(dst, src, pitch, first, second);
        MPVUMC_COPY_CHROMA_CLEAR(dst, src, pitch, first, second);
        MPVUMC_COPY_CHROMA_CLEAR(dst, src, pitch, first, second);
        MPVUMC_COPY_CHROMA_CLEAR(dst, src, pitch, first, second);
        src = (f64*)(source->planes[1] + offsets->chroma);
        dst = (f64*)(destination->planes[1] + offsets->chroma);
        MPVUMC_COPY_CHROMA_CLEAR(dst, src, pitch, first, second);
        MPVUMC_COPY_CHROMA_CLEAR(dst, src, pitch, first, second);
        MPVUMC_COPY_CHROMA_CLEAR(dst, src, pitch, first, second);
        MPVUMC_COPY_CHROMA_CLEAR(dst, src, pitch, first, second);
    } else {
        src = (f64*)(source->planes[0] + offsets->chroma);
        dst = (f64*)(destination->planes[0] + offsets->chroma);
        MPVUMC_COPY_CHROMA(dst, src, pitch, first, second);
        MPVUMC_COPY_CHROMA(dst, src, pitch, first, second);
        MPVUMC_COPY_CHROMA(dst, src, pitch, first, second);
        MPVUMC_COPY_CHROMA(dst, src, pitch, first, second);
        src = (f64*)(source->planes[1] + offsets->chroma);
        dst = (f64*)(destination->planes[1] + offsets->chroma);
        MPVUMC_COPY_CHROMA(dst, src, pitch, first, second);
        MPVUMC_COPY_CHROMA(dst, src, pitch, first, second);
        MPVUMC_COPY_CHROMA(dst, src, pitch, first, second);
        MPVUMC_COPY_CHROMA(dst, src, pitch, first, second);
    }
    pitch = destination->luma_stride / 8;
    if ((offsets->luma & 0x1F) == 0) {
        src = (f64*)(source->planes[2] + offsets->luma);
        dst = (f64*)(destination->planes[2] + offsets->luma);
        MPVUMC_COPY_LUMA_CLEAR(dst, src, pitch, first, second);
        MPVUMC_COPY_LUMA_CLEAR(dst, src, pitch, first, second);
        MPVUMC_COPY_LUMA_CLEAR(dst, src, pitch, first, second);
        MPVUMC_COPY_LUMA_CLEAR(dst, src, pitch, first, second);
        MPVUMC_COPY_LUMA_CLEAR(dst, src, pitch, first, second);
        MPVUMC_COPY_LUMA_CLEAR(dst, src, pitch, first, second);
        MPVUMC_COPY_LUMA_CLEAR(dst, src, pitch, first, second);
        MPVUMC_COPY_LUMA_CLEAR(dst, src, pitch, first, second);
        MPVUMC_COPY_LUMA_CLEAR(dst, src, pitch, first, second);
        MPVUMC_COPY_LUMA_CLEAR(dst, src, pitch, first, second);
        MPVUMC_COPY_LUMA_CLEAR(dst, src, pitch, first, second);
        MPVUMC_COPY_LUMA_CLEAR(dst, src, pitch, first, second);
        MPVUMC_COPY_LUMA_CLEAR(dst, src, pitch, first, second);
        MPVUMC_COPY_LUMA_CLEAR(dst, src, pitch, first, second);
        MPVUMC_COPY_LUMA_CLEAR(dst, src, pitch, first, second);
        MPVUMC_COPY_LUMA_CLEAR(dst, src, pitch, first, second);
    } else {
        src = (f64*)(source->planes[2] + offsets->luma);
        dst = (f64*)(destination->planes[2] + offsets->luma);
        MPVUMC_COPY_LUMA(dst, src, pitch, first, second);
        MPVUMC_COPY_LUMA(dst, src, pitch, first, second);
        MPVUMC_COPY_LUMA(dst, src, pitch, first, second);
        MPVUMC_COPY_LUMA(dst, src, pitch, first, second);
        MPVUMC_COPY_LUMA(dst, src, pitch, first, second);
        MPVUMC_COPY_LUMA(dst, src, pitch, first, second);
        MPVUMC_COPY_LUMA(dst, src, pitch, first, second);
        MPVUMC_COPY_LUMA(dst, src, pitch, first, second);
        MPVUMC_COPY_LUMA(dst, src, pitch, first, second);
        MPVUMC_COPY_LUMA(dst, src, pitch, first, second);
        MPVUMC_COPY_LUMA(dst, src, pitch, first, second);
        MPVUMC_COPY_LUMA(dst, src, pitch, first, second);
        MPVUMC_COPY_LUMA(dst, src, pitch, first, second);
        MPVUMC_COPY_LUMA(dst, src, pitch, first, second);
        MPVUMC_COPY_LUMA(dst, src, pitch, first, second);
        MPVUMC_COPY_LUMA(dst, src, pitch, first, second);
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

static const f32 const_mem = 1.0f;

static asm void mpvumc_BiMakeMb(MPVMacroblockSources* sources,
                            MPVOutputBlocks* output, s32 cbp_mask)
{
    SEQ_mpvumc_BiMakeMb();
}

static asm void mpvumc_OneMakeMb(MPVMacroblockSources* sources,
                             MPVOutputBlocks* output, s32 cbp_mask)
{
    SEQ_mpvumc_OneMakeMb();
}

static void mpvumc_OneReadMb(MPVContext* context, u8* destination,
                            MPVBlockOffsets* offsets,
                            MPVPlaneSet* planes,
                            MPVMotionInfo* motion)
{
    s32 chroma_stride;
    s32 luma_stride;
    s32 chroma_offset;
    s32 luma_offset;
    MPVMCContext* mc = &context->mc;
    MPVMCFunction chroma_function;
    MPVMCFunction luma_function;
    s32 luma_extra;
    s32 chroma_extra;
    const u8* reference;
    s32 vertical;
    s32 horizontal;
    s32 macroblock_row;
    s32 macroblock_column;
    MPVMCFunction (*chroma_table)[2];
    MPVMCFunction (*luma_table)[2];
    s32 mc_flag;
    s32 row8;
    s32 row16;
    s32 column8;

    macroblock_row = context->macroblock_row;
    chroma_stride = planes->chroma_stride;
    macroblock_column = context->macroblock_column;
    mc_flag = context->condition_state.conditions[3];
    luma_stride = planes->luma_stride;
    row8 = macroblock_row * 8;
    column8 = macroblock_column * 8;
    offsets->chroma = column8 + row8 * chroma_stride;
    row16 = macroblock_row * 16;
    offsets->luma = column8 * 2 + row16 * planes->luma_stride;
    luma_table = mpvumc_oneref_y[mc_flag];
    chroma_table = mpvumc_oneref[mc_flag];
    horizontal = motion->horizontal;
    vertical = motion->vertical;
    luma_offset = offsets->luma + (horizontal >> 1) +
                  (vertical >> 1) * luma_stride;
    luma_extra = (u32)horizontal & 1;
    luma_function = luma_table[vertical & 1][horizontal & 1];
    chroma_offset = offsets->chroma + ((horizontal / 2) >> 1) +
                    ((vertical / 2) >> 1) * chroma_stride;
    chroma_extra = (u32)(horizontal / 2) & 1;
    chroma_function = chroma_table[(vertical / 2) & 1]
                                  [(horizontal / 2) & 1];
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
    mc->reference1 = luma_stride + luma_extra + reference;
    luma_function(mc);
}

void MPVUMC_BiDirect(MPVContext* context)
{
    MPVBlockOffsets offsets;
    MPVOutputBlocks* output;
    MPVFrameBuffers* frame_buffers = &context->frame_buffers;
    MPVMacroblockSources* sources = &context->sources;
    mpvumc_OneReadMb(context, context->sources.prediction0, &offsets,
                     &frame_buffers->forward,
                     &context->forward_motion);
    mpvumc_OneReadMb(context, sources->prediction1, &offsets,
                     &frame_buffers->backward,
                     &context->backward_motion);
    output = &context->output_blocks;
    output->blocks[0].destination = context->output.chroma0 + offsets.chroma;
    output->blocks[1].destination = context->output.chroma1 + offsets.chroma;
    output->blocks[2].destination = context->output.luma + offsets.luma;
    output->blocks[3].destination = output->blocks[2].destination + 8;
    output->blocks[4].destination =
        output->blocks[2].destination + context->output.luma_stride * 8;
    output->blocks[5].destination = output->blocks[4].destination + 8;
    mpvumc_BiMakeMb(sources, output, context->cbp_mask);
}

void MPVUMC_Backward(MPVContext* context)
{
    MPVBlockOffsets offsets;
    MPVMacroblockSources* sources = &context->sources;
    MPVOutputBlocks* output;
    mpvumc_OneReadMb(context, sources->prediction0, &offsets,
                     &context->frame_buffers.backward, &context->backward_motion);
    output = &context->output_blocks;
    output->blocks[0].destination = context->output.chroma0 + offsets.chroma;
    output->blocks[1].destination = context->output.chroma1 + offsets.chroma;
    output->blocks[2].destination = context->output.luma + offsets.luma;
    output->blocks[3].destination = output->blocks[2].destination + 8;
    output->blocks[4].destination =
        output->blocks[2].destination + context->output.luma_stride * 8;
    output->blocks[5].destination = output->blocks[4].destination + 8;
    mpvumc_OneMakeMb(sources, output, context->cbp_mask);
}

void MPVUMC_Forward(MPVContext* context)
{
    MPVBlockOffsets offsets;
    MPVMacroblockSources* sources = &context->sources;
    MPVOutputBlocks* output;
    mpvumc_OneReadMb(context, sources->prediction0, &offsets,
                     &context->frame_buffers.forward, &context->forward_motion);
    output = &context->output_blocks;
    output->blocks[0].destination = context->output.chroma0 + offsets.chroma;
    output->blocks[1].destination = context->output.chroma1 + offsets.chroma;
    output->blocks[2].destination = context->output.luma + offsets.luma;
    output->blocks[3].destination = output->blocks[2].destination + 8;
    output->blocks[4].destination =
        output->blocks[2].destination + context->output.luma_stride * 8;
    output->blocks[5].destination = output->blocks[4].destination + 8;
    mpvumc_OneMakeMb(sources, output, context->cbp_mask);
}

static asm void mpvumc_OutputIntra6blk(const DctFsriBlock blocks[6],
                                   MPVOutputBlocks* output, const u8* clip)
{
    SEQ_mpvumc_OutputIntra6blk();
}

void MPVUMC_Intra(MPVContext* context)
{
    int column;
    int row;
    int column_8;
    int row_8;
    int chroma_offset;
    int luma_offset;
    int luma_stride;
    MPVOutputBlocks* output;

    column_8 = (column = context->macroblock_column) * 8;
    row_8 = (row = context->macroblock_row) * 8;
    chroma_offset = column_8 + row_8 * context->output.chroma_stride;
    luma_offset = column * 16 + row * 16 *
        (luma_stride = context->output.luma_stride);
    output = &context->output_blocks;
    output->blocks[0].destination = context->output.chroma0 + chroma_offset;
    output->blocks[1].destination = context->output.chroma1 + chroma_offset;
    output->blocks[2].destination = context->output.luma + luma_offset;
    output->blocks[3].destination = output->blocks[2].destination + 8;
    output->blocks[4].destination = output->blocks[2].destination + luma_stride * 8;
    output->blocks[5].destination = output->blocks[4].destination + 8;
    mpvumc_OutputIntra6blk(context->transform.blocks, output,
                           context->clip_base);
}

asm void MPVUMC_SetGqr(void)
{
    SEQ_MPVUMC_SetGqr();
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

void MPVUMC_Init(void)
{
    mpvumc_oneref[0][0][0] = MPVMC08_OneRef1p_TuneC;
    mpvumc_oneref[0][0][1] = MPVMC08_OneRefH2_TuneC;
    mpvumc_oneref[0][1][0] = MPVMC08_OneRefV2_TuneC;
    mpvumc_oneref[0][1][1] = MPVMC08_OneRef4p_TuneC;
    mpvumc_oneref[1][0][0] = MPVMC08_OneRef1p_TuneC;
    mpvumc_oneref[1][0][1] = MPVMC08_OneRefH2_TuneC;
    mpvumc_oneref[1][1][0] = MPVMC08_OneRefV2_TuneC;
    mpvumc_oneref[1][1][1] = MPVMC08_OneRefV2_TuneC;
    mpvumc_oneref_y[0][0][0] = MPVMC16_OneRef1p_TuneC;
    mpvumc_oneref_y[0][0][1] = MPVMC16_OneRefH2_TuneC;
    mpvumc_oneref_y[0][1][0] = MPVMC16_OneRefV2_TuneC;
    mpvumc_oneref_y[0][1][1] = MPVMC16_OneRef4p_TuneC;
    mpvumc_oneref_y[1][0][0] = MPVMC16_OneRef1p_TuneC;
    mpvumc_oneref_y[1][0][1] = MPVMC16_OneRefH2_TuneC;
    mpvumc_oneref_y[1][1][0] = MPVMC16_OneRefV2_TuneC;
    mpvumc_oneref_y[1][1][1] = MPVMC16_OneRefV2_TuneC;
}
