#ifndef MW_MWFILECONTAINERS_H
#define MW_MWFILECONTAINERS_H

template <class T, int Alignment>
class mwFileMemAllocator {
};

template <class T>
class pointer_less {
};

/* TODO: [Scope warn] Pointer-vector ABI declarations; template bodies remain retail. */
namespace std {
template <class T, class Allocator>
class vector {
public:
    vector();
    ~vector();
    const T* begin() const;
    const T* end() const;

private:
    unsigned long capacity;
    unsigned long size_value;
    T* storage;
};

template <class Iterator, class T, class Compare>
Iterator lower_bound(Iterator first, Iterator last, const T& value,
                     Compare compare);
}

#endif
