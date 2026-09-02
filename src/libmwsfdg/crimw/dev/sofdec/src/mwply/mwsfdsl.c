#include "cri/sj.h"

typedef struct LSC LSC;
typedef struct SfdHandle SfdHandle;
typedef struct MwsSupply MwsSupply;

typedef struct MwsSupplyInterface {
    void* reserved[5];
    void (*start)(MwsSupply* supply);
} MwsSupplyInterface;

struct MwsSupply {
    const MwsSupplyInterface* interface;
};

typedef struct MwsPlayer {
    unsigned char reserved_000[0x08];
    int status;
    unsigned char reserved_00C[0x34];
    SfdHandle* sfd;
    unsigned char reserved_044[0x08];
    LSC* loader;
    unsigned char reserved_050[0x24];
    signed char concat_play;
    unsigned char concat_stopped;
    unsigned char reserved_076[0x02];
    int entry_count;
    unsigned char reserved_07C[0x13C];
    const char* filename;
    unsigned char reserved_1BC[0x14];
    MwsSupply* supply;
    SJ* input_sj;
} MwsPlayer;

extern int MWSFD_IsEnableHndl(MwsPlayer* player);
extern void MWSFSVM_Error(const char* message, ...);
extern int SFD_SetConcatPlay(SfdHandle* handle);
extern void LSC_SetFlowLimit(LSC* loader, int minimum_buffer_size);
extern int LSC_GetStat(LSC* loader);
extern void LSC_Stop(LSC* loader);
extern int LSC_EntryFname(LSC* loader, const char* filename);
extern void LSC_SetLpFlg(LSC* loader, int loop);
extern void LSC_Start(LSC* loader);
extern void MWSFPLY_RecordFname(MwsPlayer* player, const char* filename);
extern void mwSfdStartSj(MwsPlayer* player, SJ* stream);
extern void MWSFPLY_SetFlowLimit(MwsPlayer* player);
extern void MWSFCRE_SetSupplySj(MwsPlayer* player);

/* This retail unit keeps the complete seamless-play diagnostic catalog. */
static const char filename_format[] = "%08x.%08x";
static const char start_invalid[] =
    "E1122630: mwPlyStartFnameLp: handle is invalid.";
static const char start_null[] =
    "E10915A: mwPlyStartFnameLp: fname is NULL.";
static const char entry_invalid[] =
    "E1122633: mwPlyEntryFname: handle is invalid.";
static const char entry_null[] =
    "E10915B: mwPlyEntryFname: fname is NULL.";
static const char entry_failed[] =
    "E204021: mwPlyEntryFname: Can't entry file'%s'";
static const char loop_invalid[] =
    "E1122641: mwPlySetLpFlg: handle is invalid.";
static const char seamless_invalid[] =
    "E1122634: mwPlyStartSeamless: handle is invalid.";
static const char link_invalid[] =
    "E1122642: mwPlyLinkStm: handle is invalid.";
static const char link_failed[] =
    "E99072101 mwPlyLinkStm: can't link stream";
static const char get_filename_invalid[] =
    "E1122637: mwPlyGetSlFname: handle is invalid.";
static const char stream_number_invalid[] =
    "E10821B : Invalid value of stm_no : %d";
static const char range_start_invalid[] =
    "E407024: mwPlyStartFnameRangeLp: handle is invalid.";
static const char range_entry_invalid[] =
    "E407023: mwPlyEntryFnameRange: handle is invalid.";
static const char afs_start_invalid[] =
    "E1122632: mwPlyStartAfsLp: handle is invalid.";
static const char afs_entry_invalid[] =
    "E1122636: mwPlyEntryAfs: handle is invalid.";
static const char afs_entry_failed[] =
    "E008311 mwPlyEntryAfs: can't entry pid=%d fid=%d";
static const char release_seamless_invalid[] =
    "E1122635: mwPlyReleaseSeamless: handle is invalid.";
static const char release_loop_invalid[] =
    "E1122631: mwPlyReleaseLp: handle is invalid.";

void mwPlyLinkStm(MwsPlayer* player, int link)
{
    SfdHandle* sfd;

    if (MWSFD_IsEnableHndl(player) == 0) {
        MWSFSVM_Error(link_invalid);
        return;
    }
    sfd = player->sfd;
    if (player->concat_play == 1 && link == 0) {
        player->concat_stopped = 1;
    }
    if (player->concat_play == 0 && link == 1) {
        if (SFD_SetConcatPlay(sfd) != 0) {
            MWSFSVM_Error(link_failed);
        }
    }
    player->concat_play = link;
}

void MWSFLSC_SetFlowLimit(MwsPlayer* player, int minimum_buffer_size)
{
    if (player->loader != 0) {
        LSC_SetFlowLimit(player->loader, minimum_buffer_size);
    }
}

int MWSFLSC_IsFsStatErr(LSC* loader)
{
    return LSC_GetStat(loader) == 3;
}

static inline void mwPlyStartSeamless(MwsPlayer* player)
{
    if (MWSFD_IsEnableHndl(player) == 0) {
        MWSFSVM_Error(link_invalid);
    } else {
        if (player->concat_play == 0 && SFD_SetConcatPlay(player->sfd) != 0) {
            MWSFSVM_Error(link_failed);
        }
        player->concat_play = 1;
    }
    mwSfdStartSj(player, player->input_sj);
    MWSFPLY_SetFlowLimit(player);
    LSC_Start(player->loader);
    if (player->supply != 0) {
        player->supply->interface->start(player->supply);
    }
    MWSFCRE_SetSupplySj(player);
}

void mwPlyStartFnameLp(MwsPlayer* player, const char* filename)
{
    const char* recorded_filename;

    if (MWSFD_IsEnableHndl(player) == 0) {
        MWSFSVM_Error(start_invalid);
        return;
    }
    if (filename == 0) {
        MWSFSVM_Error(start_null);
        return;
    }
    MWSFPLY_RecordFname(player, filename);
    LSC_Stop(player->loader);
    recorded_filename = player->filename;
    if (MWSFD_IsEnableHndl(player) == 0) {
        MWSFSVM_Error(entry_invalid);
    } else if (recorded_filename == 0) {
        MWSFSVM_Error(entry_null);
    } else if (LSC_EntryFname(player->loader, recorded_filename) < 0) {
        player->status = 4;
        MWSFSVM_Error(entry_failed, recorded_filename);
    } else {
        player->entry_count++;
    }
    if (MWSFD_IsEnableHndl(player) == 0) {
        MWSFSVM_Error(loop_invalid);
    } else {
        LSC_SetLpFlg(player->loader, 1);
    }
    if (MWSFD_IsEnableHndl(player) == 0) {
        MWSFSVM_Error(seamless_invalid);
        return;
    }
    mwPlyStartSeamless(player);
}
