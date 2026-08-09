#include "rw/rwerror.h"
#include "rw/rxpipeline.h"

typedef struct rxReqEntry rxReqEntry;
typedef struct rxScopeTrace rxScopeTrace;

struct rxReq {
  RwUInt32 numEntries;
  RwUInt32 maxEntries;
  RwUInt32 assignedSlots;
  rxReqEntry *entries;
  RxPipelineNode *node;
};

struct rxReqEntry {
  RxClusterDefinition *clusterDef;
  RxClusterValidityReq requirement;
  RwUInt32 dependencyCount;
  rxScopeTrace *scope;
  rxReqEntry *scopeNext;
  rxReq *owner;
  RwUInt32 continuityMask;
  RwUInt32 slotIndex;
  RxPipelineNode *originatingNode;
};

struct rxScopeTrace {
  rxReqEntry *entries;
  rxScopeTrace *child;
  rxScopeTrace *next;
  rxScopeTrace *parent;
};

extern void *memset(void *destination, RwInt32 value, unsigned long size);

static rxReq *_ReqCreate(RxPipelineNode *node, RwUInt32 maxEntries) {
  rxReq *req = StalacTiteAlloc(sizeof(*req));

  if (req != NULL) {
    req->entries = StalacTiteAlloc(maxEntries * sizeof(*req->entries));
    if (req->entries != NULL) {
      req->maxEntries = maxEntries;
      req->numEntries = 0;
      req->node = node;
      req->assignedSlots = 0;
      return req;
    }
  }
  return NULL;
}

static rxReqEntry *_ReqSearch4Cluster(rxReq *req,
                                      RxClusterDefinition *clusterDef) {
  /* Retail loads numEntries into the index register immediately before
   * replacing it with zero; clean source omits that dead initialization. */
  rxReqEntry *entry;
  RwUInt32 index;

  if (req->numEntries != 0) {
    entry = req->entries;
    for (index = 0; index < req->numEntries;) {
      if (entry->clusterDef == clusterDef) {
        return entry;
      }
      entry++;
      index++;
    }
  }
  return NULL;
}

static rxReqEntry *_ReqAddEntry(rxReq *req, RxClusterDefinition *clusterDef,
                                RxClusterValidityReq requirement,
                                RwUInt32 dependencyCount,
                                RxPipelineNode *originatingNode) {
  rxReqEntry *entry = _ReqSearch4Cluster(req, clusterDef);

  if (entry != NULL) {
    if (requirement == rxCLREQ_REQUIRED) {
      entry->requirement = rxCLREQ_REQUIRED;
      entry->originatingNode = originatingNode;
    }
    return entry;
  }

  entry = &req->entries[req->numEntries];
  req->numEntries++;
  entry->clusterDef = clusterDef;
  entry->requirement = requirement;
  entry->dependencyCount = dependencyCount;
  entry->scopeNext = NULL;
  entry->owner = req;
  entry->scope = NULL;
  entry->continuityMask = 0;
  entry->slotIndex = (RwUInt32)-1;
  entry->originatingNode = originatingNode;
  return entry;
}

static void _ReqDeleteEntry(rxReq *req, rxReqEntry *entry) {
  rxReqEntry *last = &req->entries[req->numEntries - 1];

  /* Soft ceiling: retail's fixed-size copy macro lowers as a paired-word
   * loop. */
  if (entry != last) {
    *entry = *last;
  }
  req->numEntries--;
}

static RwInt32 _IoSpecSearch4Cluster(const RxIoSpec *io,
                                     RxClusterDefinition *clusterDef) {
  RwUInt32 index;

  for (index = 0; index < io->numClustersOfInterest; index++) {
    if (io->clustersOfInterest[index].clusterDef == clusterDef) {
      return index;
    }
  }
  return -1;
}

