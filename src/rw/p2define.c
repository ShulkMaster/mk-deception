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
    RwUInt32 count = 0;
    RwUInt32 next = 0;

    for (;;) {
        RwUInt32 previous = next;
        RwUInt32 nodeIndex;

        next = (RwUInt32)-1;
        for (nodeIndex = 0; nodeIndex < pipeline->numNodes; nodeIndex++) {
            RwUInt32 clusterIndex;
            RxNodeDefinition* nodeDef = pipeline->nodes[nodeIndex].nodeDef;

            for (clusterIndex = 0;
                 clusterIndex < nodeDef->io.numClustersOfInterest;
                 clusterIndex++) {
                RwUInt32 cluster =
                    (RwUInt32)nodeDef->io.clustersOfInterest[clusterIndex]
                        .clusterDef;
                if (cluster > previous && cluster < next) {
                    next = cluster;
                }
            }
        }
        {
            RwUInt32 invalid = (RwUInt32)-1;
            if (next != invalid) {
                count++;
            } else {
                break;
            }
        }
    }
    return count;
}

static RwBool ReallocAndFixupSuperBlock(RxPipeline* pipeline,
                                        RwUInt32 newSize)
{
    RwUInt8* oldBlock = pipeline->superBlock;
    RwUInt8* newBlock = RwEngineInstance->fpRealloc(
        oldBlock, newSize, 0x01040409);

    if (newBlock != NULL) {
        RwInt32 displacement = newBlock - oldBlock;
        RwUInt32 index;

        pipeline->superBlock = newBlock;
        pipeline->superBlockSize = newSize;
        pipeline->nodes = (RxPipelineNode*)newBlock;
        if (pipeline->inputRequirements != NULL) {
            pipeline->inputRequirements = (RxPipelineRequiresCluster*)(
                (RwUInt8*)pipeline->inputRequirements + displacement);
        }
        if (pipeline->embeddedPacket != NULL) {
            pipeline->embeddedPacket = (RxPacket*)(
                (RwUInt8*)pipeline->embeddedPacket + displacement);
        }
        for (index = 0; index < pipeline->numNodes; index++) {
            RxPipelineNode* node = &pipeline->nodes[index];
            if (node->outputs != NULL) {
                node->outputs = (RwUInt32*)((RwUInt8*)node->outputs +
                                            displacement);
            }
            if (node->slotClusterRefs != NULL) {
                node->slotClusterRefs = (RxPipelineCluster**)(
                    (RwUInt8*)node->slotClusterRefs + displacement);
            }
            if (node->slotsContinue != NULL) {
                node->slotsContinue = (RwUInt32*)(
                    (RwUInt8*)node->slotsContinue + displacement);
            }
            if (node->privateData != NULL) {
                node->privateData =
                    (RwUInt8*)node->privateData + displacement;
            }
            if (node->inputToClusterSlot != NULL) {
                node->inputToClusterSlot = (RwUInt32*)(
                    (RwUInt8*)node->inputToClusterSlot + displacement);
            }
            if (node->topSortData != NULL) {
                node->topSortData = (RxPipelineNodeTopSortData*)(
                    (RwUInt8*)node->topSortData + displacement);
            }
        }
        return TRUE;
    }
    {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x80000013, newSize);
        RwErrorSet(&error);
    }
    return FALSE;
}

