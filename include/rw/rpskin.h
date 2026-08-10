#ifndef RW_RPSKIN_H
#define RW_RPSKIN_H

#include "rw/rwplcore.h"

typedef struct RpAtomic RpAtomic;
typedef struct RpGeometry RpGeometry;
typedef struct RpSkin RpSkin;
typedef struct RwMatrix RwMatrix;

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
    RwUInt8 field_04[0x2C];
    RpSkinSplitData splitData;
};

RpAtomic* RpSkinAtomicSetHAnimHierarchy(RpAtomic* atomic, void* hierarchy);
RpSkin* RpSkinGeometryGetSkin(RpGeometry* geometry);
RwMatrix* RpSkinGetSkinToBoneMatrices(RpSkin* skin);
RpAtomic* RpSkinAtomicSetType(RpAtomic* atomic, int type);
RpSkin* _rpSkinSplitDataCreate(RpSkin* skin, RwUInt32 boneLimit,
                               RwUInt32 numBones, RwUInt32 numMeshes,
                               RwUInt32 rleSize);
RwBool _rpSkinSplitDataDestroy(RpSkin* skin);
RwStream* _rpSkinSplitDataStreamWrite(RwStream* stream, const RpSkin* skin);
RwStream* _rpSkinSplitDataStreamRead(RwStream* stream, RpSkin* skin);
RwUInt32 _rpSkinSplitDataStreamGetSize(const RpSkin* skin);

#endif
