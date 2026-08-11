#include "dolphin/gx.h"
#include "dolphin/os.h"
#include "libmkparticle/rw_engine.h"
#include "rw/gamecube.h"
#include "rw/nodegamecube.h"
#include "rw/rplight.h"
#include "rw/rwresources.h"

extern RwMatrix* RwFrameGetLTM(RwFrame* frame);

struct RwGameCubeLightingData {
    RwUInt8 reserved_0x00[0x0C];
    RwRGBAReal ambient;
    RwBool hasAmbient;
    RwUInt32 lightMask;
    RwInt32 lightIndex;
};

struct RxGameCubeAllInOneInstanceData {
    RwResEntry* resourceEntry;
    RpMeshHeader* meshHeader;
    RwUInt32 geometryFlags;
    RwRGBAReal ambient;
    RwBool hasAmbient;
    RwUInt32 lightMask;
    RwInt32 lightIndex;
    void* morphData;
};

typedef struct RpAtomicSectorTie {
    RwUInt8 reserved_0x00[0x0C];
    RwLLLink atomicLink;
    RpWorldSector* sector;
} RpAtomicSectorTie;

typedef struct RpLightTie {
    RwLLLink link;
    RpLight* light;
} RpLightTie;

typedef struct RwGameCubeResEntryHeader {
    RwResEntry entry;
    union {
        struct {
            RwUInt16 token;
            RwUInt16 meshSerialNum;
        };
        RwGameCubeVertexBuffer vertexBuffer;
    };
} RwGameCubeResEntryHeader;

typedef struct RwResourcesGlobalsPrefix {
    RwUInt32 arenaSize;
    RwUInt32 arenaUsage;
    RwUInt32 arenaReusage;
    void* arena;
    RwLinkList entriesA;
    RwLinkList entriesB;
    RwLLLink* activeList;
} RwResourcesGlobalsPrefix;

typedef void* (*RxGCAtomicInstanceCallBack)(void*, RwResEntry**);
typedef RpAtomic* (*RxGCAtomicLightingCallBack)(RpAtomic*,
                                                RwGameCubeLightingData*);
typedef RpAtomic* (*RxGCAtomicRenderCallBack)(
    RpAtomic*, RxGameCubeAllInOneInstanceData*);

typedef struct RxGameCubeAtomicAllInOnePrivateData {
    RxGCAtomicInstanceCallBack instanceCallback;
    RxGCAtomicInstanceCallBack reinstanceCallback;
    RxGCAtomicLightingCallBack lightingCallback;
    RxGCAtomicRenderCallBack renderCallback;
} RxGameCubeAtomicAllInOnePrivateData;

extern RwModuleInfo resourcesModule;
extern RwInt32 _RwDlPreInstanceOptimize;
extern RwInt32 _rpDlGeomVtxFmtOffset;
extern RwUInt16 _RwDlTokenCurrent;

extern void _rwGCLightsGlobalEnable(RwInt32,
                                    RwGameCubeLightingData*);
extern void _rwGCLightsLocalEnable(RpLight*, RwGameCubeLightingData*);
extern RwInt32 _rwDlTokenQueryDone(RwUInt16);
extern RpAtomic* _rxGCDefaultRenderCallback(
    RpAtomic*, RxGameCubeAllInOneInstanceData*);

extern RxPipelineNode* _rxGameCubeAllInOneSetInstanceCallBack(
    RxPipelineNode*, RxGCAtomicInstanceCallBack);
extern RxPipelineNode* _rxGameCubeAllInOneSetReinstanceCallBack(
    RxPipelineNode*, RxGCAtomicInstanceCallBack);
extern RxPipelineNode* _rxGameCubeAllInOneSetLightingCallBack(
    RxPipelineNode*, RxGCAtomicLightingCallBack);
extern RxPipelineNode* RxGameCubeAllInOneSetRenderCallBack(
    RxPipelineNode*, RxGCAtomicRenderCallBack);


static RwBool _rxGCAtomicAllInOneNode(
    RxPipelineNode* self, const RxPipelineNodeParam* params);
static RwBool _rxGCAtomicAllInOnePipelineInit(RxPipelineNode* self);

static RxNodeDefinition nodeGameCubeAtomicAllInOneCSL = {
    "GameCubeAtomicAllInOne.csl",
    {_rxGCAtomicAllInOneNode, 0, 0, _rxGCAtomicAllInOnePipelineInit,
     0, 0, 0},
    {0, 0, 0, 0, 0},
    sizeof(RxGameCubeAtomicAllInOnePrivateData),
    0,
    0
};

RxNodeDefinition* RxNodeDefinitionGetGameCubeAtomicAllInOne(void)
{
    return &nodeGameCubeAtomicAllInOneCSL;
}


