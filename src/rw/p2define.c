#include "rw/rwengine.h"
#include "runtime/cstdarg.h"
#include "rw/rwerror.h"
#include "rw/rxpipeline.h"

typedef struct RxMemoryLimits {
    unsigned char* stalactite;
    unsigned char* stalagmite;
} RxMemoryLimits;

static RxMemoryLimits gMemoryLimits;

extern unsigned int _rxPipelineMaxNodes;
extern unsigned int _rxChaseDependencies(RxPipeline* pipeline);
extern RxPipelineNode* PipelineNodeDestroy(RxPipelineNode* node,
                                           RxPipeline* pipeline);

void* StalacTiteAlloc(int size)
{
    size = (size + 3) & ~3U;

    gMemoryLimits.stalactite -= size;
    if (gMemoryLimits.stalactite < gMemoryLimits.stalagmite) {
        RwError error;
        gMemoryLimits.stalactite += size;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x80000013, size);
        RwErrorSet(&error);
        return 0;
    }
    return gMemoryLimits.stalactite;
}

void* StalacMiteAlloc(int size)
{
    size = (size + 3) & ~3U;

    gMemoryLimits.stalagmite += size;
    if (gMemoryLimits.stalagmite > gMemoryLimits.stalactite) {
        RwError error;
        gMemoryLimits.stalagmite -= size;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x80000013, size);
        RwErrorSet(&error);
        return 0;
    }
    return gMemoryLimits.stalagmite - size;
}

unsigned int PipelineCalcNumUniqueClusters(RxPipeline* pipeline)
{
    RxClusterDefinition* previous;
    RxClusterDefinition* next;
    unsigned int count;
    unsigned int nodeIndex;
    unsigned int clusterIndex;

    count = 0;
    next = 0;

    for (;;) {
        previous = next;
        next = (RxClusterDefinition*)-1;
        for (nodeIndex = 0; nodeIndex < pipeline->numNodes; nodeIndex++) {
            RxNodeDefinition* nodeDef = pipeline->nodes[nodeIndex].nodeDef;

            for (clusterIndex = 0;
                 clusterIndex < nodeDef->io.numClustersOfInterest;
                 clusterIndex++) {
                RxClusterDefinition* cluster =
                    nodeDef->io.clustersOfInterest[clusterIndex].clusterDef;
                if (cluster > previous && cluster < next) {
                    next = cluster;
                }
            }
        }
        if (next == (RxClusterDefinition*)-1) {
            break;
        }
        count++;
    }
    return count;
}

static int ReallocAndFixupSuperBlock(RxPipeline* pipeline,
                                        unsigned int newSize)
{


    unsigned char* oldBlock = pipeline->superBlock;
    unsigned char* newBlock = RwEngineInstance->fpRealloc(
        oldBlock, newSize, 0x01040409);

    if (newBlock != 0) {
        unsigned int numNodes = pipeline->numNodes;
        int displacement = newBlock - oldBlock;
        unsigned int index;

        pipeline->superBlock = newBlock;
        pipeline->superBlockSize = newSize;
        pipeline->nodes = (RxPipelineNode*)pipeline->superBlock;
        pipeline->inputRequirements =
            pipeline->inputRequirements == 0
                ? 0
                : (RxPipelineRequiresCluster*)(
                      (unsigned char*)pipeline->inputRequirements + displacement);
        pipeline->embeddedPacket =
            pipeline->embeddedPacket == 0
                ? 0
                : (RxPacket*)((unsigned char*)pipeline->embeddedPacket +
                              displacement);
        for (index = 0; index < numNodes; index++) {
            pipeline->nodes[index].outputs =
                pipeline->nodes[index].outputs == 0
                    ? 0
                    : (unsigned int*)((unsigned char*)pipeline->nodes[index].outputs +
                                 displacement);
            pipeline->nodes[index].slotClusterRefs =
                pipeline->nodes[index].slotClusterRefs == 0
                    ? 0
                    : (RxPipelineCluster**)(
                        (unsigned char*)pipeline->nodes[index].slotClusterRefs +
                        displacement);
            pipeline->nodes[index].slotsContinue =
                pipeline->nodes[index].slotsContinue == 0
                    ? 0
                    : (unsigned int*)(
                          (unsigned char*)pipeline->nodes[index].slotsContinue +
                          displacement);
            pipeline->nodes[index].privateData =
                pipeline->nodes[index].privateData == 0
                    ? 0
                    : (unsigned char*)pipeline->nodes[index].privateData +
                          displacement;
            pipeline->nodes[index].inputToClusterSlot =
                pipeline->nodes[index].inputToClusterSlot == 0
                    ? 0
                    : (unsigned int*)(
                          (unsigned char*)pipeline->nodes[index].inputToClusterSlot +
                          displacement);
            pipeline->nodes[index].topSortData =
                pipeline->nodes[index].topSortData == 0
                    ? 0
                    : (RxPipelineNodeTopSortData*)(
                        (unsigned char*)pipeline->nodes[index].topSortData +
                        displacement);
        }
    } else {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x80000013, newSize);
        RwErrorSet(&error);
        return 0;
    }
    return 1;
}

