#include "libmkparticle/rw_engine.h"
#include "runtime/cstring.h"
#include "rw/rplight.h"
#include "rw/rpworld_types.h"
#include "rw/rwerror.h"
#include "rw/rwcamera_internal.h"
#include "rw/rwfreelist.h"
#include "rw/rwplcore.h"
#include "rw/rwstream.h"

static void WorldObjectSetError(RwInt32 code)
{
    RwError error;
    error.pluginID = 2;
    error.errorCode = _rwerror(code);
    RwErrorSet(&error);
}

static void WorldObjectSetErrorWithValue(RwInt32 code, RwInt32 value)
{
    RwError error;
    error.pluginID = 2;
    error.errorCode = _rwerror(code, value);
    RwErrorSet(&error);
}

static RwReal WorldObjectCoordinate(const RwV3d* vector, RwInt32 axis)
{
    return *(const RwReal*)((const RwUInt8*)&vector->x + axis);
}

RwStream* _rpGeometryNativeWrite(RwStream* stream,
                                 const RpGeometry* geometry);
RpGeometry* _rpGeometryNativeRead(RwStream* stream, RpGeometry* geometry);
RwInt32 _rpGeometryNativeSize(const RpGeometry* geometry);
RwStream* _rpWorldSectorNativeWrite(RwStream* stream,
                                    const RpWorldSector* sector);
RpWorldSector* _rpWorldSectorNativeRead(RwStream* stream,
                                        RpWorldSector* sector);
RwInt32 _rpWorldSectorNativeSize(const RpWorldSector* sector);
RwStream* _rpReadAtomicRights(RwStream*, RwInt32, void*, RwInt32, RwInt32);
RwStream* _rpWriteAtomicRights(RwStream*, RwInt32, const void*, RwInt32,
                               RwInt32);
RwInt32 _rpSizeAtomicRights(const void*, RwInt32, RwInt32);
RwStream* _rpReadWorldRights(RwStream*, RwInt32, void*, RwInt32, RwInt32);
RwStream* _rpWriteWorldRights(RwStream*, RwInt32, const void*, RwInt32,
                              RwInt32);
RwInt32 _rpSizeWorldRights(const void*, RwInt32, RwInt32);
RwStream* _rpReadSectRights(RwStream*, RwInt32, void*, RwInt32, RwInt32);
RwStream* _rpWriteSectRights(RwStream*, RwInt32, const void*, RwInt32,
                             RwInt32);
RwInt32 _rpSizeSectRights(const void*, RwInt32, RwInt32);
RwStream* _rpReadMaterialRights(RwStream*, RwInt32);
RwStream* _rpWriteMaterialRights(RwStream*, RwInt32, const RpMaterial*);
RwInt32 _rpSizeMaterialRights(const RpMaterial*);
RpWorld* RpWorldAddCamera(RpWorld*, RwCamera*);
RpWorld* RpWorldRemoveCamera(RpWorld*, RwCamera*);
RpWorld* RpWorldAddAtomic(RpWorld*, RpAtomic*);
RpWorld* RpWorldRemoveAtomic(RpWorld*, RpAtomic*);
RpWorld* RpWorldAddClump(RpWorld*, RpClump*);
RpWorld* RpWorldRemoveClump(RpWorld*, RpClump*);
RpWorld* RpWorldAddLight(RpWorld*, RpLight*);
RpWorld* RpWorldRemoveLight(RpWorld*, RpLight*);
extern RwMatrix* RwFrameGetLTM(RwFrame* frame);

typedef struct RpWorldCameraExt {
    RpWorldSector **frustumSectors;
    RwInt32 space;
    RwInt32 position;
    RpWorld *world;
    RwCameraBeginUpdateFunc oldBeginUpdate;
    RwCameraEndUpdateFunc oldEndUpdate;
    RwObjectHasFrameSyncFunction oldSync;
} RpWorldCameraExt;

typedef struct RpWorldAtomicExt {
    RpWorld *world;
    RwObjectHasFrameSyncFunction oldSync;
} RpWorldAtomicExt;

typedef struct RpWorldClumpExt {
    RpWorld *world;
    RwUInt32 clumpsInFrustumID;
} RpWorldClumpExt;

typedef struct RpWorldLightExt {
    RpWorld *world;
    RwObjectHasFrameSyncFunction oldSync;
} RpWorldLightExt;

typedef struct RpTie {
    RwLLLink lAtomicInWorldSector;
    RpAtomic* apAtom;
    RwLLLink lWorldSectorInAtomic;
    RpWorldSector* worldSector;
} RpTie;

typedef struct RpLightTie {
    RwLLLink lightInWorldSector;
    RpLight* light;
    RwLLLink WorldSectorInLight;
    RpWorldSector* sect;
} RpLightTie;

typedef struct rpWorldObjGlobals {
    RwFreeList *tieFreeList;
    RwFreeList *lightTieFreeList;
    RwUInt32 clumpsInFrustumID;
} rpWorldObjGlobals;