RxPipeline* _rpDlAtomicPipelineCreate(
    RwUInt32 pluginId, RwUInt32 pluginData,
    RxGCAtomicInstanceCallBack instanceCallback,
    RxGCAtomicInstanceCallBack reinstanceCallback,
    RxGCAtomicLightingCallBack lightingCallback,
    RxGCAtomicRenderCallBack renderCallback)
{
    RxPipeline* pipeline;
    RxLockedPipe* lockedPipeline;
    RxNodeDefinition* nodeDefinition;
    RxPipelineNode* node;

    pipeline = RxPipelineCreate();
    pipeline->pluginId = pluginId;
    pipeline->pluginData = pluginData;
    lockedPipeline = RxPipelineLock(pipeline);
    nodeDefinition = RxNodeDefinitionGetGameCubeAtomicAllInOne();
    lockedPipeline = RxLockedPipeAddFragment(
        lockedPipeline, 0, nodeDefinition, 0);
    lockedPipeline = RxLockedPipeUnlock(lockedPipeline);
    node = RxPipelineFindNodeByName(pipeline, nodeDefinition->name, 0, 0);
    _rxGameCubeAllInOneSetInstanceCallBack(node, instanceCallback);
    _rxGameCubeAllInOneSetReinstanceCallBack(node, reinstanceCallback);
    _rxGameCubeAllInOneSetLightingCallBack(node, lightingCallback);
    RxGameCubeAllInOneSetRenderCallBack(node, renderCallback);
    return pipeline;
}


RpAtomic* _rxGCAtomicDefaultLightingCallback(
    RpAtomic* atomic, RwGameCubeLightingData* lighting)
{
    lighting->lightMask = 0;
    lighting->ambient.red = 0.0f;
    lighting->ambient.green = 0.0f;
    lighting->ambient.blue = 0.0f;
    lighting->ambient.alpha = 1.0f;
    lighting->hasAmbient = 0;
    lighting->lightIndex = 0;

    if ((atomic->geometry->flags & 0x20) != 0 &&
        RwEngineInstance->field_0x04 != 0) {
        RwLLLink* sectorLink;
        RwLLLink* sectorEnd;

        _rwGCLightsGlobalEnable(1, lighting);
        RwEngineInstance->lightFrame++;
        sectorLink = atomic->worldSectorsInAtomic.link.next;
        sectorEnd = &atomic->worldSectorsInAtomic.link;
        while (sectorLink != sectorEnd) {
            RpAtomicSectorTie* atomicTie =
                (RpAtomicSectorTie*)((RwUInt8*)sectorLink - 0x0C);
            RpWorldSector* sector = atomicTie->sector;
            RwLLLink* lightLink = sector->lightsInWorldSector.link.next;
            RwLLLink* lightEnd = &sector->lightsInWorldSector.link;

            while (lightLink != lightEnd) {
                RpLight* light = ((RpLightTie*)lightLink)->light;

                if (light != 0 &&
                    light->lightFrame != RwEngineInstance->lightFrame &&
                    (light->object.object.flags & 1) != 0) {
                    RwV3d delta;
                    RwV3d* lightPosition;
                    RwSphere* sphere;
                    RwReal radius;

                    light->lightFrame = RwEngineInstance->lightFrame;
                    lightPosition =
                        &RwFrameGetLTM(RpLightGetFrame(light))->pos;
                    sphere = RpAtomicGetWorldBoundingSphere(atomic);
                    delta.x = sphere->center.x - lightPosition->x;
                    delta.y = sphere->center.y - lightPosition->y;
                    delta.z = sphere->center.z - lightPosition->z;
                    radius = sphere->radius + light->radius;
                    if (delta.x * delta.x + delta.y * delta.y +
                            delta.z * delta.z <
                        radius * radius) {
                        _rwGCLightsLocalEnable(light, lighting);
                    }
                }
                lightLink = lightLink->next;
            }
            sectorLink = sectorLink->next;
        }
    }

    if (lighting->hasAmbient != 0) {
        if (lighting->ambient.red > 1.0f)
            lighting->ambient.red = 1.0f;
        if (lighting->ambient.green > 1.0f)
            lighting->ambient.green = 1.0f;
        if (lighting->ambient.blue > 1.0f)
            lighting->ambient.blue = 1.0f;
    }
    return atomic;
}


