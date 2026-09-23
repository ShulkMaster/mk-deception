#include "sofdec/mpv_mc.h"

extern void DCT_FsriTransCbp(DctFsriParams* params);
extern void DCT_FsriTrans6Blk(DctFsriParams* params);

/* Clear one block and advance within the six-block paired-store view. */
static inline void MPVCDEC_ClearCoefficients(f64** cursor)
{
    *(*cursor)++ = 0.0;
    *(*cursor)++ = 0.0;
    *(*cursor)++ = 0.0;
    *(*cursor)++ = 0.0;
    *(*cursor)++ = 0.0;
    *(*cursor)++ = 0.0;
    *(*cursor)++ = 0.0;
    *(*cursor)++ = 0.0;
    *(*cursor)++ = 0.0;
    *(*cursor)++ = 0.0;
    *(*cursor)++ = 0.0;
    *(*cursor)++ = 0.0;
    *(*cursor)++ = 0.0;
    *(*cursor)++ = 0.0;
    *(*cursor)++ = 0.0;
    *(*cursor)++ = 0.0;
    *(*cursor)++ = 0.0;
    *(*cursor)++ = 0.0;
    *(*cursor)++ = 0.0;
    *(*cursor)++ = 0.0;
    *(*cursor)++ = 0.0;
    *(*cursor)++ = 0.0;
    *(*cursor)++ = 0.0;
    *(*cursor)++ = 0.0;
    *(*cursor)++ = 0.0;
    *(*cursor)++ = 0.0;
    *(*cursor)++ = 0.0;
    *(*cursor)++ = 0.0;
    *(*cursor)++ = 0.0;
    *(*cursor)++ = 0.0;
    *(*cursor)++ = 0.0;
    *(*cursor)++ = 0.0;
}

/* TODO: [breakthrough needed] 92.558136%; retail forms a context-relative
 * block address inside the arm; typed cursors hoist it, indexed form regresses. */
s32 MPVCDEC_NintraBlocks(MPVContext* context)
{
    MPVCodingBlock* block;
    DctFsriParams* params;
    f32* coefficients;
    s8* nonzero;
    s32 pattern;
    s32 index;

    block = &context->coding.block;
    params = &context->dct_state;
    coefficients = &context->transform.coefficients[0][0];
    nonzero = params->block_nonzero;
    block->quantizer_scale = context->quantizer_scale;
    block->quant_matrix = (const u8*)context->nonintra_quant_matrix;
    context->coding.non_intra_mode = 1;
    pattern = (s32)((u32)context->cbp_mask << 2);
    params->coded_block_pattern = pattern;
    for (index = 0; index < 6; index++) {
        if (pattern < 0) {
            block->coefficients = coefficients;
            *nonzero = context->decode_nonintra_block(context, block);
        }
        pattern = (s32)((u32)pattern << 1);
        coefficients += 64;
        nonzero++;
    }
    DCT_FsriTransCbp(params);
    return 0;
}

s32 MPVCDEC_IntraBlocks(MPVContext* context)
{
    MPVCodingBlock* block;
    DctFsriParams* params;
    {
        /* Retail clears each pair of 32-bit coefficients with one double store. */
        f64* cursor = (f64*)&context->transform.coefficients[0][0];
        MPVCDEC_ClearCoefficients(&cursor);
        MPVCDEC_ClearCoefficients(&cursor);
        MPVCDEC_ClearCoefficients(&cursor);
        MPVCDEC_ClearCoefficients(&cursor);
        MPVCDEC_ClearCoefficients(&cursor);
        MPVCDEC_ClearCoefficients(&cursor);
    }
    block = &context->coding.block;
    params = &context->dct_state;
    block->quantizer_scale = context->quantizer_scale;
    block->quant_matrix = (const u8*)context->intra_quant_matrix;
    context->coding.non_intra_mode = 0;
    block->dc_size_lut = context->y_dc_size;
    block->dc_predictor = &context->dc_predictor_y;
    block->coefficients = context->transform.coefficients[0];
    params->block_nonzero[0] = context->decode_intra_block(context, block);
    block->coefficients = context->transform.coefficients[1];
    params->block_nonzero[1] = context->decode_intra_block(context, block);
    block->coefficients = context->transform.coefficients[2];
    params->block_nonzero[2] = context->decode_intra_block(context, block);
    block->coefficients = context->transform.coefficients[3];
    params->block_nonzero[3] = context->decode_intra_block(context, block);
    block->dc_size_lut = context->chroma_dc_size;
    block->dc_predictor = &context->dc_predictor_cb;
    block->coefficients = context->transform.coefficients[4];
    params->block_nonzero[4] = context->decode_intra_block(context, block);
    block->dc_predictor = &context->dc_predictor_cr;
    block->coefficients = context->transform.coefficients[5];
    params->block_nonzero[5] = context->decode_intra_block(context, block);
    DCT_FsriTrans6Blk(params);
    return 0;
}

void MPVCDEC_InitFrm(MPVContext* context)
{
}
