#include "cri/sj.h"
#include "movie/sfx_set.h"
#include "runtime/cstring.h"
#include "sofdec/sfd_transport.h"
#include "sofdec/sfx.h"

typedef struct MwsSfxHandleView {
    SfxTagInfo tag_info;
    unsigned char reserved_03C[0x24];
    const char* picture_user_data;
    int picture_user_size;
} MwsSfxHandleView;

typedef struct MwsPlayer {
    unsigned char reserved_000[0x0C];
    int playback_state;
    unsigned char reserved_010[0x10];
    int field_020;
    unsigned char reserved_024[8];
    int field_02C;
    unsigned char reserved_030[0x10];
    SfdHandle* sfd;
    unsigned char reserved_044[0x0C];
    int composition_initialized;
    int composition_mode;
    unsigned char reserved_058[0x34];
    int picture_structure;
    int chroma_format;
    int field_094;
    int field_098;
    int field_09C;
    int chroma_position_0;
    int chroma_position_1;
    unsigned char reserved_0A8[4];
    MwsSfxHandleView* sfx;
    unsigned char reserved_0B0[0xE0];
    SJ* additional_info_sj;
    void* additional_info_buffer;
    int additional_info_buffer_size;
    void* additional_info_copy;
    unsigned char reserved_1A0[4];
    int previous_tag_number;
    int tag_state;
    unsigned char* tag_data;
    int tag_size;
} MwsPlayer;

typedef struct MwsFrameInfo {
    void* frame;
    int frame_structure;
    int width;
    int height;
    unsigned char reserved_010[0x20];
    int presentation_time;
} MwsFrameInfo;

typedef struct MwsYccPlane {
    void* y;
    void* cb;
    void* cr;
    int y_pitch;
    int c_pitch;
    int c_height;
} MwsYccPlane;

typedef struct MwsSfxFrameInfo {
    int format;
    SFXPlaneBuffer y;
    SFXPlaneBuffer cb;
    SFXPlaneBuffer cr;
    unsigned char reserved_034[0x10];
    int frame_width;
    int frame_height;
    int presentation_time;
    int tag_start;
    int tag_size;
    int field_058;
    int field_05C;
    int picture_structure;
    int chroma_format;
    int field_068;
    int field_06C;
    int field_070;
    int chroma_position_0;
    int chroma_position_1;
} MwsSfxFrameInfo;

typedef char MwsSfxTagInfoSizeCheck[
    sizeof(SfxTagInfo) == 0x3C ? 1 : -1];
typedef char MwsSfxHandlePictureUserOffsetCheck[
    (unsigned long)&((MwsSfxHandleView*)0)->picture_user_data == 0x60 ? 1 : -1];
typedef char MwsPlayerSfxOffsetCheck[
    (unsigned long)&((MwsPlayer*)0)->sfx == 0xAC ? 1 : -1];
typedef char MwsPlayerAdditionalInfoOffsetCheck[
    (unsigned long)&((MwsPlayer*)0)->additional_info_sj == 0x190 ? 1 : -1];
typedef char MwsSfxFrameInfoSizeCheck[
    sizeof(MwsSfxFrameInfo) == 0x7C ? 1 : -1];

extern int mwPlyGetFxType(MwsPlayer* player);
extern int MWSFD_IsEnableHndl(MwsPlayer* player);
extern void MWSFSVM_Error(const char* message, ...);
extern void mwSfdDestroy(MwsPlayer* player);
extern void mwPlyCalcYccPlane(void* frame, int width, int height,
                              MwsYccPlane* output);

extern int SFX_GetTypeDivField(MwsSfxHandleView* sfx);
extern int SFX_GetTypeCcs(MwsSfxHandleView* sfx);
extern void SFX_SetPicUsrDat(MwsSfxHandleView* sfx, const char* data,
                             int size);
extern void SFX_Destroy(SFXHandle* handle);
extern SFXHandle* SFX_Create(void* buffer, int buffer_size);
extern void SFX_Finish(void);
extern void SFX_Init(void);
extern void SFX_SetErrFn(SFXErrorCallback callback, void* object);

#pragma force_active on
static const char get_cnv_bottom_up_invalid[0x34] =
    "E404011: mwPlyGetCnvBottomUp: handle is invalid.";
static const char set_cnv_bottom_up_invalid[0x34] =
    "E404010: mwPlySetCnvBottomUp: handle is invalid.";
