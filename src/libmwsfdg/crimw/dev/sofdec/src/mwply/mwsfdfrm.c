#include "sofdec/sfd_player.h"
#include "sofdec/uty_math.h"
#include "runtime/cstring.h"
#include "sofdec/mws_ycc.h"

#include "runtime/asm_sequences.inc"

typedef struct SFHHandle SFHHandle;

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

typedef struct SfdCalculatedPlane {
    void* cb;
    void* cr;
    void* y;
    short c_pitch;
    short y_pitch;
} SfdCalculatedPlane;

typedef struct MwsPictureUserData {
    void* data;
    int size;
} MwsPictureUserData;

typedef char MwsSfhInfoSizeCheck[sizeof(MwsSfhInfo) == 0x14 ? 1 : -1];
typedef char MwsPlayerHeaderOffsetCheck[
    (unsigned long)&((MwsPlayer*)0)->headers == 0xC4 ? 1 : -1];
typedef char MwsPlayerPictureUserOffsetCheck[
    (unsigned long)&((MwsPlayer*)0)->picture_user_internal == 0x164 ? 1 : -1];
typedef char MwsPlayerPreviousOrderOffsetCheck[
    (unsigned long)&((MwsPlayer*)0)->previous_picture_order == 0x1A4 ? 1 : -1];

extern int MWSFD_IsEnableHndl(MwsPlayer* player);
extern void MWSFSVM_Error(const char* message, ...);
extern SfdHandle* mwPlyGetSfdHn(MwsPlayer* player);
extern void MWSFSFX_SetColAdj(MwsPlayer* player, int adjustment);
extern void SFD_CalcYccPlane(void* buffer, int width, int height,
                            SfdCalculatedPlane* plane);
extern SFHHandle* SFH_Create(const void* data, int size);
extern void SFH_Destroy(SFHHandle* header);
extern int SFH_IsSfdHeader(SFHHandle* header, int* result);
extern int SFH_IsExistStmId(SFHHandle* header, unsigned char stream_id,
                            int* result);
extern int SFH_AnlyFtrColType(SFHHandle* header, unsigned char stream_id,
                              int* type);
extern int SFH_AnlyMaxFrmNum(SFHHandle* header, int* count);
extern int SFH_AnlyFtrFxType(SFHHandle* header, unsigned char stream_id,
                            int* type);
extern int MWSFD_GetUsePicUsr(void);
extern int SFD_IsNextFrmReady(SfdHandle* handle);
extern int SUD_SearchSudDat(const void* data, int size, void** result,
                           int* result_size);
extern void MWSFSFX_SetPicUsrDat(MwsPlayer* player, const char* data, int size);
extern int MWSFSFX_IsFrmCcs(MwsPlayer* player);
extern int MWSFD_IsFrmDivField(MwsPlayer* player);
extern void MWSFD_SetColAdj(MwsPlayer* player, int adjustment);
extern void MWSFSFX_SetFxType(MwsPlayer* player, int type);
extern void MWSFTAG_UpdateTagInf(MwsPlayer* player);

static inline int mwsffrm_IsCurCcs(MwsPlayer* player)
{
    int index = player->current_header % 8;
    int adjustment;

    if (player->headers[index].valid == 0) {
        adjustment = 0;
    } else {
        adjustment = player->headers[index].color_adjustment;
    }
    return adjustment;
}

static inline int mwsffrm_GetCurFxType(MwsPlayer* player)
{
    int index = player->current_header % 8;
    int type;

    if (player->headers[index].valid == 0) {
        type = 0x11;
    } else {
        type = player->headers[index].effect_type;
    }
    return type;
}

static inline int mwsffrm_IsPicUsrDat(MwsPlayer* player)
{
    return player->picture_user_data != 0 ? 1 : 0;
}

static void mwsffrm_AnalySofdecHeader(MwsPlayer* player,
                                      const void* data, unsigned int size);
static void* mwsffrm_AnalySofdecHeader_jumptable[] =
    SEQ_mwsffrm_AnalySofdecHeader_jumptable;

static asm void mwsffrm_AnalySofdecHeader(MwsPlayer* player, const void* data,
                                          unsigned int size)
{
    SEQ_mwsffrm_AnalySofdecHeader();
}

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
    int index = player->current_header % 8;
    int type = player->headers[index].valid == 0
                   ? 0x11 : player->headers[index].effect_type;

    if (type == 0x51 || type == 0x61) {
        type = 0x41;
    }
    return type;
}

/* TODO: [provisional] linker-stripped (MAP UNUSED 0x50); RE4 mwsfdfrm.c body ported, compiles to 0x50. */
int mwPlyGetNumSkipDisp(MwsPlayer* player)
{
    if (MWSFD_IsEnableHndl(player) == 0) {
        MWSFSVM_Error("E202231: mwPlyGetNumSkipDisp: handle is invalid.");
        return 0;
    }
    return player->skipped_frames;
}

