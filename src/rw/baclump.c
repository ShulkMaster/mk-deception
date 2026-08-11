#include "libmkparticle/rw_engine.h"
#include "rw/rpworld_types.h"
#include "rw/rplight.h"
#include "rw/rwfreelist.h"
#include "rw/rwplcore.h"
#include "rw/rwstream.h"
#include "rw/rwtypehf.h"
#include "rw/rwvector.h"
#include "rw/rxpipeline.h"

extern RwMatrix* RwFrameGetLTM(RwFrame* frame);

typedef struct RpClumpGlobals {
    RwFreeList* atomicFreeList;
    RwFreeList* clumpFreeList;
} RpClumpGlobals;

typedef struct RpClumpObjectExtension {
    RpClump* clump;
    RwLLLink inClumpLink;
} RpClumpObjectExtension;

typedef struct RpGeometryList {
    RpGeometry** geometries;
    RwInt32 numGeometries;
} RpGeometryList;

static RwPluginRegistry atomicTKList = {0x70, 0x70, 0, 0, 0, 0};
static RwPluginRegistry clumpTKList = {0x2C, 0x2C, 0, 0, 0, 0};
static RwFreeList _rpAtomicFreeList;
static RwFreeList _rpClumpFreeList;
static RwInt32 _rpAtomicFreeListBlockSize = 0x80;
static RwInt32 _rpAtomicFreeListPreallocBlocks = 1;
static RwInt32 _rpClumpFreeListBlockSize = 0x80;
static RwInt32 _rpClumpFreeListPreallocBlocks = 1;
RwInt32 _rpClumpCameraExtOffset;
RwInt32 _rpClumpLightExtOffset;
RwUInt32 lastSeenExtraData;
RwUInt32 lastSeenRightsPluginId;
static RwModuleInfo clumpModule;

static RpClumpGlobals* ClumpGlobals(void)
{
    return (RpClumpGlobals*)((RwUInt8*)RwEngineInstance +
                             clumpModule.globalsOffset);
}

extern void RwResourcesFreeResEntry(RwResEntry*);
extern RwBool RwCameraDestroy(RwCamera*);
extern RwInt32 RwCameraRegisterPlugin(RwInt32, RwUInt32,
                                     RwPluginObjectConstructor,
                                     RwPluginObjectDestructor,
                                     RwPluginObjectCopy);
extern RpWorld* RpAtomicGetWorld(RpAtomic*);
extern RwReal _rwSqrt(RwReal);
extern RwInt32 _rxPipelineGlobalsOffset;


static void ClumpTidyDestroyClump(void* clump, void* data)
{
    RpClumpDestroy(clump);
}

static void ClumpTidyDestroyAtomic(void* atomic, void* data)
{
    RpAtomicDestroy(atomic);
}

RwStream* _rpReadAtomicRights(RwStream* stream, RwInt32 length,
                              void* object, RwInt32 offset, RwInt32 size)
{
    if (RwStreamReadInt32(stream, (RwInt32*)&lastSeenRightsPluginId, 4) == 0)
        return 0;
    if (length == 8 &&
        RwStreamReadInt32(stream, (RwInt32*)&lastSeenExtraData, 4) == 0)
        return 0;
    return stream;
}

RwStream* _rpWriteAtomicRights(RwStream* stream, RwInt32 length,
                               const void* object, RwInt32 offset, RwInt32 size)
{
    const RpAtomic* atomic = object;
    if (RwStreamWriteInt32(stream, (const RwInt32*)&atomic->pipeline->pluginId, 4) == 0)
        return 0;
    if (RwStreamWriteInt32(stream, (const RwInt32*)&atomic->pipeline->pluginData, 4) == 0)
        return 0;
    return stream;
}

RwInt32 _rpSizeAtomicRights(const void* object, RwInt32 offset, RwInt32 size)
{
    const RpAtomic* atomic = object;
    if (atomic->pipeline != 0) {
        if (atomic->pipeline->pluginId != 0)
            return 8;
    }
    return 0;
}


static RpAtomic* AtomicSync(RpAtomic* atomic)
{
    if (atomic->interpolator.flags & 2)
        _rpAtomicResyncInterpolatedSphere(atomic);
    atomic->object.privateFlags |= 1;
    return atomic;
}

RpAtomic* AtomicDefaultRenderCallBack(RpAtomic* atomic)
{
    RxPipeline* pipeline = atomic->pipeline;
    if (pipeline == 0)
        pipeline = RxPipelineGlobals()->defaultAtomicPipeline;
    if (RxPipelineExecute(pipeline, atomic, 1) != 0)
        return atomic;
    return 0;
}

