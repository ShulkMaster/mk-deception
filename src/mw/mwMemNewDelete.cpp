#include "mw/mwMemHeap.h"
#include "mw/mwMemNewDelete.h"

void operator delete(void* ptr) {
    if (ptr != 0) {
        _mwMemFree(ptr, 0, 0);
    }
}

void* operator new(unsigned long size, _mwMemHeap* heap, mwMemFlags flags,
                   const char* file, const char* function, unsigned int line) {
    return _mwMemMalloc(heap, size, flags, file, function, line);
}

void* operator new(unsigned long size) {
    if (mwMemSystemIsCreated() == 0) {
        mwMemUserConfigInitMemSystem();
    }
    return _mwMemMalloc(mwMemSystemGetHeap(2), size, 0x10, 0, 0, 0);
}
