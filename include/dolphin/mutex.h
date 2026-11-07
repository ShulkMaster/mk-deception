#ifndef DOLPHIN_MUTEX_H
#define DOLPHIN_MUTEX_H

typedef struct OSMutex OSMutex;
typedef struct OSThread OSThread;

#ifdef __cplusplus
extern "C" {
#endif

void OSInitMutex(OSMutex* mutex);
void OSLockMutex(OSMutex* mutex);
void OSUnlockMutex(OSMutex* mutex);
int OSTryLockMutex(OSMutex* mutex);

#ifdef __cplusplus
}
#endif

#endif
