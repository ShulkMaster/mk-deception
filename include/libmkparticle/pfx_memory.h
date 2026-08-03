#ifndef LIBMKPARTICLE_PFX_MEMORY_H
#define LIBMKPARTICLE_PFX_MEMORY_H

typedef struct PfxBuildInfo {
    int field_00;
    int field_04;
    int flag;
    int field_0C;
    int field_10;
    void* userdata;
    char* name;
} PfxBuildInfo;

typedef struct PfxEstimate {
    char pad00[0x40];
    int size;
    char pad44[0x0C];
} PfxEstimate;

void pfx_estimate_size(void* pfx, PfxEstimate* estimate, PfxBuildInfo* build);
void pfx_set_memory(void* pfx, void* memory, PfxEstimate* estimate);

#endif
