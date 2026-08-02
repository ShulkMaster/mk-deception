#include "msl/mslqueue.h"

extern void* MWSOUND_HEAP;

extern "C" void* _mwMemMalloc(
    void* heap, unsigned long size, int alignment, int arg3, int arg4,
    int arg5);
extern "C" void _mwMemFree(void* allocation, int arg1, int arg2);

extern "C" mslBankSoundEntry* mslQueueGet(mslQueue* queue) {
    mslBankSoundEntry* sound = 0;

    if (queue->write_index != queue->read_index) {
        int read_index = queue->read_index;

        sound = queue->entries[read_index].sound;
        read_index++;
        queue->read_index = read_index;
        if (read_index >= queue->capacity) {
            queue->read_index = 0;
        }
    }

    return sound;
}

extern "C" void mslQueueDelete(mslQueue* queue) {
    if (queue->entries != 0) {
        _mwMemFree(queue->entries, 0, 0);
    }
    _mwMemFree(queue, 0, 0);
}

extern "C" mslQueue* mslQueueNew(int capacity) {
    mslQueue* queue =
        (mslQueue*)_mwMemMalloc(MWSOUND_HEAP, sizeof(mslQueue), 3, 0, 0, 0);

    if (queue == 0) {
        return 0;
    }

    queue->entries = (mslQueueEntry*)_mwMemMalloc(
        MWSOUND_HEAP, capacity * sizeof(mslQueueEntry), 3, 0, 0, 0);
    if (queue->entries == 0) {
        _mwMemFree(queue, 0, 0);
        return 0;
    }

    queue->capacity = capacity;
    queue->read_index = 0;
    queue->write_index = 0;
    return queue;
}
