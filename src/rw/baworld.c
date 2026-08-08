#include "libmkparticle/rw_engine.h"
#include "rw/rpworld_types.h"
#include "rw/rplight.h"
#include "rw/rwfreelist.h"
#include "rw/rwplcore.h"
#include "rw/rxpipeline.h"

typedef struct RpWorldListEntry {
    RpWorld *world;
    RwInt32 memorySize;
    RwLLLink link;
} RpWorldListEntry;

typedef struct RpWorldGlobals {
    RwFreeList *worldListFreeList;
    RwLinkList worldList;
} RpWorldGlobals;

extern RwPluginRegistry sectorTKList;
extern RwInt32 _rxPipelineGlobalsOffset;
extern RwBool _rpWorldPipelineOpen(void);
extern void _rpWorldPipelineClose(void);
extern RpMeshHeader *_rpMeshOptimise(RpBuildMesh *, RwUInt32);
extern void RwResourcesFreeResEntry(RwResEntry *);
extern void _rpTieDestroy(RwLLLink *);
extern void _rpLightTieDestroy(RwLLLink *);
extern RwBool _rpWorldObjRegisterExtensions(void);
extern RwBool _rpClumpRegisterExtensions(void);
extern RwBool _rxWorldDevicePluginAttach(void);
extern void *_rpMaterialOpen(void *, RwInt32, RwInt32);
extern void *_rpMaterialClose(void *, RwInt32, RwInt32);
extern void *_rpGeometryOpen(void *, RwInt32, RwInt32);
extern void *_rpGeometryClose(void *, RwInt32, RwInt32);
extern void *_rpClumpOpen(void *, RwInt32, RwInt32);
extern void *_rpClumpClose(void *, RwInt32, RwInt32);
extern void *_rpLightOpen(void *, RwInt32, RwInt32);
extern void *_rpLightClose(void *, RwInt32, RwInt32);
extern void *_rpSectorOpen(void *, RwInt32, RwInt32);
extern void *_rpSectorClose(void *, RwInt32, RwInt32);
extern void *_rpBinaryWorldOpen(void *, RwInt32, RwInt32);
extern void *_rpBinaryWorldClose(void *, RwInt32, RwInt32);
extern void RwErrorSet(const RwError *);
extern RwInt32 _rwerror(RwInt32, ...);
extern void *memset(void *, RwInt32, RwUInt32);

static RwPluginRegistry worldTKList = {
    sizeof(RpWorld), sizeof(RpWorld), 0, 0, NULL, NULL};
static RwFreeList _rpWorldListFreeList;
static RwModuleInfo worldModule;

#define WORLDGLOBALS                                                           \
    ((RpWorldGlobals *)((RwUInt8 *)RwEngineInstance +                          \
                        worldModule.globalsOffset))

static RpWorldSector *WorldFindSector(RpWorldSector *sector, void *data) {
    RpWorldSector **find = data;
    if (find[0] == sector) {
        find[1] = (RpWorldSector *)TRUE;
        return NULL;
    }
    return sector;
}

