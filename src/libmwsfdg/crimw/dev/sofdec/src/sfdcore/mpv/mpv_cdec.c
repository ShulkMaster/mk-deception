#include "sofdec/mpv_mc.h"

typedef union MPVCDECCoefficients {
    f32 values[64];
    f64 pairs[32];
} MPVCDECCoefficients;

extern void DCT_FsriTransCbp(DctFsriParams* params);
extern void DCT_FsriTrans6Blk(DctFsriParams* params);

static inline void MPVCDEC_ClearCoefficients(MPVCDECCoefficients* coefficients)
{
    s32 index;
    for (index = 0; index < 32; index++) {
        coefficients->pairs[index] = 0.0;
    }
}

s32 MPVCDEC_NintraBlocks(MPVContext* context)
{
    MPVCodingBlock* block = &context->coding.block;
    DctFsriParams* params = &context->dct_state.params;
    MPVCDECCoefficients* coefficients =
        (MPVCDECCoefficients*)&context->transform.coefficients[3];
    s8* nonzero = params->block_nonzero;
    s32 pattern;
    s32 index;

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
        coefficients++;
        nonzero++;
    }
    DCT_FsriTransCbp(params);
    return 0;
}

s32 MPVCDEC_IntraBlocks(MPVContext* context)
{
    MPVCodingBlock* block;
    DctFsriParams* params;
    s32 index;
    MPVCDECCoefficients* coefficients =
        (MPVCDECCoefficients*)&context->transform.coefficients[3];
    MPVCDEC_ClearCoefficients(&coefficients[0]);
    MPVCDEC_ClearCoefficients(&coefficients[1]);
    MPVCDEC_ClearCoefficients(&coefficients[2]);
    MPVCDEC_ClearCoefficients(&coefficients[3]);
    MPVCDEC_ClearCoefficients(&coefficients[4]);
    MPVCDEC_ClearCoefficients(&coefficients[5]);
    block = &context->coding.block;
    params = &context->dct_state.params;
    block->quantizer_scale = context->quantizer_scale;
    block->quant_matrix = (const u8*)context->intra_quant_matrix;
    context->coding.non_intra_mode = 0;
    block->dc_size_lut = context->y_dc_size;
    block->dc_predictor = &context->dc_predictor_y;
    for (index = 0; index < 4; index++) {
        block->coefficients = &coefficients[index];
        params->block_nonzero[index] =
            context->decode_intra_block(context, block);
    }
    block->dc_size_lut = context->chroma_dc_size;
    block->dc_predictor = &context->dc_predictor_cb;
    block->coefficients = &coefficients[4];
    params->block_nonzero[4] = context->decode_intra_block(context, block);
    block->dc_predictor = &context->dc_predictor_cr;
    block->coefficients = &coefficients[5];
    params->block_nonzero[5] = context->decode_intra_block(context, block);
    DCT_FsriTrans6Blk(params);
    return 0;
}

void MPVCDEC_InitFrm(MPVContext* context)
{
}
