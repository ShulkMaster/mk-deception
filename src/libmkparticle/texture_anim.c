#include "libmkparticle/texture_anim.h"

#include "libmkparticle/config.h"
#include "libmkparticle/pfx_memory.h"

/* Soft ceiling: pfx_texture_animate ~90% -- flag-test and NV register emission. */
void pfx_texture_animate(PfxVm* vm, float frame_time,
                         int texture_width, int frame_width, int frame_height,
                         int frame_count) {
    PfxTextureFrame* frames;
    float u_step;
    float v_step;
    float u;
    float v;
    int frames_per_row;
    int column;
    int i;

    if (frame_count > 0x80) {
        return;
    }
    if (vm->field_0x22C != 0) {
        if ((vm->flags_0x1D4 & 0x100) == 0) {
            return;
        }
    } else {
        if ((vm->flags_0x1D4 & 0x300) == 0) {
            return;
        }
        if ((vm->flags_0x60 & 2) == 0) {
            return;
        }
    }

    frames = pfx_effect_memory_alloc(vm, frame_count * sizeof(PfxTextureFrame), 4);
    vm->texture_frames = frames;
    vm->texture_frame_time = frame_time;
    vm->texture_frame_count = (short)frame_count;
    frames_per_row = texture_width / frame_width;

    if (_pfx_config.normalized_texture_coords != 0) {
        float scale;

        scale = 1.0f / (float)texture_width;
        u_step = (float)frame_width * scale;
        v_step = (float)frame_height * scale;
    } else {
        u_step = (float)frame_width;
        v_step = (float)frame_height;
    }

    vm->texture_u_step = u_step;
    vm->texture_v_step = v_step;
    column = 0;
    u = 0.0f;
    v = 0.0f;
    for (i = 0; i < frame_count; i++) {
        frames[i].u = u;
        frames[i].v = v;
        column++;
        u += u_step;
        if (column >= frames_per_row) {
            column = 0;
            u = 0.0f;
            v += v_step;
        }
    }
}

int pfx_texture_getframe(const PfxTextureAnim* anim, float time) {
    int frame;

    frame = (int)(time / anim->frame_time);
    if (frame >= anim->frame_count) {
        switch (anim->mode) {
        case 0:
            frame = anim->frame_count - 1;
            break;
        case 1:
            frame %= anim->frame_count;
            break;
        }
    }
    return frame;
}
