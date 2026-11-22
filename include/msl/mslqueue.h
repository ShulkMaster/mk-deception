#ifndef MSL_QUEUE_H
#define MSL_QUEUE_H

#include "dolphin/types.h"

typedef struct mslBankSoundEntry mslBankSoundEntry;

typedef struct mslQueueEntry {
    mslBankSoundEntry* sound;
    u32 reserved;
} mslQueueEntry;

typedef struct mslQueue {
    mslQueueEntry* entries;
    int capacity;
    int write_index;
    int read_index;
} mslQueue;

#ifdef __cplusplus
extern "C" {
#endif

mslBankSoundEntry* mslQueueGet(mslQueue* queue);
void mslQueueDelete(mslQueue* queue);
mslQueue* mslQueueNew(int capacity);

#ifdef __cplusplus
}
#endif

#endif
