#include "libmkparticle/rw_engine.h"
#include "rw/rpworld_types.h"
#include "rw/rwcamera_internal.h"
#include "rw/rplight.h"
#include "rw/rwfreelist.h"
#include "rw/rwframe.h"
#include "rw/rwplcore.h"
#include "rw/rwstream.h"
#include "rw/rwstream_internal.h"
#include "rw/rwtypehf.h"
#include "rw/rwvector.h"
#include "rw/rxpipeline.h"

typedef struct RpClumpGlobals {
    RwFreeList* atomicFreeList;
    RwFreeList* clumpFreeList;
} RpClumpGlobals;

typedef struct RpClumpObjectExtension {
    RpClump* clump;
    RwLLLink inClumpLink;
} RpClumpObjectExtension;

RwPluginRegistry atomicTKList = {0x70, 0x70, 0, 0, 0, 0};
RwPluginRegistry clumpTKList = {0x2C, 0x2C, 0, 0, 0, 0};
static RwFreeList _rpAtomicFreeList;
static RwFreeList _rpClumpFreeList;
static int _rpAtomicFreeListBlockSize = 0x80;
static int _rpAtomicFreeListPreallocBlocks = 1;
static int _rpClumpFreeListBlockSize = 0x80;
static int _rpClumpFreeListPreallocBlocks = 1;
int _rpClumpCameraExtOffset;
int _rpClumpLightExtOffset;
unsigned int lastSeenExtraData;
unsigned int lastSeenRightsPluginId;
static RwModuleInfo clumpModule;

static RpClumpGlobals* ClumpGlobals(void)
{
    return (RpClumpGlobals*)((unsigned char*)RwEngineInstance +
                             clumpModule.globalsOffset);
}

extern void RwResourcesFreeResEntry(RwResEntry*);
extern RpWorld* RpAtomicGetWorld(RpAtomic*);
extern float _rwSqrt(float);


static void ClumpTidyDestroyClump(void* clump, void* data)
{
    RpClumpDestroy(clump);
}

static void ClumpTidyDestroyAtomic(void* atomic, void* data)
{
    RpAtomicDestroy(atomic);
}

RwStream* _rpReadAtomicRights(RwStream* stream, int length,
                              void* object, int offset, int size)
{
    if (RwStreamReadInt32(stream, (int*)&lastSeenRightsPluginId, 4) == 0)
        return 0;
    if (length == 8 &&
        RwStreamReadInt32(stream, (int*)&lastSeenExtraData, 4) == 0)
        return 0;
    return stream;
}

RwStream* _rpWriteAtomicRights(RwStream* stream, int length,
                               const void* object, int offset, int size)
{
    const RpAtomic* atomic = object;
    if (RwStreamWriteInt32(stream, (const int*)&atomic->pipeline->pluginId, 4) == 0)
        return 0;
    if (RwStreamWriteInt32(stream, (const int*)&atomic->pipeline->pluginData, 4) == 0)
        return 0;
    return stream;
}

int _rpSizeAtomicRights(const void* object, int offset, int size)
{
    const RpAtomic* atomic = object;
    if (atomic->pipeline != 0) {
        if (atomic->pipeline->pluginId != 0)
            return 8;
    }
    return 0;
}


static RwObjectHasFrame* AtomicSync(RwObjectHasFrame* object)
{
    RpAtomic* atomic = (RpAtomic*)object;

    if (atomic->interpolator.flags & 2)
        _rpAtomicResyncInterpolatedSphere(atomic);
    atomic->object.privateFlags |= 1;
    return object;
}

RpAtomic* AtomicDefaultRenderCallBack(RpAtomic* atomic)
{
    RxPipeline* pipeline = atomic->pipeline;
    if (pipeline == 0)
        pipeline = RxPipelineGlobals()->currentAtomicPipeline;
    if (RxPipelineExecute(pipeline, atomic, 1) != 0)
        return atomic;
    return 0;
}

