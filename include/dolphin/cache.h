#ifndef DOLPHIN_CACHE_H
#define DOLPHIN_CACHE_H

#ifdef __cplusplus
extern "C" {
#endif

void DCFlushRangeNoSync(void* address, unsigned long length);
void DCStoreRange(void* address, unsigned long length);
void DCFlushRange(void* address, unsigned long length);
void DCInvalidateRange(void* address, unsigned long length);
void DCEnable(void);
void ICInvalidateRange(void* address, unsigned long length);
void ICFlashInvalidate(void);
void ICEnable(void);
void LCDisable(void);
void L2GlobalInvalidate(void);
void __OSCacheInit(void);

#ifdef __cplusplus
}
#endif

#endif
