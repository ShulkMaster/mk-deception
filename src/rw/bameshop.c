#include "rw/rwengine.h"
#include "rw/rpmesh_internal.h"
#include "rw/rpworld_types.h"
#include "rw/rwerror.h"
#include "rw/rwfreelist.h"
#include "runtime/cstdlib.h"

typedef RpMeshHeader *(*RpTriStripMeshCallBack)(RpBuildMesh *, void *);

RpMeshHeader *RpBuildMeshGenerateDefaultTriStrip(RpBuildMesh *, void *);
static void MeshReportAllocationFailure(int value)
{
    RwError error;
    error.pluginID = 2;
    error.errorCode = _rwerror(0x80000013, value);
    RwErrorSet(&error);
}

typedef struct TriStripListEntry {
    unsigned short *strip;
    unsigned int stripLen;
    unsigned int stripSize;
    struct TriStripListEntry *next;
} TriStripListEntry;

typedef struct TriStripList {
    TriStripListEntry *head;
} TriStripList;

typedef struct TriBinEntry TriBinEntry;

typedef struct Edge {
    unsigned short v1;
    unsigned short v2;
    TriBinEntry *tri1;
    TriBinEntry *tri2;
    struct Edge *next;
} Edge;

struct TriBinEntry {
    unsigned int tri;
    Edge *edge[3];
    TriBinEntry *next;
    TriBinEntry *prev;

    int used;
    int used2;

    unsigned char adjCount;
    unsigned char pad21[3]; /* freelist elements are 4-byte aligned */
};

typedef struct TriBinList {
    TriBinEntry *head;
} TriBinList;

typedef struct MeshOpFreeLists {
    RwFreeList *binEntryFreeList;
    RwFreeList *edgeFreeList;
} MeshOpFreeLists;

typedef struct RpMeshopStatic {
    RpMeshHeader nullMeshHeader;
    RpTriStripMeshCallBack meshTristripMethod;
    void *data;
} RpMeshopStatic;

typedef char TriStripListEntrySizeCheck[
    sizeof(TriStripListEntry) == 0x10 ? 1 : -1];
typedef char TriStripListSizeCheck[sizeof(TriStripList) == 0x04 ? 1 : -1];
typedef char EdgeSizeCheck[sizeof(Edge) == 0x10 ? 1 : -1];
typedef char TriBinEntrySizeCheck[sizeof(TriBinEntry) == 0x24 ? 1 : -1];
typedef char TriBinListSizeCheck[sizeof(TriBinList) == 0x04 ? 1 : -1];
typedef char MeshOpFreeListsSizeCheck[
    sizeof(MeshOpFreeLists) == 0x08 ? 1 : -1];
typedef char RpMeshopStaticSizeCheck[
    sizeof(RpMeshopStatic) == 0x18 ? 1 : -1];

static RpMeshGlobals *MeshGlobals(void)
{
    return (RpMeshGlobals *)((unsigned char *)RwEngineInstance +
                             meshModule.globalsOffset);
}

#define MeshGlobalsInline()                                                    \
    ((RpMeshGlobals *)((unsigned char *)RwEngineInstance +                     \
                       meshModule.globalsOffset))

#define EdgeUnusedTriangleCount(edge)                                          \
    ((((edge)->tri1 != 0) && !(edge)->tri1->used ? 1 : 0) +                   \
     (((edge)->tri2 != 0) && !(edge)->tri2->used ? 1 : 0))

#define EdgeUnusedTriangleCountSecondPass(edge)                                \
    ((((edge)->tri1 != 0) && !(edge)->tri1->used2 ? 1 : 0) +                  \
     (((edge)->tri2 != 0) && !(edge)->tri2->used2 ? 1 : 0))

static RpMeshopStatic MeshopStatic = {
    {0}, RpBuildMeshGenerateDefaultTriStrip, 0};

static int SortPolygons(const void *pA, const void *pB) {
    static const unsigned int transBIT = 0x10;
    const RpBuildMeshTriangle *const *triangleA =
        (const RpBuildMeshTriangle *const *)pA;
    const RpBuildMeshTriangle *const *triangleB =
        (const RpBuildMeshTriangle *const *)pB;
    RpMaterial *materialA = (*triangleA)->material;
    RpMaterial *materialB = (*triangleB)->material;
    int orderA = 0;
    int orderB = 0;

    if (materialA == materialB) {
        return 0;
    }

    if (materialA != 0) {
        if (materialA->texture != 0) {
            RwRaster *raster = materialA->texture->raster;
            int format = (int)raster->format << 8;
            format &= 0x0F00;
            if (format == 0x0100 ||
                format == 0x0300 ||
                format == 0x0500) {
                orderA = transBIT;
            }
        }
        if (materialA->color.alpha != 0xff) {
            orderA |= transBIT;
        }
    }

    if (materialB != 0) {
        if (materialB->texture != 0) {
            RwRaster *raster = materialB->texture->raster;
            int format = (int)raster->format << 8;
            format &= 0x0F00;
            if (format == 0x0100 ||
                format == 0x0300 ||
                format == 0x0500) {
                orderB = transBIT;
            }
        }
        if (materialB->color.alpha != 0xff) {
            orderB |= transBIT;
        }
    }

    orderA |= (*triangleA)->rasterIndex > (*triangleB)->rasterIndex ? 8 : 0;
    orderB |= (*triangleA)->rasterIndex < (*triangleB)->rasterIndex ? 8 : 0;
    orderA |= (*triangleA)->pipelineIndex > (*triangleB)->pipelineIndex ? 4 : 0;
    orderB |= (*triangleA)->pipelineIndex < (*triangleB)->pipelineIndex ? 4 : 0;
    orderA |= (*triangleA)->textureIndex > (*triangleB)->textureIndex ? 2 : 0;
    orderB |= (*triangleA)->textureIndex < (*triangleB)->textureIndex ? 2 : 0;
    orderA |= (*triangleA)->matIndex > (*triangleB)->matIndex ? 1 : 0;
    orderB |= (*triangleA)->matIndex < (*triangleB)->matIndex ? 1 : 0;
    return orderA - orderB;
}