static RpWorld *WorldBuildMeshAtomicSector(RpWorld *world, RpBuildMesh *build,
                                           RpWorldSector *sector,
                                           RpMaterial **materials) {
    RwTexture **textures = NULL;
    RwRaster **rasters = NULL;
    RxPipeline **pipelines = NULL;
    RwUInt16 numTextures = 0, numRasters = 0, numPipelines = 0;
    RwInt32 i;

    if (world->matList.numMaterials != 0) {
        textures = RwEngineInstance->fpMalloc(world->matList.numMaterials * 4,
                                              0x10006);
        rasters = RwEngineInstance->fpMalloc(world->matList.numMaterials * 4,
                                             0x10507);
        pipelines = RwEngineInstance->fpMalloc(world->matList.numMaterials * 4,
                                               0x10507);
        for (i = 0; i < sector->numPolygons; i++) {
            RpPolygon *polygon = &sector->polygons[i];
            RpMaterial *material = materials[polygon->matIndex];
            RwTexture *texture = material->texture;
            RwRaster *raster = texture != NULL ? texture->raster : NULL;
            RxPipeline *pipeline = material->pipeline;
            RwUInt16 ti, ri, pi;
            for (ti = 0; ti < numTextures && textures[ti] != texture; ti++) {
            }
            if (ti == numTextures)
                textures[numTextures++] = texture;
            for (ri = 0; ri < numRasters && rasters[ri] != raster; ri++) {
            }
            if (ri == numRasters)
                rasters[numRasters++] = raster;
            for (pi = 0; pi < numPipelines && pipelines[pi] != pipeline; pi++) {
            }
            if (pi == numPipelines)
                pipelines[numPipelines++] = pipeline;
            _rpBuildMeshAddTriangle(
                build, material, polygon->vertIndex[0], polygon->vertIndex[1],
                polygon->vertIndex[2], polygon->matIndex, ti, ri, pi);
        }
        RwEngineInstance->fpFree(textures);
        RwEngineInstance->fpFree(rasters);
        RwEngineInstance->fpFree(pipelines);
    }
    sector->mesh = _rpMeshOptimise(build, (world->flags & 1) != 0);
    if (sector->mesh != NULL)
        return world;
    _rpBuildMeshDestroy(build);
    return NULL;
}

void _rpWorldSectorDeinstanceAll(RpSector *sector) {
    switch (sector->type) {
    case -1: {
        RpWorldSector *atomic = (RpWorldSector *)sector;
        RwLLLink *link, *next;
        if (atomic->repEntry)
            RwResourcesFreeResEntry(atomic->repEntry);
        for (link = atomic->collAtomicsInWorldSector.link.next;
             link != &atomic->collAtomicsInWorldSector.link; link = next) {
            next = link->next;
            _rpTieDestroy(link);
        }
        for (link = atomic->lightsInWorldSector.link.next;
             link != &atomic->lightsInWorldSector.link; link = next) {
            next = link->next;
            _rpLightTieDestroy(link);
        }
        _rwPluginRegistryDeInitObject(&sectorTKList, atomic);
        if (atomic->mesh) {
            _rpMeshDestroy(atomic->mesh);
            atomic->mesh = NULL;
        }
        return;
    }
    case -2:
        return;
    default: {
        RpPlaneSector *plane = (RpPlaneSector *)sector;
        if (plane->leftSubTree)
            _rpWorldSectorDeinstanceAll(plane->leftSubTree);
        if (plane->rightSubTree)
            _rpWorldSectorDeinstanceAll(plane->rightSubTree);
        return;
    }
    }
}

void _rpWorldSectorDestroyRecurse(RpSector *sector) {
    switch (sector->type) {
    case -1: {
        RpWorldSector *atomic = (RpWorldSector *)sector;
        RwInt32 i;
        RwLLLink *link, *next;
        if (atomic->repEntry)
            RwResourcesFreeResEntry(atomic->repEntry);
        for (link = atomic->collAtomicsInWorldSector.link.next;
             link != &atomic->collAtomicsInWorldSector.link; link = next) {
            next = link->next;
            _rpTieDestroy(link);
        }
        for (link = atomic->lightsInWorldSector.link.next;
             link != &atomic->lightsInWorldSector.link; link = next) {
            next = link->next;
            _rpLightTieDestroy(link);
        }
        _rwPluginRegistryDeInitObject(&sectorTKList, atomic);
        if (atomic->vertices) {
            RwEngineInstance->fpFree(atomic->vertices);
            atomic->vertices = NULL;
        }
        if (atomic->normals) {
            RwEngineInstance->fpFree(atomic->normals);
            atomic->normals = NULL;
        }
        if (atomic->preLitLum) {
            RwEngineInstance->fpFree(atomic->preLitLum);
            atomic->preLitLum = NULL;
        }
        if (atomic->polygons) {
            RwEngineInstance->fpFree(atomic->polygons);
            atomic->polygons = NULL;
        }
        for (i = 0; i < 8; i++)
            if (atomic->texCoords[i]) {
                RwEngineInstance->fpFree(atomic->texCoords[i]);
                atomic->texCoords[i] = NULL;
            }
        if (atomic->mesh) {
            _rpMeshDestroy(atomic->mesh);
            atomic->mesh = NULL;
        }
        RwEngineInstance->fpFree(atomic);
        return;
    }
    case -2:
        RwEngineInstance->fpFree(sector);
        return;
    default: {
        RpPlaneSector *plane = (RpPlaneSector *)sector;
        if (plane->leftSubTree)
            _rpWorldSectorDestroyRecurse(plane->leftSubTree);
        plane->leftSubTree = NULL;
        if (plane->rightSubTree)
            _rpWorldSectorDestroyRecurse(plane->rightSubTree);
        plane->rightSubTree = NULL;
        RwEngineInstance->fpFree(plane);
        return;
    }
    }
}

