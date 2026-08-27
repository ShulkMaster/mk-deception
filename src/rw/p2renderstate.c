#include "libmkparticle/rw_engine.h"
#include "rw/rwerror.h"
#include "rw/rxpipeline.h"

/* Retail open-codes this plugin offset in the authentic -inline off TU. */
#define RX_PIPELINE_GLOBALS()                                                \
    ((RxPipelinePlatformGlobals*)((unsigned char*)RwEngineInstance +         \
                                  _rxPipelineGlobalsOffset))

RxRenderStateVector* RxRenderStateVectorSetDefaultRenderStateVector(
    RxRenderStateVector* renderState) {
    if (renderState != 0) {
        if (RwEngineInstance->engineStatus == 3) {
            *renderState = RX_PIPELINE_GLOBALS()->defaultRenderState;
        } else {
            if (renderState != &RX_PIPELINE_GLOBALS()->defaultRenderState) {
                RwError error;
                error.pluginID = 1;
                error.errorCode = _rwerror(0x80000018);
                RwErrorSet(&error);
                return 0;
            }
            {
                RwRGBA white = { 0xFF, 0xFF, 0xFF, 0xFF };
                renderState->Flags = 7;
                renderState->ShadeMode = 2;
                renderState->SrcBlend = 5;
                renderState->DestBlend = 6;
                renderState->TextureRaster = 0;
                renderState->AddressModeU = 1;
                renderState->AddressModeV = 1;
                renderState->FilterMode = 2;
                renderState->BorderColor = white;
                renderState->FogType = 0;
                renderState->FogColor = white;
            }
        }
        return renderState;
    }
    {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x80000016);
        RwErrorSet(&error);
    }
    return 0;
}

#undef RX_PIPELINE_GLOBALS
