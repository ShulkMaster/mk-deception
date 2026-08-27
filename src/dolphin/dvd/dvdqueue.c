#include "dolphin/dvd.h"
#include "dolphin/os.h"

typedef struct DVDWaitingQueue {
    DVDCommandBlock* next;
    DVDCommandBlock* prev;
} DVDWaitingQueue;

static DVDWaitingQueue WaitingQueue[4];

void __DVDClearWaitingQueue(void)
{
    unsigned long priority;
    DVDCommandBlock* queue;

    for (priority = 0; priority < 4; priority++) {
        queue = (DVDCommandBlock*)&WaitingQueue[priority];
        queue->next = queue;
        queue->prev = queue;
    }
}

int __DVDPushWaitingQueue(int priority, DVDCommandBlock* block)
{
    int enabled = OSDisableInterrupts();
    DVDCommandBlock* queue = (DVDCommandBlock*)&WaitingQueue[priority];

    queue->prev->next = block;
    block->prev = queue->prev;
    block->next = queue;
    queue->prev = block;
    OSRestoreInterrupts(enabled);
    return 1;
}

static inline DVDCommandBlock* PopWaitingQueuePriority(int priority)
{
    DVDCommandBlock* block;
    int enabled;
    DVDCommandBlock* queue;

    enabled = OSDisableInterrupts();
    queue = (DVDCommandBlock*)&WaitingQueue[priority];
    block = queue->next;
    queue->next = block->next;
    block->next->prev = queue;
    OSRestoreInterrupts(enabled);
    block->next = 0;
    block->prev = 0;
    return block;
}

DVDCommandBlock* __DVDPopWaitingQueue(void)
{
    unsigned long priority;
    int enabled;
    DVDCommandBlock* queue;

    enabled = OSDisableInterrupts();
    for (priority = 0; priority < 4; priority++) {
        queue = (DVDCommandBlock*)&WaitingQueue[priority];
        if (queue->next != queue) {
            OSRestoreInterrupts(enabled);
            return PopWaitingQueuePriority(priority);
        }
    }

    OSRestoreInterrupts(enabled);
    return 0;
}

int __DVDCheckWaitingQueue(void)
{
    unsigned long priority;
    int enabled;
    DVDCommandBlock* queue;

    enabled = OSDisableInterrupts();
    for (priority = 0; priority < 4; priority++) {
        queue = (DVDCommandBlock*)&WaitingQueue[priority];
        if (queue->next != queue) {
            OSRestoreInterrupts(enabled);
            return 1;
        }
    }

    OSRestoreInterrupts(enabled);
    return 0;
}

int __DVDDequeueWaitingQueue(DVDCommandBlock* block)
{
    int enabled;
    DVDCommandBlock* previous;
    DVDCommandBlock* next;

    enabled = OSDisableInterrupts();
    previous = block->prev;
    next = block->next;
    if (previous == 0 || next == 0) {
        OSRestoreInterrupts(enabled);
        return 0;
    }

    previous->next = next;
    next->prev = previous;
    OSRestoreInterrupts(enabled);
    return 1;
}
