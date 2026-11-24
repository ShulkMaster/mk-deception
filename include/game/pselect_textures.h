#ifndef GAME_PSELECT_TEXTURES_H
#define GAME_PSELECT_TEXTURES_H

#include "mwScreenEngine/TextureCollection.h"

typedef GVTexturePair PselectTexOut;

#ifdef __cplusplus
extern "C" {
#endif

void get_background_select_textures(PselectTexOut out);
void get_pselect_body_textures(PselectTexOut out);
void get_bg_pselect_team_textures(PselectTexOut out, int team);
#ifdef VERSION_GQNE5D
void get_pselect_head_textures(PselectTexOut out);
#endif
#ifdef VERSION_SLUS_208_81
void get_pselect_head_textures(RwTexture** out);
#endif
void get_pz_special_move_list(PselectTexOut out, int use_difficulty);

#ifdef __cplusplus
}
#endif

#endif
