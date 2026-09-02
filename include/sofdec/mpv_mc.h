#ifndef MKD_SOFDEC_MPV_MC_H
#define MKD_SOFDEC_MPV_MC_H

#include "dolphin/types.h"
#include "cri/sj.h"
#include "sofdec/dct_fsri.h"
#include "sofdec/mpv_error.h"

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
    s32 full_pel;
    s32 r_size;
    s32 shift;
    s32 limit;
    s32 previous_horizontal;
    s32 previous_vertical;
    s32 horizontal;
    s32 vertical;
    u8 field_20[4];
} MPVMotionInfo;

typedef struct MPVPictureAttributes {
    int width;
    int height;
    int macroblocks_per_row;
    int macroblock_rows;
    int frame_rate_code;
    int temporal_reference;
    int picture_type;
    int drop_frame_flag;                       /* +0x1EC in MPVContext */
    int time_code_hours;
    int time_code_minutes;
    int time_code_seconds;
    int time_code_pictures;
    int group_count;
    int field_34;
    int field_38;
    int field_3C;
    int field_40;
    int field_44;
    int field_48;
    int field_4C;
    s16 field_50;
    s16 field_52;
    u8 field_54;
    s8 field_55;
    s8 field_56;
    s8 field_57;
    u8 field_58;
    u8 field_59;
    u8 field_5A;
    u8 field_5B;
    u8 field_5C;
    u8 field_5D;
    s8 field_5E;
    s8 field_5F;
    s8 field_60;
    u8 field_61;
    u8 field_62;
    u8 field_63;
    u8 field_64;
    u8 padding_65[3];
} MPVPictureAttributes;

typedef struct MPVPictureInfo {
    MPVPictureAttributes attributes;
    u8 decoder_state[0x18];
} MPVPictureInfo;

typedef struct MPVFrameBuffers {
    MPVPlaneSet forward;
    MPVPlaneSet backward;
    u8* output_rfb;
    MPVPictureInfo* picture_info;
    s32 decoded_dct_count;
    s32 skipped_dct_count;
} MPVFrameBuffers;

typedef struct MPVBitReader {
    u32 bits;
    u32 next_bits;
    s32 bit_offset;
    const u32* words;
} MPVBitReader;

typedef struct MPVDecoderFields {
    u8 field_190[0x0C];
    s32 mc_table;       /* +0x19C */
    s32 field_1A0;
    s32 field_1A4;
    s32 field_1A8;
    s32 field_1AC;
    void (*callback)(void* argument);
    void* callback_argument;
    u8 field_1B8[0x18];
    MPVPictureAttributes picture; /* +0x1D0 */
} MPVDecoderFields;

typedef union MPVConditionState {
    int conditions[17];
    MPVDecoderFields decoder;
} MPVConditionState;

typedef struct MPVDctCounters {
    u8 field_078[0x18];
    s32 decoded;
    s32 skipped;
    u8 field_098[0x34];
} MPVDctCounters;

typedef union MPVDctState {
    DctFsriParams params;
    MPVDctCounters counters;
} MPVDctState;

typedef union MPVTransformWorkspace {
    float coefficients[9][64];
    DctFsriBlock blocks[6];
} MPVTransformWorkspace;

typedef struct MPVCodingBlock {
    s32 run;
    s32 level;
    s32 sign;
    u32 code_length;
    s32 first_scan;
    s32 current_scan;
    u8 field_18[4];
    void* coefficients;
    const u8* quant_matrix;
    s32 quantizer_scale;
    s32* dc_predictor;
    const u8* dc_size_lut;
} MPVCodingBlock;

typedef struct MPVCodingWorkspace {
    MPVCodingBlock block;
    s32 non_intra_mode;
} MPVCodingWorkspace;

typedef struct MPVUserStream {
    SJ* stream;
    void (*callback)(void* argument, int index);
    void* callback_argument;
} MPVUserStream;

typedef struct MPVPictureUserBuffer {
    void* buffer;
    s32 capacity;
    s32 size;
} MPVPictureUserBuffer;

