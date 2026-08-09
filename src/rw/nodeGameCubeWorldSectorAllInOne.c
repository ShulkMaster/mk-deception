#include "libmkparticle/rw_engine.h"
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
extern RwResEntry* _rwDlWorldSectorInstanceOptimized(
    RpWorld*, RpWorldSector*, RpWorldSector*, RwResEntry**);
extern RwResEntry* _rwDlWorldSectorInstanceFast(
    RpWorld*, RpWorldSector*, RpWorldSector*, RwResEntry**);
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

#define RESOURCES_GLOBALS_PREFIX                                           \
    (*(RwResourcesGlobalsPrefix*)((RwUInt8*)RwEngineInstance +             \
                                  resourcesModule.globalsOffset))

static RwBool _rxGCWorldSectorAllInOneNode(
    RxPipelineNode* self, const RxPipelineNodeParam* params);
static RwBool _rxGCWorldSectorAllInOnePipelineInit(RxPipelineNode* self);

static RxNodeDefinition nodeGameCubeWorldSectorAllInOneCSL = {
    "GamerCubeWorldSectorAllInOne.csl",
    {_rxGCWorldSectorAllInOneNode, NULL, NULL,
     _rxGCWorldSectorAllInOnePipelineInit, NULL, NULL, NULL},
    {0, NULL, NULL, 0, NULL},
    sizeof(RxGameCubeAllInOnePrivateData),
    FALSE,
    0
};

RxNodeDefinition* RxNodeDefinitionGetGameCubeWorldSectorAllInOne(void)
{
    return &nodeGameCubeWorldSectorAllInOneCSL;
}

/* The complete pipeline construction body and callback order match retail.
 * Its O0 build uses savegpr helpers for r28-r31 while clean typed C selects
 * individual saves, shifting the homed callback arguments. */
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
        lockedPipeline, NULL, nodeDefinition, NULL);
    lockedPipeline = RxLockedPipeUnlock(lockedPipeline);
    node = RxPipelineFindNodeByName(pipeline, nodeDefinition->name, NULL, NULL);
    _rxGameCubeAllInOneSetInstanceCallBack(node, instanceCallback);
    _rxGameCubeAllInOneSetReinstanceCallBack(node, reinstanceCallback);
    _rxGameCubeAllInOneSetLightingCallBack(node, lightingCallback);
    RxGameCubeAllInOneSetRenderCallBack(node, renderCallback);
    return pipeline;
}

/* The stores, light traversal, clamps, and frame update are exact. Retail
 * retains a second sector cursor around the intrusive-list walk; the clean
 * source needs only the input sector, leaving register coloring residue. */
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
    lighting->hasAmbient = FALSE;
    lighting->lightIndex = 0;

    if ((((RpWorld*)RwEngineInstance->curWorld)->flags & 0x20) != 0) {
        _rwGCLightsGlobalEnable(2, lighting);
        RwEngineInstance->lightFrame++;
        link = sector->lightsInWorldSector.link.next;
        end = &sector->lightsInWorldSector.link;
        while (link != end) {
            RpLightTie* tie = (RpLightTie*)link;
            RpLight* light = tie->light;

            if (light != NULL &&
                (light->object.object.flags & 2) != 0) {
                _rwGCLightsLocalEnable(light, lighting);
            }
            link = link->next;
        }
    }

    if (lighting->hasAmbient != FALSE) {
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

/* All three instancing branches and calls now match. Retail additionally
 * copies the sector parameter to a second nonvolatile register and uses the
 * savegpr helpers; reproducing that copy would be redundant liveness. */
RpWorldSector* _rxGCSectorDefaultInstanceCallback(
    RpWorldSector* sector, RwResEntry** resourceEntry)
{
    RpWorld* world = (RpWorld*)RwEngineInstance->curWorld;

    if ((world->flags & 0x02000000) != 0) {
        if (_RwDlPreInstanceOptimize == TRUE) {
            *resourceEntry = _rwDlWorldSectorInstanceOptimized(
                world, sector, sector, &sector->repEntry);
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

/* Algorithm recovered: native/non-native selection, stale-resource teardown,
 * active-list promotion, instance/lighting/render callback ordering, and all
 * failure returns match retail. Remaining differences are callback-macro
 * result lifetimes and nonvolatile coloring rather than pipeline behavior. */
static RwBool _rxGCWorldSectorAllInOneNode(
    RxPipelineNode* self, const RxPipelineNodeParam* params)
{
    RpWorldSector* sector = (RpWorldSector*)params->dataParam;
    RxGameCubeAllInOnePrivateData* privateData =
        (RxGameCubeAllInOnePrivateData*)self->privateData;
    RpWorld* world = (RpWorld*)RwEngineInstance->curWorld;
    RxGameCubeAllInOneInstanceData instanceData;

    if ((world->flags & 0x01000000) == 0) {
        RwResEntry* resourceEntry;
        RwUInt16 numPolygons = sector->numPolygons;
        RpMeshHeader* meshHeader;
        RwUInt16 numMeshes;

        if (numPolygons == 0) {
            return TRUE;
        }
        meshHeader = sector->mesh;
        numMeshes = meshHeader->numMeshes;
        if (numMeshes == 0) {
            return TRUE;
        }

        resourceEntry = sector->repEntry;
        instanceData.resourceEntry = resourceEntry;
        instanceData.meshHeader = meshHeader;
        instanceData.worldFlags = world->flags;

        if (resourceEntry != NULL) {
            RwGameCubeResEntryHeader* header =
                (RwGameCubeResEntryHeader*)resourceEntry;

            if (header->meshSerialNum != meshHeader->serialNum) {
                RwResourcesFreeResEntry(resourceEntry);
                instanceData.resourceEntry = NULL;
            }
        }

        if (instanceData.resourceEntry != NULL) {
            RwLLLink* link = &instanceData.resourceEntry->link;

            if (link->next != NULL) {
                link->prev->next = link->next;
                link->next->prev = link->prev;
                link->next = RESOURCES_GLOBALS_PREFIX.activeList->next;
                link->prev = RESOURCES_GLOBALS_PREFIX.activeList;
                RESOURCES_GLOBALS_PREFIX.activeList->next->prev = link;
                RESOURCES_GLOBALS_PREFIX.activeList->next = link;
            }
        } else if (privateData->instanceCallback != NULL &&
                   privateData->instanceCallback(
                       sector, &instanceData.resourceEntry) != sector) {
            return FALSE;
        }
    } else {
        instanceData.resourceEntry = sector->repEntry;
        instanceData.meshHeader = sector->mesh;
        instanceData.worldFlags = world->flags;
    }

    if (privateData->lightingCallback != NULL &&
        privateData->lightingCallback(sector, (RwGameCubeLightingData*)&instanceData) !=
            sector) {
        return FALSE;
    }
    if (privateData->renderCallback != NULL &&
        privateData->renderCallback(sector, &instanceData) != sector) {
        return FALSE;
    }
    return TRUE;
}

static RwBool _rxGCWorldSectorAllInOnePipelineInit(RxPipelineNode* self)
{
    RxGameCubeAllInOnePrivateData* privateData =
        (RxGameCubeAllInOnePrivateData*)self->privateData;

    privateData->instanceCallback = _rxGCSectorDefaultInstanceCallback;
    privateData->reinstanceCallback = NULL;
    privateData->lightingCallback = _rxGCSectorDefaultLightingCallback;
    privateData->renderCallback = _rxGCDefaultRenderCallback;
    return TRUE;
}
