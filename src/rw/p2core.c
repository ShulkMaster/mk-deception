#include "libmkparticle/rw_engine.h"
#include "rw/rwerror.h"
#include "rw/rwfreelist.h"
#include "rw/rxpipeline.h"
#include "runtime/cstring.h"

extern RxExecutionContext _rxExecCtxGlobal;

#define RxHeapReset(heap) ((!(heap)->dirty) ? 1 : _rxHeapReset(heap))

static RwFreeList _rxPipesFreeList;
unsigned int _rxHeapInitialSize = 0x1000;
unsigned int _rxPipelineMaxNodes = 0x40;
static int _rxPipesFreeListBlockSize = 0x40;
static int _rxPipesFreeListPreallocBlocks = 1;
RxHeap* _rxHeapGlobal;
int RxPipelineInstanced;

int _rxPipelineClose(void)
{
    if (RxPipelineInstanced) {
        RwFreeListDestroy(RXPIPELINEGLOBAL(pipesFreeList));
        RXPIPELINEGLOBAL(pipesFreeList) = 0;
        RxHeapDestroy(_rxHeapGlobal);
        _rxHeapGlobal = 0;
        RxPipelineInstanced = 0;
    }
    return 1;
}

int _rxPipelineOpen(void)
{
    if (!RxPipelineInstanced) {
        _rxHeapGlobal = RxHeapCreate(_rxHeapInitialSize);
        if (_rxHeapGlobal == 0) {
            return 0;
        }

        RXPIPELINEGLOBAL(pipesFreeList) = RwFreeListCreateAndPreallocateSpace(
            sizeof(RxPipeline), _rxPipesFreeListBlockSize, 4,
            _rxPipesFreeListPreallocBlocks, &_rxPipesFreeList, 0x40409);
        if (RXPIPELINEGLOBAL(pipesFreeList) == 0) {
            RxHeapDestroy(_rxHeapGlobal);
            _rxHeapGlobal = 0;
            return 0;
        }

        RXPIPELINEGLOBAL(maxNodesPerPipe) = _rxPipelineMaxNodes;
        RxRenderStateVectorSetDefaultRenderStateVector(
            &RXPIPELINEGLOBAL(defaultRenderState));
        RXPIPELINEGLOBAL(allPipelines).link.prev = 0;
        RXPIPELINEGLOBAL(allPipelines).link.next = 0;
        RxPipelineInstanced = 1;
        return 1;
    }
    return 0;
}