static const char make_z32_invalid[0x30] =
    "E202285: MWSFD_MakeTblZ32: handle is invalid.";
static const char make_z32_no_frame[0x30] =
    "E202286: MWSFD_MakeTblZ32: getfrm is failed.";
static const char make_z16_invalid[0x30] =
    "E202283: MWSFD_MakeTblZ16: handle is invalid.";
static const char make_z16_no_frame[0x30] =
    "E202284: MWSFD_MakeTblZ16: getfrm is failed.";
static const char sfx_tag_start[8] = "SFXINFS";
static const char sfx_tag_end[8] = "SFXINFE";
static const char attach_additional_short[0x38] =
    "W2121001 : mwPlyAttachAddInfBuf(): bufsize is short.";
static const char cri_tag_start[8] = "CRITAGS";
static const char cri_tag_end[8] = "CRITAGE";
static const char create_additional_failed[0x2C] =
    "E201211 mwPlyCreate: can't create AddInfSJ";
static const char get_z_offset_invalid[0x34] =
    "E2011921: mwPlyFxGetOutZoffset: handle is invalid.";
static const char set_z_offset_invalid[0x34] =
    "E2011920: mwPlyFxSetOutZoffset: handle is invalid.";
static const char get_z_scale_invalid[0x34] =
    "E2011919: mwPlyFxGetOutZscale: handle is invalid.";
static const char set_z_scale_invalid[0x34] =
    "E2011918: mwPlyFxSetOutZscale: handle is invalid.";
static const char set_output_size_invalid[0x34] =
    "E306091: MWSFSFX_SetOutBufSize: handle is invalid.";
static const char get_output_size_invalid[0x3C] =
    "E307092: mwPlyFxGetOutBufPitchHeight: handle is invalid.";
static const char get_composition_invalid[0x34] =
    "E2011915: mwPlyFxGetCompoMode: handle is invalid.";
static const char set_composition_invalid[0x34] =
    "E201214: mwPlyFxSetCompoMode: handle is invalid.";
static const char composition_z_missing[0x48] =
    "E204011: mwPlyFxSetCompoMode: COMPO_Z needs setting in MWPLY Creation.";
static const char composition_auto_missing[0x4C] =
    "E204012: mwPlyFxSetCompoMode: COMPO_AUTO needs setting in MWPLY Creation.";
static const char convert_z32_invalid[0x30] =
    "E2011913: mwPlyFxCnvFrmZ32: handle is invalid.";
static const char convert_z32_no_frame[0x30] =
    "E2011914: mwPlyFxCnvFrmZ32: getfrm is failed.";
static const char convert_z16_invalid[0x30] =
    "E2011911: mwPlyFxCnvFrmZ16: handle is invalid.";
static const char convert_z16_no_frame[0x30] =
    "E2011912: mwPlyFxCnvFrmZ16: getfrm is failed.";
static const char invalid_chroma_position[0x20] =
    "E301274 : chromapos is invalid.";
static const char invalid_chroma_format[0x24] =
    "E301273 : chroma_format is invalid.";
static const char invalid_picture_structure[0x28] =
    "E301272 : picture_structure is invalid.";
static const char invalid_buffer_format[0x28] =
    "E201184 : MwsfdBufFmt value is invalid.";
#pragma force_active off

void MWSFSFX_DecideCompoMode(MwsPlayer* player)
{
    int type;

    if (player->composition_initialized == 0) {
        type = mwPlyGetFxType(player);
        if (type != -1) {
            player->composition_mode = type;
        } else {
            player->composition_mode = 0x11;
        }
    }
    SFX_SetCompoMode(&player->sfx->tag_info, player->composition_mode);
}

int MWSFD_IsFrmDivField(MwsPlayer* player)
{
    return SFX_GetTypeDivField(player->sfx);
}

int MWSFSFX_IsFrmCcs(MwsPlayer* player)
{
    return SFX_GetTypeCcs(player->sfx);
}

void MWSFSFX_SetPicUsrDat(MwsPlayer* player, const char* data, int size)
{
    SFX_SetPicUsrDat(player->sfx, data, size);
}

