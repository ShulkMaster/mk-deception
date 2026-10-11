#ifndef MSLCPP_MSL_UTILITY_H
#define MSLCPP_MSL_UTILITY_H

namespace Metrowerks {

namespace details {

template <class T1, class T2, bool FirstEmpty>
class compressed_pair_imp;

template <class T1, class T2>
class compressed_pair_imp<T1, T2, true> : private T1 {
public:
    compressed_pair_imp() {}
    compressed_pair_imp(T2 y) : second_(y) {}
    compressed_pair_imp(const T1& x, T2 y) : T1(x), second_(y) {}

    T1& first() { return *this; }
    const T1& first() const { return *this; }
    T2& second() { return second_; }
    const T2& second() const { return second_; }

    void swap(compressed_pair_imp& x)
    {
        T2 tmp = second_;
        second_ = x.second_;
        x.second_ = tmp;
    }

private:
    T2 second_;
};

}

template <class T1, class T2>
class compressed_pair : public details::compressed_pair_imp<T1, T2, true> {
    typedef details::compressed_pair_imp<T1, T2, true> base;

public:
    compressed_pair() {}
    compressed_pair(T2 y) : base(y) {}
    compressed_pair(const T1& x, T2 y) : base(x, y) {}

    void swap(compressed_pair& x) { base::swap(x); }
};

template <class T1, class T2>
inline void swap(compressed_pair<T1, T2>& x, compressed_pair<T1, T2>& y)
{
    x.swap(y);
}

}

#endif
