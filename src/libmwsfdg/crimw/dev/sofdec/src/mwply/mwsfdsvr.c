#include "cri/sj.h"

typedef struct LSC LSC;
typedef struct SfdHandle SfdHandle;
typedef struct MwsPlayer MwsPlayer;
typedef struct MwsStHandle MwsStHandle;
typedef struct MwsSupply MwsSupply;

typedef void (*MwsServerBorderCallback)(void* object);

typedef struct MwsSupplyInterface {
    void* reserved[5];
    void (*start)(MwsSupply* supply);
} MwsSupplyInterface;

struct MwsSupply {
    const MwsSupplyInterface* interface;
};

struct MwsStHandle {
    int active;
    unsigned char reserved_004[0x08];
    SJ* stream;
    int element_id;
    void* backend;
};

struct MwsPlayer {
    int reserved_000;
    int active;
    int status;
    unsigned char reserved_00C[0x34];
    SfdHandle* sfd;
    void* stream;
    unsigned char reserved_048[0x04];
    LSC* loader;
    unsigned char reserved_050[0x10];
    int sleeping_server;
    int in_mwply_server;
    int in_sfd_server;
    int decoder_server_active;
    unsigned char reserved_070[0x04];
    signed char concat_play;
    signed char terminating_supply;
    signed char paused;
    unsigned char reserved_077[0x141];
    const char* filename;
    int filename_capacity;
    int start_requested;
    int file_offset;
    int file_length;
    int file_end_position;
    MwsSupply* supply;
    unsigned char reserved_1D4[0xC0];
    MwsStHandle sound;
    unsigned char reserved_2AC[0x0C];
};

typedef struct MwsLibraryWork {
    int reserved_000;
    float frame_rate;
    int max_width;
    int decoder_count;
    int server_mode;
    unsigned char reserved_014[0x10];
    int sleeping_server;
    unsigned char reserved_028[0x10];
    int use_picture_user_data;
    int pause_border;
    MwsServerBorderCallback enter_server;
    void* enter_server_object;
    MwsServerBorderCallback leave_server;
    void* leave_server_object;
    MwsServerBorderCallback idle_server;
    void* idle_server_object;
    int server_lock;
    int vsync_lock;
    unsigned char reserved_060[0x08];
    int error_code;
    MwsPlayer players[8];
} MwsLibraryWork;

typedef char MwsStHandleSizeCheck[sizeof(MwsStHandle) == 0x18 ? 1 : -1];
typedef char MwsPlayerSizeCheck[sizeof(MwsPlayer) == 0x2B8 ? 1 : -1];
typedef char MwsLibraryWorkSizeCheck[sizeof(MwsLibraryWork) == 0x162C ? 1 : -1];

extern int mwsfd_init_flag;

extern void ADXM_WaitVsync(void);
extern int ADXM_IsSetupThrd(void);
extern int LSC_GetNumStm(LSC* loader);
extern MwsLibraryWork* MWSFLIB_GetLibWorkPtr(void);
extern int MWSFLIB_SetErrCode(int error);
extern void MWSFCRE_SetSupplySj(MwsPlayer* player);
extern void MWSFSEE_ChkSupply(MwsPlayer* player);
extern int MWSFLSC_IsFsStatErr(LSC* loader);
extern void MWSFSFX_DecideCompoMode(MwsPlayer* player);
extern void MWSFSVM_Error(const char* message, ...);
extern void MWSFSVM_GotoIdleBorder(void);
extern int MWSFSVM_TestAndSet(int* value);
extern int MWSST_GetStat(MwsStHandle* sound);
extern void MWSST_Pause(MwsStHandle* sound, int paused);
extern int MWSTM_GetStat(void* stream);
extern int MWSTM_IsFsStatErr(void* stream);
extern int MWSTM_ReqStart(void* stream);
extern void MWSTM_SetFileRange(void* stream, const char* filename, int offset,
                               int length, int end_position);
extern int SFD_GetHnStat(SfdHandle* handle);
extern void SFD_ExecOne(SfdHandle* handle);
extern int SFD_IsHnSvrWait(SfdHandle* handle);
extern int SFD_IsSvrWait(void);
extern int SFD_SetConcatPlay(SfdHandle* handle);
extern int SFD_TermSupply(SfdHandle* handle);
extern void SFD_VbIn(void);
extern void mwPlyChkSupply(MwsPlayer* player);
extern void mwPlyRestoreRsc(void);
extern void mwPlySaveRsc(void);
extern void mwPlySfdStart(MwsPlayer* player);
extern void mwSfdPause(MwsPlayer* player, int paused);

static int mwSfdExecDecSvrHndl(MwsPlayer* player);

static const char invalid_handle[] =
    "E2011101: MWSFSVR_IsSvrBdrHndl: handle is invalid.";
static const char null_handle[] =
    "E1071901 mwPlyExecSvrHndl: NULL handle.";
