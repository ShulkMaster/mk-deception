#include "cri/sj.h"
#include "runtime/cstring.h"
#include "sofdec/sfd_player.h"

typedef struct LSC LSC;
typedef struct MwsPlayer MwsPlayer;

typedef struct MwsStHandle {
    int active;
    unsigned char reserved_004[0x08];
    SJ* stream;
    int element_id;
    void* backend;
} MwsStHandle;

struct MwsPlayer {
    void* interface;
    int active;
    int status;
    int playback_mode;
    unsigned char reserved_010[0x30];
    SfdHandle* sfd;
    void* stream;
    unsigned char reserved_048[0x04];
    LSC* loader;
    unsigned char reserved_050[0x24];
    signed char concat_play;
    signed char concat_stopped;
    signed char paused;
    unsigned char reserved_077;
    int entry_count;
    unsigned char reserved_07C[0x04];
    int field_080;
    int field_084;
    int field_088;
    unsigned char reserved_08C[0x12C];
    char* filename;
    int filename_capacity;
    int start_requested;
    int file_offset;
    int file_length;
    int file_end_position;
    SJ* supply_sj;
    SJ* input_sj;
    void* input_buffer;
    int flow_limit;
    int input_buffer_extra;
    int start_mode;
    int field_1E8;
    int field_1EC;
    int field_1F0;
    SJ* memory_sj;
    void* memory_buffer;
    int memory_buffer_size;
    unsigned char reserved_200[0x94];
    MwsStHandle sound;
    unsigned char reserved_2AC[0x08];
    int sound_state;
};

typedef char MwsStHandleSizeCheck[sizeof(MwsStHandle) == 0x18 ? 1 : -1];
typedef char MwsPlayerSizeCheck[sizeof(MwsPlayer) == 0x2B8 ? 1 : -1];

extern void LSC_Stop(LSC* loader);
extern int MWSFCRE_ResetSfdHn(MwsPlayer* player);
extern void MWSFCRE_SetSupplySj(MwsPlayer* player);
extern int MWSFD_GetPauseBdr(void);
extern int MWSFD_IsEnableHndl(MwsPlayer* player);
extern void MWSFD_SetFlowLimit(MwsPlayer* player, int limit,
                               int maximum_size);
extern void MWSFFRM_InitSfhInfTable(MwsPlayer* player);
extern void* MWSFLIB_GetLibWorkPtr(void);
extern int MWSFLIB_SetErrCode(int error);
extern void MWSFSEE_StartFnameSub1(MwsPlayer* player, int offset, int length,
                                  int end_position);
extern void MWSFSEE_StartFnameSub2(MwsPlayer* player, int offset, int length);
extern void MWSFSVM_Error(const char* message, ...);
extern void MWSFTAG_InitTagInf(MwsPlayer* player);
extern void MWSFTAG_ResetAinfSj(MwsPlayer* player);
extern int MWSFTAG_SetAinfSj(MwsPlayer* player);
extern void MWSST_Pause(MwsStHandle* sound, int paused);
extern void MWSST_Reset(MwsPlayer* player);
extern void MWSST_StartSj(MwsStHandle* sound);
extern void MWSST_Stop(MwsStHandle* sound);
extern int MWSTM_GetStat(void* stream);
extern void MWSTM_ReqStop(void* stream);
extern int SFD_Pause(SfdHandle* handle, int paused);
extern int SFD_Standby(SfdHandle* handle);
extern void mwPlyLinkStm(MwsPlayer* player, int link);
extern void mwlSfdSleepDecSvr(MwsPlayer* player);

#pragma force_active on
static const char range_handle_invalid[0x34] =
    "E407021: mwPlyStartFnameRange: handle is invalid.";
static const char range_filename_null[0x30] =
    "E407022: mwPlyStartFnameRange: fname is NULL.";
static const char filename_too_long[0x20] =
    "E211121: filename is longer.";
static const char filename_range_format[0x0C] = "%08x.%08x";
static const char filename_range_failed[0x28] =
    "E211151: ADXF_GetFnameRangeEx() faild.";
#pragma force_active reset