static int LockPipelineExpandData(RxPipeline* destination,
                                     const RxPipeline* source)
{
    int index;
    unsigned int* outputsBase;
    RxPipelineNodeTopSortData* topSortBase;

    if (destination != source) {
        for (index = (int)source->numNodes - 1; index >= 0; index--) {
            memcpy(&destination->nodes[index], &source->nodes[index],
                   sizeof(RxPipelineNode));
            destination->nodes[index].slotClusterRefs = 0;
            destination->nodes[index].slotsContinue = 0;
            destination->nodes[index].privateData = 0;
            destination->nodes[index].inputToClusterSlot = 0;
            if (destination->nodes[index].initializationDataSize != 0) {
                destination->nodes[index].initializationData =
                    RwEngineInstance->fpMalloc(
                        destination->nodes[index].initializationDataSize,
                        0x30409);
                if (destination->nodes[index].initializationData == 0) {
                    RwError error;
                    error.pluginID = 1;
                    error.errorCode = _rwerror(
                        0x80000013,
                        destination->nodes[index].initializationDataSize);
                    RwErrorSet(&error);
                    return 0;
                }
                memcpy(destination->nodes[index].initializationData,
                       source->nodes[index].initializationData,
                       destination->nodes[index].initializationDataSize);
            }
        }
        destination->numNodes = source->numNodes;
    }

    outputsBase = (unsigned int*)((unsigned char*)destination->nodes +
                              rxPipelineGlobalField(maxNodesPerPipe) *
                                  sizeof(RxPipelineNode));
    for (index = (int)source->numNodes - 1; index >= 0; index--) {
        destination->nodes[index].outputs = outputsBase + index * 0x20;
        if (source->nodes[index].outputs != 0) {
            memcpy(destination->nodes[index].outputs,
                   source->nodes[index].outputs,
                   0x20 * sizeof(unsigned int));
        }
    }

    topSortBase = (RxPipelineNodeTopSortData*)(
        outputsBase + rxPipelineGlobalField(maxNodesPerPipe) * 0x20);
    for (index = 0; (unsigned int)index < source->numNodes; index++) {
        topSortBase[index].numIns = 0;
        topSortBase[index].numInsVisited = 0;
        topSortBase[index].req = 0;
        destination->nodes[index].topSortData = &topSortBase[index];
    }
    return 1;
}

static unsigned int CalcNodesOutputsCompactedMemSize(const RxPipeline* pipeline)
{
    unsigned int size = pipeline->numNodes * sizeof(RxPipelineNode);
    unsigned int index;


    for (index = 0; index < pipeline->numNodes; index++) {
        size += pipeline->nodes->numOutputs * sizeof(unsigned int);
    }
    return size;
}

static unsigned int CalcUnlockPersistentMemSize(const RxPipeline* pipeline,
                                            unsigned int numClusters)
{




    unsigned int size = 0;
    unsigned int index;

    size += numClusters * 8;
    size += numClusters * 12;
    size += pipeline->numNodes * numClusters * sizeof(unsigned int);
    size += pipeline->numNodes * (numClusters + 1) * sizeof(unsigned int);
    for (index = 0; index < pipeline->numNodes; index++) {
        const RxPipelineNode* node = &pipeline->nodes[index];
        if (node->nodeDef->pipelineNodePrivateDataSize != 0) {
            size += node->nodeDef->pipelineNodePrivateDataSize;
        }
        size += node->nodeDef->io.numClustersOfInterest * sizeof(unsigned int);
    }
    size += (numClusters - 1) * 0x1C;
    size += 0x30;
    return size;
}

