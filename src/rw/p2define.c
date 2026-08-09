#include "libmkparticle/rw_engine.h"
#include "runtime/cstdarg.h"
#include "rw/rwerror.h"
#include "rw/rxpipeline.h"

typedef struct RxMemoryLimits {
    RwUInt8* stalactite;
    RwUInt8* stalagmite;
} RxMemoryLimits;

static RxMemoryLimits gMemoryLimits;

extern RwUInt32 _rxPipelineMaxNodes;
extern RwUInt32 _rxChaseDependencies(RxPipeline* pipeline);
extern RxPipelineNode* PipelineNodeDestroy(RxPipelineNode* node,
                                           RxPipeline* pipeline);

void* StalacTiteAlloc(RwInt32 size)
{
    size = (size + 3) & ~3U;

    gMemoryLimits.stalactite -= size;
    if (gMemoryLimits.stalactite < gMemoryLimits.stalagmite) {
        RwError error;
        gMemoryLimits.stalactite += size;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x80000013, size);
        RwErrorSet(&error);
        return NULL;
    }
    return gMemoryLimits.stalactite;
}

void* StalacMiteAlloc(RwInt32 size)
{
    size = (size + 3) & ~3U;

    gMemoryLimits.stalagmite += size;
    if (gMemoryLimits.stalagmite > gMemoryLimits.stalactite) {
        RwError error;
        gMemoryLimits.stalagmite -= size;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x80000013, size);
        RwErrorSet(&error);
        return NULL;
    }
    return gMemoryLimits.stalagmite - size;
}

RwUInt32 PipelineCalcNumUniqueClusters(RxPipeline* pipeline)
{
    RxClusterDefinition* previous;
    RxClusterDefinition* next;
    RwUInt32 count;
    RwUInt32 nodeIndex;
    RwUInt32 clusterIndex;

    count = 0;
    next = NULL;

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

static RwBool ReallocAndFixupSuperBlock(RxPipeline* pipeline,
                                        RwUInt32 newSize)
{
    /* Retail retains each conditional-assignment result on the null path;
     * clean C omits those eight unused zero values. */
    RwUInt8* oldBlock = pipeline->superBlock;
    RwUInt8* newBlock = RwEngineInstance->fpRealloc(
        oldBlock, newSize, 0x01040409);

    if (newBlock != NULL) {
        RwUInt32 numNodes = pipeline->numNodes;
        RwInt32 displacement = newBlock - oldBlock;
        RwUInt32 index;

        pipeline->superBlock = newBlock;
        pipeline->superBlockSize = newSize;
        pipeline->nodes = (RxPipelineNode*)pipeline->superBlock;
        pipeline->inputRequirements =
            pipeline->inputRequirements == NULL
                ? NULL
                : (RxPipelineRequiresCluster*)(
                      (RwUInt8*)pipeline->inputRequirements + displacement);
        pipeline->embeddedPacket =
            pipeline->embeddedPacket == NULL
                ? NULL
                : (RxPacket*)((RwUInt8*)pipeline->embeddedPacket +
                              displacement);
        for (index = 0; index < numNodes; index++) {
            pipeline->nodes[index].outputs =
                pipeline->nodes[index].outputs == NULL
                    ? NULL
                    : (RwUInt32*)((RwUInt8*)pipeline->nodes[index].outputs +
                                 displacement);
            pipeline->nodes[index].slotClusterRefs =
                pipeline->nodes[index].slotClusterRefs == NULL
                    ? NULL
                    : (RxPipelineCluster**)(
                        (RwUInt8*)pipeline->nodes[index].slotClusterRefs +
                        displacement);
            pipeline->nodes[index].slotsContinue =
                pipeline->nodes[index].slotsContinue == NULL
                    ? NULL
                    : (RwUInt32*)(
                          (RwUInt8*)pipeline->nodes[index].slotsContinue +
                          displacement);
            pipeline->nodes[index].privateData =
                pipeline->nodes[index].privateData == NULL
                    ? NULL
                    : (RwUInt8*)pipeline->nodes[index].privateData +
                          displacement;
            pipeline->nodes[index].inputToClusterSlot =
                pipeline->nodes[index].inputToClusterSlot == NULL
                    ? NULL
                    : (RwUInt32*)(
                          (RwUInt8*)pipeline->nodes[index].inputToClusterSlot +
                          displacement);
            pipeline->nodes[index].topSortData =
                pipeline->nodes[index].topSortData == NULL
                    ? NULL
                    : (RxPipelineNodeTopSortData*)(
                        (RwUInt8*)pipeline->nodes[index].topSortData +
                        displacement);
        }
    } else {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x80000013, newSize);
        RwErrorSet(&error);
        return FALSE;
    }
    return TRUE;
}

