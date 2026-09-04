#include "rw/rwerror.h"
#include "runtime/cstring.h"
#include "rw/rxpipeline.h"

typedef struct rxReqEntry rxReqEntry;
typedef struct rxScopeTrace rxScopeTrace;

struct rxReq {
  unsigned int numEntries;
  unsigned int maxEntries;
  unsigned int assignedSlots;
  rxReqEntry *entries;
  RxPipelineNode *node;
};

struct rxReqEntry {
  RxClusterDefinition *clusterDef;
  RxClusterValidityReq requirement;
  unsigned int dependencyCount;
  rxScopeTrace *scope;
  rxReqEntry *scopeNext;
  rxReq *owner;
  unsigned int continuityMask;
  unsigned int slotIndex;
  RxPipelineNode *originatingNode;
};

struct rxScopeTrace {
  rxReqEntry *entries;
  rxScopeTrace *child;
  rxScopeTrace *next;
  rxScopeTrace *parent;
};

static rxReq *_ReqCreate(RxPipelineNode *node, unsigned int maxEntries) {
  rxReq *req = StalacTiteAlloc(sizeof(*req));

  if (req != 0) {
    req->entries = StalacTiteAlloc(maxEntries * sizeof(*req->entries));
    if (req->entries != 0) {
      req->maxEntries = maxEntries;
      req->numEntries = 0;
      req->node = node;
      req->assignedSlots = 0;
      return req;
    }
  }
  return 0;
}

static rxReqEntry *_ReqSearch4Cluster(rxReq *req,
                                      RxClusterDefinition *clusterDef) {


  rxReqEntry *entry;
  unsigned int index;

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
  return 0;
}

static rxReqEntry *_ReqAddEntry(rxReq *req, RxClusterDefinition *clusterDef,
                                RxClusterValidityReq requirement,
                                unsigned int dependencyCount,
                                RxPipelineNode *originatingNode) {
  rxReqEntry *entry = _ReqSearch4Cluster(req, clusterDef);

  if (entry != 0) {
    if (requirement == 1) {
      entry->requirement = 1;
      entry->originatingNode = originatingNode;
    }
    return entry;
  }

  entry = &req->entries[req->numEntries];
  req->numEntries++;
  entry->clusterDef = clusterDef;
  entry->requirement = requirement;
  entry->dependencyCount = dependencyCount;
  entry->scopeNext = 0;
  entry->owner = req;
  entry->scope = 0;
  entry->continuityMask = 0;
  entry->slotIndex = (unsigned int)-1;
  entry->originatingNode = originatingNode;
  return entry;
}

/* Removes an entry by replacing it with the requirement array's tail. */
static void _ReqDeleteEntry(rxReq *req, rxReqEntry *entry) {


  if (entry != &req->entries[req->numEntries - 1]) {
    *entry = req->entries[req->numEntries - 1];
  }
  req->numEntries--;
}

static unsigned int _IoSpecSearch4Cluster(const RxIoSpec *io,
                                      RxClusterDefinition *clusterDef) {
  unsigned int index;

  for (index = 0; index < io->numClustersOfInterest; index++) {
    if (io->clustersOfInterest[index].clusterDef == clusterDef) {
      return index;
    }
  }
  return (unsigned int)-1;
}

static void _PropDownElimPath(RxPipeline *pipeline, RxPipelineNode *node,
                              RxClusterDefinition *clusterDef) {
  RxIoSpec *io = &node->nodeDef->io;
  rxReqEntry *entry = _ReqSearch4Cluster(node->topSortData->req, clusterDef);

  if (entry != 0 && --entry->dependencyCount == 0) {
    unsigned int output;

    _ReqDeleteEntry(node->topSortData->req, entry);
    for (output = 0; output < node->numOutputs; output++) {
      if (node->outputs[output] != (unsigned int)-1) {
        RxOutputSpec *outputSpec = &node->nodeDef->io.outputs[output];
        unsigned int clusterIndex = _IoSpecSearch4Cluster(io, clusterDef);
        RxClusterValid validity =
            clusterIndex == (unsigned int)-1
                ? outputSpec->allOtherClusters
                : outputSpec->outputClusters[clusterIndex];

        if (validity == 0) {
          _PropDownElimPath(
              pipeline, &pipeline->nodes[node->outputs[output]], clusterDef);
        }
      }
    }
  }
}

