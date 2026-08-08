#include "rw/rpskin.h"
#include "rw/rpworld_types.h"
#include "rw/rxpipeline.h"

typedef struct RxGCAtomicResourceEntry RxGCAtomicResourceEntry;
typedef struct RxGCAtomicLightingData RxGCAtomicLightingData;

typedef RpAtomic* (*RxGCInstanceCallback)(RpAtomic*,
                                          RxGCAtomicResourceEntry*);
typedef RpAtomic* (*RxGCLightingCallback)(RpAtomic*,
                                          RxGCAtomicLightingData*);
typedef RpAtomic* (*RxGCRenderCallback)(RpAtomic*,
                                        RxGCAtomicResourceEntry*);

extern RxPipeline* _rpDlAtomicPipelineCreate(
    RwUInt32 pluginId, RwUInt32 pluginData,
    RxGCInstanceCallback instanceCallback,
    RxGCInstanceCallback reinstanceCallback,
    RxGCLightingCallback lightingCallback, RxGCRenderCallback renderCallback);
extern RpAtomic* _rpSkinInstanceCallback(RpAtomic*, RxGCAtomicResourceEntry*);
extern RpAtomic* _rpSkinAtomicReinstanceCallBack(
    RpAtomic*, RxGCAtomicResourceEntry*);
extern RpAtomic* _rxGCAtomicDefaultLightingCallback(
    RpAtomic*, RxGCAtomicLightingData*);
extern RpAtomic* _rpSkinRenderCallback(RpAtomic*, RxGCAtomicResourceEntry*);

#define RpAtomicSetPipeline(atomic, pipe) \
    (((atomic)->pipeline = (pipe)), (atomic))
#define RpAtomicGetGeometry(atomic) ((atomic)->geometry)
#define RxPipelineDestroy(pipe) (_rxPipelineDestroy(pipe), TRUE)

RwBool _rpSkinPipelinesCreate(RwUInt32 pipeType)
{
    RxPipeline** pipelines = _rpSkinGlobals.pipelines;

    if ((pipeType & rpSKINTYPEGENERIC) != 0) {
        pipelines[rpSKINTYPEGENERIC] =
            _rpDlAtomicPipelineCreate(
                0x116, 1, _rpSkinInstanceCallback,
                _rpSkinAtomicReinstanceCallBack,
                _rxGCAtomicDefaultLightingCallback, _rpSkinRenderCallback);
    }
    return TRUE;
}

RwBool _rpSkinPipelinesDestroy(void)
{
    RxPipeline** pipelines = _rpSkinGlobals.pipelines;

    if (pipelines[rpSKINTYPEGENERIC] != 0) {
        RxPipelineDestroy(pipelines[rpSKINTYPEGENERIC]);
        pipelines[rpSKINTYPEGENERIC] = 0;
    }
    return TRUE;
}

/* Near miss: retail selects save/restore helpers for the same body while this
 * clean two-argument definition spills the unused skin type. */
RpAtomic* _rpSkinPipelinesAttach(RpAtomic* atomic, RpSkinType skinType)
{
    RpSkin* skin;
    RxPipeline* pipeline;

    (void)skinType;
    pipeline = _rpSkinGlobals.pipelines[rpSKINTYPEGENERIC];
    atomic = RpAtomicSetPipeline(atomic, pipeline);
    skin = RpSkinGeometryGetSkin(RpAtomicGetGeometry(atomic));
    skin->field_0x2C = 1;
    return atomic;
}
