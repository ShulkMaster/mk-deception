#ifndef LIBMKPARTICLE_PARTICLE_RENDER_H
#define LIBMKPARTICLE_PARTICLE_RENDER_H

struct PfxRenderView;
struct RwTexture;

void pfx_set_texture(struct PfxRenderView* pfx, struct RwTexture* texture);
void pfx_set_renderstate(struct PfxRenderView* pfx);
void pfx_render_set_blendmode(struct PfxRenderView* pfx, int mode);

#endif
