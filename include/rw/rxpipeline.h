#ifndef RW_RXPIPELINE_H
#define RW_RXPIPELINE_H

#include "rw/rwplcore.h"

typedef struct RxHeap RxHeap;
typedef struct RxPipelineNode RxPipelineNode;
typedef struct RxClusterDefinition RxClusterDefinition;
typedef struct RxPipelineCluster RxPipelineCluster;
typedef struct RxPipelineRequiresCluster RxPipelineRequiresCluster;
typedef struct RxPacket RxPacket;

typedef enum RxEmbeddedPacketState {
    rxPKST_PACKETLESS = 0,
    rxPKST_UNUSED = 1,
    rxPKST_INUSE = 2,
    rxPKST_PENDING = 3
} RxEmbeddedPacketState;

typedef struct RxCluster {
    RwUInt16 flags;
    RwUInt16 stride;
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

void _rxPacketDestroy(RxPacket* packet);
void RxHeapFree(RxHeap* heap, void* block);

#endif