#pragma dont_inline on
static void mwsftag_GetAinfFromSj(MwsPlayer* player)
{
    SJCK source;
    SJCK result;
    SJCK consumed;
    SJ* sj = player->additional_info_sj;
    int num_data;
    int stopped;

    num_data = sj->interface->get_num_data(sj, 1);
    if (num_data == 0) {
        player->tag_data = 0;
        player->tag_size = 0;
        player->tag_state = 1;
        return;
    }
    source.data = player->additional_info_buffer;
    source.len = num_data;
    if (SJ_SearchTag(&source, cri_tag_start, cri_tag_end, &result) == 0) {
        player->tag_data = 0;
        player->tag_size = 0;
        player->tag_state = 1;
        return;
    }

    if (player->additional_info_copy != 0) {
        memcpy(player->additional_info_copy, result.data, result.len);
        player->tag_data = player->additional_info_copy;
        player->tag_size = result.len;
        player->tag_state = 1;
        sj->interface->get_chunk(sj, 1, 0x7FFFFFFF, &consumed);
        sj->interface->put_chunk(sj, 0, &consumed);
        sj->interface->reset(sj);
        return;
    }

    player->tag_data = result.data;
    player->tag_size = result.len;
    player->tag_state = 1;
    if (player->playback_state == 2) {
        stopped = 1;
    } else {
        stopped = 0;
    }
    if (stopped != 1 && player->additional_info_sj != 0) {
        SFD_SetUsrSj(player->sfd, 2, 0, 0);
    }
}
#pragma dont_inline reset

void MWSFTAG_UpdateTagInf(MwsPlayer* player)
{
    SJCK result;
    SJCK source;
    MwsSfxHandleView* sfx;
    unsigned char* data;

    if (player->additional_info_sj != 0) {
        mwsftag_GetAinfFromSj(player);
        data = player->tag_data;
        sfx = player->sfx;
        if (data == 0) {
            SFX_SetTagInf(&sfx->tag_info, 0, 0);
            return;
        }
        source.data = data;
        source.len = player->tag_size;
        if (SJ_SearchTag(&source, sfx_tag_start, sfx_tag_end, &result) == 0) {
            SFX_SetTagInf(&sfx->tag_info, 0, 0);
            return;
        }
        SFX_SetTagInf(&sfx->tag_info, result.data, result.len);
    }
}

void MWSFTAG_ResetAinfSj(MwsPlayer* player)
{
    SJ* sj = player->additional_info_sj;

    if (sj != 0) {
        sj->interface->reset(sj);
    }
}

void MWSFTAG_InitTagInf(MwsPlayer* player)
{
    player->tag_state = 0;
    player->tag_data = 0;
    player->tag_size = 0;
    player->previous_tag_number = -1;
}

int MWSFTAG_SetAinfSj(MwsPlayer* player)
{
    int result;
    int stopped;

    if (player->playback_state == 2) {
        stopped = 1;
    } else {
        stopped = 0;
    }
    if (stopped == 1) {
        return 0;
    }
    if (player->additional_info_sj == 0) {
        return 0;
    }
    result = SFD_SetUsrSj(player->sfd, 2, player->additional_info_sj, 0);
    return -(result != 0);
}

void MWSFTAG_DestroyAinfSj(MwsPlayer* player)
{
    SJ* sj = player->additional_info_sj;

    if (sj != 0) {
        sj->interface->destroy(sj);
    }
}

SJ* MWSFTAG_CreateAinfSj(MwsPlayer* player)
{
    SJ* sj;
    int use_format;

    if (player->field_02C == 0 || player->field_02C == 0x101) {
        use_format = 1;
    } else {
        use_format = 0;
    }
    if (use_format == 0) {
        return 0;
    }
    sj = SJRBF_Create(player->additional_info_buffer,
                      player->additional_info_buffer_size, 0);
    if (sj == 0) {
        MWSFSVM_Error(create_additional_failed);
        mwSfdDestroy(player);
        return 0;
    }
    return sj;
}

int MWSFTAG_IsUseAinfSj(MwsPlayer* player)
{
    if (player->field_020 == 0 || player->field_020 == 0x101) {
        return 1;
    }
    return 0;
}

void mwPlyFxSetOutBufPitchHeight(MwsPlayer* player, int pitch, int height)
{
    MwsSfxHandleView* sfx;

    if (MWSFD_IsEnableHndl(player) == 0) {
        MWSFSVM_Error(set_output_size_invalid);
        return;
    }
    sfx = player->sfx;
    SFX_SetOutBufSize(&sfx->tag_info, pitch, height);
    SFX_SetUnitWidth(&sfx->tag_info, 0);
}

