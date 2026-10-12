#ifndef MSLCPP_ITERATOR_H
#define MSLCPP_ITERATOR_H

namespace std {

struct input_iterator_tag {};
struct forward_iterator_tag : public input_iterator_tag {};
struct bidirectional_iterator_tag : public forward_iterator_tag {};
struct random_access_iterator_tag : public bidirectional_iterator_tag {};

template <class Iterator>
struct iterator_traits;

template <class T>
struct iterator_traits<T*> {
    typedef long difference_type;
    typedef T value_type;
};

}

#endif
