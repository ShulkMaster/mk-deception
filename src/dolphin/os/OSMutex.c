#include "dolphin/os.h"

#define ENQUEUE_MUTEX(mutex, queue)                 \
    do {                                            \
        OSMutex* previous = (queue)->tail;          \
        if (previous == 0) {                        \
            (queue)->head = (mutex);                \
        } else {                                    \
            previous->link.next = (mutex);          \
        }                                           \
        (mutex)->link.prev = previous;              \
        (mutex)->link.next = 0;                     \
        (queue)->tail = (mutex);                    \
    } while (0)

#define DEQUEUE_MUTEX(mutex, queue)                 \
    do {                                            \
        OSMutex* next = (mutex)->link.next;         \
        OSMutex* previous = (mutex)->link.prev;     \
        if (next == 0) {                            \
            (queue)->tail = previous;               \
        } else {                                    \
            next->link.prev = previous;             \
        }                                           \
        if (previous == 0) {                        \
            (queue)->head = next;                   \
        } else {                                    \
            previous->link.next = next;             \
        }                                           \
    } while (0)

void OSInitMutex(OSMutex* mutex)
{
    OSInitThreadQueue(&mutex->queue);
    mutex->thread = 0;
    mutex->count = 0;
}

void OSLockMutex(OSMutex* mutex)
{
    int enabled = OSDisableInterrupts();
    OSThread* current = OSGetCurrentThread();

    for (;;) {
        OSThread* owner = mutex->thread;
        if (owner == 0) {
            mutex->thread = current;
            ++mutex->count;
            ENQUEUE_MUTEX(mutex, &current->queueMutex);
            break;
        }
        if (owner == current) {
            ++mutex->count;
            break;
        }
        current->mutex = mutex;
        __OSPromoteThread(mutex->thread, current->priority);
        OSSleepThread(&mutex->queue);
        current->mutex = 0;
    }
    OSRestoreInterrupts(enabled);
}

void OSUnlockMutex(OSMutex* mutex)
{
    int enabled = OSDisableInterrupts();
    OSThread* current = OSGetCurrentThread();

    if (mutex->thread == current && --mutex->count == 0) {
        DEQUEUE_MUTEX(mutex, &current->queueMutex);
        mutex->thread = 0;
        if (current->priority < current->base) {
            current->priority = __OSGetEffectivePriority(current);
        }
        OSWakeupThread(&mutex->queue);
    }
    OSRestoreInterrupts(enabled);
}

void __OSUnlockAllMutex(OSThread* thread)
{
    while (thread->queueMutex.head != 0) {
        OSMutex* mutex = thread->queueMutex.head;
        OSMutex* next = mutex->link.next;
        if (next == 0) {
            thread->queueMutex.tail = 0;
        } else {
            next->link.prev = 0;
        }
        thread->queueMutex.head = next;
        mutex->count = 0;
        mutex->thread = 0;
        OSWakeupThread(&mutex->queue);
    }
}

int OSTryLockMutex(OSMutex* mutex)
{
    int enabled = OSDisableInterrupts();
    OSThread* current = OSGetCurrentThread();
    int locked;

    if (mutex->thread == 0) {
        mutex->thread = current;
        ++mutex->count;
        ENQUEUE_MUTEX(mutex, &current->queueMutex);
        locked = 1;
    } else if (mutex->thread == current) {
        ++mutex->count;
        locked = 1;
    } else {
        locked = 0;
    }
    OSRestoreInterrupts(enabled);
    return locked;
}

void OSInitCond(OSCond* condition)
{
    OSInitThreadQueue(&condition->queue);
}

void OSWaitCond(OSCond* condition, OSMutex* mutex)
{
    int enabled = OSDisableInterrupts();
    OSThread* current = OSGetCurrentThread();

    if (mutex->thread == current) {
        signed long count = mutex->count;
        mutex->count = 0;
        DEQUEUE_MUTEX(mutex, &current->queueMutex);
        mutex->thread = 0;
        if (current->priority < current->base) {
            current->priority = __OSGetEffectivePriority(current);
        }
        OSDisableScheduler();
        OSWakeupThread(&mutex->queue);
        OSEnableScheduler();
        OSSleepThread(&condition->queue);
        OSLockMutex(mutex);
        mutex->count = count;
    }
    OSRestoreInterrupts(enabled);
}

void OSSignalCond(OSCond* condition)
{
    OSWakeupThread(&condition->queue);
}
