#include "sofdec/sfd_transport.h"

typedef struct MwsSfhInfo {
    int valid;
    int header_number;
    int color_adjustment;
    int maximum_frames;
    int effect_type;
} MwsSfhInfo;

typedef struct MwsPlayer {
    unsigned char reserved_000[0x1C];
    int skipped_display_frames;
    unsigned char reserved_020[0x20];
    SfdHandle* sfd;
    unsigned char reserved_044[0x14];
    int frame_sync;
    unsigned char reserved_05C[0x20];
    void* current_frame;
    int acquired_frames;
    int released_frames;
    int skipped_frames;
    int frame_field_58;
    int frame_field_5C;
    int frame_field_6C;
    int frame_field_6D;
    int frame_field_6E;
    int frame_field_3C;
    int frame_field_40;
    int frame_field_A8;
    unsigned char reserved_0AC[0x0C];
    int header_count;
    int current_header;
    int next_header;
    MwsSfhInfo headers[8];
    unsigned char picture_user_internal[0x18];
    void* picture_user_source;
    void* picture_user_buffer;
    int picture_user_capacity;
    void* picture_user_data;
    int picture_user_size;
    unsigned char reserved_190[0x14];
    int previous_picture_order;
} MwsPlayer;

typedef struct MwsFrameOutput {
    void* frame;
    int frame_structure;
    int width;
    int height;
    int macroblocks_per_row;
    int macroblock_rows;
    int picture_type;
    int frame_rate;
    int display_time;
    int display_time_source;
    int display_scale;
    int picture_order;
    int presentation_time;
    int presentation_source;
    int field_38;
    int field_3C;
    void* picture_user_data;
    int picture_user_size;
    int display_mode;
    int reserved_4C;
    unsigned char transport_fields[0x38];
} MwsFrameOutput;

typedef struct MwsYccPlane {
    void* y;
    void* cb;
    void* cr;
    int y_pitch;
    int c_pitch;
    int c_height;
} MwsYccPlane;

typedef struct SfdCalculatedPlane {
    void* cb;
    void* cr;
    void* y;
    short c_pitch;
    short y_pitch;
} SfdCalculatedPlane;

typedef char MwsSfhInfoSizeCheck[sizeof(MwsSfhInfo) == 0x14 ? 1 : -1];
typedef char MwsPlayerHeaderOffsetCheck[
    (unsigned long)&((MwsPlayer*)0)->headers == 0xC4 ? 1 : -1];
typedef char MwsPlayerPictureUserOffsetCheck[
    (unsigned long)&((MwsPlayer*)0)->picture_user_internal == 0x164 ? 1 : -1];
typedef char MwsPlayerPreviousOrderOffsetCheck[
    (unsigned long)&((MwsPlayer*)0)->previous_picture_order == 0x1A4 ? 1 : -1];
typedef char MwsYccPlaneSizeCheck[sizeof(MwsYccPlane) == 0x18 ? 1 : -1];

extern int MWSFD_IsEnableHndl(MwsPlayer* player);
extern void MWSFSVM_Error(const char* message, ...);
extern SfdHandle* mwPlyGetSfdHn(MwsPlayer* player);
extern int SFD_GetFrm(SfdHandle* handle, void** frame);
extern void SFD_RelFrm(SfdHandle* handle, void* frame);
extern int SFD_SetCond(SfdHandle* handle, int condition,
                       SfdConditionValue value);
extern void MWSFSFX_SetColAdj(MwsPlayer* player, int adjustment);
extern void SFD_CalcYccPlane(int width, int height, int format,
                            SfdCalculatedPlane* plane);
extern void* SFH_Create(const void* data, int size);
extern void SFH_Destroy(void* header);
extern int SFH_IsSfdHeader(void* header, int* result);
extern int SFH_IsExistStmId(void* header, int stream_id, int* result);
extern int SFH_AnlyFtrColType(void* header, int stream_id, int* type);
extern int SFH_AnlyMaxFrmNum(void* header, int* count);
extern int SFH_AnlyFtrFxType(void* header, int stream_id,
                            unsigned int* type);
extern int MWSFD_GetUsePicUsr(void);
extern int SFD_GetFps(SfdHandle* handle, int* frame_rate);
extern int UTY_MulDiv(int value, int multiplier, int divisor);
extern void* memcpy(void* destination, const void* source,
                    unsigned long size);
extern void* memset(void* destination, int value, unsigned long size);
extern int SFD_IsNextFrmReady(SfdHandle* handle);
extern int SUD_SearchSudDat(const void* data, int size, void** result,
                           int* result_size);
