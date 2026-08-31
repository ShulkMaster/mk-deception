#include "sofdec/mpv_mc.h"
#include "runtime/cstring.h"
#include "dolphin/base/PPCArch.h"
#include "dolphin/cache.h"
#include "sofdec/uty_mem.h"

typedef struct MPVLibWork {
    int conditions[17];
    u32 field_44;
    u32 flags;
    void* field_4C;
    void* index_work;
    int handle_count;
    MPVContext* handles;
} MPVLibWork;

void MPV_MbCbFn(void);

const char MPVLIB_version_str[0x54] =
    "\nCRI MPV/GC Ver.1.933 Build:Sep  3 2004 11:38:28\n"
    "\0Append: MW2407 GC20Apr2004Patch1\n";
static const int mpvlib_cond_dfl[17] = {
    0, 1, 1, 0, 0, 0, 3, 0x7FFFFFFF, (int)MPV_MbCbFn,
    0, 0, 0, 0, 0, 0, 0, 0x5A5A5A5A
};
const int mpvlib_siz_mpvwork = 0x5C;
const int mpvlib_siz_mpvobj = 0x1378;
const int mpvlib_siz_mpvixa = 0x1C60;
static const struct MPVLibChecks {
    u32 endian_probe;
    char version[6];
    u8 padding[2];
} mpvlib_checks = {0x01020304, "1.933", {0, 0}};

int mpvlib_use_lc;
u8 mpv_clip_0_255_tbl[0x400];
u8* mpv_clip_0_255_base;
static MPVContext* mpvlib_mpvobj;
void* mpvlib_oix;
void* mpvlib_iix;
static MPVLibWork mpvlib_libwork;
static const char* cri_verstr_ptr;
u32 gap_06_804984BC_bss;

extern u32* mpvvlc_run_level_8;
extern s16* mpvvlc_run_level_4;
extern s16* mpvvlc_run_level_2;
extern s16* mpvvlc_run_level_1;
extern s16* mpvvlc_run_level_0a;
extern s16* mpvvlc_run_level_0b;
extern s16* mpvvlc_run_level_0c;
extern u8* mpvvlc_y_dcsiz;
extern u8* mpvvlc_c_dcsiz;

extern void MPVM2V_SetCond(MPVContext* handle, int index, int value);
extern void MPVM2V_Destroy(MPVContext* handle);
extern void* MPVM2V_Create(void);
extern void MPVM2V_Finish(void);
extern void MPVM2V_Init(void);
extern void MPVUMC_Finish(void);
extern void MPVUMC_Init(void);
extern void MPVCMC_InitObj(MPVContext* handle);
extern void MPVCMC_Init(void);
extern void MPVHDEC_Init(void);
extern void MPVFRM_Init(void);
extern int MPVVLC_IsVlcSizErr(void);
extern void MPVVLC_Init(void* work, MPVContext* decoder);
extern void MPVBDEC_Init(void* context);
extern int MPVDEC_CheckVersion(const char* version, int object_size,
                               int alignment);
extern int MPVABDEC_IntraBlock(void* context, void* block);
extern int MPVABDEC_NintraBlock(void* context, void* block);
extern void MPV_SetUsrSj(MPVContext* handle, int index, void* stream,
                         void (*callback)(void* argument, int index),
                         void* callback_argument);
extern void MPV_SetPicUsrBuf(MPVContext* handle, void* buffer, int size);
void MPV_MbCbFn(void);
static void mpvlib_InitPicAtr(MPVPictureAttributes* attributes);

static inline int mpvlib_CheckHandle(MPVContext* handle)
{
    mpvlib_mpvobj = handle;
    if (handle == 0) {
        return -1;
    }
    if (handle->state != 2) {
        return -1;
    }
    return 0;
}

int MPVLIB_CheckHn(MPVContext* handle)
{
    int state;

    mpvlib_mpvobj = handle;
    if (handle == 0) {
        return -1;
    }

    state = handle->state;
    return ((state - 2) | (2 - state)) >> 31;
}

