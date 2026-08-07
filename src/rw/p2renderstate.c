#include "libmkparticle/rw_engine.h"
#include "rw/rwerror.h"
#include "rw/rxpipeline.h"

enum {
    rwENGINESTATUSSTARTED = 3
};

RxRenderStateVector* RxRenderStateVectorSetDefaultRenderStateVector(
    RxRenderStateVector* renderState) {
    if (renderState != NULL) {
        if (RwEngineInstance->engineStatus == rwENGINESTATUSSTARTED) {
            *renderState = RXPIPELINEDEFAULTRENDERSTATE;
        } else {
            if (renderState != &RXPIPELINEDEFAULTRENDERSTATE) {
                RwError error;
                error.pluginID = 1;
                error.errorCode = _rwerror(0x80000018);
                RwErrorSet(&error);
                return NULL;
            }
            {
                RwRGBA white = { 0xFF, 0xFF, 0xFF, 0xFF };
                renderState->Flags = 7;
                renderState->ShadeMode = rwSHADEMODEGOURAUD;
                renderState->SrcBlend = rwBLENDSRCALPHA;
                renderState->DestBlend = rwBLENDINVSRCALPHA;
                renderState->TextureRaster = NULL;
                renderState->AddressModeU = rwTEXTUREADDRESSWRAP;
                renderState->AddressModeV = rwTEXTUREADDRESSWRAP;
                renderState->FilterMode = rwFILTERLINEAR;
                renderState->BorderColor = white;
                renderState->FogType = rwFOGTYPENAFOGTYPE;
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
    return NULL;
}