static RpWorld     *WorldSyncCamera(RpWorld * world, RwCamera * camera);

static RwInt32      cameraExtOffset = 0;
static RwInt32      atomicExtOffset = 0;
static RwInt32      clumpExtOffset = 0;
static RwInt32      lightExtOffset = 0;
static RwModuleInfo worldObjModule;

static RpWorldCameraExt *WorldCameraExtension(const void *camera)
{
    return (RpWorldCameraExt *)((RwUInt8 *)camera + cameraExtOffset);
}

static RpWorldAtomicExt *WorldAtomicExtension(const void *atomic)
{
    return (RpWorldAtomicExt *)((RwUInt8 *)atomic + atomicExtOffset);
}

static RpWorldClumpExt *WorldClumpExtension(const void *clump)
{
    return (RpWorldClumpExt *)((RwUInt8 *)clump + clumpExtOffset);
}

static RpWorldLightExt *WorldLightExtension(const void *light)
{
    return (RpWorldLightExt *)((RwUInt8 *)light + lightExtOffset);
}

static rpWorldObjGlobals *WorldObjectGlobals(void)
{
    return (rpWorldObjGlobals *)((RwUInt8 *)RwEngineInstance +
                                 worldObjModule.globalsOffset);
}



static RwInt32 _rpTieFreeListBlockSize = 0x100;
static RwInt32 _rpTieFreeListPreallocBlocks = 1;
static RwFreeList _rpTieFreeList;

static RwInt32 _rpLightTieFreeListBlockSize = 0x20;
static RwInt32 _rpLightTieFreeListPreallocBlocks = 1;
static RwFreeList _rpLightTieFreeList;

static void        *
WorldObjectOpen(void *instance, RwInt32 offset,
                RwInt32 size )
{
    worldObjModule.globalsOffset = offset;
    WorldObjectGlobals()->tieFreeList =
        RwFreeListCreateAndPreallocateSpace(sizeof(RpTie), _rpTieFreeListBlockSize,
                                             sizeof(RwUInt32),
                                             _rpTieFreeListPreallocBlocks,
                                             &_rpTieFreeList,
                                             0x00040000 |
                                                 0x507);
    if (WorldObjectGlobals()->tieFreeList == 0) {
        return 0;
    }

    WorldObjectGlobals()->lightTieFreeList =
        RwFreeListCreateAndPreallocateSpace(
            sizeof(RpLightTie), _rpLightTieFreeListBlockSize,
            sizeof(RwUInt32), _rpLightTieFreeListPreallocBlocks,
            &_rpLightTieFreeList, 0x00040000 | 0x507);
    if (WorldObjectGlobals()->lightTieFreeList == 0) {
        RwFreeListDestroy(WorldObjectGlobals()->tieFreeList);
        WorldObjectGlobals()->tieFreeList = 0;
        return 0;
    }

    RwEngineInstance->renderFrame = 1;
    WorldObjectGlobals()->clumpsInFrustumID = 0;
    worldObjModule.numInstances++;
    return instance;
}

static void        *
WorldObjectClose(void *instance,
                 RwInt32 offset ,
                 RwInt32 size )
{
    if (WorldObjectGlobals()->lightTieFreeList != 0) {
        RwFreeListDestroy(WorldObjectGlobals()->lightTieFreeList);
        WorldObjectGlobals()->lightTieFreeList = 0;
    }
    if (WorldObjectGlobals()->tieFreeList != 0) {
        RwFreeListDestroy(WorldObjectGlobals()->tieFreeList);
        WorldObjectGlobals()->tieFreeList = 0;
    }
    worldObjModule.numInstances--;
    return instance;
}



static              RwBool
SectorsInFrustumDeinitialise(RpWorldCameraExt * cameraExt)
{
    if (cameraExt->frustumSectors != 0) {
        RwEngineInstance->fpFree(cameraExt->frustumSectors);
    }
    cameraExt->frustumSectors = 0;
    cameraExt->space = 0;
    cameraExt->position = 0;
    return 1;
}

static              RwBool
SectorsInFrustumAddSpace(RpWorldCameraExt * cameraExt, RwInt32 nNum)
{
    RpWorldSector     **newFrustumSectors;
    RwInt32 memSize =
        (cameraExt->space + nNum) * sizeof(*cameraExt->frustumSectors);

    if (cameraExt->frustumSectors != 0) {
        newFrustumSectors = RwEngineInstance->fpRealloc(
            cameraExt->frustumSectors, memSize,
            0x00030000 | 0x507 |
                0x01000000);
    } else {
        newFrustumSectors = RwEngineInstance->fpMalloc(
            memSize, 0x00030000 | 0x507 |
                         0x01000000);
    }

    if (newFrustumSectors != 0) {
        cameraExt->frustumSectors = newFrustumSectors;
        cameraExt->space += nNum;
        return 1;
    }
    WorldObjectSetErrorWithValue(0x80000013, memSize);
    return 0;
}

