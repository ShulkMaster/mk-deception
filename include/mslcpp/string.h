#ifndef MSLCPP_STRING_H
#define MSLCPP_STRING_H

#include "runtime/cstring.h"
#include "mslcpp/msl_error.h"
#include "mslcpp/algorithm.h"

extern "C" void* memmove(void* destination, const void* source, unsigned long size);

namespace std {

template <class charT>
struct char_traits;

template <>
struct char_traits<char> {
    typedef char char_type;

    static void assign(char_type& c1, const char_type& c2) { c1 = c2; }
    static unsigned long length(const char_type* s) { return strlen(s); }
    static char_type* copy(char_type* s1, const char_type* s2, unsigned long n)
    {
        return (char_type*)memcpy(s1, s2, n);
    }
    static char_type* move(char_type* s1, const char_type* s2, unsigned long n)
    {
        return (char_type*)memmove(s1, s2, n);
    }
    static char_type* assign(char_type* s, unsigned long n, char_type a)
    {
        return (char_type*)memset(s, a, n);
    }
};

template <class T1, class T2>
class _EmptyMemberOpt : public T1 {
public:
    _EmptyMemberOpt(const T1& a) : T1(a), m_(0) {}

    T1& first() { return *this; }
    const T1& first() const { return *this; }

    T2 m_;
};

template <class charT, class traits, class Allocator>
class basic_string {
public:
    typedef traits traits_type;
    typedef Allocator allocator_type;
    typedef unsigned long size_type;
    typedef charT* pointer;
    typedef const charT* const_pointer;
    typedef charT& reference;
    typedef charT* iterator;

    explicit basic_string(const allocator_type& a = allocator_type());
    ~basic_string();

    basic_string& operator=(const charT* s);

    iterator begin();
    iterator end();

    size_type size() const { return size_; }
    size_type length() const { return size_; }
    size_type max_size() const;
    void reserve(size_type res_arg = 0);

    reference operator[](size_type pos);

    basic_string& append(const charT* s, size_type n);
    basic_string& append(const charT* s) { return append(s, traits::length(s)); }
    basic_string& assign(const basic_string& str, size_type pos, size_type n);
    basic_string& assign(const charT* s, size_type n);
    basic_string& replace(size_type pos, size_type n1, size_type n2, charT c);

    const charT* data() const;
    allocator_type get_allocator() const;

private:
    static size_type __round(size_type n) { return n + 3 - (n & 3); }
    void change_size(size_type n, bool keep);