static RpMesh *SortPolygonsInTriListMesh(RpMesh *mesh,
                                         RpMeshHeader *meshHeader ,
                                         void *pData ) {
    unsigned int maxVertex = 0;
    unsigned int numTriangles = mesh->numIndices / 3;
    unsigned int tagBytes;
    unsigned int i;
    unsigned int *vertexTagBuffer;
    RxVertexIndex *indices;

    indices = mesh->indices;
    for (i = 0; i < mesh->numIndices; i++) {
        maxVertex |= indices[i];
    }

    tagBytes = (maxVertex + 31) >> 3;
    vertexTagBuffer = RwEngineInstance->fpMalloc(tagBytes,
                               0x10000 | 0x502);
    if (vertexTagBuffer != 0) {
        unsigned int arraySize = sizeof(RxVertexIndex) * mesh->numIndices;
        RxVertexIndex *oldIndices = RwEngineInstance->fpMalloc(
            arraySize, 0x10000 | 0x502);

        if (oldIndices != 0) {
            unsigned int copied = 0;
            memcpy(oldIndices, indices, arraySize);

            while (copied < numTriangles) {
                unsigned char *tag = (unsigned char *)vertexTagBuffer;
                unsigned int remaining = tagBytes;
                unsigned int triangleIndex;
                while (remaining-- != 0) {
                    *tag++ = 0;
                }

                for (triangleIndex = 0; triangleIndex < numTriangles;
                     triangleIndex++) {
                    RxVertexIndex *triangle = &oldIndices[triangleIndex * 3];
                    if ((triangle[0] & triangle[1] & triangle[2]) != 0xffff &&
                        !(vertexTagBuffer[triangle[0] >> 5] &
                          (1 << (triangle[0] & 31))) &&
                        !(vertexTagBuffer[triangle[1] >> 5] &
                          (1 << (triangle[1] & 31))) &&
                        !(vertexTagBuffer[triangle[2] >> 5] &
                          (1 << (triangle[2] & 31)))) {
                        vertexTagBuffer[triangle[0] >> 5] |=
                            1 << (triangle[0] & 31);
                        vertexTagBuffer[triangle[1] >> 5] |=
                            1 << (triangle[1] & 31);
                        vertexTagBuffer[triangle[2] >> 5] |=
                            1 << (triangle[2] & 31);
                        *indices++ = triangle[0];
                        *indices++ = triangle[1];
                        *indices++ = triangle[2];
                        triangle[0] = triangle[1] = triangle[2] = 0xffff;
                        copied++;
                    }
                }
            }
            RwEngineInstance->fpFree(oldIndices);
        }
        RwEngineInstance->fpFree(vertexTagBuffer);
    }
    return mesh;
}

static Edge *TriStripAddEdge(RwFreeList *edgeFreeList, Edge **edgelist,
                             unsigned short v1, unsigned short v2, unsigned int tri,
                             TriBinEntry **binEntryArray) {
    Edge *edge;

    for (edge = *edgelist; edge != 0; edge = edge->next) {
        if (edge->v2 == v1 && edge->v1 == v2 && edge->tri2 == 0) {
            edge->tri1->adjCount++;
            binEntryArray[tri]->adjCount++;
            edge->tri2 = binEntryArray[tri];
            return edge;
        }
    }

    edge = RwEngineInstance->fpFreeListAlloc(edgeFreeList,
                           0x30000 | 0x502);
    edge->v1 = v1;
    edge->v2 = v2;
    edge->tri1 = binEntryArray[tri];
    edge->tri2 = 0;
    edge->next = *edgelist;
    *edgelist = edge;
    return edge;
}

static TriBinEntry **
TriStripBinEntryArrayDestroy(unsigned int numTris, MeshOpFreeLists *meshOpFreeLists,
                             Edge *edge, TriBinEntry **binEntryArray) {
    unsigned int i;

    while (edge != 0) {
        Edge *next = edge->next;
        RwEngineInstance->fpFreeListFree(meshOpFreeLists->edgeFreeList, edge);
        edge = next;
    }
    RwFreeListDestroy(meshOpFreeLists->edgeFreeList);
    meshOpFreeLists->edgeFreeList = 0;

    for (i = 0; i < numTris; i++) {
        RwEngineInstance->fpFreeListFree(meshOpFreeLists->binEntryFreeList, binEntryArray[i]);
        binEntryArray[i] = 0;
    }
    RwFreeListDestroy(meshOpFreeLists->binEntryFreeList);
    meshOpFreeLists->binEntryFreeList = 0;
    RwEngineInstance->fpFree(binEntryArray);
    return 0;
}