extern void MWSFSFX_SetPicUsrDat(MwsPlayer* player, void* data, int size);
extern int MWSFSFX_IsFrmCcs(MwsPlayer* player);
extern int MWSFD_IsFrmDivField(MwsPlayer* player);
extern void MWSFD_SetColAdj(MwsPlayer* player, int adjustment);
extern void MWSFSFX_SetFxType(MwsPlayer* player, int type);
extern void MWSFTAG_UpdateTagInf(MwsPlayer* player);

#pragma force_active on
static const char get_skip_invalid[0x34] =
    "E202231: mwPlyGetNumSkipDisp: handle is invalid.";
static const char get_user_invalid[0x34] =
    "E3122201: mwPlyGetNextPicUsr: handle is invalid.";
static const char next_frame_invalid[0x34] =
    "E1122618: mwPlyIsNextFrmReady: handle is invalid.";
static const char release_frame_invalid[0x30] =
    "E1122615: mwPlyRelCurFrm: handle is invalid.";
static const char to_sfd_picture_type[0x24] =
    "mwl_convPtypeToSFD : Invalid Ptype";
static const char decide_frame_type[0x34] =
    "E301271: mwsffrm_DecideFrmType() : Invalid Pstruct";
static const char from_sfd_picture_type[0x28] =
    "mwl_convPtypeFromSFD : Invalid Ptype";
static const char get_fps_failed[0x20] =
    "E201301: MWSFD: GetFps failed.";
static const char get_cur_frame_invalid[0x30] =
    "E1122614: mwPlyGetCurFrm: handle is invalid.";
static const char get_sync_invalid[0x30] =
    "E2010801: mwPlyGetFrmSync: handle is invalid.";
static const char set_sync_invalid[0x34] =
    "E1122629: mwPlySetFrmSync: handle is invalid.";
#pragma force_active off

static void mwsffrm_AnalySofdecHeader(MwsPlayer* player,
                                      const void* data, unsigned int size)
{
    void* header;
    int is_header;
    int stream_exists;
    int color_type;
    int maximum_frames;
    unsigned int source_effect;
    int color_adjustment;
    int effect_type;
    MwsSfhInfo* info;

    player->header_count++;
    if (size < 0x800 || data == 0) {
        return;
    }
    header = SFH_Create(data, size);
    if (header == 0) {
        return;
    }
    if (SFH_IsSfdHeader(header, &is_header) == 0 || is_header == 0) {
        SFH_Destroy(header);
        return;
    }

    color_adjustment = 0;
    if (SFH_IsExistStmId(header, 0xE0, &stream_exists) == 0 ||
        stream_exists == 0) {
        color_adjustment = 0;
    } else if (SFH_AnlyFtrColType(header, 0xE0, &color_type) == 0) {
        color_adjustment = 0;
    } else if (color_type == 3) {
        color_adjustment = 1;
    }
    if (SFH_AnlyMaxFrmNum(header, &maximum_frames) == 0) {
        maximum_frames = -1;
    }
    if (SFH_AnlyFtrFxType(header, 0xE0, &source_effect) == 0) {
        effect_type = 0x11;
    } else {
        switch (source_effect) {
        case 1:
            effect_type = 0x21;
            break;
        case 3:
            effect_type = 0x51;
            break;
        case 6:
            effect_type = 0x61;
            break;
        case 0:
        case 2:
        case 4:
        case 5:
        case 7:
        case 8:
        default:
            effect_type = 0x11;
            break;
        }
    }

    info = &player->headers[player->next_header];
    info->header_number = player->header_count - 1;
    info->color_adjustment = color_adjustment;
    info->maximum_frames = maximum_frames;
    info->effect_type = effect_type;
    info->valid = 1;
    player->next_header++;
    player->next_header %= 8;
    SFH_Destroy(header);
}

#pragma explicit_zero_data on
int gap_05_803A8A84_data = 0;
#pragma explicit_zero_data off

void MWSFFRM_SetShfCbFn(MwsPlayer* player)
{
    SfdHandle* sfd = player->sfd;

    SFD_SetCond(sfd, 0x4B,
                (SfdConditionValue)mwsffrm_AnalySofdecHeader);
    SFD_SetCond(sfd, 0x4C, (SfdConditionValue)player);
}

void MWSFFRM_InitSfhInfTable(MwsPlayer* player)
{
    int zero = 0;

    player->header_count = zero;
    player->current_header = zero;
    player->next_header = zero;
#define INIT_HEADER(index)                                                   \
    player->headers[index].valid = zero;                                    \
    player->headers[index].header_number = zero;                            \
    player->headers[index].color_adjustment = zero;                         \
    player->headers[index].maximum_frames = zero;                           \
    player->headers[index].effect_type = 0x11
    INIT_HEADER(0);
    INIT_HEADER(1);
    INIT_HEADER(2);
    INIT_HEADER(3);
    INIT_HEADER(4);
    INIT_HEADER(5);
    INIT_HEADER(6);
    INIT_HEADER(7);
#undef INIT_HEADER
    MWSFSFX_SetColAdj(player, zero);
}

