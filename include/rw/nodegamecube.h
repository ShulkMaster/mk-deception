#ifndef RW_NODEGAMECUBE_H
#define RW_NODEGAMECUBE_H

#include "rw/rpworld_types.h"
#include "rw/rxpipeline.h"
#include "rw/rwresources.h"

typedef struct RwGameCubeLightingData RwGameCubeLightingData;
typedef struct RxGameCubeAllInOneInstanceData RxGameCubeAllInOneInstanceData;

typedef RpWorldSector* (*RxGCSectorInstanceCallBack)(
    RpWorldSector*, RwResEntry**);
typedef RpWorldSector* (*RxGCSectorLightingCallBack)(
    RpWorldSector*, RwGameCubeLightingData*);
typedef RpWorldSector* (*RxGCSectorRenderCallBack)(
    RpWorldSector*, RxGameCubeAllInOneInstanceData*);

RxPipeline* _rpDlSectorPipelineCreate(
    RwUInt32 pluginId, RwUInt32 pluginData,
    RxGCSectorInstanceCallBack instanceCallback,
    RxGCSectorInstanceCallBack reinstanceCallback,
    RxGCSectorLightingCallBack lightingCallback,
    RxGCSectorRenderCallBack renderCallback);
RpWorldSector* _rxGCSectorDefaultLightingCallback(
    RpWorldSector* sector, RwGameCubeLightingData* lighting);
RpWorldSector* _rxGCSectorDefaultInstanceCallback(
    RpWorldSector* sector, RwResEntry** resourceEntry);
RxNodeDefinition* RxNodeDefinitionGetGameCubeWorldSectorAllInOne(void);

#endif
