#include "libmkparticle/rw_engine.h"
#include "rw/rpworld_types.h"
#include "rw/rplight.h"
#include "rw/rwfreelist.h"
#include "rw/rwplcore.h"
#include "rw/rxpipeline.h"

typedef struct RpWorldListEntry {
    RpWorld *world;
    int memorySize;
    RwLLLink link;
} RpWorldListEntry;

typedef struct RpWorldGlobals {
    RwFreeList *worldListFreeList;
    RwLinkList worldList;
} RpWorldGlobals;

typedef struct RpWorldFindSectorData {
    const RpWorldSector *sector;
    int foundSector;
} RpWorldFindSectorData;

extern RwPluginRegistry sectorTKList;
extern RpMeshHeader *_rpMeshOptimise(RpBuildMesh *, unsigned int);
extern void RwResourcesFreeResEntry(RwResEntry *);
extern void RwErrorSet(const RwError *);
extern int _rwerror(int, ...);
extern void *memset(void *, int, unsigned int);

static RwPluginRegistry worldTKList = {
    sizeof(RpWorld), sizeof(RpWorld), 0, 0, 0, 0};
static RwFreeList _rpWorldListFreeList;
static RwModuleInfo worldModule;

static RpWorldGlobals *WorldGlobals(void) {
    return (RpWorldGlobals *)((unsigned char *)RwEngineInstance +
                              worldModule.globalsOffset);
}

static RpWorldSector *WorldFindSector(RpWorldSector *sector, void *data) {
    RpWorldFindSectorData *find = data;
    if (find->sector == sector) {
        find->foundSector = 1;
        return 0;
    }
    return sector;
}

static RpWorld *WorldBuildMeshAtomicSector(RpWorld *world, RpBuildMesh *build,
                                           RpWorldSector *sector,
                                           RpMaterial **materials) {
    RpMeshHeader *mesh;
    int i;
    RwTexture **textures;
    RwRaster **rasters;
    RxPipeline **pipelines;
    unsigned short numTextures = 0;
    unsigned short numRasters = 0;
    unsigned short numPipelines = 0;
    unsigned int numMaterials;

    numMaterials = world->matList.numMaterials;
    if (numMaterials != 0) {
        textures = RwEngineInstance->fpMalloc(numMaterials * 4, 0x10006);
        rasters = RwEngineInstance->fpMalloc(numMaterials * 4, 0x10507);
        pipelines = RwEngineInstance->fpMalloc(numMaterials * 4, 0x10507);
        for (i = 0; i < sector->numTriangles; i++) {
            RpTriangle *triangle;
            RpMaterial *material;
            unsigned short textureIndex;
            unsigned short rasterIndex;
            unsigned short pipelineIndex;
            RxPipeline *pipeline = 0;
            RwTexture *texture = 0;
            RwRaster *raster = 0;

            triangle = &sector->triangles[i];
            material = materials[triangle->matIndex];
            texture = material->texture;
            for (textureIndex = 0; textureIndex < numTextures;
                 textureIndex++) {
                if (textures[textureIndex] == texture)
                    break;
            }
            if (textureIndex == numTextures) {
                textures[textureIndex] = texture;
                numTextures++;
            }
            if (texture != 0)
                raster = texture->raster;
            for (rasterIndex = 0; rasterIndex < numRasters; rasterIndex++) {
                if (rasters[rasterIndex] == raster)
                    break;
            }
            if (rasterIndex == numRasters) {
                rasters[rasterIndex] = raster;
                numRasters++;
            }
            pipeline = material->pipeline;
            for (pipelineIndex = 0; pipelineIndex < numPipelines;
                 pipelineIndex++) {
                if (pipelines[pipelineIndex] == pipeline)
                    break;
            }
            if (pipelineIndex == numPipelines) {
                pipelines[pipelineIndex] = pipeline;
                numPipelines++;
            }
            _rpBuildMeshAddTriangle(build, material, triangle->vertIndex[0],
                                    triangle->vertIndex[1],
                                    triangle->vertIndex[2], triangle->matIndex,
                                    textureIndex, rasterIndex, pipelineIndex);
        }
        RwEngineInstance->fpFree(textures);
        RwEngineInstance->fpFree(rasters);
        RwEngineInstance->fpFree(pipelines);
    }
    if (world->flags & 1)
        mesh = _rpMeshOptimise(build, 1);
    else
        mesh = _rpMeshOptimise(build, 0);
    if (mesh != 0) {
        sector->mesh = mesh;
    } else {
        _rpBuildMeshDestroy(build);
        return 0;
    }
    return world;
}

