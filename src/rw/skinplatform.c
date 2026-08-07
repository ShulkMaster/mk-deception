typedef struct RxPipeline RxPipeline;

typedef struct RpSkinGlobals {
    unsigned char platformIndependent[0x24];
    RxPipeline* pipelines[6];
} RpSkinGlobals;

extern RpSkinGlobals _rpSkinGlobals;

RxPipeline* RpSkinGetGameCubePipeline(int skinType) {
    RxPipeline* pipeline = _rpSkinGlobals.pipelines[skinType];
    return pipeline;
}