static RwBool LockPipelineExpandData(RxPipeline* destination,
                                     const RxPipeline* source)
{
    RwInt32 index;
    RwUInt32* outputsBase;
    RxPipelineNodeTopSortData* topSortBase;

    if (destination != source) {
        for (index = (RwInt32)source->numNodes - 1; index >= 0; index--) {
            memcpy(&destination->nodes[index], &source->nodes[index],
                   sizeof(RxPipelineNode));
            destination->nodes[index].slotClusterRefs = NULL;
            destination->nodes[index].slotsContinue = NULL;
            destination->nodes[index].privateData = NULL;
            destination->nodes[index].inputToClusterSlot = NULL;
            if (destination->nodes[index].initializationDataSize != 0) {
                destination->nodes[index].initializationData =
                    RwEngineInstance->fpMalloc(
                        destination->nodes[index].initializationDataSize,
                        0x30409);
                if (destination->nodes[index].initializationData == NULL) {
                    RwError error;
                    error.pluginID = 1;
                    error.errorCode = _rwerror(
                        0x80000013,
                        destination->nodes[index].initializationDataSize);
                    RwErrorSet(&error);
                    return FALSE;
                }
                memcpy(destination->nodes[index].initializationData,
                       source->nodes[index].initializationData,
                       destination->nodes[index].initializationDataSize);
            }
        }
        destination->numNodes = source->numNodes;
    }

    outputsBase = (RwUInt32*)((RwUInt8*)destination->nodes +
                              RXPIPELINEGLOBAL(maxNodes) *
                                  sizeof(RxPipelineNode));
    for (index = (RwInt32)source->numNodes - 1; index >= 0; index--) {
        destination->nodes[index].outputs = outputsBase + index * 0x20;
        if (source->nodes[index].outputs != NULL) {
            memcpy(destination->nodes[index].outputs,
                   source->nodes[index].outputs,
                   0x20 * sizeof(RwUInt32));
        }
    }

    topSortBase = (RxPipelineNodeTopSortData*)(
        outputsBase + RXPIPELINEGLOBAL(maxNodes) * 0x20);
    for (index = 0; (RwUInt32)index < source->numNodes; index++) {
        topSortBase[index].numIns = 0;
        topSortBase[index].numInsVisited = 0;
        topSortBase[index].req = NULL;
        destination->nodes[index].topSortData = &topSortBase[index];
    }
    return TRUE;
}

static RwUInt32 CalcNodesOutputsCompactedMemSize(const RxPipeline* pipeline)
{
    RwUInt32 size = pipeline->numNodes * sizeof(RxPipelineNode);
    RwUInt32 index;

    /* Retail reloads the first node on every iteration. */
    for (index = 0; index < pipeline->numNodes; index++) {
        size += pipeline->nodes->numOutputs * sizeof(RwUInt32);
    }
    return size;
}

static RwUInt32 CalcUnlockPersistentMemSize(const RxPipeline* pipeline,
                                            RwUInt32 numClusters)
{
    /*
     * Retail's live body is identical except for the commutative final add,
     * but it saves LR through _savegpr_29 despite this function being a leaf.
     */
    RwUInt32 size = 0;
    RwUInt32 index;

    size += numClusters * 8;
    size += numClusters * 12;
    size += pipeline->numNodes * numClusters * sizeof(RwUInt32);
    size += pipeline->numNodes * (numClusters + 1) * sizeof(RwUInt32);
    for (index = 0; index < pipeline->numNodes; index++) {
        const RxPipelineNode* node = &pipeline->nodes[index];
        if (node->nodeDef->pipelineNodePrivateDataSize != 0) {
            size += node->nodeDef->pipelineNodePrivateDataSize;
        }
        size += node->nodeDef->io.numClustersOfInterest * sizeof(RwUInt32);
    }
    size += (numClusters - 1) * 0x1C;
    size += 0x30;
    return size;
}

