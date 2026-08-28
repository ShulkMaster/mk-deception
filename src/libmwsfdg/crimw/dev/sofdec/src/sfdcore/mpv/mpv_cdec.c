#include "dolphin/types.h"

typedef union MPVCDECCoefficients {
    f32 values[64];
    f64 pairs[32];
} MPVCDECCoefficients;

typedef struct MPVCDECBlock {
    s32 run, level, sign;
    u32 code_length;
    s32 first_scan, current_scan;
    u8 field_18[4];
    MPVCDECCoefficients* coefficients;
    const u8* quant_matrix;
    s32 quantizer_scale;
    s32* dc_predictor;
    const u8* dc_size_lut;
} MPVCDECBlock;

typedef struct MPVCDECDctParams {
    s8 block_nonzero[6];
    u8 field_06[0x22];
    s32 coded_block_pattern;
    f32* coefficients;
    void** output_blocks;
    u8 field_34[0x20];
} MPVCDECDctParams;

typedef struct MPVCDECContext MPVCDECContext;
typedef s32 (*MPVCDECBlockDecoder)(MPVCDECContext*, MPVCDECBlock*);

struct MPVCDECContext {
    u8 field_0000[0x44];
    MPVCDECBlock block;
    s32 non_intra_mode;
    MPVCDECDctParams dct;
    u8 field_00CC[0x21C];
    s32 quantizer_scale;
    u8 field_02EC[0x5C];
    s32 coded_block_pattern;
    s32 dc_predictor_y, dc_predictor_cb, dc_predictor_cr;
    u8 field_0358[0x28];
    s16 transformed_blocks[6][64];
    MPVCDECCoefficients coefficients[6];
    u8 intra_quant_matrix[64];
    u8 non_intra_quant_matrix[64];
    u8 field_0D00[0x618];
    MPVCDECBlockDecoder decode_intra;
    MPVCDECBlockDecoder decode_non_intra;
    u8 field_1320[8];
    const u8* y_dc_size_lut;
    const u8* c_dc_size_lut;
};

extern void DCT_FsriTransCbp(MPVCDECDctParams* params);
extern void DCT_FsriTrans6Blk(MPVCDECDctParams* params);

static inline void MPVCDEC_ClearCoefficients(MPVCDECCoefficients* coefficients)
{
    s32 index;
    for (index = 0; index < 32; index++) {
        coefficients->pairs[index] = 0.0;
    }
}

s32 MPVCDEC_NintraBlocks(MPVCDECContext* context)
{
    MPVCDECBlock* block = &context->block;
    MPVCDECDctParams* params = &context->dct;
    MPVCDECCoefficients* coefficients = context->coefficients;
    s8* nonzero = params->block_nonzero;
    s32 pattern;
    s32 index;

    block->quantizer_scale = context->quantizer_scale;
    block->quant_matrix = context->non_intra_quant_matrix;
    context->non_intra_mode = 1;
    pattern = context->coded_block_pattern << 2;
    params->coded_block_pattern = pattern;
    for (index = 0; index < 6; index++) {
        if (pattern < 0) {
            block->coefficients = coefficients;
            *nonzero = context->decode_non_intra(context, block);
        }
        pattern <<= 1;
        coefficients++;
        nonzero++;
    }
    DCT_FsriTransCbp(params);
    return 0;
}

s32 MPVCDEC_IntraBlocks(MPVCDECContext* context)
{
    MPVCDECBlock* block;
    MPVCDECDctParams* params;
    s32 index;
    MPVCDEC_ClearCoefficients(&context->coefficients[0]);
    MPVCDEC_ClearCoefficients(&context->coefficients[1]);
    MPVCDEC_ClearCoefficients(&context->coefficients[2]);
    MPVCDEC_ClearCoefficients(&context->coefficients[3]);
    MPVCDEC_ClearCoefficients(&context->coefficients[4]);
    MPVCDEC_ClearCoefficients(&context->coefficients[5]);
    block = &context->block;
    params = &context->dct;
    block->quantizer_scale = context->quantizer_scale;
    block->quant_matrix = context->intra_quant_matrix;
    context->non_intra_mode = 0;
    block->dc_size_lut = context->y_dc_size_lut;
    block->dc_predictor = &context->dc_predictor_y;
    for (index = 0; index < 4; index++) {
        block->coefficients = &context->coefficients[index];
        params->block_nonzero[index] = context->decode_intra(context, block);
    }
    block->dc_size_lut = context->c_dc_size_lut;
    block->dc_predictor = &context->dc_predictor_cb;
    block->coefficients = &context->coefficients[4];
    params->block_nonzero[4] = context->decode_intra(context, block);
    block->dc_predictor = &context->dc_predictor_cr;
    block->coefficients = &context->coefficients[5];
    params->block_nonzero[5] = context->decode_intra(context, block);
    DCT_FsriTrans6Blk(params);
    return 0;
}

void MPVCDEC_InitFrm(MPVCDECContext* context)
{
}
