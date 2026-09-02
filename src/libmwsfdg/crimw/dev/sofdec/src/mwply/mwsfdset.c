#include "cri/sj.h"
#include "sofdec/sfd_player_types.h"

typedef struct SfdHandle SfdHandle;
typedef struct MwsStream MwsStream;
typedef struct MwsLoader MwsLoader;
typedef struct MwsSound {
    int active;
    unsigned char reserved_04[8];
    SJ* stream;
    int element_id;
    void* backend;
} MwsSound;

typedef struct MwsPlayer {
    unsigned char reserved_000[4];
    int active;
    int status;
    int playback_mode;
    unsigned char reserved_010[0x30];
    SfdHandle* sfd;
    MwsStream* stream;
    unsigned char reserved_048[4];
    MwsLoader* loader;
    unsigned char reserved_050[0x244];
    MwsSound sound;
} MwsPlayer;

extern void MWSFSVM_Error(const char* message, ...);
extern void MWSFLIB_SetErrCode(int error);
extern int MWSFRNA_GetOutPan(MwsPlayer* player);
extern void MWSFRNA_SetOutPan(MwsPlayer* player, int pan);
extern int MWSFRNA_GetOutVol(MwsPlayer* player);
extern void MWSFRNA_SetOutVol(MwsPlayer* player, int volume);
extern int MWSST_GetOutVol(MwsSound* sound);
extern void MWSST_SetOutVol(MwsSound* sound, int volume);
extern int SFD_GetTime(SfdHandle* handle, int* value, int* scale);
extern int SFD_SetCond(SfdHandle* handle, int condition,
                       SfdConditionValue value);
extern int SFD_GetHnStat(SfdHandle* handle);
extern int SFD_SetAudioCh(SfdHandle* handle, int channel);
extern void MWSTM_SetFlowLimit(MwsStream* stream, int minimum_size,
                               int maximum_size);
extern void MWSFLSC_SetFlowLimit(MwsPlayer* player, int limit);

static inline int mwsfd_IsEnabled(MwsPlayer* player)
{
    if (player == 0) {
        return 0;
    }
    return player->active;
}

static const char get_handle_invalid[] =
    "E1122640: mwPlyGetSfdHn: handle is invalid.";
static const char get_pan_invalid[] =
    "E1122608 mwSfdGetOutPan: handle is invalid.";
static const char set_pan_invalid[] =
    "E1122607 mwSfdSetOutPan: handle is invalid.";
static const char get_volume_invalid[] =
    "E1122606 mwSfdGetOutVol: handle is invalid.";
static const char set_volume_invalid[] =
    "E1122605 mwSfdSetOutVol: handle is invalid.";
static const char get_time_invalid[] =
    "E1122603 mwSfdGetTime; handle is invalid.";
static const char get_time_failed[] =
    "E2006 mwSfdGetTime; can't get time";
static const char sync_mode_invalid[] =
    "E1122626: mwPlySetSyncMode: handle is invalid.";
static const char get_status_invalid[] =
    "W2004 mwSfdGetStat: handle is invalid";
static const char audio_channel_invalid[] =
    "E1122616 mwPlySetAudioCh: handle is invalid.";
static const char audio_channel_failed[] =
    "E10911A mwPlySetAudioCh: Invalid ch no.";

int mwSfdGetOutPan(MwsPlayer* player)
{
    if (mwsfd_IsEnabled(player) == 0) {
        MWSFSVM_Error(get_pan_invalid);
        return 0;
    }
    return MWSFRNA_GetOutPan(player);
}

void mwSfdSetOutPan(MwsPlayer* player, int pan)
{
    if (mwsfd_IsEnabled(player) == 0) {
        MWSFSVM_Error(set_pan_invalid);
        return;
    }
    MWSFRNA_SetOutPan(player, pan);
}

int mwSfdGetOutVol(MwsPlayer* player)
{
    int movie_volume;
    int sound_volume;

    if (mwsfd_IsEnabled(player) == 0) {
        MWSFSVM_Error(get_volume_invalid);
        return 0;
    }
    movie_volume = MWSFRNA_GetOutVol(player);
    sound_volume = MWSST_GetOutVol(&player->sound);
    if (movie_volume == sound_volume) {
        return movie_volume;
    }
    if (movie_volume != 0) {
        return movie_volume;
    }
    return sound_volume;
}

void mwSfdSetOutVol(MwsPlayer* player, int volume)
{
    if (mwsfd_IsEnabled(player) == 0) {
        MWSFSVM_Error(set_volume_invalid);
        return;
    }
    MWSFRNA_SetOutVol(player, volume);
    MWSST_SetOutVol(&player->sound, volume);
}

void mwSfdGetTime(MwsPlayer* player, int* value, int* scale)
{
    if (mwsfd_IsEnabled(player) == 0) {
        MWSFSVM_Error(get_time_invalid);
        *value = 0;
        *scale = 1;
        return;
    }
    if (SFD_GetTime(player->sfd, value, scale) != 0) {
        MWSFLIB_SetErrCode(-0x135);
        MWSFSVM_Error(get_time_failed);
        *value = 0;
        *scale = 1;
    }
    if (*value < 0) {
        *value = 0;
        *scale = 1;
    }
}

void mwPlySetSyncMode(MwsPlayer* player, int mode)
{
    SfdHandle* sfd;

    if (mwsfd_IsEnabled(player) == 0) {
        MWSFSVM_Error(sync_mode_invalid);
        return;
    }
    sfd = player->sfd;
    if (mode == 0) {
        SFD_SetCond(sfd, 0xF, 0);
    } else if (mode == 1) {
        SFD_SetCond(sfd, 0xF, 1);
    } else if (mode == 2) {
        if (player->playback_mode == 1) {
            SFD_SetCond(sfd, 0xF, 2);
        } else {
            SFD_SetCond(sfd, 0xF, 1);
        }
    } else if (mode == 3) {
        SFD_SetCond(sfd, 0xF, 5);
        SFD_SetCond(sfd, 0x47, 1);
    }
}

int mwSfdGetStat(MwsPlayer* player)
{
    int status;

    if (mwsfd_IsEnabled(player) == 0) {
        MWSFLIB_SetErrCode(-12);
        MWSFSVM_Error(get_status_invalid);
        return 0;
    }
    status = player->status;
    if (status == 2) {
        status = SFD_GetHnStat(player->sfd);
        if (status == 4 || status == 6) {
            return 2;
        }
        if (status < 0) {
            return 4;
        }
        return 1;
    }
    return status;
}

int MWSFD_IsEnableHndl(MwsPlayer* player)
{
    if (player == 0) {
        return 0;
    }
    return player->active;
}

void MWSFD_SetFlowLimit(MwsPlayer* player, int limit, int maximum_size)
{
    MWSTM_SetFlowLimit(player->stream, limit, maximum_size);
    MWSFLSC_SetFlowLimit(player, limit);
}

void MWSFD_SetCond(MwsPlayer* player, int condition, int value)
{
    SFD_SetCond(player != 0 ? player->sfd : 0, condition, value);
}

void mwPlySetAudioCh(MwsPlayer* player, int channel)
{
    if (mwsfd_IsEnabled(player) == 0) {
        MWSFSVM_Error(audio_channel_invalid);
        return;
    }
    if (SFD_SetAudioCh(player->sfd, channel) != 0) {
        MWSFSVM_Error(audio_channel_failed);
    }
}

SfdHandle* mwPlyGetSfdHn(MwsPlayer* player)
{
    if (mwsfd_IsEnabled(player) == 0) {
        MWSFSVM_Error(get_handle_invalid);
        return 0;
    }
    return player->sfd;
}
