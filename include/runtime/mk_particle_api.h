#ifndef RUNTIME_MK_PARTICLE_API_H
#define RUNTIME_MK_PARTICLE_API_H

typedef struct MkPfx MkPfx;
typedef struct PfxClone PfxClone;

int vdestroy_pfx(MkPfx* pfx);
int vdestroy_pfx_clone(PfxClone* clone);

#endif