static RwBool _NodeCreate(RxPipeline* pipeline, RxPipelineNode* node,
                          RxNodeDefinition* nodeDef)
{
    RxPipelineNodeTopSortData* topSortData;
    RwUInt32* outputs;
    RwBool result = TRUE;
    RwUInt32 n;

    n = nodeDef->io.numOutputs;

    memset(node, 0, sizeof(*node));
    if (n > 0x20) {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x29);
        RwErrorSet(&error);
        result = FALSE;
    }
    if (nodeDef->io.numClustersOfInterest > 0x20) {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x28);
        RwErrorSet(&error);
        result = FALSE;
    }
    /*
     * Retail also materializes and discards the comparison between the
     * configured and runtime maximum node counts, consistent with a stripped
     * SDK assertion. The live bound check below is otherwise exact.
     */
    if (n >= RXPIPELINEGLOBAL(maxNodes)) {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x2A);
        RwErrorSet(&error);
        result = FALSE;
    }
    if (result) {
        outputs = (RwUInt32*)&pipeline->nodes[RXPIPELINEGLOBAL(maxNodes)];
        outputs += pipeline->numNodes * 0x20;
        node->outputs = outputs;
        node->numOutputs = n;
        for (n = 0; n < node->numOutputs; n++) {
            *outputs = (RwUInt32)-1;
            outputs++;
        }
        outputs = (RwUInt32*)&pipeline->nodes[RXPIPELINEGLOBAL(maxNodes)];
        topSortData = (RxPipelineNodeTopSortData*)&outputs[
            RXPIPELINEGLOBAL(maxNodes) * 0x20];
        topSortData += pipeline->numNodes;
        topSortData->numIns = 0;
        topSortData->numInsVisited = 0;
        topSortData->req = NULL;
        node->topSortData = topSortData;
        node->initializationData = NULL;
        node->initializationDataSize = 0;
        node->nodeDef = nodeDef;
        pipeline->numNodes++;
    }
    return result;
}