RpGeometryList* GeometryListDeinitialize(RpGeometryList* geometryList)
{
    int i;
    for (i = 0; i < geometryList->numGeometries; i++)
        RpGeometryDestroy(geometryList->geometries[i]);
    if (geometryList->geometries != 0) {
        RwEngineInstance->fpFree(geometryList->geometries);
        geometryList->geometries = 0;
    }
    return geometryList;
}


static void* ClumpInitCameraExt(void* object, int offset, int size)
{
    RpClumpObjectExtension* ext =
        (RpClumpObjectExtension*)((unsigned char*)object +
                                  _rpClumpCameraExtOffset);
    ext->inClumpLink.prev = 0;
    ext->inClumpLink.next = 0;
    ext->clump = 0;
    return object;
}

static void* ClumpDeInitCameraExt(void* object, int offset, int size)
{
    return object;
}

static void* ClumpInitLightExt(void* object, int offset, int size)
{
    RpClumpObjectExtension* ext =
        (RpClumpObjectExtension*)((unsigned char*)object +
                                  _rpClumpLightExtOffset);
    ext->inClumpLink.prev = 0;
    ext->inClumpLink.next = 0;
    ext->clump = 0;
    return object;
}

static void* ClumpDeInitLightExt(void* object, int offset, int size)
{
    return object;
}

static RpLight* DestroyClumpLight(RpLight* light, void* data)
{
    RpClumpObjectExtension* ext = (RpClumpObjectExtension*)((unsigned char*)light + _rpClumpLightExtOffset);
    RpClumpRemoveLight(ext->clump, light);
    RpLightDestroy(light);
    return light;
}

static RwCamera* DestroyClumpCamera(RwCamera* camera, void* data)
{
    RpClumpObjectExtension* ext = (RpClumpObjectExtension*)((unsigned char*)camera + _rpClumpCameraExtOffset);
    RpClumpRemoveCamera(ext->clump, camera);
    RwCameraDestroy(camera);
    return camera;
}

static RpClump* ClumpCallBack(RpClump* clump, void* data)
{
    return clump;
}

static RpAtomic* DestroyClumpAtomic(RpAtomic* atomic, void* data)
{
    RpAtomicDestroy(atomic);
    return atomic;
}


void _rpAtomicResyncInterpolatedSphere(RpAtomic* atomic)
{
    RpGeometry* geometry = atomic->geometry;
    RpInterpolator* interp;
    if (geometry == 0)
        return;
    interp = &atomic->interpolator;
    if (interp->startMorphTarget == interp->endMorphTarget ||
        interp->startMorphTarget >= geometry->numMorphTargets ||
        interp->endMorphTarget >= geometry->numMorphTargets) {
        RpMorphTarget* target;
        if (interp->startMorphTarget >= geometry->numMorphTargets ||
            interp->endMorphTarget >= geometry->numMorphTargets)
            target = &geometry->morphTarget[0];
        else
            target = &geometry->morphTarget[interp->startMorphTarget];
        atomic->boundingSphere = target->sphere;
    } else {
        RpMorphTarget* start = &geometry->morphTarget[interp->startMorphTarget];
        RpMorphTarget* end = &geometry->morphTarget[interp->endMorphTarget];
        float alpha = interp->position * interp->recipTime;
        atomic->boundingSphere.radius =
            start->sphere.radius +
            alpha * (end->sphere.radius - start->sphere.radius);
        atomic->boundingSphere.center.x =
            end->sphere.center.x - start->sphere.center.x;
        atomic->boundingSphere.center.y =
            end->sphere.center.y - start->sphere.center.y;
        atomic->boundingSphere.center.z =
            end->sphere.center.z - start->sphere.center.z;
        atomic->boundingSphere.center.x *= alpha;
        atomic->boundingSphere.center.y *= alpha;
        atomic->boundingSphere.center.z *= alpha;
        atomic->boundingSphere.center.x += start->sphere.center.x;
        atomic->boundingSphere.center.y += start->sphere.center.y;
        atomic->boundingSphere.center.z += start->sphere.center.z;
    }
    interp->flags &= ~2;
    atomic->object.privateFlags |= 1;
}


