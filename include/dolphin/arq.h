#ifndef DOLPHIN_ARQ_H
#define DOLPHIN_ARQ_H

#include "dolphin/types.h"
#include "dolphin/ar.h"

#ifdef __cplusplus
extern "C" {
#endif

void ARQInit(void);
void ARQPostRequest(ARQRequest* request, u32 owner, u32 type,
                    u32 priority, u32 source,
                    u32 destination, u32 length,
                    ARQCallback callback);
void __ARQPopTaskQueueHi(void);

#ifdef __cplusplus
}
#endif

#endif
