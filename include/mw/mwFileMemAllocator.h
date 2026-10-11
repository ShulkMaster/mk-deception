#ifndef MW_MWFILEMEMALLOCATOR_H
#define MW_MWFILEMEMALLOCATOR_H

#include "mw/mwTargetMemAlign.h"
#include "mslcpp/memory.h"

class mwFileMultithreadedMemTraits {
public:
    static void* allocate(unsigned long size, mwTargetMemAlign alignment, const char* name);
    static void deallocate(void* ptr);
};

inline void* operator new(unsigned long, void* storage)
{
    return storage;
}

template <class T, int Alignment>
class mwFileMemAllocator : public std::allocator<T> {
public:
    typedef unsigned long size_type;
    typedef T value_type;
    typedef T* pointer;
    typedef const T* const_pointer;
    typedef T& reference;
    typedef const T& const_reference;

    template <class U>
    struct rebind {
        typedef mwFileMemAllocator<U, Alignment> other;
    };

    mwFileMemAllocator() {}
    mwFileMemAllocator(const mwFileMemAllocator& x) : std::allocator<T>(x) {}
    template <class U>
    mwFileMemAllocator(const mwFileMemAllocator<U, Alignment>& x) : std::allocator<T>(x) {}
    ~mwFileMemAllocator() {}

    pointer allocate(size_type n)
    {
        if (n == 0) {
            n = 1;
        }
        return (pointer)mwFileMultithreadedMemTraits::allocate(
            n * sizeof(T), (mwTargetMemAlign)Alignment, "STL allocation");
    }

    void deallocate(pointer p, size_type)
    {
        if (p != 0) {
            mwFileMultithreadedMemTraits::deallocate(p);
        }
    }

    size_type max_size() const { return (size_type)-1 / sizeof(T); }

    void construct(pointer p, const T& x) { new (p) T(x); }
    void destroy(pointer p) { p->~T(); }
};

#endif