static RwCamera    *
WorldCameraBeginUpdate(RwCamera * camera)
{
    RpWorldCameraExt   *cameraExt = WorldCameraExtension(camera);
    RwEngineInstance->curWorld = cameraExt->world;
    RwEngineInstance->renderFrame++;
    return cameraExt->oldBeginUpdate(camera);
}

static RwCamera    *
WorldCameraEndUpdate(RwCamera * camera)
{
    RpWorldCameraExt   *cameraExt = WorldCameraExtension(camera);
    RwEngineInstance->curWorld = 0;
    return cameraExt->oldEndUpdate(camera);
}

static RwObjectHasFrame *
WorldCameraSync(RwObjectHasFrame * object)
{
    RpWorldCameraExt   *cameraExt = WorldCameraExtension(object);
    RpWorld *world;

    if (cameraExt->oldSync(object) == 0) {
        return 0;
    }
    world = cameraExt->world;
    if (world != 0) {
        RwStandardFunc hintRenderFrontToBack =
            RwEngineInstance->fpHintRenderFrontToBack;
        WorldSyncCamera(world, (RwCamera *)object);
        hintRenderFrontToBack(0, 0,
                              world->renderOrder == rpWORLDRENDERFRONT2BACK);
    }
    return object;
}

static void        *
WorldInitCameraExt(void *object,
                   RwInt32 offsetInObject ,
                   RwInt32 sizeInObject )
{
    RpWorldCameraExt *extension = WorldCameraExtension(object);
    RwCamera           *camera = (RwCamera *) object;
    extension->frustumSectors = 0;
    extension->space = 0;
    extension->position = 0;
    extension->oldBeginUpdate = camera->beginUpdate;
    extension->oldEndUpdate = camera->endUpdate;
    extension->oldSync = camera->object.sync;
    camera->object.sync = WorldCameraSync;
    camera->beginUpdate = WorldCameraBeginUpdate;
    camera->endUpdate = WorldCameraEndUpdate;

    extension->world = 0;
    return object;
}

static void        *
WorldCopyCameraExt(void *dstObject,
                   const void *srcObject,
                   RwInt32 offsetInObject ,
                   RwInt32 sizeInObject )
{
    RpWorldCameraExt *destinationExt =
        WorldCameraExtension(dstObject);
    const RpWorldCameraExt *sourceExt =
        WorldCameraExtension(srcObject);

    destinationExt->frustumSectors = 0;
    destinationExt->space = 0;
    destinationExt->position = 0;
    if (sourceExt->world != 0) {
        RpWorldAddCamera(sourceExt->world, (RwCamera *) dstObject);
    }
    return dstObject;
}

static void        *
WorldDeInitCameraExt(void *object,
                     RwInt32 offsetInObject ,
                     RwInt32 sizeInObject )
{
    RpWorldCameraExt   *cameraExt = WorldCameraExtension(object);
    RwCamera           *camera = (RwCamera *) object;

    SectorsInFrustumDeinitialise(cameraExt);

    camera->beginUpdate = cameraExt->oldBeginUpdate;
    camera->endUpdate = cameraExt->oldEndUpdate;
    camera->object.sync = cameraExt->oldSync;
    return object;
}



RwBool
_rpLightTieDestroy(RpLightTie * tie)
{
    rwLinkListRemoveLLLink(&tie->WorldSectorInLight);
    rwLinkListRemoveLLLink(&tie->lightInWorldSector);
    RwEngineInstance->fpFreeListFree(WorldObjectGlobals()->lightTieFreeList, tie);
    return 1;
}

RwBool
_rpTieDestroy(RpTie * tie)
{
    if (tie->apAtom != 0 && tie->worldSector != 0) {
        rwLinkListRemoveLLLink(&tie->lWorldSectorInAtomic);
        rwLinkListRemoveLLLink(&tie->lAtomicInWorldSector);
        RwEngineInstance->fpFreeListFree(WorldObjectGlobals()->tieFreeList, tie);
    }
    return 1;
}

static void
AtomicDestroyTies(RpAtomic * atomic)
{
    RwLLLink *link = atomic->worldSectorsInAtomic.link.next;
    RwLLLink *end = &atomic->worldSectorsInAtomic.link;

    while (link != end) {
        RpTie *tie = (RpTie *)((RwUInt8 *)link - 12);
        link = link->next;
        _rpTieDestroy(tie);
    }
}

