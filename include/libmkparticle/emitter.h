#ifndef LIBMKPARTICLE_EMITTER_H
#define LIBMKPARTICLE_EMITTER_H

#include "libmkparticle/vm.h"

int pfx_emitter_exhausted(PfxVmEmitter* emitter);
int pfx_emitter_unused(PfxVmEmitter* emitter);
int pfx_emitter_restart_cycle(PfxVmEmitter* emitter);
void pfx_emitter_reset(PfxVmEmitter* emitter);
int _pfx_emitter_get_birthcount(PfxVmEmitter* emitter, PfxVm* pfx,
                                float frame_time);
void pfx_emitter_run_frame(PfxVm* pfx, int emitter_index, float frame_time);
void pfx_emitter_scan_for_fields(PfxVmEmitter* emitter,
                                 unsigned int* fields);
PfxEmitterInstruction* pfx_emitter_find_insn(PfxVmEmitter* emitter,
                                             int description);
int pfx_emitter_insn_is_static(PfxEmitterInstruction* instruction);

#endif
