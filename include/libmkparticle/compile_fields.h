#ifndef LIBMKPARTICLE_COMPILE_FIELDS_H
#define LIBMKPARTICLE_COMPILE_FIELDS_H

#include "libmkparticle/table.h"
#include "libmkparticle/vm.h"

typedef PfxVmEmitter PfxEmitterCompileView;

void _pfx_emitter_compile(PfxVmEmitter* emitter, PfxTableRegistry* registry);
#endif
