#include "rw/rwengine.h"
#include "rw/rwerror.h"
#include "rw/rwcamera_internal.h"
#include "rw/rwfreelist.h"
#include "rw/rwmatrix.h"
#include "rw/rwtypehf.h"
#include "rw/rwvector.h"

extern void _rwFrameSyncDirty(void);
extern void _rwPipeInitForCamera(RwCamera* camera);

RwPluginRegistry cameraTKList = { sizeof(RwCamera), sizeof(RwCamera), 0, 0,
                                  0, 0 };
static RwFreeList _rwCameraFreeList;
static int _rwCameraFreeListBlockSize = 4;
static int _rwCameraFreeListPreallocBlocks = 1;
static RwModuleInfo cameraModule;

static void CameraSetClosestVertex(RwFrustumPlane* frustumPlane)
{
    frustumPlane->closestX =
        (unsigned char)(((*(unsigned int*)&frustumPlane->plane.normal.x) >> 31) + 1);
    frustumPlane->closestY =
        (unsigned char)(((*(unsigned int*)&frustumPlane->plane.normal.y) >> 31) + 1);
    frustumPlane->closestZ =
        (unsigned char)(((*(unsigned int*)&frustumPlane->plane.normal.z) >> 31) + 1);
}

static void CameraBuildSidePlane(RwFrustumPlane* frustumPlane,
                                 const RwV3d* point, const RwV3d* first,
                                 const RwV3d* origin, const RwV3d* second)
{
    RwV3d edgeA;
    RwV3d edgeB;
    float invLength;

    edgeA.x = first->x - origin->x;
    edgeA.y = first->y - origin->y;
    edgeA.z = first->z - origin->z;
    edgeB.x = second->x - origin->x;
    edgeB.y = second->y - origin->y;
    edgeB.z = second->z - origin->z;
    frustumPlane->plane.normal.x = edgeA.y * edgeB.z - edgeA.z * edgeB.y;
    frustumPlane->plane.normal.y = edgeA.z * edgeB.x - edgeA.x * edgeB.z;
    frustumPlane->plane.normal.z = edgeA.x * edgeB.y - edgeA.y * edgeB.x;
    invLength = _rwInvSqrt(frustumPlane->plane.normal.x *
                               frustumPlane->plane.normal.x +
                           frustumPlane->plane.normal.y *
                               frustumPlane->plane.normal.y +
                           frustumPlane->plane.normal.z *
                               frustumPlane->plane.normal.z);
    frustumPlane->plane.normal.x *= invLength;
    frustumPlane->plane.normal.y *= invLength;
    frustumPlane->plane.normal.z *= invLength;
    frustumPlane->plane.distance = point->x * frustumPlane->plane.normal.x +
                                   point->y * frustumPlane->plane.normal.y +
                                   point->z * frustumPlane->plane.normal.z;
    CameraSetClosestVertex(frustumPlane);
}

static void CameraUpdateZShiftScale(RwCamera* camera)
{
    float nearScreenZ = RwEngineInstance->dOpenDevice.zBufferNear;
    float farScreenZ = RwEngineInstance->dOpenDevice.zBufferFar;
    float nearPlane;
    float farPlane;
    float adjustment;
    float planeDifference;
    float zScale;
    float zShift;

    switch (camera->projectionType) {
    case 2:
        farPlane = camera->farPlane;
        nearPlane = camera->nearPlane;
        break;
    case 1:
    default:
        farPlane = 1.0f / camera->farPlane;
        nearPlane = 1.0f / camera->nearPlane;
        break;
    }

    adjustment = 0.0001f * (farScreenZ - nearScreenZ);
    farScreenZ -= adjustment;
    nearScreenZ += adjustment;
    planeDifference = farPlane - nearPlane;
    zScale = (farScreenZ - nearScreenZ) / planeDifference;
    zShift = 0.5f * ((farScreenZ + nearScreenZ) -
                     zScale * (farPlane + nearPlane));
    camera->zScale = zScale;
    camera->zShift = zShift;
}