static rxScopeTrace *_ScopeTraceCreate(rxScopeTrace **traces) {
  rxScopeTrace *trace = StalacTiteAlloc(sizeof(*trace));

  if (trace != 0) {
    trace->child = 0;
    trace->entries = 0;
    trace->parent = 0;
    trace->next = *traces;
    *traces = trace;
    return trace;
  }
  return 0;
}

static void _ScopeTraceAddEntry(rxScopeTrace *trace, rxReqEntry *entry) {
  entry->scopeNext = trace->entries;
  trace->entries = entry;
}

static void _ScopeTraceMerge(rxScopeTrace **traces, rxScopeTrace *first,
                             rxScopeTrace *second) {


  rxScopeTrace *firstRoot = first;
  rxScopeTrace *secondRoot = second;
  rxScopeTrace *leaf;

  while (firstRoot->parent != 0) {
    firstRoot = firstRoot->parent;
  }
  while (secondRoot->parent != 0) {
    secondRoot = secondRoot->parent;
  }
  if (firstRoot == secondRoot) {
    return;
  }

  leaf = firstRoot;
  while (leaf->child != 0) {
    leaf = leaf->child;
  }
  leaf->child = secondRoot;
  secondRoot->parent = firstRoot;

  while (*traces != secondRoot) {
    traces = &(*traces)->next;
  }
  *traces = (*traces)->next;
}




static unsigned int _PropagateDependenciesAndKillDeadPaths(RxPipeline *pipeline) {
  unsigned int nodesRemaining = pipeline->numNodes;
  RxPipelineNode *node = &pipeline->nodes[pipeline->numNodes - 1];
  unsigned int remaining;

  do {
    RxIoSpec *io = &node->nodeDef->io;
    unsigned int clusterIndex;
    unsigned int outputIndex;

    for (clusterIndex = 0;
         clusterIndex < node->nodeDef->io.numClustersOfInterest;
         clusterIndex++) {
      RxClusterDefinition *cluster =
          node->nodeDef->io.clustersOfInterest[clusterIndex].clusterDef;
      unsigned int comparison;

      if (cluster == 0) {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x1F, node->nodeDef->name, clusterIndex);
        RwErrorSet(&error);
        return 0x1F;
      }
      for (comparison = clusterIndex + 1;
           comparison < node->nodeDef->io.numClustersOfInterest;
           comparison++) {
        RxClusterDefinition *other =
            node->nodeDef->io.clustersOfInterest[comparison].clusterDef;

        if (cluster == other) {
          RwError error;
          error.pluginID = 1;
          error.errorCode = _rwerror(0x1E, node->nodeDef->name, other->name);
          RwErrorSet(&error);
          return 0x1E;
        }
      }
    }

    {
      unsigned int maxClusters = PipelineCalcNumUniqueClusters(pipeline);

      if ((node->topSortData->req = _ReqCreate(node, maxClusters)) == 0) {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x20);
        RwErrorSet(&error);
        return 0x20;
      }
    }

    for (outputIndex = 0; outputIndex < node->numOutputs; outputIndex++) {
      if (node->outputs[outputIndex] != (unsigned int)-1) {
        RxPipelineNode *outputNode =
            &pipeline->nodes[node->outputs[outputIndex]];
        RxOutputSpec *outputSpec = &node->nodeDef->io.outputs[outputIndex];
        unsigned int entryIndex;

        for (entryIndex = 0;
             entryIndex < outputNode->topSortData->req->numEntries;
             entryIndex++) {
          rxReqEntry *outputEntry =
              entryIndex < outputNode->topSortData->req->numEntries
                  ? &outputNode->topSortData->req->entries[entryIndex]
                  : 0;
          unsigned int ioIndex =
              _IoSpecSearch4Cluster(io, outputEntry->clusterDef);
          RxClusterValid validity =
              ioIndex == (unsigned int)-1 ? outputSpec->allOtherClusters
                                      : outputSpec->outputClusters[ioIndex];

          if (validity == 0) {
            if (_ReqAddEntry(node->topSortData->req, outputEntry->clusterDef,
                             outputEntry->requirement,
                             node->topSortData->numIns,
                             outputEntry->originatingNode) == 0) {
              RwError error;
              error.pluginID = 1;
              error.errorCode = _rwerror(0x20);
              RwErrorSet(&error);
              return 0x20;
            }
          } else if (outputEntry->requirement == 1) {
            if (validity != 1) {
              RwError error;
              error.pluginID = 1;
              error.errorCode =
                  _rwerror(0x1D, outputEntry->clusterDef->name,
                           outputEntry->originatingNode->nodeDef->name,
                           node->nodeDef->name, outputIndex, outputSpec->name);
              RwErrorSet(&error);
              return 0x1D;
            }
          } else if (validity == 2) {
            _PropDownElimPath(pipeline, outputNode, outputEntry->clusterDef);
          }
        }
      }
    }

    for (clusterIndex = 0; clusterIndex < io->numClustersOfInterest;
         clusterIndex++) {
      if (io->inputRequirements[clusterIndex] != 0 &&
          _ReqAddEntry(node->topSortData->req,
                       io->clustersOfInterest[clusterIndex].clusterDef,
                       io->inputRequirements[clusterIndex],
                       node->topSortData->numIns, node) == 0) {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x20);
        RwErrorSet(&error);
        return 0x20;
      }
    }

    node--;
    remaining = --nodesRemaining;
  } while (remaining != 0);
  return 0;
}



