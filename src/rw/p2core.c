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
        RwFreeListDestroy(RxPipelineGlobals()->pipelines);
        RxPipelineGlobals()->pipelines = 0;
        RxHeapDestroy(_rxHeapGlobal);
        _rxHeapGlobal = 0;
        RxPipelineInstanced = 0;
    }
    return 1;
}

RwBool _rxPipelineOpen(void)
{
    if (!RxPipelineInstanced) {
        _rxHeapGlobal = RxHeapCreate(_rxHeapInitialSize);
        if (_rxHeapGlobal == 0) {
            return 0;
        }

        RxPipelineGlobals()->pipelines = RwFreeListCreateAndPreallocateSpace(
            sizeof(RxPipeline), _rxPipesFreeListBlockSize, 4,
            _rxPipesFreeListPreallocBlocks, &_rxPipesFreeList, 0x40409);
        if (RxPipelineGlobals()->pipelines == 0) {
            RxHeapDestroy(_rxHeapGlobal);
            _rxHeapGlobal = 0;
            return 0;
        }

        RxPipelineGlobals()->maxNodes = _rxPipelineMaxNodes;
        RxRenderStateVectorSetDefaultRenderStateVector(
            &RxPipelineGlobals()->defaultRenderState);
        RxPipelineGlobals()->currentNode = 0;
        RxPipelineGlobals()->currentPipeline = 0;
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
        RwUInt32 nodeIndex;

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
            RwUInt8* output;
            RwUInt8* nextOutput;
            RxPipelineNodeTopSortData* topSort;
            RxPipelineNodeTopSortData* nextTopSort;
            RwUInt32 i;

            output = (RwUInt8*)pipeline->nodes +
                     RxPipelineGlobals()->maxNodes * sizeof(*node);
            output += nodeIndex * 0x80;
            nextOutput = output + 0x80;
            for (i = nodeIndex; i < pipeline->numNodes - 1; ++i) {
                memcpy(output, nextOutput, 0x80);
                output = nextOutput;
                nextOutput += 0x80;
            }

            topSort = (RxPipelineNodeTopSortData*)((RwUInt8*)pipeline->nodes +
                      RxPipelineGlobals()->maxNodes * sizeof(*node));
            topSort = (RxPipelineNodeTopSortData*)((RwUInt8*)topSort +
                      RxPipelineGlobals()->maxNodes * 0x80);
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



RxPipeline* RxPipelineExecute(RxPipeline* pipeline, void* data,
                              RwBool heapReset)
{
    RxPipelineNode* node;
    RxNodeDefinition* nodeDef;
    RwBool result;

    if (heapReset && _rxHeapGlobal->dirty) {
        _rxHeapReset(_rxHeapGlobal);
    }

    _rxExecCtxGlobal.executionStatus = 1;
    _rxExecCtxGlobal.pipeline = pipeline;
    _rxExecCtxGlobal.params.dataParam = data;
    _rxExecCtxGlobal.params.heap = RxHeapGetGlobalHeap();
    pipeline->embeddedPacketState = 0;

    node = pipeline->nodes;
    nodeDef = node->nodeDef;
    result = nodeDef->nodeMethods.nodeBody(node, &_rxExecCtxGlobal.params);
    if (!result) {
        _rxExecCtxGlobal.executionStatus = 0;
    }
    if (pipeline->embeddedPacketState > 1) {
        pipeline->embeddedPacketState = 2;
        _rxPacketDestroy(pipeline->embeddedPacket);
    }

    _rxExecCtxGlobal.pipeline = 0;
    _rxExecCtxGlobal.params.dataParam = 0;
    _rxExecCtxGlobal.params.heap = 0;
    if (_rxExecCtxGlobal.executionStatus) {
        return pipeline;
    }
    return 0;
}

RxPipeline* RxPipelineCreate(void)
{
    RxPipeline* pipeline = RwEngineInstance->fpFreeListAlloc(
        RxPipelineGlobals()->pipelines, 0x30409);

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
        RwUInt32 i;
        RwUInt32 numNodes;
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
        RwEngineInstance->fpFreeListFree(RxPipelineGlobals()->pipelines, pipeline);
    }
}