void _rpWorldSectorDeinstanceAll(RpSector *sector) {
    switch (sector->type) {
    case -1: {
        RpWorldSector *atomic = (RpWorldSector *)sector;
        RwLLLink *cur;
        RwLLLink *end;
        RpTie *tie;
        RpLightTie *lightTie;

        if (atomic->repEntry)
            RwResourcesFreeResEntry(atomic->repEntry);
        cur = atomic->collAtomicsInWorldSector.link.next;
        end = &atomic->collAtomicsInWorldSector.link;
        while (cur != end) {
            tie = (RpTie*)cur;
            cur = cur->next;
            _rpTieDestroy(tie);
        }
        cur = atomic->lightsInWorldSector.link.next;
        end = &atomic->lightsInWorldSector.link;
        while (cur != end) {
            lightTie = (RpLightTie*)cur;
            cur = cur->next;
            _rpLightTieDestroy(lightTie);
        }
        _rwPluginRegistryDeInitObject(&sectorTKList, atomic);
        if (atomic->mesh) {
            _rpMeshDestroy(atomic->mesh);
            atomic->mesh = 0;
        }
        break;
    }
    case -2:
        break;
    default: {
        RpPlaneSector *plane = (RpPlaneSector *)sector;
        if (plane->leftSubTree)
            _rpWorldSectorDeinstanceAll(plane->leftSubTree);
        if (plane->rightSubTree)
            _rpWorldSectorDeinstanceAll(plane->rightSubTree);
        break;
    }
    }
}

void _rpWorldSectorDestroyRecurse(RpSector *sector) {
    switch (sector->type) {
    case -1: {
        RpWorldSector *atomic = (RpWorldSector *)sector;
        RwLLLink *cur;
        RwLLLink *end;
        RpTie *tie;
        RpLightTie *lightTie;
        int i;

        if (atomic->repEntry)
            RwResourcesFreeResEntry(atomic->repEntry);
        cur = atomic->collAtomicsInWorldSector.link.next;
        end = &atomic->collAtomicsInWorldSector.link;
        while (cur != end) {
            tie = (RpTie*)cur;
            cur = cur->next;
            _rpTieDestroy(tie);
        }
        cur = atomic->lightsInWorldSector.link.next;
        end = &atomic->lightsInWorldSector.link;
        while (cur != end) {
            lightTie = (RpLightTie*)cur;
            cur = cur->next;
            _rpLightTieDestroy(lightTie);
        }
        _rwPluginRegistryDeInitObject(&sectorTKList, atomic);
        if (atomic->vertices) {
            RwEngineInstance->fpFree(atomic->vertices);
            atomic->vertices = 0;
        }
        if (atomic->normals) {
            RwEngineInstance->fpFree(atomic->normals);
            atomic->normals = 0;
        }
        if (atomic->preLitLum) {
            RwEngineInstance->fpFree(atomic->preLitLum);
            atomic->preLitLum = 0;
        }
        if (atomic->triangles) {
            RwEngineInstance->fpFree(atomic->triangles);
            atomic->triangles = 0;
        }
        for (i = 0; i < 8; i++)
            if (atomic->texCoords[i]) {
                RwEngineInstance->fpFree(atomic->texCoords[i]);
                atomic->texCoords[i] = 0;
            }
        if (atomic->mesh) {
            _rpMeshDestroy(atomic->mesh);
            atomic->mesh = 0;
        }
        RwEngineInstance->fpFree(atomic);
        break;
    }
    case -2:
        RwEngineInstance->fpFree(sector);
        break;
    default: {
        RpPlaneSector *plane = (RpPlaneSector *)sector;
        if (plane->leftSubTree)
            _rpWorldSectorDestroyRecurse(plane->leftSubTree);
        plane->leftSubTree = 0;
        if (plane->rightSubTree)
            _rpWorldSectorDestroyRecurse(plane->rightSubTree);
        plane->rightSubTree = 0;
        RwEngineInstance->fpFree(plane);
        break;
    }
    }
}

