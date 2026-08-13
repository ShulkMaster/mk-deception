#ifndef RW_RXPIPELINE_H
#define RW_RXPIPELINE_H

#include "libmkparticle/rw_engine.h"

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

typedef int RxClusterValidityReq;
typedef int RxClusterValid;

struct RxClusterDefinition {
    char* name;
    unsigned int defaultStride;
    unsigned int defaultAttributes;
    const char* attributeSet;
};

struct RxOutputSpec {
    char* name;
    RxClusterValid* outputClusters;
    RxClusterValid allOtherClusters;
};

struct RxClusterRef {
    RxClusterDefinition* clusterDef;
    int forcePresent;
    unsigned int reserved;
};

struct RxPipelineCluster {
    RxClusterDefinition* clusterRef;
    unsigned int creationAttributes;
};

struct RxPipelineRequiresCluster {
    RxClusterDefinition* clusterDef;
    RxClusterValidityReq rqdOrOpt;
    unsigned int slotIndex;
};

typedef struct RxIoSpec {
    unsigned int numClustersOfInterest;
    RxClusterRef* clustersOfInterest;
    RxClusterValidityReq* inputRequirements;
    unsigned int numOutputs;
    RxOutputSpec* outputs;
} RxIoSpec;

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

struct RxNodeDefinition {
    char* name;
    RxNodeMethods nodeMethods;
    RxIoSpec io;
    unsigned int pipelineNodePrivateDataSize;
    int editable;
    int InputPipesCnt;
};

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

struct RxPipelineNodeTopSortData {
    unsigned int numIns;
    unsigned int numInsVisited;
    rxReq* req;
};

struct RxPipelineNodeParam {
    void* dataParam;
    RxHeap* heap;
};

struct RxPacket {
    unsigned short flags;
    unsigned short numClusters;
    RxPipeline* pipeline;
    unsigned int* inputToClusterSlot;
    unsigned int* slotsContinue;
    RxPipelineCluster** slotClusterRefs;
    RxCluster clusters[1];
};

typedef struct RxPipelinePlatformGlobals {
    RwFreeList* pipelines;
    RxRenderStateVector defaultRenderState;
    RxPipeline* currentPipeline;
    RxPipelineNode* currentNode;
    unsigned int maxNodes;
    RxPipeline* defaultAtomicPipeline;
    RxPipeline* defaultWorldSectorPipeline;
    RxPipeline* defaultMaterialPipeline;
    RxPipeline* field_0x48;
    RxPipeline* field_0x4C;
    RxPipeline* field_0x50;
    RxPipeline* platformAtomicPipeline;
    RxPipeline* platformWorldSectorPipeline;
    RxPipeline* platformMaterialPipeline;
} RxPipelinePlatformGlobals;

typedef struct RxExecutionContext {
    RxPipeline* pipeline;
    unsigned int field04;
    int executionStatus;
    unsigned int field0C;
    RxPipelineNodeParam params;
} RxExecutionContext;

struct RxHeapFreeBlock {
    unsigned int size;
    RxHeapBlock* block;
};

struct RxHeapBlock {
    RxHeapBlock* prev;
    RxHeapBlock* next;
    unsigned int size;
    RxHeapFreeBlock* freeEntry;
    unsigned int bookkeeping[4];
};

struct RxHeapSuperBlock {
    RxHeapBlock* start;
    unsigned int size;
    RxHeapSuperBlock* next;
};

struct RxHeap {
    unsigned int superBlockSize;
    RxHeapSuperBlock* firstSuperBlock;
    RxHeapBlock* firstBlock;
    RxHeapFreeBlock* freeBlocks;
    unsigned int freeBlocksAllocated;
    unsigned int freeBlocksUsed;
    int dirty;
};

extern int _rxPipelineGlobalsOffset;

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
int _rxWorldDevicePluginAttach(void);

#endif
