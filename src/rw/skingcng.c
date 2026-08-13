#include "rw/nodegamecube.h"
#include "rw/rpskin.h"
#include "rw/rpworld_types.h"
#include "rw/rxpipeline.h"

int _rpSkinPipelinesCreate(unsigned int pipeType)
{
    RxPipeline** pipelines = _rpSkinGlobals.pipelines;

    if ((pipeType & rpSKINTYPEGENERIC) != 0) {
        pipelines[rpSKINTYPEGENERIC] =
            _rpDlAtomicPipelineCreate(
                0x116, 1, _rpSkinInstanceCallback,
                _rpSkinAtomicReinstanceCallBack,
                _rxGCAtomicDefaultLightingCallback, _rpSkinRenderCallback);
    }
    return 1;
}

int _rpSkinPipelinesDestroy(void)
{
    RxPipeline** pipelines = _rpSkinGlobals.pipelines;

    if (pipelines[rpSKINTYPEGENERIC] != 0) {
        _rxPipelineDestroy(pipelines[rpSKINTYPEGENERIC]);
        pipelines[rpSKINTYPEGENERIC] = 0;
    }
    return 1;
}


RpAtomic* _rpSkinPipelinesAttach(RpAtomic* atomic, RpSkinType skinType)
{
    RpSkin* skin;
    RxPipeline* pipeline;

    (void)skinType;
    pipeline = _rpSkinGlobals.pipelines[rpSKINTYPEGENERIC];
    atomic->pipeline = pipeline;
    skin = RpSkinGeometryGetSkin(atomic->geometry);
    skin->platformData = 1;
    return atomic;
}