int mwPlyGetFxType(MwsPlayer* player)
{
    MwsSfhInfo* info = &player->headers[player->current_header % 8];
    int type = info->valid == 0 ? 0x11 : info->effect_type;

    if (type == 0x51 || type == 0x61) {
        type = 0x41;
    }
    return type;
}

void mwPlyRelCurFrm(MwsPlayer* player)
{
    SfdHandle* sfd;
    void* frame;
    int acquired;
    int released;

    if (MWSFD_IsEnableHndl(player) == 0) {
        MWSFSVM_Error(release_frame_invalid);
        return;
    }
    frame = player->current_frame;
    acquired = player->acquired_frames;
    released = player->released_frames;
    sfd = mwPlyGetSfdHn(player);
    if (acquired > released) {
        SFD_RelFrm(sfd, frame);
        player->released_frames++;
        player->acquired_frames = player->released_frames;
    }
}

void mwPlyCalcYccPlane(int width, int height, int format,
                       MwsYccPlane* output)
{
    SfdCalculatedPlane plane;

    SFD_CalcYccPlane(width, height, format, &plane);
    output->y = plane.y;
    output->cb = plane.cb;
    output->cr = plane.cr;
    output->y_pitch = plane.y_pitch;
    output->c_pitch = plane.c_pitch;
    output->c_height = plane.c_pitch;
}

void mwl_convFrmInfFromSFD(MwsPlayer* player, SfdVideoFrameInfo* source,
                           void* output_pointer)
{
    MwsFrameOutput* output = output_pointer;
    int frame_structure;
    int picture_type;
    int frame_rate;
    int divisor;
    int display_time;
    int presentation_time;
    int width;
    int height;
    int macroblocks_per_row;
    int macroblock_rows;
    int display_time_source;
    int display_scale;
    int picture_order;
    int presentation_source;
    int field_38;
    int field_3C;
    void* frame;
    SfdHandle* sfd;
    void** picture_user;
    void* picture_user_data;
    int picture_user_size;
    void* internal_picture_user;
    void* configured_picture_user;

    sfd = player->sfd;
    frame = source->frame_buffer;
    switch (source->output_format) {
    case 1:
        frame_structure = 1;
        break;
    case 2:
        frame_structure = 2;
        break;
    case 3:
        frame_structure = 3;
        break;
    default:
        frame_structure = 3;
        break;
    }
    switch (source->picture_type) {
    case 1:
        picture_type = 1;
        break;
    case 2:
        picture_type = 2;
        break;
    case 3:
        picture_type = 3;
        break;
    case 4:
        picture_type = 4;
        break;
    default:
        MWSFSVM_Error(from_sfd_picture_type);
        picture_type = 1;
        break;
    }
    width = source->width;
    height = source->height;
    macroblocks_per_row = source->macroblocks_per_row;
    macroblock_rows = source->macroblock_rows;
    display_time_source = source->field_34;
    display_scale = source->display_time_scale;
    presentation_source = source->field_30;
    picture_order = source->picture_order;
    field_38 = source->field_24;
    field_3C = source->field_28;
    if (SFD_GetFps(sfd, &frame_rate) != 0) {
        MWSFSVM_Error(get_fps_failed);
    }
    divisor = display_scale * 1000;
    display_time = UTY_MulDiv(display_time_source, frame_rate, divisor);
    presentation_time =
        UTY_MulDiv(presentation_source, frame_rate, divisor);

    output->frame = frame;
    output->frame_structure = frame_structure;
    output->width = width;
    output->height = height;
    output->macroblocks_per_row = macroblocks_per_row;
    output->macroblock_rows = macroblock_rows;
    output->picture_type = picture_type;
    output->frame_rate = frame_rate;
    output->display_time = display_time;
    output->display_time_source = display_time_source;
    output->display_scale = display_scale;
    output->picture_order = picture_order;
    output->presentation_time = presentation_time;
    output->presentation_source = presentation_source;
    output->field_38 = field_38;
    output->field_3C = field_3C;

    internal_picture_user = player->picture_user_internal;
    configured_picture_user = player->picture_user_source;
    picture_user = (void**)source->picture_user_buffer;
    picture_user_data = picture_user[0];
    picture_user_size = (int)picture_user[1];
    if (MWSFD_GetUsePicUsr() != 1) {
        output->picture_user_data = 0;
        output->picture_user_size = 0;
    } else if (configured_picture_user == internal_picture_user) {
        output->picture_user_data = 0;
        output->picture_user_size = 0;
    } else {
        if (picture_user_data != 0 && picture_user_size > 4) {
            picture_user_data = (unsigned char*)picture_user_data + 4;
            picture_user_size -= 4;
        }
        output->picture_user_data = picture_user_data;
        output->picture_user_size = picture_user_size;
    }
    memcpy(output->transport_fields, &source->display_mode, 0x38);
}