static void _rxGCDefaultReinstance(
    RpGeometry* geometry, RwGameCubeVertexBuffer* vertexBuffer,
    RwUInt32 flags)
{
    RwUInt32 streamIndex = 0;
    RwUInt16 locked = geometry->lockedSinceLastInst;
    RwUInt32 geometryFlags = geometry->flags | 2;
    RwUInt32 numVertices = geometry->numVertices;
    RpGameCubeVtxFmt* format = *(RpGameCubeVtxFmt**)(
        (RwUInt8*)geometry + _rpDlGeomVtxFmtOffset);

    if (format == 0)
        format = _rpGameCubeVtxFmtGetDefault();

    if ((geometryFlags & 2) != 0) {
        if ((locked & 2) != 0) {
            void* destination = vertexBuffer->arrays[streamIndex].data;
            RwUInt32 size = _rwGCNVtxFmtInstPos3D(
                destination, geometry->morphTarget->verts,
                format->positionType, numVertices,
                vertexBuffer->arrays[streamIndex].stride, 0,
                (RwReal)(1 << format->positionFraction));
            DCFlushRange(destination, size);
        }
        streamIndex++;
    }
    if ((geometryFlags & 0x10) != 0) {
        if ((locked & 4) != 0) {
            void* destination = vertexBuffer->arrays[streamIndex].data;
            RwUInt32 size = _rwGCNVtxFmtInstNrm(
                destination, geometry->morphTarget->normals,
                format->normalType, numVertices,
                vertexBuffer->arrays[streamIndex].stride);
            DCFlushRange(destination, size);
        }
        streamIndex++;
    }
    if ((geometryFlags & 8) != 0) {
        if ((locked & 8) != 0) {
            void* destination = vertexBuffer->arrays[streamIndex].data;
            RwUInt32 size;

            if (*(RpGameCubeVtxFmt**)(
                    (RwUInt8*)geometry + _rpDlGeomVtxFmtOffset) == 0) {
                RwUInt32 i;

                vertexBuffer->reserved_0x00[1] &= ~1U;
                for (i = 0; i < numVertices; i++) {
                    if (((RwRGBA*)geometry->preLitLum)[i].alpha < 0xFF) {
                        vertexBuffer->reserved_0x00[1] |= 1;
                        break;
                    }
                }
            } else if (format->colorType > 2) {
                vertexBuffer->reserved_0x00[1] |= 1;
            } else {
                vertexBuffer->reserved_0x00[1] &= ~1U;
            }
            size = _rwGCNVtxFmtInstClr(
                destination, geometry->preLitLum, format->colorType,
                numVertices, vertexBuffer->arrays[streamIndex].stride);
            DCFlushRange(destination, size);
        }
        streamIndex++;
    }
    if ((geometryFlags & 0x84) != 0) {
        RwInt32 i;

        for (i = 0; i < geometry->numTexCoordSets; i++) {
            if ((locked & (0x10 << i)) != 0) {
                void* destination = vertexBuffer->arrays[streamIndex].data;
                RwUInt32 size = _rwGCNVtxFmtInstTex(
                    destination, geometry->texCoords[i],
                    format->texCoordType[i], numVertices,
                    vertexBuffer->arrays[streamIndex].stride,
                    (RwReal)(1 << format->texCoordFraction[i]));
                DCFlushRange(destination, size);
            }
            streamIndex++;
        }
    }
    GXInvalidateVtxCache();
}


void* _rxGCAtomicDefaultReinstanceCallback(
    void* object, RwResEntry** resourceEntry)
{
    RpAtomic* atomic = (RpAtomic*)object;
    RpGeometry* geometry = atomic->geometry;
    RwGameCubeResEntryHeader* header =
        (RwGameCubeResEntryHeader*)*resourceEntry;
    RwGameCubeVertexBuffer* vertexBuffer = &header->vertexBuffer;

    if (geometry->lockedSinceLastInst != 0) {
        if (header->token == _RwDlTokenCurrent) {
            GXSetDrawSync(_RwDlTokenCurrent);
            _RwDlTokenCurrent = (_RwDlTokenCurrent + 1) % 57344;
        }
        while (_rwDlTokenQueryDone(header->token) == 0) {
        }
        _rxGCDefaultReinstance(geometry, vertexBuffer,
                               vertexBuffer->reserved_0x00[2]);
        geometry->lockedSinceLastInst = 0;
    }
    if (geometry->numMorphTargets != 1 &&
        (atomic->interpolator.flags & 1) != 0) {
        if (header->token == _RwDlTokenCurrent) {
            GXSetDrawSync(_RwDlTokenCurrent);
            _RwDlTokenCurrent = (_RwDlTokenCurrent + 1) % 57344;
        }
        while (_rwDlTokenQueryDone(header->token) == 0) {
        }
        _rxGCInstanceMorphUpdate(geometry, vertexBuffer,
                                 &atomic->interpolator);
        atomic->interpolator.flags &= ~1U;
    }
    return object;
}


