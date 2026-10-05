#ifndef MW_MWFILEMEMTRAITS_H
#define MW_MWFILEMEMTRAITS_H

#include "mw/mwTargetMemAlign.h"

class mwFileMemTraits {
public:
    static void deallocate(void* ptr);
    static void* allocate(unsigned long size, mwTargetMemAlign align, const char* name);
};

#endif
