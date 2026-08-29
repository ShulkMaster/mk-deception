#include "rw/rwengine.h"
#include "runtime/cstring.h"
#include "rw/rplight.h"
#include "rw/native_internal.h"
#include "rw/rpworld_types.h"
#include "rw/rwerror.h"
#include "rw/rwcamera_internal.h"
#include "rw/rwfreelist.h"
#include "rw/rwframe.h"
#include "rw/rwplcore.h"
#include "rw/rwstream.h"

static void WorldObjectSetError(int code)
{
    RwError error;
    error.pluginID = 2;
    error.errorCode = _rwerror(code);
    RwErrorSet(&error);
}

static void WorldObjectSetErrorWithValue(int code, int value)
{
    RwError error;
    error.pluginID = 2;
    error.errorCode = _rwerror(code, value);
    RwErrorSet(&error);
}

static float WorldObjectCoordinate(const RwV3d* vector, int axis)
{
    return *(const float*)((const unsigned char*)&vector->x + axis);
}

RwStream* _rpReadAtomicRights(RwStream*, int, void*, int, int);
RwStream* _rpWriteAtomicRights(RwStream*, int, const void*, int,
                               int);
int _rpSizeAtomicRights(const void*, int, int);
RwStream* _rpReadWorldRights(RwStream*, int, void*, int, int);
RwStream* _rpWriteWorldRights(RwStream*, int, const void*, int,
                              int);
int _rpSizeWorldRights(const void*, int, int);
RwStream* _rpReadSectRights(RwStream*, int, void*, int, int);
RwStream* _rpWriteSectRights(RwStream*, int, const void*, int,
                             int);
int _rpSizeSectRights(const void*, int, int);
RwStream* _rpReadMaterialRights(RwStream*, int);
RwStream* _rpWriteMaterialRights(RwStream*, int, const RpMaterial*);
int _rpSizeMaterialRights(const RpMaterial*);
typedef struct RpWorldCameraExt {
    RpWorldSector **frustumSectors;
    int space;
    int position;
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
    unsigned int clumpsInFrustumID;
} RpWorldClumpExt;

typedef struct RpWorldLightExt {
    RpWorld *world;
    RwObjectHasFrameSyncFunction oldSync;
} RpWorldLightExt;

typedef struct rpWorldObjGlobals {
    RwFreeList *tieFreeList;
    RwFreeList *lightTieFreeList;
    unsigned int clumpsInFrustumID;
} rpWorldObjGlobals;

static RpWorld     *WorldSyncCamera(RpWorld * world, RwCamera * camera);

static int      cameraExtOffset = 0;
static int      atomicExtOffset = 0;
static int      clumpExtOffset = 0;
static int      lightExtOffset = 0;
static RwModuleInfo worldObjModule;

static RpWorldCameraExt* WorldCameraExtension(const void* camera)
{
    return (RpWorldCameraExt*)((unsigned char*)camera + cameraExtOffset);
}

static RpWorldAtomicExt* WorldAtomicExtension(const void* atomic)
{
    return (RpWorldAtomicExt*)((unsigned char*)atomic + atomicExtOffset);
}

static RpWorldClumpExt* WorldClumpExtension(const void* clump)
{
    return (RpWorldClumpExt*)((unsigned char*)clump + clumpExtOffset);
}

static RpWorldLightExt* WorldLightExtension(const void* light)
{
    return (RpWorldLightExt*)((unsigned char*)light + lightExtOffset);
}

static rpWorldObjGlobals* WorldObjectGlobals(void)
{
    return (rpWorldObjGlobals*)((unsigned char*)RwEngineInstance +
                                worldObjModule.globalsOffset);
}



static int _rpTieFreeListBlockSize = 0x100;
static int _rpTieFreeListPreallocBlocks = 1;
static RwFreeList _rpTieFreeList;

static int _rpLightTieFreeListBlockSize = 0x20;
static int _rpLightTieFreeListPreallocBlocks = 1;
static RwFreeList _rpLightTieFreeList;

