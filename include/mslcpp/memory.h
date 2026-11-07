#ifndef MSLCPP_MEMORY_H
#define MSLCPP_MEMORY_H

namespace std {

class __allocator_base {
public:
    __allocator_base() {}
    __allocator_base(const __allocator_base&) {}
};

template <class T>
class allocator : public __allocator_base {
public:
    allocator() {}
    allocator(const allocator& x) : __allocator_base(x) {}
    ~allocator() {}
    template <class U>
    allocator(const allocator<U>& x) : __allocator_base(x) {}
};

}

#endif