static void CameraBuildPerspClipPlanes(RwCamera* camera)
{
    const RwMatrix* ltm = &((RwFrame*)camera->object.object.parent)->ltm;
    RwV3d* corners = camera->frustumCorners;
    RwFrustumPlane* planes = camera->frustumPlanes;
    RwV3d center;
    RwV3d right;
    RwV3d up;
    unsigned int index;

    center.x = ltm->right.x * -camera->viewOffset.x +
               ltm->up.x * camera->viewOffset.y;
    center.y = ltm->right.y * -camera->viewOffset.x +
               ltm->up.y * camera->viewOffset.y;
    center.z = ltm->right.z * -camera->viewOffset.x +
               ltm->up.z * camera->viewOffset.y;

    right.x = ltm->right.x * camera->viewWindow.x;
    right.y = ltm->right.y * camera->viewWindow.x;
    right.z = ltm->right.z * camera->viewWindow.x;
    up.x = ltm->up.x * camera->viewWindow.y;
    up.y = ltm->up.y * camera->viewWindow.y;
    up.z = ltm->up.z * camera->viewWindow.y;

    corners[0].x = ltm->at.x + right.x + up.x;
    corners[0].y = ltm->at.y + right.y + up.y;
    corners[0].z = ltm->at.z + right.z + up.z;
    right.x *= 2.0f;
    right.y *= 2.0f;
    right.z *= 2.0f;
    up.x *= 2.0f;
    up.y *= 2.0f;
    up.z *= 2.0f;
    corners[1].x = corners[0].x - right.x;
    corners[1].y = corners[0].y - right.y;
    corners[1].z = corners[0].z - right.z;
    corners[2].x = corners[1].x - up.x;
    corners[2].y = corners[1].y - up.y;
    corners[2].z = corners[1].z - up.z;
    corners[3].x = corners[2].x + right.x;
    corners[3].y = corners[2].y + right.y;
    corners[3].z = corners[2].z + right.z;

    for (index = 0; index < 4; index++) {
        RwV3d direction;
        RwV3d origin;

        direction.x = corners[index].x - center.x;
        direction.y = corners[index].y - center.y;
        direction.z = corners[index].z - center.z;
        origin.x = center.x + ltm->pos.x;
        origin.y = center.y + ltm->pos.y;
        origin.z = center.z + ltm->pos.z;
        corners[index].x = origin.x + direction.x * camera->nearPlane;
        corners[index].y = origin.y + direction.y * camera->nearPlane;
        corners[index].z = origin.z + direction.z * camera->nearPlane;
        corners[index + 4].x = origin.x + direction.x * camera->farPlane;
        corners[index + 4].y = origin.y + direction.y * camera->farPlane;
        corners[index + 4].z = origin.z + direction.z * camera->farPlane;
    }

    planes[0].plane.normal = ltm->at;
    planes[0].plane.distance =
        corners[4].x * planes[0].plane.normal.x +
        corners[4].y * planes[0].plane.normal.y +
        corners[4].z * planes[0].plane.normal.z;
    CameraSetClosestVertex(&planes[0]);
    planes[1].plane.normal.x = -planes[0].plane.normal.x;
    planes[1].plane.normal.y = -planes[0].plane.normal.y;
    planes[1].plane.normal.z = -planes[0].plane.normal.z;
    planes[1].plane.distance =
        corners[0].x * planes[1].plane.normal.x +
        corners[0].y * planes[1].plane.normal.y +
        corners[0].z * planes[1].plane.normal.z;
    CameraSetClosestVertex(&planes[1]);

    CameraBuildSidePlane(&planes[2], &corners[1], &corners[1], &corners[5],
                         &corners[6]);
    CameraBuildSidePlane(&planes[3], &corners[1], &corners[4], &corners[5],
                         &corners[1]);
    CameraBuildSidePlane(&planes[4], &corners[3], &corners[3], &corners[7],
                         &corners[4]);
    CameraBuildSidePlane(&planes[5], &corners[3], &corners[6], &corners[7],
                         &corners[3]);
}