static void
WorldAttachAtomicSphere(RpWorld * world, RpAtomic * atomic)
{
    RpSector *sector = world->rootSector;
    RpSector *stack[64];
    const RwSphere *sphere = RpAtomicGetWorldBoundingSphere(atomic);
    RwV3d lower = sphere->center;
    RwV3d upper = sphere->center;
    RwInt32 stackDepth = 0;

    lower.x -= sphere->radius;
    lower.y -= sphere->radius;
    lower.z -= sphere->radius;
    upper.x += sphere->radius;
    upper.y += sphere->radius;
    upper.z += sphere->radius;

    do {
        if (sector->type < 0) {
            RpTie *tie = RwEngineInstance->fpFreeListAlloc(WorldObjectGlobals()->tieFreeList,
                                         0x00030000 |
                                             0x507);
            tie->worldSector = (RpWorldSector *)sector;
            tie->apAtom = atomic;
            rwLinkListAddLLLink(&((RpWorldSector *)sector)->collAtomicsInWorldSector,
                                &tie->lAtomicInWorldSector);
            rwLinkListAddLLLink(&atomic->worldSectorsInAtomic,
                                &tie->lWorldSectorInAtomic);
            sector = stack[stackDepth--];
        } else {
            RpPlaneSector *plane = (RpPlaneSector *)sector;
            if (WorldObjectCoordinate(&lower, plane->type) < plane->leftValue) {
                sector = plane->leftSubTree;
                if (plane->rightValue < WorldObjectCoordinate(&upper, plane->type)) {
                    stack[++stackDepth] = plane->rightSubTree;
                }
            } else if (plane->rightValue < WorldObjectCoordinate(&upper, plane->type)) {
                sector = plane->rightSubTree;
            } else {
                sector = stack[stackDepth--];
                }
        }
    } while (stackDepth >= 0);
}

static RwObjectHasFrame *
WorldAtomicSync(RwObjectHasFrame * type)
{
    RpAtomic           *atomic = (RpAtomic *) type;
    RpWorldAtomicExt   *atomicExt = WorldAtomicExtension(type);
    RpWorld *world;

    if (atomicExt->oldSync(type) == 0) {
        return 0;
    }
    world = atomicExt->world;
    if (world != 0) {
        AtomicDestroyTies(atomic);
        WorldAttachAtomicSphere(world, atomic);
    }
    return type;
}

static void        *
WorldInitAtomicExt(void *object,
                   RwInt32 offset ,
                   RwInt32 size )
{
    RpWorldAtomicExt   *atomicExt = WorldAtomicExtension(object);
    RpAtomic           *atomic = (RpAtomic *) object;

    atomicExt->world = 0;
    atomic->renderFrame = RwEngineInstance->renderFrame - 1;
    atomicExt->oldSync = (RwObjectHasFrameSyncFunction)atomic->sync;
    atomic->sync = WorldAtomicSync;
    return object;
}

static void        *
WorldCopyAtomicExt(void *dstObject,
                   const void * srcObject ,
                   RwInt32 offset ,
                   RwInt32 size )
{
    return dstObject;
}

static void        *
WorldDeInitAtomicExt(void *object,
                     RwInt32 offset ,
                     RwInt32 size )
{
    RpWorldAtomicExt   *atomicExt = WorldAtomicExtension(object);
    RpAtomic           *atomic = (RpAtomic *) object;

    AtomicDestroyTies(atomic);

    atomic->sync = atomicExt->oldSync;
    return object;
}

static void        *
WorldInitClumpExt(void *object,
                  RwInt32 offset ,
                  RwInt32 size )
{
    RpWorldClumpExt    *clumpExt = WorldClumpExtension(object);

    clumpExt->world = 0;
    clumpExt->clumpsInFrustumID = WorldObjectGlobals()->clumpsInFrustumID;
    return object;
}

static void        *
WorldCopyClumpExt(void *dstObject, const void *srcObject,
                  RwInt32 offset ,
                  RwInt32 size )
{
    const RpWorldClumpExt *srcClumpExt =
        WorldClumpExtension(srcObject);

    if (srcClumpExt->world != 0) {
        RpWorldAddClump(srcClumpExt->world, (RpClump *) dstObject);
    }
    return dstObject;
}

static void        *
WorldDeInitClumpExt(void *object,
                    RwInt32 offset ,
                    RwInt32 size )
{
    return object;
}

static void
LightDestroyTies(RpLight * light)
{
    RwLLLink *link = light->worldSectorsInLight.link.next;
    RwLLLink *end = &light->worldSectorsInLight.link;

    while (link != end) {
        RpLightTie *tie = (RpLightTie *)((RwUInt8 *)link - 12);
        link = link->next;
        _rpLightTieDestroy(tie);
    }
}