static void PipelineTallyInputs(RxPipeline* pipeline)
{
    RxPipelineNode* node = pipeline->nodes;
    RwUInt32 index;

    for (index = 0; index < pipeline->numNodes;) {
        if (node->nodeDef != NULL) {
            node->topSortData->numInsVisited = 0;
            node->topSortData->numIns = 0;
        }
        node++;
        index++;
    }
    node = pipeline->nodes;
    for (index = 0; index < pipeline->numNodes;) {
        if (node->nodeDef != NULL && node->numOutputs != 0) {
            RwInt32 remaining;
            RwUInt32 outputsRemaining = node->numOutputs;
            RwUInt32* output = node->outputs;
            do {
                if ((RwInt32)*output != -1) {
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
    RwUInt32 nodesArraySlot;
} PipelineTopSortState;

static void PipelineTopSort(PipelineTopSortState* data, RwUInt32 nodeIndex)
{
    /* Canonical RenderWare source retained. Retail lowers the fixed 0x0c and
     * 0x28 aggregate swaps as CTR copies; this compiler unrolls them. */
    RxPipelineNode* currentNode;
    RwUInt32 i = data->nodesArraySlot;
    RwUInt32 j = nodeIndex;

    if (i != j) {
        RwUInt32 temporaryOutput;
        RwUInt32* outputsI;
        RwUInt32* outputsJ;
        RxPipelineNodeTopSortData temporaryTopSortData;
        RxPipelineNodeTopSortData* topSortDataI;
        RxPipelineNodeTopSortData* topSortDataJ;
        RxPipelineNode temporaryNode;
        RwUInt32 k;
        RwUInt32 l;

        outputsI = data->pipeline->nodes[i].outputs;
        outputsJ = data->pipeline->nodes[j].outputs;
        for (k = 0; k < 0x20; k++) {
            temporaryOutput = outputsI[k];
            outputsI[k] = outputsJ[k];
            outputsJ[k] = temporaryOutput;
        }
        data->pipeline->nodes[i].outputs = outputsJ;
        data->pipeline->nodes[j].outputs = outputsI;

        topSortDataI = data->pipeline->nodes[i].topSortData;
        topSortDataJ = data->pipeline->nodes[j].topSortData;
        temporaryTopSortData = *topSortDataI;
        *topSortDataI = *topSortDataJ;
        *topSortDataJ = temporaryTopSortData;
        data->pipeline->nodes[i].topSortData = topSortDataJ;
        data->pipeline->nodes[j].topSortData = topSortDataI;

        temporaryNode = data->pipeline->nodes[i];
        data->pipeline->nodes[i] = data->pipeline->nodes[j];
        data->pipeline->nodes[j] = temporaryNode;

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

    currentNode = &data->pipeline->nodes[data->nodesArraySlot];
    data->nodesArraySlot++;
    if (currentNode->numOutputs != 0) {
        for (i = 0; i < currentNode->numOutputs; i++) {
            RwUInt32 outputIndex = currentNode->outputs[i];

            if (outputIndex != (RwUInt32)-1) {
                RxPipelineNode* outputNode = &data->pipeline->nodes[outputIndex];
                outputNode->topSortData->numInsVisited++;
                if (outputNode->topSortData->numIns ==
                    outputNode->topSortData->numInsVisited) {
                    PipelineTopSort(data, outputIndex);
                }
            }
        }
    }
}

static RwUInt32 PipelineNode2Index(RxPipeline* pipeline, RxPipelineNode* node)
{
    RwUInt32 index = node - pipeline->nodes;

    if (&pipeline->nodes[index] == node &&
        index < pipeline->numNodes) {
        return index;
    }
    return -1;
}

static RxPipeline* PipelineUnlockTopSort(RxPipeline* pipeline)
{
    PipelineTopSortState state;
    RwUInt32 index;

    state.pipeline = pipeline;
    state.nodesArraySlot = 0;
    PipelineTallyInputs(pipeline);
    if (pipeline->nodes[pipeline->entryPoint].topSortData->numIns != 0) {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x24);
        RwErrorSet(&error);
        return NULL;
    }
    for (index = 0; index < pipeline->numNodes; index++) {
        if (index != pipeline->entryPoint &&
            pipeline->nodes[index].topSortData->numIns == 0) {
            RwError error;
            error.pluginID = 1;
            error.errorCode = _rwerror(0x22);
            RwErrorSet(&error);
            return NULL;
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
            return NULL;
        }
    }
    pipeline->entryPoint = 0;
    return pipeline;
}

RxNodeOutput RxPipelineNodeFindOutputByIndex(RxPipelineNode* node,
                                             RwUInt32 outputIndex)
{
    if (node != NULL && node->nodeDef != NULL &&
        outputIndex < node->numOutputs) {
        return &node->outputs[outputIndex];
    }
    return NULL;
}

RxNodeInput RxPipelineNodeFindInput(RxPipelineNode* node)
{
    if (node != NULL && node->nodeDef != NULL) {
        return node;
    }
    return NULL;
}

/* Retail shares the shrinkwrap and callback rollback edges through goto fail.
 * This structured form preserves that cleanup but duplicates the final edge. */
RxPipeline* RxLockedPipeUnlock(RxLockedPipe* pipeline)
{
    RxPipelineNodeTopSortData* newTopSortData;
    RxPipelineNodeTopSortData* topSortData;
    RwUInt32 numUniqueClusters;
    RwUInt32 unlockStartBlockSize;
    RwUInt32 unlockEndBlockSize;
    RwUInt32 topSortBlockSize;
    RwUInt32 depChaseBlockSize;
    RwUInt32 totalOutputs;
    RwUInt32* newOutputs;
    RwUInt32* outputs;
    RwBool error;
    RwUInt32 doneNodes;
    RwInt32 i;

    if (pipeline != NULL && pipeline->locked) {
      if (pipeline->numNodes != 0) {
        totalOutputs = 0;
        error = FALSE;
        doneNodes = 0;
        if (pipeline->entryPoint >= pipeline->numNodes ||
            pipeline->nodes[pipeline->entryPoint].nodeDef == NULL) {
            RwError error;
            error.pluginID = 1;
            error.errorCode = _rwerror(0x24);
            RwErrorSet(&error);
            return NULL;
        }

        numUniqueClusters = PipelineCalcNumUniqueClusters(pipeline);
        topSortBlockSize =
            RXPIPELINEGLOBAL(maxNodes) * sizeof(RxPipelineNodeTopSortData) +
            (RXPIPELINEGLOBAL(maxNodes) * sizeof(RxPipelineNode) +
             RXPIPELINEGLOBAL(maxNodes) * 0x80);
        depChaseBlockSize =
            RXPIPELINEGLOBAL(maxNodes) * sizeof(RxPipelineNodeTopSortData);
        depChaseBlockSize += CalcNodesOutputsCompactedMemSize(pipeline);
        depChaseBlockSize += pipeline->numNodes * 0x14;
        depChaseBlockSize += pipeline->numNodes * numUniqueClusters * 0x24;
        depChaseBlockSize += pipeline->numNodes * numUniqueClusters * 0x10;
        depChaseBlockSize +=
            CalcUnlockPersistentMemSize(pipeline, numUniqueClusters);
        unlockStartBlockSize = topSortBlockSize;
        if (depChaseBlockSize > unlockStartBlockSize) {
            unlockStartBlockSize = depChaseBlockSize;
        }
        if (unlockStartBlockSize > pipeline->superBlockSize &&
            !ReallocAndFixupSuperBlock(pipeline, unlockStartBlockSize)) {
            return NULL;
        }

        gMemoryLimits.stalactite =
            (RwUInt8*)pipeline->superBlock + unlockStartBlockSize;
        gMemoryLimits.stalagmite = pipeline->superBlock;
        pipeline = PipelineUnlockTopSort(pipeline);
        if (pipeline == NULL) {
            return NULL;
        }

        outputs = (RwUInt32*)&pipeline->nodes[RXPIPELINEGLOBAL(maxNodes)];
        topSortData = (RxPipelineNodeTopSortData*)&outputs[
            0x20 * RXPIPELINEGLOBAL(maxNodes)];
        topSortData += pipeline->numNodes - 1;
        newTopSortData = (RxPipelineNodeTopSortData*)(
            (RwUInt8*)pipeline->superBlock + unlockStartBlockSize);
        newTopSortData--;
        for (i = (RwInt32)pipeline->numNodes - 1; i >= 0; i--) {
            memcpy(newTopSortData, topSortData, sizeof(*newTopSortData));
            pipeline->nodes[i].topSortData = newTopSortData;
            topSortData--;
            newTopSortData--;
        }
        newOutputs = (RwUInt32*)&pipeline->nodes[pipeline->numNodes];
        for (i = 0; (RwUInt32)i < pipeline->numNodes; i++) {
            if (pipeline->nodes[i].numOutputs == 0) {
                pipeline->nodes[i].outputs = NULL;
            } else {
                memcpy(newOutputs, outputs,
                       pipeline->nodes[i].numOutputs * sizeof(RwUInt32));
                pipeline->nodes[i].outputs = newOutputs;
            }
            outputs += 0x20;
            newOutputs += pipeline->nodes[i].numOutputs;
            totalOutputs += pipeline->nodes[i].numOutputs;
        }
        gMemoryLimits.stalagmite = (RwUInt8*)&outputs[totalOutputs];
        gMemoryLimits.stalactite = (RwUInt8*)topSortData;

        if (_rxChaseDependencies(pipeline) != 0) {
            return NULL;
        }
        unlockEndBlockSize = (RwUInt32)(
            gMemoryLimits.stalagmite - (RwUInt8*)pipeline->superBlock);
        if (!ReallocAndFixupSuperBlock(pipeline, unlockEndBlockSize)) {
            LockPipelineExpandData(pipeline, pipeline);
            return NULL;
        }
        for (i = 0; (RwUInt32)i < pipeline->numNodes; i++) {
            pipeline->nodes[i].topSortData = NULL;
        }

        for (i = (RwInt32)pipeline->numNodes - 1; i >= 0; i--) {
                RxNodeDefinition* nodeDef = pipeline->nodes[i].nodeDef;
                RxPipelineNode* node = &pipeline->nodes[i];
                RwInt32 oldCount = nodeDef->InputPipesCnt++;

                if (oldCount == 0) {
                    if (nodeDef->nodeMethods.nodeInit != NULL) {
                        if (!nodeDef->nodeMethods.nodeInit(nodeDef)) {
                            doneNodes =
                                (pipeline->numNodes - 1) - (RwUInt32)i;
                            error = TRUE;
                            break;
                        }
                    }
                }
                if (nodeDef->nodeMethods.pipelineNodeInit != NULL) {
                    if (!nodeDef->nodeMethods.pipelineNodeInit(node)) {
                        nodeDef->InputPipesCnt--;
                        if (nodeDef->InputPipesCnt == 0) {
                            if (nodeDef->nodeMethods.nodeTerm != NULL) {
                                nodeDef->nodeMethods.nodeTerm(nodeDef);
                            }
                        }
                        doneNodes = (pipeline->numNodes - 1) - (RwUInt32)i;
                        error = TRUE;
                        break;
                    }
                }
        }

        if (!error) {
            for (i = (RwInt32)pipeline->numNodes - 1; i >= 0; i--) {
                RxNodeDefinition* nodeDef = pipeline->nodes[i].nodeDef;
                RxPipelineNode* node = &pipeline->nodes[i];
                if (nodeDef->nodeMethods.pipelineNodeConfig != NULL) {
                    if (!nodeDef->nodeMethods.pipelineNodeConfig(node,
                                                                 pipeline)) {
                        doneNodes = pipeline->numNodes;
                        error = TRUE;
                        break;
                    }
                }
            }
        }

        if (error) {
            for (i = (pipeline->numNodes - 1) - (doneNodes - 1);
                 (RwUInt32)i < pipeline->numNodes; i++) {
                RxNodeDefinition* nodeDef = pipeline->nodes[i].nodeDef;
                RxPipelineNode* node = &pipeline->nodes[i];
                if (nodeDef->nodeMethods.pipelineNodeTerm != NULL) {
                    nodeDef->nodeMethods.pipelineNodeTerm(node);
                }
                nodeDef->InputPipesCnt--;
                if (nodeDef->InputPipesCnt == 0) {
                    if (nodeDef->nodeMethods.nodeTerm != NULL) {
                        nodeDef->nodeMethods.nodeTerm(nodeDef);
                    }
                }
            }
            LockPipelineExpandData(pipeline, pipeline);
            return NULL;
        }
      }

        pipeline->locked = FALSE;
        return pipeline;
    }
    if (pipeline == NULL) {
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
    return NULL;
}

RxLockedPipe* RxPipelineLock(RxPipeline* pipeline)
{
    /*
     * Retail keeps the decremented input-pipe count in r0; this clean named
     * result uses r27. All live operations and the function size otherwise
     * match exactly.
     */
    if (!pipeline->locked) {
        RwUInt32 requiredSize =
            RXPIPELINEGLOBAL(maxNodes) *
                sizeof(RxPipelineNodeTopSortData) +
            (RXPIPELINEGLOBAL(maxNodes) * sizeof(RxPipelineNode) +
             RXPIPELINEGLOBAL(maxNodes) * 0x80);
        RwUInt32 index;
        if (pipeline->nodes != NULL) {
            if (requiredSize > pipeline->superBlockSize &&
                !ReallocAndFixupSuperBlock(pipeline, requiredSize)) {
                return NULL;
            }
            if (!LockPipelineExpandData(pipeline, pipeline)) {
                return NULL;
            }
        } else {
            pipeline->superBlock = RwEngineInstance->fpMalloc(
                requiredSize, 0x01030409);
            if (pipeline->superBlock == NULL) {
                RwError error;
                error.pluginID = 1;
                error.errorCode = _rwerror(0x80000013, requiredSize);
                RwErrorSet(&error);
                return NULL;
            }
            pipeline->superBlockSize = requiredSize;
            pipeline->nodes = pipeline->superBlock;
        }
        pipeline->locked = TRUE;
        if (pipeline->nodes != NULL) {
            for (index = 0; index < pipeline->numNodes; index++) {
                RxNodeMethods* nodeMethods =
                    &pipeline->nodes[index].nodeDef->nodeMethods;
                RwInt32 inputPipesCount;
                if (nodeMethods->pipelineNodeTerm != NULL) {
                    nodeMethods->pipelineNodeTerm(&pipeline->nodes[index]);
                }
                inputPipesCount =
                    --pipeline->nodes[index].nodeDef->InputPipesCnt;
                if (inputPipesCount == 0 && nodeMethods->nodeTerm != NULL) {
                    nodeMethods->nodeTerm(pipeline->nodes[index].nodeDef);
                }
                pipeline->nodes[index].slotClusterRefs = NULL;
            }
        }
    }
    return pipeline;
}

/* Retail carries one additional copy of the staged validity predicate into a
 * third nonvolatile register. The clean two-stage validation below recovers
 * every search/callback/index operation without retaining that redundant
 * boolean lifetime. */
RxPipelineNode* RxPipelineFindNodeByName(RxPipeline* pipeline,
                                         const RwChar* name,
                                         RxPipelineNode* start,
                                         RwInt32* nodeIndex)
{
    RwBool hasNodes = FALSE;
    RwBool validArguments = FALSE;

    if (pipeline != NULL && name != NULL) {
        validArguments = TRUE;
    }
    if (validArguments && pipeline->numNodes != 0) {
        hasNodes = TRUE;
    }
    if (hasNodes) {
        RxPipelineNode* node = pipeline->nodes;
        RwInt32 remaining = pipeline->numNodes;

        if (start != NULL) {
            while (node != start && remaining > 0) {
                node++;
                remaining--;
            }
            node++;
            remaining--;
        }
        while (remaining > 0) {
            if (node->nodeDef != NULL &&
                RwEngineInstance->stringFuncs.vecStrcmp(node->nodeDef->name,
                                                        name) == 0) {
                if (nodeIndex != NULL) {
                    *nodeIndex = pipeline->numNodes - remaining;
                }
                return node;
            }
            node++;
            remaining--;
        }
    }
    if (nodeIndex != NULL) {
        *nodeIndex = -1;
    }
    return NULL;
}

/*
 * Retail colors firstIndex/nodeDef0 as r23/r24; clean source uses r24/r23.
 * The complete function is otherwise instruction- and size-identical.
 */
RxLockedPipe* RxLockedPipeAddFragment(RxLockedPipe* pipeline,
                                      RwUInt32* firstIndex,
                                      RxNodeDefinition* nodeDef0, ...)
{
    __va_list arguments;
    RxNodeDefinition* nodeDef;
    RwUInt32 count;
    RwUInt32 created;
    RwUInt32 firstNode;
    RxPipelineNode* previous;

    if (pipeline != NULL && pipeline->locked) {
        count = 0;
        va_start(arguments, nodeDef0);
        nodeDef = nodeDef0;
        while (nodeDef != NULL) {
            count++;
            nodeDef = *(RxNodeDefinition**)__va_arg(arguments, 1);
        }
        va_end(arguments);
        if (count != 0) {
            previous = NULL;
            if (pipeline->numNodes + count > RXPIPELINEGLOBAL(maxNodes)) {
                RwError error;
                error.pluginID = 1;
                error.errorCode = _rwerror(0x2A);
                RwErrorSet(&error);
                return NULL;
            }

            firstNode = pipeline->numNodes;
            created = 0;
            va_start(arguments, nodeDef0);
            nodeDef = nodeDef0;
            while (nodeDef != NULL) {
                RxPipelineNode* node =
                    &pipeline->nodes[firstNode + created];
                if (!_NodeCreate(pipeline, node, nodeDef)) {
                    break;
                }
                if (previous != NULL &&
                    RxLockedPipeAddPath(
                        pipeline,
                        RxPipelineNodeFindOutputByIndex(previous, 0),
                        RxPipelineNodeFindInput(node)) == NULL) {
                    PipelineNodeDestroy(node, pipeline);
                    break;
                }
                previous = node;
                created++;
                nodeDef = *(RxNodeDefinition**)__va_arg(arguments, 1);
            }
            va_end(arguments);

            if (created == count) {
                if (firstIndex != NULL) {
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
        if (pipeline == NULL) {
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
    return NULL;
}

RxPipeline* RxLockedPipeAddPath(RxLockedPipe* pipeline, RxNodeOutput output,
                                RxNodeInput input)
{
    if (pipeline != NULL && pipeline->locked && output != NULL &&
        *output == (RwUInt32)-1 && input != NULL && input->nodeDef != NULL) {
        RwUInt32 inputIndex = PipelineNode2Index(pipeline, input);
        if (inputIndex != (RwUInt32)-1) {
            *output = inputIndex;
            return pipeline;
        }
    }
    /*
     * Retail recomputes and discards the input-node and disconnected-output
     * predicates on this failure path, consistent with stripped assertions.
     */
    return NULL;
}