static RwCamera* CameraBuildPerspViewMatrix(RwCamera* camera)
{

    const RwMatrix* ltm = &((RwFrame*)camera->object.object.parent)->ltm;
    RwMatrix* view = &camera->viewMatrix;
    RwV3d vector;
    float scalar;

    scalar = -0.5f * camera->recipViewWindow.x;
    vector.x = ltm->right.x * scalar;
    vector.y = ltm->right.y * scalar;
    vector.z = ltm->right.z * scalar;
    scalar = -(scalar * camera->viewOffset.x - 0.5f);
    vector.x += ltm->at.x * scalar;
    vector.y += ltm->at.y * scalar;
    vector.z += ltm->at.z * scalar;
    view->right.x = vector.x;
    view->up.x = vector.y;
    view->at.x = vector.z;
    view->pos.x =
        0.5f - (scalar + (ltm->pos.x * vector.x +
                          ltm->pos.y * vector.y +
                          ltm->pos.z * vector.z));

    scalar = -0.5f * camera->recipViewWindow.y;
    vector.x = ltm->up.x * scalar;
    vector.y = ltm->up.y * scalar;
    vector.z = ltm->up.z * scalar;
    scalar = scalar * camera->viewOffset.y + 0.5f;
    vector.x += ltm->at.x * scalar;
    vector.y += ltm->at.y * scalar;
    vector.z += ltm->at.z * scalar;
    view->right.y = vector.x;
    view->up.y = vector.y;
    view->at.y = vector.z;
    view->pos.y =
        0.5f - (scalar + (ltm->pos.x * vector.x +
                          ltm->pos.y * vector.y +
                          ltm->pos.z * vector.z));

    view->right.z = ltm->at.x;
    view->up.z = ltm->at.y;
    view->at.z = ltm->at.z;
    view->pos.z = -(ltm->pos.x * ltm->at.x + ltm->pos.y * ltm->at.y +
                    ltm->pos.z * ltm->at.z);
    RwMatrixOptimize(view, 0);
    return camera;
}

static void CameraBuildParallelClipPlanes(RwCamera* camera)
{
    const RwMatrix* ltm = &((RwFrame*)camera->object.object.parent)->ltm;
    RwV3d* corners = camera->frustumCorners;
    RwFrustumPlane* planes = camera->frustumPlanes;
    float nearX = (1.0f - camera->nearPlane) * -camera->viewOffset.x;
    float farX = (1.0f - camera->farPlane) * -camera->viewOffset.x;
    float nearY = (1.0f - camera->nearPlane) * camera->viewOffset.y;
    float farY = (1.0f - camera->farPlane) * camera->viewOffset.y;

    corners[0].x = corners[2].x = camera->viewWindow.x + nearX;
    corners[1].x = corners[3].x = -camera->viewWindow.x + nearX;
    corners[4].x = corners[6].x = camera->viewWindow.x + farX;
    corners[5].x = corners[7].x = -camera->viewWindow.x + farX;
    corners[0].y = corners[1].y = camera->viewWindow.y + nearY;
    corners[2].y = corners[3].y = -camera->viewWindow.y + nearY;
    corners[4].y = corners[5].y = camera->viewWindow.y + farY;
    corners[6].y = corners[7].y = -camera->viewWindow.y + farY;
    corners[0].z = corners[1].z = corners[2].z = corners[3].z =
        camera->nearPlane;
    corners[4].z = corners[5].z = corners[6].z = corners[7].z =
        camera->farPlane;
    RwV3dTransformPoints(corners, corners, 8, ltm);

    planes[0].plane.normal = ltm->at;
    planes[0].plane.distance =
        corners[4].x * planes[0].plane.normal.x +
        corners[4].y * planes[0].plane.normal.y +
        corners[4].z * planes[0].plane.normal.z;
    CameraSetClosestVertex(&planes[0]);
    planes[1].plane.normal.x = -planes[0].plane.normal.x;
    planes[1].plane.normal.y = -planes[0].plane.normal.y;
    planes[1].plane.normal.z = -planes[0].plane.normal.z;
    planes[1].plane.distance =
        corners[0].x * planes[1].plane.normal.x +
        corners[0].y * planes[1].plane.normal.y +
        corners[0].z * planes[1].plane.normal.z;
    CameraSetClosestVertex(&planes[1]);

    CameraBuildSidePlane(&planes[2], &corners[1], &corners[1], &corners[5],
                         &corners[6]);
    CameraBuildSidePlane(&planes[3], &corners[1], &corners[4], &corners[5],
                         &corners[1]);
    planes[4].plane.normal.x = -planes[2].plane.normal.x;
    planes[4].plane.normal.y = -planes[2].plane.normal.y;
    planes[4].plane.normal.z = -planes[2].plane.normal.z;
    planes[4].plane.distance =
        corners[3].x * planes[4].plane.normal.x +
        corners[3].y * planes[4].plane.normal.y +
        corners[3].z * planes[4].plane.normal.z;
    CameraSetClosestVertex(&planes[4]);
    planes[5].plane.normal.x = -planes[3].plane.normal.x;
    planes[5].plane.normal.y = -planes[3].plane.normal.y;
    planes[5].plane.normal.z = -planes[3].plane.normal.z;
    planes[5].plane.distance =
        corners[3].x * planes[5].plane.normal.x +
        corners[3].y * planes[5].plane.normal.y +
        corners[3].z * planes[5].plane.normal.z;
    CameraSetClosestVertex(&planes[5]);
}