static void        *
WorldObjectOpen(void *instance, int offset,
                int size )
{
    worldObjModule.globalsOffset = offset;
    ((rpWorldObjGlobals*)((unsigned char*)RwEngineInstance +
                          worldObjModule.globalsOffset))->tieFreeList =
        RwFreeListCreateAndPreallocateSpace(sizeof(RpTie), _rpTieFreeListBlockSize,
                                             sizeof(unsigned int),
                                             _rpTieFreeListPreallocBlocks,
                                             &_rpTieFreeList,
                                             0x00040000 |
                                                 0x507);
    if (((rpWorldObjGlobals*)((unsigned char*)RwEngineInstance +
                              worldObjModule.globalsOffset))->tieFreeList == 0) {
        return 0;
    }

    ((rpWorldObjGlobals*)((unsigned char*)RwEngineInstance +
                          worldObjModule.globalsOffset))->lightTieFreeList =
        RwFreeListCreateAndPreallocateSpace(
            sizeof(RpLightTie), _rpLightTieFreeListBlockSize,
            sizeof(unsigned int), _rpLightTieFreeListPreallocBlocks,
            &_rpLightTieFreeList, 0x00040000 | 0x507);
    if (((rpWorldObjGlobals*)((unsigned char*)RwEngineInstance +
                              worldObjModule.globalsOffset))->lightTieFreeList == 0) {
        RwFreeListDestroy(((rpWorldObjGlobals*)((unsigned char*)RwEngineInstance +
                                                worldObjModule.globalsOffset))->tieFreeList);
        ((rpWorldObjGlobals*)((unsigned char*)RwEngineInstance +
                              worldObjModule.globalsOffset))->tieFreeList = 0;
        return 0;
    }

    RwEngineInstance->renderFrame = 1;
    ((rpWorldObjGlobals*)((unsigned char*)RwEngineInstance +
                          worldObjModule.globalsOffset))->clumpsInFrustumID = 0;
    worldObjModule.numInstances++;
    return instance;
}

static void        *
WorldObjectClose(void *instance,
                 int offset ,
                 int size )
{
    if (((rpWorldObjGlobals*)((unsigned char*)RwEngineInstance +
                              worldObjModule.globalsOffset))->lightTieFreeList != 0) {
        RwFreeListDestroy(((rpWorldObjGlobals*)((unsigned char*)RwEngineInstance +
                                                worldObjModule.globalsOffset))->lightTieFreeList);
        ((rpWorldObjGlobals*)((unsigned char*)RwEngineInstance +
                              worldObjModule.globalsOffset))->lightTieFreeList = 0;
    }
    if (((rpWorldObjGlobals*)((unsigned char*)RwEngineInstance +
                              worldObjModule.globalsOffset))->tieFreeList != 0) {
        RwFreeListDestroy(((rpWorldObjGlobals*)((unsigned char*)RwEngineInstance +
                                                worldObjModule.globalsOffset))->tieFreeList);
        ((rpWorldObjGlobals*)((unsigned char*)RwEngineInstance +
                              worldObjModule.globalsOffset))->tieFreeList = 0;
    }
    worldObjModule.numInstances--;
    return instance;
}



static              int
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

static              int
SectorsInFrustumAddSpace(RpWorldCameraExt * cameraExt, int nNum)
{
    RpWorldSector     **newFrustumSectors;
    int memSize =
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
    RpWorldCameraExt   *cameraExt =
        (RpWorldCameraExt*)((unsigned char*)camera + cameraExtOffset);
    RwEngineInstance->curWorld = cameraExt->world;
    RwEngineInstance->renderFrame++;
    return cameraExt->oldBeginUpdate(camera);
}

static RwCamera    *
WorldCameraEndUpdate(RwCamera * camera)
{
    RpWorldCameraExt   *cameraExt =
        (RpWorldCameraExt*)((unsigned char*)camera + cameraExtOffset);
    RwEngineInstance->curWorld = 0;
    return cameraExt->oldEndUpdate(camera);
}

static RwObjectHasFrame *
WorldCameraSync(RwObjectHasFrame * object)
{
    RpWorldCameraExt   *cameraExt =
        (RpWorldCameraExt*)((unsigned char*)object + cameraExtOffset);
    RpWorld *world;

    if (cameraExt->oldSync(object) == 0) {
        return 0;
    }
    world = cameraExt->world;
    if (world != 0) {
        RwStandardFunc hintRenderFrontToBack =
            RWENGINESTANDARD(RwStandardCall, rwSTANDARDHINTRENDERF2B);
        WorldSyncCamera(world, (RwCamera *)object);
        hintRenderFrontToBack(0, 0,
                              world->renderOrder == rpWORLDRENDERFRONT2BACK);
    }
    return object;
}

