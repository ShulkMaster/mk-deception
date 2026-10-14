#ifndef MSLCPP_ALGORITHM_H
#define MSLCPP_ALGORITHM_H

#include "mslcpp/iterator.h"

namespace std {

template <class InputIterator, class T>
inline InputIterator find(InputIterator first, InputIterator last, const T& value)
{
    while (first != last && !(*first == value)) {
        ++first;
    }
    return first;
}

template <class T>
inline const T& min(const T& a, const T& b)
{
    return b < a ? b : a;
}

template <class T>
inline const T& max(const T& a, const T& b)
{
    return a < b ? b : a;
}

template <class ForwardIterator, class T>
inline void replace(ForwardIterator first, ForwardIterator last, const T& old_value,
                    const T& new_value)
{
    for (; first != last; ++first) {
        if (*first == old_value) {
            *first = new_value;
        }
    }
}

template <class ForwardIterator, class T, class Compare>
ForwardIterator lower_bound(ForwardIterator first, ForwardIterator last, const T& value,
                            Compare comp)
{
    typedef typename iterator_traits<ForwardIterator>::difference_type difference_type;
    difference_type len = last - first;
    while (len > 0) {
        ForwardIterator i = first;
        difference_type half = len / 2;
        i += half;
        if (comp(*i, value)) {
            first = ++i;
            len -= half + 1;
        } else {
            len = half;
        }
    }
    return first;
}

template <class T>
inline T* copy(T* first, T* last, T* result)
{
    for (; first < last; ++first, ++result) {
        *result = *first;
    }
    return result;
}

template <class InputIterator, class OutputIterator, class T>
OutputIterator remove_copy(InputIterator first, InputIterator last, OutputIterator result,
                           const T& value)
{
    for (; first != last; ++first) {
        if (!(*first == value)) {
            *result = *first;
            ++result;
        }
    }
    return result;
}

template <class ForwardIterator, class T>
inline ForwardIterator remove(ForwardIterator first, ForwardIterator last, const T& value)
{
    while (first != last && !(*first == value)) {
        ++first;
    }
    if (first == last) {
        return first;
    }
    ForwardIterator i = first;
    return std::remove_copy(++i, last, first, value);
}

template <class T>
inline T* copy_backward(T* first, T* last, T* result)
{
    while (last > first) {
        *--result = *--last;
    }
    return result;
}

template <class ForwardIterator1, class ForwardIterator2>
inline void iter_swap(ForwardIterator1 a, ForwardIterator2 b)
{
    typename iterator_traits<ForwardIterator1>::value_type tmp;
    tmp = *a;
    *a = *b;
    *b = tmp;
}

template <class RandomAccessIterator, class Distance, class T, class Compare>
void __insert_heap_down(RandomAccessIterator first, Distance len, Distance hole,
                        const T& value, Compare comp)
{
    RandomAccessIterator hole_p = first + hole;
    Distance child = 2 * (hole + 1);
    while (child <= len) {
        RandomAccessIterator child_p = first + child;
        if (child == len || comp(*child_p, *(child_p - 1))) {
            --child;
            --child_p;
        }
        if (!comp(value, *child_p)) {
            break;
        }
        *hole_p = *child_p;
        hole_p = child_p;
        ++child;
        child *= 2;
    }
    *hole_p = value;
}

template <class RandomAccessIterator, class Compare>
void push_heap(RandomAccessIterator first, RandomAccessIterator last, Compare comp)
{
    typedef typename iterator_traits<RandomAccessIterator>::difference_type difference_type;
    typedef typename iterator_traits<RandomAccessIterator>::value_type value_type;
    difference_type len = last - first;
    if (len < 2) {
        return;
    }
    difference_type parent = (len - 2) / 2;
    RandomAccessIterator parent_p = first + parent;
    RandomAccessIterator hole_p = first + (len - 1);
    if (comp(*parent_p, *hole_p)) {
        value_type tmp;
        tmp = *hole_p;
        do {
            *hole_p = *parent_p;
            hole_p = parent_p;
            if (parent_p == first) {
                break;
            }
            --parent;
            parent /= 2;
            parent_p = first + parent;
        } while (comp(*parent_p, tmp));
        *hole_p = tmp;
    }
}

template <class RandomAccessIterator, class Compare>
void pop_heap(RandomAccessIterator first, RandomAccessIterator last, Compare comp)
{
    typedef typename iterator_traits<RandomAccessIterator>::difference_type difference_type;
    typedef typename iterator_traits<RandomAccessIterator>::value_type value_type;
    difference_type len = last - first;
    if (len >= 2) {
        if (len == 2) {
            std::iter_swap(first, last - 1);
        } else {
            --last;
            value_type tmp;
            tmp = *last;
            *last = *first;
            __insert_heap_down(first, len - 1, difference_type(0), tmp, comp);
        }
    }
}

template <class RandomAccessIterator, class Compare>
void make_heap(RandomAccessIterator first, RandomAccessIterator last, Compare comp)
{
    typedef typename iterator_traits<RandomAccessIterator>::difference_type difference_type;
    typedef typename iterator_traits<RandomAccessIterator>::value_type value_type;
    difference_type len = last - first;
    if (len < 2) {
        return;
    }
    difference_type parent = (len - 2) / 2;
    RandomAccessIterator parent_p = first + parent;
    difference_type child = 2 * (parent + 1);
    RandomAccessIterator child_p = first + child;
    RandomAccessIterator left_p = child_p - 1;
    difference_type c = child;
    RandomAccessIterator cp = child_p;
    if (child == len || comp(*child_p, *left_p)) {
        cp = left_p;
        --c;
    }
    while (true) {
        if (comp(*parent_p, *cp)) {
            value_type tmp;
            tmp = *parent_p;
            *parent_p = *cp;
            __insert_heap_down(first, len, c, tmp, comp);
        }
        if (parent-- == 0) {
            break;
        }
        child_p -= 2;
        child -= 2;
        left_p -= 2;
        c = child;
        cp = child_p;
        --parent_p;
        if (comp(*child_p, *left_p)) {
            cp = left_p;
            c = child - 1;
        }
    }
}

}

#endif
