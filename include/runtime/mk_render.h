#ifndef MK_RENDER_H
#define MK_RENDER_H

#include "runtime/mk_obj.h"

#ifdef __cplusplus
extern "C" {
#endif

void render_transl_atomics(void);
RpAtomic* set_transl_callback(RpAtomic* atomic, void* data);
void init_mk_render(void);
void InsertPFXCloneInTranslTree(MkHdr* clone);
void InsertPFXInTranslTree(MkHdr* pfx);
void render_mkatomic(RpAtomic* atomic);
void render_mkobj(MkObj* object);

#ifdef __cplusplus
}
#endif

#endif
