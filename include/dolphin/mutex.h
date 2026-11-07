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

#ifdef __cplusplus
}
#endif

#endif
