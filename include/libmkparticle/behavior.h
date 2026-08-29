#ifndef LIBMKPARTICLE_BEHAVIOR_H
#define LIBMKPARTICLE_BEHAVIOR_H

typedef struct PfxVm PfxVm;

typedef struct PfxBehavior {
    char pad00[0x58];
    unsigned char segment_0x58[0x84];
    unsigned char segment_0xDC[0x244];
    unsigned char segment_0x320[0x64];
    void* link_0x384;
} PfxBehavior;

typedef char PfxBehaviorSizeCheck[(sizeof(PfxBehavior) == 0x388) ? 1 : -1];

int pfx_num_behaviors(PfxVm* pfx);
PfxBehavior* pfx_behavior(PfxVm* pfx, int index);

void pfx_behaviors_frame_begin(void* pfx);
void pfx_behaviors_frame_end(void* pfx);

#endif
