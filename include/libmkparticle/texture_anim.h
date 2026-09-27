#ifndef LIBMKPARTICLE_TEXTURE_ANIM_H
#define LIBMKPARTICLE_TEXTURE_ANIM_H

#include "libmkparticle/vm.h"

typedef struct PfxTextureAnim {
    short frame_count;
    short mode;
    float frame_time;
} PfxTextureAnim;

void pfx_texture_animate(PfxVm* vm,
                         int texture_width, int frame_width, int frame_height,
                         int frame_count, float frame_time);
int pfx_texture_getframe(const PfxTextureAnim* anim, float time);

#endif
