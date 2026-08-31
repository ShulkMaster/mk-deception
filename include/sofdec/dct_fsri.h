#ifndef MKD_SOFDEC_DCT_FSRI_H
#define MKD_SOFDEC_DCT_FSRI_H

#include "dolphin/types.h"

typedef union DctFsriBlock {
    s16 samples[64];
    u32 packed[32];
} DctFsriBlock;

typedef struct DctFsriParams {
    s8 block_nonzero[6];
    u8 field_06[0x22];
    int coded_block_pattern;
    float* coefficients;
    DctFsriBlock** output_blocks;
    u8 field_34[0x14];
    float* workspace;
    u8 field_4C[8];
} DctFsriParams;

void DCT_FsriInitPa(DctFsriParams* params);
void DCT_FsriInit(void);
void DCT_FsriInitScaleTbl(float scale_table[64]);

typedef char DctFsriBlockSizeCheck[
    sizeof(DctFsriBlock) == 0x80 ? 1 : -1];
typedef char DctFsriParamsSizeCheck[
    sizeof(DctFsriParams) == 0x54 ? 1 : -1];

#endif