static void        *
WorldInitCameraExt(void *object,
                   int offsetInObject ,
                   int sizeInObject )
{
    RpWorldCameraExt *extension =
        (RpWorldCameraExt*)((unsigned char*)object + cameraExtOffset);
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
                   int offsetInObject ,
                   int sizeInObject )
{
    RpWorldCameraExt *destinationExt =
        (RpWorldCameraExt*)((unsigned char*)dstObject + cameraExtOffset);
    const RpWorldCameraExt *sourceExt =
        (const RpWorldCameraExt*)((const unsigned char*)srcObject + cameraExtOffset);

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
                     int offsetInObject ,
                     int sizeInObject )
{
    RpWorldCameraExt   *cameraExt =
        (RpWorldCameraExt*)((unsigned char*)object + cameraExtOffset);
    RwCamera           *camera = (RwCamera *) object;

    SectorsInFrustumDeinitialise(cameraExt);

    camera->beginUpdate = cameraExt->oldBeginUpdate;
    camera->endUpdate = cameraExt->oldEndUpdate;
    camera->object.sync = cameraExt->oldSync;
    return object;
}



int
_rpLightTieDestroy(RpLightTie * tie)
{
    RwLLLink *previous;

    tie->worldSectorInLight.prev->next = tie->worldSectorInLight.next;
    previous = tie->worldSectorInLight.prev;
    tie->worldSectorInLight.next->prev = previous;
    tie->lightInWorldSector.prev->next = tie->lightInWorldSector.next;
    previous = tie->lightInWorldSector.prev;
    tie->lightInWorldSector.next->prev = previous;
    RwEngineInstance->fpFreeListFree(
        *(RwFreeList**)((unsigned char*)RwEngineInstance +
                        worldObjModule.globalsOffset + sizeof(void*)),
        tie);
    return 1;
}

int
_rpTieDestroy(RpTie * tie)
{
    RwLLLink *previous;

    if (tie->atomic != 0 && tie->worldSector != 0) {
        tie->lWorldSectorInAtomic.prev->next = tie->lWorldSectorInAtomic.next;
        previous = tie->lWorldSectorInAtomic.prev;
        tie->lWorldSectorInAtomic.next->prev = previous;
        tie->lAtomicInWorldSector.prev->next = tie->lAtomicInWorldSector.next;
        previous = tie->lAtomicInWorldSector.prev;
        tie->lAtomicInWorldSector.next->prev = previous;
        RwEngineInstance->fpFreeListFree(
            *(RwFreeList**)((unsigned char*)RwEngineInstance +
                            worldObjModule.globalsOffset),
            tie);
    }
    return 1;
}