static RpGeometryList* GeometryListDeinitialize(RpGeometryList* geometryList)
{
    RwInt32 i;
    for (i = 0; i < geometryList->numGeometries; i++)
        RpGeometryDestroy(geometryList->geometries[i]);
    if (geometryList->geometries != 0) {
        RwEngineInstance->fpFree(geometryList->geometries);
        geometryList->geometries = 0;
    }
    return geometryList;
}


static void* ClumpInitCameraExt(void* object, RwInt32 offset, RwInt32 size)
{
    RpClumpObjectExtension* ext =
        (RpClumpObjectExtension*)((RwUInt8*)object +
                                  _rpClumpCameraExtOffset);
    ext->inClumpLink.next = ext->inClumpLink.prev = 0;
    ext->clump = 0;
    return object;
}

static void* ClumpDeInitCameraExt(void* object, RwInt32 offset, RwInt32 size)
{
    return object;
}

static void* ClumpInitLightExt(void* object, RwInt32 offset, RwInt32 size)
{
    RpClumpObjectExtension* ext =
        (RpClumpObjectExtension*)((RwUInt8*)object +
                                  _rpClumpLightExtOffset);
    ext->inClumpLink.next = ext->inClumpLink.prev = 0;
    ext->clump = 0;
    return object;
}

static void* ClumpDeInitLightExt(void* object, RwInt32 offset, RwInt32 size)
{
    return object;
}

static RpLight* DestroyClumpLight(RpLight* light, void* data)
{
    RpClumpObjectExtension* ext = (RpClumpObjectExtension*)((RwUInt8*)light + _rpClumpLightExtOffset);
    RpClumpRemoveLight(ext->clump, light);
    RpLightDestroy(light);
    return light;
}

static RwCamera* DestroyClumpCamera(RwCamera* camera, void* data)
{
    RpClumpObjectExtension* ext = (RpClumpObjectExtension*)((RwUInt8*)camera + _rpClumpCameraExtOffset);
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
        RwReal alpha = interp->position * interp->recipTime;
        atomic->boundingSphere.radius =
            start->sphere.radius +
            alpha * (end->sphere.radius - start->sphere.radius);
        atomic->boundingSphere.x = end->sphere.x - start->sphere.x;
        atomic->boundingSphere.y = end->sphere.y - start->sphere.y;
        atomic->boundingSphere.z = end->sphere.z - start->sphere.z;
        atomic->boundingSphere.x *= alpha;
        atomic->boundingSphere.y *= alpha;
        atomic->boundingSphere.z *= alpha;
        atomic->boundingSphere.x += start->sphere.x;
        atomic->boundingSphere.y += start->sphere.y;
        atomic->boundingSphere.z += start->sphere.z;
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
            RwReal sx;
            RwReal sy;
            RwReal sz;
            RwReal scale;
            RwReal scaleSquared;
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

void* _rpClumpClose(void* instance, RwInt32 offset, RwInt32 size)
{
    RwFreeListForAllUsed(ClumpGlobals()->clumpFreeList, ClumpTidyDestroyClump, 0);
    RwFreeListForAllUsed(ClumpGlobals()->atomicFreeList, ClumpTidyDestroyAtomic, 0);
    RwFreeListDestroy(ClumpGlobals()->atomicFreeList);
    RwFreeListDestroy(ClumpGlobals()->clumpFreeList);
    ClumpGlobals()->atomicFreeList = 0;
    ClumpGlobals()->clumpFreeList = 0;
    clumpModule.numInstances--;
    return instance;
}

void* _rpClumpOpen(void* instance, RwInt32 offset, RwInt32 size)
{
    clumpModule.globalsOffset = offset;
    ClumpGlobals()->atomicFreeList = RwFreeListCreateAndPreallocateSpace(
        atomicTKList.sizeOfStruct, _rpAtomicFreeListBlockSize, 4,
        _rpAtomicFreeListPreallocBlocks, &_rpAtomicFreeList, 0x40010);
    if (ClumpGlobals()->atomicFreeList != 0) {
        ClumpGlobals()->clumpFreeList = RwFreeListCreateAndPreallocateSpace(
            clumpTKList.sizeOfStruct, _rpClumpFreeListBlockSize, 4,
            _rpClumpFreeListPreallocBlocks, &_rpClumpFreeList, 0x40014);
        if (ClumpGlobals()->clumpFreeList != 0) {
            clumpModule.numInstances++;
            return instance;
        }
        RwFreeListDestroy(ClumpGlobals()->atomicFreeList);
        ClumpGlobals()->atomicFreeList = 0;
    }
    return 0;
}

RwBool _rpClumpRegisterExtensions(void)
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
        RwCamera* camera = (RwCamera*)((RwUInt8*)link - 4 - _rpClumpCameraExtOffset);
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
        RpLight* light = (RpLight*)((RwUInt8*)link - 4 - _rpClumpLightExtOffset);
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
    atomic->boundingSphere.x = 0.0f;
    atomic->boundingSphere.y = 0.0f;
    atomic->boundingSphere.z = 0.0f;
    atomic->worldBoundingSphere.radius = 0.0f;
    atomic->worldBoundingSphere.x = 0.0f;
    atomic->worldBoundingSphere.y = 0.0f;
    atomic->worldBoundingSphere.z = 0.0f;
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


RpAtomic* RpAtomicSetGeometry(RpAtomic* atomic, RpGeometry* geometry, RwUInt32 flags)
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

RwBool RpAtomicDestroy(RpAtomic* atomic)
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

RwBool RpClumpDestroy(RpClump* clump)
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
    rwLinkListAddLLLink(&clump->atomicList, &atomic->inClumpLink);
    atomic->clump = clump;
    return clump;
}


