#ifndef DOLPHIN_MUTEX_H
#define DOLPHIN_MUTEX_H

#include "platform/os_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void OSInitMutex(OSMutex* mutex);
void OSLockMutex(OSMutex* mutex);
void OSUnlockMutex(OSMutex* mutex);
int OSTryLockMutex(OSMutex* mutex);
void OSYieldThread(void);
OSThread* OSGetCurrentThread(void);
int OSCreateThread(OSThread* thread, void (*entry)(void*), void* argument,
                   void* stack_top, unsigned int stack_size, int priority,
                   unsigned short attributes);
void OSCancelThread(OSThread* thread);
void OSResumeThread(OSThread* thread);

#ifdef __cplusplus
}
#endif

#endif