static TriBinEntry **
TriStripBinEntryArrayCreate(unsigned int numTris, MeshOpFreeLists *meshOpFreeLists,
                            Edge **edgelist, RpBuildMeshTriangle *triList) {
    TriBinEntry **binEntryArray;
    unsigned int i;

    binEntryArray = RwEngineInstance->fpMalloc(
        numTris * sizeof(*binEntryArray),
        0x30000 | 0x502);
    meshOpFreeLists->binEntryFreeList =
        RwFreeListCreate(sizeof(TriBinEntry), numTris,
                         sizeof(binEntryArray[0]->tri),
                         0x30000 | 0x502);
    meshOpFreeLists->edgeFreeList =
        RwFreeListCreate(sizeof(Edge), numTris / 2U + 1U,
                         sizeof(binEntryArray[0]->tri),
                         0x30000 | 0x502);

    for (i = 0; i < numTris; i++) {
        binEntryArray[i] = RwEngineInstance->fpFreeListAlloc(
            meshOpFreeLists->binEntryFreeList,
            0x30000 | 0x502);
        binEntryArray[i]->adjCount = 0;
        binEntryArray[i]->tri = i;
        binEntryArray[i]->prev = 0;
        binEntryArray[i]->next = 0;
        binEntryArray[i]->used = 0;
        binEntryArray[i]->edge[0] = TriStripAddEdge(
            meshOpFreeLists->edgeFreeList, edgelist, triList[i].vertIndex[0],
            triList[i].vertIndex[1], i, binEntryArray);
        binEntryArray[i]->edge[1] = TriStripAddEdge(
            meshOpFreeLists->edgeFreeList, edgelist, triList[i].vertIndex[1],
            triList[i].vertIndex[2], i, binEntryArray);
        binEntryArray[i]->edge[2] = TriStripAddEdge(
            meshOpFreeLists->edgeFreeList, edgelist, triList[i].vertIndex[2],
            triList[i].vertIndex[0], i, binEntryArray);
    }

    return binEntryArray;
}

static Edge *TriStripGetTriEdge(TriBinEntry *binEntry, unsigned short v1,
                                unsigned short v2) {
    if ((binEntry->edge[0]->v1 == v1 && binEntry->edge[0]->v2 == v2) ||
        (binEntry->edge[0]->v1 == v2 && binEntry->edge[0]->v2 == v1))
        return binEntry->edge[0];

    if ((binEntry->edge[1]->v1 == v1 && binEntry->edge[1]->v2 == v2) ||
        (binEntry->edge[1]->v1 == v2 && binEntry->edge[1]->v2 == v1))
        return binEntry->edge[1];

    if ((binEntry->edge[2]->v1 == v1 && binEntry->edge[2]->v2 == v2) ||
        (binEntry->edge[2]->v1 == v2 && binEntry->edge[2]->v2 == v1))
        return binEntry->edge[2];

    return 0;
}

static void TriStripMarkTriUsed(TriBinEntry *tri, TriBinList *binListArray,
                                int currentAttempt) {
    unsigned char i;

    if (currentAttempt < 4) {
        tri->used2 = 1;
        return;
    }

    tri->used = 1;
    if (binListArray[tri->adjCount].head == tri) {
        binListArray[tri->adjCount].head =
            binListArray[tri->adjCount].head->next;
        if (binListArray[tri->adjCount].head != 0) {
            binListArray[tri->adjCount].head->prev = 0;
        }
    } else {
        if (tri->next != 0) {
            tri->next->prev = tri->prev;
        }
        if (tri->prev != 0) {
            tri->prev->next = tri->next;
        }
    }

    for (i = 0; i < 3; i++) {
        TriBinEntry *adjacent = 0;

        if (tri->edge[i]->tri1 != 0 && tri->edge[i]->tri1 != tri &&
            !tri->edge[i]->tri1->used) {
            adjacent = tri->edge[i]->tri1;
        } else if (tri->edge[i]->tri2 != 0 && !tri->edge[i]->tri2->used) {
            adjacent = tri->edge[i]->tri2;
        }
        if (adjacent == 0) {
            continue;
        }

        if (binListArray[adjacent->adjCount].head == adjacent) {
            binListArray[adjacent->adjCount].head =
                binListArray[adjacent->adjCount].head->next;
            if (binListArray[adjacent->adjCount].head != 0) {
                binListArray[adjacent->adjCount].head->prev = 0;
            }
        } else {
            if (adjacent->next != 0) {
                adjacent->next->prev = adjacent->prev;
            }
            if (adjacent->prev != 0) {
                adjacent->prev->next = adjacent->next;
            }
        }

        adjacent->adjCount--;
        adjacent->next = binListArray[adjacent->adjCount].head;
        if (adjacent->next != 0) {
            adjacent->next->prev = adjacent;
        }
        binListArray[adjacent->adjCount].head = adjacent;
        adjacent->prev = 0;
    }
}