static RwCamera* CameraBuildParallelViewMatrix(RwCamera* camera)
{

    const RwMatrix* ltm = &((RwFrame*)camera->object.object.parent)->ltm;
    RwMatrix* view = &camera->viewMatrix;
    RwV3d vector;
    float scalar;

    scalar = -0.5f * camera->recipViewWindow.x;
    vector.x = ltm->right.x * scalar;
    vector.y = ltm->right.y * scalar;
    vector.z = ltm->right.z * scalar;
    scalar = -(scalar * camera->viewOffset.x);
    vector.x += ltm->at.x * scalar;
    vector.y += ltm->at.y * scalar;
    vector.z += ltm->at.z * scalar;
    view->right.x = vector.x;
    view->up.x = vector.y;
    view->at.x = vector.z;
    view->pos.x =
        0.5f - (scalar + (ltm->pos.x * vector.x +
                          ltm->pos.y * vector.y +
                          ltm->pos.z * vector.z));

    scalar = -0.5f * camera->recipViewWindow.y;
    vector.x = ltm->up.x * scalar;
    vector.y = ltm->up.y * scalar;
    vector.z = ltm->up.z * scalar;
    scalar = scalar * camera->viewOffset.y;
    vector.x += ltm->at.x * scalar;
    vector.y += ltm->at.y * scalar;
    vector.z += ltm->at.z * scalar;
    view->right.y = vector.x;
    view->up.y = vector.y;
    view->at.y = vector.z;
    view->pos.y =
        0.5f - (scalar + (ltm->pos.x * vector.x +
                          ltm->pos.y * vector.y +
                          ltm->pos.z * vector.z));

    view->right.z = ltm->at.x;
    view->up.z = ltm->at.y;
    view->at.z = ltm->at.z;
    view->pos.z = -(ltm->pos.x * ltm->at.x + ltm->pos.y * ltm->at.y +
                    ltm->pos.z * ltm->at.z);
    RwMatrixOptimize(view, 0);
    return camera;
}

