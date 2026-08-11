#include "libmkparticle/rw_engine.h"
#include "rw/gamecube.h"
#include "rw/nodegamecube.h"
#include "rw/rplight.h"

struct RwGameCubeLightingData {
    RwUInt8 reserved_0x00[0x0C];
    RwRGBAReal ambient;
    RwBool hasAmbient;
    RwUInt32 lightMask;
    RwInt32 lightIndex;
};

typedef struct RpLightTie {
    RwLLLink link;
    RpLight* light;
} RpLightTie;

typedef struct RwGameCubeResEntryHeader {
    RwResEntry entry;
    RwUInt16 field_0x18;
    RwUInt16 meshSerialNum;
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

struct RxGameCubeAllInOneInstanceData {
    RwResEntry* resourceEntry;
    RpMeshHeader* meshHeader;
    RwUInt32 worldFlags;
    RwRGBAReal ambient;
    RwBool hasAmbient;
    RwUInt32 lightMask;
    RwInt32 lightIndex;
};

typedef struct RxGameCubeAllInOnePrivateData {
    RxGCSectorInstanceCallBack instanceCallback;
    RxGCSectorInstanceCallBack reinstanceCallback;
    RxGCSectorLightingCallBack lightingCallback;
    RxGCSectorRenderCallBack renderCallback;
} RxGameCubeAllInOnePrivateData;

extern RwModuleInfo resourcesModule;
extern RwInt32 _RwDlPreInstanceOptimize;

extern void _rwGCLightsGlobalEnable(RwInt32,
                                    RwGameCubeLightingData*);
extern void _rwGCLightsLocalEnable(RpLight*, RwGameCubeLightingData*);
extern RpWorldSector* _rxGCDefaultRenderCallback(
    RpWorldSector*, RxGameCubeAllInOneInstanceData*);

extern RxPipelineNode* _rxGameCubeAllInOneSetInstanceCallBack(
    RxPipelineNode*, RxGCSectorInstanceCallBack);
extern RxPipelineNode* _rxGameCubeAllInOneSetReinstanceCallBack(
    RxPipelineNode*, RxGCSectorInstanceCallBack);
extern RxPipelineNode* _rxGameCubeAllInOneSetLightingCallBack(
    RxPipelineNode*, RxGCSectorLightingCallBack);
extern RxPipelineNode* RxGameCubeAllInOneSetRenderCallBack(
    RxPipelineNode*, RxGCSectorRenderCallBack);

static RwBool _rxGCWorldSectorAllInOneNode(
    RxPipelineNode* self, const RxPipelineNodeParam* params);
static RwBool _rxGCWorldSectorAllInOnePipelineInit(RxPipelineNode* self);

static RxNodeDefinition nodeGameCubeWorldSectorAllInOneCSL = {
    "GamerCubeWorldSectorAllInOne.csl",
    {_rxGCWorldSectorAllInOneNode, 0, 0,
     _rxGCWorldSectorAllInOnePipelineInit, 0, 0, 0},
    {0, 0, 0, 0, 0},
    sizeof(RxGameCubeAllInOnePrivateData),
    0,
    0
};

RxNodeDefinition* RxNodeDefinitionGetGameCubeWorldSectorAllInOne(void)
{
    return &nodeGameCubeWorldSectorAllInOneCSL;
}


RxPipeline* _rpDlSectorPipelineCreate(
    RwUInt32 pluginId, RwUInt32 pluginData,
    RxGCSectorInstanceCallBack instanceCallback,
    RxGCSectorInstanceCallBack reinstanceCallback,
    RxGCSectorLightingCallBack lightingCallback,
    RxGCSectorRenderCallBack renderCallback)
{
    RxPipeline* pipeline;
    RxLockedPipe* lockedPipeline;
    RxNodeDefinition* nodeDefinition;
    RxPipelineNode* node;

    pipeline = RxPipelineCreate();
    pipeline->pluginId = pluginId;
    pipeline->pluginData = pluginData;
    lockedPipeline = RxPipelineLock(pipeline);
    nodeDefinition = RxNodeDefinitionGetGameCubeWorldSectorAllInOne();
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


RpWorldSector* _rxGCSectorDefaultLightingCallback(
    RpWorldSector* sector, RwGameCubeLightingData* lighting)
{
    RwLLLink* link;
    RwLLLink* end;

    lighting->lightMask = 0;
    lighting->ambient.red = 0.0f;
    lighting->ambient.green = 0.0f;
    lighting->ambient.blue = 0.0f;
    lighting->ambient.alpha = 1.0f;
    lighting->hasAmbient = 0;
    lighting->lightIndex = 0;

    if ((((RpWorld*)RwEngineInstance->field_0x04)->flags & 0x20) != 0) {
        _rwGCLightsGlobalEnable(2, lighting);
        RwEngineInstance->lightFrame++;
        link = sector->lightsInWorldSector.link.next;
        end = &sector->lightsInWorldSector.link;
        while (link != end) {
            RpLightTie* tie = (RpLightTie*)link;
            RpLight* light = tie->light;

            if (light != 0 &&
                (light->object.object.flags & 2) != 0) {
                _rwGCLightsLocalEnable(light, lighting);
            }
            link = link->next;
        }
    }

    if (lighting->hasAmbient != 0) {
        if (lighting->ambient.red > 1.0f) {
            lighting->ambient.red = 1.0f;
        }
        if (lighting->ambient.green > 1.0f) {
            lighting->ambient.green = 1.0f;
        }
        if (lighting->ambient.blue > 1.0f) {
            lighting->ambient.blue = 1.0f;
        }
    }
    return sector;
}


RpWorldSector* _rxGCSectorDefaultInstanceCallback(
    RpWorldSector* sector, RwResEntry** resourceEntry)
{
    RpWorld* world = (RpWorld*)RwEngineInstance->field_0x04;

    if ((world->flags & 0x02000000) != 0) {
        if (_RwDlPreInstanceOptimize == 1) {
            *resourceEntry =
                _rwDlWorldSectorInstanceOptimized(world, sector);
        } else {
            *resourceEntry = _rwDlWorldSectorInstanceFast(
                world, sector, sector, &sector->repEntry);
        }
    } else {
        *resourceEntry = _rwDlWorldSectorInstanceFast(
            world, sector, sector, &sector->repEntry);
    }
    return sector;
}


static RwBool _rxGCWorldSectorAllInOneNode(
    RxPipelineNode* self, const RxPipelineNodeParam* params)
{
    RpWorldSector* sector = (RpWorldSector*)params->dataParam;
    RxGameCubeAllInOnePrivateData* privateData =
        (RxGameCubeAllInOnePrivateData*)self->privateData;
    RpWorld* world = (RpWorld*)RwEngineInstance->field_0x04;
    RxGameCubeAllInOneInstanceData instanceData;

    if ((world->flags & 0x01000000) == 0) {
        RwResEntry* resourceEntry;
        RwUInt16 numPolygons = sector->numTriangles;
        RpMeshHeader* meshHeader;
        RwUInt16 numMeshes;

        if (numPolygons == 0) {
            return 1;
        }
        meshHeader = sector->mesh;
        numMeshes = meshHeader->numMeshes;
        if (numMeshes == 0) {
            return 1;
        }

        resourceEntry = sector->repEntry;
        instanceData.resourceEntry = resourceEntry;
        instanceData.meshHeader = meshHeader;
        instanceData.worldFlags = world->flags;

        if (resourceEntry != 0) {
            RwGameCubeResEntryHeader* header =
                (RwGameCubeResEntryHeader*)resourceEntry;

            if (header->meshSerialNum != meshHeader->serialNum) {
                RwResourcesFreeResEntry(resourceEntry);
                instanceData.resourceEntry = 0;
            }
        }

        if (instanceData.resourceEntry != 0) {
            RwLLLink* link = &instanceData.resourceEntry->link;

            if (link->next != 0) {
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
        } else if (privateData->instanceCallback != 0 &&
                   privateData->instanceCallback(
                       sector, &instanceData.resourceEntry) != sector) {
            return 0;
        }
    } else {
        instanceData.resourceEntry = sector->repEntry;
        instanceData.meshHeader = sector->mesh;
        instanceData.worldFlags = world->flags;
    }

    if (privateData->lightingCallback != 0 &&
        privateData->lightingCallback(sector, (RwGameCubeLightingData*)&instanceData) !=
            sector) {
        return 0;
    }
    if (privateData->renderCallback != 0 &&
        privateData->renderCallback(sector, &instanceData) != sector) {
        return 0;
    }
    return 1;
}

static RwBool _rxGCWorldSectorAllInOnePipelineInit(RxPipelineNode* self)
{
    RxGameCubeAllInOnePrivateData* privateData =
        (RxGameCubeAllInOnePrivateData*)self->privateData;

    privateData->instanceCallback = _rxGCSectorDefaultInstanceCallback;
    privateData->reinstanceCallback = 0;
    privateData->lightingCallback = _rxGCSectorDefaultLightingCallback;
    privateData->renderCallback = _rxGCDefaultRenderCallback;
    return 1;
}