RwSphere* RpAtomicGetWorldBoundingSphere(RpAtomic* atomic)
{
    RwFrame* frame = atomic->object.parent;
    if (atomic->interpolator.flags & 2)
        _rpAtomicResyncInterpolatedSphere(atomic);
    if (RwFrameDirty(frame) || (atomic->object.privateFlags & 1)) {
        RwMatrix* matrix = RwFrameGetLTM(frame);
        RwV3dTransformPoint((RwV3d*)&atomic->worldBoundingSphere,
                            (const RwV3d*)&atomic->boundingSphere, matrix);
        if ((matrix->flags & 3) != 3) {
            float sx;
            float sy;
            float sz;
            float scale;
            float scaleSquared;
            RwV3d* right = &matrix->right;
            RwV3d* up = &matrix->up;
            RwV3d* at = &matrix->at;

            sx = right->x * right->x + right->y * right->y +
                 right->z * right->z;
            sy = up->x * up->x + up->y * up->y + up->z * up->z;
            sz = at->x * at->x + at->y * at->y + at->z * at->z;
            scaleSquared =
                sx >= (sy >= sz ? sy : sz) ? sx : (sy >= sz ? sy : sz);
            scale = _rwSqrt(scaleSquared);
            atomic->worldBoundingSphere.radius =
                atomic->boundingSphere.radius * scale;
        } else {
            atomic->worldBoundingSphere.radius = atomic->boundingSphere.radius;
        }
        atomic->object.privateFlags &= ~1;
    }
    return &atomic->worldBoundingSphere;
}

void* _rpClumpClose(void* instance, int offset, int size)
{
    RwFreeListForAllUsed(((RpClumpGlobals*)((unsigned char*)RwEngineInstance +
                                            clumpModule.globalsOffset))->clumpFreeList,
                         ClumpTidyDestroyClump, 0);
    RwFreeListForAllUsed(((RpClumpGlobals*)((unsigned char*)RwEngineInstance +
                                            clumpModule.globalsOffset))->atomicFreeList,
                         ClumpTidyDestroyAtomic, 0);
    RwFreeListDestroy(((RpClumpGlobals*)((unsigned char*)RwEngineInstance +
                                         clumpModule.globalsOffset))->atomicFreeList);
    RwFreeListDestroy(((RpClumpGlobals*)((unsigned char*)RwEngineInstance +
                                         clumpModule.globalsOffset))->clumpFreeList);
    ((RpClumpGlobals*)((unsigned char*)RwEngineInstance +
                       clumpModule.globalsOffset))->atomicFreeList = 0;
    ((RpClumpGlobals*)((unsigned char*)RwEngineInstance +
                       clumpModule.globalsOffset))->clumpFreeList = 0;
    clumpModule.numInstances--;
    return instance;
}

void* _rpClumpOpen(void* instance, int offset, int size)
{
    clumpModule.globalsOffset = offset;
    ((RpClumpGlobals*)((unsigned char*)RwEngineInstance +
                       clumpModule.globalsOffset))->atomicFreeList =
        RwFreeListCreateAndPreallocateSpace(
        atomicTKList.sizeOfStruct, _rpAtomicFreeListBlockSize, 4,
        _rpAtomicFreeListPreallocBlocks, &_rpAtomicFreeList, 0x40010);
    if (((RpClumpGlobals*)((unsigned char*)RwEngineInstance +
                           clumpModule.globalsOffset))->atomicFreeList != 0) {
        ((RpClumpGlobals*)((unsigned char*)RwEngineInstance +
                           clumpModule.globalsOffset))->clumpFreeList =
            RwFreeListCreateAndPreallocateSpace(
            clumpTKList.sizeOfStruct, _rpClumpFreeListBlockSize, 4,
            _rpClumpFreeListPreallocBlocks, &_rpClumpFreeList, 0x40014);
        if (((RpClumpGlobals*)((unsigned char*)RwEngineInstance +
                               clumpModule.globalsOffset))->clumpFreeList != 0) {
            clumpModule.numInstances++;
            return instance;
        }
        RwFreeListDestroy(((RpClumpGlobals*)((unsigned char*)RwEngineInstance +
                                             clumpModule.globalsOffset))->atomicFreeList);
        ((RpClumpGlobals*)((unsigned char*)RwEngineInstance +
                           clumpModule.globalsOffset))->atomicFreeList = 0;
    }
    return 0;
}

