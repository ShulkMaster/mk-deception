#ifndef DOLPHIN_CACHE_H
#define DOLPHIN_CACHE_H

#include "dolphin/types.h"

#ifdef __cplusplus
extern "C" {
#endif

void DCFlushRangeNoSync(void* address, u32 length);
void DCStoreRange(void* address, u32 length);
void DCFlushRange(void* address, u32 length);
void DCInvalidateRange(void* address, u32 length);
void DCEnable(void);
void ICInvalidateRange(void* address, u32 length);
void ICFlashInvalidate(void);
void ICEnable(void);
void LCDisable(void);
void L2GlobalInvalidate(void);
void __OSCacheInit(void);

#ifdef __cplusplus
}
#endif

#endif
