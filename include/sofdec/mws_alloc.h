#ifndef SOFDEC_MWS_ALLOC_H
#define SOFDEC_MWS_ALLOC_H

typedef void* (*MwsMallocFn)(void* object, unsigned int size);
typedef void (*MwsFreeFn)(void* object, void* memory);

#ifdef __cplusplus
extern "C" {
#endif

void mwPlySetMallocFn(MwsMallocFn malloc_fn, MwsFreeFn free_fn, void* object);

#ifdef __cplusplus
}
#endif

#endif