static RwObjectHasFrame *
WorldLightSync(RwObjectHasFrame * object)
{
    RpLight *light = (RpLight *)object;
    RpWorldLightExt *lightExt = WorldLightExtension(light);
    RpWorld *world;
    RwFrame *frame;

    if (lightExt->oldSync(object) == 0) {
        WorldObjectSetError(0x80000016);
        return object;
    }
    if (RpLightGetType(light) < 0x80) {
        return object;
    }

    world = lightExt->world;
    frame = RpLightGetFrame(light);
    if (world != 0 && frame != 0) {
        RpSector *sector = world->rootSector;
        RpSector *stack[64];
        RwV3d lower = RwFrameGetLTM(frame)->pos;
        RwV3d upper = lower;
        RwReal radius = light->radius;
        RwInt32 stackDepth = 0;

        LightDestroyTies(light);
        lower.x -= radius;
        lower.y -= radius;
        lower.z -= radius;
        upper.x += radius;
        upper.y += radius;
        upper.z += radius;

        do {
            if (sector->type < 0) {
                RpLightTie *tie = RwEngineInstance->fpFreeListAlloc(
                    WorldObjectGlobals()->lightTieFreeList,
                    0x00030000 | 0x507);
                    tie->sect = (RpWorldSector *)sector;
                    tie->light = light;
                    rwLinkListAddLLLink(&((RpWorldSector *)sector)->lightsInWorldSector,
                                        &tie->lightInWorldSector);
                    rwLinkListAddLLLink(&light->worldSectorsInLight,
                                        &tie->WorldSectorInLight);
                sector = stack[stackDepth--];
            } else {
                RpPlaneSector *plane = (RpPlaneSector *)sector;
                if (WorldObjectCoordinate(&lower, plane->type) < plane->leftValue) {
                    sector = plane->leftSubTree;
                    if (plane->rightValue < WorldObjectCoordinate(&upper, plane->type)) {
                        stack[++stackDepth] = plane->rightSubTree;
                    }
                } else if (plane->rightValue < WorldObjectCoordinate(&upper, plane->type)) {
                    sector = plane->rightSubTree;
                } else {
                    sector = stack[stackDepth--];
                }
            }
        } while (stackDepth >= 0);
    }
    return object;
}

static void        *
WorldInitLightExt(void *object,
                  RwInt32 offset ,
                  RwInt32 size )
{
    RpLight            *light = (RpLight *) object;
    RpWorldLightExt    *lightExt = WorldLightExtension(object);

    lightExt->world = 0;
    lightExt->oldSync = light->object.sync;
    light->object.sync = WorldLightSync;
    return object;
}

static void        *
WorldCopyLightExt(void *dstObject,
                  const void *srcObject,
                  RwInt32 offset ,
                  RwInt32 size )
{
    const RpWorldLightExt *srcLightExt =
        WorldLightExtension(srcObject);

    if (srcLightExt->world != 0) {
        RpWorldAddLight(srcLightExt->world, (RpLight *) dstObject);
    }
    return dstObject;
}

static void        *
WorldDeInitLightExt(void *object,
                    RwInt32 offset ,
                    RwInt32 size )
{
    RpLight            *light = (RpLight *) object;

    LightDestroyTies(light);
    return object;
}




static RpWorld     *
WorldSyncCamera(RpWorld * world, RwCamera * camera)
{
    RpWorldCameraExt *cameraExt = WorldCameraExtension(camera);
    RpSector *sector = cameraExt->world->rootSector;
    RpSector *stack[64];
    const RwFrustumPlane *planes = camera->frustumPlanes;
    RwV3d viewpoint = RwFrameGetLTM((RwFrame*)camera->object.object.parent)->pos;
    RwV3d lower = camera->frustumBoundBox.inf;
    RwV3d upper = camera->frustumBoundBox.sup;
    RwBool backToFront = world->renderOrder == rpWORLDRENDERBACK2FRONT;
    RwInt32 stackDepth = 0;
    RwInt32 count = 0;

    do {
        if (sector->type < 0) {
            RpWorldSector *worldSector = (RpWorldSector *)sector;
            const RwV3d *corners = (const RwV3d *)&worldSector->boundingBox;
            RwBool outside = 0;
            RwInt32 i;

            for (i = 0; i < 6; i++) {
                RwV3d corner;
                RwSplitBits side;
                corner.x = corners[planes[i].closestX].x;
                corner.y = corners[planes[i].closestY].y;
                corner.z = corners[planes[i].closestZ].z;
                side.nReal = RwV3dDotProduct(&corner, &planes[i].plane.normal) -
                             planes[i].plane.distance;
                outside = side.nInt > 0;
                if (outside) {
                    break;
                }
            }

            if (!outside) {
                if (count >= cameraExt->space) {
                    if (!SectorsInFrustumAddSpace(
                            cameraExt, 50)) {
                        cameraExt->position = count;
                        return world;
                    }
                }
                cameraExt->frustumSectors[count++] = worldSector;
            }
            sector = stack[stackDepth--];
        } else {
            RpPlaneSector *plane = (RpPlaneSector *)sector;
            RwSplitBits leftDistance;
            RwSplitBits rightDistance;

            leftDistance.nReal =
                WorldObjectCoordinate(&lower, plane->type) - plane->leftValue;
            rightDistance.nReal =
                plane->rightValue - WorldObjectCoordinate(&upper, plane->type);
            if (leftDistance.nInt < 0 && rightDistance.nInt < 0) {
                RwBool viewpointHigher =
                    WorldObjectCoordinate(&viewpoint, plane->type) > plane->value;
                if (backToFront == viewpointHigher) {
                    sector = plane->leftSubTree;
                    stack[++stackDepth] = plane->rightSubTree;
                } else {
                    sector = plane->rightSubTree;
                    stack[++stackDepth] = plane->leftSubTree;
                }
            } else {
                sector = leftDistance.nInt < 0 ? plane->leftSubTree
                                              : plane->rightSubTree;
            }
        }
    } while (stackDepth >= 0);

    cameraExt->position = count;
    return world;
}