static int _NodeCreate(RxPipeline* pipeline, RxPipelineNode* node,
                          RxNodeDefinition* nodeDef)
{
    RxPipelineNodeTopSortData* topSortData;
    unsigned int* outputs;
    int result = 1;
    unsigned int n;

    n = nodeDef->io.numOutputs;

    memset(node, 0, sizeof(*node));
    if (n > 0x20) {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x29);
        RwErrorSet(&error);
        result = 0;
    }
    if (nodeDef->io.numClustersOfInterest > 0x20) {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x28);
        RwErrorSet(&error);
        result = 0;
    }





    if (n >= rxPipelinePlatformData()->maxNodesPerPipe) {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x2A);
        RwErrorSet(&error);
        result = 0;
    }
    if (result) {
        outputs = (unsigned int*)&pipeline->nodes[rxPipelinePlatformData()->maxNodesPerPipe];
        outputs += pipeline->numNodes * 0x20;
        node->outputs = outputs;
        node->numOutputs = n;
        for (n = 0; n < node->numOutputs; n++) {
            *outputs = (unsigned int)-1;
            outputs++;
        }
        outputs = (unsigned int*)&pipeline->nodes[rxPipelinePlatformData()->maxNodesPerPipe];
        topSortData = (RxPipelineNodeTopSortData*)&outputs[
            rxPipelinePlatformData()->maxNodesPerPipe * 0x20];
        topSortData += pipeline->numNodes;
        topSortData->numIns = 0;
        topSortData->numInsVisited = 0;
        topSortData->req = 0;
        node->topSortData = topSortData;
        node->initializationData = 0;
        node->initializationDataSize = 0;
        node->nodeDef = nodeDef;
        pipeline->numNodes++;
    }
    return result;
}

static void PipelineTallyInputs(RxPipeline* pipeline)
{
    RxPipelineNode* node = pipeline->nodes;
    unsigned int index;

    for (index = 0; index < pipeline->numNodes;) {
        if (node->nodeDef != 0) {
            node->topSortData->numInsVisited = 0;
            node->topSortData->numIns = 0;
        }
        node++;
        index++;
    }
    node = pipeline->nodes;
    for (index = 0; index < pipeline->numNodes;) {
        if (node->nodeDef != 0 && node->numOutputs != 0) {
            int remaining;
            unsigned int outputsRemaining = node->numOutputs;
            unsigned int* output = node->outputs;
            do {
                if ((int)*output != -1) {
                    pipeline->nodes[*output].topSortData->numIns++;
                }
                output++;
                remaining = --outputsRemaining;
            } while (remaining != 0);
        }
        node++;
        index++;
    }
}

typedef struct PipelineTopSortState {
    RxPipeline* pipeline;
    unsigned int sortedCount;
} PipelineTopSortState;

static void PipelineTopSort(PipelineTopSortState* data, unsigned int nodeIndex)
{


    RxPipelineNode* placedNode;
    unsigned int i = data->sortedCount;
    unsigned int j = nodeIndex;

    if (i != j) {
        unsigned int swappedOutput;
        unsigned int* sortedOutputs;
        unsigned int* selectedOutputs;
        RxPipelineNodeTopSortData swappedSortState;
        RxPipelineNodeTopSortData* sortedState;
        RxPipelineNodeTopSortData* selectedState;
        RxPipelineNode swappedNode;
        unsigned int k;
        unsigned int l;

        sortedOutputs = data->pipeline->nodes[i].outputs;
        selectedOutputs = data->pipeline->nodes[j].outputs;
        for (k = 0; k < 0x20; k++) {
            swappedOutput = sortedOutputs[k];
            sortedOutputs[k] = selectedOutputs[k];
            selectedOutputs[k] = swappedOutput;
        }
        data->pipeline->nodes[i].outputs = selectedOutputs;
        data->pipeline->nodes[j].outputs = sortedOutputs;

        sortedState = data->pipeline->nodes[i].topSortData;
        selectedState = data->pipeline->nodes[j].topSortData;
        swappedSortState = *sortedState;
        *sortedState = *selectedState;
        *selectedState = swappedSortState;
        data->pipeline->nodes[i].topSortData = selectedState;
        data->pipeline->nodes[j].topSortData = sortedState;

        swappedNode = data->pipeline->nodes[i];
        data->pipeline->nodes[i] = data->pipeline->nodes[j];
        data->pipeline->nodes[j] = swappedNode;

        for (k = 0; k < data->pipeline->numNodes; k++) {
            RxPipelineNode* node = &data->pipeline->nodes[k];
            for (l = 0; l < node->numOutputs; l++) {
                if (node->outputs[l] == i) {
                    node->outputs[l] = j;
                } else if (node->outputs[l] == j) {
                    node->outputs[l] = i;
                }
            }
        }
    }

    placedNode = &data->pipeline->nodes[data->sortedCount];
    data->sortedCount++;
    if (placedNode->numOutputs != 0) {
        for (i = 0; i < placedNode->numOutputs; i++) {
            unsigned int successorIndex = placedNode->outputs[i];

            if (successorIndex != (unsigned int)-1) {
                RxPipelineNode* successor =
                    &data->pipeline->nodes[successorIndex];
                successor->topSortData->numInsVisited++;
                if (successor->topSortData->numIns ==
                    successor->topSortData->numInsVisited) {
                    PipelineTopSort(data, successorIndex);
                }
            }
        }
    }
}

