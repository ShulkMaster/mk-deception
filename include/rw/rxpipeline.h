#ifndef RW_RXPIPELINE_H
#define RW_RXPIPELINE_H

#include "rw/rwengine.h"

typedef int RwTextureAddressMode;
typedef int RwTextureFilterMode;

#include "rw/rwcore_types.h"
#include "rw/rwplcore.h"

typedef struct RxHeap RxHeap;
typedef struct RxHeapBlock RxHeapBlock;
typedef struct RxHeapFreeBlock RxHeapFreeBlock;
typedef struct RxHeapSuperBlock RxHeapSuperBlock;
typedef struct RxPipeline RxPipeline;
typedef struct RxPipelineNode RxPipelineNode;
typedef struct RxNodeDefinition RxNodeDefinition;
typedef struct RxPipelineNodeTopSortData RxPipelineNodeTopSortData;
typedef struct RxPipelineNodeParam RxPipelineNodeParam;
typedef struct RxClusterDefinition RxClusterDefinition;
typedef struct RxClusterRef RxClusterRef;
typedef struct RxOutputSpec RxOutputSpec;
typedef struct rxReq rxReq;
typedef struct RwFreeList RwFreeList;
typedef struct RxPipelineCluster RxPipelineCluster;
typedef struct RxPipelineRequiresCluster RxPipelineRequiresCluster;
typedef RxPipeline RxLockedPipe;
typedef unsigned int* RxNodeOutput;
typedef RxPipelineNode* RxNodeInput;
typedef struct RxPacket RxPacket;

typedef int RwShadeMode;
typedef int RwBlendFunction;
typedef int RwFogType;

typedef struct RxRenderStateVector {
    unsigned int Flags;
    RwShadeMode ShadeMode;
    RwBlendFunction SrcBlend;
    RwBlendFunction DestBlend;
    RwRaster* TextureRaster;
    RwTextureAddressMode AddressModeU;
    RwTextureAddressMode AddressModeV;
    RwTextureFilterMode FilterMode;
    RwRGBA BorderColor;
    RwFogType FogType;
    RwRGBA FogColor;
} RxRenderStateVector;
typedef char RxRenderStateVectorSizeCheck[
    sizeof(RxRenderStateVector) == 0x2C ? 1 : -1];

typedef int RxEmbeddedPacketState;

typedef struct RxCluster {
    union {
        struct {
            unsigned short flags;
            unsigned short stride;
        };
        unsigned int flagsAndStride;
    };
    void* data;
    void* currentData;
    unsigned int numAlloced;
    unsigned int numUsed;
    RxPipelineCluster* clusterRef;
    unsigned int attributes;
} RxCluster;
typedef char RxClusterSizeCheck[sizeof(RxCluster) == 0x1C ? 1 : -1];

typedef struct RxPipeline {
    int locked;
    unsigned int numNodes;
    RxPipelineNode* nodes;
    unsigned int packetNumClusterSlots;
    RxEmbeddedPacketState embeddedPacketState;
    RxPacket* embeddedPacket;
    unsigned int numInputRequirements;
    RxPipelineRequiresCluster* inputRequirements;
    void* superBlock;
    unsigned int superBlockSize;
    unsigned int entryPoint;
    unsigned int pluginId;
    unsigned int pluginData;
} RxPipeline;
typedef char RxPipelineSizeCheck[sizeof(RxPipeline) == 0x34 ? 1 : -1];

typedef int RxClusterValidityReq;
typedef int RxClusterValid;

struct RxClusterDefinition {
    char* name;
    unsigned int defaultStride;
    unsigned int defaultAttributes;
    const char* attributeSet;
};
typedef char RxClusterDefinitionSizeCheck[
    sizeof(RxClusterDefinition) == 0x10 ? 1 : -1];

struct RxOutputSpec {
    char* name;
    RxClusterValid* outputClusters;
    RxClusterValid allOtherClusters;
};
typedef char RxOutputSpecSizeCheck[sizeof(RxOutputSpec) == 0x0C ? 1 : -1];

struct RxClusterRef {
    RxClusterDefinition* clusterDef;
    int forcePresent;
    unsigned int reserved;
};
typedef char RxClusterRefSizeCheck[sizeof(RxClusterRef) == 0x0C ? 1 : -1];

struct RxPipelineCluster {
    RxClusterDefinition* clusterRef;
    unsigned int creationAttributes;
};
typedef char RxPipelineClusterSizeCheck[
    sizeof(RxPipelineCluster) == 0x08 ? 1 : -1];

struct RxPipelineRequiresCluster {
    RxClusterDefinition* clusterDef;
    RxClusterValidityReq rqdOrOpt;
    unsigned int slotIndex;
};
typedef char RxPipelineRequiresClusterSizeCheck[
    sizeof(RxPipelineRequiresCluster) == 0x0C ? 1 : -1];