int _rpClumpRegisterExtensions(void)
{
    _rpClumpCameraExtOffset = RwCameraRegisterPlugin(12, 0x10, ClumpInitCameraExt, ClumpDeInitCameraExt, 0);
    if (_rpClumpCameraExtOffset < 0)
        return 0;
    _rpClumpLightExtOffset = RpLightRegisterPlugin(12, 0x10, ClumpInitLightExt, ClumpDeInitLightExt, 0);
    if (_rpClumpLightExtOffset < 0)
        return 0;
    return 1;
}


RpClump* RpClumpRender(RpClump* clump)
{
    RpClump* result;
    RwLLLink* link;
    RwLLLink* end;

    result = clump;
    link = clump->atomicList.next;
    end = &clump->atomicList;
    while (link != end) {
        RpAtomic* atomic = RpAtomicFromClumpLink(link);
        if (atomic->object.flags & 4) {
            RwFrameGetLTM(atomic->object.parent);
            if (atomic->renderCallBack(atomic) == 0)
                result = 0;
        }
        link = link->next;
    }
    return result;
}

RpClump* RpClumpForAllAtomics(RpClump* clump, RpAtomicCallBack callback, void* data)
{
    RwLLLink* link;
    RwLLLink* end;
    RwLLLink* next;

    link = clump->atomicList.next;
    end = &clump->atomicList;
    while (link != end) {
        RpAtomic* atomic = RpAtomicFromClumpLink(link);
        next = link->next;
        if (callback(atomic, data) == 0)
            return clump;
        link = next;
    }
    return clump;
}

RpClump* RpClumpForAllCameras(RpClump* clump,
                              RwCamera* (*callback)(RwCamera*, void*),
                              void* data)
{
    RwLLLink* link;
    RwLLLink* end;
    RwLLLink* next;

    link = clump->cameraList.next;
    end = &clump->cameraList;
    while (link != end) {
        RwCamera* camera = (RwCamera*)((unsigned char*)link - 4 - _rpClumpCameraExtOffset);
        next = link->next;
        if (callback(camera, data) == 0)
            return clump;
        link = next;
    }
    return clump;
}

RpClump* RpClumpForAllLights(RpClump* clump, RpLightCallBack callback, void* data)
{
    RwLLLink* link;
    RwLLLink* end;
    RwLLLink* next;

    link = clump->lightList.next;
    end = &clump->lightList;
    while (link != end) {
        RpLight* light = (RpLight*)((unsigned char*)link - 4 - _rpClumpLightExtOffset);
        next = link->next;
        if (callback(light, data) == 0)
            return clump;
        link = next;
    }
    return clump;
}


