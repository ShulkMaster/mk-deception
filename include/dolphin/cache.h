#ifndef DOLPHIN_CACHE_H
#define DOLPHIN_CACHE_H

#ifdef __cplusplus
extern "C" {
#endif

void DCFlushRangeNoSync(void* address, unsigned long length);
void DCStoreRange(void* address, unsigned long length);
void DCFlushRange(void* address, unsigned long length);
void DCInvalidateRange(void* address, unsigned long length);

#ifdef __cplusplus
}
#endif

#endif
