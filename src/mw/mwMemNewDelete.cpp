struct _mwMemHeap;

enum mwMemFlags {
    MWMEM_DEFAULT = 0
};

extern "C" {
void* _mwMemMalloc(_mwMemHeap* heap, unsigned long size, int flags, void* file,
                   void* function, void* line);
void _mwMemFree(void* ptr, int a, int b);
int mwMemSystemIsCreated(void);
void mwMemUserConfigInitMemSystem(void);
_mwMemHeap* mwMemSystemGetHeap(int which);
}

void operator delete(void* ptr) {
    if (ptr != 0) {
        _mwMemFree(ptr, 0, 0);
    }
}

void* operator new(unsigned long size, _mwMemHeap* heap, mwMemFlags flags,
                   const char* file, const char* function, unsigned int line) {
    return _mwMemMalloc(heap, size, flags, (void*)file, (void*)function,
                        (void*)line);
}

void* operator new(unsigned long size) {
    if (mwMemSystemIsCreated() == 0) {
        mwMemUserConfigInitMemSystem();
    }
    return _mwMemMalloc(mwMemSystemGetHeap(2), size, 0x10, 0, 0, 0);
}