RpAtomic* RpAtomicCreate(void)
{
    RpAtomic* atomic = RwEngineInstance->fpFreeListAlloc(
        ClumpGlobals()->atomicFreeList, 0x30014);
    if (atomic == 0)
        return 0;
    rwObjectInitialize(atomic, 1, 0);
    atomic->sync = AtomicSync;
    atomic->repEntry = 0;
    atomic->object.flags = 5;
    atomic->object.privateFlags = 1;
    _rwObjectHasFrameSetFrame(atomic, 0);
    atomic->geometry = 0;
    atomic->boundingSphere.radius = 0.0f;
    atomic->boundingSphere.center.x = 0.0f;
    atomic->boundingSphere.center.y = 0.0f;
    atomic->boundingSphere.center.z = 0.0f;
    atomic->worldBoundingSphere.radius = 0.0f;
    atomic->worldBoundingSphere.center.x = 0.0f;
    atomic->worldBoundingSphere.center.y = 0.0f;
    atomic->worldBoundingSphere.center.z = 0.0f;
    atomic->renderCallBack = AtomicDefaultRenderCallBack;
    atomic->interpolator.startMorphTarget = 0;
    atomic->interpolator.endMorphTarget = 0;
    atomic->interpolator.time = 1.0f;
    atomic->interpolator.recipTime = 1.0f;
    atomic->interpolator.position = 0.0f;
    atomic->interpolator.flags = 3;
    atomic->inClumpLink.prev = 0;
    atomic->inClumpLink.next = 0;
    atomic->clump = 0;
    atomic->pipeline = 0;
    rwLinkListInitialize(&atomic->worldSectorsInAtomic);
    _rwPluginRegistryInitObject(&atomicTKList, atomic);
    return atomic;
}


RpAtomic* RpAtomicSetGeometry(RpAtomic* atomic, RpGeometry* geometry, unsigned int flags)
{
    if (atomic->geometry != geometry) {
        if (geometry != 0)
            _rpGeometryAddRef(geometry);
        if (atomic->geometry != 0)
            RpGeometryDestroy(atomic->geometry);
        atomic->geometry = geometry;
        if (!(flags & 1)) {
            if (geometry != 0)
                atomic->boundingSphere = geometry->morphTarget[0].sphere;
            if (atomic->object.parent != 0 && RpAtomicGetWorld(atomic) != 0)
                RwFrameUpdateObjects(atomic->object.parent);
        }
    }
    return atomic;
}

int RpAtomicDestroy(RpAtomic* atomic)
{
    _rwPluginRegistryDeInitObject(&atomicTKList, atomic);
    if (atomic->repEntry != 0)
        RwResourcesFreeResEntry(atomic->repEntry);
    RpAtomicSetGeometry(atomic, 0, 0);
    _rwObjectHasFrameReleaseFrame(atomic);
    RwEngineInstance->fpFreeListFree(ClumpGlobals()->atomicFreeList, atomic);
    return 1;
}

void RpClumpSetCallBack(RpClump* clump, RpClumpCallBack callback)
{
    if (callback != 0) {
        clump->callback = callback;
        return;
    }
    clump->callback = ClumpCallBack;
}


RpClump* RpClumpCreate(void)
{
    RpClump* clump = RwEngineInstance->fpFreeListAlloc(
        ClumpGlobals()->clumpFreeList, 0x30010);
    if (clump == 0)
        return 0;
    rwObjectInitialize(clump, 2, 0);
    clump->object.parent = 0;
    rwLinkListInitialize(&clump->atomicList);
    rwLinkListInitialize(&clump->lightList);
    rwLinkListInitialize(&clump->cameraList);
    clump->inWorldLink.prev = 0;
    clump->inWorldLink.next = 0;
    RpClumpSetCallBack(clump, 0);
    _rwPluginRegistryInitObject(&clumpTKList, clump);
    return clump;
}

int RpClumpDestroy(RpClump* clump)
{
    RwFrame* frame;
    _rwPluginRegistryDeInitObject(&clumpTKList, clump);
    RpClumpForAllAtomics(clump, DestroyClumpAtomic, 0);
    RpClumpForAllLights(clump, DestroyClumpLight, 0);
    RpClumpForAllCameras(clump, DestroyClumpCamera, 0);
    frame = clump->object.parent;
    if (frame != 0)
        RwFrameDestroyHierarchy(frame);
    RwEngineInstance->fpFreeListFree(ClumpGlobals()->clumpFreeList, clump);
    return 1;
}

