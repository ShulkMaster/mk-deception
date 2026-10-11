#ifndef MW_MWFILEQUEUE_H
#define MW_MWFILEQUEUE_H

#include "mw/mwFileMemAllocator.h"
#include "mw/mwFileMutex.h"
#include "mslcpp/queue.h"

class mwFileCondition : public OSCond {
public:
    mwFileCondition();
    ~mwFileCondition();
    void wait(mwFileMutex& mutex);
    void signal();
};

class mwFileEvent {
public:
    mwFileEvent();
    void reset();
    void set();

private:
    OSMutex mutex;
    OSCond condition;
    unsigned char signaled;
};

template <class T, class Allocator>
class circular_buffer {
public:
    circular_buffer(Allocator a) : buffer(a) {}

    void resize(unsigned long size)
    {
        tail = 0;
        head = 0;
        buffer.resize(size);
    }

    bool full() const;
    bool empty() const { return count == 0; }
    bool push(T value);
    T front();
    void pop();

private:
    std::vector<T, Allocator> buffer;
    unsigned long head;
    unsigned long tail;
    unsigned long count;
};

template <class T, class Allocator>
bool circular_buffer<T, Allocator>::full() const
{
    return count == buffer.size();
}

template <class T, class Allocator>
bool circular_buffer<T, Allocator>::push(T value)
{
    if (full()) {
        return false;
    }
    buffer[tail] = value;
    tail = (tail + 1) % buffer.size();
    count++;
    return true;
}

template <class T, class Allocator>
T circular_buffer<T, Allocator>::front()
{
    return buffer[head];
}

template <class T, class Allocator>
void circular_buffer<T, Allocator>::pop()
{
    head = (head + 1) % buffer.size();
    count--;
}

template <class T>
class mwProducerConsumerQueue {
public:
    mwProducerConsumerQueue() : buffer(mwFileMemAllocator<T, 3>()) {}

    void resize(unsigned long size);
    void produce(T value);
    bool consumeNonBlocking(T& value);

    mutable mwFileMutex mutex;
    mwFileCondition condition;
    circular_buffer<T, mwFileMemAllocator<T, 3> > buffer;
};

template <class T>
void mwProducerConsumerQueue<T>::resize(unsigned long size)
{
    mwFileMutexLock lock(mutex);
    buffer.resize(size);
}

template <class T>
void mwProducerConsumerQueue<T>::produce(T value)
{
    mutex.lock();
    while (buffer.full()) {
        condition.wait(mutex);
    }
    buffer.push(value);
    condition.signal();
    mutex.unlock();
}

template <class T>
bool mwProducerConsumerQueue<T>::consumeNonBlocking(T& value)
{
    mwFileMutexLock lock(mutex);
    if (buffer.empty()) {
        return false;
    }
    value = buffer.front();
    buffer.pop();
    condition.signal();
    return true;
}

template <class T, class Compare>
class reserve_priority_queue
    : public std::priority_queue<T, std::vector<T, mwFileMemAllocator<T, 3> >, Compare> {
    typedef std::priority_queue<T, std::vector<T, mwFileMemAllocator<T, 3> >, Compare> base;

public:
    reserve_priority_queue(const mwFileMemAllocator<T, 3>& a)
        : base(Compare(), std::vector<T, mwFileMemAllocator<T, 3> >(a))
    {
    }

    void reserve(unsigned long n) { this->c.reserve(n); }
};

#endif
