#ifndef RW_RWVECTOR_H
#define RW_RWVECTOR_H

#include "rw/rtquat.h"
#include "rw/rwplcore.h"

float _rwInvSqrt(float value);
RwV3d* RwV3dTransformPoint(RwV3d* pointOut, const RwV3d* pointIn,
                           const RwMatrix* matrix);
RwV3d* RwV3dTransformPoints(RwV3d* pointsOut, const RwV3d* pointsIn,
                            int numPoints, const RwMatrix* matrix);
RwV3d* RwV3dTransformVector(RwV3d* vectorOut, const RwV3d* vectorIn,
                            const RwMatrix* matrix);
void* _rwVectorOpen(void* instance, int offset, int size);
void* _rwVectorClose(void* instance, int offset, int size);

#endif