RpClump* RpClumpAddAtomic(RpClump* clump, RpAtomic* atomic)
{
    RwLLLink* link;
    atomic->inClumpLink.next = clump->atomicList.next;
    atomic->inClumpLink.prev = &clump->atomicList;
    clump->atomicList.next->prev = &atomic->inClumpLink;
    link = &atomic->inClumpLink;
    clump->atomicList.next = link;
    atomic->clump = clump;
    return clump;
}


RpClump* RpClumpRemoveLight(RpClump* clump, RpLight* light)
{
    RpClumpObjectExtension* ext =
        (RpClumpObjectExtension*)((unsigned char*)light +
                                  _rpClumpLightExtOffset);
    RwLLLink* previous;

    ext->inClumpLink.prev->next = ext->inClumpLink.next;
    previous = ext->inClumpLink.prev;
    ext->inClumpLink.next->prev = previous;
    ext->inClumpLink.prev = 0;
    ext->inClumpLink.next = 0;
    ext->clump = 0;
    return clump;
}

RpClump* RpClumpRemoveCamera(RpClump* clump, RwCamera* camera)
{
    RpClumpObjectExtension* ext =
        (RpClumpObjectExtension*)((unsigned char*)camera +
                                  _rpClumpCameraExtOffset);
    RwLLLink* previous;

    ext->inClumpLink.prev->next = ext->inClumpLink.next;
    previous = ext->inClumpLink.prev;
    ext->inClumpLink.next->prev = previous;
    ext->inClumpLink.prev = 0;
    ext->inClumpLink.next = 0;
    ext->clump = 0;
    return clump;
}

int RpAtomicRegisterPlugin(int size, unsigned int id, RwPluginObjectConstructor ctor, RwPluginObjectDestructor dtor, RwPluginObjectCopy copy)
{
    int offset = _rwPluginRegistryAddPlugin(
        &atomicTKList, size, id, ctor, dtor, copy);
    return offset;
}
int RpClumpRegisterPlugin(int size, unsigned int id, RwPluginObjectConstructor ctor, RwPluginObjectDestructor dtor, RwPluginObjectCopy copy)
{
    int offset = _rwPluginRegistryAddPlugin(
        &clumpTKList, size, id, ctor, dtor, copy);
    return offset;
}
int RpAtomicRegisterPluginStream(unsigned int id, RwPluginDataChunkReadCallBack read, RwPluginDataChunkWriteCallBack write, RwPluginDataChunkGetSizeCallBack size)
{
    int offset = _rwPluginRegistryAddPluginStream(
        &atomicTKList, id, read, write, size);
    return offset;
}
int RpAtomicSetStreamAlwaysCallBack(unsigned int id, RwPluginDataChunkAlwaysCallBack callback)
{
    int offset = _rwPluginRegistryAddPlgnStrmlwysCB(
        &atomicTKList, id, callback);
    return offset;
}
int RpAtomicSetStreamRightsCallBack(unsigned int id, RwPluginDataChunkRightsCallBack callback)
{
    int offset = _rwPluginRegistryAddPlgnStrmRightsCB(
        &atomicTKList, id, callback);
    return offset;
}
int RpClumpRegisterPluginStream(unsigned int id, RwPluginDataChunkReadCallBack read, RwPluginDataChunkWriteCallBack write, RwPluginDataChunkGetSizeCallBack size)
{
    int offset = _rwPluginRegistryAddPluginStream(
        &clumpTKList, id, read, write, size);
    return offset;
}
int RpAtomicGetPluginOffset(unsigned int id)
{
    int offset = _rwPluginRegistryGetPluginOffset(&atomicTKList, id);
    return offset;
}

RpAtomic* RpAtomicSetFrame(RpAtomic* atomic, RwFrame* frame)
{
    _rwObjectHasFrameSetFrame(atomic, frame);
    atomic->object.privateFlags |= 1;
    return atomic;
}