typedef struct RxIoSpec {
    unsigned int numClustersOfInterest;
    RxClusterRef* clustersOfInterest;
    RxClusterValidityReq* inputRequirements;
    unsigned int numOutputs;
    RxOutputSpec* outputs;
} RxIoSpec;
typedef char RxIoSpecSizeCheck[sizeof(RxIoSpec) == 0x14 ? 1 : -1];

typedef int (*RxNodeBodyFn)(RxPipelineNode*, const RxPipelineNodeParam*);
typedef int (*RxNodeInitFn)(RxNodeDefinition*);
typedef void (*RxNodeTermFn)(RxNodeDefinition*);
typedef int (*RxPipelineNodeInitFn)(RxPipelineNode*);
typedef void (*RxPipelineNodeTermFn)(RxPipelineNode*);
typedef int (*RxPipelineNodeConfigFn)(RxPipelineNode*, RxPipeline*);
typedef unsigned int (*RxConfigMsgHandlerFn)(RxPipelineNode*, unsigned int,
                                         unsigned int, void*);

typedef struct RxNodeMethods {
    RxNodeBodyFn nodeBody;
    RxNodeInitFn nodeInit;
    RxNodeTermFn nodeTerm;
    RxPipelineNodeInitFn pipelineNodeInit;
    RxPipelineNodeTermFn pipelineNodeTerm;
    RxPipelineNodeConfigFn pipelineNodeConfig;
    RxConfigMsgHandlerFn configMsgHandler;
} RxNodeMethods;
typedef char RxNodeMethodsSizeCheck[sizeof(RxNodeMethods) == 0x1C ? 1 : -1];

struct RxNodeDefinition {
    char* name;
    RxNodeMethods nodeMethods;
    RxIoSpec io;
    unsigned int pipelineNodePrivateDataSize;
    int editable;
    int InputPipesCnt;
};
typedef char RxNodeDefinitionSizeCheck[
    sizeof(RxNodeDefinition) == 0x40 ? 1 : -1];

struct RxPipelineNode {
    RxNodeDefinition* nodeDef;
    unsigned int numOutputs;
    unsigned int* outputs;
    RxPipelineCluster** slotClusterRefs;
    unsigned int* slotsContinue;
    void* privateData;
    unsigned int* inputToClusterSlot;
    RxPipelineNodeTopSortData* topSortData;
    void* initializationData;
    unsigned int initializationDataSize;
};
typedef char RxPipelineNodeSizeCheck[
    sizeof(RxPipelineNode) == 0x28 ? 1 : -1];

struct RxPipelineNodeTopSortData {
    unsigned int numIns;
    unsigned int numInsVisited;
    rxReq* req;
};
typedef char RxPipelineNodeTopSortDataSizeCheck[
    sizeof(RxPipelineNodeTopSortData) == 0x0C ? 1 : -1];

struct RxPipelineNodeParam {
    void* dataParam;
    RxHeap* heap;
};
typedef char RxPipelineNodeParamSizeCheck[
    sizeof(RxPipelineNodeParam) == 0x08 ? 1 : -1];

struct RxPacket {
    unsigned short flags;
    unsigned short numClusters;
    RxPipeline* pipeline;
    unsigned int* inputToClusterSlot;
    unsigned int* slotsContinue;
    RxPipelineCluster** slotClusterRefs;
    RxCluster clusters[1];
};
typedef char RxPacketSizeCheck[sizeof(RxPacket) == 0x30 ? 1 : -1];

typedef struct RxPipelinePlatformGlobals {
    RwFreeList* pipesFreeList;
    RxRenderStateVector defaultRenderState;
    RwLinkList allPipelines;
    unsigned int maxNodesPerPipe;
    RxPipeline* currentAtomicPipeline;
    RxPipeline* currentWorldSectorPipeline;
    RxPipeline* currentMaterialPipeline;
    RxPipeline* genericAtomicPipeline;
    RxPipeline* genericWorldSectorPipeline;
    RxPipeline* genericMaterialPipeline;
    RxPipeline* platformAtomicPipeline;
    RxPipeline* platformWorldSectorPipeline;
    RxPipeline* platformMaterialPipeline;
} RxPipelinePlatformGlobals;
typedef char RxPipelinePlatformGlobalsSizeCheck[
    sizeof(RxPipelinePlatformGlobals) == 0x60 ? 1 : -1];

typedef struct RxExecutionContext {
    RxPipeline* pipeline;
    RxPipelineNode* currentNode;
    int exitCode;
    unsigned int pad; /* +0x0C: canonical alignment padding. */
    RxPipelineNodeParam params;
} RxExecutionContext;
typedef char RxExecutionContextSizeCheck[
    sizeof(RxExecutionContext) == 0x18 ? 1 : -1];