static void *WorldClose(void *instance, RwInt32 offset, RwInt32 size) {
    if (WORLDGLOBALS->worldListFreeList) {
        RwFreeListDestroy(WORLDGLOBALS->worldListFreeList);
        WORLDGLOBALS->worldListFreeList = NULL;
    }
    _rpWorldPipelineClose();
    worldModule.numInstances--;
    return instance;
}

static void *WorldOpen(void *instance, RwInt32 offset, RwInt32 size) {
    worldModule.globalsOffset = offset;
    if (!_rpWorldPipelineOpen())
        return NULL;
    WORLDGLOBALS->worldListFreeList = RwFreeListCreateAndPreallocateSpace(
        0x10, 8, 4, 1, &_rpWorldListFreeList, 0x4000B);
    if (!WORLDGLOBALS->worldListFreeList)
        return NULL;
    rwLinkListInitialize(&WORLDGLOBALS->worldList);
    worldModule.numInstances++;
    return instance;
}

RpWorldSector *_rpSectorDefaultRenderCallBack(RpWorldSector *sector) {
    RxPipeline *pipeline;
    if (!sector->numPolygons)
        return sector;
    pipeline = sector->pipeline;
    if (!pipeline && RwEngineInstance->curWorld)
        pipeline = ((RpWorld *)RwEngineInstance->curWorld)->pipeline;
    if (!pipeline)
        pipeline = *(RxPipeline **)((RwUInt8 *)RwEngineInstance +
                                    _rxPipelineGlobalsOffset + 0x40);
    if (RxPipelineExecute(pipeline, sector, 1) != NULL) {
        return sector;
    }
    return NULL;
}

void _rpWorldRegisterWorld(RpWorld *world, RwInt32 size) {
    RpWorldListEntry *entry = RwEngineInstance->fpFreeListAlloc(
        WORLDGLOBALS->worldListFreeList, 0x40507);
    if (entry) {
        entry->world = world;
        entry->memorySize = size;
        rwLinkListAddLLLink(&WORLDGLOBALS->worldList, &entry->link);
    }
}

void _rpWorldUnregisterWorld(RpWorld *world) {
    RwLLLink *link;
    for (link = WORLDGLOBALS->worldList.link.next;
         link != &WORLDGLOBALS->worldList.link; link = link->next) {
        RpWorldListEntry *entry = (RpWorldListEntry *)((RwUInt8 *)link - 8);
        if (entry->world == world) {
            rwLinkListRemoveLLLink(link);
            RwEngineInstance->fpFreeListFree(WORLDGLOBALS->worldListFreeList,
                                             entry);
            return;
        }
    }
}

RpWorld *RpWorldLock(RpWorld *world) {
    RpSector *stack[64];
    RwInt32 top = 0;
    RpSector *sector = world->rootSector;
    if (!sector)
        return NULL;
    do {
        if (sector->type < 0) {
            RpWorldSector *atomic = (RpWorldSector *)sector;
            if (atomic->mesh) {
                _rpMeshDestroy(atomic->mesh);
                atomic->mesh = NULL;
            }
            sector = stack[top--];
        } else {
            RpPlaneSector *p = (RpPlaneSector *)sector;
            sector = p->leftSubTree;
            stack[++top] = p->rightSubTree;
        }
    } while (top >= 0);
    return world;
}