static unsigned int TriStripFollow(TriStripListEntry *strip, Edge *nextEdge,
                               TriBinList *binListArray,
                               RpBuildMeshTriangle *triList ,
                               int currentAttempt) {
    unsigned int trianglesAdded = 0;
    Edge *alternateEdge = 0;

    while (nextEdge != 0) {
        TriBinEntry *triangle = 0;
        Edge *incomingEdge;
        unsigned short thirdVertex;
        int incomingIndex = -1;
        int turn;

        if (currentAttempt < 4) {
            if (nextEdge->tri1 != 0 && !nextEdge->tri1->used2) {
                triangle = nextEdge->tri1;
            } else if (nextEdge->tri2 != 0 && !nextEdge->tri2->used2) {
                triangle = nextEdge->tri2;
            }
        } else {
            if (nextEdge->tri1 != 0 && !nextEdge->tri1->used) {
                triangle = nextEdge->tri1;
            } else if (nextEdge->tri2 != 0 && !nextEdge->tri2->used) {
                triangle = nextEdge->tri2;
            }
        }
        if (triangle == 0) {
            return trianglesAdded;
        }

        trianglesAdded++;
        TriStripMarkTriUsed(triangle, binListArray, currentAttempt);

        if (nextEdge == triangle->edge[0]) {
            incomingIndex = 0;
        } else if (nextEdge == triangle->edge[1]) {
            incomingIndex = 1;
        } else if (nextEdge == triangle->edge[2]) {
            incomingIndex = 2;
        }

        if (triangle->edge[(incomingIndex + 1) % 3]->tri1 == triangle) {
            thirdVertex = triangle->edge[(incomingIndex + 1) % 3]->v2;
        } else {
            thirdVertex = triangle->edge[(incomingIndex + 1) % 3]->v1;
        }

        incomingEdge = nextEdge;
        nextEdge = TriStripGetTriEdge(
            triangle, strip->strip[strip->stripLen - 1], thirdVertex);
        if (currentAttempt < 4) {
            turn = nextEdge == 0 || EdgeUnusedTriangleCountSecondPass(nextEdge) == 0;
        } else {
            turn = nextEdge == 0 || EdgeUnusedTriangleCount(nextEdge) == 0;
        }

        if (turn) {
            if (triangle->edge[0] != incomingEdge &&
                triangle->edge[0] != nextEdge) {
                alternateEdge = triangle->edge[0];
            } else if (triangle->edge[1] != incomingEdge &&
                       triangle->edge[1] != nextEdge) {
                alternateEdge = triangle->edge[1];
            } else if (triangle->edge[2] != incomingEdge &&
                       triangle->edge[2] != nextEdge) {
                alternateEdge = triangle->edge[2];
            }

            if (currentAttempt < 4) {
                turn = alternateEdge != 0 && EdgeUnusedTriangleCountSecondPass(alternateEdge) != 0;
            } else {
                turn = alternateEdge != 0 && EdgeUnusedTriangleCount(alternateEdge) != 0;
            }

            if (turn) {
                if (strip->stripLen & 1) {
                    strip->strip[strip->stripLen] = thirdVertex;
                    strip->stripLen++;
                    nextEdge = 0;
                } else {
                    strip->strip[strip->stripLen] =
                        strip->strip[strip->stripLen - 2];
                    strip->stripLen++;
                    strip->strip[strip->stripLen] = thirdVertex;
                    strip->stripLen++;
                    nextEdge = alternateEdge;
                }
            } else {
                strip->strip[strip->stripLen] = thirdVertex;
                strip->stripLen++;
                nextEdge = 0;
            }
        } else {
            strip->strip[strip->stripLen] = thirdVertex;
            strip->stripLen++;
        }
    }
    return trianglesAdded;
}

