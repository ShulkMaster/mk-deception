#ifndef GAME_FX_H
#define GAME_FX_H

#include "game/moveset.h"
#include "runtime/mk_particle.h"

struct FxPfxDefinition {
    unsigned int flags;
    int field_04;
    int field_08;
    Vec origin;
    float particle_size;
    float red;
    float green;
    float blue;
    float alpha;
    int birth_rate;
    int particle_capacity;
    int emitter_field_40;
    unsigned int texture_handle;
    int texture_width;
    int frame_width;
    int frame_height;
    int frame_count;
    float frame_time;
    int texture_mode;
    int kill_plane_x;
    int kill_plane_y;
    int kill_plane_z;
    float plane_x;
    float plane_y;
    float plane_z;
    int lifetime_mode;
    int lifetime_minimum;
    int lifetime_maximum;
    PfxInitCb initialize;
};

typedef char FxPfxDefinitionSizeCheck[sizeof(struct FxPfxDefinition) == 0x7C ? 1 : -1];

typedef struct FxHdrLatch {
    MkHdr* object;
    unsigned int instance;
} FxHdrLatch;

extern PlyrMirrorObjLatch p1_freeze_light_item;
extern PlyrMirrorObjLatch p2_freeze_light_item;
extern FxHdrLatch p1_freeze_proc_item;
extern FxHdrLatch p2_freeze_proc_item;

#ifdef __cplusplus
extern "C" {
#endif

void kill_all_fstyle_signs(void);
MkPfx* create_pfx(
    int bind_source, int process_id, MkProcEntryFn entry,
    MkPfx** effect_out, const struct FxPfxDefinition* definition,
    const char* name);

#ifdef __cplusplus
}
#endif

#endif