static const char start_failed[] =
    "E211141 MWSTM_ReqStart: can't start '%s'";
static const char link_failed[] =
    "E99072103 mwPlyStartXX: can't link stream";
static const char terminate_failed[] =
    "E99072102 mwlSfdExecDecSvrPlaying: can't term";

extern int mwg_field_no;
extern int mwg_vcnt;
extern MwsPlayer* mwsfd_hn_last;

static inline void mwsfd_VsyncBody(void)
{
    MwsLibraryWork* work;

    mwg_field_no++;
    mwg_vcnt++;
    if (mwsfd_init_flag == 1) {
        work = MWSFLIB_GetLibWorkPtr();
        if (MWSFSVM_TestAndSet(&work->vsync_lock) != 0) {
            if (mwsfd_init_flag == 1) {
                SFD_VbIn();
            }
            work->vsync_lock = 0;
        }
    }
}

void mwlSfdSleepDecSvr(MwsPlayer* player)
{
    MwsLibraryWork* work;
    int count;
    int enabled;

    mwPlySaveRsc();
    work = MWSFLIB_GetLibWorkPtr();
    player->sleeping_server = 1;
    work->sleeping_server = 1;
    MWSFSVM_GotoIdleBorder();
    work = MWSFLIB_GetLibWorkPtr();
    player->sleeping_server = 0;
    work->sleeping_server = 0;
    mwPlyRestoreRsc();

    if (player->in_mwply_server == 1) {
        count = 0;
        enabled = 1;
        do {
            work = MWSFLIB_GetLibWorkPtr();
            player->sleeping_server = enabled;
            work->sleeping_server = enabled;
            ADXM_WaitVsync();
            work = MWSFLIB_GetLibWorkPtr();
            player->sleeping_server = 0;
            work->sleeping_server = 0;
            if (player->in_mwply_server == 0) {
                break;
            }
            count++;
        } while (count < 10);
    }
}

void MWSFSVR_SetHnSfdSvrFlg(MwsPlayer* player, int enabled)
{
    player->in_sfd_server = enabled;
}

void MWSFSVR_SetHnMwplySvrFlg(MwsPlayer* player, int enabled)
{
    player->in_mwply_server = enabled;
}

void MWSFSVR_SetMwsfdSvrFlg(int enabled)
{
    MwsLibraryWork* work = MWSFLIB_GetLibWorkPtr();
    work->server_lock = enabled;
}

#pragma dont_inline on
int mwsfd_ExecSvrHndl(MwsPlayer* player)
{
    SfdHandle* sfd;

    sfd = player->sfd;
    player->in_mwply_server = 1;
    if (player->active != 1) {
        player->in_mwply_server = 0;
        return 0;
    }
    mwsfd_hn_last = player;
    player->in_sfd_server = 1;
    SFD_ExecOne(sfd);
    player->in_sfd_server = 0;
    if (player->status == 0) {
        player->decoder_server_active = 0;
    } else {
        player->decoder_server_active = 1;
        mwSfdExecDecSvrHndl(player);
    }
    player->in_mwply_server = 0;
    return SFD_IsHnSvrWait(sfd) != 1;
}

int mwSfdExecSvrHndl(MwsPlayer* player)
{
    if (mwsfd_init_flag != 1) return 0;
    if (player == 0) {
        MWSFSVM_Error(null_handle);
        return 0;
    }
    if (player->active != 1) return 0;
    if (player->in_mwply_server == 1) return 0;
    if (MWSFLIB_GetLibWorkPtr()->sleeping_server == 1) return 0;
    return mwsfd_ExecSvrHndl(player);
}

int MWSFSVR_DecodeServer(void* object)
{
    MwsLibraryWork* work;
    MwsLibraryWork* callback_work;
    MwsPlayer* player;
    MwsServerBorderCallback callback;
    void* callback_object;
    int index;
    int server_wait;

    (void)object;
    if (mwsfd_init_flag != 1) return 0;
    work = MWSFLIB_GetLibWorkPtr();
    if (MWSFSVM_TestAndSet(&work->server_lock) == 0) return 0;
    callback_work = MWSFLIB_GetLibWorkPtr();
    callback = callback_work->enter_server;
    callback_object = callback_work->enter_server_object;
    if (callback != 0) {
        callback(callback_object);
    }
    index = 0;
    do {
        player = &work->players[0];
        if (player != 0) {
            mwSfdExecSvrHndl(player);
        }
        index++;
        /* Advance the embedded-player base by one retail handle stride. */
        work = (MwsLibraryWork*)((unsigned char*)work + sizeof(MwsPlayer));
    } while (index < 8);
    work = MWSFLIB_GetLibWorkPtr();
    work->server_lock = 0;
    server_wait = SFD_IsSvrWait() != 1;
    callback_work = MWSFLIB_GetLibWorkPtr();
    callback = callback_work->leave_server;
    callback_object = callback_work->leave_server_object;
    if (callback != 0) {
        callback(callback_object);
    }
    if (server_wait == 0 && MWSFLIB_GetLibWorkPtr()->sleeping_server != 1) {
        callback_work = MWSFLIB_GetLibWorkPtr();
        callback = callback_work->idle_server;
        callback_object = callback_work->idle_server_object;
        if (callback != 0) {
            callback(callback_object);
        }
    }
    return server_wait;
}
#pragma dont_inline reset

