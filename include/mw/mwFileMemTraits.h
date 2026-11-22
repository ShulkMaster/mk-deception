#ifndef MW_MWFILEMEMTRAITS_H
#define MW_MWFILEMEMTRAITS_H

#include "dolphin/types.h"
#include "mw/mwTargetMemAlign.h"

class mwFileMemTraits {
public:
    static void deallocate(void* ptr);
    static void* allocate(size_t size, mwTargetMemAlign align, const char* name);
};

#endif
