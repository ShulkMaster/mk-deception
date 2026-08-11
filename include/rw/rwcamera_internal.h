#ifndef RW_RWCAMERA_INTERNAL_H
#define RW_RWCAMERA_INTERNAL_H

#include "rw/rwim3d.h"

typedef struct RwCamera RwCamera;

typedef struct RwPlane {
    RwV3d normal;
    RwReal distance;
} RwPlane;

typedef struct RwFrustumPlane {
    RwPlane plane;
    RwUInt8 closestX;
    RwUInt8 closestY;
    RwUInt8 closestZ;
    RwUInt8 field_0x13;
} RwFrustumPlane;

typedef RwCamera* (*RwCameraBeginUpdateFunc)(RwCamera* camera);
typedef RwCamera* (*RwCameraEndUpdateFunc)(RwCamera* camera);

struct RwCamera {
    RwObjectHasFrame object;
    RwInt32 projectionType;
    RwCameraBeginUpdateFunc beginUpdate;
    RwCameraEndUpdateFunc endUpdate;
    RwMatrix viewMatrix;
    RwRaster* frameBuffer;
    RwRaster* zBuffer;
    RwV2d viewWindow;
    RwV2d recipViewWindow;
    RwV2d viewOffset;
    RwReal nearPlane;
    RwReal farPlane;
    RwReal fogPlane;
    RwReal zScale;
    RwReal zShift;
    RwFrustumPlane frustumPlanes[6];
    RwBBox frustumBoundBox;
    RwV3d frustumCorners[8];
};

#endif