struct RxHeapFreeBlock {
    unsigned int size;
    RxHeapBlock* block;
};
typedef char RxHeapFreeBlockSizeCheck[
    sizeof(RxHeapFreeBlock) == 0x08 ? 1 : -1];

struct RxHeapBlock {
    RxHeapBlock* prev;
    RxHeapBlock* next;
    unsigned int size;
    RxHeapFreeBlock* freeEntry;
    /* +0x10..+0x1F: unused header extent; keeps payload 0x20-byte aligned. */
    unsigned int padding[4];
};
typedef char RxHeapBlockSizeCheck[sizeof(RxHeapBlock) == 0x20 ? 1 : -1];

struct RxHeapSuperBlock {
    RxHeapBlock* start;
    unsigned int size;
    RxHeapSuperBlock* next;
};
typedef char RxHeapSuperBlockSizeCheck[
    sizeof(RxHeapSuperBlock) == 0x0C ? 1 : -1];

struct RxHeap {
    unsigned int superBlockSize;
    RxHeapSuperBlock* firstSuperBlock;
    RxHeapBlock* firstBlock;
    RxHeapFreeBlock* freeBlocks;
    unsigned int freeBlocksAllocated;
    unsigned int freeBlocksUsed;
    int dirty;
};
typedef char RxHeapSizeCheck[sizeof(RxHeap) == 0x1C ? 1 : -1];

extern int _rxPipelineGlobalsOffset;
extern RxExecutionContext _rxExecCtxGlobal;
extern RxHeap* _rxHeapGlobal;

#define RXPIPELINEGLOBAL(field)                                                \
    (((RxPipelinePlatformGlobals*)((unsigned char*)RwEngineInstance +          \
                                   _rxPipelineGlobalsOffset))->field)

static inline RxPipelinePlatformGlobals* RxPipelineGlobals(void)
{
    return (RxPipelinePlatformGlobals*)((unsigned char*)RwEngineInstance +
                                        _rxPipelineGlobalsOffset);
}

void _rxPacketDestroy(RxPacket* packet);
int _rxPipelineOpen(void);
int _rxPipelineClose(void);
RxPipeline* RpWorldSetDefaultSectorPipeline(RxPipeline* pipeline);
RxPipeline* RpAtomicSetDefaultPipeline(RxPipeline* pipeline);
int _rpWorldPipelineOpen(void);
void _rpWorldPipelineClose(void);
int _rpWorldPipeAttach(void);
RxPipeline* RxPipelineExecute(RxPipeline* pipeline, void* data,
                              int heapReset);
RxHeap* RxHeapGetGlobalHeap(void);
RxPipeline* RxPipelineCreate(void);
void _rxPipelineDestroy(RxPipeline* pipeline);
RxPipeline* RxLockedPipeUnlock(RxLockedPipe* pipeline);
RxLockedPipe* RxPipelineLock(RxPipeline* pipeline);
RxPipelineNode* RxPipelineFindNodeByName(RxPipeline* pipeline,
                                         const char* name,
                                         RxPipelineNode* start,
                                         int* nodeIndex);
RxLockedPipe* RxLockedPipeAddFragment(RxLockedPipe* pipeline,
                                      unsigned int* firstIndex,
                                      RxNodeDefinition* nodeDef0, ...);
RxPipeline* RxLockedPipeAddPath(RxLockedPipe* pipeline, RxNodeOutput output,
                                RxNodeInput input);
RxNodeOutput RxPipelineNodeFindOutputByIndex(RxPipelineNode* node,
                                             unsigned int outputIndex);
RxNodeInput RxPipelineNodeFindInput(RxPipelineNode* node);
void* StalacTiteAlloc(int size);
void* StalacMiteAlloc(int size);
unsigned int PipelineCalcNumUniqueClusters(RxPipeline* pipeline);
void RxHeapFree(RxHeap* heap, void* block);
int _rxHeapReset(RxHeap* heap);
#define RxHeapReset(heap) ((!(heap)->dirty) ? 1 : _rxHeapReset(heap))
void RxHeapDestroy(RxHeap* heap);
RxHeap* RxHeapCreate(unsigned int size);
RxRenderStateVector* RxRenderStateVectorSetDefaultRenderStateVector(
    RxRenderStateVector* renderState);
void _rx_rxRadixExchangeSort(unsigned char* base, unsigned int numEntries,
                             unsigned int entrySize, unsigned int keyOffset,
                             unsigned int keyLowerBound,
                             unsigned int keyUpperBound);
void* _rwRenderPipelineOpen(void* instance, int offset, int size);
void* _rwRenderPipelineClose(void* instance, int offset, int size);
int _rwPipeAttach(void);
int _rxWorldDevicePluginAttach(void);

#endif
