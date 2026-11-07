#ifndef MSL_MSLCORE_H
#define MSL_MSLCORE_H

typedef struct _mslSystem _mslSystem;

#ifdef __cplusplus
extern "C" {
#endif

int mslTick(void);
void mslStopAll(_mslSystem* system);
int mslSuspendSpuDma(void);
int mslResumeSpuDma(void);

#ifdef __cplusplus
}
#endif

#endif