static void _PropDownElimPath(RxPipeline *pipeline, RxPipelineNode *node,
                              RxClusterDefinition *clusterDef) {
  RxIoSpec *io = &node->nodeDef->io;
  rxReqEntry *entry = _ReqSearch4Cluster(node->topSortData->req, clusterDef);

  if (entry != NULL && --entry->dependencyCount == 0) {
    RwUInt32 output;

    _ReqDeleteEntry(node->topSortData->req, entry);
    for (output = 0; output < node->numOutputs; output++) {
      if (node->outputs[output] != (RwUInt32)-1) {
        RxOutputSpec *outputSpec = &node->nodeDef->io.outputs[output];
        RwInt32 clusterIndex = _IoSpecSearch4Cluster(io, clusterDef);
        RxClusterValid validity =
            (RwUInt32)clusterIndex == (RwUInt32)-1
                ? outputSpec->allOtherClusters
                : outputSpec->outputClusters[clusterIndex];

        if (validity == rxCLVALID_NOCHANGE) {
          _PropDownElimPath(
              pipeline, &pipeline->nodes[node->outputs[output]], clusterDef);
        }
      }
    }
  }
}

static rxScopeTrace *_ScopeTraceCreate(rxScopeTrace **traces) {
  rxScopeTrace *trace = StalacTiteAlloc(sizeof(*trace));

  if (trace != NULL) {
    trace->child = NULL;
    trace->entries = NULL;
    trace->parent = NULL;
    trace->next = *traces;
    *traces = trace;
    return trace;
  }
  return NULL;
}

static void _ScopeTraceAddEntry(rxScopeTrace *trace, rxReqEntry *entry) {
  entry->scopeNext = trace->entries;
  trace->entries = entry;
}

static void _ScopeTraceMerge(rxScopeTrace **traces, rxScopeTrace *first,
                             rxScopeTrace *second) {
  /* The functional body matches retail exactly. Retail nevertheless saves LR
   * through _savegpr_29 in this call-free leaf; clean MWCC omits that traffic. */
  rxScopeTrace *firstRoot = first;
  rxScopeTrace *secondRoot = second;
  rxScopeTrace *leaf;

  while (firstRoot->parent != NULL) {
    firstRoot = firstRoot->parent;
  }
  while (secondRoot->parent != NULL) {
    secondRoot = secondRoot->parent;
  }
  if (firstRoot == secondRoot) {
    return;
  }

  leaf = firstRoot;
  while (leaf->child != NULL) {
    leaf = leaf->child;
  }
  leaf->child = secondRoot;
  secondRoot->parent = firstRoot;

  while (*traces != secondRoot) {
    traces = &(*traces)->next;
  }
  *traces = (*traces)->next;
}