RxPipelineNode* PipelineNodeDestroy(RxPipelineNode* node,
                                    RxPipeline* pipeline)
{
    if (!pipeline->locked) {
        if (node->nodeDef->nodeMethods.pipelineNodeTerm != 0) {
            node->nodeDef->nodeMethods.pipelineNodeTerm(node);
        }
        --node->nodeDef->InputPipesCnt;
        if (node->nodeDef->InputPipesCnt == 0) {
            if (node->nodeDef->nodeMethods.nodeTerm != 0) {
                node->nodeDef->nodeMethods.nodeTerm(node->nodeDef);
            }
            if (node->nodeDef->editable) {
                RwEngineInstance->fpFree(node->nodeDef);
                node->nodeDef = 0;
            }
        }
        if (node->initializationData != 0) {
            RwEngineInstance->fpFree(node->initializationData);
            node->initializationData = 0;
            node->initializationDataSize = 0;
        }
        memset(node, 0, sizeof(*node));
    } else {
        unsigned int nodeIndex;

        if (node->initializationData != 0) {
            RwEngineInstance->fpFree(node->initializationData);
            node->initializationData = 0;
            node->initializationDataSize = 0;
        }
        if (node->nodeDef->InputPipesCnt == 0 && node->nodeDef->editable) {
            RwEngineInstance->fpFree(node->nodeDef);
            node->nodeDef = 0;
        }

        nodeIndex = node - pipeline->nodes;
        if (nodeIndex < pipeline->numNodes - 1) {
            unsigned int* output;
            unsigned int* nextOutput;
            RxPipelineNodeTopSortData* topSort;
            RxPipelineNodeTopSortData* nextTopSort;
            unsigned int i;

            output = (unsigned int*)&pipeline->nodes[
                RXPIPELINEGLOBAL(maxNodesPerPipe)];
            output += 0x20 * nodeIndex;
            nextOutput = output + 0x20;
            for (i = nodeIndex; i < pipeline->numNodes - 1; ++i) {
                memcpy(output, nextOutput, 0x80);
                output = nextOutput;
                nextOutput += 0x20;
            }

            output = (unsigned int*)&pipeline->nodes[
                RXPIPELINEGLOBAL(maxNodesPerPipe)];
            topSort = (RxPipelineNodeTopSortData*)&output[
                0x20 * RXPIPELINEGLOBAL(maxNodesPerPipe)];
            nextTopSort = topSort + 1;
            for (i = nodeIndex; i < pipeline->numNodes - 1; ++i) {
                memcpy(topSort, nextTopSort, sizeof(*topSort));
                topSort = nextTopSort;
                ++nextTopSort;
            }
            for (i = nodeIndex; i < pipeline->numNodes - 1; ++i) {
                memcpy(&pipeline->nodes[i], &pipeline->nodes[i + 1],
                       sizeof(*node));
                pipeline->nodes[i].outputs -= 0x20;
                --pipeline->nodes[i].topSortData;
            }
            for (i = 0; i < pipeline->numNodes - 1; ++i) {
                unsigned int j;

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



RxPipeline* RxPipelineExecute(RxPipeline* pipeline, void* data,
                              int heapReset)
{
    RxPipelineNode* node;
    unsigned int result;
    RxNodeDefinition* nodeDef;

    if (heapReset) {
        RxHeapReset(_rxHeapGlobal);
    }

    _rxExecCtxGlobal.exitCode = 1;
    _rxExecCtxGlobal.pipeline = pipeline;
    _rxExecCtxGlobal.params.dataParam = data;
    _rxExecCtxGlobal.params.heap = RxHeapGetGlobalHeap();
    pipeline->embeddedPacketState = 0;

    node = pipeline->nodes;
    nodeDef = node->nodeDef;
    result = nodeDef->nodeMethods.nodeBody(node, &_rxExecCtxGlobal.params);
    if (!result) {
        _rxExecCtxGlobal.exitCode = result;
    }
    if (pipeline->embeddedPacketState > 1) {
        pipeline->embeddedPacketState = 2;
        _rxPacketDestroy(pipeline->embeddedPacket);
    }

    _rxExecCtxGlobal.pipeline = 0;
    _rxExecCtxGlobal.params.dataParam = 0;
    _rxExecCtxGlobal.params.heap = 0;
    if (_rxExecCtxGlobal.exitCode) {
        return pipeline;
    }
    return 0;
}

RxPipeline* RxPipelineCreate(void)
{
    RxPipeline* pipeline = RwEngineInstance->fpFreeListAlloc(
        RXPIPELINEGLOBAL(pipesFreeList), 0x30409);

    if (pipeline != 0) {
        memset(pipeline, 0, sizeof(*pipeline));
        pipeline->locked = 0;
        return pipeline;
    } else {
        RwError error;

        error.pluginID = 1;
        error.errorCode = _rwerror(0x80000013, sizeof(*pipeline));
        RwErrorSet(&error);
        return 0;
    }
}




void _rxPipelineDestroy(RxPipeline* pipeline)
{
    if (pipeline != 0) {
        unsigned int i;
        unsigned int numNodes;
        RxPipelineNode* node;

        node = pipeline->nodes;
        numNodes = pipeline->numNodes;

        for (i = 0; i < numNodes; ++i) {
            PipelineNodeDestroy(node, pipeline);
            ++node;
        }
        pipeline->nodes = 0;
        if (pipeline->superBlock != 0) {
            RwEngineInstance->fpFree(pipeline->superBlock);
            pipeline->superBlock = 0;
            pipeline->superBlockSize = 0;
        }
        RwEngineInstance->fpFreeListFree(RXPIPELINEGLOBAL(pipesFreeList), pipeline);
    }
}