    size_type size_;
    _EmptyMemberOpt<allocator_type, size_type> capacity_;
    pointer data_;
};

template <class charT, class traits, class Allocator>
basic_string<charT, traits, Allocator>::basic_string(const allocator_type& a)
    : size_(0), capacity_(a)
{
    data_ = 0;
    capacity_.m_ = 0;
}

template <class charT, class traits, class Allocator>
basic_string<charT, traits, Allocator>::~basic_string()
{
    if (data_ != 0) {
        capacity_.deallocate(data_, capacity_.m_ + 1);
    }
}

template <class charT, class traits, class Allocator>
typename basic_string<charT, traits, Allocator>::iterator
basic_string<charT, traits, Allocator>::begin()
{
    return data_;
}

template <class charT, class traits, class Allocator>
typename basic_string<charT, traits, Allocator>::iterator
basic_string<charT, traits, Allocator>::end()
{
    return data_ + size_;
}

template <class charT, class traits, class Allocator>
basic_string<charT, traits, Allocator>&
basic_string<charT, traits, Allocator>::operator=(const charT* s)
{
    return assign(s, traits::length(s));
}

template <class charT, class traits, class Allocator>
typename basic_string<charT, traits, Allocator>::reference
basic_string<charT, traits, Allocator>::operator[](size_type pos)
{
    return data_[pos];
}

template <class charT, class traits, class Allocator>
basic_string<charT, traits, Allocator>&
basic_string<charT, traits, Allocator>::append(const charT* s, size_type n)
{
    if (n != 0) {
        size_type old_size = size_;
        change_size(old_size + n, true);
        traits::copy(data_ + old_size, s, n);
    }
    return *this;
}

template <class charT, class traits, class Allocator>
basic_string<charT, traits, Allocator>&
basic_string<charT, traits, Allocator>::assign(const charT* s, size_type n)
{
    change_size(n, false);
    traits::copy(data_, s, n);
    return *this;
}

template <class charT, class traits, class Allocator>
const charT* basic_string<charT, traits, Allocator>::data() const
{
    static const charT __nullstring = charT();
    if (data_ == 0) {
        return &__nullstring;
    }
    return data_;
}

template <class charT, class traits, class Allocator>
void basic_string<charT, traits, Allocator>::change_size(size_type n, bool keep)
{
    if (n > max_size()) {
        __msl_error("basic_string::length error.  Resizing out of range\n");
    }
    if (n > capacity_.m_) {
        size_type min_cap = 3;
        size_type new_cap = std::max(capacity_.m_, min_cap);
        while (new_cap < n) {
            new_cap = __round(2 * new_cap);
        }
        pointer p = capacity_.allocate(new_cap + 1);
        if (data_ != 0) {
            if (keep) {
                traits::copy(p, data_, size_);
            }
            capacity_.deallocate(data_, capacity_.m_ + 1);
        }
        data_ = p;
        capacity_.m_ = new_cap;
    }
    if (data_ != 0) {
        traits::assign(data_[n], charT());
    }
    size_ = n;
}

template <class charT, class traits, class Allocator>
typename basic_string<charT, traits, Allocator>::size_type
basic_string<charT, traits, Allocator>::max_size() const
{
    return get_allocator().max_size() - 1;
}

template <class charT, class traits, class Allocator>
typename basic_string<charT, traits, Allocator>::allocator_type
basic_string<charT, traits, Allocator>::get_allocator() const
{
    return capacity_;
}

template <class charT, class traits, class Allocator>
basic_string<charT, traits, Allocator>&
basic_string<charT, traits, Allocator>::assign(const basic_string& str, size_type pos, size_type n)
{
    if (pos > str.size_) {
        __msl_error("basic_string::assign pos out of range");
    }
    size_type rlen = std::min(n, str.size_ - pos);
    if (this == &str) {
        replace(0, pos, 0, charT());
        change_size(rlen, true);
    } else {
        change_size(rlen, false);
        traits::copy(data_, str.data_ + pos, rlen);
    }
    return *this;
}

template <class charT, class traits, class Allocator>
void basic_string<charT, traits, Allocator>::reserve(size_type res_arg)
{
    if (res_arg > max_size()) {
        __msl_error("basic_string::reserve argument out of range\n");
    }
    if (res_arg < size_) {
        res_arg = size_;
    }
    res_arg = __round(res_arg);
    if (res_arg != capacity_.m_) {
        pointer p = capacity_.allocate(res_arg + 1);
        if (data_ != 0) {
            traits::copy(p, data_, (data_ + size_ + 1) - data_);
            capacity_.deallocate(data_, capacity_.m_ + 1);
        }
        data_ = p;
        capacity_.m_ = res_arg;
    }
}

template <class charT, class traits, class Allocator>
basic_string<charT, traits, Allocator>&
basic_string<charT, traits, Allocator>::replace(size_type pos, size_type n1, size_type n2, charT c)
{
    if (pos > size_) {
        __msl_error("basic_string::replace pos out of range\n");
    }
    size_type xlen = std::min(n1, size_ - pos);
    if (n2 > max_size() || size_ - xlen > max_size() - n2) {
        __msl_error("basic_string::length error\n");
    }
    size_type new_size = n2 + (size_ - xlen);
    if (new_size > capacity_.m_) {
        size_type new_cap = __round(new_size);
        pointer p = capacity_.allocate(new_cap + 1);
        if (data_ != 0) {
            traits::copy(p, data_, pos);
        }
        if (n2 != 0) {
            traits::assign(p + pos, n2, c);
        }
        if (data_ != 0) {
            traits::copy(p + (pos + n2), data_ + (pos + xlen), size_ - (pos + xlen));
        }
        traits::assign(p[new_size], charT());
        if (data_ != 0) {
            capacity_.deallocate(data_, capacity_.m_ + 1);
        }
        data_ = p;
        capacity_.m_ = new_cap;
        size_ = new_size;
    } else if (data_ != 0) {
        traits::move(data_ + (pos + n2), data_ + (pos + xlen), size_ - (pos + xlen));
        if (n2 != 0) {
            traits::assign(data_ + pos, n2, c);
        }
        traits::assign(data_[new_size], charT());
        size_ = new_size;
    }
    return *this;
}

}

#endif
