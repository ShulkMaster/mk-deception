#ifndef MSLCPP_VECTOR_H
#define MSLCPP_VECTOR_H

#include "mslcpp/msl_utility.h"
#include "mslcpp/msl_error.h"
#include "mslcpp/iterator.h"
#include "mslcpp/algorithm.h"

namespace std {

template <class T, class Allocator>
class __vector_deleter {
public:
    typedef Allocator allocator_type;
    typedef unsigned long size_type;
    typedef T* pointer;

    __vector_deleter(const allocator_type& a) : cap_(a, 0), size_(0), data_(0) {}
    ~__vector_deleter();

    void clear();
    void swap(__vector_deleter& x);

    allocator_type& alloc() { return cap_.first(); }
    const allocator_type& alloc() const { return cap_.first(); }
    size_type& cap() { return cap_.second(); }

    Metrowerks::compressed_pair<allocator_type, size_type> cap_;
    size_type size_;
    pointer data_;
};

template <class T, class Allocator>
__vector_deleter<T, Allocator>::~__vector_deleter()
{
    clear();
    if (data_ != 0) {
        alloc().deallocate(data_, cap());
    }
}

template <class T, class Allocator>
void __vector_deleter<T, Allocator>::clear()
{
    pointer first = data_;
    for (pointer e = first + size_; e > first;) {
        alloc().destroy(--e);
    }
    size_ = 0;
}

template <class T, class Allocator>
inline void swap(__vector_deleter<T, Allocator>& x, __vector_deleter<T, Allocator>& y)
{
    if (&x != &y) {
        Metrowerks::swap(x.cap_, y.cap_);
        T* tmp_data = x.data_;
        x.data_ = y.data_;
        y.data_ = tmp_data;
        unsigned long tmp_size = x.size_;
        x.size_ = y.size_;
        y.size_ = tmp_size;
    }
}

template <class T, class Allocator>
void __vector_deleter<T, Allocator>::swap(__vector_deleter& x)
{
    std::swap(*this, x);
}

template <class T, class Allocator>
class __vector_pod {
public:
    typedef Allocator allocator_type;
    typedef unsigned long size_type;
    typedef T value_type;
    typedef T* pointer;
    typedef const T* const_pointer;
    typedef T& reference;
    typedef T* iterator;
    typedef const T* const_iterator;

    __vector_pod() : cap_(0), size_(0), data_(0) {}
    __vector_pod(const allocator_type& a) : cap_(a, 0), size_(0), data_(0) {}
    ~__vector_pod()
    {
        if (data_ != 0) {
            alloc().deallocate(data_, cap());
        }
    }

    iterator begin() { return data_; }
    const_iterator begin() const { return data_; }
    iterator end() { return data_ + size_; }
    const_iterator end() const { return data_ + size_; }

    size_type size() const { return size_; }
    size_type max_size() const { return alloc().max_size(); }
    size_type capacity() const { return cap_.second(); }

    reference operator[](size_type n) { return data_[n]; }

    void reserve(size_type n);
    void resize(size_type sz, const value_type& x);
    void push_back(const value_type& x) { insert(end(), 1, x); }
    void insert(iterator position, size_type n, const value_type& x);
    iterator insert(iterator position, const value_type& x);
    iterator erase(iterator position);
    iterator erase(iterator first, iterator last);

    allocator_type& alloc() { return cap_.first(); }
    const allocator_type& alloc() const { return cap_.first(); }
    size_type& cap() { return cap_.second(); }