RpClump* RpClumpRemoveLight(RpClump* clump, RpLight* light)
{
    RpClumpObjectExtension* ext = (RpClumpObjectExtension*)((RwUInt8*)light + _rpClumpLightExtOffset);
    rwLinkListRemoveLLLink(&ext->inClumpLink);
    ext->inClumpLink.prev = 0;
    ext->inClumpLink.next = 0;
    ext->clump = 0;
    return clump;
}

RpClump* RpClumpRemoveCamera(RpClump* clump, RwCamera* camera)
{
    RpClumpObjectExtension* ext = (RpClumpObjectExtension*)((RwUInt8*)camera + _rpClumpCameraExtOffset);
    rwLinkListRemoveLLLink(&ext->inClumpLink);
    ext->inClumpLink.prev = 0;
    ext->inClumpLink.next = 0;
    ext->clump = 0;
    return clump;
}

RwInt32 RpAtomicRegisterPlugin(RwInt32 size, RwUInt32 id, RwPluginObjectConstructor ctor, RwPluginObjectDestructor dtor, RwPluginObjectCopy copy)
{
    RwInt32 offset = _rwPluginRegistryAddPlugin(
        &atomicTKList, size, id, ctor, dtor, copy);
    return offset;
}
RwInt32 RpClumpRegisterPlugin(RwInt32 size, RwUInt32 id, RwPluginObjectConstructor ctor, RwPluginObjectDestructor dtor, RwPluginObjectCopy copy)
{
    RwInt32 offset = _rwPluginRegistryAddPlugin(
        &clumpTKList, size, id, ctor, dtor, copy);
    return offset;
}
RwInt32 RpAtomicRegisterPluginStream(RwUInt32 id, RwPluginDataChunkReadCallBack read, RwPluginDataChunkWriteCallBack write, RwPluginDataChunkGetSizeCallBack size)
{
    RwInt32 offset = _rwPluginRegistryAddPluginStream(
        &atomicTKList, id, read, write, size);
    return offset;
}
RwInt32 RpAtomicSetStreamAlwaysCallBack(RwUInt32 id, RwPluginDataChunkAlwaysCallBack callback)
{
    RwInt32 offset = _rwPluginRegistryAddPlgnStrmlwysCB(
        &atomicTKList, id, callback);
    return offset;
}
RwInt32 RpAtomicSetStreamRightsCallBack(RwUInt32 id, RwPluginDataChunkRightsCallBack callback)
{
    RwInt32 offset = _rwPluginRegistryAddPlgnStrmRightsCB(
        &atomicTKList, id, callback);
    return offset;
}
RwInt32 RpClumpRegisterPluginStream(RwUInt32 id, RwPluginDataChunkReadCallBack read, RwPluginDataChunkWriteCallBack write, RwPluginDataChunkGetSizeCallBack size)
{
    RwInt32 offset = _rwPluginRegistryAddPluginStream(
        &clumpTKList, id, read, write, size);
    return offset;
}
RwInt32 RpAtomicGetPluginOffset(RwUInt32 id)
{
    RwInt32 offset = _rwPluginRegistryGetPluginOffset(&atomicTKList, id);
    return offset;
}

RpAtomic* RpAtomicSetFrame(RpAtomic* atomic, RwFrame* frame)
{
    _rwObjectHasFrameSetFrame(atomic, frame);
    atomic->object.privateFlags |= 1;
    return atomic;
}
