#ifndef MSLCPP_MEMORY_H
#define MSLCPP_MEMORY_H

namespace std {

template <class T>
class allocator {
public:
    allocator() {}
    allocator(const allocator&) {}
    template <class U>
    allocator(const allocator<U>&) {}
};

}

#endif