static unsigned int _ForAllNodeReqsAddOutputClustersAndBuildContinuityBitfields(
    RxPipeline *pipeline) {
  unsigned int nodesRemaining = pipeline->numNodes;
  RxPipelineNode *node = pipeline->nodes;
  unsigned int remaining;

  do {
    RxIoSpec *io = &node->nodeDef->io;
    unsigned int index;

    for (index = 0; index < io->numClustersOfInterest; index++) {
      if (io->clustersOfInterest[index].forcePresent != 0 &&
          _ReqAddEntry(node->topSortData->req,
                       io->clustersOfInterest[index].clusterDef,
                       0, 1, node) == 0) {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x20);
        RwErrorSet(&error);
        return 0x20;
      }
    }

    for (index = 0; index < node->numOutputs; index++) {
      if (node->outputs[index] != (unsigned int)-1) {
        RxPipelineNode *outputNode =
            &pipeline->nodes[node->outputs[index]];
        RxOutputSpec *outputSpec = &node->nodeDef->io.outputs[index];
        unsigned int entryIndex;

        for (entryIndex = 0;
             entryIndex < outputNode->topSortData->req->numEntries;
             entryIndex++) {
          rxReqEntry *outputEntry =
              entryIndex < outputNode->topSortData->req->numEntries
                  ? &outputNode->topSortData->req->entries[entryIndex]
                  : 0;
          unsigned int ioIndex = _IoSpecSearch4Cluster(io, outputEntry->clusterDef);
          RxClusterValid validity = ioIndex == (unsigned int)-1
                                        ? outputSpec->allOtherClusters
                                        : outputSpec->outputClusters[ioIndex];
          rxReqEntry *entry;

          if (validity == 2) {
            continue;
          }
          if (validity != 0) {
            entry =
                _ReqAddEntry(node->topSortData->req, outputEntry->clusterDef,
                             0, 1, node);
            if (entry == 0) {
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
          if (entry != 0) {
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



static unsigned int _TraceClusterScopes(RxPipeline *pipeline,
                                    rxScopeTrace **traces) {
  unsigned int nodesRemaining = pipeline->numNodes;
  RxPipelineNode *node = pipeline->nodes;
  unsigned int remaining;

  do {
    unsigned int entryIndex;

    for (entryIndex = 0; entryIndex < node->topSortData->req->numEntries;
         entryIndex++) {
      rxReqEntry *entry = entryIndex < node->topSortData->req->numEntries
                              ? &node->topSortData->req->entries[entryIndex]
                              : 0;

      if (entry->scope == 0) {
        if ((entry->scope = _ScopeTraceCreate(traces)) == 0) {
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
      if (node->outputs[entryIndex] != (unsigned int)-1) {
        RxPipelineNode *outputNode =
            &pipeline->nodes[node->outputs[entryIndex]];
        unsigned int outputEntryIndex;

        for (outputEntryIndex = 0;
             outputEntryIndex < outputNode->topSortData->req->numEntries;
             outputEntryIndex++) {
          rxReqEntry *outputEntry =
              outputEntryIndex < outputNode->topSortData->req->numEntries
                  ? &outputNode->topSortData->req->entries[outputEntryIndex]
                  : 0;

          if (outputEntry->requirement != 0) {
            rxReqEntry *entry = _ReqSearch4Cluster(node->topSortData->req,
                                                   outputEntry->clusterDef);

            if (entry != 0 &&
                (entry->continuityMask & (1U << entryIndex)) != 0) {
              if (outputEntry->scope == 0) {
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

static unsigned int _AssignClusterSlots(RxPipeline *pipeline,
                                    rxScopeTrace **traces) {
  unsigned int numSlots = 0;
  rxScopeTrace *trace = *traces;

  while (trace != 0) {
    unsigned int occupiedSlots = 0;
    rxScopeTrace *branch = trace;
    unsigned int slot;

    do {
      rxReqEntry *entry = branch->entries;

      while (entry != 0) {
        occupiedSlots |= entry->owner->assignedSlots;
        entry = entry->scopeNext;
      }
      branch = branch->child;
    } while (branch != 0);

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

      while (entry != 0) {
        entry->slotIndex = slot;
        entry->owner->assignedSlots |= 1U << slot;
        entry = entry->scopeNext;
      }
      branch = branch->child;
    } while (branch != 0);
    trace = trace->next;
  }

  pipeline->packetNumClusterSlots = numSlots;
  return 0;
}

static unsigned int _EnumPipelineClusters(rxScopeTrace *traces,
                                      void (*callback)(RxClusterDefinition *,
                                                       unsigned int, void *),
                                      void *data) {
  unsigned int count = 0;
  rxScopeTrace *trace = traces;

  while (trace != 0) {
    rxScopeTrace *previous = traces;

    while (previous != trace) {
      if (previous->entries->clusterDef == trace->entries->clusterDef) {
        break;
      }
      previous = previous->next;
    }
    if (previous == trace) {
      if (callback != 0) {
        callback(trace->entries->clusterDef, count, data);
      }
      count++;
    }
    trace = trace->next;
  }
  return count;
}

static unsigned int _CountHeadNodeRqdsAndOpts(RxPipeline *pipeline) {
  RxPipelineNode *node = pipeline->nodes;
  unsigned int count = 0;
  unsigned int index;

  for (index = 0; index < node->topSortData->req->numEntries; index++) {
    rxReqEntry *entry = index < node->topSortData->req->numEntries
                            ? &node->topSortData->req->entries[index]
                            : 0;

    if (entry->requirement == 1 ||
        entry->requirement == 2) {
      count++;
    }
  }
  return count;
}

static void
_WriteHeadNodeRqdsAndOpts2PipelineRequirements(RxPipeline *pipeline) {
  RxPipelineNode *node = pipeline->nodes;
  unsigned int outputIndex = 0;
  unsigned int index;

  for (index = 0; index < node->topSortData->req->numEntries; index++) {
    rxReqEntry *entry = index < node->topSortData->req->numEntries
                            ? &node->topSortData->req->entries[index]
                            : 0;

    if (entry->requirement == 1 ||
        entry->requirement == 2) {
      RxPipelineRequiresCluster *requirement =
          &pipeline->inputRequirements[outputIndex++];

      requirement->clusterDef = entry->clusterDef;
      requirement->rqdOrOpt = entry->requirement;
      requirement->slotIndex = entry->slotIndex;
    }
  }
  pipeline->numInputRequirements = outputIndex;
  _rx_rxRadixExchangeSort(
      (unsigned char *)pipeline->inputRequirements, pipeline->numInputRequirements,
      sizeof(*pipeline->inputRequirements), 0, 0, (unsigned int)-1);
}

/* Materializes each discovered definition into the pipeline cluster array. */
static void _MyEnumPipelineClustersCallBack(RxClusterDefinition *clusterDef,
                                            unsigned int index, void *data) {

  RxPipelineCluster **clusters = data;
  RxPipelineCluster *cluster = StalacMiteAlloc(sizeof(*cluster));

  if (*clusters == 0) {
    *clusters = cluster;
  }
  cluster->clusterRef = clusterDef;
  cluster->creationAttributes = clusterDef->defaultAttributes;
}

static unsigned int _ForAllNodesWriteClusterAllocations(RxPipeline *pipeline,
                                                    rxScopeTrace *traces) {
  unsigned int numHeadRequirements;
  unsigned int numPipelineClusters;
  RxPipelineCluster *pipelineClusters;
  RxPipelineNode *node;
  unsigned char *titeEnd;
  unsigned char *miteStart;
  unsigned int continuity;
  unsigned int i;
  unsigned int j;
  unsigned int k;

  numHeadRequirements = _CountHeadNodeRqdsAndOpts(pipeline);
  numPipelineClusters = _EnumPipelineClusters(traces, 0, 0);
  titeEnd = StalacTiteAlloc(0);
  miteStart = StalacMiteAlloc(0);
  i = titeEnd - miteStart;

  memset(miteStart, 0, i);
  pipeline->embeddedPacket =
      StalacMiteAlloc(sizeof(RxPacket) + (pipeline->packetNumClusterSlots - 1) *
                                             sizeof(RxCluster));
  pipeline->embeddedPacket->numClusters = pipeline->packetNumClusterSlots;
  pipeline->embeddedPacket->pipeline = pipeline;
  pipeline->embeddedPacketState = 0;

  pipelineClusters = 0;
  _EnumPipelineClusters(traces, _MyEnumPipelineClustersCallBack,
                        &pipelineClusters);

  for (i = 0; i < pipeline->numNodes; i++) {
    node = &pipeline->nodes[i];

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
      j = (node->nodeDef->pipelineNodePrivateDataSize + 3) & ~3U;
      node->privateData = StalacMiteAlloc(j);
    }

    continuity = (unsigned int)-1;
    for (j = 0; j < node->topSortData->req->numEntries; j++) {
      RxPipelineCluster *pipelineCluster = 0;
      rxReqEntry *entry = j < node->topSortData->req->numEntries
                              ? &node->topSortData->req->entries[j]
                              : 0;

      for (k = 0; k < numPipelineClusters; k++) {
        if (pipelineClusters[k].clusterRef == entry->clusterDef) {
          pipelineCluster = &pipelineClusters[k];
          break;
        }
      }
      node->slotClusterRefs[entry->slotIndex] = pipelineCluster;
      node->slotsContinue[entry->slotIndex + 1] = entry->continuityMask;
      continuity &= entry->continuityMask;
    }
    node->slotsContinue[0] = continuity;

    for (j = 0; j < node->nodeDef->io.numClustersOfInterest; j++) {
      node->inputToClusterSlot[j] = (unsigned int)-1;
      for (k = 0; k < pipeline->packetNumClusterSlots; k++) {
        if (node->slotClusterRefs[k] != 0 &&
            node->slotClusterRefs[k]->clusterRef ==
                node->nodeDef->io.clustersOfInterest[j].clusterDef) {
          node->inputToClusterSlot[j] = k;
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

unsigned int _rxChaseDependencies(RxPipeline *pipeline) {
  rxScopeTrace *traces = 0;
  unsigned int result = _PropagateDependenciesAndKillDeadPaths(pipeline);

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

  return result;
}
