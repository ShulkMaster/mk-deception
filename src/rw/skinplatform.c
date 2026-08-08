#include "rw/rpskin.h"

RxPipeline* RpSkinGetGameCubePipeline(RpSkinType skinType) {
    RxPipeline* pipeline = _rpSkinGlobals.pipelines[skinType];
    return pipeline;
}
