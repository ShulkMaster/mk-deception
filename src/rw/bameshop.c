/*
 * Purpose: Provide construction and enumeration facilities for meshes.
 *
 * For more on triangle strip-ification algorithms, see
 * o Stripe
 *   http://www.cs.sunysb.edu/~stripe/
 * o Fast and Effective Stripification of Polygonal Surface Models
 *   http://www.cosy.sbg.ac.at/~held/projects/strips/strips.html
 *   http://www.gvu.gatech.edu/gvu/i3dg/papers.html
 *
 * Copyright (c) 1998 Criterion Software Ltd.
 */

/****************************************************************************
 Includes
 */

#include "libmkparticle/rw_engine.h"
#include "rw/rpworld_types.h"
#include "rw/rwerror.h"
#include "rw/rwfreelist.h"

typedef RpMeshHeader *(*RpTriStripMeshCallBack)(RpBuildMesh *, void *);

typedef struct rpMeshGlobals {
    RwInt16 nextSerialNum;
    RwUInt16 reserved02;
    RwFreeList *triStripListEntryFreeList;
    RwUInt8 meshFlags[0x20];
    RwUInt8 primitiveType[6];
} rpMeshGlobals;

extern RwModuleInfo meshModule;
RpMeshHeader *RpBuildMeshGenerateDefaultTriStrip(RpBuildMesh *, void *);
extern void qsort(void *, RwUInt32, RwUInt32,
                  RwInt32 (*)(const void *, const void *));

#define RWFUNCTION(name) ((void)0)
#define RWAPIFUNCTION(name) ((void)0)
#define RWSTRING(value) value
#define RWASSERT(condition) ((void)0)
#define RWRETURN(value) return (value)
#define __RWUNUSED__
#define rwID_MESHMODULE 0x502
#define rwMEMHINTDUR_FUNCTION 0x10000
#define rwMEMHINTDUR_EVENT 0x30000
#define rwMEMHINTFLAG_RESIZABLE 0x01000000
#define rpMESHHEADERTRISTRIP 1
#define totalIndicesInMesh totalIndices
#define rwRASTERFORMATPIXELFORMATMASK 0x0F00
#define rwRASTERFORMAT1555 0x0100
#define rwRASTERFORMAT4444 0x0300
#define rwRASTERFORMAT8888 0x0500
#define RwTextureGetRaster(texture) ((texture)->raster)
#define RwRasterGetFormat(raster) ((RwInt32)(raster)->format << 8)
#define RwMalloc(size, hint) RwEngineInstance->fpMalloc((size), (hint))
#define RwRealloc(memory, size, hint)                                          \
    RwEngineInstance->fpRealloc((memory), (size), (hint))
#define RwFree(memory) RwEngineInstance->fpFree((memory))
#define RwFreeListAlloc(list, hint)                                            \
    RwEngineInstance->fpFreeListAlloc((list), (hint))
#define RwFreeListFree(list, memory)                                           \
    RwEngineInstance->fpFreeListFree((list), (memory))
#define E_RW_NOMEM 0x80000013
#define RWERROR_IMPL(code, value)                                              \
    do {                                                                       \
        RwError error;                                                         \
        error.pluginID = 2;                                                    \
        error.errorCode = _rwerror((code), (value));                           \
        RwErrorSet(&error);                                                    \
    } while (0)
#define RWERROR(args) RWERROR_IMPL args
#define MACRO_START do
#define MACRO_STOP while (0)

/****************************************************************************
 Local Types
 */

typedef struct TriStripListEntry TriStripListEntry;
struct TriStripListEntry {
    RwUInt16 *strip;
    RwUInt32 stripLen;
    RwUInt32 stripSize;
    TriStripListEntry *next;
};

typedef struct TriStripList TriStripList;
struct TriStripList {
    TriStripListEntry *head;
};

typedef struct TriBinEntry TriBinEntry;

typedef struct Edge Edge;
struct Edge {
    RwUInt16 v1, v2;   /* Two vertices involved in this edge       */
    TriBinEntry *tri1; /* Triangles sharing this edge              */
    TriBinEntry *tri2;
    Edge *next; /* Next edge                                */
};

struct TriBinEntry {
    RwUInt32 tri;
    Edge *edge[3];
    TriBinEntry *next;
    TriBinEntry *prev;

    RwBool used;
    RwBool used2;

    RwUInt8 adjCount;
};

typedef struct TriBinList TriBinList;
struct TriBinList {
    TriBinEntry *head;
};

typedef struct MeshOpFreeLists MeshOpFreeLists;
struct MeshOpFreeLists {
    RwFreeList *binEntryFreeList;
    RwFreeList *edgeFreeList;
};

typedef struct RpMeshopStatic RpMeshopStatic;
struct RpMeshopStatic {
    RpMeshHeader nullMeshHeader;
    RpTriStripMeshCallBack meshTristripMethod;
    void *data;
};

/****************************************************************************
 Local (Static) Prototypes
 */

#if (defined(_WINDOWS))
#define __RWCDECL __cdecl
#endif /* (defined(_WINDOWS)) */

#if (!defined(__RWCDECL))
#define __RWCDECL /* No op */
#endif            /* (!defined(__RWCDECL)) */

/****************************************************************************
 Local Defines
 */

#define RPMESHGLOBAL(var)                                                      \
    (((rpMeshGlobals *)((RwUInt8 *)RwEngineInstance +                          \
                        meshModule.globalsOffset))                             \
         ->var)

#define RasterHasAlpha(_type)                                                  \
    (((_type) == rwRASTERFORMAT1555) || ((_type) == rwRASTERFORMAT4444) ||     \
     ((_type) == rwRASTERFORMAT8888))

#define JOINSTRIPS
#define TURNCORNERS
#define ODDTURNSx
#define TRISTRIPLOOKAHEADx

/****************************************************************************/

#define TESTVERTEXBIT(vertexIndex)                                             \
    (vertexTagBuffer[(vertexIndex) >> 5] & (1 << ((vertexIndex) & 31)))

#define SETVERTEXBIT(vertexIndex)                                              \
    vertexTagBuffer[(vertexIndex) >> 5] |= (1 << ((vertexIndex) & 31))

#define private_memset(_s, _c, _n)                                             \
    MACRO_START {                                                              \
        char *cs = (char *)(_s);                                               \
        char c = (char)(_c);                                                   \
        RwUInt32 count = _n;                                                   \
                                                                               \
        while (count--) {                                                      \
            *cs++ = c;                                                         \
        }                                                                      \
    }                                                                          \
    MACRO_STOP

#define EdgeAdjCount(edge)                                                     \
    (((edge)->tri1 && !((edge)->tri1->used)) +                                 \
     ((edge)->tri2 && !((edge)->tri2->used)))

#define EdgeAdjCount2(edge)                                                    \
    (((edge)->tri1 && !((edge)->tri1->used2)) +                                \
     ((edge)->tri2 && !((edge)->tri2->used2)))

/****************************************************************************
 Globals (across program)
 */

/****************************************************************************
 Local (static) Globals
 */

static RpMeshopStatic MeshopStatic = {
    {0, 0, 0, 0, 0}, RpBuildMeshGenerateDefaultTriStrip, FALSE};

/****************************************************************************
 Local (static) Functions
 */

/****************************************************************************
 SortPolygons

 On entry   :
 On exit    :
 */
