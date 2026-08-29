#ifndef MKD_SOFDEC_MPV_MC_H
#define MKD_SOFDEC_MPV_MC_H

#include "dolphin/types.h"

typedef struct MPVMCContext MPVMCContext;
typedef void (*MPVMCFunction)(MPVMCContext* context);

struct MPVMCContext {
    MPVMCFunction functions08[4]; /* +0x00 */
    u8 field_10[8];               /* +0x10: unaccessed, purpose unknown */
    u8* destination;              /* +0x18 */
    u32 field_1c;                 /* +0x1C: unaccessed, purpose unknown */
    s32 reference_stride;         /* +0x20 */
    const u8* reference0;         /* +0x24 */
    const u8* reference1;         /* +0x28 */
    u8 field_2c[8];               /* +0x2C: unaccessed, purpose unknown */
    MPVMCFunction functions16[4]; /* +0x34 */
};

typedef struct MPVPlaneSet {
    u8* planes[3];
    s16 chroma_stride;
    s16 luma_stride;
} MPVPlaneSet;

typedef struct MPVBlockOffsets {
    s32 chroma;
    s32 luma;
} MPVBlockOffsets;

typedef struct MPVOutputBlock {
    u8* destination;
    s32 stride;
} MPVOutputBlock;

typedef struct MPVOutputBlocks {
    s32 count;
    MPVOutputBlock blocks[6];
} MPVOutputBlocks;

typedef struct MPVMacroblockSources {
    void* field_00;
    s16* residual;
    u8* prediction0;
    u8* prediction1;
} MPVMacroblockSources;

typedef struct MPVYccPlane {
    u8* chroma0;
    u8* chroma1;
    u8* luma;
    s16 chroma_stride;
    s16 luma_stride;
} MPVYccPlane;

typedef struct MPVMotionInfo {
    u8 field_00[0x18];
    s32 horizontal;
    s32 vertical;
    u8 field_20[4];
} MPVMotionInfo;

typedef struct MPVContext MPVContext;
struct MPVContext {
    u8 field_000[0xCC];
    MPVMCContext mc;                         /* +0x0CC */
    MPVMacroblockSources sources;            /* +0x110 */
    MPVOutputBlocks output_blocks;           /* +0x120 */
    MPVOutputBlocks secondary_output_blocks; /* +0x154 */
    u8 field_188[0x14];
    s32 mc_table;                            /* +0x19C */
    u8 field_1A0[4];
    s32 field_1A4;
    u8 field_1A8[0x28];
    s32 width;
    s32 height;
    s32 macroblocks_per_row;
    u8 field_1DC[0x88];
    MPVPlaneSet forward;                     /* +0x264 */
    MPVPlaneSet backward;                    /* +0x274 */
    u8* output_rfb;
    u8 field_288[0xC];
    MPVYccPlane output;
    u8 field_2A4[0x30];
    void (*decode_macroblock)(MPVContext*);
    u8 field_2D8[0x14];
    MPVMotionInfo forward_motion;
    MPVMotionInfo backward_motion;
    s32 macroblock_index;
    s32 macroblock_row;
    s32 macroblock_column;
    u8 field_340[8];
    s32 cbp_mask;
    u8 field_34C[0x34];
    float intra_blocks[6][64];
    u8 field_980[0x380];
    u8 field_D00[1];
};

typedef char MPVMCContextSizeCheck[sizeof(MPVMCContext) == 0x44 ? 1 : -1];
typedef char MPVPlaneSetSizeCheck[sizeof(MPVPlaneSet) == 0x10 ? 1 : -1];
typedef char MPVBlockOffsetsSizeCheck[
    sizeof(MPVBlockOffsets) == 0x08 ? 1 : -1];
typedef char MPVOutputBlockSizeCheck[
    sizeof(MPVOutputBlock) == 0x08 ? 1 : -1];
typedef char MPVOutputBlocksSizeCheck[
    sizeof(MPVOutputBlocks) == 0x34 ? 1 : -1];
typedef char MPVMacroblockSourcesSizeCheck[
    sizeof(MPVMacroblockSources) == 0x10 ? 1 : -1];
typedef char MPVYccPlaneSizeCheck[sizeof(MPVYccPlane) == 0x10 ? 1 : -1];
typedef char MPVMotionInfoSizeCheck[
    sizeof(MPVMotionInfo) == 0x24 ? 1 : -1];

void MPVMC08_OneRef1p_TuneC(MPVMCContext* context);
void MPVMC08_OneRefH2_TuneC(MPVMCContext* context);
void MPVMC08_OneRefV2_TuneC(MPVMCContext* context);
void MPVMC08_OneRef4p_TuneC(MPVMCContext* context);
void MPVMC08_Init(MPVMCFunction functions[4]);

void MPVMC16_OneRef1p_TuneC(MPVMCContext* context);
void MPVMC16_OneRefH2_TuneC(MPVMCContext* context);
void MPVMC16_OneRefV2_TuneC(MPVMCContext* context);
void MPVMC16_OneRef4p_TuneC(MPVMCContext* context);
void MPVMC16_Init(MPVMCContext* context);

#endif