static RwUInt32 _PropagateDependenciesAndKillDeadPaths(RxPipeline *pipeline) {
  RwUInt32 nodesRemaining = pipeline->numNodes;
  RxPipelineNode *node = &pipeline->nodes[pipeline->numNodes - 1];

  while (nodesRemaining != 0) {
    RxIoSpec *io = &node->nodeDef->io;
    RwUInt32 clusterIndex;
    RwUInt32 outputIndex;

    for (clusterIndex = 0; clusterIndex < io->numClustersOfInterest;
         clusterIndex++) {
      RxClusterDefinition *cluster =
          io->clustersOfInterest[clusterIndex].clusterDef;
      RwUInt32 comparison;

      if (cluster == NULL) {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x1F, node->nodeDef->name, clusterIndex);
        RwErrorSet(&error);
        return 0x1F;
      }
      for (comparison = clusterIndex + 1;
           comparison < io->numClustersOfInterest; comparison++) {
        RxClusterDefinition *other =
            io->clustersOfInterest[comparison].clusterDef;

        if (cluster == other) {
          RwError error;
          error.pluginID = 1;
          error.errorCode = _rwerror(0x1E, node->nodeDef->name, other->name);
          RwErrorSet(&error);
          return 0x1E;
        }
      }
    }

    node->topSortData->req =
        _ReqCreate(node, PipelineCalcNumUniqueClusters(pipeline));
    if (node->topSortData->req == NULL) {
      RwError error;
      error.pluginID = 1;
      error.errorCode = _rwerror(0x20);
      RwErrorSet(&error);
      return 0x20;
    }

    for (outputIndex = 0; outputIndex < node->numOutputs; outputIndex++) {
      RwUInt32 outputNodeIndex = node->outputs[outputIndex];

      if (outputNodeIndex + 0x10000U != (RwUInt32)-1) {
        RxPipelineNode *outputNode = &pipeline->nodes[outputNodeIndex];
        RxOutputSpec *outputSpec = &io->outputs[outputIndex];
        RwUInt32 entryIndex;

        for (entryIndex = 0;
             entryIndex < outputNode->topSortData->req->numEntries;
             entryIndex++) {
          rxReqEntry *outputEntry =
              entryIndex < outputNode->topSortData->req->numEntries
                  ? &outputNode->topSortData->req->entries[entryIndex]
                  : NULL;
          RwInt32 ioIndex = _IoSpecSearch4Cluster(io, outputEntry->clusterDef);
          RxClusterValid validity = ioIndex == -1
                                        ? outputSpec->allOtherClusters
                                        : outputSpec->outputClusters[ioIndex];

          if (validity == rxCLVALID_NOCHANGE) {
            if (_ReqAddEntry(node->topSortData->req, outputEntry->clusterDef,
                             outputEntry->requirement,
                             node->topSortData->numIns,
                             outputEntry->originatingNode) == NULL) {
              RwError error;
              error.pluginID = 1;
              error.errorCode = _rwerror(0x20);
              RwErrorSet(&error);
              return 0x20;
            }
          } else if (outputEntry->requirement == rxCLREQ_REQUIRED) {
            if (validity != rxCLVALID_VALID) {
              RwError error;
              error.pluginID = 1;
              error.errorCode =
                  _rwerror(0x1D, outputEntry->clusterDef->name,
                           outputEntry->originatingNode->nodeDef->name,
                           node->nodeDef->name, outputIndex, outputSpec->name);
              RwErrorSet(&error);
              return 0x1D;
            }
          } else if (validity == rxCLVALID_INVALID) {
            _PropDownElimPath(pipeline, outputNode, outputEntry->clusterDef);
          }
        }
      }
    }

    for (clusterIndex = 0; clusterIndex < io->numClustersOfInterest;
         clusterIndex++) {
      RxClusterValidityReq requirement = io->inputRequirements[clusterIndex];

      if (requirement != rxCLREQ_DONTWANT &&
          _ReqAddEntry(node->topSortData->req,
                       io->clustersOfInterest[clusterIndex].clusterDef,
                       requirement, node->topSortData->numIns, node) == NULL) {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x20);
        RwErrorSet(&error);
        return 0x20;
      }
    }

    node--;
    nodesRemaining--;
  }
  return 0;
}

/* Retail lowers the signed -1 I/O lookup sentinel through an unsigned two-step
 * compare. Clean typed C emits cmpwi; the remaining differences are that one
 * instruction and equivalent pipeline/output-entry register coloring. */
static RwUInt32 _ForAllNodeReqsAddOutputClustersAndBuildContinuityBitfields(
    RxPipeline *pipeline) {
  RwUInt32 nodesRemaining = pipeline->numNodes;
  RxPipelineNode *node = pipeline->nodes;
  RwUInt32 remaining;

  do {
    RxIoSpec *io = &node->nodeDef->io;
    RwUInt32 index;

    for (index = 0; index < io->numClustersOfInterest; index++) {
      if (io->clustersOfInterest[index].forcePresent != 0 &&
          _ReqAddEntry(node->topSortData->req,
                       io->clustersOfInterest[index].clusterDef,
                       rxCLREQ_DONTWANT, 1, node) == NULL) {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x20);
        RwErrorSet(&error);
        return 0x20;
      }
    }

    for (index = 0; index < node->numOutputs; index++) {
      if (node->outputs[index] != (RwUInt32)-1) {
        RxPipelineNode *outputNode =
            &pipeline->nodes[node->outputs[index]];
        RxOutputSpec *outputSpec = &node->nodeDef->io.outputs[index];
        RwUInt32 entryIndex;

        for (entryIndex = 0;
             entryIndex < outputNode->topSortData->req->numEntries;
             entryIndex++) {
          rxReqEntry *outputEntry =
              entryIndex < outputNode->topSortData->req->numEntries
                  ? &outputNode->topSortData->req->entries[entryIndex]
                  : NULL;
          RwInt32 ioIndex = _IoSpecSearch4Cluster(io, outputEntry->clusterDef);
          RxClusterValid validity = ioIndex == -1
                                        ? outputSpec->allOtherClusters
                                        : outputSpec->outputClusters[ioIndex];
          rxReqEntry *entry;

          if (validity == rxCLVALID_INVALID) {
            continue;
          }
          if (validity != rxCLVALID_NOCHANGE) {
            entry =
                _ReqAddEntry(node->topSortData->req, outputEntry->clusterDef,
                             rxCLREQ_DONTWANT, 1, node);
            if (entry == NULL) {
              RwError error;
              error.pluginID = 1;
              error.errorCode = _rwerror(0x20);
              RwErrorSet(&error);
              return 0x20;
            }
          } else {
            entry = _ReqSearch4Cluster(node->topSortData->req,
                                       outputEntry->clusterDef);
          }
          if (entry != NULL) {
            entry->continuityMask |= 1U << index;
          }
        }
      }
    }
    node++;
    remaining = --nodesRemaining;
  } while (remaining != 0);
  return 0;
}