static int TriStripStripTris(RpBuildMeshTriangle *triList, unsigned int numTris,
                                TriStripList *stripList, int preprocess) {
    Edge *edgeList = 0;
    Edge *forwardEdge;
    Edge *reverseEdge;
    TriBinList bins[4];
    MeshOpFreeLists freeLists;
    TriBinEntry **entries;
    TriStripListEntry *forward;
    TriStripListEntry *reverse;
    unsigned int consumed = 0;
    unsigned int i;

    bins[0].head = 0;
    bins[1].head = 0;
    bins[2].head = 0;
    bins[3].head = 0;
    freeLists.binEntryFreeList = 0;
    freeLists.edgeFreeList = 0;

    entries = TriStripBinEntryArrayCreate(numTris, &freeLists, &edgeList,
                                          triList);
    for (i = 0; i < numTris; i++) {
        entries[i]->next = bins[entries[i]->adjCount].head;
        if (entries[i]->next != 0) {
            entries[i]->next->prev = entries[i];
        }
        bins[entries[i]->adjCount].head = entries[i];
        entries[i]->prev = 0;
    }

    forward = RwEngineInstance->fpFreeListAlloc(MeshGlobalsInline()->triStripListEntryFreeList,
                              0x10000 | 0x502);
    reverse = RwEngineInstance->fpFreeListAlloc(MeshGlobalsInline()->triStripListEntryFreeList,
                              0x10000 | 0x502);
    forward->stripSize = reverse->stripSize = numTris * 2 + 2;
    forward->stripLen = reverse->stripLen = 0;
    forward->strip = RwEngineInstance->fpMalloc(sizeof(unsigned int) * forward->stripSize,
                              0x10000 | 0x502);
    reverse->strip = RwEngineInstance->fpMalloc(sizeof(unsigned int) * reverse->stripSize,
                              0x10000 | 0x502);

    while (consumed < numTris) {
        TriStripListEntry *output;
        unsigned int bestLength = 0;
        int bestRotation = 0;
        int attempt = preprocess ? 0 : 3;

        if (bins[0].head != 0) {
            TriBinEntry *isolated = bins[0].head;
            RpBuildMeshTriangle *triangle = &triList[isolated->tri];

            output = RwEngineInstance->fpFreeListAlloc(MeshGlobalsInline()->triStripListEntryFreeList,
                                     0x30000 | 0x502);
            output->next = stripList->head;
            stripList->head = output;
            output->stripSize = output->stripLen = 3;
            output->strip = RwEngineInstance->fpMalloc(sizeof(unsigned int) * 3,
                                     0x30000 | 0x502);
            output->strip[0] = triangle->vertIndex[0];
            output->strip[1] = triangle->vertIndex[1];
            output->strip[2] = triangle->vertIndex[2];
            isolated->used = isolated->used2 = 1;
            bins[0].head = isolated->next;
            if (bins[0].head != 0) {
                bins[0].head->prev = 0;
            }
            consumed++;
        } else {
            TriBinEntry *starter;
            RpBuildMeshTriangle *source;
            unsigned int savedConsumed = consumed;
            unsigned int initialRotation;
            unsigned int rotation;
            unsigned int j;

            i = 1;
            while (bins[i].head == 0) {
                i++;
            }
            starter = bins[i].head;

            if (EdgeUnusedTriangleCount(starter->edge[2]) >= 2 &&
                EdgeUnusedTriangleCount(starter->edge[1]) >= 2) {
                rotation = 1;
            } else if (EdgeUnusedTriangleCount(starter->edge[0]) >= 2 &&
                       EdgeUnusedTriangleCount(starter->edge[2]) >= 2) {
                rotation = 2;
            } else if (EdgeUnusedTriangleCount(starter->edge[1]) >= 2 &&
                       EdgeUnusedTriangleCount(starter->edge[0]) >= 2) {
                rotation = 0;
            } else if (EdgeUnusedTriangleCount(starter->edge[0]) >
                       EdgeUnusedTriangleCount(starter->edge[1])) {
                rotation = EdgeUnusedTriangleCount(starter->edge[0]) >
                                   EdgeUnusedTriangleCount(starter->edge[2])
                               ? 2
                               : 1;
            } else {
                rotation = EdgeUnusedTriangleCount(starter->edge[1]) >
                                   EdgeUnusedTriangleCount(starter->edge[2])
                               ? 0
                               : 1;
            }
            initialRotation = rotation;
            bestRotation = rotation;

            do {
                int canReverse;
                consumed = savedConsumed;
                for (j = 0; j < numTris; j++) {
                    entries[j]->used2 = entries[j]->used;
                }

                switch (attempt++) {
                case 0:
                    rotation = initialRotation;
                    break;
                case 1:
                    rotation = (initialRotation + 1) % 3;
                    break;
                case 2:
                    rotation = (initialRotation + 2) % 3;
                    break;
                default:
                    rotation = bestRotation;
                    break;
                }

                switch (rotation) {
                case 0:
                    forwardEdge = starter->edge[1];
                    reverseEdge = starter->edge[0];
                    break;
                case 1:
                    forwardEdge = starter->edge[2];
                    reverseEdge = starter->edge[1];
                    break;
                case 2:
                    forwardEdge = starter->edge[0];
                    reverseEdge = starter->edge[2];
                    break;
                }
                source = &triList[starter->tri];
                forward->strip[0] = source->vertIndex[rotation];
                forward->strip[1] = source->vertIndex[(rotation + 1) % 3];
                forward->strip[2] = source->vertIndex[(rotation + 2) % 3];
                forward->stripLen = 3;

                TriStripMarkTriUsed(starter, bins, attempt);
                consumed++;
                consumed += TriStripFollow(forward, forwardEdge, bins, triList,
                                           attempt);
                canReverse = attempt < 4 ? EdgeUnusedTriangleCountSecondPass(reverseEdge) != 0
                                         : EdgeUnusedTriangleCount(reverseEdge) != 0;

                if (canReverse) {
                    reverse->strip[0] = forward->strip[1];
                    reverse->strip[1] = forward->strip[0];
                    reverse->stripLen = 2;
                    consumed += TriStripFollow(reverse, reverseEdge, bins,
                                               triList, attempt);
                    if (reverse->stripLen & 1) {
                        reverse->strip[reverse->stripLen] =
                            reverse->strip[reverse->stripLen - 2];
                        reverse->stripLen++;
                    }
                    if (consumed > bestLength) {
                        bestLength = consumed;
                        bestRotation = rotation;
                    }
                    if (attempt < 4)
                        continue;

                    output = RwEngineInstance->fpFreeListAlloc(
                        MeshGlobalsInline()->triStripListEntryFreeList,
                        0x30000 | 0x502);
                    output->next = stripList->head;
                    stripList->head = output;
                    output->stripSize =
                        forward->stripLen + reverse->stripLen - 2;
                    output->stripLen = 0;
                    output->strip = RwEngineInstance->fpMalloc(
                        sizeof(unsigned int) * output->stripSize,
                        0x30000 | 0x502);
                    while (reverse->stripLen > 2) {
                        output->strip[output->stripLen++] =
                            reverse->strip[--reverse->stripLen];
                    }
                    memcpy(output->strip + output->stripLen, forward->strip,
                           sizeof(unsigned int) * forward->stripLen);
                    output->stripLen = output->stripSize;
                } else {
                    if (consumed > bestLength) {
                        bestLength = consumed;
                        bestRotation = rotation;
                    }
                    if (attempt < 4)
                        continue;

                    output = RwEngineInstance->fpFreeListAlloc(
                        MeshGlobalsInline()->triStripListEntryFreeList,
                        0x30000 | 0x502);
                    output->next = stripList->head;
                    stripList->head = output;
                    output->stripSize = output->stripLen = forward->stripLen;
                    output->strip = RwEngineInstance->fpMalloc(
                        sizeof(unsigned int) * output->stripLen,
                        0x30000 | 0x502);
                    memcpy(output->strip, forward->strip,
                           sizeof(unsigned int) * forward->stripLen);
                }
            } while (attempt < 4);
        }
    }

    RwEngineInstance->fpFree(reverse->strip);
    RwEngineInstance->fpFreeListFree(MeshGlobalsInline()->triStripListEntryFreeList, reverse);
    RwEngineInstance->fpFree(forward->strip);
    RwEngineInstance->fpFreeListFree(MeshGlobalsInline()->triStripListEntryFreeList, forward);
    TriStripBinEntryArrayDestroy(numTris, &freeLists, edgeList, entries);
    return 1;
}

