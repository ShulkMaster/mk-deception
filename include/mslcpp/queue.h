#ifndef MSLCPP_QUEUE_H
#define MSLCPP_QUEUE_H

#include "mslcpp/vector.h"
#include "mslcpp/functional.h"
#include "mslcpp/algorithm.h"

namespace std {

template <class T, class Container, class Compare>
class priority_queue {
public:
    typedef typename Container::value_type value_type;
    typedef typename Container::size_type size_type;
    typedef Container container_type;

    priority_queue(const Compare& x, const Container& y) : c(y), comp(x)
    {
        make_heap(c.begin(), c.end(), comp);
    }

    bool empty() const { return c.empty(); }
    size_type size() const { return c.size(); }
    const value_type& top() const { return c.front(); }

    void push(const value_type& x);
    void pop();

protected:
    Container c;
    Compare comp;
};

template <class T, class Container, class Compare>
void priority_queue<T, Container, Compare>::push(const value_type& x)
{
    c.push_back(x);
    push_heap(c.begin(), c.end(), comp);
}

template <class T, class Container, class Compare>
void priority_queue<T, Container, Compare>::pop()
{
    pop_heap(c.begin(), c.end(), comp);
    c.pop_back();
}

}

#endif
