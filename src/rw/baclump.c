#include "libmkparticle/rw_engine.h"
#include "rw/rpworld_types.h"
#include "rw/rplight.h"
#include "rw/rwfreelist.h"
#include "rw/rwplcore.h"
#include "rw/rwstream.h"
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

typedef struct RpGeometryList {
    RpGeometry** geometries;
    RwInt32 numGeometries;
} RpGeometryList;

static RwPluginRegistry atomicTKList = {0x70, 0x70, 0, 0, NULL, NULL};
static RwPluginRegistry clumpTKList = {0x2C, 0x2C, 0, 0, NULL, NULL};
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

#define CLUMPGLOBALS ((RpClumpGlobals*)((RwUInt8*)RwEngineInstance + clumpModule.globalsOffset))

extern void RwResourcesFreeResEntry(RwResEntry*);
extern RwBool RwCameraDestroy(RwCamera*);
extern RwInt32 RwCameraRegisterPlugin(RwInt32, RwUInt32,
                                     RwPluginObjectConstructor,
                                     RwPluginObjectDestructor,
                                     RwPluginObjectCopy);
extern RpWorld* RpAtomicGetWorld(RpAtomic*);
extern RwReal _rwSqrt(RwReal);
extern RwInt32 _rxPipelineGlobalsOffset;

void RpClumpRemoveLight(RpClump*, RpLight*);
void RpClumpRemoveCamera(RpClump*, RwCamera*);

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
        return NULL;
    if (length == 8 &&
        RwStreamReadInt32(stream, (RwInt32*)&lastSeenExtraData, 4) == 0)
        return NULL;
    return stream;
}

RwStream* _rpWriteAtomicRights(RwStream* stream, RwInt32 length,
                               const void* object, RwInt32 offset, RwInt32 size)
{
    const RpAtomic* atomic = object;
    if (RwStreamWriteInt32(stream, (const RwInt32*)&atomic->pipeline->pluginId, 4) == 0)
        return NULL;
    if (RwStreamWriteInt32(stream, (const RwInt32*)&atomic->pipeline->pluginData, 4) == 0)
        return NULL;
    return stream;
}

RwInt32 _rpSizeAtomicRights(const void* object, RwInt32 offset, RwInt32 size)
{
    const RpAtomic* atomic = object;
    if (atomic->pipeline != NULL) {
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
    if (pipeline == NULL)
        pipeline = RXPIPELINEGLOBAL(defaultAtomicPipeline);
    if (RxPipelineExecute(pipeline, atomic, TRUE) != NULL)
        return atomic;
    return NULL;
}

static RpGeometryList* GeometryListDeinitialize(RpGeometryList* geometryList)
{
    RwInt32 i;
    for (i = 0; i < geometryList->numGeometries; i++)
        RpGeometryDestroy(geometryList->geometries[i]);
    if (geometryList->geometries != NULL) {
        RwEngineInstance->fpFree(geometryList->geometries);
        geometryList->geometries = NULL;
    }
    return geometryList;
}

static void* ClumpInitCameraExt(void* object, RwInt32 offset, RwInt32 size)
{
    RpClumpObjectExtension* ext =
        (RpClumpObjectExtension*)((RwUInt8*)object +
                                  _rpClumpCameraExtOffset);
    ext->inClumpLink.prev = NULL;
    ext->inClumpLink.next = NULL;
    ext->clump = NULL;
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
    ext->inClumpLink.prev = NULL;
    ext->inClumpLink.next = NULL;
    ext->clump = NULL;
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
    if (geometry == NULL)
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
        atomic->boundingSphere.radius = start->sphere.radius + alpha * (end->sphere.radius - start->sphere.radius);
        atomic->boundingSphere.x = start->sphere.x + alpha * (end->sphere.x - start->sphere.x);
        atomic->boundingSphere.y = start->sphere.y + alpha * (end->sphere.y - start->sphere.y);
        atomic->boundingSphere.z = start->sphere.z + alpha * (end->sphere.z - start->sphere.z);
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
        RwReal scale;
        RwV3dTransformPoint((RwV3d*)&atomic->worldBoundingSphere,
                            (const RwV3d*)&atomic->boundingSphere, matrix);
        if ((matrix->flags & 3) != 3) {
            RwReal sx = matrix->right.x * matrix->right.x + matrix->right.y * matrix->right.y + matrix->right.z * matrix->right.z;
            RwReal sy = matrix->up.x * matrix->up.x + matrix->up.y * matrix->up.y + matrix->up.z * matrix->up.z;
            RwReal sz = matrix->at.x * matrix->at.x + matrix->at.y * matrix->at.y + matrix->at.z * matrix->at.z;
            scale = sx > sy ? (sx > sz ? sx : sz) : (sy > sz ? sy : sz);
            atomic->worldBoundingSphere.radius = atomic->boundingSphere.radius * _rwSqrt(scale);
        } else {
            atomic->worldBoundingSphere.radius = atomic->boundingSphere.radius;
        }
        atomic->object.privateFlags &= ~1;
    }
    return &atomic->worldBoundingSphere;
}

