#ifndef PLATFORM_GCARAM_H
#define PLATFORM_GCARAM_H

#include "dolphin/types.h"

void gc_aram_mwmem_heap_setup(void);
void gc_aram_init(void);
u32 ARAM_MSL_GetSize(void);
u32 ARAM_MSL_GetBase(void);

#endif
