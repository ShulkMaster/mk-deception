#ifndef RW_RPSKIN_H
#define RW_RPSKIN_H

#include "rw/rwplcore.h"

typedef struct RpAtomic RpAtomic;
typedef struct RpGeometry RpGeometry;
typedef struct RpHAnimHierarchy RpHAnimHierarchy;
typedef struct RpSkin RpSkin;
typedef struct RwMatrix RwMatrix;
typedef struct RxPipeline RxPipeline;

typedef struct RwMatrixWeights {
    RwReal w0;
    RwReal w1;
    RwReal w2;
    RwReal w3;
} RwMatrixWeights;

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
    RwUInt32 numUsedBones;
    RwUInt8* usedBoneList;
    RwMatrix* skinToBoneMatrices;
    RwUInt32 maxNumWeights;
    RwUInt32* vertexBoneIndices;
    RwMatrixWeights* vertexBoneWeights;
    void* nativeData;
    void* nativeData2;
    void* platformWeights;
    void* platformIndices;
    RwUInt32 platformData;
    RpSkinSplitData splitData;
    void* skinData;
};

typedef struct RpSkinGlobals {
    RwInt32 engineOffset;
    RwInt32 atomicOffset;
    RwInt32 geometryOffset;
    void* alignedScratchMemory;
    void* scratchMemory;
    RwUInt32 reserved_0x14;
    void* skinFreeList;
    RwUInt32 reserved_0x1C;
    RwInt32 numInstances;
    RxPipeline* pipelines[6];
} RpSkinGlobals;

extern RpSkinGlobals _rpSkinGlobals;

typedef enum RpSkinType {
    rpSKINTYPEGENERIC = 1,
    rpSKINTYPEMATFX = 2,
    rpSKINTYPETOON = 3
} RpSkinType;

RwBool RpSkinPluginAttach(void);
RpAtomic* RpSkinAtomicSetHAnimHierarchy(RpAtomic* atomic,
                                        RpHAnimHierarchy* hierarchy);
RpSkin* RpSkinGeometryGetSkin(RpGeometry* geometry);
RpGeometry* RpSkinGeometrySetSkin(RpGeometry* geometry, RpSkin* skin);
RpSkin* RpSkinDestroy(RpSkin* skin);
RwMatrix* RpSkinGetSkinToBoneMatrices(RpSkin* skin);
RpAtomic* RpSkinAtomicSetType(RpAtomic* atomic, RpSkinType type);
RxPipeline* RpSkinGetGameCubePipeline(RpSkinType type);
RpSkin* _rpSkinSplitDataCreate(RpSkin* skin, RwUInt32 boneLimit,
                               RwUInt32 numBones, RwUInt32 numMeshes,
                               RwUInt32 rleSize);
RwBool _rpSkinSplitDataDestroy(RpSkin* skin);
RwStream* _rpSkinSplitDataStreamWrite(RwStream* stream, const RpSkin* skin);
RwStream* _rpSkinSplitDataStreamRead(RwStream* stream, RpSkin* skin);
RwUInt32 _rpSkinSplitDataStreamGetSize(const RpSkin* skin);
RwUInt32 _rpSkinGeometryNativeSize(const RpGeometry* geometry);
RwStream* _rpSkinGeometryNativeWrite(RwStream* stream,
                                     const RpGeometry* geometry);
RwStream* _rpSkinGeometryNativeRead(RwStream* stream, RpGeometry* geometry);
RwUInt32 _rpSkinAtomicNativeSize(const RpAtomic* atomic);
void _rpSkinMatrixBlendUpdateASM(RwMatrix* destination,
                                 const RwMatrix* skinToBone,
                                 const RwMatrix* hierarchyMatrices,
                                 const RwMatrix* transform,
                                 const RwUInt8* usedBoneList,
                                 RwUInt32 numUsedBones);
RwBool _rpSkinPipelinesCreate(RwUInt32 pipeType);
RwBool _rpSkinPipelinesDestroy(void);
RpAtomic* _rpSkinPipelinesAttach(RpAtomic* atomic, RpSkinType skinType);

#endif