void MWSFD_SetColAdj(MwsPlayer* player, int adjustment)
{
    SFX_SetColAdj(&player->sfx->tag_info, adjustment);
}

void MWSFSFX_SetColAdj(MwsPlayer* player, int adjustment)
{
    SFX_SetColAdj(&player->sfx->tag_info, adjustment);
}

void MWSFSFX_SetFxType(MwsPlayer* player, int type)
{
    SFX_SetFxType(&player->sfx->tag_info, type);
}

void MWSFSFX_SetCompoMode(MwsPlayer* player, int mode)
{
    SFX_SetCompoMode(&player->sfx->tag_info, mode);
}

void MWSFSFX_CnvFrmInfToSfx(MwsPlayer* player, MwsFrameInfo* input,
                            MwsSfxFrameInfo* output)
{
    int tag_size;
    int tag_start;
    MwsYccPlane plane;
    const char* errors = get_cnv_bottom_up_invalid;
    int converted;
    int width;
    int height;

    switch (input->frame_structure) {
    case 1:
        converted = 1;
        break;
    case 2:
        converted = 2;
        break;
    case 3:
        converted = 3;
        break;
    default:
        MWSFSVM_Error(errors + 0x514);
        converted = 3;
        break;
    }
    output->format = converted;

    width = input->width;
    height = input->height;
    output->frame_width = width;
    output->frame_height = height;
    if (input->frame_structure != 3) {
        output->y.pixels = input->frame;
        output->y.width = width;
        output->y.height = height;
    } else {
        mwPlyCalcYccPlane(input->frame, width, height, &plane);
        output->y.pixels = plane.y;
        output->y.width = plane.y_pitch;
        output->y.height = height;
        output->cb.pixels = plane.cb;
        output->cb.width = plane.c_pitch;
        output->cb.height = height;
        output->cr.pixels = plane.cr;
        output->cr.width = plane.c_height;
        output->cr.height = height;
    }

    output->presentation_time = input->presentation_time;
    SFX_GetTagInf(&player->sfx->tag_info, &tag_start, &tag_size);
    output->tag_start = tag_start;
    output->tag_size = tag_size;
    output->field_058 = 0;
    output->field_05C = 0;

    converted = 3;
    switch (player->picture_structure) {
    case 1:
        converted = 1;
        break;
    case 2:
        converted = 2;
        break;
    case 3:
        converted = 3;
        break;
    default:
        MWSFSVM_Error(errors + 0x4EC);
        break;
    }
    output->picture_structure = converted;

    converted = 1;
    switch (player->chroma_format) {
    case 1:
        converted = 1;
        break;
    case 2:
        converted = 2;
        break;
    case 3:
        converted = 3;
        break;
    default:
        MWSFSVM_Error(errors + 0x4C8);
        break;
    }
    output->chroma_format = converted;

    output->field_068 = player->field_094;
    output->field_06C = player->field_098;
    output->field_070 = player->field_09C;
    converted = 1;
    switch (player->chroma_position_0) {
    case 0:
        converted = 0;
        break;
    case 1:
        converted = 1;
        break;
    default:
        MWSFSVM_Error(errors + 0x4A8);
        break;
    }
    output->chroma_position_0 = converted;

    converted = 1;
    switch (player->chroma_position_1) {
    case 0:
        converted = 0;
        break;
    case 1:
        converted = 1;
        break;
    default:
        MWSFSVM_Error(errors + 0x4A8);
        break;
    }
    output->chroma_position_1 = converted;
}

SFXHandle* MWSFSFX_GetSfxHn(MwsPlayer* player)
{
    return (SFXHandle*)player->sfx;
}

void MWSFSFX_Destroy(SFXHandle* sfx)
{
    SFX_Destroy(sfx);
}

SFXHandle* MWSFSFX_Create(void* buffer, int buffer_size,
                          int width, int height)
{
    return SFX_Create(buffer, buffer_size);
}

int MWSFSFX_CalcHnWorkSiz(void)
{
    return 0x301F;
}

void MWSFSFX_Finish(void)
{
    SFX_Finish();
}

static void mwsfsfx_SfxErrCbFn(void* object, const char* message)
{
    MWSFSVM_Error(message);
}

void MWSFSFX_Init(void)
{
    SFX_Init();
    SFX_SetErrFn(mwsfsfx_SfxErrCbFn, 0);
}

const int gap_04_80319014_rodata = 0;