void MWSFPLY_RecordFname(MwsPlayer* player, const char* filename)
{
    if ((int)strlen(filename) > player->filename_capacity) {
        MWSFSVM_Error(filename_too_long);
        strncpy(player->filename, filename, player->filename_capacity);
    } else {
        strcpy(player->filename, filename);
    }
}

void mwPlyChkSupply(MwsPlayer* player)
{
    void* stream = player->stream;
    SfdHandle* sfd = player->sfd;

    if (stream != 0 && MWSTM_GetStat(stream) == 3) {
        SFD_TermSupply(sfd);
    }
}

void MWSFPLY_SetFlowLimit(MwsPlayer* player)
{
    int flow_limit = player->flow_limit;
    int adjusted_limit = (int)(0.8 * flow_limit);

    MWSFD_SetFlowLimit(player, adjusted_limit, flow_limit);
}

#pragma force_active on
static const char start_afs_invalid[0x2C] =
    "E1122638: mwPlyStartAfs: handle is invalid.";
static const char pause_handle_invalid[0x28] =
    "E1122604 mwSfdPause; handle is invalid.";
static const char pause_failed[0x24] =
    "E2007 mwSfdPause; can't pause (%s)";
static const char pause_on[0x04] = "ON";
static const char pause_off[0x04] = "OFF";
static const char stop_handle_invalid[0x28] =
    "E1122602 mwSfdStop: handle is invalid.";
static const char stop_failed[0x20] =
    "E2003 mwSfdStop:can't stop SFD";
static const char start_sj_handle_invalid[0x2C] =
    "E1122609 mwSfdStartSj: handle is invalid.";
static const char reset_failed[0x2C] =
    "E0203263: mw_sfd_start_ex: RESET failed.";
static const char set_additional_info_failed[0x2C] =
    "E201213 mw_sfd_start_ex: can't set AddInfSJ";
static const char standby_failed[0x20] =
    "E20010703F mwPlySfdStandby: ";
static const char start_mem_handle_invalid[0x2C] =
    "E1122610 mwSfdStartMem: handle is invalid.";
static const char start_filename_handle_invalid[0x30] =
    "E1122601: mwPlyStartFname: handle is invalid.";
static const char start_filename_null[0x2C] =
    "E10915C: mwPlyStartFname: fname is NULL.";
static const char pause_status_invalid[0x30] =
    "E10821A : Invalid value of SFD_GetPaStat : %d";
static const char start_failed[0x1C] =
    "E20010703G mwPlySfdStart: ";
#pragma force_active reset

void mwSfdPause(MwsPlayer* player, int paused)
{
    SfdHandle* sfd;
    int pause_status;

    if (MWSFD_IsEnableHndl(player) == 0) {
        MWSFSVM_Error(pause_handle_invalid);
        return;
    }
    sfd = player->sfd;
    if (player->paused == 0 && paused == 0) {
        return;
    }
    if (MWSFD_GetPauseBdr() == 1 && player->playback_mode == 1) {
        if (SFD_GetCond(sfd, 6, &pause_status) == 0) {
            if (pause_status == 1) {
                mwlSfdSleepDecSvr(player);
            }
        } else {
            mwlSfdSleepDecSvr(player);
        }
    }
    if (SFD_Pause(sfd, paused) != 0) {
        MWSFLIB_SetErrCode(-0x136);
        MWSFSVM_Error(pause_failed, paused == 1 ? pause_on : pause_off);
    }
    MWSST_Pause(&player->sound, paused);
    player->paused = paused;
}

void mwSfdStop(MwsPlayer* player)
{
    SfdHandle* sfd;

    if (MWSFD_IsEnableHndl(player) == 0) {
        MWSFSVM_Error(stop_handle_invalid);
        return;
    }
    sfd = player->sfd;
    if (sfd != 0) {
        mwlSfdSleepDecSvr(player);
        player->status = 0;
        if (SFD_Stop(sfd) != 0) {
            MWSFLIB_SetErrCode(-0x134);
            MWSFSVM_Error(stop_failed);
        }
        MWSST_Stop(&player->sound);
        player->sound_state = 0;
        if (player->stream != 0) {
            MWSTM_ReqStop(player->stream);
        }
        LSC_Stop(player->loader);
    }
    mwPlyLinkStm(player, 0);
    player->entry_count = 0;
    LSC_Stop(player->loader);
}

