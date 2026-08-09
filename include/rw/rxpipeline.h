#ifndef RW_RXPIPELINE_H
#define RW_RXPIPELINE_H

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
typedef RwUInt32* RxNodeOutput;
typedef RxPipelineNode* RxNodeInput;
typedef struct RxPacket RxPacket;
typedef struct RwGlobals RwGlobals;

typedef struct RwRGBA {
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    unsigned char alpha;
} RwRGBA;

typedef enum RwShadeMode {
    rwSHADEMODEGOURAUD = 2
} RwShadeMode;

typedef enum RwBlendFunction {
    rwBLENDSRCALPHA = 5,
    rwBLENDINVSRCALPHA = 6
} RwBlendFunction;

typedef enum RwFogType {
    rwFOGTYPENAFOGTYPE = 0
} RwFogType;

typedef struct RxRenderStateVector {
    RwUInt32 Flags;
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

typedef enum RxEmbeddedPacketState {
    rxPKST_PACKETLESS = 0,
    rxPKST_UNUSED = 1,
    rxPKST_INUSE = 2,
    rxPKST_PENDING = 3
} RxEmbeddedPacketState;

typedef struct RxCluster {
    union {
        struct {
            RwUInt16 flags;
            RwUInt16 stride;
        };
        RwUInt32 flagsAndStride;
    };
    void* data;
    void* currentData;
    RwUInt32 numAlloced;
    RwUInt32 numUsed;
    RxPipelineCluster* clusterRef;
    RwUInt32 attributes;
} RxCluster;

typedef struct RxPipeline {
    RwBool locked;
    RwUInt32 numNodes;
    RxPipelineNode* nodes;
    RwUInt32 packetNumClusterSlots;
    RxEmbeddedPacketState embeddedPacketState;
    RxPacket* embeddedPacket;
    RwUInt32 numInputRequirements;
    RxPipelineRequiresCluster* inputRequirements;
    void* superBlock;
    RwUInt32 superBlockSize;
    RwUInt32 entryPoint;
    RwUInt32 pluginId;
    RwUInt32 pluginData;
} RxPipeline;

typedef enum RxClusterValidityReq {
    rxCLREQ_DONTWANT = 0,
    rxCLREQ_REQUIRED = 1,
    rxCLREQ_OPTIONAL = 2
} RxClusterValidityReq;

typedef enum RxClusterValid {
    rxCLVALID_NOCHANGE = 0,
    rxCLVALID_VALID = 1,
    rxCLVALID_INVALID = 2
} RxClusterValid;

struct RxClusterDefinition {
    RwChar* name;
    RwUInt32 defaultStride;
    RwUInt32 defaultAttributes;
    const RwChar* attributeSet;
};

struct RxOutputSpec {
    RwChar* name;
    RxClusterValid* outputClusters;
    RxClusterValid allOtherClusters;
};

struct RxClusterRef {
    RxClusterDefinition* clusterDef;
    RwBool forcePresent;
    RwUInt32 reserved;
};

struct RxPipelineCluster {
    RxClusterDefinition* clusterRef;
    RwUInt32 creationAttributes;
};

struct RxPipelineRequiresCluster {
    RxClusterDefinition* clusterDef;
    RxClusterValidityReq rqdOrOpt;
    RwUInt32 slotIndex;
};

typedef struct RxIoSpec {
    RwUInt32 numClustersOfInterest;
    RxClusterRef* clustersOfInterest;
    RxClusterValidityReq* inputRequirements;
    RwUInt32 numOutputs;
    RxOutputSpec* outputs;
} RxIoSpec;

typedef RwBool (*RxNodeBodyFn)(RxPipelineNode*, const RxPipelineNodeParam*);
typedef RwBool (*RxNodeInitFn)(RxNodeDefinition*);
typedef void (*RxNodeTermFn)(RxNodeDefinition*);
typedef RwBool (*RxPipelineNodeInitFn)(RxPipelineNode*);
typedef void (*RxPipelineNodeTermFn)(RxPipelineNode*);
typedef RwBool (*RxPipelineNodeConfigFn)(RxPipelineNode*, RxPipeline*);
typedef RwUInt32 (*RxConfigMsgHandlerFn)(RxPipelineNode*, RwUInt32,
                                         RwUInt32, void*);

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
    RwChar* name;
    RxNodeMethods nodeMethods;
    RxIoSpec io;
    RwUInt32 pipelineNodePrivateDataSize;
    RwBool editable;
    RwInt32 InputPipesCnt;
};

struct RxPipelineNode {
    RxNodeDefinition* nodeDef;
    RwUInt32 numOutputs;
    RwUInt32* outputs;
    RxPipelineCluster** slotClusterRefs;
    RwUInt32* slotsContinue;
    void* privateData;
    RwUInt32* inputToClusterSlot;
    RxPipelineNodeTopSortData* topSortData;
    void* initializationData;
    RwUInt32 initializationDataSize;
};