void* _rxGCAtomicDefaultInstanceCallback(
    void* object, RwResEntry** resourceEntry)
{
    RpAtomic* atomic = (RpAtomic*)object;
    RpGeometry* geometry = atomic->geometry;
    void* owner;
    RwResEntry** ownerRef;

    if (geometry->numMorphTargets != 1) {
        owner = atomic;
        ownerRef = (RwResEntry**)&atomic->repEntry;
    } else {
        owner = geometry;
        ownerRef = &geometry->repEntry;
    }
    if ((geometry->flags & 0x02000000) != 0) {
        if (_RwDlPreInstanceOptimize == 1)
            *resourceEntry = _rwDlGeometryInstanceOptimized(
                geometry, owner, ownerRef);
        else
            *resourceEntry = _rwDlGeometryInstanceFast(
                geometry, owner, ownerRef);
    } else {
        *resourceEntry = _rwDlGeometryInstanceFast(geometry, owner, ownerRef);
    }
    geometry->lockedSinceLastInst = 0;
    return object;
}


static RwBool _rxGCAtomicAllInOneNode(
    RxPipelineNode* self, const RxPipelineNodeParam* params)
{
    RpAtomic* atomic = (RpAtomic*)params->dataParam;
    RpGeometry* geometry = atomic->geometry;
    RxGameCubeAtomicAllInOnePrivateData* privateData =
        (RxGameCubeAtomicAllInOnePrivateData*)self->privateData;
    RxGameCubeAllInOneInstanceData instanceData;

    if ((geometry->flags & 0x01000000) == 0) {
        RwResEntry* resourceEntry;
        RpMeshHeader* meshHeader;

        if (geometry->numVertices == 0)
            return 1;
        meshHeader = geometry->meshHeader;
        if (meshHeader->numMeshes == 0)
            return 1;

        if (geometry->numMorphTargets != 1)
            resourceEntry = (RwResEntry*)atomic->repEntry;
        else
            resourceEntry = geometry->repEntry;
        instanceData.resourceEntry = resourceEntry;
        instanceData.meshHeader = meshHeader;
        instanceData.geometryFlags = geometry->flags;

        if (resourceEntry != 0 &&
            ((RwGameCubeResEntryHeader*)resourceEntry)->meshSerialNum !=
                meshHeader->serialNum) {
            RwResourcesFreeResEntry(resourceEntry);
            instanceData.resourceEntry = 0;
        }
        if (instanceData.resourceEntry != 0) {
            if (privateData->reinstanceCallback != 0 &&
                privateData->reinstanceCallback(
                    atomic, &instanceData.resourceEntry) != atomic)
                return 0;

            if (instanceData.resourceEntry->link.next != 0) {
                RwLLLink* link = &instanceData.resourceEntry->link;
                RwResourcesGlobalsPrefix* resources =
                    (RwResourcesGlobalsPrefix*)((RwUInt8*)RwEngineInstance +
                                                resourcesModule.globalsOffset);

                link->prev->next = link->next;
                link->next->prev = link->prev;
                link->next = resources->activeList->next;
                link->prev = resources->activeList;
                resources->activeList->next->prev = link;
                resources->activeList->next = link;
            }
        } else {
            if (privateData->instanceCallback != 0 &&
                privateData->instanceCallback(
                    atomic, &instanceData.resourceEntry) != atomic)
                return 0;
            if (privateData->reinstanceCallback != 0 &&
                privateData->reinstanceCallback(
                    atomic, &instanceData.resourceEntry) != atomic)
                return 0;
        }
    } else {
        instanceData.resourceEntry = geometry->repEntry;
        instanceData.meshHeader = geometry->meshHeader;
        instanceData.geometryFlags = geometry->flags;
        if (privateData->reinstanceCallback != 0 &&
            privateData->reinstanceCallback(
                atomic, &instanceData.resourceEntry) != atomic)
            return 0;
    }

    if (privateData->lightingCallback != 0 &&
        privateData->lightingCallback(
            atomic, (RwGameCubeLightingData*)&instanceData) != atomic)
        return 0;
    if (privateData->renderCallback != 0 &&
        privateData->renderCallback(atomic, &instanceData) != atomic)
        return 0;
    return 1;
}

static RwBool _rxGCAtomicAllInOnePipelineInit(RxPipelineNode* self)
{
    RxGameCubeAtomicAllInOnePrivateData* privateData =
        (RxGameCubeAtomicAllInOnePrivateData*)self->privateData;

    privateData->instanceCallback = _rxGCAtomicDefaultInstanceCallback;
    privateData->reinstanceCallback = _rxGCAtomicDefaultReinstanceCallback;
    privateData->lightingCallback = _rxGCAtomicDefaultLightingCallback;
    privateData->renderCallback = _rxGCDefaultRenderCallback;
    return 1;
}