static void *WorldClose(void *instance, int offset, int size) {
    if (((RpWorldGlobals *)((unsigned char *)RwEngineInstance +
                            worldModule.globalsOffset))->worldListFreeList) {
        RwFreeListDestroy(((RpWorldGlobals *)((unsigned char *)RwEngineInstance +
                                               worldModule.globalsOffset))->worldListFreeList);
        ((RpWorldGlobals *)((unsigned char *)RwEngineInstance +
                            worldModule.globalsOffset))->worldListFreeList = 0;
    }
    _rpWorldPipelineClose();
    worldModule.numInstances--;
    return instance;
}

static void *WorldOpen(void *instance, int offset, int size) {
    RwLLLink *sentinel;

    worldModule.globalsOffset = offset;
    if (!_rpWorldPipelineOpen())
        return 0;
    ((RpWorldGlobals *)((unsigned char *)RwEngineInstance +
                        worldModule.globalsOffset))->worldListFreeList =
        RwFreeListCreateAndPreallocateSpace(
        0x10, 8, 4, 1, &_rpWorldListFreeList, 0x4000B);
    if (!((RpWorldGlobals *)((unsigned char *)RwEngineInstance +
                             worldModule.globalsOffset))->worldListFreeList)
        return 0;
    ((RpWorldGlobals *)((unsigned char *)RwEngineInstance +
                        worldModule.globalsOffset))->worldList.link.next =
        &((RpWorldGlobals *)((unsigned char *)RwEngineInstance +
                             worldModule.globalsOffset))->worldList.link;
    sentinel = &((RpWorldGlobals *)((unsigned char *)RwEngineInstance +
                                    worldModule.globalsOffset))->worldList.link;
    ((RpWorldGlobals *)((unsigned char *)RwEngineInstance +
                        worldModule.globalsOffset))->worldList.link.prev = sentinel;
    worldModule.numInstances++;
    return instance;
}

RpWorldSector *_rpSectorDefaultRenderCallBack(RpWorldSector *sector) {
    RxPipeline *pipeline = 0;

    if (sector->numTriangles <= 0)
        return sector;
    if (sector->pipeline) {
        pipeline = sector->pipeline;
    } else {
        if (((RpWorld *)RwEngineInstance->field_0x04)->pipeline) {
            pipeline = ((RpWorld *)RwEngineInstance->field_0x04)->pipeline;
        } else {
            pipeline = *(RxPipeline **)((unsigned char *)RwEngineInstance +
                                        _rxPipelineGlobalsOffset + 0x40);
        }
    }
    if (RxPipelineExecute(pipeline, sector, 1) != 0) {
        return sector;
    }
    return 0;
}

void _rpWorldRegisterWorld(RpWorld *world, int size) {
    RpWorldListEntry *entry = RwEngineInstance->fpFreeListAlloc(
        WorldGlobals()->worldListFreeList, 0x40507);
    if (entry) {
        entry->world = world;
        entry->memorySize = size;
        rwLinkListAddLLLink(&WorldGlobals()->worldList, &entry->link);
    }
}


