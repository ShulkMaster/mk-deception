#ifndef MW_MWFILECONTAINERS_H
#define MW_MWFILECONTAINERS_H

#include "mslcpp/string.h"
#include "mw/mwFileQueue.h"

extern int mwFileStringCompareIgnoreCase(const char* left, const char* right);

template <class T>
struct pointer_less {
    bool operator()(const T* a, const T* b) const
    {
        return mwFileStringCompareIgnoreCase(a->name, b->name) < 0;
    }
};

#endif
