#ifndef MW_MWMEM_NEW_DELETE_H
#define MW_MWMEM_NEW_DELETE_H

#include "mw/mwMem.h"

#ifdef __cplusplus

void operator delete(void* pointer);
void* operator new(unsigned long size);
void* operator new(unsigned long size, _mwMemHeap* heap, mwMemFlags flags,
                   const char* file, const char* function,
                   unsigned int line);

#endif

void* __nw__FUl(unsigned long size);
void __dl__FPv(void* p);
#endif
