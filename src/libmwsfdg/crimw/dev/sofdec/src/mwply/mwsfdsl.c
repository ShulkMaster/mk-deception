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
    int stat;
    unsigned char reserved_00C[0x34];
    SfdHandle* sfd;
    unsigned char reserved_044[0x08];
    LSC* lsc;
    unsigned char reserved_050[0x24];
    signed char linkstm;
    unsigned char linkstm_req;
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
    if (player->linkstm == 1 && link == 0) {
        player->linkstm_req = 1;
    }
    if (player->linkstm == 0 && link == 1) {
        if (SFD_SetConcatPlay(sfd) != 0) {
            MWSFSVM_Error(link_failed);
        }
    }
    player->linkstm = link;
}

void MWSFLSC_SetFlowLimit(MwsPlayer* player, int minimum_buffer_size)
{
    if (player->lsc != 0) {
        LSC_SetFlowLimit(player->lsc, minimum_buffer_size);
    }
}

int MWSFLSC_IsFsStatErr(LSC* loader)
{
    return LSC_GetStat(loader) == 3;
}

static inline void mwPlyEntryFname(MwsPlayer* player, const char* filename)
{
    if (MWSFD_IsEnableHndl(player) == 0) {
        MWSFSVM_Error(entry_invalid);
        return;
    }
    if (filename == 0) {
        MWSFSVM_Error(entry_null);
        return;
    }
    if (LSC_EntryFname(player->lsc, filename) < 0) {
        player->stat = 4;
        MWSFSVM_Error(entry_failed, filename);
        return;
    }
    player->entry_count++;
}

static inline void mwPlySetLpFlg(MwsPlayer* player, int loop)
{
    if (MWSFD_IsEnableHndl(player) == 0) {
        MWSFSVM_Error(loop_invalid);
        return;
    }
    LSC_SetLpFlg(player->lsc, loop);
}

static inline void mwPlyStartSub(MwsPlayer* player)
{
    mwPlyLinkStm(player, 1);
    mwSfdStartSj(player, player->input_sj);
    MWSFPLY_SetFlowLimit(player);
    LSC_Start(player->lsc);
    if (player->supply != 0) {
        player->supply->interface->start(player->supply);
    }
    MWSFCRE_SetSupplySj(player);
}

static inline void mwPlyStartSeamless(MwsPlayer* player)
{
    if (MWSFD_IsEnableHndl(player) == 0) {
        MWSFSVM_Error(seamless_invalid);
        return;
    }
    mwPlyStartSub(player);
}

/* TODO: [near miss] 98.88%; inlined entry/loop/seamless/LinkStm bodies exact; retail
 * keeps a player copy (mr r30,r29) for the StartSub tail that we coalesce into r29. */
void mwPlyStartFnameLp(MwsPlayer* player, const char* filename)
{
    if (MWSFD_IsEnableHndl(player) == 0) {
        MWSFSVM_Error(start_invalid);
        return;
    }
    if (filename == 0) {
        MWSFSVM_Error(start_null);
        return;
    }
    MWSFPLY_RecordFname(player, filename);
    LSC_Stop(player->lsc);
    mwPlyEntryFname(player, player->filename);
    mwPlySetLpFlg(player, 1);
    mwPlyStartSeamless(player);
}
