#ifndef RW_RPSKIN_H
#define RW_RPSKIN_H

#include "rw/rwplcore.h"

typedef struct RpAtomic RpAtomic;
typedef struct RpGeometry RpGeometry;
typedef struct RpHAnimHierarchy RpHAnimHierarchy;
typedef struct RpSkin RpSkin;
typedef struct RwMatrix RwMatrix;
typedef struct RwResEntry RwResEntry;
typedef struct RxGameCubeAtomicAllInOneInstanceData
    RxGameCubeAtomicAllInOneInstanceData;
typedef struct RxPipeline RxPipeline;

typedef struct RwMatrixWeights {
    float w0;
    float w1;
    float w2;
    float w3;
} RwMatrixWeights;

typedef struct RpSkinBlendPositionData {
    unsigned char* destination;
    unsigned char* source;
    unsigned int stride;
    unsigned int numVertices;
} RpSkinBlendPositionData;

typedef struct RpSkinBlendPositionNormalData {
    unsigned char* destinationPositions;
    unsigned char* destinationNormals;
    unsigned char* sourcePositions;
    unsigned char* sourceNormals;
    unsigned int positionStride;
    unsigned int normalStride;
    unsigned int nbtStride;
    unsigned int numVertices;
} RpSkinBlendPositionNormalData;

typedef struct SkinAtomicState {
    RpHAnimHierarchy* hierarchy;
    void* positions;
    void* normals;
} SkinAtomicState;

typedef struct RpSkinRLECount {
    unsigned char start;
    unsigned char size;
} RpSkinRLECount;

typedef struct RpSkinRLE {
    unsigned char startBone;
    unsigned char count;
} RpSkinRLE;

typedef struct RpSkinSplitData {
    unsigned int boneLimit;
    unsigned int numMeshes;
    unsigned int rleSize;
    unsigned char* remapIndices;
    RpSkinRLECount* rleCount;
    RpSkinRLE* rle;
} RpSkinSplitData;

struct RpSkin {
    unsigned int numBones;
    unsigned int numUsedBones;
    unsigned char* usedBoneList;
    RwMatrix* skinToBoneMatrices;
    unsigned int maxNumWeights;
    unsigned int* vertexBoneIndices;
    RwMatrixWeights* vertexBoneWeights;
    void* nativeData;
    void* nativeData2;
    void* platformWeights;
    void* platformIndices;
    unsigned int platformData;
    RpSkinSplitData splitData;
    void* skinData;
};

typedef struct RpSkinGlobals {
    int engineOffset;
    int atomicOffset;
    int geometryOffset;
    void* alignedScratchMemory;
    void* scratchMemory;
    unsigned int reserved_0x14;
    void* skinFreeList;
    unsigned int reserved_0x1C;
    int numInstances;
    RxPipeline* pipelines[6];
} RpSkinGlobals;

extern RpSkinGlobals _rpSkinGlobals;

void* _rpSkinInstanceCallback(void* object, RwResEntry** resourceEntry);
void* _rpSkinAtomicReinstanceCallBack(void* object,
                                      RwResEntry** resourceEntry);
void* _rpSkinRenderCallback(
    void* object, RxGameCubeAtomicAllInOneInstanceData* instanceData);

typedef enum RpSkinType {
    rpSKINTYPEGENERIC = 1,
    rpSKINTYPEMATFX = 2,
    rpSKINTYPETOON = 3
} RpSkinType;

int RpSkinPluginAttach(void);
RpAtomic* RpSkinAtomicSetHAnimHierarchy(RpAtomic* atomic,
                                        RpHAnimHierarchy* hierarchy);
RpSkin* RpSkinGeometryGetSkin(RpGeometry* geometry);
RpGeometry* RpSkinGeometrySetSkin(RpGeometry* geometry, RpSkin* skin);
RpSkin* RpSkinDestroy(RpSkin* skin);
RwMatrix* RpSkinGetSkinToBoneMatrices(RpSkin* skin);
RpAtomic* RpSkinAtomicSetType(RpAtomic* atomic, RpSkinType type);
RxPipeline* RpSkinGetGameCubePipeline(RpSkinType type);
RpSkin* _rpSkinSplitDataCreate(RpSkin* skin, unsigned int boneLimit,
                               unsigned int numBones, unsigned int numMeshes,
                               unsigned int rleSize);
int _rpSkinSplitDataDestroy(RpSkin* skin);
RwStream* _rpSkinSplitDataStreamWrite(RwStream* stream, const RpSkin* skin);
RwStream* _rpSkinSplitDataStreamRead(RwStream* stream, RpSkin* skin);
unsigned int _rpSkinSplitDataStreamGetSize(const RpSkin* skin);
unsigned int _rpSkinGeometryNativeSize(const RpGeometry* geometry);
RwStream* _rpSkinGeometryNativeWrite(RwStream* stream,
                                     const RpGeometry* geometry);
RwStream* _rpSkinGeometryNativeRead(RwStream* stream, RpGeometry* geometry);
unsigned int _rpSkinAtomicNativeSize(const RpAtomic* atomic);
void _rpSkinMatrixBlendUpdateASM(RwMatrix* destination,
                                 const RwMatrix* skinToBone,
                                 const RwMatrix* hierarchyMatrices,
                                 const RwMatrix* transform,
                                 const unsigned char* usedBoneList,
                                 unsigned int numUsedBones);
void _rwDlSkinUpdate2WeightsP(const RwMatrix* matrices, const RpSkin* skin,
                              const RpSkinBlendPositionData* data);
void _rwDlSkinUpdate3WeightsP(const RwMatrix* matrices, const RpSkin* skin,
                              const RpSkinBlendPositionData* data);
void _rwDlSkinUpdate4WeightsP(const RwMatrix* matrices, const RpSkin* skin,
                              const RpSkinBlendPositionData* data);
void _rwDlSkinUpdate2WeightsPN(
    const RwMatrix* matrices, const RpSkin* skin,
    const RpSkinBlendPositionNormalData* data);
void _rwDlSkinUpdate3WeightsPN(
    const RwMatrix* matrices, const RpSkin* skin,
    const RpSkinBlendPositionNormalData* data);
void _rwDlSkinUpdate4WeightsPN(
    const RwMatrix* matrices, const RpSkin* skin,
    const RpSkinBlendPositionNormalData* data);
int _rpSkinPipelinesCreate(unsigned int pipeType);
int _rpSkinPipelinesDestroy(void);
RpAtomic* _rpSkinPipelinesAttach(RpAtomic* atomic, RpSkinType skinType);

#endif
