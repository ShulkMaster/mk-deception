#ifndef RW_RWCAMERA_INTERNAL_H
#define RW_RWCAMERA_INTERNAL_H

#include "rw/rwcore_types.h"

typedef struct RwCamera RwCamera;
struct RpWorld;

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

RwCamera* RwCameraCreate(void);
int RwCameraDestroy(RwCamera* camera);
int RwCameraRegisterPlugin(int size, unsigned int pluginID,
                           RwPluginObjectConstructor constructCB,
                           RwPluginObjectDestructor destructCB,
                           RwPluginObjectCopy copyCB);
int RwCameraFrustumTestSphere(const RwCamera* camera,
                              const RwSphere* sphere);
RwCamera* RwCameraSetViewWindow(RwCamera* camera,
                                const RwV2d* viewWindow);
RwCamera* RwCameraShowRaster(RwCamera* camera, void* device,
                             unsigned int flags);
struct RpWorld* RwCameraGetWorld(const RwCamera* camera);
struct RpWorld* RpWorldAddCamera(struct RpWorld* world, RwCamera* camera);
struct RpWorld* RpWorldRemoveCamera(struct RpWorld* world, RwCamera* camera);
void* _rwCameraOpen(void* instance, int offset, int size);
void* _rwCameraClose(void* instance, int offset, int size);

#endif