static int TriStripJoin(TriStripList *stripList, int maintainWinding) {
    TriStripListEntry *joined;
    TriStripListEntry *remaining;
    unsigned int i;

    if (stripList->head == 0) {
        return 0;
    }

    joined = RwEngineInstance->fpFreeListAlloc(
        MeshGlobalsInline()->triStripListEntryFreeList,
        0x30000 | 0x502);
    joined->stripLen = 0;
    joined->stripSize = 0;

    for (remaining = stripList->head; remaining != 0;
         remaining = remaining->next) {
        joined->stripSize += remaining->stripLen + 6;
    }
    joined->strip = RwEngineInstance->fpMalloc(sizeof(unsigned int) * joined->stripSize,
                             0x30000 | 0x502);

    remaining = stripList->head;
    for (i = 0; i < remaining->stripLen; i++) {
        joined->strip[joined->stripLen++] = remaining->strip[i];
    }
    RwEngineInstance->fpFree(remaining->strip);
    remaining->strip = 0;
    {
        TriStripListEntry *next = remaining->next;
        RwEngineInstance->fpFreeListFree(MeshGlobalsInline()->triStripListEntryFreeList, remaining);
        remaining = next;
    }

    while (remaining != 0) {
        TriStripListEntry *selected;
        TriStripListEntry *previous;

        for (selected = remaining; selected != 0; selected = selected->next) {
            if (selected->strip[0] == joined->strip[joined->stripLen - 1]) {
                if ((joined->stripLen & 1) && maintainWinding) {
                    joined->strip[joined->stripLen++] = selected->strip[0];
                }
                break;
            }
        }

        if (selected == 0) {
            for (selected = remaining; selected != 0;
                 selected = selected->next) {
                if (selected->strip[0] == joined->strip[joined->stripLen - 2]) {
                    if (!(joined->stripLen & 1) && maintainWinding) {
                        joined->strip[joined->stripLen++] = selected->strip[0];
                        joined->strip[joined->stripLen++] = selected->strip[0];
                    } else {
                        joined->strip[joined->stripLen++] = selected->strip[0];
                    }
                    break;
                }
            }
        }

        if (selected == 0) {
            selected = remaining;
            if (!(joined->stripLen & 1) || !maintainWinding) {
                joined->strip[joined->stripLen] =
                    joined->strip[joined->stripLen - 1];
                joined->strip[joined->stripLen + 1] = selected->strip[0];
                joined->stripLen += 2;
            } else {
                joined->strip[joined->stripLen] =
                    joined->strip[joined->stripLen - 1];
                joined->strip[joined->stripLen + 1] = selected->strip[0];
                joined->strip[joined->stripLen + 2] = selected->strip[0];
                joined->stripLen += 3;
            }
        }

        for (i = 0; i < selected->stripLen; i++) {
            joined->strip[joined->stripLen++] = selected->strip[i];
        }
        RwEngineInstance->fpFree(selected->strip);
        selected->strip = 0;

        if (remaining == selected) {
            remaining = selected->next;
            RwEngineInstance->fpFreeListFree(
                MeshGlobalsInline()->triStripListEntryFreeList, selected);
        } else {
            previous = remaining;
            while (previous->next != selected) {
                previous = previous->next;
            }
            previous->next = selected->next;
            RwEngineInstance->fpFreeListFree(
                MeshGlobalsInline()->triStripListEntryFreeList, selected);
        }
    }

    stripList->head = joined;
    joined->next = 0;
    return 1;
}

