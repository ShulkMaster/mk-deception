#ifndef MSL_MSLCORE_H
#define MSL_MSLCORE_H

#include "msl/msl_types.h"

#ifdef __cplusplus
extern "C" {
#endif

extern _mslSystem* gMsi;

void mslStopAll(_mslSystem* system);
int mslSuspendSpuDma(void);
int mslResumeSpuDma(void);

#ifdef __cplusplus
}
#endif

#endif
