#include "rw/rwengine.h"
#include "rw/gamecube.h"
#include "rw/nodegamecube.h"
#include "rw/rplight.h"
#include "rw/rwresources.h"

typedef struct RxGameCubeWorldSectorAllInOnePrivateData {
    RxGCSectorInstanceCallBack instanceCallback;
    RxGCSectorInstanceCallBack reinstanceCallback;
    RxGCSectorLightingCallBack lightingCallback;
    RxGCSectorRenderCallBack renderCallback;
} RxGameCubeWorldSectorAllInOnePrivateData;

extern RxPipelineNode* _rxGameCubeAllInOneSetInstanceCallBack(
    RxPipelineNode*, RxGCSectorInstanceCallBack);
extern RxPipelineNode* _rxGameCubeAllInOneSetReinstanceCallBack(
    RxPipelineNode*, RxGCSectorInstanceCallBack);
extern RxPipelineNode* _rxGameCubeAllInOneSetLightingCallBack(
    RxPipelineNode*, RxGCSectorLightingCallBack);
extern RxPipelineNode* RxGameCubeAllInOneSetRenderCallBack(
    RxPipelineNode*, RxGCSectorRenderCallBack);

static int _rxGCWorldSectorAllInOneNode(
    RxPipelineNode* self, const RxPipelineNodeParam* params);
static int _rxGCWorldSectorAllInOnePipelineInit(RxPipelineNode* self);

static RxNodeDefinition nodeGameCubeWorldSectorAllInOneCSL = {
    "GamerCubeWorldSectorAllInOne.csl",
    {_rxGCWorldSectorAllInOneNode, 0, 0,
     _rxGCWorldSectorAllInOnePipelineInit, 0, 0, 0},
    {0, 0, 0, 0, 0},
    sizeof(RxGameCubeWorldSectorAllInOnePrivateData),
    0,
    0
};

RxNodeDefinition* RxNodeDefinitionGetGameCubeWorldSectorAllInOne(void)
{
    return &nodeGameCubeWorldSectorAllInOneCSL;
}


RxPipeline* _rpDlSectorPipelineCreate(
    unsigned int pluginId, unsigned int pluginData,
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

    if ((((RpWorld*)RwEngineInstance->curWorld)->flags & 0x20) != 0) {
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
    RpWorld* world = (RpWorld*)RwEngineInstance->curWorld;

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


static int _rxGCWorldSectorAllInOneNode(
    RxPipelineNode* self, const RxPipelineNodeParam* params)
{
    RpWorldSector* sector = (RpWorldSector*)params->dataParam;
    RxGameCubeWorldSectorAllInOnePrivateData* privateData =
        (RxGameCubeWorldSectorAllInOnePrivateData*)self->privateData;
    RpWorld* world = (RpWorld*)RwEngineInstance->curWorld;
    RxGameCubeWorldSectorAllInOneInstanceData instanceData;

    if ((world->flags & 0x01000000) == 0) {
        RwResEntry* resourceEntry;
        unsigned short numPolygons = sector->numTriangles;
        RpMeshHeader* meshHeader;
        unsigned short numMeshes;

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

            if (header->vertexBuffer.meshSerialNum != meshHeader->serialNum) {
                RwResourcesFreeResEntry(resourceEntry);
                instanceData.resourceEntry = 0;
            }
        }

        if (instanceData.resourceEntry != 0) {
            RwLLLink* link = &instanceData.resourceEntry->link;

            if (link->next != 0) {
                RwResourcesGlobals* resources =
                    (RwResourcesGlobals*)((unsigned char*)RwEngineInstance +
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

static int _rxGCWorldSectorAllInOnePipelineInit(RxPipelineNode* self)
{
    RxGameCubeWorldSectorAllInOnePrivateData* privateData =
        (RxGameCubeWorldSectorAllInOnePrivateData*)self->privateData;

    privateData->instanceCallback = _rxGCSectorDefaultInstanceCallback;
    privateData->reinstanceCallback = 0;
    privateData->lightingCallback = _rxGCSectorDefaultLightingCallback;
    privateData->renderCallback =
        (RxGCSectorRenderCallBack)_rxGCDefaultRenderCallback;
    return 1;
}
