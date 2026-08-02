#ifndef LIBMKPARTICLE_GC_2D_H
#define LIBMKPARTICLE_GC_2D_H

#include "libmkparticle/pfx2d.h"

/*
 * GC native 2D draw path used by pfx2d (menu ScreenObj / ScreenPoly chrome).
 *
 * Retail: pfx2d_render -> native2d_instance_geometry (cpu verts->gpu[])
 *         pfx2d_end_render -> native2d_draw (GX_QUADS via WGPIPE).
 * Requires a live RwTexture + raster (NULL TGA fails upstream).
 *
 * Soft ceilings: native2d_draw ~97.9% (r3<->r4 y/WGPIPE + first alpha lwz);
 * instance_geometry ~92.1% (raster r5 vs r4 + ptr++ vs li offs).
 * Matched: init/begin/end/set/reset/init_object.
 */

int native2d_init(int pool_size);
void native2d_begin_render(void);
void native2d_end_render(void);
void native2d_set_renderstate(void);
void native2d_reset_renderstate(void);
void native2d_draw(Pfx2dObj* obj);
void native2d_instance_geometry(Pfx2dObj* obj);
void native2d_init_object(Pfx2dObj* obj);

#endif
