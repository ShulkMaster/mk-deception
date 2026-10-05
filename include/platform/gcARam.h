#ifndef PLATFORM_GCARAM_H
#define PLATFORM_GCARAM_H

#include "dolphin/types.h"
#include "mw/mwMem.h"

extern _mwMemHeap* SystemSwappableHeap;

void gc_aram_mwmem_heap_setup(void);
void gc_aram_init(void);
u32 ARAM_MSL_GetSize(void);
u32 ARAM_MSL_GetBase(void);

#endif