/* TODO: [provisional] linker-stripped (MAP UNUSED 0xA0); RE4 mwsfdfrm.c body ported, compiles to 0x64 (body incomplete). */
void* mwPlyGetNextPicUsr(MwsPlayer* player, int* size)
{
    if (MWSFD_IsEnableHndl(player) == 0) {
        MWSFSVM_Error("E3122201: mwPlyGetNextPicUsr: handle is invalid.");
        return 0;
    }
    *size = player->picture_user_size;
    return player->picture_user_data;
}

/* TODO: [provisional] linker-stripped (MAP UNUSED 0x58); RE4 mwsfdfrm.c body ported, compiles to 0x58. */
int mwPlyIsNextFrmReady(MwsPlayer* player)
{
    if (MWSFD_IsEnableHndl(player) == 0) {
        MWSFSVM_Error("E1122618: mwPlyIsNextFrmReady: handle is invalid.");
        return 0;
    }
    return SFD_IsNextFrmReady(mwPlyGetSfdHn(player));
}

void mwPlyRelCurFrm(MwsPlayer* player)
{
    SfdHandle* sfd;
    void* frame;
    int acquired;
    int released;

    if (MWSFD_IsEnableHndl(player) == 0) {
        MWSFSVM_Error("E1122615: mwPlyRelCurFrm: handle is invalid.");
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

/* TODO: [provisional] linker-stripped (MAP UNUSED 0x78); RE4 mwsfdfrm.c body plus a case 4 arm, compiles to 0x78. */
int mwl_convPtypeToSFD(int type)
{
    int result;

    switch (type) {
    case 1:
        result = 1;
        break;
    case 2:
        result = 2;
        break;
    case 3:
        result = 3;
        break;
    case 4:
        result = 4;
        break;
    default:
        MWSFSVM_Error("mwl_convPtypeToSFD : Invalid Ptype");
        result = 3;
        break;
    }
    return result;
}

void mwPlyCalcYccPlane(void* buffer, int width, int height,
                       MwsYccPlane* output)
{
    SfdCalculatedPlane plane;

    SFD_CalcYccPlane(buffer, width, height, &plane);
    output->y = plane.y;
    output->cb = plane.cb;
    output->cr = plane.cr;
    output->y_pitch = plane.y_pitch;
    output->cb_pitch = plane.c_pitch;
    output->cr_pitch = plane.c_pitch;
}

/* TODO: [provisional] linker-stripped (MAP UNUSED 0xA4); body of mwPlyGetCurFrm's frame-type block, compiles to 0xA4. */
static int mwsffrm_DecideFrmType(MwsPlayer* player, SfdVideoFrameInfo* frame)
{
    int result = 0;

    switch (frame->field_58) {
    case 3:
        if ((signed char)frame->fields_6C[0] == 0) {
            result = 2;
        }
        break;
    case 1:
    case 2:
        result = 2;
        break;
    case 0:
    default:
        MWSFSVM_Error("E301271: mwsffrm_DecideFrmType() : Invalid Pstruct");
        break;
    }
    if (MWSFD_GetUsePicUsr() == 1 && MWSFD_IsFrmDivField(player) == 1) {
        result = 2;
    }
    return result;
}

/* TODO: [provisional] linker-stripped (MAP UNUSED 0x78); RE4 mwsfdfrm.c body ported, compiles to 0x78. */
static int mwl_convPtypeFromSFD(int type)
{
    int result;

    switch (type) {
    case 1:
        result = 1;
        break;
    case 2:
        result = 2;
        break;
    case 3:
        result = 3;
        break;
    case 4:
        result = 4;
        break;
    default:
        MWSFSVM_Error("mwl_convPtypeFromSFD : Invalid Ptype");
        result = 1;
        break;
    }
    return result;
}

void mwl_convFrmInfFromSFD(MwsPlayer* player, SfdVideoFrameInfo* source,
                           void* output_pointer)
{
    int display_time;
    int frame_structure;
    int picture_type;
    SfdHandle* sfd;
    int divisor;
    void* frame;
    int frame_rate;
    int presentation_time;
    void* configured_picture_user;
    int picture_user_size;
    MwsPictureUserData* picture_user;
    void* internal_picture_user;
    void* picture_user_data;
    int width;
    int height;
    int macroblocks_per_row;
    int macroblock_rows;
    int display_time_source;
    int display_scale;
    int presentation_source;
    int field_38;
    int field_3C;
    int picture_order;
    MwsFrameOutput* output = output_pointer;

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
    width = source->width;
    height = source->height;
    macroblocks_per_row = source->macroblocks_per_row;
    macroblock_rows = source->macroblock_rows;
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
        MWSFSVM_Error("mwl_convPtypeFromSFD : Invalid Ptype");
        picture_type = 1;
        break;
    }
    display_time_source = source->field_34;
    display_scale = source->display_time_scale;
    presentation_source = source->field_30;
    picture_order = source->picture_order;
    field_38 = source->field_24;
    field_3C = source->field_28;
    if (SFD_GetFps(sfd, &frame_rate) != 0) {
        MWSFSVM_Error("E201301: MWSFD: GetFps failed.");
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
    picture_user = source->picture_user_buffer;
    picture_user_data = picture_user->data;
    picture_user_size = picture_user->size;
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
    memcpy(output->transport_fields, &source->display_mode,
           sizeof(output->transport_fields));
}

void mwPlyGetCurFrm(void* object, void* output)
{
    MwsPlayer* player = object;
    MwsFrameOutput* frame_output = output;
    int index;
    SfdHandle* sfd;
    void* user_data;
    int user_size;
    int display_mode;
    int skip_limit;
    SfdVideoFrameInfo* frame;
    int found_size;
    void* found_data;
    MwsPictureUserData* picture_user;
    int color_adjustment;
    SfdVideoFrameInfo* metadata_frame;
    int effect_type;

    if (MWSFD_IsEnableHndl(player) == 0) {
        MWSFSVM_Error("E1122614: mwPlyGetCurFrm: handle is invalid.");
        frame_output->frame = 0;
        return;
    }
    sfd = mwPlyGetSfdHn(player);
    SFD_GetFrm(sfd, (void**)&frame);
    if (frame != 0 && player->frame_sync == 0) {
        skip_limit = player->skipped_display_frames;
        for (index = 0; index < skip_limit; index++) {
            if (mwPlyIsNextFrmReady(player) != 1) {
                break;
            }
            SFD_RelFrm(sfd, frame);
            player->skipped_frames++;
            SFD_GetFrm(sfd, (void**)&frame);
        }
    }
    if (frame != 0) {
        player->acquired_frames++;
        player->current_frame = frame;
        metadata_frame = frame;
        player->frame_field_58 = metadata_frame->field_58;
        player->frame_field_5C = metadata_frame->field_5C;
        player->frame_field_6C = (signed char)metadata_frame->fields_6C[0];
        player->frame_field_6D = (signed char)metadata_frame->fields_6C[1];
        player->frame_field_6E = (signed char)metadata_frame->fields_6C[2];
        player->frame_field_3C = metadata_frame->field_3C;
        player->frame_field_40 = metadata_frame->field_40;
        player->frame_field_A8 = 0;
        mwl_convFrmInfFromSFD(player, frame, frame_output);

        picture_user = frame->picture_user_buffer;
        user_data = picture_user->data;
        user_size = picture_user->size;
        if (MWSFD_GetUsePicUsr() == 1 && player->picture_user_buffer != 0) {
            if (user_data != 0 && user_size > 4) {
                SUD_SearchSudDat((unsigned char*)user_data + 4, user_size - 4,
                                 &found_data, &found_size);
            } else {
                found_data = 0;
                found_size = 0;
            }
            if (found_data != 0 && found_size > 0) {
                if (found_size > player->picture_user_capacity) {
                    found_size = player->picture_user_capacity;
                }
                memset(player->picture_user_buffer, 0,
                       player->picture_user_capacity);
                memcpy(player->picture_user_buffer, found_data, found_size);
                player->picture_user_data = player->picture_user_buffer;
                player->picture_user_size = found_size;
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

        color_adjustment = mwsffrm_IsCurCcs(player) == 1;
        if (mwsffrm_IsPicUsrDat(player) == 1) {
            if (MWSFSFX_IsFrmCcs(player) == 1) {
                color_adjustment = 1;
            } else {
                color_adjustment = 0;
            }
        }
        MWSFD_SetColAdj(player, color_adjustment);

        display_mode = 0;
        switch (frame->field_58) {
        case 3:
            if ((signed char)frame->fields_6C[0] == 0) {
                display_mode = 2;
            }
            break;
        case 1:
        case 2:
            display_mode = 2;
            break;
        case 0:
        default:
            MWSFSVM_Error("E301271: mwsffrm_DecideFrmType() : Invalid Pstruct");
            break;
        }
        if (MWSFD_GetUsePicUsr() == 1 && MWSFD_IsFrmDivField(player) == 1) {
            display_mode = 2;
        }
        frame_output->display_mode = display_mode;

        player->current_header = frame_output->picture_order;
        effect_type = mwsffrm_GetCurFxType(player);
        MWSFSFX_SetFxType(player, effect_type);
    } else {
        frame_output->frame = 0;
    }
}

/* TODO: [provisional] linker-stripped (MAP UNUSED 0x50); RE4 mwsfdfrm.c body ported, compiles to 0x50. */
int mwPlyGetFrmSync(MwsPlayer* player)
{
    if (MWSFD_IsEnableHndl(player) == 0) {
        MWSFSVM_Error("E2010801: mwPlyGetFrmSync: handle is invalid.");
        return 0;
    }
    return player->frame_sync;
}

void mwPlySetFrmSync(MwsPlayer* player, int sync)
{
    if (MWSFD_IsEnableHndl(player) == 0) {
        MWSFSVM_Error("E1122629: mwPlySetFrmSync: handle is invalid.");
        return;
    }
    player->frame_sync = sync;
}
