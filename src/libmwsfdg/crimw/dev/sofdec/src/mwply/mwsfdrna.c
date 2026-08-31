#include "dolphin/types.h"
#include "sofdec/sfd_player.h"

typedef struct MwsfdPlayer {
    u8 reserved_00[0x40];
    SfdHandle* sfd_handle;
} MwsfdPlayer;

int MWSFRNA_GetOutPan(MwsfdPlayer* player, int channel)
{
    return SFD_GetOutPan(player->sfd_handle, channel);
}

void MWSFRNA_SetOutPan(MwsfdPlayer* player, int channel, int pan)
{
    SFD_SetOutPan(player->sfd_handle, channel, pan);
}

int MWSFRNA_GetOutVol(MwsfdPlayer* player)
{
    return SFD_GetOutVol(player->sfd_handle);
}

void MWSFRNA_SetOutVol(MwsfdPlayer* player, int volume)
{
    SFD_SetOutVol(player->sfd_handle, volume);
}