void* _rpClumpClose(void* instance, RwInt32 offset, RwInt32 size)
{
    RwFreeListForAllUsed(CLUMPGLOBALS->clumpFreeList, ClumpTidyDestroyClump, NULL);
    RwFreeListForAllUsed(CLUMPGLOBALS->atomicFreeList, ClumpTidyDestroyAtomic, NULL);
    RwFreeListDestroy(CLUMPGLOBALS->atomicFreeList);
    RwFreeListDestroy(CLUMPGLOBALS->clumpFreeList);
    CLUMPGLOBALS->atomicFreeList = NULL;
    CLUMPGLOBALS->clumpFreeList = NULL;
    clumpModule.numInstances--;
    return instance;
}

void* _rpClumpOpen(void* instance, RwInt32 offset, RwInt32 size)
{
    clumpModule.globalsOffset = offset;
    CLUMPGLOBALS->atomicFreeList = RwFreeListCreateAndPreallocateSpace(
        atomicTKList.sizeOfStruct, _rpAtomicFreeListBlockSize, 4,
        _rpAtomicFreeListPreallocBlocks, &_rpAtomicFreeList, 0x40010);
    if (CLUMPGLOBALS->atomicFreeList == NULL)
        return NULL;
    CLUMPGLOBALS->clumpFreeList = RwFreeListCreateAndPreallocateSpace(
        clumpTKList.sizeOfStruct, _rpClumpFreeListBlockSize, 4,
        _rpClumpFreeListPreallocBlocks, &_rpClumpFreeList, 0x40014);
    if (CLUMPGLOBALS->clumpFreeList == NULL) {
        RwFreeListDestroy(CLUMPGLOBALS->atomicFreeList);
        CLUMPGLOBALS->atomicFreeList = NULL;
        return NULL;
    }
    clumpModule.numInstances++;
    return instance;
}

RwBool _rpClumpRegisterExtensions(void)
{
    _rpClumpCameraExtOffset = RwCameraRegisterPlugin(12, 0x10, ClumpInitCameraExt, ClumpDeInitCameraExt, NULL);
    if (_rpClumpCameraExtOffset < 0)
        return FALSE;
    _rpClumpLightExtOffset = RpLightRegisterPlugin(12, 0x10, ClumpInitLightExt, ClumpDeInitLightExt, NULL);
    if (_rpClumpLightExtOffset < 0)
        return FALSE;
    return TRUE;
}

RpClump* RpClumpRender(RpClump* clump)
{
    RpClump* result = clump;
    RwLLLink* link = clump->atomicList.link.next;
    while (link != &clump->atomicList.link) {
        RpAtomic* atomic = RP_ATOMIC_FROM_CLUMP_LINK(link);
        if (atomic->object.flags & 4) {
            RwFrameGetLTM(atomic->object.parent);
            if (atomic->renderCallBack(atomic) == NULL)
                result = NULL;
        }
        link = link->next;
    }
    return result;
}

RpClump* RpClumpForAllAtomics(RpClump* clump, RpAtomicCallBack callback, void* data)
{
    RpAtomicCallBack callBack = callback;
    void* callBackData = data;
    RwLLLink* link = clump->atomicList.link.next;
    while (link != &clump->atomicList.link) {
        RwLLLink* next = link->next;
        if (callBack(RP_ATOMIC_FROM_CLUMP_LINK(link), callBackData) == NULL)
            return clump;
        link = next;
    }
    return clump;
}

RpClump* RpClumpForAllCameras(RpClump* clump,
                              RwCamera* (*callback)(RwCamera*, void*),
                              void* data)
{
    RwCamera* (*callBack)(RwCamera*, void*) = callback;
    void* callBackData = data;
    RwLLLink* link = clump->cameraList.link.next;
    while (link != &clump->cameraList.link) {
        RwLLLink* next = link->next;
        RwCamera* camera = (RwCamera*)((RwUInt8*)link - 4 - _rpClumpCameraExtOffset);
        if (callBack(camera, callBackData) == NULL)
            return clump;
        link = next;
    }
    return clump;
}

RpClump* RpClumpForAllLights(RpClump* clump, RpLightCallBack callback, void* data)
{
    RpLightCallBack callBack = callback;
    void* callBackData = data;
    RwLLLink* link = clump->lightList.link.next;
    while (link != &clump->lightList.link) {
        RwLLLink* next = link->next;
        RpLight* light = (RpLight*)((RwUInt8*)link - 4 - _rpClumpLightExtOffset);
        if (callBack(light, callBackData) == NULL)
            return clump;
        link = next;
    }
    return clump;
}