static RwObjectHasFrame* CameraSync(RwObjectHasFrame* object)
{
    RwCamera* camera = (RwCamera*)object;

    if (camera->projectionType == 1) {
        CameraBuildPerspViewMatrix(camera);
        CameraBuildPerspClipPlanes(camera);
    } else {
        CameraBuildParallelViewMatrix(camera);
        CameraBuildParallelClipPlanes(camera);
    }
    RwBBoxCalculate(&camera->frustumBoundBox, camera->frustumCorners, 8);
    return object;
}

static RwCamera* CameraEndUpdate(RwCamera* camera)
{
    RwCameraDeviceCall endUpdate = RWENGINESTANDARD(RwCameraDeviceCall, rwSTANDARDCAMERAENDUPDATE);

    if (endUpdate(0, camera, 0)) {
        RwEngineInstance->curCamera = 0;
        return camera;
    }
    return 0;
}

static RwCamera* CameraBeginUpdate(RwCamera* camera)
{
    RwCameraDeviceCall beginUpdate;

    RwEngineInstance->curCamera = camera;
    _rwFrameSyncDirty();
    beginUpdate = RWENGINESTANDARD(RwCameraDeviceCall, rwSTANDARDCAMERABEGINUPDATE);
    if (beginUpdate(0, camera, 0)) {
        _rwPipeInitForCamera(camera);
        return camera;
    }
    return 0;
}

void* _rwCameraClose(void* instance, int offset, int size)
{
    if (*(RwFreeList**)((unsigned char*)RwEngineInstance +
                        cameraModule.globalsOffset) != 0) {
        RwFreeListDestroy(
            *(RwFreeList**)((unsigned char*)RwEngineInstance +
                            cameraModule.globalsOffset));
        *(RwFreeList**)((unsigned char*)RwEngineInstance +
                        cameraModule.globalsOffset) = 0;
    }
    cameraModule.numInstances--;
    return instance;
}

void* _rwCameraOpen(void* instance, int offset, int size)
{
    cameraModule.globalsOffset = offset;
    *(RwFreeList**)((unsigned char*)RwEngineInstance +
                    cameraModule.globalsOffset) =
        RwFreeListCreateAndPreallocateSpace(
            cameraTKList.sizeOfStruct, _rwCameraFreeListBlockSize, 4,
            _rwCameraFreeListPreallocBlocks, &_rwCameraFreeList, 0x40005);
    if (*(RwFreeList**)((unsigned char*)RwEngineInstance +
                        cameraModule.globalsOffset) == 0) {
        return 0;
    }
    cameraModule.numInstances++;
    return instance;
}

RwCamera* RwCameraEndUpdate(RwCamera* camera)
{
    RwCamera* result = camera->endUpdate(camera);
    return result;
}

RwCamera* RwCameraBeginUpdate(RwCamera* camera)
{
    RwCamera* result = camera->beginUpdate(camera);
    return result;
}

RwCamera* RwCameraSetNearClipPlane(RwCamera* camera, float nearClip)
{
    RwFrame* frame;

    camera->nearPlane = nearClip;
    CameraUpdateZShiftScale(camera);
    frame = (RwFrame*)camera->object.object.parent;
    if (frame != 0) {
        RwFrameUpdateObjects(frame);
    }
    return camera;
}

RwCamera* RwCameraSetFarClipPlane(RwCamera* camera, float farClip)
{
    RwFrame* frame;

    camera->farPlane = farClip;
    CameraUpdateZShiftScale(camera);
    frame = (RwFrame*)camera->object.object.parent;
    if (frame != 0) {
        RwFrameUpdateObjects(frame);
    }
    return camera;
}

int RwCameraFrustumTestSphere(const RwCamera* camera,
                                  const RwSphere* sphere)
{

    int result = 2;
    const RwFrustumPlane* plane = camera->frustumPlanes;
    int count = 6;

    while (count-- != 0) {
        float distance;
        distance = sphere->center.x * plane->plane.normal.x +
                   sphere->center.y * plane->plane.normal.y +
                   sphere->center.z * plane->plane.normal.z -
                   plane->plane.distance;
        if (distance > sphere->radius) {
            return 0;
        }
        if (distance > -sphere->radius) {
            result = 1;
        }
        plane++;
    }
    return result;
}