RpWorld *RpWorldUnlock(RpWorld *world) {
    RpSector *stack[64];
    RwInt32 top = 0;
    RpSector *sector = world->rootSector;
    if (!sector)
        return NULL;
    do {
        if (sector->type < 0) {
            RpWorldSector *atomic = (RpWorldSector *)sector;
            if (!atomic->mesh) {
                RpBuildMesh *mesh = _rpBuildMeshCreate(atomic->numPolygons);
                if (!mesh ||
                    !WorldBuildMeshAtomicSector(world, mesh, atomic,
                                                world->matList.materials +
                                                    atomic->matListWindowBase))
                    return NULL;
            }
            sector = stack[top--];
        } else {
            RpPlaneSector *p = (RpPlaneSector *)sector;
            sector = p->leftSubTree;
            stack[++top] = p->rightSubTree;
        }
    } while (top >= 0);
    return world;
}

RpWorld *RpWorldSectorGetWorld(const RpWorldSector *target) {
    RwLLLink *link;
    for (link = WORLDGLOBALS->worldList.link.next;
         link != &WORLDGLOBALS->worldList.link; link = link->next) {
        RpWorldListEntry *e = (RpWorldListEntry *)((RwUInt8 *)link - 8);
        if (e->world->object.privateFlags & 1) {
            if ((const void *)target >= (const void *)e->world &&
                (const void *)target <
                    (const void *)((RwUInt8 *)e->world + e->memorySize))
                return e->world;
        } else {
            RpWorldSector *find[2];
            find[0] = (RpWorldSector *)target;
            find[1] = NULL;
            RpWorldForAllWorldSectors(e->world, WorldFindSector, find);
            if (find[1])
                return e->world;
        }
    }
    return NULL;
}

RwBool RpWorldDestroy(RpWorld *world) {
    _rpWorldUnregisterWorld(world);
    RpWorldLock(world);
    _rpMaterialListDeinitialize(&world->matList);
    if (world->object.privateFlags & 1) {
        if (world->rootSector) {
            _rpWorldSectorDeinstanceAll(world->rootSector);
        }
        _rwPluginRegistryDeInitObject(&worldTKList, world);
        RwEngineInstance->fpFree(world);
    } else {
        if (world->rootSector) {
            _rpWorldSectorDestroyRecurse(world->rootSector);
        }
        _rwPluginRegistryDeInitObject(&worldTKList, world);
        RwEngineInstance->fpFree(world);
    }
    return TRUE;
}

void RpWorldSetSectorRenderCallBack(RpWorld *world,
                                    RpWorldSectorCallBackRender cb) {
    if (cb == NULL) {
        cb = _rpSectorDefaultRenderCallBack;
    }
    world->renderCallBack = cb;
}

RpWorld *RpWorldCreate(RwBBox *box) {
    RpWorld *world = RwEngineInstance->fpMalloc(sizeof(RpWorld), 0x3000B);
    RpWorldSector *sector;
    if (!world) {
        RwError error;
        error.pluginID = 2;
        error.errorCode = _rwerror(0x80000013, sizeof(RpWorld));
        RwErrorSet(&error);
        return NULL;
    }
    rwObjectInitialize(world, 7, 0);
    _rpMaterialListInitialize(&world->matList);
    world->renderOrder = rpWORLDRENDERBACK2FRONT;
    world->flags = 0;
    sector = RwEngineInstance->fpMalloc(sectorTKList.sizeOfStruct, 0x3000B);
    if (!sector) {
        RwError error;
        error.pluginID = 2;
        error.errorCode = _rwerror(0x80000013, sizeof(RwInt32));
        RwErrorSet(&error);
        RwEngineInstance->fpFree(world);
        return NULL;
    }
    sector->type = -1;
    sector->repEntry = NULL;
    sector->mesh = NULL;
    rwLinkListInitialize(&sector->collAtomicsInWorldSector);
    rwLinkListInitialize(&sector->lightsInWorldSector);
    sector->numVertices = sector->numPolygons = 0;
    sector->vertices = NULL;
    sector->polygons = NULL;
    sector->normals = NULL;
    memset(sector->texCoords, 0, sizeof(sector->texCoords));
    sector->preLitLum = NULL;
    sector->boundingBox = *box;
    sector->tightBoundingBox = *box;
    sector->pipeline = NULL;
    world->rootSector = (RpSector *)sector;
    world->numTexCoordSets = 0;
    world->worldOrigin.x = world->worldOrigin.y = world->worldOrigin.z = 0.0f;
    world->boundingBox = *box;
    rwLinkListInitialize(&world->clumpList);
    world->numClumpsInWorld = 0;
    world->currentClumpLink = &world->clumpList.link;
    rwLinkListInitialize(&world->lightList);
    rwLinkListInitialize(&world->directionalLightList);
    RpWorldSetSectorRenderCallBack(world, NULL);
    world->pipeline = NULL;
    _rpWorldRegisterWorld(world, sizeof(RpWorld));
    _rwPluginRegistryInitObject(&worldTKList, world);
    _rwPluginRegistryInitObject(&sectorTKList, sector);
    if (!RpWorldUnlock(world)) {
        RpWorldDestroy(world);
        return NULL;
    }
    return world;
}