/* Retail differs only in the nonvolatile registers assigned to the output
 * indices and output-node pointer; the instruction stream and size agree. */
static RwUInt32 _TraceClusterScopes(RxPipeline *pipeline,
                                    rxScopeTrace **traces) {
  RwUInt32 nodesRemaining = pipeline->numNodes;
  RxPipelineNode *node = pipeline->nodes;
  RwUInt32 remaining;

  do {
    RwUInt32 entryIndex;

    for (entryIndex = 0; entryIndex < node->topSortData->req->numEntries;
         entryIndex++) {
      rxReqEntry *entry = entryIndex < node->topSortData->req->numEntries
                              ? &node->topSortData->req->entries[entryIndex]
                              : NULL;

      if (entry->scope == NULL) {
        if ((entry->scope = _ScopeTraceCreate(traces)) == NULL) {
          RwError error;
          error.pluginID = 1;
          error.errorCode = _rwerror(0x20);
          RwErrorSet(&error);
          return 0x20;
        }
        _ScopeTraceAddEntry(entry->scope, entry);
      }
    }

    for (entryIndex = 0; entryIndex < node->numOutputs; entryIndex++) {
      if (node->outputs[entryIndex] != (RwUInt32)-1) {
        RxPipelineNode *outputNode =
            &pipeline->nodes[node->outputs[entryIndex]];
        RwUInt32 outputEntryIndex;

        for (outputEntryIndex = 0;
             outputEntryIndex < outputNode->topSortData->req->numEntries;
             outputEntryIndex++) {
          rxReqEntry *outputEntry =
              outputEntryIndex < outputNode->topSortData->req->numEntries
                  ? &outputNode->topSortData->req->entries[outputEntryIndex]
                  : NULL;

          if (outputEntry->requirement != rxCLREQ_DONTWANT) {
            rxReqEntry *entry = _ReqSearch4Cluster(node->topSortData->req,
                                                   outputEntry->clusterDef);

            if (entry != NULL &&
                (entry->continuityMask & (1U << entryIndex)) != 0) {
              if (outputEntry->scope == NULL) {
                _ScopeTraceAddEntry(entry->scope, outputEntry);
                outputEntry->scope = entry->scope;
              } else {
                _ScopeTraceMerge(traces, outputEntry->scope, entry->scope);
              }
            }
          }
        }
      }
    }
    node++;
    remaining = --nodesRemaining;
  } while (remaining != 0);
  return 0;
}

static RwUInt32 _AssignClusterSlots(RxPipeline *pipeline,
                                    rxScopeTrace **traces) {
  RwUInt32 numSlots = 0;
  rxScopeTrace *trace = *traces;

  while (trace != NULL) {
    RwUInt32 occupiedSlots = 0;
    rxScopeTrace *branch = trace;
    RwUInt32 slot;

    do {
      rxReqEntry *entry = branch->entries;

      while (entry != NULL) {
        occupiedSlots |= entry->owner->assignedSlots;
        entry = entry->scopeNext;
      }
      branch = branch->child;
    } while (branch != NULL);

    slot = 0;
    while ((occupiedSlots & 1) != 0) {
      occupiedSlots >>= 1;
      slot++;
    }
    if (slot >= numSlots) {
      numSlots = slot + 1;
    }

    branch = trace;
    do {
      rxReqEntry *entry = branch->entries;

      while (entry != NULL) {
        entry->slotIndex = slot;
        entry->owner->assignedSlots |= 1U << slot;
        entry = entry->scopeNext;
      }
      branch = branch->child;
    } while (branch != NULL);
    trace = trace->next;
  }

  pipeline->packetNumClusterSlots = numSlots;
  return 0;
}

