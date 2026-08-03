#ifndef MSL_QUEUE_H
#define MSL_QUEUE_H

typedef struct mslBankSoundEntry mslBankSoundEntry;

typedef struct mslQueueEntry {
    mslBankSoundEntry* sound;     /* +0x00 */
    unsigned long reserved;       /* +0x04 */
} mslQueueEntry; /* 0x08 */

typedef struct mslQueue {
    mslQueueEntry* entries;       /* +0x00 */
    int capacity;                 /* +0x04 */
    int write_index;              /* +0x08 */
    int read_index;               /* +0x0C */
} mslQueue; /* 0x10 */

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