    Metrowerks::compressed_pair<allocator_type, size_type> cap_;
    size_type size_;
    pointer data_;
};

/* TODO: [near miss] 99.43%; only the @stringBase0 addend differs (template string pool order). */
template <class T, class Allocator>
void __vector_pod<T, Allocator>::reserve(size_type n)
{
    if (n > cap_.second()) {
        if (n > max_size()) {
            __msl_error("vector::reserve length error\n");
        }
        pointer old = data_;
        data_ = cap_.first().allocate(n);
        if (old != 0) {
            std::copy(old, old + size_, data_);
            alloc().deallocate(old, cap());
        }
        cap_.second() = n;
    }
}

template <class T, class Allocator>
void __vector_pod<T, Allocator>::resize(size_type sz, const value_type& x)
{
    if (sz > size_) {
        insert(data_ + size_, sz - size_, x);
    } else if (sz < size_) {
        erase(data_ + sz, data_ + size_);
    }
}

/* TODO: [near miss] 99.24691%; max-size/old-buffer and suffix-copy register
 * homes differ; two template string-pool addends remain. */
template <class T, class Allocator>
void __vector_pod<T, Allocator>::insert(iterator position, size_type n, const value_type& x)
{
    if (n != 0) {
        size_type ms = max_size();
        if (n > ms || size_ > ms - n) {
            __msl_error("vector::insert length error\n");
        }
        size_type cur_cap = capacity();
        size_type new_size = size_ + n;
        if (new_size <= cur_cap) {
            pointer old_end = data_ + size_;
            const_pointer xr = &x;
            size_type num_move = old_end - position;
            if (old_end - (num_move - n) <= xr && xr < old_end) {
                xr += n;
            }
            if (num_move != 0) {
                pointer src = old_end;
                pointer dst = old_end + n;
                for (; src > position;) {
                    *--dst = *--src;
                }
            }
            pointer p = position;
            for (size_type i = n; i != 0; --i, ++p) {
                *p = *xr;
            }
            size_ += n;
        } else {
            pointer old = data_;
            size_type pos = position - old;
            size_type new_cap = cap_.second() != 0 ? cap_.second() : 1;
            while (new_size > new_cap) {
                if (new_cap < ms / 2) {
                    new_cap *= 2;
                } else {
                    new_cap = ms;
                }
            }
            data_ = cap_.first().allocate(new_cap);
            pointer p = data_ + pos;
            for (size_type i = n; i != 0; --i, ++p) {
                *p = x;
            }
            if (old != 0) {
                std::copy(old, old + pos, data_);
                std::copy(old + pos, old + size_, data_ + pos + n);
                alloc().deallocate(old, cap());
            }
            size_ = new_size;
            cap_.second() = new_cap;
        }
    }
}

template <class T, class Allocator>
inline typename __vector_pod<T, Allocator>::iterator
__vector_pod<T, Allocator>::insert(iterator position, const value_type& x)
{
    size_type n = position - begin();
    insert(position, 1, x);
    return begin() + n;
}

template <class T, class Allocator>
typename __vector_pod<T, Allocator>::iterator
__vector_pod<T, Allocator>::erase(iterator position)
{
    pointer last = data_ + size_;
    if (last - position - 1 != 0) {
        std::copy(position + 1, last, position);
    }
    --size_;
    return position;
}

template <class T, class Allocator>
typename __vector_pod<T, Allocator>::iterator
__vector_pod<T, Allocator>::erase(iterator first, iterator last)
{
    if (first == last) {
        return first;
    }
    pointer old_end = data_ + size_;
    if (old_end - last != 0) {
        std::copy(last, old_end, first);
    }
    size_ -= last - first;
    return first;
}

template <class T, class Allocator, bool IsPOD>
class __vector_imp;

template <class T, class Allocator>
class __vector_imp<T, Allocator, false> : public __vector_deleter<T, Allocator> {
    typedef __vector_deleter<T, Allocator> base;

public:
    typedef Allocator allocator_type;
    typedef unsigned long size_type;
    typedef T value_type;
    typedef T* pointer;
    typedef const T* const_pointer;
    typedef T& reference;
    typedef const T& const_reference;
    typedef T* iterator;

    __vector_imp(const allocator_type& a) : base(a) {}
    __vector_imp(const __vector_imp& x) : base(x.cap_.first())
    {
        const_pointer first = x.data_;
        init(first, first + x.size_, forward_iterator_tag());
    }

    iterator begin() { return this->data_; }
    iterator end() { return this->data_ + this->size_; }
    const_reference front() const { return *this->data_; }

    size_type size() const { return this->size_; }
    size_type max_size() const { return this->alloc().max_size(); }
    size_type capacity() const { return this->cap_.second(); }

    reference operator[](size_type n) { return this->data_[n]; }