static int __RWCDECL SortPolygons(const void *pA, const void *pB) {
    /* Stock ordering is exact; residue only normalizes raster's format byte. */
    const RwUInt32 transBIT = 16;
    const RwUInt32 rastBIT = 8;
    const RwUInt32 pipeBIT = 4;
    const RwUInt32 texBIT = 2;
    const RwUInt32 matBIT = 1;

    const RpBuildMeshTriangle *const *mtpA =
        (const RpBuildMeshTriangle *const *)pA;
    const RpBuildMeshTriangle *const *mtpB =
        (const RpBuildMeshTriangle *const *)pB;

    RpMaterial *materialA = (*mtpA)->material;
    RpMaterial *materialB = (*mtpB)->material;

    RwRaster *rasterA = (RwRaster *)NULL;
    RwRaster *rasterB = (RwRaster *)NULL;
    RwTexture *textureA = (RwTexture *)NULL;
    RwTexture *textureB = (RwTexture *)NULL;

    /* IMO use UInts for bitfields! Sign bits are nothing but trouble... */
    RwUInt32 orderA = 0;
    RwUInt32 orderB = 0;

    RWFUNCTION(RWSTRING("SortPolygons"));

    /* Easy case first */
    if (materialA == materialB) {
        RWRETURN(0);
    }

    /* We sort on:
     *   transparency > raster > pipeline > texture > material
     *
     * Transparency is required for correct alpha render ordering.
     * Raster upload is the greatest cost.
     * Pipeline swap might be a significant cost - vector code upload, CPU-side
     * code cache miss. Texture state changes might also hurt even with the same
     * raster. (?) Sorting things in memory order (i.e on RpMaterial pointer) is
     * probably generally a good thing, pff...
     */

    if (materialA) {
        /* Place transparent materials after non transparent ones */
        if (materialA->texture) {
            textureA = materialA->texture;
            rasterA = RwTextureGetRaster(textureA);

            if (RasterHasAlpha(RwRasterGetFormat(rasterA) &
                               (RwInt32)rwRASTERFORMATPIXELFORMATMASK)) {
                orderA |= transBIT;
            }
        }

        if (materialA->color.alpha != 0xff) {
            orderA |= transBIT;
        }
    }

    if (materialB) {
        /* Place transparent materials after non transparent ones */
        if (materialB->texture) {
            textureB = materialB->texture;
            rasterB = RwTextureGetRaster(textureB);

            if (RasterHasAlpha(RwRasterGetFormat(rasterB) &
                               (RwInt32)rwRASTERFORMATPIXELFORMATMASK)) {
                orderB |= transBIT;
            }
        }

        if (materialB->color.alpha != 0xff) {
            orderB |= transBIT;
        }
    }

    orderA |= ((*mtpA)->rasterIndex > (*mtpB)->rasterIndex) ? rastBIT : 0;
    orderB |= ((*mtpA)->rasterIndex < (*mtpB)->rasterIndex) ? rastBIT : 0;

    orderA |= ((*mtpA)->pipelineIndex > (*mtpB)->pipelineIndex) ? pipeBIT : 0;
    orderB |= ((*mtpA)->pipelineIndex < (*mtpB)->pipelineIndex) ? pipeBIT : 0;

    orderA |= ((*mtpA)->textureIndex > (*mtpB)->textureIndex) ? texBIT : 0;
    orderB |= ((*mtpA)->textureIndex < (*mtpB)->textureIndex) ? texBIT : 0;

    orderA |= ((*mtpA)->matIndex > (*mtpB)->matIndex) ? matBIT : 0;
    orderB |= ((*mtpA)->matIndex < (*mtpB)->matIndex) ? matBIT : 0;

    RWRETURN(orderA - orderB);
}

/****************************************************************************
 SortPolygonsInTriListMesh

 On entry   :
 On exit    :
 */
static RpMesh *SortPolygonsInTriListMesh(RpMesh *mesh,
                                         RpMeshHeader *meshHeader __RWUNUSED__,
                                         void *pData __RWUNUSED__) {
    /* Retail uses divwu for the same unsigned / 3 and otherwise matches. */
    RwUInt32 *vertexTagBuffer;
    RwUInt32 maxVertex, i, numTriangles;
    RxVertexIndex *indices;

    RWFUNCTION(RWSTRING("SortPolygonsInTriListMesh"));
    RWASSERT(mesh);

    indices = mesh->indices;
    numTriangles = mesh->numIndices / 3;

    /* Find the maximum vertex in a fairly rough and ready way */
    maxVertex = 0;
    for (i = 0; i < mesh->numIndices; i++) {
        maxVertex |= indices[i];
    }

    /* Allocate a vertex tag table (8 vertices per byte) round to next long */
    vertexTagBuffer = (RwUInt32 *)RwMalloc(
        (maxVertex + 31) >> 3, rwMEMHINTDUR_FUNCTION | rwID_MESHMODULE);
    if (vertexTagBuffer) {
        /* Cool, now we can generate a new array of triangles,
         * copy the old ones to work from */
        RxVertexIndex *oldIndices;
        RwUInt32 arraySize =

            sizeof(RxVertexIndex) * mesh->numIndices;

        oldIndices = (RxVertexIndex *)RwMalloc(
            arraySize, rwMEMHINTDUR_FUNCTION | rwID_MESHMODULE);
        if (oldIndices) {
            RwUInt32 outIndex;

            memcpy(oldIndices, indices, arraySize);

            /* Copy runs of indices from the 'oldIndices' buffer back into
             * the 'indices' buffer, marking oldIndices as used by setting
             * all three vertex indices to 65535.
             */

            /* While we haven't copied all the triangles */
            outIndex = 0;
            while (outIndex < numTriangles) {
                RwUInt32 inIndex;

                /* Reset the vertex tag buffer, round to next long */
                private_memset(vertexTagBuffer, 0, ((maxVertex + 31) >> 3));

                inIndex = 0;
                while (inIndex < numTriangles) {
                    RxVertexIndex *testInds =

                        &oldIndices[inIndex * 3];

                    if ((testInds[0] & testInds[1] & testInds[2]) != 65535) {
                        /* This triangle hasn't been grabbed yet */
                        /* If none of the vertices are used,
                         * copy the triangles */
                        if ((!TESTVERTEXBIT(testInds[0])) &&
                            (!TESTVERTEXBIT(testInds[1])) &&
                            (!TESTVERTEXBIT(testInds[2]))) {
                            /* Setbits */
                            SETVERTEXBIT(testInds[0]);
                            SETVERTEXBIT(testInds[1]);
                            SETVERTEXBIT(testInds[2]);

                            /* Grabbit and markit */
                            *indices++ = testInds[0];
                            *indices++ = testInds[1];
                            *indices++ = testInds[2];
                            testInds[0] = testInds[1] = testInds[2] = 65535;

                            outIndex++;
                        }
                    }
                    inIndex++;
                }

                /* We've reached the end of the line, time to start a new scan
                 * if we don't have all the polys yet
                 */
            }

            /* Right, all done */
            RwFree(oldIndices);
        }

        /* Failed to allocate a triangle buffer
         *  - not much we can do with this mesh */
        RwFree(vertexTagBuffer);

        RWRETURN(mesh);
    }

    /* Failed to allocate a vertex tag table */
    RWRETURN(mesh);
}

/****************************************************************************
 TriStripAddEdge

 On entry:
 On exit:
 */
static Edge *TriStripAddEdge(RwFreeList *edgeFreeList, Edge **edgelist,
                             RwUInt16 v1, RwUInt16 v2, RwUInt32 tri,
                             TriBinEntry **binEntryArray) {
    Edge *temp = *edgelist;
    Edge *newEdge;

    RWFUNCTION(RWSTRING("TriStripAddEdge"));

    while (temp) {
        if (v1 == temp->v2 && v2 == temp->v1) {
            /* Can't have edge adjacency more than 2 */
            if (!temp->tri2) {
                /* Then adjcount must be equal to 1 */
                /* Up the adjacency on existing triangle on this edge */
                temp->tri1->adjCount++;

                /* Adjacency of triangle we are adding edge from goes up too */
                binEntryArray[tri]->adjCount++;
                temp->tri2 = binEntryArray[tri];
                RWRETURN(temp);
            }
        }
        temp = temp->next;
    }

    /* not in list so malloc a new edge */
    newEdge = (Edge *)RwFreeListAlloc(edgeFreeList,
                                      rwMEMHINTDUR_EVENT | rwID_MESHMODULE);
    newEdge->v1 = v1;
    newEdge->v2 = v2;
    newEdge->tri1 = binEntryArray[tri];
    newEdge->tri2 = (TriBinEntry *)NULL;
    newEdge->next = *edgelist;

    *edgelist = newEdge;

    RWRETURN(newEdge);
}