static void
AtomicDestroyTies(RpAtomic * atomic)
{
    RwLLLink *end = &atomic->worldSectorsInAtomic.link;
    RwLLLink *link = atomic->worldSectorsInAtomic.link.next;

    while (link != end) {
        RpTie *tie = (RpTie *)((unsigned char *)link - 12);
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
    int stackDepth = 0;

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
            tie->atomic = atomic;
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
    RpWorldAtomicExt   *atomicExt =
        (RpWorldAtomicExt*)((unsigned char*)type + atomicExtOffset);
    RpWorld *world;

    if (atomicExt->oldSync(type) != 0) {
        world = atomicExt->world;
        if (world != 0) {
            AtomicDestroyTies(atomic);
            WorldAttachAtomicSphere(world, atomic);
        }
        return type;
    }
    return 0;
}

static void        *
WorldInitAtomicExt(void *object,
                   int offset ,
                   int size )
{
    RpWorldAtomicExt   *atomicExt =
        (RpWorldAtomicExt*)((unsigned char*)object + atomicExtOffset);
    RpAtomic           *atomic = (RpAtomic *) object;

    atomicExt->world = 0;
    atomic->renderFrame = RwEngineInstance->renderFrame - 1;
    atomicExt->oldSync = atomic->sync;
    atomic->sync = WorldAtomicSync;
    return object;
}

static void        *
WorldCopyAtomicExt(void *dstObject,
                   const void * srcObject ,
                   int offset ,
                   int size )
{
    return dstObject;
}

static void        *
WorldDeInitAtomicExt(void *object,
                     int offset ,
                     int size )
{
    RpWorldAtomicExt   *atomicExt =
        (RpWorldAtomicExt*)((unsigned char*)object + atomicExtOffset);
    RpAtomic           *atomic = (RpAtomic *) object;

    AtomicDestroyTies(atomic);

    atomic->sync = atomicExt->oldSync;
    return object;
}

static void        *
WorldInitClumpExt(void *object,
                  int offset ,
                  int size )
{
    RpWorldClumpExt *clumpExt =
        (RpWorldClumpExt *)((unsigned char *)object + clumpExtOffset);

    clumpExt->world = 0;
    clumpExt->clumpsInFrustumID =
        ((rpWorldObjGlobals *)((unsigned char *)RwEngineInstance +
                               worldObjModule.globalsOffset))->clumpsInFrustumID;
    return object;
}

static void        *
WorldCopyClumpExt(void *dstObject, const void *srcObject,
                  int offset ,
                  int size )
{
    const RpWorldClumpExt *srcClumpExt =
        (const RpWorldClumpExt*)((const unsigned char*)srcObject + clumpExtOffset);

    if (srcClumpExt->world != 0) {
        RpWorldAddClump(srcClumpExt->world, (RpClump *) dstObject);
    }
    return dstObject;
}

static void        *
WorldDeInitClumpExt(void *object,
                    int offset ,
                    int size )
{
    return object;
}

static void
LightDestroyTies(RpLight * light)
{
    RwLLLink *link = light->worldSectorsInLight.link.next;
    RwLLLink *end = &light->worldSectorsInLight.link;

    while (link != end) {
        RpLightTie *tie = (RpLightTie *)((unsigned char *)link - 12);
        link = link->next;
        _rpLightTieDestroy(tie);
    }
}

static RwObjectHasFrame *
WorldLightSync(RwObjectHasFrame * object)
{
    RpLight *light = (RpLight *)object;
    RpWorldLightExt *lightExt =
        (RpWorldLightExt*)((unsigned char*)light + lightExtOffset);
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
        float radius = light->radius;
        int stackDepth = 0;

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
                    tie->worldSector = (RpWorldSector *)sector;
                    tie->light = light;
                    rwLinkListAddLLLink(&((RpWorldSector *)sector)->lightsInWorldSector,
                                        &tie->lightInWorldSector);
                    rwLinkListAddLLLink(&light->worldSectorsInLight,
                                        &tie->worldSectorInLight);
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
                  int offset ,
                  int size )
{
    RpLight            *light = (RpLight *) object;
    RpWorldLightExt    *lightExt =
        (RpWorldLightExt*)((unsigned char*)object + lightExtOffset);

    lightExt->world = 0;
    lightExt->oldSync = light->object.sync;
    light->object.sync = WorldLightSync;
    return object;
}

static void        *
WorldCopyLightExt(void *dstObject,
                  const void *srcObject,
                  int offset ,
                  int size )
{
    const RpWorldLightExt *srcLightExt =
        (const RpWorldLightExt*)((const unsigned char*)srcObject + lightExtOffset);

    if (srcLightExt->world != 0) {
        RpWorldAddLight(srcLightExt->world, (RpLight *) dstObject);
    }
    return dstObject;
}

static void        *
WorldDeInitLightExt(void *object,
                    int offset ,
                    int size )
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
    int backToFront = world->renderOrder == rpWORLDRENDERBACK2FRONT;
    int stackDepth = 0;
    int count = 0;

    do {
        if (sector->type < 0) {
            RpWorldSector *worldSector = (RpWorldSector *)sector;
            const RwV3d *corners = (const RwV3d *)&worldSector->boundingBox;
            int outside = 0;
            int i;

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
                int viewpointHigher =
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
    RpWorldAtomicExt *atomicExt =
        (RpWorldAtomicExt*)((unsigned char*)atomic + atomicExtOffset);
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
    RpWorldLightExt *lightExt =
        (RpWorldLightExt*)((unsigned char*)light + lightExtOffset);
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
    RpWorldCameraExt *cameraExt =
        (RpWorldCameraExt*)((unsigned char*)camera + cameraExtOffset);
    if (cameraExt != 0 && cameraExt->world == world) {
        RpWorldRemoveCamera(world, camera);
    }
    return camera;
}


static RwStream    *
writeGeometryMesh(RwStream * output,
                  int binaryLength ,
                  const void *pluginData,
                  int offsetInObject ,
                  int sizeInObject )
{
    const RpGeometry *geometry = pluginData;
    return _rpMeshWrite(geometry->meshHeader, geometry, output,
                        &geometry->matList);
}

static RwStream    *
readGeometryMesh(RwStream * stream,
                 int binaryLength ,
                 void *object,
                 int offsetInObject ,
                 int sizeInObject )
{
    RpGeometry         *geometry = (RpGeometry *) object;
    geometry->meshHeader = _rpMeshRead(stream, geometry, &geometry->matList);
    if (geometry->meshHeader != 0) {
        return stream;
    }
    return 0;
}

static int
sizeGeometryMesh(const void *pluginData,
                 int offsetInObject ,
                 int sizeInObject )
{
    const RpGeometry *geometry = pluginData;
    return _rpMeshSize(geometry->meshHeader, pluginData);
}

static RwStream *
writeGeometryNative(RwStream *output,
                    int binaryLength ,
                    const void *pluginData,
                    int offsetInObject ,
                    int sizeInObject )
{
    const RpGeometry *geometry = pluginData;
    return _rpGeometryNativeWrite(output, geometry);
}

static RwStream *
readGeometryNative(RwStream *stream,
                   int binaryLength ,
                   void *object,
                   int offsetInObject ,
                   int sizeInObject )
{
    RpGeometry  *geometry = (RpGeometry *)object;
    if (_rpGeometryNativeRead(stream, geometry) != 0) {
        return stream;
    }
    return 0;
}

static int
sizeGeometryNative(const void *pluginData,
                   int offsetInObject ,
                   int sizeInObject )
{
    const RpGeometry *geometry = pluginData;
    return _rpGeometryNativeSize(geometry);
}

static RwStream *
writeWorldSectorNative(RwStream *output,
                       int binaryLength ,
                       const void *pluginData,
                       int offsetInObject ,
                       int sizeInObject )
{
    const RpWorldSector *sector = pluginData;
    return _rpWorldSectorNativeWrite(output, sector);
}

static RwStream *
readWorldSectorNative(RwStream *stream,
                      int binaryLength ,
                      void *object,
                      int offsetInObject ,
                      int sizeInObject )
{
    RpWorldSector   *sector = (RpWorldSector *)object;
    if (_rpWorldSectorNativeRead(stream, sector) != 0) {
        return stream;
    }
    return 0;
}

static int
sizeWorldSectorNative(const void *pluginData,
                      int offsetInObject ,
                      int sizeInObject )
{
    const RpWorldSector *sector = pluginData;
    return _rpWorldSectorNativeSize(sector);
}

static RwStream    *
writeSectorMesh(RwStream * output,
                int binaryLength ,
                const void *pluginData,
                int offsetInObject ,
                int sizeInObject )
{
    const RpWorldSector *sector = pluginData;
    const RpWorld *world = RpWorldSectorGetWorld(sector);
    return _rpMeshWrite(sector->mesh, world, output, &world->matList);
}

static RwStream    *
readSectorMesh(RwStream * input,
               int binaryLength ,
               void *pluginData,
               int offsetInObject ,
               int sizeInObject )
{
    RpWorldSector *sector = pluginData;
    const RpWorld *world = RpWorldSectorGetWorld(sector);
    sector->mesh = _rpMeshRead(input, world, &world->matList);
    if (sector->mesh != 0) {
        return input;
    }
    return 0;
}

static int
sizeSectorMesh(const void *object,
               int offsetInObject ,
               int sizeInObject )
{
    const RpWorldSector *sector = (const RpWorldSector *)object;
    return _rpMeshSize(sector->mesh, RpWorldSectorGetWorld(sector));
}

int
_rpWorldObjRegisterExtensions(void)
{
    int registrations[21];
    int status = 0;
    unsigned int count = 0;
    unsigned int i;

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
    RpWorldCameraExt *cameraExt =
        (RpWorldCameraExt*)((unsigned char*)camera + cameraExtOffset);
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
    RpWorldCameraExt *cameraExt =
        (RpWorldCameraExt*)((unsigned char*)camera + cameraExtOffset);
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
    RpWorldCameraExt* cameraExt =
        (RpWorldCameraExt*)((unsigned char*)camera + cameraExtOffset);
    return cameraExt->world;
}



RpWorld            *
RpWorldAddAtomic(RpWorld * world, RpAtomic * atomic)
{
    RpWorldAtomicExt *atomicExt =
        (RpWorldAtomicExt*)((unsigned char*)atomic + atomicExtOffset);
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
    RpWorldAtomicExt *atomicExt =
        (RpWorldAtomicExt*)((unsigned char*)atomic + atomicExtOffset);
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
    RpWorldAtomicExt* atomicExt =
        (RpWorldAtomicExt*)((unsigned char*)atomic + atomicExtOffset);
    return atomicExt->world;
}

RpWorld            *
RpWorldAddClump(RpWorld * world, RpClump * object)
{
    RwFrame *frame = (RwFrame*)object->object.parent;
    RpWorldClumpExt *extension =
        (RpWorldClumpExt*)((unsigned char*)object + clumpExtOffset);
    RwLLLink *link;

    object->inWorldLink.next = world->clumpList.link.next;
    object->inWorldLink.prev = &world->clumpList.link;
    world->clumpList.link.next->prev = &object->inWorldLink;
    link = &object->inWorldLink;
    world->clumpList.link.next = link;
    ++world->numClumpsInWorld;
    extension->world = world;
    RpClumpForAllAtomics(object, WorldAddClumpAtomic, world);
    RpClumpForAllLights(object, WorldAddClumpLight, world);
    RpClumpForAllCameras(object, WorldAddClumpCamera, world);

    if (frame != 0) {
        RwMatrixOptimize(&frame->modelling, 0);
        RwFrameUpdateObjects(frame);
    }
    extension->clumpsInFrustumID =
        ((rpWorldObjGlobals*)((unsigned char*)RwEngineInstance +
                             worldObjModule.globalsOffset))->clumpsInFrustumID;
    return world;
}

RpWorld            *
RpWorldRemoveClump(RpWorld * world, RpClump * object)
{
    RpWorldClumpExt *extension =
        (RpWorldClumpExt*)((unsigned char*)object + clumpExtOffset);
    RwLLLink *previous;

    if (extension->world != 0) {
        --extension->world->numClumpsInWorld;
        if (&object->inWorldLink == extension->world->currentClumpLink) {
            extension->world->currentClumpLink = object->inWorldLink.next;
        }
        object->inWorldLink.prev->next = object->inWorldLink.next;
        previous = object->inWorldLink.prev;
        object->inWorldLink.next->prev = previous;
        RpClumpForAllAtomics(object, WorldRemoveClumpAtomic, world);
        RpClumpForAllLights(object, WorldRemoveClumpLight, world);
        RpClumpForAllCameras(object, WorldRemoveClumpCamera, world);
        extension->world = 0;
        return world;
    }
    {
        RwError error;
        error.pluginID = 2;
        error.errorCode = _rwerror(4);
        RwErrorSet(&error);
    }
    return 0;
}

RpWorld            *
RpClumpGetWorld(const RpClump * clump)
{
    RpWorldClumpExt* clumpExt =
        (RpWorldClumpExt*)((unsigned char*)clump + clumpExtOffset);
    return clumpExt->world;
}



RpWorld            *
RpWorldAddLight(RpWorld * world, RpLight * light)
{
    RpWorldLightExt *lightExt;
    RwFrame *frame;
    RwLLLink *directionalLink;
    RwLLLink *localLink;

    lightExt = (RpWorldLightExt*)((unsigned char*)light + lightExtOffset);
    lightExt->world = world;
    if (light->object.object.subType < 0x80) {
        light->inWorld.next = world->directionalLightList.link.next;
        light->inWorld.prev = &world->directionalLightList.link;
        world->directionalLightList.link.next->prev = &light->inWorld;
        directionalLink = &light->inWorld;
        world->directionalLightList.link.next = directionalLink;
    } else {
        frame = (RwFrame*)light->object.object.parent;
        if (frame != 0) {
            RwFrameUpdateObjects(frame);
        }
        light->inWorld.next = world->lightList.link.next;
        light->inWorld.prev = &world->lightList.link;
        world->lightList.link.next->prev = &light->inWorld;
        localLink = &light->inWorld;
        world->lightList.link.next = localLink;
    }
    return world;
}

RpWorld            *
RpWorldRemoveLight(RpWorld * world, RpLight * light)
{
    RpWorldLightExt *lightExt =
        (RpWorldLightExt*)((unsigned char*)light + lightExtOffset);
    lightExt->world = 0;
    LightDestroyTies(light);
    rwLinkListRemoveLLLink(&light->inWorld);
    return world;
}

RpWorld            *
RpLightGetWorld(const RpLight * light)
{
    RpWorldLightExt* lightExt =
        (RpWorldLightExt*)((unsigned char*)light + lightExtOffset);
    return lightExt->world;
}