static RpAtomic    *
WorldAddClumpAtomic(RpAtomic * atomic, void *data)
{
    RpWorldAddAtomic(data, atomic);
    return atomic;
}

static RpAtomic    *
WorldRemoveClumpAtomic(RpAtomic * atomic, void *data)
{
    RpWorld *world = data;
    RpWorldAtomicExt *atomicExt = WorldAtomicExtension(atomic);
    if (atomicExt != 0 && atomicExt->world == world) {
        RpWorldRemoveAtomic(world, atomic);
    }
    return atomic;
}


static RpLight    *
WorldAddClumpLight(RpLight *light, void *data)
{
    RpWorldAddLight(data, light);
    return light;
}

static RpLight    *
WorldRemoveClumpLight(RpLight * light, void *data)
{
    RpWorld *world = data;
    RpWorldLightExt *lightExt = WorldLightExtension(light);
    if (lightExt != 0 && lightExt->world == world) {
        RpWorldRemoveLight(world, light);
    }
    return light;
}

static RwCamera    *
WorldAddClumpCamera(RwCamera * camera, void *data)
{
    RpWorldAddCamera(data, camera);
    return camera;
}

static RwCamera    *
WorldRemoveClumpCamera(RwCamera * camera, void *data)
{
    RpWorld *world = data;
    RpWorldCameraExt *cameraExt = WorldCameraExtension(camera);
    if (cameraExt != 0 && cameraExt->world == world) {
        RpWorldRemoveCamera(world, camera);
    }
    return camera;
}


static RwStream    *
writeGeometryMesh(RwStream * output,
                  RwInt32 binaryLength ,
                  const void *pluginData,
                  RwInt32 offsetInObject ,
                  RwInt32 sizeInObject )
{
    const RpGeometry *geometry = pluginData;
    return _rpMeshWrite(geometry->meshHeader, geometry, output,
                        &geometry->matList);
}

static RwStream    *
readGeometryMesh(RwStream * stream,
                 RwInt32 binaryLength ,
                 void *object,
                 RwInt32 offsetInObject ,
                 RwInt32 sizeInObject )
{
    RpGeometry         *geometry = (RpGeometry *) object;
    geometry->meshHeader = _rpMeshRead(stream, geometry, &geometry->matList);
    return geometry->meshHeader != 0 ? stream : 0;
}

static RwInt32
sizeGeometryMesh(const void *pluginData,
                 RwInt32 offsetInObject ,
                 RwInt32 sizeInObject )
{
    const RpGeometry *geometry = pluginData;
    return _rpMeshSize(geometry->meshHeader, geometry);
}

static RwStream *
writeGeometryNative(RwStream *output,
                    RwInt32 binaryLength ,
                    const void *pluginData,
                    RwInt32 offsetInObject ,
                    RwInt32 sizeInObject )
{
    const RpGeometry *geometry = pluginData;
    return _rpGeometryNativeWrite(output, geometry);
}

static RwStream *
readGeometryNative(RwStream *stream,
                   RwInt32 binaryLength ,
                   void *object,
                   RwInt32 offsetInObject ,
                   RwInt32 sizeInObject )
{
    RpGeometry  *geometry = (RpGeometry *)object;
    return _rpGeometryNativeRead(stream, geometry) != 0 ? stream : 0;
}

static RwInt32
sizeGeometryNative(const void *pluginData,
                   RwInt32 offsetInObject ,
                   RwInt32 sizeInObject )
{
    const RpGeometry *geometry = pluginData;
    return _rpGeometryNativeSize(geometry);
}

static RwStream *
writeWorldSectorNative(RwStream *output,
                       RwInt32 binaryLength ,
                       const void *pluginData,
                       RwInt32 offsetInObject ,
                       RwInt32 sizeInObject )
{
    const RpWorldSector *sector = pluginData;
    return _rpWorldSectorNativeWrite(output, sector);
}

static RwStream *
readWorldSectorNative(RwStream *stream,
                      RwInt32 binaryLength ,
                      void *object,
                      RwInt32 offsetInObject ,
                      RwInt32 sizeInObject )
{
    RpWorldSector   *sector = (RpWorldSector *)object;
    return _rpWorldSectorNativeRead(stream, sector) != 0 ? stream : 0;
}

static RwInt32
sizeWorldSectorNative(const void *pluginData,
                      RwInt32 offsetInObject ,
                      RwInt32 sizeInObject )
{
    const RpWorldSector *sector = pluginData;
    return _rpWorldSectorNativeSize(sector);
}

