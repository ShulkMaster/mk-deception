#ifndef RW_RWCAMERA_INTERNAL_H
#define RW_RWCAMERA_INTERNAL_H

#include "rw/rwim3d.h"

typedef struct RwCamera RwCamera;

typedef struct RwPlane {
    RwV3d normal;
    float distance;
} RwPlane;

typedef struct RwFrustumPlane {
    RwPlane plane;
    unsigned char closestX;
    unsigned char closestY;
    unsigned char closestZ;
    unsigned char field_0x13;
} RwFrustumPlane;

typedef RwCamera* (*RwCameraBeginUpdateFunc)(RwCamera* camera);
typedef RwCamera* (*RwCameraEndUpdateFunc)(RwCamera* camera);

struct RwCamera {
    RwObjectHasFrame object;
    int projectionType;
    RwCameraBeginUpdateFunc beginUpdate;
    RwCameraEndUpdateFunc endUpdate;
    RwMatrix viewMatrix;
    RwRaster* frameBuffer;
    RwRaster* zBuffer;
    RwV2d viewWindow;
    RwV2d recipViewWindow;
    RwV2d viewOffset;
    float nearPlane;
    float farPlane;
    float fogPlane;
    float zScale;
    float zShift;
    RwFrustumPlane frustumPlanes[6];
    RwBBox frustumBoundBox;
    RwV3d frustumCorners[8];
};

#endif