void mwSfdVsync(void)
{
    mwsfd_VsyncBody();
}

int MWSFSVR_IdleThrdProc(void* object)
{
    int result = 0;
    if (ADXM_IsSetupThrd() == 1 &&
        MWSFLIB_GetLibWorkPtr()->server_mode != 1) {
        result = MWSFSVR_DecodeServer(object);
    }
    return result;
}

int MWSFSVR_MainThrdProc(void* object)
{
    int result = 0;
    if (ADXM_IsSetupThrd() == 1) {
        if (MWSFLIB_GetLibWorkPtr()->server_mode == 1) {
            result = MWSFSVR_DecodeServer(object);
        }
    } else {
        mwSfdVsync();
        result = MWSFSVR_DecodeServer(object);
    }
    return result;
}

int MWSFSVR_VsyncThrdProc(void* object)
{
    (void)object;
    if (ADXM_IsSetupThrd() == 1) {
        mwSfdVsync();
        return 0;
    }
    return 0;
}

static int mwSfdExecDecSvrHndl(MwsPlayer* player)
{
    SfdHandle* sfd;

    switch (player->status) {
    case 1: {
        int stream_result;

        sfd = player->sfd;
        if (player->start_requested == 1) {
            if (MWSTM_GetStat(player->stream) == 2) {
                stream_result = -1;
            } else {
                if (player->supply != 0) {
                    player->supply->interface->start(player->supply);
                }
                MWSTM_SetFileRange(player->stream, player->filename,
                                   player->file_offset, player->file_length,
                                   player->file_end_position);
                if (MWSTM_ReqStart(player->stream) == -1) {
                    player->status = 4;
                    MWSFLIB_SetErrCode(-0x66);
                    MWSFSVM_Error(start_failed, player->filename);
                    player->start_requested = 0;
                    stream_result = -1;
                } else {
                    MWSFCRE_SetSupplySj(player);
                    stream_result = 1;
                }
            }
            if (stream_result == 1) player->start_requested = 0;
        }

        if (player->sound.active == 1) {
            MwsStHandle* sound = &player->sound;
            int sfd_status = SFD_GetHnStat(player->sfd);
            int sound_status = MWSST_GetStat(sound);

            if (sfd_status == 3 &&
                (sound_status == 2 ||
                 sound->stream->interface->get_num_data(sound->stream, 1) == 0)) {
                mwPlySfdStart(player);
                if (player->paused == 0) mwSfdPause(player, 0);
                if (player->concat_play == 1 &&
                    SFD_SetConcatPlay(player->sfd) != 0) {
                    MWSFSVM_Error(link_failed);
                }
                if (player->paused == 0) MWSST_Pause(sound, 0);
            }
        } else if (SFD_GetHnStat(player->sfd) == 3) {
            mwPlySfdStart(player);
            if (player->paused == 0) mwSfdPause(player, 0);
            if (player->concat_play == 1 &&
                SFD_SetConcatPlay(player->sfd) != 0) {
                MWSFSVM_Error(link_failed);
            }
        }
        {
            int sfd_status = SFD_GetHnStat(sfd);

            if (sfd_status == 4 || sfd_status == 6) {
                player->status = 2;
                MWSFSFX_DecideCompoMode(player);
            }
        }
        break;
    }
    case 2:
        sfd = player->sfd;
        if (player->terminating_supply == 1) {
            if (LSC_GetNumStm(player->loader) == 0) {
                if (SFD_TermSupply(sfd) != 0) {
                    MWSFSVM_Error(terminate_failed);
                }
                player->terminating_supply = 0;
            }
        } else {
            mwPlyChkSupply(player);
        }
        if (SFD_GetHnStat(sfd) == 6) player->status = 3;
        break;
    case 0:
        break;
    default:
        break;
    }

    if (player->stream != 0 && MWSTM_IsFsStatErr(player->stream) != 0) {
        player->status = 4;
    }
    if (player->loader != 0 && MWSFLSC_IsFsStatErr(player->loader) == 1) {
        player->status = 4;
    }
    MWSFSEE_ChkSupply(player);
    return 0;
}

const int gap_04_80319E2C_rodata = 0;

int gap_06_804AE1F4_bss[3];
MwsPlayer* mwsfd_hn_last;
int mwg_vcnt;
int mwg_field_no;
