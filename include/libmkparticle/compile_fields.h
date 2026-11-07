#ifndef LIBMKPARTICLE_COMPILE_FIELDS_H
#define LIBMKPARTICLE_COMPILE_FIELDS_H

typedef struct PfxTableRegistry PfxTableRegistry;
typedef struct PfxVmEmitter PfxVmEmitter;

void _pfx_emitter_compile(PfxVmEmitter* emitter, PfxTableRegistry* registry);
#endif