static RwBool LockPipelineExpandData(RxPipeline* destination,
                                     const RxPipeline* source)
{
    RwUInt32 maxNodes = RXPIPELINEGLOBAL(maxNodes);
    RwInt32 index;
    RwUInt32* outputsBase;
    RxPipelineNodeTopSortData* topSortBase;

    if (destination != source) {
        for (index = (RwInt32)source->numNodes - 1; index >= 0; index--) {
            RxPipelineNode* destinationNode = &destination->nodes[index];
            const RxPipelineNode* sourceNode = &source->nodes[index];
            *destinationNode = *sourceNode;
            destinationNode->slotClusterRefs = NULL;
            destinationNode->slotsContinue = NULL;
            destinationNode->privateData = NULL;
            destinationNode->inputToClusterSlot = NULL;
            if (destinationNode->initializationDataSize != 0) {
                destinationNode->initializationData =
                    RwEngineInstance->fpMalloc(
                        destinationNode->initializationDataSize, 0x30409);
                if (destinationNode->initializationData == NULL) {
                    RwError error;
                    error.pluginID = 1;
                    error.errorCode = _rwerror(
                        0x80000013,
                        destinationNode->initializationDataSize);
                    RwErrorSet(&error);
                    return FALSE;
                }
                memcpy(destinationNode->initializationData,
                       sourceNode->initializationData,
                       destinationNode->initializationDataSize);
            }
        }
        destination->numNodes = source->numNodes;
    }

    outputsBase = (RwUInt32*)((RwUInt8*)destination->nodes +
                              maxNodes * sizeof(RxPipelineNode));
    for (index = (RwInt32)source->numNodes - 1; index >= 0; index--) {
        RxPipelineNode* destinationNode = &destination->nodes[index];
        const RxPipelineNode* sourceNode = &source->nodes[index];
        destinationNode->outputs = outputsBase + index * 0x20;
        if (sourceNode->outputs != NULL) {
            memcpy(destinationNode->outputs, sourceNode->outputs,
                   0x20 * sizeof(RwUInt32));
        }
    }

    topSortBase = (RxPipelineNodeTopSortData*)(outputsBase + maxNodes * 0x20);
    for (index = 0; index < (RwInt32)source->numNodes; index++) {
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
    RwBool result = TRUE;
    RwUInt32 numOutputs = nodeDef->io.numOutputs;
    RwUInt32 index;

    memset(node, 0, sizeof(*node));
    if (numOutputs > 0x20) {
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
    if (numOutputs >= RXPIPELINEGLOBAL(maxNodes)) {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x2A);
        RwErrorSet(&error);
        result = FALSE;
    }
    if (result) {
        RwUInt32* outputs = (RwUInt32*)((RwUInt8*)pipeline->nodes +
                                        RXPIPELINEGLOBAL(maxNodes) *
                                            sizeof(RxPipelineNode));
        RxPipelineNodeTopSortData* topSortData;
        outputs = (RwUInt32*)((RwUInt8*)outputs +
                              pipeline->numNodes * 0x20 * sizeof(RwUInt32));
        node->outputs = outputs;
        node->numOutputs = numOutputs;
        for (index = 0; index < node->numOutputs; index++) {
            *outputs = (RwUInt32)-1;
            outputs++;
        }
        topSortData = (RxPipelineNodeTopSortData*)((RwUInt8*)pipeline->nodes +
                                                   RXPIPELINEGLOBAL(maxNodes) *
                                                       sizeof(RxPipelineNode));
        topSortData = (RxPipelineNodeTopSortData*)((RwUInt8*)topSortData +
                                                   RXPIPELINEGLOBAL(maxNodes) *
                                                       0x20 * sizeof(RwUInt32));
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
    RwUInt32 numSorted;
} PipelineTopSortState;

static void PipelineTopSort(PipelineTopSortState* state, RwUInt32 nodeIndex)
{
    RxPipeline* pipeline = state->pipeline;
    RwUInt32 destinationIndex = state->numSorted;
    RxPipelineNode* destination;
    RwUInt32 outputIndex;

    if (destinationIndex != nodeIndex) {
        RxPipelineNode* source = &pipeline->nodes[nodeIndex];
        RwUInt32* destinationOutputs =
            pipeline->nodes[destinationIndex].outputs;
        RwUInt32* sourceOutputs = source->outputs;
        RxPipelineNodeTopSortData* destinationSort =
            pipeline->nodes[destinationIndex].topSortData;
        RxPipelineNodeTopSortData* sourceSort = source->topSortData;
        RxPipelineNode temporaryNode;
        RxPipelineNodeTopSortData temporarySort;

        for (outputIndex = 0; outputIndex < 0x20; outputIndex++) {
            RwUInt32 temporary = destinationOutputs[outputIndex];
            destinationOutputs[outputIndex] = sourceOutputs[outputIndex];
            sourceOutputs[outputIndex] = temporary;
        }
        pipeline->nodes[destinationIndex].outputs = sourceOutputs;
        source->outputs = destinationOutputs;
        temporarySort = *destinationSort;
        *destinationSort = *sourceSort;
        *sourceSort = temporarySort;
        pipeline->nodes[destinationIndex].topSortData = sourceSort;
        source->topSortData = destinationSort;
        temporaryNode = pipeline->nodes[destinationIndex];
        pipeline->nodes[destinationIndex] = *source;
        *source = temporaryNode;

        for (outputIndex = 0; outputIndex < pipeline->numNodes;
             outputIndex++) {
            RxPipelineNode* node = &pipeline->nodes[outputIndex];
            RwUInt32 output;
            for (output = 0; output < node->numOutputs; output++) {
                if (node->outputs[output] == destinationIndex) {
                    node->outputs[output] = nodeIndex;
                } else if (node->outputs[output] == nodeIndex) {
                    node->outputs[output] = destinationIndex;
                }
            }
        }
    }

    destination = &pipeline->nodes[state->numSorted++];
    for (outputIndex = 0; outputIndex < destination->numOutputs;
         outputIndex++) {
        RwUInt32 output = destination->outputs[outputIndex];
        if (output + 0x10000U != (RwUInt32)-1) {
            RxPipelineNode* next = &pipeline->nodes[output];
            next->topSortData->numInsVisited++;
            if (next->topSortData->numIns ==
                next->topSortData->numInsVisited) {
                PipelineTopSort(state, output);
            }
        }
    }
}

static RwUInt32 PipelineNode2Index(const RxPipeline* pipeline,
                                  const RxPipelineNode* node)
{
    /* Retail uses signed divw; MWCC strength-reduces this clean constant form. */
    RwInt32 index =
        (RwInt32)((RwUInt32)node - (RwUInt32)pipeline->nodes) /
        (RwInt32)sizeof(RxPipelineNode);
    if (&pipeline->nodes[index] == node &&
        (RwUInt32)index < pipeline->numNodes) {
        return index;
    }
    return -1;
}

static RxPipeline* PipelineUnlockTopSort(RxPipeline* pipeline)
{
    PipelineTopSortState state;
    RwUInt32 index;

    state.pipeline = pipeline;
    state.numSorted = 0;
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

RxPipeline* RxLockedPipeUnlock(RxLockedPipe* pipeline)
{
    RwUInt32 numClusters;
    RwUInt32 maxNodes = RXPIPELINEGLOBAL(maxNodes);
    RwUInt32 lockedSize;
    RwUInt32 workingSize;
    RwUInt32 allocationSize;
    RwUInt32 outputsCount = 0;
    RwUInt32 initializedCount = 0;
    RwBool failed = FALSE;
    RwUInt32 index;

    if (pipeline == NULL || !pipeline->locked) {
        RwError error;
        error.pluginID = 1;
        error.errorCode = pipeline == NULL ? _rwerror(0x80000016)
                                           : _rwerror(0x34);
        RwErrorSet(&error);
        return NULL;
    }
    if (pipeline->numNodes == 0) {
        pipeline->locked = FALSE;
        return pipeline;
    }
    if (pipeline->entryPoint >= pipeline->numNodes ||
        pipeline->nodes[pipeline->entryPoint].nodeDef == NULL) {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x24);
        RwErrorSet(&error);
        return NULL;
    }

    numClusters = PipelineCalcNumUniqueClusters(pipeline);
    lockedSize = maxNodes * sizeof(RxPipelineNode) + maxNodes * 0x80 +
                 maxNodes * sizeof(RxPipelineNodeTopSortData);
    workingSize =
        pipeline->numNodes * 0x14 +
        pipeline->numNodes * numClusters * 0x24 +
        pipeline->numNodes * numClusters * 0x10 +
        maxNodes * sizeof(RxPipelineNodeTopSortData) +
        CalcNodesOutputsCompactedMemSize(pipeline) +
        CalcUnlockPersistentMemSize(pipeline, numClusters);
    allocationSize = workingSize > lockedSize ? workingSize : lockedSize;
    if (allocationSize > pipeline->superBlockSize &&
        !ReallocAndFixupSuperBlock(pipeline, allocationSize)) {
        return NULL;
    }

    gMemoryLimits.stalactite =
        (RwUInt8*)pipeline->superBlock + allocationSize;
    gMemoryLimits.stalagmite = pipeline->superBlock;
    if (PipelineUnlockTopSort(pipeline) == NULL) {
        return NULL;
    }

    {
        RwUInt8* expandedOutputs =
            (RwUInt8*)pipeline->nodes + maxNodes * sizeof(RxPipelineNode);
        RxPipelineNodeTopSortData* expandedSort =
            (RxPipelineNodeTopSortData*)(expandedOutputs + maxNodes * 0x80);
        RxPipelineNodeTopSortData* compactSort =
            (RxPipelineNodeTopSortData*)((RwUInt8*)pipeline->superBlock +
                                         allocationSize) -
            pipeline->numNodes;
        RwInt32 reverse;
        RwUInt8* compactOutputs =
            (RwUInt8*)pipeline->nodes +
            pipeline->numNodes * sizeof(RxPipelineNode);

        for (reverse = (RwInt32)pipeline->numNodes - 1; reverse >= 0;
             reverse--) {
            compactSort[reverse] = expandedSort[reverse];
            pipeline->nodes[reverse].topSortData = &compactSort[reverse];
        }
        for (index = 0; index < pipeline->numNodes; index++) {
            RxPipelineNode* node = &pipeline->nodes[index];
            if (node->numOutputs == 0) {
                node->outputs = NULL;
            } else {
                memcpy(compactOutputs, expandedOutputs,
                       node->numOutputs * sizeof(RwUInt32));
                node->outputs = (RwUInt32*)compactOutputs;
            }
            expandedOutputs += 0x80;
            compactOutputs += node->numOutputs * sizeof(RwUInt32);
            outputsCount += node->numOutputs;
        }
        gMemoryLimits.stalagmite =
            expandedOutputs + outputsCount * sizeof(RwUInt32);
        gMemoryLimits.stalactite =
            (RwUInt8*)compactSort - sizeof(RxPipelineNodeTopSortData);
    }

    if (_rxChaseDependencies(pipeline) != 0) {
        return NULL;
    }
    {
        RwUInt32 finalSize =
            (RwUInt32)(gMemoryLimits.stalagmite -
                       (RwUInt8*)pipeline->superBlock);
        if (!ReallocAndFixupSuperBlock(pipeline, finalSize)) {
            return NULL;
        }
    }
    for (index = 0; index < pipeline->numNodes; index++) {
        pipeline->nodes[index].topSortData = NULL;
    }

    for (index = pipeline->numNodes; index != 0; index--) {
        RxPipelineNode* node = &pipeline->nodes[index - 1];
        RxNodeDefinition* nodeDef = node->nodeDef;
        RwInt32 oldCount = nodeDef->InputPipesCnt++;

        if ((oldCount == 0 && nodeDef->nodeMethods.nodeInit != NULL &&
             !nodeDef->nodeMethods.nodeInit(nodeDef)) ||
            (nodeDef->nodeMethods.pipelineNodeInit != NULL &&
             !nodeDef->nodeMethods.pipelineNodeInit(node))) {
            if (oldCount == 0) {
                nodeDef->InputPipesCnt--;
                if (nodeDef->InputPipesCnt == 0 &&
                    nodeDef->nodeMethods.nodeTerm != NULL) {
                    nodeDef->nodeMethods.nodeTerm(nodeDef);
                }
            }
            initializedCount = pipeline->numNodes - index;
            failed = TRUE;
            break;
        }
    }

    if (!failed) {
        for (index = pipeline->numNodes; index != 0; index--) {
            RxPipelineNode* node = &pipeline->nodes[index - 1];
            if (node->nodeDef->nodeMethods.pipelineNodeConfig != NULL &&
                !node->nodeDef->nodeMethods.pipelineNodeConfig(node,
                                                               pipeline)) {
                initializedCount = pipeline->numNodes;
                failed = TRUE;
                break;
            }
        }
    }

    if (failed) {
        RwUInt32 first = pipeline->numNodes - initializedCount;
        for (index = first; index < pipeline->numNodes; index++) {
            RxPipelineNode* node = &pipeline->nodes[index];
            RxNodeDefinition* nodeDef = node->nodeDef;
            if (nodeDef->nodeMethods.pipelineNodeTerm != NULL) {
                nodeDef->nodeMethods.pipelineNodeTerm(node);
            }
            nodeDef->InputPipesCnt--;
            if (nodeDef->InputPipesCnt == 0 &&
                nodeDef->nodeMethods.nodeTerm != NULL) {
                nodeDef->nodeMethods.nodeTerm(nodeDef);
            }
        }
        LockPipelineExpandData(pipeline, pipeline);
        return NULL;
    }

    pipeline->locked = FALSE;
    return pipeline;
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