/* Converts a node pointer back to its validated pipeline-array index. */
static unsigned int PipelineNode2Index(RxPipeline* pipeline, RxPipelineNode* node)
{
    unsigned int index;

    index = ((unsigned char*)node - (unsigned char*)pipeline->nodes) /
            sizeof(*node);

    if (&pipeline->nodes[index] == node &&
        index < pipeline->numNodes) {
        return index;
    }
    return -1;
}

static RxPipeline* PipelineUnlockTopSort(RxPipeline* pipeline)
{
    PipelineTopSortState state;
    unsigned int index;

    state.pipeline = pipeline;
    state.sortedCount = 0;
    PipelineTallyInputs(pipeline);
    if (pipeline->nodes[pipeline->entryPoint].topSortData->numIns != 0) {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x24);
        RwErrorSet(&error);
        return 0;
    }
    for (index = 0; index < pipeline->numNodes; index++) {
        if (index != pipeline->entryPoint &&
            pipeline->nodes[index].topSortData->numIns == 0) {
            RwError error;
            error.pluginID = 1;
            error.errorCode = _rwerror(0x22);
            RwErrorSet(&error);
            return 0;
        }
    }
    PipelineTopSort(&state, pipeline->entryPoint);
    for (index = 0; index < pipeline->numNodes; index++) {
        if (pipeline->nodes[index].topSortData->numIns !=
            pipeline->nodes[index].topSortData->numInsVisited) {
            RwError error;
            error.pluginID = 1;
            error.errorCode = _rwerror(0x1C);
            RwErrorSet(&error);
            return 0;
        }
    }
    pipeline->entryPoint = 0;
    return pipeline;
}

RxNodeOutput RxPipelineNodeFindOutputByIndex(RxPipelineNode* node,
                                             unsigned int outputIndex)
{
    if (node != 0 && node->nodeDef != 0 &&
        outputIndex < node->numOutputs) {
        return &node->outputs[outputIndex];
    }
    return 0;
}

RxNodeInput RxPipelineNodeFindInput(RxPipelineNode* node)
{
    if (node != 0 && node->nodeDef != 0) {
        return node;
    }
    return 0;
}