static RwUInt32 _EnumPipelineClusters(rxScopeTrace *traces,
                                      void (*callback)(RxClusterDefinition *,
                                                       RwUInt32, void *),
                                      void *data) {
  RwUInt32 count = 0;
  rxScopeTrace *trace = traces;

  while (trace != NULL) {
    rxScopeTrace *previous = traces;

    while (previous != trace) {
      if (previous->entries->clusterDef == trace->entries->clusterDef) {
        break;
      }
      previous = previous->next;
    }
    if (previous == trace) {
      if (callback != NULL) {
        callback(trace->entries->clusterDef, count, data);
      }
      count++;
    }
    trace = trace->next;
  }
  return count;
}

static RwUInt32 _CountHeadNodeRqdsAndOpts(RxPipeline *pipeline) {
  RxPipelineNode *node = pipeline->nodes;
  RwUInt32 count = 0;
  RwUInt32 index;

  for (index = 0; index < node->topSortData->req->numEntries; index++) {
    rxReqEntry *entry = index < node->topSortData->req->numEntries
                            ? &node->topSortData->req->entries[index]
                            : NULL;

    if (entry->requirement == rxCLREQ_REQUIRED ||
        entry->requirement == rxCLREQ_OPTIONAL) {
      count++;
    }
  }
  return count;
}

static void
_WriteHeadNodeRqdsAndOpts2PipelineRequirements(RxPipeline *pipeline) {
  RxPipelineNode *node = pipeline->nodes;
  RwUInt32 outputIndex = 0;
  RwUInt32 index;

  for (index = 0; index < node->topSortData->req->numEntries; index++) {
    rxReqEntry *entry = index < node->topSortData->req->numEntries
                            ? &node->topSortData->req->entries[index]
                            : NULL;

    if (entry->requirement == rxCLREQ_REQUIRED ||
        entry->requirement == rxCLREQ_OPTIONAL) {
      RxPipelineRequiresCluster *requirement =
          &pipeline->inputRequirements[outputIndex++];

      requirement->clusterDef = entry->clusterDef;
      requirement->rqdOrOpt = entry->requirement;
      requirement->slotIndex = entry->slotIndex;
    }
  }
  pipeline->numInputRequirements = outputIndex;
  _rx_rxRadixExchangeSort(
      (RwUInt8 *)pipeline->inputRequirements, pipeline->numInputRequirements,
      sizeof(*pipeline->inputRequirements), 0, 0, (RwUInt32)-1);
}

static void _MyEnumPipelineClustersCallBack(RxClusterDefinition *clusterDef,
                                            RwUInt32 index, void *data) {
  /* Soft ceiling: the body is exact; only save-helper selection differs. */
  RxPipelineCluster **clusters = data;
  RxPipelineCluster *cluster = StalacMiteAlloc(sizeof(*cluster));

  if (*clusters == NULL) {
    *clusters = cluster;
  }
  cluster->clusterRef = clusterDef;
  cluster->creationAttributes = clusterDef->defaultAttributes;
}

/* Retail retains the rounded private-data size in a nonvolatile register before
 * allocation. The clean direct argument leaves only that move plus equivalent
 * register coloring and argument homing; all allocation and mapping CFG agrees. */