int MPV_GetCond(MPVContext* handle, int index, int* value)
{
    int* conditions;

    if (handle == 0) {
        conditions = mpvlib_libwork.conditions;
    } else {
        if (mpvlib_CheckHandle(handle) != 0) {
            return MPVERR_SetCode(0, 0xFF030210);
        }
        conditions = handle->condition_state.conditions;
    }

    *value = conditions[index];
    return 0;
}

int MPV_SetCond(MPVContext* handle, int index, int value)
{
    MPVContext* current;
    int* conditions;
    int remaining;

    if (index == 8 && value == 0) {
        value = (int)MPV_MbCbFn;
    }

    if (handle == 0) {
        current = mpvlib_libwork.handles;
        remaining = mpvlib_libwork.handle_count;
        while (remaining-- > 0) {
            if (current->state == 2) {
                current->condition_state.conditions[index] = value;
            }
            current++;
        }
        conditions = mpvlib_libwork.conditions;
    } else {
        if (mpvlib_CheckHandle(handle) != 0) {
            return MPVERR_SetCode(0, 0xFF030202);
        }
        conditions = handle->condition_state.conditions;
    }

    conditions[index] = value;
    MPVM2V_SetCond(handle, index, value);
    return 0;
}

void MPV_MbCbFn(void)
{
}

int MPV_Destroy(MPVContext* handle)
{
    if (mpvlib_CheckHandle(handle) != 0) {
        return MPVERR_SetCode(0, 0xFF030201);
    }

    MPVM2V_Destroy(handle);
    if ((mpvlib_libwork.flags & 0x10000000) != 0) {
        DCInvalidateRange(handle, sizeof(*handle));
    }
    handle->state = 1;
    return 0;
}

void MPV_GetDctCnt(MPVContext* handle, int* decoded, int* skipped)
{
    *decoded = handle->dct_state.counters.decoded;
    *skipped = handle->dct_state.counters.skipped;
}

static MPVContext* mpvlib_InitHn(MPVContext* handle)
{
    u8* index_work = mpvlib_libwork.index_work;
    DctFsriParams* dct_params;
    int index;

    handle->run_level_8 = mpvvlc_run_level_8;
    handle->run_level_4 = mpvvlc_run_level_4 - 8;
    handle->run_level_2 = mpvvlc_run_level_2 - 16;
    handle->run_level_1 = mpvvlc_run_level_1 - 16;
    handle->run_level_0a = mpvvlc_run_level_0a;
    handle->run_level_0b = mpvvlc_run_level_0b;
    handle->run_level_0c = mpvvlc_run_level_0c;
    handle->index_1260 = index_work + 0x1260;
    handle->index_1280 = index_work + 0x1280;
    handle->index_1120 = index_work + 0x1120;
    handle->index_1100 = index_work + 0x1100;
    handle->index_1160 = index_work + 0x1160;
    handle->clip_base = mpv_clip_0_255_base;

    handle->sources.field_00 = handle->clip_base;
    handle->sources.residual = handle->transform.blocks[0].samples;
    handle->sources.prediction0 = handle->field_D00;
    handle->sources.prediction1 = handle->sources.prediction0 + 0x180;
    handle->dct_output_blocks[0] = &handle->transform.blocks[2];
    handle->dct_output_blocks[1] = &handle->transform.blocks[3];
    handle->dct_output_blocks[2] = &handle->transform.blocks[4];
    handle->dct_output_blocks[3] = &handle->transform.blocks[5];
    handle->dct_output_blocks[4] = &handle->transform.blocks[0];
    handle->dct_output_blocks[5] = &handle->transform.blocks[1];
    handle->field_2E4 = 0;
    handle->field_18C = 0;

    UTY_MemcpyDword((unsigned int*)handle->condition_state.conditions,
                    (const unsigned int*)mpvlib_libwork.conditions, 16);
    MPVERR_InitErrInf(&handle->error_info);
    MPVCMC_InitObj(handle);
    dct_params = &handle->dct_state.params;
    DCT_FsriInitPa(dct_params);
    dct_params->workspace = (float*)handle->field_D00;
    dct_params->coefficients = &handle->transform.coefficients[3][0];
    dct_params->output_blocks = handle->dct_output_blocks;
    mpvlib_InitPicAtr(&handle->condition_state.decoder.picture);

    handle->field_1300 = 0;
    handle->field_1304 = 0;
    handle->y_dc_size = mpvvlc_y_dcsiz;
    handle->chroma_dc_size = mpvvlc_c_dcsiz;
    handle->field_358 = 0;
    handle->stc_code_0 = 0;
    handle->stc_code_1 = 0;
    handle->stc_code_2 = 0;
    handle->field_1314 = 0;
    handle->decode_intra_block = MPVABDEC_IntraBlock;
    handle->decode_nonintra_block = MPVABDEC_NintraBlock;
    handle->field_1324 = handle->condition_state.decoder.field_1AC;

    for (index = 0; index < 4; index++) {
        MPV_SetUsrSj(handle, index, 0, 0, 0);
    }
    MPV_SetPicUsrBuf(handle, 0, 0);
    handle->state = 2;
    return handle;
}

