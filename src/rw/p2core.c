#include "libmkparticle/rw_engine.h"
#include "rw/rwerror.h"
#include "rw/rwfreelist.h"
#include "rw/rxpipeline.h"
#include "runtime/cstring.h"

extern RxExecutionContext _rxExecCtxGlobal;

static RwFreeList _rxPipesFreeList;
RwUInt32 _rxHeapInitialSize = 0x1000;
RwUInt32 _rxPipelineMaxNodes = 0x40;
static RwInt32 _rxPipesFreeListBlockSize = 0x40;
static RwInt32 _rxPipesFreeListPreallocBlocks = 1;
RxHeap* _rxHeapGlobal;
RwBool RxPipelineInstanced;

RwBool _rxPipelineClose(void)
{
    if (RxPipelineInstanced) {
        RwFreeListDestroy(RXPIPELINEGLOBAL(pipelines));
        RXPIPELINEGLOBAL(pipelines) = NULL;
        RxHeapDestroy(_rxHeapGlobal);
        _rxHeapGlobal = NULL;
        RxPipelineInstanced = FALSE;
    }
    return TRUE;
}

RwBool _rxPipelineOpen(void)
{
    if (!RxPipelineInstanced) {
        _rxHeapGlobal = RxHeapCreate(_rxHeapInitialSize);
        if (_rxHeapGlobal == NULL) {
            return FALSE;
        }

        RXPIPELINEGLOBAL(pipelines) = RwFreeListCreateAndPreallocateSpace(
            sizeof(RxPipeline), _rxPipesFreeListBlockSize, 4,
            _rxPipesFreeListPreallocBlocks, &_rxPipesFreeList, 0x40409);
        if (RXPIPELINEGLOBAL(pipelines) == NULL) {
            RxHeapDestroy(_rxHeapGlobal);
            _rxHeapGlobal = NULL;
            return FALSE;
        }

        RXPIPELINEGLOBAL(maxNodes) = _rxPipelineMaxNodes;
        RxRenderStateVectorSetDefaultRenderStateVector(
            &RXPIPELINEGLOBAL(defaultRenderState));
        RXPIPELINEGLOBAL(currentNode) = NULL;
        RXPIPELINEGLOBAL(currentPipeline) = NULL;
        RxPipelineInstanced = TRUE;
        return TRUE;
    }
    return FALSE;
}

/* Near miss: exact compaction/repair; remaining differences are pointer
 * expression scheduling and register allocation. */
RxPipelineNode* PipelineNodeDestroy(RxPipelineNode* node,
                                    RxPipeline* pipeline)
{
    if (!pipeline->locked) {
        if (node->nodeDef->nodeMethods.pipelineNodeTerm != NULL) {
            node->nodeDef->nodeMethods.pipelineNodeTerm(node);
        }
        --node->nodeDef->InputPipesCnt;
        if (node->nodeDef->InputPipesCnt == 0) {
            if (node->nodeDef->nodeMethods.nodeTerm != NULL) {
                node->nodeDef->nodeMethods.nodeTerm(node->nodeDef);
            }
            if (node->nodeDef->editable) {
                RwEngineInstance->fpFree(node->nodeDef);
                node->nodeDef = NULL;
            }
        }
        if (node->initializationData != NULL) {
            RwEngineInstance->fpFree(node->initializationData);
            node->initializationData = NULL;
            node->initializationDataSize = 0;
        }
        memset(node, 0, sizeof(*node));
    } else {
        RwUInt32 nodeIndex;

        if (node->initializationData != NULL) {
            RwEngineInstance->fpFree(node->initializationData);
            node->initializationData = NULL;
            node->initializationDataSize = 0;
        }
        if (node->nodeDef->InputPipesCnt == 0 && node->nodeDef->editable) {
            RwEngineInstance->fpFree(node->nodeDef);
            node->nodeDef = NULL;
        }

        nodeIndex = node - pipeline->nodes;
        if (nodeIndex < pipeline->numNodes - 1) {
            RwUInt8* output;
            RwUInt8* nextOutput;
            RxPipelineNodeTopSortData* topSort;
            RxPipelineNodeTopSortData* nextTopSort;
            RwUInt32 i;

            output = (RwUInt8*)pipeline->nodes +
                     RXPIPELINEGLOBAL(maxNodes) * sizeof(*node);
            output += nodeIndex * 0x80;
            nextOutput = output + 0x80;
            for (i = nodeIndex; i < pipeline->numNodes - 1; ++i) {
                memcpy(output, nextOutput, 0x80);
                output = nextOutput;
                nextOutput += 0x80;
            }

            topSort = (RxPipelineNodeTopSortData*)((RwUInt8*)pipeline->nodes +
                      RXPIPELINEGLOBAL(maxNodes) * sizeof(*node));
            topSort = (RxPipelineNodeTopSortData*)((RwUInt8*)topSort +
                      RXPIPELINEGLOBAL(maxNodes) * 0x80);
            nextTopSort = topSort + 1;
            for (i = nodeIndex; i < pipeline->numNodes - 1; ++i) {
                memcpy(topSort, nextTopSort, sizeof(*topSort));
                topSort = nextTopSort;
                ++nextTopSort;
            }
            for (i = nodeIndex; i < pipeline->numNodes - 1; ++i) {
                memcpy(&pipeline->nodes[i], &pipeline->nodes[i + 1],
                       sizeof(*node));
                pipeline->nodes[i].outputs =
                    (RwUInt32*)((RwUInt8*)pipeline->nodes[i].outputs - 0x80);
                --pipeline->nodes[i].topSortData;
            }
            for (i = 0; i < pipeline->numNodes - 1; ++i) {
                RwUInt32 j;

                for (j = 0; j < pipeline->nodes[i].numOutputs; ++j) {
                    if (pipeline->nodes[i].outputs[j] >= nodeIndex) {
                        if (pipeline->nodes[i].outputs[j] == nodeIndex) {
                            pipeline->nodes[i].outputs[j] = 0xFFFFFFFF;
                        } else {
                            --pipeline->nodes[i].outputs[j];
                        }
                    }
                }
            }
        }
    }
    --pipeline->numNodes;
    return node;
}