    void reserve(size_type n);
    void resize(size_type sz);
    iterator erase(iterator first, iterator last);
    void push_back(const value_type& x) { insert(end(), 1, x); }
    void pop_back()
    {
        --this->size_;
        this->alloc().destroy(this->data_ + this->size_);
    }
    void insert(iterator position, size_type n, const value_type& x);

    template <class ForwardIterator>
    void init(ForwardIterator first, ForwardIterator last, forward_iterator_tag);
};

/* TODO: [breakthrough needed] QueueItem const-range 87.38333%; placement-copy
 * remains outlined; recover per-consumer inline policy and source/pool homes. */
template <class T, class Allocator>
template <class ForwardIterator>
void __vector_imp<T, Allocator, false>::init(ForwardIterator first, ForwardIterator last,
                                            forward_iterator_tag)
{
    size_type n = last - first;
    if (this->size_ > max_size()) {
        __msl_error("vector length error\n");
    }
    if (n != 0) {
        this->data_ = this->cap_.first().allocate(n);
        this->cap_.second() = n;
        for (pointer p = this->data_; first != last; ++first, ++p, ++this->size_) {
            this->alloc().construct(p, *first);
        }
    }
}

template <class T, class Allocator>
void __vector_imp<T, Allocator, false>::resize(size_type sz)
{
    if (sz > this->size_) {
        insert(end(), sz - this->size_, value_type());
    } else if (sz < this->size_) {
        erase(begin() + sz, end());
    }
}

template <class T, class Allocator>
typename __vector_imp<T, Allocator, false>::iterator
__vector_imp<T, Allocator, false>::erase(iterator first, iterator last)
{
    if (first == last) {
        return first;
    }
    pointer e = this->data_ + this->size_;
    long n = e - last;
    if (n != 0) {
        std::copy(last, e, first);
    }
    for (pointer p = first + n; p < e; ++p) {
        this->alloc().destroy(p);
    }
    this->size_ -= last - first;
    return first;
}

/* TODO: [breakthrough needed] QueueItem 90.52778%; placement-copy remains
 * outlined; recover its instantiation policy while preserving exact getter calls. */
template <class T, class Allocator>
void __vector_imp<T, Allocator, false>::reserve(size_type n)
{
    if (n > capacity()) {
        if (n > max_size()) {
            __msl_error("vector::reserve length error\n");
        }
        base tmp(this->cap_.first());
        tmp.data_ = tmp.cap_.first().allocate(n);
        tmp.cap_.second() = n;
        pointer last = this->data_ + this->size_;
        pointer p = tmp.data_;
        for (pointer first = this->data_; first < last; ++first, ++p, ++tmp.size_) {
            tmp.alloc().construct(p, *first);
        }
        tmp.swap(*this);
    }
}

/* TODO: [breakthrough needed] QueueItem 78.10043%, Callback 67.99262%; retain
 * allocator-accessor calls while recovering deferred placement construction. */
template <class T, class Allocator>
void __vector_imp<T, Allocator, false>::insert(iterator position, size_type n,
                                               const value_type& x)
{
    if (n != 0) {
        size_type ms = max_size();
        if (n > ms || this->size_ > ms - n) {
            __msl_error("vector::insert length error\n");
        }
        if (this->size_ + n <= capacity()) {
            pointer old_end = this->data_ + this->size_;
            const_pointer xr = &x;
            size_type num_move = old_end - position;
            if (n > num_move) {
                pointer p = old_end;
                for (; n > num_move; --n, ++p, ++this->size_) {
                    this->alloc().construct(p, x);
                }
                for (pointer first = position; first < old_end; ++first, ++p, ++this->size_) {
                    this->alloc().construct(p, *first);
                }
            } else {
                pointer p = old_end;
                for (pointer first = old_end - n; first < old_end; ++first, ++p, ++this->size_) {
                    this->alloc().construct(p, *first);
                }
                if (old_end - (num_move - n) <= xr && xr < old_end) {
                    xr += n;
                }
                copy_backward(position, old_end - n, old_end);
            }
            for (pointer p = position; n != 0; --n, ++p) {
                *p = *xr;
            }
        } else {
            base tmp(this->alloc());
            size_type new_size = this->size_ + n;
            size_type new_cap = this->cap_.second() != 0 ? this->cap_.second() : 1;
            while (new_size > new_cap) {
                if (new_cap < ms / 2) {
                    new_cap *= 2;
                } else {
                    new_cap = ms;
                }
            }
            tmp.data_ = tmp.cap_.first().allocate(new_cap);
            tmp.cap_.second() = new_cap;
            pointer p = tmp.data_;
            pointer first = this->data_;
            pointer last = this->data_ + this->size_;
            for (; first < position; ++first, ++p, ++tmp.size_) {
                tmp.alloc().construct(p, *first);
            }
            for (; n != 0; --n, ++p, ++tmp.size_) {
                tmp.alloc().construct(p, x);
            }
            for (; first < last; ++first, ++p, ++tmp.size_) {
                tmp.alloc().construct(p, *first);
            }
            tmp.swap(*this);
        }
    }
}

template <class T, class Allocator>
class __vector_imp<T*, Allocator, true>
    : public __vector_pod<unsigned long,
                          typename Allocator::template rebind<unsigned long>::other> {
    typedef typename Allocator::template rebind<unsigned long>::other base_allocator;
    typedef __vector_pod<unsigned long, base_allocator> base;

public:
    typedef Allocator allocator_type;
    typedef unsigned long size_type;
    typedef T* value_type;
    typedef T** iterator;
    typedef T* const* const_iterator;
    typedef T*& reference;

    __vector_imp() {}
    __vector_imp(const allocator_type& a) : base(base_allocator(a)) {}

    iterator begin() { return (iterator)base::begin(); }
    const_iterator begin() const { return (const_iterator)base::begin(); }
    iterator end() { return (iterator)base::end(); }
    const_iterator end() const { return (const_iterator)base::end(); }
    reference operator[](size_type n) { return (reference)base::operator[](n); }

    void push_back(const value_type& x) { base::push_back((const unsigned long&)x); }
    iterator insert(iterator position, const value_type& x)
    {
        return (iterator)base::insert((unsigned long*)position, (const unsigned long&)x);
    }
    void resize(size_type sz) { base::resize(sz, 0); }
    iterator erase(iterator position) { return (iterator)base::erase((unsigned long*)position); }
    iterator erase(iterator first, iterator last)
    {
        return (iterator)base::erase((unsigned long*)first, (unsigned long*)last);
    }
};

template <class T, class Allocator>
class vector : public __vector_imp<T, Allocator, false> {
    typedef __vector_imp<T, Allocator, false> base;

public:
    typedef Allocator allocator_type;
    typedef unsigned long size_type;
    typedef T value_type;
    typedef T* iterator;
    typedef T& reference;
    typedef const T& const_reference;

