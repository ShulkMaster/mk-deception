#ifndef LIBMKPARTICLE_PARTICLE_SYSTEM_H
#define LIBMKPARTICLE_PARTICLE_SYSTEM_H

#include "libmkparticle/vm.h"

struct RwCamera;

void pfxsystem_frame_begin(void);
void pfxsystem_skip_render_frame(void);
void pfxsystem_set_frame_info(int unused0, int unused1, const float* matrix,
                              struct RwCamera* camera);
void pfxsystem_widescreen_offset(int x, int y);
void get_pfxsystem_widescreen_offset(int* out_x, int* out_y);
void pfxsystem_init(void);
void pfxsystem_set_global(int id, float value);
int pfx_frame_begin(PfxVm* pfx);
void pfx_frame_end(PfxVm* pfx);
void pfx_frame_end_check(PfxVm* pfx);
void pfx_count_begin(void);
void pfx_count_end(void);
void pfx_count_add(PfxVm* pfx);

#endif