void _rpWorldUnregisterWorld(RpWorld *world) {
    RwLLLink *link;
    RwLLLink *end;

    link = WorldGlobals()->worldList.link.next;
    end = &WorldGlobals()->worldList.link;
    while (link != end) {
        RpWorldListEntry *entry = (RpWorldListEntry *)((unsigned char *)link - 8);
        if (entry->world == world) {
            rwLinkListRemoveLLLink(&entry->link);
            RwEngineInstance->fpFreeListFree(WorldGlobals()->worldListFreeList,
                                             entry);
            return;
        }
        link = link->next;
    }
}

RpWorld *RpWorldLock(RpWorld *world) {
    RpSector *stack[64];
    int top = 0;
    RpSector *sector = world->rootSector;
    if (!sector)
        return 0;
    do {
        if (sector->type < 0) {
            RpWorldSector *atomic = (RpWorldSector *)sector;
            if (atomic->mesh) {
                _rpMeshDestroy(atomic->mesh);
                atomic->mesh = 0;
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
    RpSector *sector;
    RpSector *stack[64];
    int top = 0;

    sector = world->rootSector;
    if (!sector)
        return 0;
    do {
        if (sector->type < 0) {
            RpWorldSector *atomic = (RpWorldSector *)sector;
            RpMaterial **materials =
                &world->matList.materials[atomic->matListWindowBase];
            if (!atomic->mesh) {
                RpBuildMesh *mesh;

                mesh = _rpBuildMeshCreate(atomic->numTriangles);
                if (mesh != 0) {
                    world = WorldBuildMeshAtomicSector(
                        world, mesh, atomic, materials);
                    if (world == 0) {
                        return 0;
                    }
                } else {
                    return 0;
                }
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
    RwLLLink *end;

    link = WorldGlobals()->worldList.link.next;
    end = &WorldGlobals()->worldList.link;
    while (link != end) {
        const RpWorldListEntry *entry =
            (const RpWorldListEntry *)((const unsigned char *)link - 8);

        if (entry->world->object.privateFlags & 1) {
            if ((const void *)target >= (const void *)entry->world) {
                const void *endOfWorld =
                    (const void *)((const unsigned char *)entry->world +
                                   entry->memorySize);
                if ((const void *)target < endOfWorld)
                    return entry->world;
            }
        } else {
            RpWorldFindSectorData find;
            find.sector = target;
            find.foundSector = 0;
            RpWorldForAllWorldSectors(entry->world, WorldFindSector, &find);
            if (find.foundSector)
                return entry->world;
        }
        link = link->next;
    }
    return 0;
}

int RpWorldDestroy(RpWorld *world) {
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
    return 1;
}

void RpWorldSetSectorRenderCallBack(RpWorld *world,
                                    RpWorldSectorCallBackRender cb) {
    if (cb == 0) {
        cb = _rpSectorDefaultRenderCallBack;
    }
    world->renderCallBack = cb;
}

RpWorld *RpWorldCreate(RwBBox *box) {
    RpWorld *world;
    RpWorldSector *sector;

    world = RwEngineInstance->fpMalloc(worldTKList.sizeOfStruct, 0x3000B);
    if (!world) {
        RwError error;
        error.pluginID = 2;
        error.errorCode = _rwerror(0x80000013, worldTKList.sizeOfStruct);
        RwErrorSet(&error);
        return 0;
    }
    rwObjectInitialize(world, 7, 0);
    _rpMaterialListInitialize(&world->matList);
    world->renderOrder = rpWORLDRENDERBACK2FRONT;
    world->flags = 0;
    sector = RwEngineInstance->fpMalloc(sectorTKList.sizeOfStruct, 0x3000B);
    if (!sector) {
        RwError error;
        error.pluginID = 2;
        error.errorCode = _rwerror(0x80000013, sizeof(int));
        RwErrorSet(&error);
        RwEngineInstance->fpFree(world);
        return 0;
    }
    sector->type = -1;
    sector->repEntry = 0;
    sector->mesh = 0;
    rwLinkListInitialize(&sector->collAtomicsInWorldSector);
    rwLinkListInitialize(&sector->lightsInWorldSector);
    sector->numVertices = 0;
    sector->numTriangles = 0;
    sector->vertices = 0;
    sector->triangles = 0;
    sector->normals = 0;
    memset(sector->texCoords, 0, sizeof(sector->texCoords));
    sector->preLitLum = 0;
    sector->boundingBox.inf = box->inf;
    sector->boundingBox.sup = box->sup;
    sector->tightBoundingBox.inf = box->inf;
    sector->tightBoundingBox.sup = box->sup;
    sector->pipeline = 0;
    world->rootSector = (RpSector *)sector;
    world->numTexCoordSets = 0;
    world->worldOrigin.x = world->worldOrigin.y = world->worldOrigin.z = 0.0f;
    world->boundingBox.inf = box->inf;
    world->boundingBox.sup = box->sup;
    rwLinkListInitialize(&world->clumpList);
    world->numClumpsInWorld = 0;
    world->currentClumpLink = &world->clumpList.link;
    rwLinkListInitialize(&world->lightList);
    rwLinkListInitialize(&world->directionalLightList);
    RpWorldSetSectorRenderCallBack(world, 0);
    world->pipeline = 0;
    _rpWorldRegisterWorld(world, worldTKList.sizeOfStruct);
    _rwPluginRegistryInitObject(&worldTKList, world);
    _rwPluginRegistryInitObject(&sectorTKList, sector);
    if (!RpWorldUnlock(world)) {
        RpWorldDestroy(world);
        return 0;
    }
    return world;
}

RpWorld *RpWorldForAllLights(RpWorld *world, RpLightCallBack cb, void *data) {
    RwLLLink *link;
    RwLLLink *end;
    RwLLLink *next;

    if (world->directionalLightList.link.next !=
        &world->directionalLightList.link) {
        link = world->directionalLightList.link.next;
        end = &world->directionalLightList.link;
        while (end != link) {
            RpLight *light = (RpLight *)((unsigned char *)link - 0x34);
            next = link->next;
            if (cb(light, data) == 0) {
                return world;
            }
            link = next;
        }
    }

    if (world->lightList.link.next != &world->lightList.link) {
        link = world->lightList.link.next;
        end = &world->lightList.link;
        while (end != link) {
            RpLight *light = (RpLight *)((unsigned char *)link - 0x34);
            next = link->next;
            if (cb(light, data) == 0) {
                return world;
            }
            link = next;
        }
    }
    return world;
}


RpWorld *RpWorldForAllWorldSectors(RpWorld *world, RpWorldSectorCallBack cb,
                                   void *data) {
    RpSector *sector;
    RpSector *stack[64];
    int top = 0;

    sector = world->rootSector;
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

int RpWorldRegisterPlugin(int size, unsigned int id,
                              RwPluginObjectConstructor c,
                              RwPluginObjectDestructor d,
                              RwPluginObjectCopy cp) {
    int result;
    result = _rwPluginRegistryAddPlugin(&worldTKList, size, id, c, d, cp);
    return result;
}
int RpWorldRegisterPluginStream(unsigned int id,
                                    RwPluginDataChunkReadCallBack r,
                                    RwPluginDataChunkWriteCallBack w,
                                    RwPluginDataChunkGetSizeCallBack s) {
    return 0;
}

int RpWorldPluginAttach(void) {
    int result = 0;
    result |=
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
        return 0;
    }
    if (!_rpWorldObjRegisterExtensions()) {
        return 0;
    }
    if (!_rpClumpRegisterExtensions()) {
        return 0;
    }
    if (!_rxWorldDevicePluginAttach()) {
        return 0;
    }
    return 1;
}