void mwSfdStopDec(MwsPlayer* player)
{
    SfdHandle* sfd = player->sfd;

    if (sfd != 0) {
        mwlSfdSleepDecSvr(player);
        player->status = 0;
        if (SFD_Stop(sfd) != 0) {
            MWSFLIB_SetErrCode(-0x134);
            MWSFSVM_Error(stop_failed);
        }
        MWSST_Stop(&player->sound);
        player->sound_state = 0;
        if (player->stream != 0) {
            MWSTM_ReqStop(player->stream);
        }
        LSC_Stop(player->loader);
    }
}

static inline void mwSfdPrepareStart(MwsPlayer* player)
{
    MWSFLIB_GetLibWorkPtr();
    if (player->sfd != 0) {
        if (MWSFCRE_ResetSfdHn(player) != 0) {
            MWSFSVM_Error(reset_failed);
            return;
        }
        MWSST_Reset(player);
        MWSFTAG_ResetAinfSj(player);
        if (MWSFTAG_SetAinfSj(player) != 0) {
            MWSFSVM_Error(set_additional_info_failed);
            return;
        }
        MWSFTAG_InitTagInf(player);
        MWSFFRM_InitSfhInfTable(player);
    }
    player->field_080 = 0;
    player->field_084 = 0;
    if (SFD_Standby(player->sfd) != 0) {
        MWSFLIB_SetErrCode(-0x137);
        MWSFSVM_Error(standby_failed);
    }
    mwSfdPause(player, player->paused);
    MWSST_Pause(&player->sound, 1);
    MWSST_StartSj(&player->sound);
    player->field_088 = 0;
    player->concat_stopped = 0;
    player->status = 1;
}

void mwSfdStartSj(MwsPlayer* player, SJ* stream)
{
    if (MWSFD_IsEnableHndl(player) == 0) {
        MWSFSVM_Error(start_sj_handle_invalid);
        return;
    }
    mwSfdStopDec(player);
    player->supply_sj = stream;
    player->start_mode = 2;
    player->field_1E8 = 0;
    player->field_1EC = 0;
    player->field_1F0 = 0;
    mwSfdPrepareStart(player);
    MWSFCRE_SetSupplySj(player);
}

void mwSfdStartMem(MwsPlayer* player, void* buffer, int buffer_size)
{
    if (MWSFD_IsEnableHndl(player) == 0) {
        MWSFSVM_Error(start_mem_handle_invalid);
        return;
    }
    mwSfdStopDec(player);
    player->memory_sj->interface->destroy(player->memory_sj);
    player->memory_sj = SJMEM_Create(buffer, buffer_size);
    player->supply_sj = player->memory_sj;
    player->memory_buffer = buffer;
    player->memory_buffer_size = buffer_size;
    mwSfdPrepareStart(player);
    MWSFCRE_SetSupplySj(player);
}

void mwSfdStartFnameSub(MwsPlayer* player, const char* filename, int offset,
                        int length)
{
    player->supply_sj = player->input_sj;
    mwSfdStopDec(player);
    mwSfdPrepareStart(player);
    MWSFPLY_RecordFname(player, filename);
    player->file_offset = 0;
    player->file_length = 0;
    player->file_end_position = 0xFFFFF;
    player->start_requested = 1;
    MWSFSEE_StartFnameSub1(player, offset, length, 0xFFFFF);
    MWSFSEE_StartFnameSub2(player, offset, length);
}

#pragma dont_inline on
void mwSfdStartFname(MwsPlayer* player, const char* filename)
{
    if (MWSFD_IsEnableHndl(player) == 0) {
        MWSFSVM_Error(start_filename_handle_invalid);
        return;
    }
    if (filename == 0) {
        MWSFSVM_Error(start_filename_null);
        return;
    }
    mwSfdStartFnameSub(player, filename, 0, -1);
}
#pragma dont_inline reset

int mwPlySfdStart(MwsPlayer* player)
{
    if (SFD_Start(player->sfd) != 0) {
        MWSFLIB_SetErrCode(-0x133);
        MWSFSVM_Error(start_failed);
        return -0x133;
    }
    return 0;
}
