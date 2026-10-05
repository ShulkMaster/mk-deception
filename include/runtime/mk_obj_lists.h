#ifndef MK_OBJ_LISTS_H
#define MK_OBJ_LISTS_H

struct MkObj;
struct MkPtr;

#ifdef __cplusplus
extern "C" {
#endif

struct MkPtr* insert_particle_mkobj(struct MkObj* obj);
void ground_me(void* obj);

#ifdef __cplusplus
}
#endif

#endif