static RwStream    *
writeSectorMesh(RwStream * output,
                RwInt32 binaryLength ,
                const void *pluginData,
                RwInt32 offsetInObject ,
                RwInt32 sizeInObject )
{
    const RpWorldSector *sector = pluginData;
    const RpWorld *world = RpWorldSectorGetWorld(sector);
    return _rpMeshWrite(sector->mesh, world, output, &world->matList);
}

static RwStream    *
readSectorMesh(RwStream * input,
               RwInt32 binaryLength ,
               void *pluginData,
               RwInt32 offsetInObject ,
               RwInt32 sizeInObject )
{
    RpWorldSector *sector = pluginData;
    const RpWorld *world = RpWorldSectorGetWorld(sector);
    sector->mesh = _rpMeshRead(input, world, &world->matList);
    return sector->mesh != 0 ? input : 0;
}

static RwInt32
sizeSectorMesh(const void *object,
               RwInt32 offsetInObject ,
               RwInt32 sizeInObject )
{
    const RpWorldSector *sector = (const RpWorldSector *)object;
    return _rpMeshSize(sector->mesh, RpWorldSectorGetWorld(sector));
}

RwBool
_rpWorldObjRegisterExtensions(void)
{
    RwInt32 registrations[21];
    RwInt32 status = 0;
    RwUInt32 count = 0;
    RwUInt32 i;

    registrations[count++] = RwEngineRegisterPlugin(
        sizeof(rpWorldObjGlobals), 0x509, WorldObjectOpen,
        WorldObjectClose);
    cameraExtOffset = RwCameraRegisterPlugin(sizeof(RpWorldCameraExt),
                                             0x509,
                                             WorldInitCameraExt,
                                             WorldDeInitCameraExt,
                                             WorldCopyCameraExt);
    registrations[count++] = cameraExtOffset;
    atomicExtOffset = RpAtomicRegisterPlugin(sizeof(RpWorldAtomicExt),
                                             0x509,
                                             WorldInitAtomicExt,
                                             WorldDeInitAtomicExt,
                                             WorldCopyAtomicExt);
    registrations[count++] = atomicExtOffset;
    clumpExtOffset = RpClumpRegisterPlugin(sizeof(RpWorldClumpExt),
                                           0x509,
                                           WorldInitClumpExt,
                                           WorldDeInitClumpExt,
                                           WorldCopyClumpExt);
    registrations[count++] = clumpExtOffset;
    lightExtOffset = RpLightRegisterPlugin(sizeof(RpWorldLightExt),
                                           0x509,
                                           WorldInitLightExt,
                                           WorldDeInitLightExt,
                                           WorldCopyLightExt);
    registrations[count++] = lightExtOffset;

    registrations[count++] = RpGeometryRegisterPlugin(
        0, 0x50E, 0, 0, 0);
    registrations[count++] = RpWorldSectorRegisterPlugin(
        0, 0x50E, 0, 0, 0);
    registrations[count++] = RpGeometryRegisterPluginStream(
        0x50E, readGeometryMesh, writeGeometryMesh,
        sizeGeometryMesh);
    registrations[count++] = RpWorldSectorRegisterPluginStream(
        0x50E, readSectorMesh, writeSectorMesh, sizeSectorMesh);

    registrations[count++] = RpGeometryRegisterPlugin(
        0, 0x510, 0, 0, 0);
    registrations[count++] = RpWorldSectorRegisterPlugin(
        0, 0x510, 0, 0, 0);
    registrations[count++] = RpGeometryRegisterPluginStream(
        0x510, readGeometryNative, writeGeometryNative,
        sizeGeometryNative);
    registrations[count++] = RpWorldSectorRegisterPluginStream(
        0x510, readWorldSectorNative, writeWorldSectorNative,
        sizeWorldSectorNative);

    registrations[count++] = RpAtomicRegisterPlugin(
        0, 0x1F, 0, 0, 0);
    registrations[count++] = RpAtomicRegisterPluginStream(
        0x1F, _rpReadAtomicRights, _rpWriteAtomicRights,
        _rpSizeAtomicRights);
    registrations[count++] = RpWorldRegisterPlugin(
        0, 0x1F, 0, 0, 0);
    registrations[count++] = RpWorldRegisterPluginStream(
        0x1F, _rpReadWorldRights, _rpWriteWorldRights,
        _rpSizeWorldRights);
    registrations[count++] = RpWorldSectorRegisterPlugin(
        0, 0x1F, 0, 0, 0);
    registrations[count++] = RpWorldSectorRegisterPluginStream(
        0x1F, _rpReadSectRights, _rpWriteSectRights,
        _rpSizeSectRights);
    registrations[count++] = RpMaterialRegisterPlugin(
        0, 0x1F, 0, 0, 0);
    registrations[count++] = RpMaterialRegisterPluginStream(
        0x1F, _rpReadMaterialRights, _rpWriteMaterialRights,
        _rpSizeMaterialRights);

    for (i = 0; i < count; i++) {
        status |= registrations[i];
    }
    if (status < 0 || !_rpWorldPipeAttach()) {
        return 0;
    }
    return 1;
}

