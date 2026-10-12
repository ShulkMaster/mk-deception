#ifndef MSLCPP_FUNCTIONAL_H
#define MSLCPP_FUNCTIONAL_H

namespace std {

template <class Arg1, class Arg2, class Result>
struct binary_function {
};

template <class T>
struct greater : public binary_function<T, T, bool> {
    bool operator()(const T& x, const T& y) const { return x > y; }
};

}

#endif