MPVContext* MPV_Create(void)
{
    MPVContext* current = mpvlib_libwork.handles;
    MPVContext* handle = 0;
    int remaining = mpvlib_libwork.handle_count;

    while (remaining-- > 0) {
        if (current->state == 1) {
            handle = current;
            break;
        }
        current++;
    }
    if (handle == 0) {
        return 0;
    }

    if ((mpvlib_libwork.flags & 0x10000000) != 0) {
        DCInvalidateRange(handle, sizeof(*handle));
        memset(handle, 0, sizeof(*handle));
    }
    handle = mpvlib_InitHn(handle);
    handle->m2v_handle = MPVM2V_Create();
    return handle;
}

void MPV_Finish(void)
{
    MPVUMC_Finish();
    MPVM2V_Finish();
    if ((mpvlib_libwork.flags & 0x10000000) != 0) {
        DCInvalidateRange(mpvlib_libwork.index_work, 0x1C60);
    }
}

static void mpvlib_InitPicAtr(MPVPictureAttributes* attributes)
{
    int zero;
    int three;
    int one;
    int negative_one;
    int byte_max;

    memset(attributes, 0, 4);
    zero = 0;
    three = 3;
    one = 1;
    negative_one = -1;
    byte_max = 0xFF;
    attributes->width = zero;
    attributes->height = zero;
    attributes->macroblocks_per_row = zero;
    attributes->macroblock_rows = zero;
    attributes->frame_rate_code = zero;
    attributes->temporal_reference = zero;
    attributes->picture_type = zero;
    attributes->drop_frame_flag = zero;
    attributes->time_code_hours = zero;
    attributes->time_code_minutes = zero;
    attributes->time_code_seconds = zero;
    attributes->time_code_pictures = zero;
    attributes->group_count = zero;
    attributes->field_34 = zero;
    attributes->field_38 = three;
    attributes->field_3C = one;
    attributes->field_40 = one;
    attributes->field_44 = one;
    attributes->field_48 = zero;
    attributes->field_4C = zero;
    attributes->field_50 = negative_one;
    attributes->field_52 = negative_one;
    attributes->field_54 = zero;
    attributes->field_55 = negative_one;
    attributes->field_56 = negative_one;
    attributes->field_57 = negative_one;
    attributes->field_58 = zero;
    attributes->field_59 = one;
    attributes->field_5A = zero;
    attributes->field_5B = zero;
    attributes->field_5C = zero;
    attributes->field_5D = byte_max;
    attributes->field_5E = negative_one;
    attributes->field_5F = negative_one;
    attributes->field_60 = negative_one;
    attributes->field_61 = zero;
    attributes->field_62 = byte_max;
    attributes->field_63 = byte_max;
    attributes->field_64 = byte_max;
}