RxHeap* RxHeapGetGlobalHeap(void)
{
    return _rxHeapGlobal;
}

/* Near miss: retail retains the otherwise-unused heap-reset result from an
 * assertion-like source expression; clean C preserves the reset behavior. */
RxPipeline* RxPipelineExecute(RxPipeline* pipeline, void* data,
                              RwBool heapReset)
{
    RxPipelineNode* node;
    RxNodeDefinition* nodeDef;
    RwBool result;

    if (heapReset && _rxHeapGlobal->dirty) {
        _rxHeapReset(_rxHeapGlobal);
    }

    _rxExecCtxGlobal.executionStatus = TRUE;
    _rxExecCtxGlobal.pipeline = pipeline;
    _rxExecCtxGlobal.params.dataParam = data;
    _rxExecCtxGlobal.params.heap = RxHeapGetGlobalHeap();
    pipeline->embeddedPacketState = rxPKST_PACKETLESS;

    node = pipeline->nodes;
    nodeDef = node->nodeDef;
    result = nodeDef->nodeMethods.nodeBody(node, &_rxExecCtxGlobal.params);
    if (!result) {
        _rxExecCtxGlobal.executionStatus = FALSE;
    }
    if (pipeline->embeddedPacketState > rxPKST_UNUSED) {
        pipeline->embeddedPacketState = rxPKST_INUSE;
        _rxPacketDestroy(pipeline->embeddedPacket);
    }

    _rxExecCtxGlobal.pipeline = NULL;
    _rxExecCtxGlobal.params.dataParam = NULL;
    _rxExecCtxGlobal.params.heap = NULL;
    if (_rxExecCtxGlobal.executionStatus) {
        return pipeline;
    }
    return NULL;
}

RxPipeline* RxPipelineCreate(void)
{
    RxPipeline* pipeline = RwEngineInstance->fpFreeListAlloc(
        RXPIPELINEGLOBAL(pipelines), 0x30409);

    if (pipeline != NULL) {
        memset(pipeline, 0, sizeof(*pipeline));
        pipeline->locked = FALSE;
        return pipeline;
    } else {
        RwError error;

        error.pluginID = 1;
        error.errorCode = _rwerror(0x80000013, sizeof(*pipeline));
        RwErrorSet(&error);
        return NULL;
    }
}

/* Near match: destruction order and register allocation are exact. Retail
 * clears its local pipeline pointer after the final free, a dead assignment
 * omitted from clean source. */
void _rxPipelineDestroy(RxPipeline* pipeline)
{
    if (pipeline != NULL) {
        RwUInt32 i;
        RwUInt32 numNodes;
        RxPipelineNode* node;

        node = pipeline->nodes;
        numNodes = pipeline->numNodes;

        for (i = 0; i < numNodes; ++i) {
            PipelineNodeDestroy(node, pipeline);
            ++node;
        }
        pipeline->nodes = NULL;
        if (pipeline->superBlock != NULL) {
            RwEngineInstance->fpFree(pipeline->superBlock);
            pipeline->superBlock = NULL;
            pipeline->superBlockSize = 0;
        }
        RwEngineInstance->fpFreeListFree(RXPIPELINEGLOBAL(pipelines), pipeline);
    }
}