static TriBinEntry **
TriStripBinEntryArrayDestroy(RwUInt32 numTris, MeshOpFreeLists *meshOpFreeLists,
                             Edge *edge, TriBinEntry **binEntryArray) {
    RwUInt32 i;

    RWFUNCTION(RWSTRING("TriStripBinEntryArrayDestroy"));

    /*
     * Release meshOpFreeLists->edgeFreeList
     */
    while (edge) {
        Edge *next = edge->next;

        RwFreeListFree(meshOpFreeLists->edgeFreeList, edge);
        edge = (Edge *)NULL;

        edge = next;
    }
    RwFreeListDestroy(meshOpFreeLists->edgeFreeList);
    meshOpFreeLists->edgeFreeList = (RwFreeList *)NULL;

    /*
     * Release meshOpFreeLists->binEntryFreeList
     */
    for (i = 0; i < numTris; i++) {
        /* free memory from meshOpFreeLists->binEntryFreeList */
        RwFreeListFree(meshOpFreeLists->binEntryFreeList, binEntryArray[i]);
        binEntryArray[i] = (TriBinEntry *)NULL;
    }
    RwFreeListDestroy(meshOpFreeLists->binEntryFreeList);
    meshOpFreeLists->binEntryFreeList = (RwFreeList *)NULL;

    /*
     * Release binEntryArray
     */
    RwFree(binEntryArray);
    binEntryArray = (TriBinEntry **)NULL;

    RWRETURN(binEntryArray);
}

/****************************************************************************
 TriStripBinEntryArrayCreate

 On entry:
 On exit:
 */