RpAtomic* RpAtomicCreate(void)
{
    RpAtomic* atomic = RwEngineInstance->fpFreeListAlloc(
        CLUMPGLOBALS->atomicFreeList, 0x30014);
    if (atomic == NULL)
        return NULL;
    rwObjectInitialize(atomic, 1, 0);
    atomic->object.flags = 5;
    atomic->object.privateFlags = 1;
    atomic->sync = AtomicSync;
    _rwObjectHasFrameSetFrame(atomic, NULL);
    atomic->repEntry = NULL;
    atomic->geometry = NULL;
    atomic->boundingSphere.x = atomic->boundingSphere.y = atomic->boundingSphere.z = atomic->boundingSphere.radius = 0.0f;
    atomic->worldBoundingSphere = atomic->boundingSphere;
    atomic->clump = NULL;
    atomic->inClumpLink.next = atomic->inClumpLink.prev = NULL;
    atomic->renderCallBack = AtomicDefaultRenderCallBack;
    atomic->interpolator.flags = 3;
    atomic->interpolator.startMorphTarget = atomic->interpolator.endMorphTarget = 0;
    atomic->interpolator.time = atomic->interpolator.recipTime = 1.0f;
    atomic->interpolator.position = 0.0f;
    rwLinkListInitialize(&atomic->worldSectorsInAtomic);
    atomic->pipeline = NULL;
    _rwPluginRegistryInitObject(&atomicTKList, atomic);
    return atomic;
}

RpAtomic* RpAtomicSetGeometry(RpAtomic* atomic, RpGeometry* geometry, RwUInt32 flags)
{
    if (atomic->geometry != geometry) {
        if (geometry != NULL)
            _rpGeometryAddRef(geometry);
        if (atomic->geometry != NULL)
            RpGeometryDestroy(atomic->geometry);
        atomic->geometry = geometry;
        if (!(flags & 1)) {
            if (geometry != NULL)
                atomic->boundingSphere = geometry->morphTarget[0].sphere;
            if (atomic->object.parent != NULL && RpAtomicGetWorld(atomic) != NULL)
                RwFrameUpdateObjects(atomic->object.parent);
        }
    }
    return atomic;
}

RwBool RpAtomicDestroy(RpAtomic* atomic)
{
    _rwPluginRegistryDeInitObject(&atomicTKList, atomic);
    if (atomic->repEntry != NULL)
        RwResourcesFreeResEntry(atomic->repEntry);
    RpAtomicSetGeometry(atomic, NULL, 0);
    _rwObjectHasFrameReleaseFrame(atomic);
    RwEngineInstance->fpFreeListFree(CLUMPGLOBALS->atomicFreeList, atomic);
    return TRUE;
}

void RpClumpSetCallBack(RpClump* clump, RpClumpCallBack callback)
{
    if (callback != NULL) {
        clump->callback = callback;
        return;
    }
    clump->callback = ClumpCallBack;
}

RpClump* RpClumpCreate(void)
{
    RpClump* clump = RwEngineInstance->fpFreeListAlloc(
        CLUMPGLOBALS->clumpFreeList, 0x30010);
    if (clump == NULL)
        return NULL;
    rwObjectInitialize(clump, 2, 0);
    rwLinkListInitialize(&clump->atomicList);
    rwLinkListInitialize(&clump->lightList);
    rwLinkListInitialize(&clump->cameraList);
    clump->inWorldLink.next = clump->inWorldLink.prev = NULL;
    RpClumpSetCallBack(clump, NULL);
    _rwPluginRegistryInitObject(&clumpTKList, clump);
    return clump;
}

RwBool RpClumpDestroy(RpClump* clump)
{
    RwFrame* frame;
    _rwPluginRegistryDeInitObject(&clumpTKList, clump);
    RpClumpForAllAtomics(clump, DestroyClumpAtomic, NULL);
    RpClumpForAllLights(clump, DestroyClumpLight, NULL);
    RpClumpForAllCameras(clump, DestroyClumpCamera, NULL);
    frame = clump->object.parent;
    if (frame != NULL)
        RwFrameDestroyHierarchy(frame);
    RwEngineInstance->fpFreeListFree(CLUMPGLOBALS->clumpFreeList, clump);
    return TRUE;
}

RpClump* RpClumpAddAtomic(RpClump* clump, RpAtomic* atomic)
{
    rwLinkListAddLLLink(&clump->atomicList, &atomic->inClumpLink);
    atomic->clump = clump;
    return clump;
}

void RpClumpRemoveLight(RpClump* clump, RpLight* light)
{
    RpClumpObjectExtension* ext = (RpClumpObjectExtension*)((RwUInt8*)light + _rpClumpLightExtOffset);
    rwLinkListRemoveLLLink(&ext->inClumpLink);
    ext->inClumpLink.prev = NULL;
    ext->inClumpLink.next = NULL;
    ext->clump = NULL;
}

void RpClumpRemoveCamera(RpClump* clump, RwCamera* camera)
{
    RpClumpObjectExtension* ext = (RpClumpObjectExtension*)((RwUInt8*)camera + _rpClumpCameraExtOffset);
    rwLinkListRemoveLLLink(&ext->inClumpLink);
    ext->inClumpLink.prev = NULL;
    ext->inClumpLink.next = NULL;
    ext->clump = NULL;
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