RpWorld            *
RpWorldAddCamera(RpWorld * world, RwCamera * camera)
{
    RpWorldCameraExt *cameraExt = WorldCameraExtension(camera);
    RwFrame *frame = (RwFrame*)camera->object.object.parent;
    if (frame != 0) {
        RwFrameUpdateObjects(frame);
    }
    cameraExt->world = world;
    return world;
}

RpWorld            *
RpWorldRemoveCamera(RpWorld * world, RwCamera * camera)
{
    RpWorldCameraExt *cameraExt = WorldCameraExtension(camera);
    if (cameraExt->world != 0) {
        cameraExt->world = 0;
        cameraExt->position = 0;
        return world;
    }
    return 0;
}

RpWorld            *
RwCameraGetWorld(const RwCamera * camera)
{
    return WorldCameraExtension(camera)->world;
}



RpWorld            *
RpWorldAddAtomic(RpWorld * world, RpAtomic * atomic)
{
    RpWorldAtomicExt *atomicExt = WorldAtomicExtension(atomic);
    RwFrame *frame = (RwFrame*)atomic->object.parent;
    if (frame != 0) {
        RwFrameUpdateObjects(frame);
    }
    atomicExt->world = world;
    return world;
}

RpWorld            *
RpWorldRemoveAtomic(RpWorld * world, RpAtomic * atomic)
{
    RpWorldAtomicExt *atomicExt = WorldAtomicExtension(atomic);
    if (atomic->repEntry != 0) {
        RwResourcesFreeResEntry(atomic->repEntry);
    }
    AtomicDestroyTies(atomic);
    atomicExt->world = 0;
    return world;
}

RpWorld            *
RpAtomicGetWorld(const RpAtomic * atomic)
{
    return WorldAtomicExtension(atomic)->world;
}

RpWorld            *
RpWorldAddClump(RpWorld * world, RpClump * object)
{
    RpWorldClumpExt *extension = WorldClumpExtension(object);
    RwFrame *frame = (RwFrame*)object->object.parent;
    rwLinkListAddLLLink(&world->clumpList, &object->inWorldLink);
    ++world->numClumpsInWorld;
    extension->world = world;
    RpClumpForAllAtomics(object, WorldAddClumpAtomic, world);
    RpClumpForAllLights(object, WorldAddClumpLight, world);
    RpClumpForAllCameras(object, WorldAddClumpCamera, world);

    if (frame != 0) {
        RwMatrixOptimize(&frame->modelling, 0);
        RwFrameUpdateObjects(frame);
    }
    extension->clumpsInFrustumID = WorldObjectGlobals()->clumpsInFrustumID;
    return world;
}

RpWorld            *
RpWorldRemoveClump(RpWorld * world, RpClump * object)
{
    RpWorldClumpExt *extension = WorldClumpExtension(object);
    RpWorld *attachedWorld = extension->world;

    if (attachedWorld != 0) {
        --attachedWorld->numClumpsInWorld;
        if (attachedWorld->currentClumpLink == &object->inWorldLink) {
            attachedWorld->currentClumpLink = object->inWorldLink.next;
        }
        rwLinkListRemoveLLLink(&object->inWorldLink);
        RpClumpForAllAtomics(object, WorldRemoveClumpAtomic, world);
        RpClumpForAllLights(object, WorldRemoveClumpLight, world);
        RpClumpForAllCameras(object, WorldRemoveClumpCamera, world);
        extension->world = 0;
        return world;
    }
    WorldObjectSetError(4);
    return 0;
}

RpWorld            *
RpClumpGetWorld(const RpClump * clump)
{
    return WorldClumpExtension(clump)->world;
}



RpWorld            *
RpWorldAddLight(RpWorld * world, RpLight * light)
{
    RpWorldLightExt *lightExt = WorldLightExtension(light);
    lightExt->world = world;
    if (RpLightGetType(light) < 0x80) {
        rwLinkListAddLLLink(&world->directionalLightList, &light->inWorld);
    } else {
        RwFrame *frame = RpLightGetFrame(light);
        if (frame != 0) {
            RwFrameUpdateObjects(frame);
        }
        rwLinkListAddLLLink(&world->lightList, &light->inWorld);
    }
    return world;
}

RpWorld            *
RpWorldRemoveLight(RpWorld * world, RpLight * light)
{
    RpWorldLightExt *lightExt = WorldLightExtension(light);
    lightExt->world = 0;
    LightDestroyTies(light);
    rwLinkListRemoveLLLink(&light->inWorld);
    return world;
}

RpWorld            *
RpLightGetWorld(const RpLight * light)
{
    return WorldLightExtension(light)->world;
}
