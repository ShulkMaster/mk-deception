#ifndef RW_RXPIPELINE_H
#define RW_RXPIPELINE_H

#include "rw/rwcore_types.h"
#include "rw/rwplcore.h"

typedef struct RxHeap RxHeap;
typedef struct RxHeapBlock RxHeapBlock;
typedef struct RxHeapFreeBlock RxHeapFreeBlock;
typedef struct RxHeapSuperBlock RxHeapSuperBlock;
typedef struct RxPipelineNode RxPipelineNode;
typedef struct RxClusterDefinition RxClusterDefinition;
typedef struct RxPipelineCluster RxPipelineCluster;
typedef struct RxPipelineRequiresCluster RxPipelineRequiresCluster;
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
    unsigned char core[0x3C];
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