int MPV_Init(int handle_count, void* work)
{
    MPVContext* handles;
    u8* after_handles;
    u8* index_work;
    u32 hid2;
    u32 work_address;
    int error;
    int remaining;
    u8* clip;
    u8 value;
    MPVContext* current;

    cri_verstr_ptr = MPVLIB_version_str;
    if ((0x380 & 31) != 0) {
        error = MPVERR_SetCode(0, 0xFF03FF06);
    } else if (MPVVLC_IsVlcSizErr() != 0) {
        error = MPVERR_SetCode(0, 0xFF03FF03);
    } else if (sizeof(MPVContext) > 0x2000) {
        error = MPVERR_SetCode(0, 0xFF03FF01);
    } else if (mpvlib_cond_dfl[16] != 0x5A5A5A5A) {
        error = MPVERR_SetCode(0, 0xFF03FF02);
    } else if (MPVDEC_CheckVersion(mpvlib_checks.version,
                                   0x1378, 0x80) != 0) {
        error = MPVERR_SetCode(0, 0xFF03FF07);
    } else {
        /* The retail build deliberately traps if its endian probe is invalid. */
        if (*(const u8*)&mpvlib_checks.endian_probe != 1) {
            for (;;) {
            }
        }
        error = 0;
    }
    if (error != 0) {
        if (error == 0xFF030005) {
            return error;
        }
        for (;;) {
        }
    }

    hid2 = PPCMfhid2();
    if (mpvlib_use_lc == 0) {
        hid2 &= ~0x10000000;
    }
    mpvlib_libwork.flags = hid2;

    work_address = (u32)work;
    if (mpvlib_libwork.field_44 != 0) {
        work_address |= 0x02000000;
    }
    handles = (MPVContext*)((work_address + 31) & ~31);
    UTY_MemsetDword((unsigned int*)handles, 0,
                    (((u32)handle_count << 13) + 0x2000) >> 2);

    after_handles = (u8*)handles + handle_count * sizeof(MPVContext);
    index_work = after_handles + 0x3A0;
    if ((mpvlib_libwork.flags & 0x10000000) != 0) {
        DCInvalidateRange(index_work, 0x1C60);
        memset(index_work, 0, 0x1C60);
    }

    UTY_MemcpyDword((unsigned int*)mpvlib_libwork.conditions,
                    (const unsigned int*)mpvlib_cond_dfl, 16);
    mpvlib_libwork.field_4C = after_handles;
    mpvlib_libwork.index_work = index_work;
    mpvlib_libwork.handle_count = handle_count;
    mpvlib_libwork.handles = handles;

    MPVERR_Init();
    MPVHDEC_Init();
    MPVFRM_Init();
    MPVVLC_Init(index_work + 0x12B0, (MPVContext*)index_work);
    MPVBDEC_Init(index_work);
    MPVUMC_Init();
    MPVCMC_Init();

    clip = mpv_clip_0_255_tbl;
    remaining = 0x30;
    do {
        clip[0] = 0;
        clip[1] = 0;
        clip[2] = 0;
        clip[3] = 0;
        clip[4] = 0;
        clip[5] = 0;
        clip[6] = 0;
        clip[7] = 0;
        clip += 8;
    } while (--remaining != 0);

    value = 0;
    remaining = 0x20;
    do {
        clip[0] = value++;
        clip[1] = value++;
        clip[2] = value++;
        clip[3] = value++;
        clip[4] = value++;
        clip[5] = value++;
        clip[6] = value++;
        clip[7] = value++;
        clip += 8;
    } while (--remaining != 0);

    remaining = 0x30;
    do {
        clip[0] = 0xFF;
        clip[1] = 0xFF;
        clip[2] = 0xFF;
        clip[3] = 0xFF;
        clip[4] = 0xFF;
        clip[5] = 0xFF;
        clip[6] = 0xFF;
        clip[7] = 0xFF;
        clip += 8;
    } while (--remaining != 0);
    mpv_clip_0_255_base = mpv_clip_0_255_tbl + 0x180;
    if (index_work + 0x1860 != 0) {
        UTY_MemcpyDword((unsigned int*)(index_work + 0x1860),
                        (const unsigned int*)mpv_clip_0_255_tbl, 0x100);
        mpv_clip_0_255_base = index_work + 0x19E0;
    }

    current = handles;
    remaining = handle_count;
    while (remaining-- > 0) {
        current->state = 1;
        current++;
    }

    DCT_FsriInit();
    DCT_FsriInitScaleTbl((float*)(index_work + 0x1160));
    MPVM2V_Init();
    return 0;
}