RxPipeline* RxLockedPipeUnlock(RxLockedPipe* pipeline)
{
    RxPipelineNodeTopSortData* newTopSortData;
    RxPipelineNodeTopSortData* topSortData;
    unsigned int numUniqueClusters;
    unsigned int unlockStartBlockSize;
    unsigned int unlockEndBlockSize;
    unsigned int topSortBlockSize;
    unsigned int depChaseBlockSize;
    unsigned int totalOutputs;
    unsigned int* newOutputs;
    unsigned int* outputs;
    int error;
    unsigned int doneNodes;
    int i;

    if (pipeline != 0 && pipeline->locked) {
      if (pipeline->numNodes != 0) {
        totalOutputs = 0;
        error = 0;
        doneNodes = 0;
        if (pipeline->entryPoint >= pipeline->numNodes ||
            pipeline->nodes[pipeline->entryPoint].nodeDef == 0) {
            RwError error;
            error.pluginID = 1;
            error.errorCode = _rwerror(0x24);
            RwErrorSet(&error);
            return 0;
        }

        numUniqueClusters = PipelineCalcNumUniqueClusters(pipeline);
        topSortBlockSize =
            rxPipelinePlatformData()->maxNodesPerPipe * sizeof(RxPipelineNodeTopSortData) +
            (rxPipelinePlatformData()->maxNodesPerPipe * sizeof(RxPipelineNode) +
             rxPipelinePlatformData()->maxNodesPerPipe * 0x80);
        depChaseBlockSize =
            rxPipelinePlatformData()->maxNodesPerPipe * sizeof(RxPipelineNodeTopSortData);
        depChaseBlockSize += CalcNodesOutputsCompactedMemSize(pipeline);
        depChaseBlockSize = pipeline->numNodes * 0x14 +
            (pipeline->numNodes * numUniqueClusters * 0x24 +
             pipeline->numNodes * numUniqueClusters * 0x10 +
             depChaseBlockSize);
        depChaseBlockSize +=
            CalcUnlockPersistentMemSize(pipeline, numUniqueClusters);
        unlockStartBlockSize = topSortBlockSize;
        if (depChaseBlockSize > unlockStartBlockSize) {
            unlockStartBlockSize = depChaseBlockSize;
        }
        if (unlockStartBlockSize > pipeline->superBlockSize &&
            !ReallocAndFixupSuperBlock(pipeline, unlockStartBlockSize)) {
            return 0;
        }

        gMemoryLimits.stalactite =
            (unsigned char*)pipeline->superBlock + unlockStartBlockSize;
        gMemoryLimits.stalagmite = pipeline->superBlock;
        pipeline = PipelineUnlockTopSort(pipeline);
        if (pipeline == 0) {
            return 0;
        }

        outputs = (unsigned int*)&pipeline->nodes[rxPipelinePlatformData()->maxNodesPerPipe];
        topSortData = (RxPipelineNodeTopSortData*)&outputs[
            0x20 * rxPipelinePlatformData()->maxNodesPerPipe];
        topSortData += pipeline->numNodes - 1;
        newTopSortData = (RxPipelineNodeTopSortData*)(
            (unsigned char*)pipeline->superBlock + unlockStartBlockSize);
        newTopSortData--;
        for (i = (int)pipeline->numNodes - 1; i >= 0; i--) {
            memcpy(newTopSortData, topSortData, sizeof(*newTopSortData));
            pipeline->nodes[i].topSortData = newTopSortData;
            topSortData--;
            newTopSortData--;
        }
        newOutputs = (unsigned int*)&pipeline->nodes[pipeline->numNodes];
        for (i = 0; (unsigned int)i < pipeline->numNodes; i++) {
            if (pipeline->nodes[i].numOutputs == 0) {
                pipeline->nodes[i].outputs = 0;
            } else {
                memcpy(newOutputs, outputs,
                       pipeline->nodes[i].numOutputs * sizeof(unsigned int));
                pipeline->nodes[i].outputs = newOutputs;
            }
            outputs += 0x20;
            newOutputs += pipeline->nodes[i].numOutputs;
            totalOutputs += pipeline->nodes[i].numOutputs;
        }
        gMemoryLimits.stalagmite = (unsigned char*)&outputs[totalOutputs];
        gMemoryLimits.stalactite = (unsigned char*)topSortData;

        if (_rxChaseDependencies(pipeline) != 0) {
            return 0;
        }
        unlockEndBlockSize = (unsigned int)(
            gMemoryLimits.stalagmite - (unsigned char*)pipeline->superBlock);
        if (!ReallocAndFixupSuperBlock(pipeline, unlockEndBlockSize)) {
            LockPipelineExpandData(pipeline, pipeline);
            return 0;
        }
        for (i = 0; (unsigned int)i < pipeline->numNodes; i++) {
            pipeline->nodes[i].topSortData = 0;
        }

        for (i = (int)pipeline->numNodes - 1; i >= 0; i--) {
                RxNodeDefinition* nodeDef = pipeline->nodes[i].nodeDef;
                RxPipelineNode* node = &pipeline->nodes[i];
                int oldCount = nodeDef->InputPipesCnt++;

                if (oldCount == 0) {
                    if (nodeDef->nodeMethods.nodeInit != 0) {
                        if (!nodeDef->nodeMethods.nodeInit(nodeDef)) {
                            doneNodes =
                                (pipeline->numNodes - 1) - (unsigned int)i;
                            error = 1;
                            break;
                        }
                    }
                }
                if (nodeDef->nodeMethods.pipelineNodeInit != 0) {
                    if (!nodeDef->nodeMethods.pipelineNodeInit(node)) {
                        nodeDef->InputPipesCnt--;
                        if (nodeDef->InputPipesCnt == 0) {
                            if (nodeDef->nodeMethods.nodeTerm != 0) {
                                nodeDef->nodeMethods.nodeTerm(nodeDef);
                            }
                        }
                        doneNodes = (pipeline->numNodes - 1) - (unsigned int)i;
                        error = 1;
                        break;
                    }
                }
        }

        if (!error) {
            for (i = (int)pipeline->numNodes - 1; i >= 0; i--) {
                RxNodeDefinition* nodeDef = pipeline->nodes[i].nodeDef;
                RxPipelineNode* node = &pipeline->nodes[i];
                if (nodeDef->nodeMethods.pipelineNodeConfig != 0) {
                    if (!nodeDef->nodeMethods.pipelineNodeConfig(node,
                                                                 pipeline)) {
                        doneNodes = pipeline->numNodes;
                        error = 1;
                        break;
                    }
                }
            }
        }

        if (error) {
            for (i = (pipeline->numNodes - 1) - (doneNodes - 1);
                 (unsigned int)i < pipeline->numNodes; i++) {
                RxNodeDefinition* nodeDef = pipeline->nodes[i].nodeDef;
                RxPipelineNode* node = &pipeline->nodes[i];
                if (nodeDef->nodeMethods.pipelineNodeTerm != 0) {
                    nodeDef->nodeMethods.pipelineNodeTerm(node);
                }
                nodeDef->InputPipesCnt--;
                if (nodeDef->InputPipesCnt == 0) {
                    if (nodeDef->nodeMethods.nodeTerm != 0) {
                        nodeDef->nodeMethods.nodeTerm(nodeDef);
                    }
                }
            }
            LockPipelineExpandData(pipeline, pipeline);
            return 0;
        }
      }

        pipeline->locked = 0;
        return pipeline;
    }
    if (pipeline == 0) {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x80000016);
        RwErrorSet(&error);
    } else {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x34);
        RwErrorSet(&error);
    }
    return 0;
}