struct RxPipelineNodeTopSortData {
    RwUInt32 numIns;
    RwUInt32 numInsVisited;
    rxReq* req;
};

struct RxPipelineNodeParam {
    void* dataParam;
    RxHeap* heap;
};

struct RxPacket {
    RwUInt16 flags;
    RwUInt16 numClusters;
    RxPipeline* pipeline;
    RwUInt32* inputToClusterSlot;
    RwUInt32* slotsContinue;
    RxPipelineCluster** slotClusterRefs;
    RxCluster clusters[1];
};

typedef struct RxPipelinePlatformGlobals {
    RwFreeList* pipelines;
    RxRenderStateVector defaultRenderState;
    RxPipeline* currentPipeline;
    RxPipelineNode* currentNode;
    RwUInt32 maxNodes;
    RxPipeline* defaultAtomicPipeline;
    RxPipeline* defaultWorldSectorPipeline;
    RxPipeline* defaultMaterialPipeline;
    RxPipeline* pipeline48;
    RxPipeline* pipeline4C;
    RxPipeline* pipeline50;
    RxPipeline* platformAtomicPipeline;
    RxPipeline* platformWorldSectorPipeline;
    RxPipeline* platformMaterialPipeline;
} RxPipelinePlatformGlobals;

typedef struct RxExecutionContext {
    RxPipeline* pipeline;
    RwUInt32 field04;
    RwBool executionStatus;
    RwUInt32 field0C;
    RxPipelineNodeParam params;
} RxExecutionContext;

struct RxHeapFreeBlock {
    RwUInt32 size;
    RxHeapBlock* block;
};

struct RxHeapBlock {
    RxHeapBlock* prev;
    RxHeapBlock* next;
    RwUInt32 size;
    RxHeapFreeBlock* freeEntry;
    RwUInt32 bookkeeping[4];
};

struct RxHeapSuperBlock {
    RxHeapBlock* start;
    RwUInt32 size;
    RxHeapSuperBlock* next;
};

struct RxHeap {
    RwUInt32 superBlockSize;
    RxHeapSuperBlock* firstSuperBlock;
    RxHeapBlock* firstBlock;
    RxHeapFreeBlock* freeBlocks;
    RwUInt32 freeBlocksAllocated;
    RwUInt32 freeBlocksUsed;
    RwBool dirty;
};

extern RwGlobals* RwEngineInstance;
extern RwInt32 _rxPipelineGlobalsOffset;

#define RXPIPELINEGLOBAL(field) \
    (((RxPipelinePlatformGlobals*)((unsigned char*)RwEngineInstance + \
                                   _rxPipelineGlobalsOffset))->field)

#define RXPIPELINEDEFAULTRENDERSTATE \
    (*(RxRenderStateVector*)((unsigned char*)RwEngineInstance + \
                             _rxPipelineGlobalsOffset + 4))

void _rxPacketDestroy(RxPacket* packet);
RwBool _rxPipelineOpen(void);
RwBool _rxPipelineClose(void);
RxPipeline* RxPipelineExecute(RxPipeline* pipeline, void* data,
                              RwBool heapReset);
RxHeap* RxHeapGetGlobalHeap(void);
RxPipeline* RxPipelineCreate(void);
void _rxPipelineDestroy(RxPipeline* pipeline);
RxPipeline* RxLockedPipeUnlock(RxLockedPipe* pipeline);
RxLockedPipe* RxPipelineLock(RxPipeline* pipeline);
RxPipelineNode* RxPipelineFindNodeByName(RxPipeline* pipeline,
                                         const RwChar* name,
                                         RxPipelineNode* start,
                                         RwInt32* nodeIndex);
RxLockedPipe* RxLockedPipeAddFragment(RxLockedPipe* pipeline,
                                      RwUInt32* firstIndex,
                                      RxNodeDefinition* nodeDef0, ...);
RxPipeline* RxLockedPipeAddPath(RxLockedPipe* pipeline, RxNodeOutput output,
                                RxNodeInput input);
RxNodeOutput RxPipelineNodeFindOutputByIndex(RxPipelineNode* node,
                                             RwUInt32 outputIndex);
RxNodeInput RxPipelineNodeFindInput(RxPipelineNode* node);
void* StalacTiteAlloc(RwInt32 size);
void* StalacMiteAlloc(RwInt32 size);
RwUInt32 PipelineCalcNumUniqueClusters(RxPipeline* pipeline);
void RxHeapFree(RxHeap* heap, void* block);
RwBool _rxHeapReset(RxHeap* heap);
void RxHeapDestroy(RxHeap* heap);
RxHeap* RxHeapCreate(RwUInt32 size);
RxRenderStateVector* RxRenderStateVectorSetDefaultRenderStateVector(
    RxRenderStateVector* renderState);
void _rx_rxRadixExchangeSort(RwUInt8* base, RwUInt32 numEntries,
                             RwUInt32 entrySize, RwUInt32 keyOffset,
                             RwUInt32 keyLowerBound,
                             RwUInt32 keyUpperBound);

#endif