void mwPlyGetCurFrm(MwsPlayer* player, void* output)
{
    MwsFrameOutput* frame_output = output;
    SfdVideoFrameInfo* frame;
    SfdHandle* sfd;
    void** picture_user;
    void* user_data;
    int user_size;
    int index;
    int color_adjustment;
    int display_mode;
    int effect_type;
    MwsSfhInfo* info;
    const char* errors = get_skip_invalid;

    if (MWSFD_IsEnableHndl(player) == 0) {
        MWSFSVM_Error(errors + 0x16C);
        frame_output->frame = 0;
        return;
    }
    sfd = mwPlyGetSfdHn(player);
    SFD_GetFrm(sfd, (void**)&frame);
    if (frame != 0 && player->frame_sync == 0) {
        for (index = 0; index < player->skipped_display_frames; index++) {
            if (MWSFD_IsEnableHndl(player) == 0) {
                MWSFSVM_Error(errors + 0x68);
                break;
            }
            mwPlyGetSfdHn(player);
            if (SFD_IsNextFrmReady(sfd) != 1) {
                break;
            }
            SFD_RelFrm(sfd, frame);
            player->skipped_frames++;
            SFD_GetFrm(sfd, (void**)&frame);
        }
    }
    if (frame == 0) {
        frame_output->frame = 0;
        return;
    }

    player->acquired_frames++;
    player->current_frame = frame;
    player->frame_field_58 = frame->field_58;
    player->frame_field_5C = frame->field_5C;
    player->frame_field_6C = (signed char)frame->fields_6C[0];
    player->frame_field_6D = (signed char)frame->fields_6C[1];
    player->frame_field_6E = (signed char)frame->fields_6C[2];
    player->frame_field_3C = frame->field_3C;
    player->frame_field_40 = frame->field_40;
    player->frame_field_A8 = 0;
    mwl_convFrmInfFromSFD(player, frame, frame_output);

    picture_user = (void**)frame->picture_user_buffer;
    user_data = picture_user[0];
    user_size = (int)picture_user[1];
    if (MWSFD_GetUsePicUsr() == 1 && player->picture_user_buffer != 0) {
        if (user_data != 0 && user_size > 4) {
            SUD_SearchSudDat((unsigned char*)user_data + 4, user_size - 4,
                             &user_data, &user_size);
        } else {
            user_data = 0;
            user_size = 0;
        }
        if (user_data != 0 && user_size > 0) {
            if (user_size > player->picture_user_capacity) {
                user_size = player->picture_user_capacity;
            }
            memset(player->picture_user_buffer, 0,
                   player->picture_user_capacity);
            memcpy(player->picture_user_buffer, user_data, user_size);
            player->picture_user_data = player->picture_user_buffer;
            player->picture_user_size = user_size;
        } else {
            player->picture_user_data = 0;
            player->picture_user_size = 0;
        }
        MWSFSFX_SetPicUsrDat(player, player->picture_user_data,
                            player->picture_user_size);
    }

    if (player->previous_picture_order < frame_output->picture_order) {
        MWSFTAG_UpdateTagInf(player);
    }
    player->previous_picture_order = frame_output->picture_order;

    info = &player->headers[player->current_header % 8];
    color_adjustment = info->valid == 0 ? 0 : info->color_adjustment;
    if (player->picture_user_data != 0) {
        color_adjustment = MWSFSFX_IsFrmCcs(player) == 1;
    }
    MWSFD_SetColAdj(player, color_adjustment);

    display_mode = 0;
    if (frame->field_58 == 1 || frame->field_58 == 2) {
        display_mode = 2;
    } else if (frame->field_58 == 3) {
        if ((signed char)frame->fields_6C[0] == 0) {
            display_mode = 2;
        }
    } else {
        MWSFSVM_Error(errors + 0xF0);
    }
    if (MWSFD_GetUsePicUsr() == 1 && MWSFD_IsFrmDivField(player) == 1) {
        display_mode = 2;
    }
    frame_output->display_mode = display_mode;

    player->current_header = frame_output->picture_order;
    info = &player->headers[player->current_header % 8];
    effect_type = info->valid == 0 ? 0x11 : info->effect_type;
    MWSFSFX_SetFxType(player, effect_type);
}

void mwPlySetFrmSync(MwsPlayer* player, int sync)
{
    if (MWSFD_IsEnableHndl(player) == 0) {
        MWSFSVM_Error(set_sync_invalid);
        return;
    }
    player->frame_sync = sync;
}