RpWorld *RpWorldForAllLights(RpWorld *world, RpLightCallBack cb, void *data) {
    RwLLLink *link;
    RwLLLink *next;

    link = world->directionalLightList.link.next;
    while (link != &world->directionalLightList.link) {
        next = link->next;
        if (cb((RpLight *)((RwUInt8 *)link - 0x34), data) == NULL) {
            return world;
        }
        link = next;
    }

    link = world->lightList.link.next;
    while (link != &world->lightList.link) {
        next = link->next;
        if (cb((RpLight *)((RwUInt8 *)link - 0x34), data) == NULL) {
            return world;
        }
        link = next;
    }
    return world;
}

RpWorld *RpWorldForAllWorldSectors(RpWorld *world, RpWorldSectorCallBack cb,
                                   void *data) {
    RpSector *stack[64];
    RwInt32 top = 0;
    RpSector *sector = world->rootSector;
    do {
        if (sector->type < 0) {
            if (!cb((RpWorldSector *)sector, data))
                return world;
            sector = stack[top--];
        } else {
            RpPlaneSector *p = (RpPlaneSector *)sector;
            sector = p->leftSubTree;
            stack[++top] = p->rightSubTree;
        }
    } while (top >= 0);
    return world;
}

RwInt32 RpWorldRegisterPlugin(RwInt32 size, RwUInt32 id,
                              RwPluginObjectConstructor c,
                              RwPluginObjectDestructor d,
                              RwPluginObjectCopy cp) {
    return _rwPluginRegistryAddPlugin(&worldTKList, size, id, c, d, cp);
}
RwInt32 RpWorldRegisterPluginStream(RwUInt32 id,
                                    RwPluginDataChunkReadCallBack r,
                                    RwPluginDataChunkWriteCallBack w,
                                    RwPluginDataChunkGetSizeCallBack s) {
    return 0;
}

RwBool RpWorldPluginAttach(void) {
    RwInt32 result =
        RwEngineRegisterPlugin(4, 0x501, _rpMaterialOpen, _rpMaterialClose);
    result |= RwEngineRegisterPlugin(0x30, 0x502, _rpMeshOpen, _rpMeshClose);
    result |=
        RwEngineRegisterPlugin(4, 0x503, _rpGeometryOpen, _rpGeometryClose);
    result |= RwEngineRegisterPlugin(8, 0x504, _rpClumpOpen, _rpClumpClose);
    result |= RwEngineRegisterPlugin(4, 0x505, _rpLightOpen, _rpLightClose);
    result |= RwEngineRegisterPlugin(0, 0x50A, _rpSectorOpen, _rpSectorClose);
    result |= RwEngineRegisterPlugin(0xC, 0x507, WorldOpen, WorldClose);
    result |= RwEngineRegisterPlugin(0, 0x50B, _rpBinaryWorldOpen,
                                     _rpBinaryWorldClose);
    if (result < 0) {
        return FALSE;
    }
    if (!_rpWorldObjRegisterExtensions()) {
        return FALSE;
    }
    if (!_rpClumpRegisterExtensions()) {
        return FALSE;
    }
    if (!_rxWorldDevicePluginAttach()) {
        return FALSE;
    }
    return TRUE;
}