typedef struct MPVContext MPVContext;
struct MPVContext {
    MPVBitReader bit_reader;
    u32* run_level_8;
    s16* run_level_4;
    s16* run_level_2;
    s16* run_level_1;
    s16* run_level_0a;
    s16* run_level_0b;
    s16* run_level_0c;
    void* index_1120;
    void* index_1100;
    void* index_1160;
    void* index_1260;
    void* index_1280;
    u8* clip_base;
    MPVCodingWorkspace coding;
    MPVDctState dct_state;                    /* +0x078 */
    MPVMCContext mc;                         /* +0x0CC */
    MPVMacroblockSources sources;            /* +0x110 */
    MPVOutputBlocks output_blocks;           /* +0x120 */
    MPVOutputBlocks secondary_output_blocks; /* +0x154 */
    s32 state;                                /* +0x188 */
    s32 field_18C;
    MPVConditionState condition_state;        /* +0x190 */
    u8 field_238[0x18];
    MPVErrorInfo error_info;                  /* +0x250 */
    MPVFrameBuffers frame_buffers;            /* +0x264 */
    MPVYccPlane output;
    u32 field_2A4;
    s32 bit_rate;
    s32 vbv_buffer_units;
    u32 field_2B0;
    s32 link_flag_0;
    s32 link_flag_1;
    s32 vbv_delay;
    void (*decode_macroblock)(MPVContext*, SJ*); /* +0x2C0 */
    void (*skip_macroblocks)(MPVContext*, int);
    int (*decode_intra_blocks)(void*);
    int (*decode_nonintra_blocks)(void*);
    void (*motion_intra)(MPVContext*);
    void (*motion_skipped)(MPVContext*);
    void (*motion_backward)(MPVContext*);
    void (*motion_forward)(MPVContext*);
    void (*motion_bidirect)(MPVContext*);
    s32 field_2E4;
    u32 quantizer_scale;                       /* +0x2E8 */
    MPVMotionInfo forward_motion;
    MPVMotionInfo backward_motion;
    s32 macroblock_index;
    s32 macroblock_row;
    s32 macroblock_column;
    s32 last_macroblock_index;                 /* +0x340 */
    u32 field_344;
    s32 cbp_mask;
    s32 dc_predictor_y;
    s32 dc_predictor_cb;
    s32 dc_predictor_cr;
    s32 field_358;
    s32 stc_code_0;                            /* +0x35C */
    s32 stc_code_1;
    s32 stc_code_2;
    DctFsriBlock* dct_output_blocks[6];
    MPVTransformWorkspace transform;
    s8 intra_quant_matrix[64];                 /* +0xC80 */
    s8 nonintra_quant_matrix[64];              /* +0xCC0 */
    u8 field_D00[0x600];
    s32 field_1300;
    s32 field_1304;
    SJCK header_chunk;                         /* +0x1308 */
    s32 field_1310;
    s32 field_1314;
    int (*decode_intra_block)(void*, void*);
    int (*decode_nonintra_block)(void*, void*);
    u8 field_1320[4];
    s32 field_1324;
    u8* y_dc_size;
    u8* chroma_dc_size;
    void* m2v_handle;                         /* +0x1330 */
    s32 user_data_index;                        /* +0x1334 */
    MPVUserStream user_streams[4];              /* +0x1338 */
    MPVPictureUserBuffer picture_user;          /* +0x1368 */
    u8 field_1374[0x0C];
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
typedef char MPVDecoderFieldsSizeCheck[
    sizeof(MPVDecoderFields) == 0xA8 ? 1 : -1];
typedef char MPVConditionStateSizeCheck[
    sizeof(MPVConditionState) == 0xA8 ? 1 : -1];
typedef char MPVPictureAttributesSizeCheck[
    sizeof(MPVPictureAttributes) == 0x68 ? 1 : -1];
typedef char MPVPictureInfoSizeCheck[
    sizeof(MPVPictureInfo) == 0x80 ? 1 : -1];
typedef char MPVDctCountersSizeCheck[
    sizeof(MPVDctCounters) == 0x54 ? 1 : -1];
typedef char MPVDctStateSizeCheck[sizeof(MPVDctState) == 0x54 ? 1 : -1];
typedef char MPVTransformWorkspaceSizeCheck[
    sizeof(MPVTransformWorkspace) == 0x900 ? 1 : -1];
typedef char MPVCodingWorkspaceSizeCheck[
    sizeof(MPVCodingWorkspace) == 0x34 ? 1 : -1];
typedef char MPVUserStreamSizeCheck[sizeof(MPVUserStream) == 0x0C ? 1 : -1];
typedef char MPVPictureUserBufferSizeCheck[
    sizeof(MPVPictureUserBuffer) == 0x0C ? 1 : -1];
typedef char MPVContextSizeCheck[sizeof(MPVContext) == 0x1380 ? 1 : -1];

int MPV_GetCond(MPVContext* handle, int index, int* value);
int MPV_SetCond(MPVContext* handle, int index, int value);
int MPVLIB_CheckHn(MPVContext* handle);
int MPV_Init(int handle_count, void* work);
MPVContext* MPV_Create(void);
int MPV_Destroy(MPVContext* handle);
void MPV_Finish(void);
int MPV_DecodePicAtrSj(MPVContext* handle, SJ* stream);
int MPV_DecodePicAtr(MPVContext* handle, const SJCK* input,
                     int* consumed_size);
int MPV_GetPicAtr(MPVContext* handle, MPVPictureInfo* picture);
void MPV_GetPicUsr(MPVContext* handle, void** buffer, int* size);
void MPV_SetPicUsrBuf(MPVContext* handle, void* buffer, int capacity);
int MPV_GetBitRate(MPVContext* handle, int* bit_rate);
int MPV_GetVbvBufSiz(MPVContext* handle, int* buffer_size, int* delay,
                     int* byte_rate);
int MPV_DecodeFrmSj(MPVContext* handle, SJ* stream,
                    MPVFrameBuffers* buffers);
int MPV_SkipFrmSj(MPVContext* handle, SJ* stream);
void MPV_GetDctCnt(MPVContext* handle, int* decoded, int* skipped);
int MPV_GetLinkFlg(MPVContext* handle, int* first, int* second);

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