    explicit vector(const allocator_type& a) : base(a) {}
    vector(const vector& x) : base(x) {}

    iterator begin() { return base::begin(); }
    iterator end() { return base::end(); }
    const_reference front() const { return base::front(); }

    bool empty() const { return this->size_ == 0; }
    reference operator[](size_type n) { return base::operator[](n); }

    void push_back(const value_type& x) { base::push_back(x); }
    void pop_back() { base::pop_back(); }
};

template <class T, class Allocator>
class vector<T*, Allocator> : public __vector_imp<T*, Allocator, true> {
    typedef __vector_imp<T*, Allocator, true> base;

public:
    typedef Allocator allocator_type;
    typedef unsigned long size_type;
    typedef T* value_type;
    typedef T** iterator;
    typedef T* const* const_iterator;
    typedef T*& reference;

    vector() {}
    explicit vector(const allocator_type& a) : base(a) {}

    iterator begin() { return base::begin(); }
    const_iterator begin() const { return base::begin(); }
    iterator end() { return base::end(); }
    const_iterator end() const { return base::end(); }

    size_type size() const { return base::size(); }
    bool empty() const { return this->size_ == 0; }
    reference operator[](size_type n) { return base::operator[](n); }

    void push_back(const value_type& x) { base::push_back(x); }
    iterator insert(iterator position, const value_type& x) { return base::insert(position, x); }
    void resize(size_type sz) { base::resize(sz); }
};

}

#endif
