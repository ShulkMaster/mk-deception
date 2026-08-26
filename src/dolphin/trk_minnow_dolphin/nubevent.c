#include "dolphin/trk.h"

typedef struct TRKEventQueue {
    u32 mutex;
    int count;
    int next;
    TRKEvent events[2];
    u32 next_event_id;
} TRKEventQueue;

TRKEventQueue gTRKEventQueue;

extern void* TRK_memcpy(void* destination, const void* source, u32 size);

DSError TRKInitializeEventQueue(void)
{
    TRKInitializeMutex(&gTRKEventQueue);
    TRKAcquireMutex(&gTRKEventQueue);
    gTRKEventQueue.count = 0;
    gTRKEventQueue.next = 0;
    gTRKEventQueue.next_event_id = 0x100;
    TRKReleaseMutex(&gTRKEventQueue);
    return 0;
}

BOOL TRKGetNextEvent(TRKEvent* event)
{
    BOOL result = 0;

    TRKAcquireMutex(&gTRKEventQueue);
    if (gTRKEventQueue.count > 0) {
        TRK_memcpy(event, &gTRKEventQueue.events[gTRKEventQueue.next], sizeof(TRKEvent));
        gTRKEventQueue.count--;
        if (++gTRKEventQueue.next == 2)
            gTRKEventQueue.next = 0;
        result = 1;
    }
    TRKReleaseMutex(&gTRKEventQueue);
    return result;
}

DSError TRKPostEvent(TRKEvent* event)
{
    DSError result = 0;
    int next;

    TRKAcquireMutex(&gTRKEventQueue);
    if (gTRKEventQueue.count == 2) {
        result = 0x100;
    } else {
        next = (gTRKEventQueue.next + gTRKEventQueue.count) % 2;
        TRK_memcpy(&gTRKEventQueue.events[next], event, sizeof(TRKEvent));
        gTRKEventQueue.events[next].event_id = gTRKEventQueue.next_event_id;
        if (++gTRKEventQueue.next_event_id < 0x100)
            gTRKEventQueue.next_event_id = 0x100;
        gTRKEventQueue.count++;
    }
    TRKReleaseMutex(&gTRKEventQueue);
    return result;
}

void TRKConstructEvent(TRKEvent* event, int event_type)
{
    event->event_type = event_type;
    event->event_id = 0;
    event->message_buffer_id = -1;
}

void TRKDestructEvent(TRKEvent* event)
{
    TRKReleaseBuffer(event->message_buffer_id);
}