RwCamera* RwCameraClear(RwCamera* camera, RwRGBA* color, int clearMode)
{
    RwCameraClearCall clear = RWENGINESTANDARD(RwCameraClearCall, rwSTANDARDCAMERACLEAR);

    if (clear(camera, color, clearMode)) {
        return camera;
    }
    return 0;
}

RwCamera* RwCameraShowRaster(RwCamera* camera, void* device, unsigned int flags)
{
    if (RwRasterShowRaster(camera->frameBuffer, device, flags) != 0) {
        return camera;
    }
    return 0;
}

RwCamera* RwCameraSetProjection(RwCamera* camera,
                                int projection)
{
    switch (projection) {
    case 1:
    case 2: {
        RwFrame* frame;

        camera->projectionType = projection;
        frame = (RwFrame*)camera->object.object.parent;
        if (frame != 0) {
            RwFrameUpdateObjects(frame);
        }
        CameraUpdateZShiftScale(camera);
        return camera;
    }
    default:
        break;
    }
    {
        RwError error;
        error.pluginID = 1;
        error.errorCode =
            _rwerror(0x80000003, "Invalid projection type specified");
        RwErrorSet(&error);
    }
    return 0;
}

RwCamera* RwCameraSetViewWindow(RwCamera* camera,
                                const RwV2d* viewWindow)
{
    RwFrame* frame;

    camera->viewWindow = *viewWindow;
    camera->recipViewWindow.x = 1.0f / camera->viewWindow.x;
    camera->recipViewWindow.y = 1.0f / camera->viewWindow.y;
    frame = (RwFrame*)camera->object.object.parent;
    if (frame != 0) {
        RwFrameUpdateObjects(frame);
    }

    return camera;
}

int RwCameraRegisterPlugin(int size, unsigned int pluginID,
                              RwPluginObjectConstructor constructCB,
                              RwPluginObjectDestructor destructCB,
                              RwPluginObjectCopy copyCB)
{
    int offset = _rwPluginRegistryAddPlugin(
        &cameraTKList, size, pluginID, constructCB, destructCB, copyCB);
    return offset;
}

int RwCameraDestroy(RwCamera* camera)
{
    _rwPluginRegistryDeInitObject(&cameraTKList, camera);
    _rwObjectHasFrameReleaseFrame(camera);
    RwEngineInstance->fpFreeListFree(
        *(RwFreeList**)((unsigned char*)RwEngineInstance +
                        cameraModule.globalsOffset),
        camera);
    return 1;
}

RwCamera* RwCameraCreate(void)
{
    RwCamera* camera = RwEngineInstance->fpFreeListAlloc(
        *(RwFreeList**)((unsigned char*)RwEngineInstance +
                        cameraModule.globalsOffset),
        0x30005);

    if (camera == 0) {
        return 0;
    }
    camera->object.object.type = 4;
    camera->object.object.subType = 0;
    camera->object.object.flags = 0;
    camera->object.object.privateFlags = 0;
    camera->object.object.parent = 0;
    camera->object.sync = CameraSync;
    camera->beginUpdate = CameraBeginUpdate;
    camera->endUpdate = CameraEndUpdate;
    camera->viewWindow.x = camera->viewWindow.y = 1.0f;
    camera->recipViewWindow.x = camera->recipViewWindow.y = 1.0f;
    camera->viewOffset.x = camera->viewOffset.y = 0.0f;
    camera->nearPlane = 0.05f;
    camera->farPlane = 10.0f;
    camera->fogPlane = 5.0f;
    camera->frameBuffer = 0;
    camera->zBuffer = 0;
    camera->projectionType = 1;
    CameraUpdateZShiftScale(camera);
    camera->viewMatrix.flags = 0;
    _rwPluginRegistryInitObject(&cameraTKList, camera);
    return camera;
}
