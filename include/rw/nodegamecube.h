#ifndef RW_NODEGAMECUBE_H
#define RW_NODEGAMECUBE_H

#include "rw/rpworld_types.h"
#include "rw/rxpipeline.h"
#include "rw/rwresources.h"

typedef struct RwGameCubeLightingData {
    unsigned char reserved_0x00[0x0C];
    RwRGBAReal ambient;
    int hasAmbient;
    unsigned int lightMask;
    int lightIndex;
} RwGameCubeLightingData;
typedef struct RxGameCubeAtomicAllInOneInstanceData {
    RwResEntry* resourceEntry;
    RpMeshHeader* meshHeader;
    int geometryFlags;
    RwRGBAReal ambient;
    int hasAmbient;
    unsigned int lightMask;
    int lightIndex;
    void* morphData;
} RxGameCubeAtomicAllInOneInstanceData;

typedef struct RxGameCubeWorldSectorAllInOneInstanceData {
    RwResEntry* resourceEntry;
    RpMeshHeader* meshHeader;
    unsigned int worldFlags;
    RwRGBAReal ambient;
    int hasAmbient;
    unsigned int lightMask;
    int lightIndex;
} RxGameCubeWorldSectorAllInOneInstanceData;

extern int _RwDlPreInstanceOptimize;

void _rwGCLightsGlobalEnable(int flags,
                             RwGameCubeLightingData* lighting);
void _rwGCLightsLocalEnable(RpLight* light,
                            RwGameCubeLightingData* lighting);

typedef void* (*RxGCAtomicInstanceCallBack)(void*, RwResEntry**);
typedef RpAtomic* (*RxGCAtomicLightingCallBack)(
    RpAtomic*, RwGameCubeLightingData*);
typedef void* (*RxGCAtomicRenderCallBack)(
    void*, RxGameCubeAtomicAllInOneInstanceData*);

typedef RpWorldSector* (*RxGCSectorInstanceCallBack)(
    RpWorldSector*, RwResEntry**);
typedef RpWorldSector* (*RxGCSectorLightingCallBack)(
    RpWorldSector*, RwGameCubeLightingData*);
typedef RpWorldSector* (*RxGCSectorRenderCallBack)(
    RpWorldSector*, RxGameCubeWorldSectorAllInOneInstanceData*);

RxPipeline* _rpDlAtomicPipelineCreate(
    unsigned int pluginId, unsigned int pluginData,
    RxGCAtomicInstanceCallBack instanceCallback,
    RxGCAtomicInstanceCallBack reinstanceCallback,
    RxGCAtomicLightingCallBack lightingCallback,
    RxGCAtomicRenderCallBack renderCallback);
void* _rxGCAtomicDefaultInstanceCallback(void*, RwResEntry**);
void* _rxGCAtomicDefaultReinstanceCallback(void*, RwResEntry**);
RpAtomic* _rxGCAtomicDefaultLightingCallback(
    RpAtomic*, RwGameCubeLightingData*);
void* _rxGCDefaultRenderCallback(
    void*, RxGameCubeAtomicAllInOneInstanceData*);

RxPipeline* _rpDlSectorPipelineCreate(
    unsigned int pluginId, unsigned int pluginData,
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
