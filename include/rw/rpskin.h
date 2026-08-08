#ifndef RW_RPSKIN_H
#define RW_RPSKIN_H

#include "rw/rwplcore.h"

typedef struct RpAtomic RpAtomic;
typedef struct RpGeometry RpGeometry;
typedef struct RpSkin RpSkin;
typedef struct RwMatrix RwMatrix;
typedef struct RxPipeline RxPipeline;

typedef struct RpSkinRLECount {
    RwUInt8 start;
    RwUInt8 size;
} RpSkinRLECount;

typedef struct RpSkinRLE {
    RwUInt8 startBone;
    RwUInt8 count;
} RpSkinRLE;

typedef struct RpSkinSplitData {
    RwUInt32 boneLimit;
    RwUInt32 numMeshes;
    RwUInt32 rleSize;
    RwUInt8* remapIndices;
    RpSkinRLECount* rleCount;
    RpSkinRLE* rle;
} RpSkinSplitData;

struct RpSkin {
    RwUInt32 numBones;
    RwUInt8 field04[0x28];
    RwUInt32 field_0x2C;
    RpSkinSplitData splitData;
};

typedef struct RpSkinGlobals {
    RwUInt8 platformIndependent[0x24];
    RxPipeline* pipelines[6];
} RpSkinGlobals;

extern RpSkinGlobals _rpSkinGlobals;

typedef enum RpSkinType {
    rpSKINTYPEGENERIC = 1,
    rpSKINTYPEMATFX = 2,
    rpSKINTYPETOON = 3
} RpSkinType;

RpAtomic* RpSkinAtomicSetHAnimHierarchy(RpAtomic* atomic, void* hierarchy);
RpSkin* RpSkinGeometryGetSkin(RpGeometry* geometry);
RwMatrix* RpSkinGetSkinToBoneMatrices(RpSkin* skin);
RpAtomic* RpSkinAtomicSetType(RpAtomic* atomic, int type);
RxPipeline* RpSkinGetGameCubePipeline(RpSkinType type);
RpSkin* _rpSkinSplitDataCreate(RpSkin* skin, RwUInt32 boneLimit,
                               RwUInt32 numBones, RwUInt32 numMeshes,
                               RwUInt32 rleSize);
RwBool _rpSkinSplitDataDestroy(RpSkin* skin);
RwStream* _rpSkinSplitDataStreamWrite(RwStream* stream, const RpSkin* skin);
RwStream* _rpSkinSplitDataStreamRead(RwStream* stream, RpSkin* skin);
RwUInt32 _rpSkinSplitDataStreamGetSize(const RpSkin* skin);
RwBool _rpSkinPipelinesCreate(RwUInt32 pipeType);
RwBool _rpSkinPipelinesDestroy(void);
RpAtomic* _rpSkinPipelinesAttach(RpAtomic* atomic, RpSkinType skinType);

#endif
