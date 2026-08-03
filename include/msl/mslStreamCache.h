#ifndef MSL_STREAM_CACHE_H
#define MSL_STREAM_CACHE_H

#ifdef __cplusplus
extern "C" {
#endif

int mslStreamCache_GetSizeBuffer(void);
int mslStreamCache_GetNumBuffers(void);
unsigned long mslStreamCache_GetStreamBuffer(void);
void mslStreamCache_ReleaseBuffer(int address);
void mslStreamCache_Initialize_A(int base_address);

#ifdef __cplusplus
}
#endif

#endif