static TriBinEntry **
TriStripBinEntryArrayCreate(RwUInt32 numTris, MeshOpFreeLists *meshOpFreeLists,
                            Edge **edgelist, RpBuildMeshTriangle *triList) {
    TriBinEntry **binEntryArray = (TriBinEntry **)NULL;
    RwUInt32 i;

    RWFUNCTION(RWSTRING("TriStripBinEntryArrayCreate"));

    binEntryArray = (TriBinEntry **)RwMalloc(
        sizeof(TriBinEntry *) * numTris, rwMEMHINTDUR_EVENT | rwID_MESHMODULE);
    meshOpFreeLists->binEntryFreeList =
        RwFreeListCreate(sizeof(TriBinEntry), numTris, sizeof(RwUInt32),
                         rwMEMHINTDUR_EVENT | rwID_MESHMODULE);
    meshOpFreeLists->edgeFreeList =
        RwFreeListCreate(sizeof(Edge), (numTris / 2) + 1, sizeof(RwUInt32),
                         rwMEMHINTDUR_EVENT | rwID_MESHMODULE);

    for (i = 0; i < numTris; i++) {
        binEntryArray[i] = (TriBinEntry *)RwFreeListAlloc(
            meshOpFreeLists->binEntryFreeList,
            rwMEMHINTDUR_EVENT | rwID_MESHMODULE);
        binEntryArray[i]->adjCount = 0;
        binEntryArray[i]->tri = i;
        binEntryArray[i]->prev = (TriBinEntry *)NULL;
        binEntryArray[i]->next = (TriBinEntry *)NULL;
        binEntryArray[i]->used = FALSE;
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

    RWRETURN(binEntryArray);
}

/****************************************************************************
 TriStripGetTriEdge

 On entry:
 On exit:
 */
static Edge *TriStripGetTriEdge(TriBinEntry *binEntry, RwUInt16 v1,
                                RwUInt16 v2 /*, RwInt32 currentAttempt */) {
    RWFUNCTION(RWSTRING("TriStripGetTriEdge"));
    RWASSERT(binEntry);

    if ((binEntry->edge[0]->v1 == v1 && binEntry->edge[0]->v2 == v2) ||
        (binEntry->edge[0]->v1 == v2 && binEntry->edge[0]->v2 == v1)) {
        RWRETURN(binEntry->edge[0]);
    }
    if ((binEntry->edge[1]->v1 == v1 && binEntry->edge[1]->v2 == v2) ||
        (binEntry->edge[1]->v1 == v2 && binEntry->edge[1]->v2 == v1)) {
        RWRETURN(binEntry->edge[1]);
    }
    if ((binEntry->edge[2]->v1 == v1 && binEntry->edge[2]->v2 == v2) ||
        (binEntry->edge[2]->v1 == v2 && binEntry->edge[2]->v2 == v1)) {
        RWRETURN(binEntry->edge[2]);
    }

    RWRETURN((Edge *)NULL);
}

/****************************************************************************
 TriStripMarkTriUsed

 On entry:
 On exit:
 */
static void TriStripMarkTriUsed(TriBinEntry *tri, TriBinList *binListArray,
                                /* TriBinEntry **binEntryArray, */
                                RwInt32 currentAttempt) {
    /* Stock bin updates are exact; retail omits O0 parameter-home state. */
    RwUInt8 i;
    TriBinEntry *newTri;

    RWFUNCTION(RWSTRING("TriStripMarkTriUsed"));
    RWASSERT(tri);

    if (currentAttempt < 4) {
        /* mark used */
        tri->used2 = TRUE;

        /* no need to touch the bin lists or triangle adjacency counts
           in the temporary stripping tests */
    } else {
        /* mark used */
        tri->used = TRUE;

        /* so remove it from the bin lists */
        if (binListArray[tri->adjCount].head == tri) {
            /* remove from head of a list */
            binListArray[tri->adjCount].head =
                binListArray[tri->adjCount].head->next;
            if (binListArray[tri->adjCount].head) {
                binListArray[tri->adjCount].head->prev = (TriBinEntry *)NULL;
            }
        } else {
            /* remove it from the list */
            if (tri->next) {
                tri->next->prev = tri->prev;
            }
            if (tri->prev) {
                tri->prev->next = tri->next;
            }
        }

        for (i = 0; i < 3; i++) {
            newTri = FALSE;
            /* for each of the three edges find the other adjacent
               triangle if present and update it's adjacency counts
               and binList membership */
            if (tri->edge[i]->tri1 && tri->edge[i]->tri1 != tri &&
                tri->edge[i]->tri1->used == FALSE) {
                newTri = tri->edge[i]->tri1;
            } else if (tri->edge[i]->tri2 &&
                       tri->edge[i]->tri2->used == FALSE) {
                newTri = tri->edge[i]->tri2;
            }

            if (newTri) {
                if (binListArray[newTri->adjCount].head == newTri) {
                    /* remove from head of a list */
                    binListArray[newTri->adjCount].head =
                        binListArray[newTri->adjCount].head->next;
                    if (binListArray[newTri->adjCount].head) {
                        binListArray[newTri->adjCount].head->prev =
                            (TriBinEntry *)NULL;
                    }
                } else {
                    /* remove it from the list */
                    if (newTri->next) {
                        newTri->next->prev = newTri->prev;
                    }
                    if (newTri->prev) {
                        newTri->prev->next = newTri->next;
                    }
                }

                newTri->adjCount--;

                /* reinsert into the list */
                newTri->next = binListArray[newTri->adjCount].head;
                if (newTri->next) {
                    newTri->next->prev = newTri;
                }
                binListArray[newTri->adjCount].head = newTri;
                newTri->prev = (TriBinEntry *)NULL;
            }
        }
    }

    RWRETURNVOID();
}

#ifdef TRISTRIPLOOKAHEAD
/****************************************************************************
 TriStripIsLastTriInStrip

 On entry:
 On exit:
 */
static RwBool TriStripIsLastTriInStrip(Edge *outgoingEdge,
                                       TriBinEntry *currentTri,
                                       TriBinEntry **binEntryArray,
                                       RwInt32 currentAttempt) {
    RwInt32 i;
    TriBinEntry *nextTri = (TriBinEntry *)NULL;

    RWFUNCTION(RWSTRING("TriStripIsLastTriInStrip"));

    if (currentAttempt < 4) {
        if (outgoingEdge->tri1 && outgoingEdge->tri1 != currentTri &&
            !outgoingEdge->tri1->used2) {
            nextTri = outgoingEdge->tri1;
        } else if (outgoingEdge->tri2 && outgoingEdge->tri2 != currentTri &&
                   !outgoingEdge->tri2->used2) {
            nextTri = outgoingEdge->tri2;
        }

        if (!nextTri) {
            RWRETURN(TRUE);
        }

        for (i = 0; i < 3; i++) {
            if (nextTri->edge[i] != outgoingEdge && nextTri->edge[i] &&
                EdgeAdjCount2(nextTri->edge[i]) > 1) {
                RWRETURN(FALSE);
            }
        }
    } else {
        if (outgoingEdge->tri1 && outgoingEdge->tri1 != currentTri &&
            !outgoingEdge->tri1->used) {
            nextTri = outgoingEdge->tri1;
        } else if (outgoingEdge->tri2 && outgoingEdge->tri2 != currentTri &&
                   !outgoingEdge->tri2->used) {
            nextTri = outgoingEdge->tri2;
        }

        if (!nextTri) {
            RWRETURN(TRUE);
        }

        for (i = 0; i < 3; i++) {
            if (nextTri->edge[i] != outgoingEdge && nextTri->edge[i] &&
                EdgeAdjCount(nextTri->edge[i]) > 1) {
                RWRETURN(FALSE);
            }
        }
    }

    RWRETURN(TRUE);
}
#endif /* TRISTRIPLOOKAHEAD */

/****************************************************************************
 TriStripFollow

 On entry:
 On exit:
 */
static RwUInt32 TriStripFollow(TriStripListEntry *strip, Edge *nextEdge,
                               TriBinList *binListArray,
                               RpBuildMeshTriangle *triList __RWUNUSED__,
                               /* TriBinEntry **binEntryArray, */
                               /* Edge *edgelist, */
                               RwInt32 currentAttempt) {
    /* Stock traversal and winding decisions are exact; debug lifetimes differ.
     */
    RwUInt32 addedTris = 0;
    Edge *prevEdge = (Edge *)NULL;
    Edge *otherEdge = (Edge *)NULL;
    RwBool nextIsLast = FALSE;
    RwBool otherIsLast = FALSE;
    RwBool turnResult;

    RWFUNCTION(RWSTRING("TriStripFollow"));

    while (nextEdge) {
        TriBinEntry *bestTri = (TriBinEntry *)NULL;
        RwUInt16 v1;
        RwUInt16 v2;
        RwUInt16 v3 = 0;
        RwInt32 nextEdgeIndex = -1; /* index of nextEdge in bestTri */

        /* Order the vertices to get winding order right */
        if (strip->stripLen % 2) {
            v1 = strip->strip[strip->stripLen - 1];
            v2 = strip->strip[strip->stripLen - 2];
        } else {
            v1 = strip->strip[strip->stripLen - 2];
            v2 = strip->strip[strip->stripLen - 1];
        }

        /* Find next triangle from edge (go for high adjacency) */
        if (currentAttempt < 4) {
            if (nextEdge->tri1 && nextEdge->tri1->used2 == FALSE) {
                bestTri = nextEdge->tri1;
            } else if (nextEdge->tri2 && nextEdge->tri2->used2 == FALSE) {
                bestTri = nextEdge->tri2;
            }
        } else {
            if (nextEdge->tri1 && nextEdge->tri1->used == FALSE) {
                bestTri = nextEdge->tri1;
            } else if (nextEdge->tri2 && nextEdge->tri2->used == FALSE) {
                bestTri = nextEdge->tri2;
            }
        }

        if (!bestTri) {
            RWRETURN(addedTris);
        }

        addedTris++;

        /* sort out the adjacency counts */
        TriStripMarkTriUsed(bestTri, binListArray,
                            /* binEntryArray, */ currentAttempt);

        /* work out the 3rd vertex */
        if (nextEdge == bestTri->edge[0]) {
            nextEdgeIndex = 0;
        } else if (nextEdge == bestTri->edge[1]) {
            nextEdgeIndex = 1;
        } else if (nextEdge == bestTri->edge[2]) {
            nextEdgeIndex = 2;
        }

        RWASSERT(nextEdgeIndex != -1);

        /* now find the 3rd vertex from the next edge around the triangle */
        if (bestTri->edge[(nextEdgeIndex + 1) % 3]->tri1 ==
            bestTri) /* edge direction */
        {
            v3 = bestTri->edge[(nextEdgeIndex + 1) % 3]->v2;
        } else {
            v3 = bestTri->edge[(nextEdgeIndex + 1) % 3]->v1;
        }

        prevEdge = nextEdge;
        nextEdge =
            TriStripGetTriEdge(bestTri, strip->strip[strip->stripLen - 1],
                               v3 /* ,  currentAttempt */);
#ifdef TRISTRIPLOOKAHEAD
        nextIsLast = TriStripIsLastTriInStrip(nextEdge, bestTri, binEntryArray,
                                              currentAttempt);
#endif /* TRISTRIPLOOKAHEAD */

#ifdef TURNCORNERS
        if (currentAttempt < 4) {
            turnResult =
                !nextEdge || (EdgeAdjCount2(nextEdge) == 0) || nextIsLast;
        } else {
            turnResult =
                !nextEdge || (EdgeAdjCount(nextEdge) == 0) || nextIsLast;
        }
#else  /* TURNCORNERS */
        turnResult = FALSE;
#endif /* TURNCORNERS */
        if (turnResult) {
            /* find the 3rd edge */
            if (bestTri->edge[0] != prevEdge && bestTri->edge[0] != nextEdge) {
                otherEdge = bestTri->edge[0];
            } else if (bestTri->edge[1] != prevEdge &&
                       bestTri->edge[1] != nextEdge) {
                otherEdge = bestTri->edge[1];
            } else if (bestTri->edge[2] != prevEdge &&
                       bestTri->edge[2] != nextEdge) {
                otherEdge = bestTri->edge[2];
            }

#ifdef TRISTRIPLOOKAHEAD
            otherIsLast = TriStripIsLastTriInStrip(
                otherEdge, bestTri, binEntryArray, currentAttempt);
#endif /* TRISTRIPLOOKAHEAD */

            if (currentAttempt < 4) {
                turnResult = (otherEdge && EdgeAdjCount2(otherEdge) &&
                              !(otherIsLast && nextIsLast));
            } else {
                turnResult = (otherEdge && EdgeAdjCount(otherEdge) &&
                              !(otherIsLast && nextIsLast));
            }
            if (turnResult) {
                /* other edge has adjacencies */
                if (strip->stripLen % 2) {
                    /* normal vert */
                    strip->strip[strip->stripLen] = v3;
                    strip->stripLen++;
                    /* swapper */
#ifdef ODDTURNS /* odd turns */
                    strip->strip[strip->stripLen] = v3;
                    strip->stripLen++;

                    /* swapper */
                    strip->strip[strip->stripLen] =
                        strip->strip[strip->stripLen - 4];
                    strip->stripLen++;
                    nextEdge = otherEdge;
#else  /* ODDTURNS */
                    nextEdge = (Edge *)NULL;
#endif /* ODDTURNS */
                } else {
                    /* swapper */
                    strip->strip[strip->stripLen] =
                        strip->strip[strip->stripLen - 2];
                    strip->stripLen++;
                    /* now output the appropriate vertex */
                    strip->strip[strip->stripLen] = v3;
                    strip->stripLen++;
                    nextEdge = otherEdge;
                }
            } else {
                /* just output the appropriate vertex */
                strip->strip[strip->stripLen] = v3;
                strip->stripLen++;
                nextEdge = (Edge *)NULL;
            }
        } else {
            /* just output the appropriate vertex */
            strip->strip[strip->stripLen] = v3;
            strip->stripLen++;
        }
    }

    RWRETURN(addedTris);
}

/****************************************************************************
 TriStripStripTris

 On entry:
 On exit:
 */
static RwBool TriStripStripTris(RpBuildMeshTriangle *triList, RwUInt32 numTris,
                                TriStripList *stripList, RwBool preprocess) {
    /* Canonical strip selection and ownership are exact; debug state differs.
     */
    /* build a face and edge list */
    Edge *edgelist = (Edge *)NULL;
    TriStripListEntry *newStrip, *buildStrip, *revBuildStrip;
    RwUInt32 i, j;
    RwUInt32 trisUsed = 0;
    RwUInt32 trisUsedTemp = 0;
    TriBinList binListArray[4];
    TriBinEntry **binEntryArray = (TriBinEntry **)NULL;
    Edge *nextEdge = (Edge *)NULL;
    Edge *firstEdge = (Edge *)NULL;
    RwUInt32 offset, cacheOffset;
    MeshOpFreeLists meshOpFreeLists;
    RwInt32 bestOffset, currentAttempt;
    RwUInt32 bestSize = 0;
    RwBool testResult;

    RWFUNCTION(RWSTRING("TriStripStripTris"));

    /* We build several lists based on triangle adjacency */
    binListArray[0].head = (TriBinEntry *)NULL;
    binListArray[1].head = (TriBinEntry *)NULL;
    binListArray[2].head = (TriBinEntry *)NULL;
    binListArray[3].head = (TriBinEntry *)NULL;

    meshOpFreeLists.binEntryFreeList = (RwFreeList *)NULL;
    meshOpFreeLists.edgeFreeList = (RwFreeList *)NULL;

    binEntryArray = TriStripBinEntryArrayCreate(numTris, &meshOpFreeLists,
                                                &edgelist, triList);

    /* Build the lists of similarly adjacent triangles */
    for (i = 0; i < numTris; i++) {
        binEntryArray[i]->next = binListArray[binEntryArray[i]->adjCount].head;

        if (binEntryArray[i]->next) {
            binEntryArray[i]->next->prev = binEntryArray[i];
        }
        binListArray[binEntryArray[i]->adjCount].head = binEntryArray[i];
        binEntryArray[i]->prev = (TriBinEntry *)NULL;
    }

    /* Allocate a buildstrip to avoid reallocs */
    buildStrip = (TriStripListEntry *)RwFreeListAlloc(
        RPMESHGLOBAL(triStripListEntryFreeList),
        rwMEMHINTDUR_FUNCTION | rwID_MESHMODULE);
    buildStrip->stripSize = (numTris * 2) + 2;
    buildStrip->stripLen = 0;
    buildStrip->strip =
        (RwUInt16 *)RwMalloc(sizeof(RwUInt32) * buildStrip->stripSize,
                             rwMEMHINTDUR_FUNCTION | rwID_MESHMODULE);

    /* And another for backwards stripping */
    revBuildStrip = (TriStripListEntry *)RwFreeListAlloc(
        RPMESHGLOBAL(triStripListEntryFreeList),
        rwMEMHINTDUR_FUNCTION | rwID_MESHMODULE);
    revBuildStrip->stripSize = (numTris * 2) + 2;
    revBuildStrip->stripLen = 0;
    revBuildStrip->strip =
        (RwUInt16 *)RwMalloc(sizeof(RwUInt32) * revBuildStrip->stripSize,
                             rwMEMHINTDUR_FUNCTION | rwID_MESHMODULE);

    while (trisUsed < numTris) {
        bestOffset = 0;
        bestSize = 0;

        /* Do we preview the strips? If we do then we try more times than normal
         */
        if (preprocess) {
            /* Try each edge (0,1 and 2) */
            currentAttempt = 0;
        } else {
            /* Just go ahead and generate the strips first time */
            currentAttempt = 3;
        }

        /* Find a starter */
        if (binListArray[0].head) {
            /* Pull the first triangle off the list */
            RwUInt32 tri = binListArray[0].head->tri;

            /* If we have any separated tri's make into a strip */
            newStrip = (TriStripListEntry *)RwFreeListAlloc(
                RPMESHGLOBAL(triStripListEntryFreeList),
                rwMEMHINTDUR_EVENT | rwID_MESHMODULE);
            newStrip->next = stripList->head;
            stripList->head = newStrip;

            newStrip->stripSize = 3;
            newStrip->stripLen = 3;
            newStrip->strip = (RwUInt16 *)RwMalloc(
                sizeof(RwUInt32) * 3, rwMEMHINTDUR_EVENT | rwID_MESHMODULE);
            newStrip->strip[0] = triList[tri].vertIndex[0];
            newStrip->strip[1] = triList[tri].vertIndex[1];
            newStrip->strip[2] = triList[tri].vertIndex[2];
            binListArray[0].head->used = TRUE;
            binListArray[0].head->used2 = TRUE;
            binListArray[0].head = binListArray[0].head->next;
            if (binListArray[0].head) {
                binListArray[0].head->prev = (TriBinEntry *)NULL;
            }
            trisUsed++;
        } else {
            RpBuildMeshTriangle *source;

            /* Pick start triangle from lowest adjacency list */
            i = 1;
            while (!binListArray[i].head) {
                i++;
            }

            /* Then trace an edge */
            if (EdgeAdjCount(binListArray[i].head->edge[2]) >= 2 &&
                EdgeAdjCount(binListArray[i].head->edge[1]) >= 2) {
                offset = 1;
            } else if (EdgeAdjCount(binListArray[i].head->edge[0]) >= 2 &&
                       EdgeAdjCount(binListArray[i].head->edge[2]) >= 2) {
                offset = 2;
            } else if (EdgeAdjCount(binListArray[i].head->edge[1]) >= 2 &&
                       EdgeAdjCount(binListArray[i].head->edge[0]) >= 2) {
                offset = 0;
            } else {
                if (EdgeAdjCount(binListArray[i].head->edge[0]) >
                    EdgeAdjCount(binListArray[i].head->edge[1])) {
                    if (EdgeAdjCount(binListArray[i].head->edge[0]) >
                        EdgeAdjCount(binListArray[i].head->edge[2])) {
                        offset = 2;
                    } else {
                        offset = 1;
                    }
                } else {
                    if (EdgeAdjCount(binListArray[i].head->edge[1]) >
                        EdgeAdjCount(binListArray[i].head->edge[2])) {
                        offset = 0;
                    } else {
                        offset = 1;
                    }
                }
            }

            /* Cache the tris used */
            trisUsedTemp = trisUsed;
            cacheOffset = offset;
            bestOffset = offset;

            /* Retry each possible starting edge and retain the best strip. */
            do {

                /* restore the number of triangles currently used */
                trisUsed = trisUsedTemp;

                /* reset all adjCount2 vars to be the same as adjCount vars. The
                 * adjCount2 vars are temporary - we use them to test the
                 * stripping from each edge */
                for (j = 0; j < numTris; j++) {
                    binEntryArray[j]->used2 = binEntryArray[j]->used;
                }

                /* This is going to be attempted 3 times and we're going to pick
                 * the best attempt */
                switch (currentAttempt++) {
                case 0:
                    /* Edge order remains the same */
                    offset = (cacheOffset + 0) % 3;
                    break;
                case 1:
                    /* Try a different edge order */
                    offset = (cacheOffset + 1) % 3;
                    break;
                case 2:
                    /* Try the final edge order */
                    offset = (cacheOffset + 2) % 3;
                    break;
                default:
                    /* This is the final (and catch-all) case that we use */
                    offset = bestOffset;
                    break;
                }

                /* Now do it */
                switch (offset) {
                case 0:
                    nextEdge = binListArray[i].head->edge[1];  /* 1->2 */
                    firstEdge = binListArray[i].head->edge[0]; /* 0->1 */
                    break;
                case 1:
                    nextEdge = binListArray[i].head->edge[2];  /* 2->0 */
                    firstEdge = binListArray[i].head->edge[1]; /* 1->2 */
                    break;
                case 2:
                    nextEdge = binListArray[i].head->edge[0];  /* 0->1 */
                    firstEdge = binListArray[i].head->edge[2]; /* 2->0 */
                    break;
                }

                source = &triList[binListArray[i].head->tri];

                buildStrip->strip[0] = source->vertIndex[(0 + offset) % 3];
                buildStrip->strip[1] = source->vertIndex[(1 + offset) % 3];
                buildStrip->strip[2] = source->vertIndex[(2 + offset) % 3];
                buildStrip->stripLen = 3;

                TriStripMarkTriUsed(binListArray[i].head, binListArray,
                                    /* binEntryArray, */ currentAttempt);
                trisUsed++;

                /* Follow along a strip from it */
                trisUsed +=
                    TriStripFollow(buildStrip, nextEdge, binListArray, triList,
                                   /* binEntryArray, edgelist, */
                                   currentAttempt);

                /* If we're trying to build the best strip then use the "temp"
                 * vars */
                if (currentAttempt < 4) {
                    testResult = (EdgeAdjCount2(firstEdge) > 0);
                } else {
                    testResult = (EdgeAdjCount(firstEdge) > 0);
                }
                if (testResult) {
                    /* Now work back from the start */
                    revBuildStrip->strip[0] = buildStrip->strip[1];
                    revBuildStrip->strip[1] = buildStrip->strip[0];
                    revBuildStrip->stripLen = 2;
                    trisUsed += TriStripFollow(revBuildStrip, firstEdge,
                                               binListArray, triList,
                                               /* binEntryArray, edgelist, */
                                               currentAttempt);

                    if (revBuildStrip->stripLen % 2) {
                        revBuildStrip->strip[revBuildStrip->stripLen] =
                            revBuildStrip->strip[revBuildStrip->stripLen - 2];
                        revBuildStrip->stripLen++;
                    }

                    /* Check the length, record if necessary,
                     * reset trisUsed and jump back */
                    if (trisUsed > bestSize) {
                        bestSize = trisUsed;
                        bestOffset = offset;
                    }
                    if (currentAttempt < 4) {
                        /* we're still trying to get the best strip length,
                         * so jump back */
                        continue;
                    }

                    /* Finally copy it and add to the list */
                    newStrip = (TriStripListEntry *)RwFreeListAlloc(
                        RPMESHGLOBAL(triStripListEntryFreeList),
                        rwMEMHINTDUR_EVENT | rwID_MESHMODULE);
                    newStrip->next = stripList->head;
                    stripList->head = newStrip;

                    newStrip->stripSize =
                        buildStrip->stripLen + revBuildStrip->stripLen - 2;
                    newStrip->stripLen = 0;
                    newStrip->strip = (RwUInt16 *)RwMalloc(
                        sizeof(RwUInt32) * newStrip->stripSize,
                        rwMEMHINTDUR_EVENT | rwID_MESHMODULE);
                    while (revBuildStrip->stripLen > 2) {
                        newStrip->strip[newStrip->stripLen] =
                            revBuildStrip->strip[revBuildStrip->stripLen - 1];
                        newStrip->stripLen++;
                        revBuildStrip->stripLen--;
                    }
                    memcpy(&newStrip->strip[newStrip->stripLen],
                           buildStrip->strip,
                           sizeof(RwUInt32) * buildStrip->stripLen);
                    newStrip->stripLen = newStrip->stripSize;
                } else {
                    /* Check the length, record if necessary, reset trisUsed and
                     * jump back
                     */
                    if (trisUsed > bestSize) {
                        bestSize = trisUsed;
                        bestOffset = offset;
                    }
                    if (currentAttempt < 4) {
                        /* we're still trying to get the best strip length,
                         * so jump back */
                        continue;
                    }

                    /* Finally copy it and add to the list */
                    newStrip = (TriStripListEntry *)RwFreeListAlloc(
                        RPMESHGLOBAL(triStripListEntryFreeList),
                        rwMEMHINTDUR_EVENT | rwID_MESHMODULE);
                    newStrip->next = stripList->head;
                    stripList->head = newStrip;

                    newStrip->stripSize = buildStrip->stripLen;
                    newStrip->stripLen = buildStrip->stripLen;
                    newStrip->strip = (RwUInt16 *)RwMalloc(
                        sizeof(RwUInt32) * buildStrip->stripLen,
                        rwMEMHINTDUR_EVENT | rwID_MESHMODULE);
                    memcpy(newStrip->strip, buildStrip->strip,
                           sizeof(RwUInt32) * buildStrip->stripLen);
                }
            } while (currentAttempt < 4);
        }
    }

    RwFree(revBuildStrip->strip);
    revBuildStrip->strip = (RwUInt16 *)NULL;
    RwFreeListFree(RPMESHGLOBAL(triStripListEntryFreeList), revBuildStrip);
    revBuildStrip = (TriStripListEntry *)NULL;

    RwFree(buildStrip->strip);
    buildStrip->strip = (RwUInt16 *)NULL;
    RwFreeListFree(RPMESHGLOBAL(triStripListEntryFreeList), buildStrip);
    buildStrip = (TriStripListEntry *)NULL;

    binEntryArray = TriStripBinEntryArrayDestroy(numTris, &meshOpFreeLists,
                                                 edgelist, binEntryArray);

    RWRETURN(TRUE);
}

#ifdef JOINSTRIPS
/****************************************************************************
 TriStripJoin

 On entry:
 On exit:
 */
static RwBool TriStripJoin(TriStripList *stripList, RwBool maintainWinding) {
    RwUInt32 i, j;
    TriStripListEntry *newStrip = (TriStripListEntry *)NULL;
    TriStripListEntry *stripPtr;
    TriStripListEntry *next;

    RWFUNCTION(RWSTRING("TriStripJoin"));
    RWASSERT(stripList);

    if (!stripList->head) {
        RWRETURN(FALSE);
    }

    newStrip = (TriStripListEntry *)RwFreeListAlloc(
        RPMESHGLOBAL(triStripListEntryFreeList),
        rwMEMHINTDUR_EVENT | rwID_MESHMODULE);
    newStrip->stripLen = 0;
    newStrip->stripSize = 0;

    /* calc a max new strip length */
    stripPtr = stripList->head;
    while (stripPtr) {
        /* sum the strips + max 6 joining indices */
        newStrip->stripSize += stripPtr->stripLen + 6;
        stripPtr = stripPtr->next;
    }
    newStrip->strip =
        (RwUInt16 *)RwMalloc(sizeof(RwUInt32) * newStrip->stripSize,
                             rwMEMHINTDUR_EVENT | rwID_MESHMODULE);

    stripPtr = stripList->head;
    for (i = 0; i < stripPtr->stripLen; i++) {
        newStrip->strip[newStrip->stripLen] = stripPtr->strip[i];
        newStrip->stripLen++;
    }

    RwFree(stripPtr->strip);
    stripPtr->strip = (RwUInt16 *)NULL;

    next = stripPtr->next;
    RwFreeListFree(RPMESHGLOBAL(triStripListEntryFreeList), stripPtr);
    stripPtr = (TriStripListEntry *)NULL;
    stripPtr = next;

    /* this algorithm allows us to ignore the winding order
       of triangles in tristrips resulting in less degenerate
       triangles when joining strips. Winding can only be ignored where
       backface culling is disabled. The algorithm also searches for and
       joins strips which start with vertices used at the end of
       the previous strip. */
    while (stripPtr) {
        TriStripListEntry *tempStrip = stripPtr;

        /* can we find a strip starting with the vertex the last one
           ended with ? */
        while (tempStrip) {
            if (newStrip->strip[newStrip->stripLen - 1] ==
                tempStrip->strip[0]) {
                if ((!(newStrip->stripLen % 2)) || (!maintainWinding)) {
                    /* no extra verts in this case */
                    newStrip->stripLen += 0;
                } else {
                    newStrip->strip[newStrip->stripLen] = tempStrip->strip[0];
                    newStrip->stripLen += 1;
                }
                break;
            }
            tempStrip = tempStrip->next;
        }

        if (!tempStrip) {
            /* if we didn't find a strip can we find a strip starting with
               the last but one vertex the last strip ended with ? */
            tempStrip = stripPtr;

            while (tempStrip) {
                if (newStrip->strip[newStrip->stripLen - 2] ==
                    tempStrip->strip[0]) {
                    if ((!(newStrip->stripLen % 2)) && maintainWinding) {
                        newStrip->strip[newStrip->stripLen] =
                            tempStrip->strip[0];
                        newStrip->strip[newStrip->stripLen + 1] =
                            tempStrip->strip[0];
                        newStrip->stripLen += 2;
                    } else {
                        newStrip->strip[newStrip->stripLen] =
                            tempStrip->strip[0];
                        newStrip->stripLen += 1;
                    }
                    break;
                }
                tempStrip = tempStrip->next;
            }
        }

        if (!tempStrip) {
            /* if we haven't found a strip yet use the next one in the
               list */
            tempStrip = stripPtr;

            if ((!(newStrip->stripLen % 2)) || (!maintainWinding)) {
                newStrip->strip[newStrip->stripLen] =
                    newStrip->strip[newStrip->stripLen - 1];
                newStrip->strip[newStrip->stripLen + 1] = tempStrip->strip[0];
                newStrip->stripLen += 2;
            } else {
                newStrip->strip[newStrip->stripLen] =
                    newStrip->strip[newStrip->stripLen - 1];
                newStrip->strip[newStrip->stripLen + 1] = tempStrip->strip[0];
                newStrip->strip[newStrip->stripLen + 2] = tempStrip->strip[0];
                newStrip->stripLen += 3;
            }
        }

        /* copy over the verts in the strip */
        for (j = 0; j < tempStrip->stripLen; j++) {
            newStrip->strip[newStrip->stripLen] = tempStrip->strip[j];
            newStrip->stripLen++;
        }
        RwFree(tempStrip->strip);
        tempStrip->strip = (RwUInt16 *)NULL;

        if (stripPtr == tempStrip) {
            stripPtr = stripPtr->next;
            RwFreeListFree(RPMESHGLOBAL(triStripListEntryFreeList), tempStrip);
        } else {
            TriStripListEntry *tempStrip2 = stripPtr;

            while (tempStrip2->next != tempStrip) {
                tempStrip2 = tempStrip2->next;
            }
            tempStrip2->next = tempStrip->next;
            RwFreeListFree(RPMESHGLOBAL(triStripListEntryFreeList), tempStrip);
        }
    }

    stripList->head = newStrip;
    newStrip->next = (TriStripListEntry *)NULL;

    RWRETURN(TRUE);
}
#endif /* JOINSTRIPS */

/****************************************************************************
 TriStripMeshGenerate

 On entry:
 On exit:
 */
static RpMeshHeader *TriStripMeshGenerate(RpBuildMesh *mesh, RwBool preprocess,
                                          RwBool maintainWinding) {
    /* Canonical grouping, joining, and allocation ownership are exact. */
    RpMeshHeader *result;
    RpBuildMeshTriangle **triPointers;
    RpMesh *meshEl;
    RwUInt32 i;
    RwUInt32 j;
    RwUInt32 numMats;
    RwUInt32 meshSize;
    RwUInt32 triPointerIndex;
    RpBuildMeshTriangle **tempTriPtr;
    RpBuildMeshTriangle *triList;
    TriStripList stripList;
    TriStripListEntry *stripPtr;
    RwUInt16 numOutMeshes = 0;
    RwUInt16 numAllocatedOutMeshes = 0;
    RxVertexIndex *stripMeshInds;
    RpMesh **outMeshes = (RpMesh **)NULL;
    RpMesh *outMeshInfo = (RpMesh *)NULL;
    RwUInt32 totalIndices;

    RWFUNCTION(RWSTRING("TriStripMeshGenerate"));
    RWASSERT(mesh);

    triPointers = (RpBuildMeshTriangle **)RwMalloc(
        mesh->numTriangles * sizeof(RpBuildMeshTriangle *),
        rwMEMHINTDUR_FUNCTION | rwID_MESHMODULE);

    if (!triPointers) {
        RWRETURN((RpMeshHeader *)NULL);
    }

    /* Fill in pointers so that we can sort */
    for (i = 0; i < mesh->numTriangles; i++) {
        triPointers[i] = &(mesh->meshTriangles[i]);
    }

    /* Now sort 'em */
    qsort(triPointers, mesh->numTriangles, sizeof(RpBuildMeshTriangle *),
          SortPolygons);

    /* Figure out how many materials there are */
    numMats = 1;
    if (mesh->numTriangles >= 2) {
        RpMaterial *lastMat = triPointers[0]->material;

        for (i = 1; i < mesh->numTriangles; i++) {
            if (triPointers[i]->material != lastMat) {
                /* We found another material */
                lastMat = triPointers[i]->material;
                numMats++;
            }
        }
    }

    /* Allocate an outMeshes array */
    outMeshes = (RpMesh **)RwMalloc(sizeof(RpMesh *) * numMats,
                                    rwMEMHINTDUR_FUNCTION | rwID_MESHMODULE |
                                        rwMEMHINTFLAG_RESIZABLE);
    outMeshInfo = (RpMesh *)RwMalloc(sizeof(RpMesh) * numMats,
                                     rwMEMHINTDUR_FUNCTION | rwID_MESHMODULE);
    numAllocatedOutMeshes = (RwUInt16)numMats;

    /* Fill in the materials and use numIndices
     * to indicate triangle run length */
    numMats = 1;
    outMeshInfo[0].material = triPointers[0]->material;
    outMeshInfo[0].numIndices = 0;
    outMeshInfo[0].indices = (RxVertexIndex *)NULL;
    if (mesh->numTriangles >= 2) {
        for (i = 0; i < mesh->numTriangles - 1; i++) {
            if (triPointers[i]->material != triPointers[i + 1]->material) {
                outMeshInfo[numMats].material = triPointers[i + 1]->material;
                outMeshInfo[numMats].numIndices = (i + 1);
                outMeshInfo[numMats].indices = (RxVertexIndex *)NULL;
                outMeshInfo[numMats - 1].numIndices =
                    (i + 1) - outMeshInfo[numMats - 1].numIndices;
                numMats++;
            }
        }
    }
    outMeshInfo[numMats - 1].numIndices =
        mesh->numTriangles - outMeshInfo[numMats - 1].numIndices;

    /* Now lets strip them */
    RPMESHGLOBAL(triStripListEntryFreeList) = RwFreeListCreate(
        sizeof(TriStripListEntry), (mesh->numTriangles / 10) + 5,
        sizeof(RwUInt32), rwMEMHINTDUR_FUNCTION | rwID_MESHMODULE);

    triPointerIndex = 0;
    tempTriPtr = triPointers;
    stripList.head = (TriStripListEntry *)NULL;

    for (i = 0; i < numMats; i++) {
        /* build a tri list for this mesh */
        triList = (RpBuildMeshTriangle *)RwMalloc(
            sizeof(RpBuildMeshTriangle) * outMeshInfo[i].numIndices,
            rwMEMHINTDUR_FUNCTION | rwID_MESHMODULE);
        for (j = 0; j < outMeshInfo[i].numIndices; j++) {
            triList[j] = *tempTriPtr[0];
            tempTriPtr++;
        }

        /* Build a set of strips */
        TriStripStripTris(triList, outMeshInfo[i].numIndices, &stripList,
                          preprocess);

#ifdef JOINSTRIPS
        /* Join the strips with degenerate triangles */
        TriStripJoin(&stripList, maintainWinding);
#endif

        stripPtr = stripList.head;
        while (stripPtr) {
            RwUInt32 bytes = sizeof(RpMesh) +

                             (sizeof(RxVertexIndex) * stripPtr->stripLen);

            /* build this into a mesh (this allocated structure is aligned) */
            meshEl = (RpMesh *)RwMalloc(bytes, rwMEMHINTDUR_FUNCTION |
                                                   rwID_MESHMODULE);
            meshEl->material = outMeshInfo[i].material;
            meshEl->numIndices = stripPtr->stripLen;
            meshEl->indices = (RxVertexIndex *)(meshEl + 1);

            for (j = 0; j < meshEl->numIndices; j++) {
                meshEl->indices[j] = (RxVertexIndex)stripPtr->strip[j];
            }
#ifndef JOINSTRIPS
            if (numOutMeshes == numAllocatedOutMeshes) {
                outMeshes =
                    RwRealloc(outMeshes, sizeof(RpMesh *) * (numOutMeshes * 2),
                              rwMEMHINTDUR_EVENT | rwID_MESHMODULE |
                                  rwMEMHINTFLAG_RESIZABLE);
                numAllocatedOutMeshes = numOutMeshes * 2;
            }
#endif /* JOINSTRIPS */
            outMeshes[numOutMeshes] = meshEl;
            numOutMeshes++;

            stripPtr = stripPtr->next;
        }

        while (NULL != stripList.head) {
            stripPtr = stripList.head;
            stripList.head = stripPtr->next;

            RwFree(stripPtr->strip);
            stripPtr->strip = (RwUInt16 *)NULL;

            RwFreeListFree(RPMESHGLOBAL(triStripListEntryFreeList), stripPtr);
            stripPtr = (TriStripListEntry *)NULL;
        }
        RwFree(triList);
        triList = (RpBuildMeshTriangle *)NULL;
    }

    RwFreeListDestroy(RPMESHGLOBAL(triStripListEntryFreeList));
    RPMESHGLOBAL(triStripListEntryFreeList) = (RwFreeList *)NULL;

    /* now build the whole mesh */
    meshSize = sizeof(RpMeshHeader);
    totalIndices = 0;
    for (i = 0; i < numOutMeshes; i++) {
        RwUInt32 indexSize;

        indexSize = (sizeof(RxVertexIndex) * outMeshes[i]->numIndices);
        meshSize += sizeof(RpMesh) + indexSize;

        totalIndices += outMeshes[i]->numIndices;
    }

    result = _rpMeshHeaderCreate(meshSize);
    result->flags = rpMESHHEADERTRISTRIP;
    result->numMeshes = numOutMeshes;
    result->serialNum = RPMESHGLOBAL(nextSerialNum);
    result->firstMeshOffset = 0;
    result->totalIndicesInMesh = totalIndices;
    RPMESHGLOBAL(nextSerialNum)++;

    meshEl = (RpMesh *)(result + 1);
    stripMeshInds = (RxVertexIndex *)(meshEl + numOutMeshes);
    for (i = 0; i < numOutMeshes; i++) {
        /* Add in the next mesh */
        meshEl->indices = stripMeshInds;
        meshEl->numIndices = outMeshes[i]->numIndices;
        meshEl->material = outMeshes[i]->material;

        /* And the indices */
        meshSize = (sizeof(RxVertexIndex) * outMeshes[i]->numIndices);
        memcpy(stripMeshInds, outMeshes[i]->indices, meshSize);

        /* Skip to next */
        stripMeshInds += meshEl->numIndices;
        meshEl++;

        /* Don't need this any more */
        RwFree(outMeshes[i]);
        outMeshes[i] = (RpMesh *)NULL;
    }

    RwFree(triPointers);
    triPointers = (RpBuildMeshTriangle **)NULL;

    RwFree(outMeshes);
    outMeshes = (RpMesh **)NULL;

    RwFree(outMeshInfo);
    outMeshInfo = (RpMesh *)NULL;

    RWRETURN(result);
}

RpMeshHeader *RpBuildMeshGenerateDefaultTriStrip(RpBuildMesh *buildMesh,
                                                 void *data __RWUNUSED__) {
    RWAPIFUNCTION(RWSTRING("RpBuildMeshGenerateDefaultTriStrip"));
    RWASSERT(buildMesh);

    RWRETURN(TriStripMeshGenerate(buildMesh, FALSE, TRUE));
}

/****************************************************************************
 _rpTriListMeshGenerate

 On entry   :
 On exit    :
 */
RpMeshHeader *_rpTriListMeshGenerate(RpBuildMesh *buildMesh,
                                     void *data __RWUNUSED__) {
    RpBuildMeshTriangle **triPointers;

    RWFUNCTION(RWSTRING("_rpTriListMeshGenerate"));
    RWASSERT(buildMesh);

    triPointers = (RpBuildMeshTriangle **)RwMalloc(
        buildMesh->numTriangles * sizeof(RpBuildMeshTriangle *),
        rwMEMHINTDUR_FUNCTION | rwID_MESHMODULE);

    if (triPointers) {
        RpMeshHeader *result;
        RwUInt32 i;
        RwUInt32 numMats;
        RwUInt32 meshSize;
        RpMesh *meshEl;
        RxVertexIndex *meshTriInds;

        /* Fill in pointers so that we can sort */
        for (i = 0; i < buildMesh->numTriangles; i++) {
            triPointers[i] = &(buildMesh->meshTriangles[i]);
        }

        /* Now sort the pointers by material */
        qsort(triPointers, buildMesh->numTriangles,
              sizeof(RpBuildMeshTriangle *), SortPolygons);

        /* Figure out how many different materials there are */
        numMats = 1;
        if (buildMesh->numTriangles >= 2) {
            RpMaterial *lastMat = triPointers[0]->material;

            for (i = 1; i < buildMesh->numTriangles; i++) {
                if (triPointers[i]->material != lastMat) {
                    /* We found another different one */
                    lastMat = triPointers[i]->material;
                    numMats++;
                }
            }
        }

        /* And generate an output mesh
         * (allow a bit per material for alignment) */
        meshSize = (sizeof(RpMeshHeader)) + (sizeof(RpMesh) * numMats) +
                   (sizeof(RxVertexIndex) * 3 * buildMesh->numTriangles);

        result = _rpMeshHeaderCreate(meshSize);
        if (!result) {
            RwFree(triPointers);
            RWERROR((E_RW_NOMEM, meshSize));
            RWRETURN((RpMeshHeader *)NULL);
        }

        result->flags = 0;
        result->numMeshes = 1;
        result->serialNum = RPMESHGLOBAL(nextSerialNum);
        result->firstMeshOffset = 0;
        /* Nothing clever done for tri lists re connecting triangles */
        result->totalIndicesInMesh = buildMesh->numTriangles * 3;
        RPMESHGLOBAL(nextSerialNum)++;

        meshEl = (RpMesh *)(result + 1);
        meshTriInds = (RxVertexIndex *)(meshEl + numMats);

        /* Start here */
        meshEl->numIndices = 0;
        meshEl->material = triPointers[0]->material;
        meshEl->indices = meshTriInds;
        i = 0;
        do {
            if (triPointers[i]->material != meshEl->material) {
                meshEl++;

                meshEl->numIndices = 0;
                meshEl->material = triPointers[i]->material;
                meshEl->indices = meshTriInds;

                result->numMeshes++;
            }

            *meshTriInds++ = (RxVertexIndex)triPointers[i]->vertIndex[0];
            *meshTriInds++ = (RxVertexIndex)triPointers[i]->vertIndex[1];
            *meshTriInds++ = (RxVertexIndex)triPointers[i]->vertIndex[2];

            meshEl->numIndices += 3;

            i++;
        } while (i < buildMesh->numTriangles);

        /* We can do more to optimise this mesh.  By separating polygons which
         * share vertices, we can dispatch more polygons in one go when we have
         * a situation where polygons are supplying the texture coordinates,
         * eg.
         */

        _rpMeshHeaderForAllMeshes(result, SortPolygonsInTriListMesh, NULL);

        /* Free the workspace */
        RwFree(triPointers);

        /* Return the new mesh */
        RWRETURN(result);
    }

    /* Failed to allocate workspace */
    RWERROR(
        (E_RW_NOMEM, buildMesh->numTriangles * sizeof(RpBuildMeshTriangle *)));
    RWRETURN((RpMeshHeader *)NULL);
}

/****************************************************************************
 _rpMeshOptimise

 On entry   : Mesh to optimise, flags indicating type of mesh to build
 On exit    : New mesh list on success, or NULL on failure
 */
RpMeshHeader *_rpMeshOptimise(RpBuildMesh *mesh, RwUInt32 flags) {
    /* The complete body is exact; retail selects helper save/restore emission.
     */
    RpTriStripMeshCallBack func = (RpTriStripMeshCallBack)NULL;
    void *data = NULL;

    RWFUNCTION(RWSTRING("_rpMeshOptimise"));
    RWASSERT(mesh);

    if (mesh) {
        if (mesh->numTriangles == 0) {
            /* Flags don't matter, with 0 triangles, it makes no difference! */
            _rpBuildMeshDestroy(mesh);
            RWRETURN(&MeshopStatic.nullMeshHeader);
        } else {
            RpMeshHeader *newMesh;

            if (flags & rpMESHHEADERTRISTRIP) {
                /* How have we been told to tristrip the meshes? */
                func = MeshopStatic.meshTristripMethod;
                data = MeshopStatic.data;
            } else {
                func = _rpTriListMeshGenerate;
                data = NULL;
            }

            RWASSERT(((RpTriStripMeshCallBack)NULL) != func);

            newMesh = func(mesh, data);

            if (newMesh) {
                _rpBuildMeshDestroy(mesh);

                RWRETURN(newMesh);
            }

            /* Failed to generate the mesh, error already output */
        }
    }

    RWRETURN((RpMeshHeader *)NULL);
}