RxLockedPipe* RxPipelineLock(RxPipeline* pipeline)
{

    if (!pipeline->locked) {
        unsigned int requiredSize =
            rxPipelinePlatformData()->maxNodesPerPipe *
                sizeof(RxPipelineNodeTopSortData) +
            (rxPipelinePlatformData()->maxNodesPerPipe * sizeof(RxPipelineNode) +
             rxPipelinePlatformData()->maxNodesPerPipe * 0x80);
        unsigned int index;
        if (pipeline->nodes != 0) {
            if (requiredSize > pipeline->superBlockSize &&
                !ReallocAndFixupSuperBlock(pipeline, requiredSize)) {
                return 0;
            }
            if (!LockPipelineExpandData(pipeline, pipeline)) {
                return 0;
            }
        } else {
            pipeline->superBlock = RwEngineInstance->fpMalloc(
                requiredSize, 0x01030409);
            if (pipeline->superBlock == 0) {
                RwError error;
                error.pluginID = 1;
                error.errorCode = _rwerror(0x80000013, requiredSize);
                RwErrorSet(&error);
                return 0;
            }
            pipeline->superBlockSize = requiredSize;
            pipeline->nodes = pipeline->superBlock;
        }
        pipeline->locked = 1;
        if (pipeline->nodes != 0) {
            for (index = 0; index < pipeline->numNodes; index++) {
                RxNodeMethods* nodeMethods =
                    &pipeline->nodes[index].nodeDef->nodeMethods;
                if (nodeMethods->pipelineNodeTerm != 0) {
                    nodeMethods->pipelineNodeTerm(&pipeline->nodes[index]);
                }
                if (--pipeline->nodes[index].nodeDef->InputPipesCnt == 0 &&
                    nodeMethods->nodeTerm != 0) {
                    nodeMethods->nodeTerm(pipeline->nodes[index].nodeDef);
                }
                pipeline->nodes[index].slotClusterRefs = 0;
            }
        }
    }
    return pipeline;
}