static RpMeshHeader *TriStripMeshGenerate(RpBuildMesh *mesh, int preprocess,
                                          int maintainWinding) {
    RpBuildMeshTriangle **sorted;
    RpBuildMeshTriangle **source;
    RpMesh **generated;
    RpMesh *materialRuns;
    RpMeshHeader *header;
    unsigned short generatedCount = 0;
    unsigned short materialCount = 1;
    unsigned int runCount;
    unsigned int totalIndices = 0;
    unsigned int headerSize = sizeof(RpMeshHeader);
    unsigned int i;

    sorted = RwEngineInstance->fpMalloc(
        mesh->numTriangles * sizeof(RpBuildMeshTriangle *),
        0x10000 | 0x502);
    if (sorted == 0) {
        return 0;
    }

    for (i = 0; i < mesh->numTriangles; i++) {
        sorted[i] = &mesh->meshTriangles[i];
    }
    qsort(sorted, mesh->numTriangles, sizeof(*sorted), SortPolygons);

    if (mesh->numTriangles >= 2) {
        RpMaterial *previous = sorted[0]->material;
        for (i = 1; i < mesh->numTriangles; i++) {
            if (sorted[i]->material != previous) {
                previous = sorted[i]->material;
                materialCount++;
            }
        }
    }

    generated = RwEngineInstance->fpMalloc(sizeof(*generated) * materialCount,
                         0x10000 | 0x502 |
                             0x01000000);
    materialRuns = RwEngineInstance->fpMalloc(sizeof(*materialRuns) * materialCount,
                            0x10000 | 0x502);
    runCount = 1;
    materialRuns[0].indices = 0;
    materialRuns[0].numIndices = 0;
    materialRuns[0].material = sorted[0]->material;
    if (mesh->numTriangles >= 2) {
        for (i = 0; i < mesh->numTriangles - 1; i++) {
            if (sorted[i]->material != sorted[i + 1]->material) {
                materialRuns[runCount].indices = 0;
                materialRuns[runCount].numIndices = i + 1;
                materialRuns[runCount].material = sorted[i + 1]->material;
                materialRuns[runCount - 1].numIndices =
                    i + 1 - materialRuns[runCount - 1].numIndices;
                runCount++;
            }
        }
    }
    materialRuns[runCount - 1].numIndices =
        mesh->numTriangles - materialRuns[runCount - 1].numIndices;

    MeshGlobalsInline()->triStripListEntryFreeList = RwFreeListCreate(
        sizeof(TriStripListEntry), (mesh->numTriangles / 10) + 5,
        sizeof(unsigned int), 0x10000 | 0x502);

    source = sorted;
    for (i = 0; i < runCount; i++) {
        TriStripList strips;
        TriStripListEntry *entry;
        RpBuildMeshTriangle *triangles;
        unsigned int j;

        strips.head = 0;
        triangles = RwEngineInstance->fpMalloc(
            sizeof(*triangles) * materialRuns[i].numIndices,
            0x10000 | 0x502);
        for (j = 0; j < materialRuns[i].numIndices; j++) {
            triangles[j] = **source++;
        }

        TriStripStripTris(triangles, materialRuns[i].numIndices, &strips,
                          preprocess);
        TriStripJoin(&strips, maintainWinding);

        for (entry = strips.head; entry != 0; entry = entry->next) {
            unsigned int bytes =
                sizeof(RpMesh) + sizeof(RxVertexIndex) * entry->stripLen;
            RpMesh *output = RwEngineInstance->fpMalloc(
                bytes, 0x10000 | 0x502);
            output->indices = (RxVertexIndex *)(output + 1);
            output->numIndices = entry->stripLen;
            output->material = materialRuns[i].material;
            for (j = 0; j < output->numIndices; j++) {
                output->indices[j] = entry->strip[j];
            }
            generated[generatedCount++] = output;
        }

        while (strips.head != 0) {
            entry = strips.head;
            strips.head = entry->next;
            RwEngineInstance->fpFree(entry->strip);
            RwEngineInstance->fpFreeListFree(MeshGlobalsInline()->triStripListEntryFreeList, entry);
        }
        RwEngineInstance->fpFree(triangles);
    }

    RwFreeListDestroy(MeshGlobalsInline()->triStripListEntryFreeList);
    MeshGlobalsInline()->triStripListEntryFreeList = 0;

    for (i = 0; i < generatedCount; i++) {
        headerSize += sizeof(RpMesh) +
                      sizeof(RxVertexIndex) * generated[i]->numIndices;
        totalIndices += generated[i]->numIndices;
    }

    header = _rpMeshHeaderCreate(headerSize);
    header->flags = 1;
    header->numMeshes = generatedCount;
    header->serialNum = MeshGlobalsInline()->nextSerialNum++;
    header->firstMeshOffset = 0;
    header->totalIndices = totalIndices;

    {
        RpMesh *destination = (RpMesh *)(header + 1);
        RxVertexIndex *destinationIndices =
            (RxVertexIndex *)(destination + generatedCount);
        for (i = 0; i < generatedCount; i++) {
            unsigned int bytes =
                sizeof(RxVertexIndex) * generated[i]->numIndices;
            destination->indices = destinationIndices;
            destination->numIndices = generated[i]->numIndices;
            destination->material = generated[i]->material;
            memcpy(destinationIndices, generated[i]->indices, bytes);
            destinationIndices += destination->numIndices;
            destination++;
            RwEngineInstance->fpFree(generated[i]);
        }
    }

    RwEngineInstance->fpFree(sorted);
    RwEngineInstance->fpFree(generated);
    RwEngineInstance->fpFree(materialRuns);
    return header;
}