static RwUInt32 _ForAllNodesWriteClusterAllocations(RxPipeline *pipeline,
                                                    rxScopeTrace *traces) {
  RwUInt32 numHeadRequirements = _CountHeadNodeRqdsAndOpts(pipeline);
  RwUInt32 numPipelineClusters = _EnumPipelineClusters(traces, NULL, NULL);
  RwUInt8 *titeEnd = StalacTiteAlloc(0);
  RwUInt8 *miteStart = StalacMiteAlloc(0);
  RwUInt32 arenaSize = titeEnd - miteStart;
  RxPipelineCluster *pipelineClusters;
  RwUInt32 nodeIndex;

  memset(miteStart, 0, arenaSize);
  pipeline->embeddedPacket =
      StalacMiteAlloc(sizeof(RxPacket) + (pipeline->packetNumClusterSlots - 1) *
                                             sizeof(RxCluster));
  pipeline->embeddedPacket->numClusters = pipeline->packetNumClusterSlots;
  pipeline->embeddedPacket->pipeline = pipeline;
  pipeline->embeddedPacketState = rxPKST_PACKETLESS;

  pipelineClusters = NULL;
  _EnumPipelineClusters(traces, _MyEnumPipelineClustersCallBack,
                        &pipelineClusters);

  for (nodeIndex = 0; nodeIndex < pipeline->numNodes; nodeIndex++) {
    RxPipelineNode *node = &pipeline->nodes[nodeIndex];
    RwUInt32 continuity;
    RwUInt32 entryIndex;

    if (pipeline->packetNumClusterSlots != 0) {
      node->slotClusterRefs = StalacMiteAlloc(pipeline->packetNumClusterSlots *
                                              sizeof(*node->slotClusterRefs));
    }
    node->slotsContinue = StalacMiteAlloc(
        (pipeline->packetNumClusterSlots + 1) * sizeof(*node->slotsContinue));
    if (node->nodeDef->io.numClustersOfInterest != 0) {
      node->inputToClusterSlot =
          StalacMiteAlloc(node->nodeDef->io.numClustersOfInterest *
                          sizeof(*node->inputToClusterSlot));
    }
    if (node->nodeDef->pipelineNodePrivateDataSize != 0) {
      node->privateData = StalacMiteAlloc(
          (node->nodeDef->pipelineNodePrivateDataSize + 3) & ~3U);
    }

    continuity = (RwUInt32)-1;
    for (entryIndex = 0; entryIndex < node->topSortData->req->numEntries;
         entryIndex++) {
      RxPipelineCluster *pipelineCluster = NULL;
      rxReqEntry *entry = entryIndex < node->topSortData->req->numEntries
                              ? &node->topSortData->req->entries[entryIndex]
                              : NULL;
      RwUInt32 clusterIndex;

      for (clusterIndex = 0; clusterIndex < numPipelineClusters;
           clusterIndex++) {
        if (pipelineClusters[clusterIndex].clusterRef == entry->clusterDef) {
          pipelineCluster = &pipelineClusters[clusterIndex];
          break;
        }
      }
      node->slotClusterRefs[entry->slotIndex] = pipelineCluster;
      node->slotsContinue[entry->slotIndex + 1] = entry->continuityMask;
      continuity &= entry->continuityMask;
    }
    node->slotsContinue[0] = continuity;

    for (entryIndex = 0; entryIndex < node->nodeDef->io.numClustersOfInterest;
         entryIndex++) {
      RwUInt32 slot;

      node->inputToClusterSlot[entryIndex] = (RwUInt32)-1;
      for (slot = 0; slot < pipeline->packetNumClusterSlots; slot++) {
        if (node->slotClusterRefs[slot] != NULL &&
            node->slotClusterRefs[slot]->clusterRef ==
                node->nodeDef->io.clustersOfInterest[entryIndex].clusterDef) {
          node->inputToClusterSlot[entryIndex] = slot;
          break;
        }
      }
    }
  }

  if (numHeadRequirements != 0) {
    pipeline->inputRequirements = StalacMiteAlloc(
        numHeadRequirements * sizeof(*pipeline->inputRequirements));
  }
  _WriteHeadNodeRqdsAndOpts2PipelineRequirements(pipeline);
  return 0;
}

RwUInt32 _rxChaseDependencies(RxPipeline *pipeline) {
  rxScopeTrace *traces = NULL;
  RwUInt32 result = _PropagateDependenciesAndKillDeadPaths(pipeline);

  if (result == 0) {
    result =
        _ForAllNodeReqsAddOutputClustersAndBuildContinuityBitfields(pipeline);
    if (result == 0) {
      result = _TraceClusterScopes(pipeline, &traces);
      if (result == 0) {
        result = _AssignClusterSlots(pipeline, &traces);
        if (result == 0) {
          result = _ForAllNodesWriteClusterAllocations(pipeline, traces);
        }
      }
    }
  }
  /* Soft ceiling: retail materializes an unused final success predicate. */
  return result;
}