RxPipelineNode* RxPipelineFindNodeByName(RxPipeline* pipeline,
                                         const char* name,
                                         RxPipelineNode* start,
                                         int* nodeIndex)
{
    int hasNodes = 0;
    int validArguments = 0;

    if (pipeline != 0 && name != 0) {
        validArguments = 1;
    }
    if (validArguments && pipeline->numNodes != 0) {
        hasNodes = 1;
    }
    if (hasNodes) {
        RxPipelineNode* node = pipeline->nodes;
        int remaining = pipeline->numNodes;

        if (start != 0) {
            while (node != start && remaining > 0) {
                node++;
                remaining--;
            }
            node++;
            remaining--;
        }
        while (remaining > 0) {
            if (node->nodeDef != 0 &&
                RwEngineInstance->stringFuncs.strcmp(node->nodeDef->name,
                                                        name) == 0) {
                if (nodeIndex != 0) {
                    *nodeIndex = pipeline->numNodes - remaining;
                }
                return node;
            }
            node++;
            remaining--;
        }
    }
    if (nodeIndex != 0) {
        *nodeIndex = -1;
    }
    return 0;
}





RxLockedPipe* RxLockedPipeAddFragment(RxLockedPipe* pipeline,
                                      unsigned int* firstIndex,
                                      RxNodeDefinition* nodeDef0, ...)
{
    __va_list arguments;
    RxNodeDefinition* nodeDef;
    unsigned int count;
    unsigned int created;
    unsigned int firstNode;
    RxPipelineNode* previous;

    if (pipeline != 0 && pipeline->locked) {
        count = 0;
        va_start(arguments, nodeDef0);
        nodeDef = nodeDef0;
        while (nodeDef != 0) {
            count++;
            nodeDef = *(RxNodeDefinition**)__va_arg(arguments, 1);
        }
        va_end(arguments);
        if (count != 0) {
            previous = 0;
            if (pipeline->numNodes + count > rxPipelinePlatformData()->maxNodesPerPipe) {
                RwError error;
                error.pluginID = 1;
                error.errorCode = _rwerror(0x2A);
                RwErrorSet(&error);
                return 0;
            }

            firstNode = pipeline->numNodes;
            created = 0;
            va_start(arguments, nodeDef0);
            nodeDef = nodeDef0;
            while (nodeDef != 0) {
                RxPipelineNode* node =
                    &pipeline->nodes[firstNode + created];
                if (!_NodeCreate(pipeline, node, nodeDef)) {
                    break;
                }
                if (previous != 0 &&
                    RxLockedPipeAddPath(
                        pipeline,
                        RxPipelineNodeFindOutputByIndex(previous, 0),
                        RxPipelineNodeFindInput(node)) == 0) {
                    PipelineNodeDestroy(node, pipeline);
                    break;
                }
                previous = node;
                created++;
                nodeDef = *(RxNodeDefinition**)__va_arg(arguments, 1);
            }
            va_end(arguments);

            if (created == count) {
                if (firstIndex != 0) {
                    *firstIndex = firstNode;
                }
                return pipeline;
            }
            while (created-- != 0) {
                PipelineNodeDestroy(
                    &pipeline->nodes[created + firstNode], pipeline);
            }
        }
    } else {
        if (pipeline == 0) {
            RwError error;
            error.pluginID = 1;
            error.errorCode = _rwerror(0x80000016);
            RwErrorSet(&error);
        } else {
            RwError error;
            error.pluginID = 1;
            error.errorCode = _rwerror(0x34);
            RwErrorSet(&error);
        }
    }
    return 0;
}

RxPipeline* RxLockedPipeAddPath(RxLockedPipe* pipeline, RxNodeOutput output,
                                RxNodeInput input)
{
    if (pipeline != 0 && pipeline->locked && output != 0 &&
        *output == (unsigned int)-1 && input != 0 && input->nodeDef != 0) {
        unsigned int inputIndex = PipelineNode2Index(pipeline, input);
        if (inputIndex != (unsigned int)-1) {
            *output = inputIndex;
            return pipeline;
        }
    }




    return 0;
}