RpMeshHeader *RpBuildMeshGenerateDefaultTriStrip(RpBuildMesh *buildMesh,
                                                 void *data ) {
    return TriStripMeshGenerate(buildMesh, 0, 1);
}

RpMeshHeader *_rpTriListMeshGenerate(RpBuildMesh *buildMesh,
                                     void *data ) {
    RpBuildMeshTriangle **triPointers;
    RpMeshHeader *header;
    RpMesh *outputMesh;
    RxVertexIndex *outputIndex;
    unsigned int materialCount = 1;
    unsigned int meshSize;
    unsigned int i;

    triPointers = RwEngineInstance->fpMalloc(
        buildMesh->numTriangles * sizeof(RpBuildMeshTriangle *),
        0x10000 | 0x502);
    if (triPointers == 0) {
        MeshReportAllocationFailure(
            buildMesh->numTriangles * sizeof(RpBuildMeshTriangle *));
        return 0;
    }

    for (i = 0; i < buildMesh->numTriangles; i++) {
        triPointers[i] = &buildMesh->meshTriangles[i];
    }
    qsort(triPointers, buildMesh->numTriangles, sizeof(*triPointers),
          SortPolygons);

    if (buildMesh->numTriangles >= 2) {
        RpMaterial *previous = triPointers[0]->material;
        for (i = 1; i < buildMesh->numTriangles; i++) {
            if (triPointers[i]->material != previous) {
                previous = triPointers[i]->material;
                materialCount++;
            }
        }
    }

    meshSize = sizeof(RpMeshHeader) + sizeof(RpMesh) * materialCount +
               sizeof(RxVertexIndex) * 3 * buildMesh->numTriangles;
    header = _rpMeshHeaderCreate(meshSize);
    if (header == 0) {
        RwEngineInstance->fpFree(triPointers);
        MeshReportAllocationFailure(meshSize);
        return 0;
    }

    header->flags = 0;
    header->numMeshes = 1;
    header->serialNum = MeshGlobals()->nextSerialNum++;
    header->firstMeshOffset = 0;
    header->totalIndices = buildMesh->numTriangles * 3;

    outputMesh = (RpMesh *)(header + 1);
    outputIndex = (RxVertexIndex *)(outputMesh + materialCount);
    outputMesh->indices = outputIndex;
    outputMesh->numIndices = 0;
    outputMesh->material = triPointers[0]->material;

    for (i = 0; i < buildMesh->numTriangles; i++) {
        RpBuildMeshTriangle *triangle = triPointers[i];
        if (triangle->material != outputMesh->material) {
            outputMesh++;
            outputMesh->indices = outputIndex;
            outputMesh->numIndices = 0;
            outputMesh->material = triangle->material;
            header->numMeshes++;
        }
        *outputIndex++ = triangle->vertIndex[0];
        *outputIndex++ = triangle->vertIndex[1];
        *outputIndex++ = triangle->vertIndex[2];
        outputMesh->numIndices += 3;
    }

    _rpMeshHeaderForAllMeshes(header, SortPolygonsInTriListMesh, 0);
    RwEngineInstance->fpFree(triPointers);
    return header;
}

RpMeshHeader *_rpMeshOptimise(RpBuildMesh *mesh, int flags) {
    RpTriStripMeshCallBack generate = 0;
    void *generateData = 0;
    RpMeshHeader *result;

    if (mesh != 0) {
        if (mesh->numTriangles == 0) {
            _rpBuildMeshDestroy(mesh);
            return &MeshopStatic.nullMeshHeader;
        }

        if (flags & 1) {
            generate = MeshopStatic.meshTristripMethod;
            generateData = MeshopStatic.data;
        } else {
            generate = _rpTriListMeshGenerate;
            generateData = 0;
        }

        result = generate(mesh, generateData);
        if (result != 0) {
            _rpBuildMeshDestroy(mesh);
            return result;
        }
    }
    return 0;
}
