#include "libmkparticle/rw_engine.h"
#include "rw/gamecube.h"
#include "rw/rpskin.h"
#include "rw/rwresources.h"
#include "rw/rxpipeline.h"

typedef struct GeomCondVertexData {
    void* data;
    RwUInt32 type;
    signed char dependencies[16];
} GeomCondVertexData;

typedef struct GeomCondMap {
    void* data;
    RwUInt32 count;
} GeomCondMap;

extern GeomCondMap* VertexDataCreateMaps(const GeomCondVertexData* streams,
                                         RwUInt32 numStreams,
                                         RwUInt32 numVertices);
extern GeomCondMap* VertexDataCreateRemapped(
    const GeomCondMap* maps, const GeomCondVertexData* streams,
    RwUInt32 numStreams, RwUInt32 numVertices);
extern RwUInt16** IndexDataCreateRemapped(
    const GeomCondMap* maps, const RwUInt16* const* input,
    RwUInt32 numArrays, RwUInt32 numIndices);
extern void* memset(void* destination, RwInt32 value, RwUInt32 size);
extern void* memcpy(void* destination, const void* source, RwUInt32 size);
extern void DCFlushRange(void* start, RwUInt32 length);
extern void GXInvalidateVtxCache(void);

extern RwInt32 _rpDlGeomVtxFmtOffset;
extern RwUInt16 _RwDlTokenLastSeen;

static RwGameCubeVertexDescriptor VtxDesc;

static RwBool ReconditionVertexIndexData(RpGeometry* geometry,
                                         GeomCondMap** remappedVertices,
                                         RwUInt16**** remappedIndices)
{
    GeomCondVertexData streams[13];
    RpMorphTarget* morphTarget = geometry->morphTarget;
    RpSkin* skin;
    GeomCondMap* maps;
    RwUInt16** sourceIndices;
    RwUInt32 numStreams;
    RwUInt32 skinStream;
    RwUInt32 numIndexStreams;
    RwUInt32 meshIndex;
    RwUInt32 streamIndex;

    if (*(RpGameCubeVtxFmt**)((RwUInt8*)geometry + _rpDlGeomVtxFmtOffset) ==
        NULL) {
        _rpGameCubeVtxFmtGetDefault();
    }

    streams[0].data = morphTarget->verts;
    streams[0].type = 8;
    streams[0].dependencies[0] = -1;
    numStreams = 1;

    if ((geometry->flags & 0x10) != 0) {
        streams[1].data = morphTarget->normals;
        streams[1].type = 8;
        streams[1].dependencies[0] = -1;
        numStreams = 2;
    }
    if ((geometry->flags & 8) != 0) {
        streams[numStreams].data = geometry->preLitLum;
        streams[numStreams].type = 4;
        streams[numStreams].dependencies[0] = -1;
        numStreams++;
    }
    if ((geometry->flags & 0x84) != 0) {
        RwInt32 texCoord;
        for (texCoord = 0; texCoord < geometry->numTexCoordSets; texCoord++) {
            streams[numStreams].data = geometry->texCoords[texCoord];
            streams[numStreams].type = 7;
            streams[numStreams].dependencies[0] = -1;
            numStreams++;
        }
    }

    skin = RpSkinGeometryGetSkin(geometry);
    if (skin->maxNumWeights > 1) {
        streams[numStreams].data = skin->platformIndices;
        switch (skin->maxNumWeights) {
        case 1:
            streams[numStreams].type = 1;
            break;
        case 2:
            streams[numStreams].type = 2;
            break;
        case 3:
            streams[numStreams].type = 3;
            break;
        case 4:
            streams[numStreams].type = 4;
            break;
        }
    } else {
        streams[numStreams].data = skin->vertexBoneIndices;
        streams[numStreams].type = 4;
    }
    streams[numStreams].dependencies[0] = -1;
    skinStream = numStreams++;

    if (skin->maxNumWeights > 1) {
        streams[numStreams].data = skin->platformWeights;
        streams[numStreams].type = streams[skinStream].type;
    } else {
        streams[numStreams].data = skin->vertexBoneWeights;
        streams[numStreams].type = 9;
    }
    streams[numStreams].dependencies[0] = -1;
    numStreams++;
    numIndexStreams = numStreams - 2;

    if ((geometry->flags & 0x10) != 0) {
        streams[0].dependencies[0] = 1;
        streams[0].dependencies[1] = (signed char)(numStreams - 2);
        streams[0].dependencies[2] = (signed char)(numStreams - 1);
        streams[0].dependencies[3] = -1;
        streams[1].dependencies[0] = 0;
        streams[1].dependencies[1] = (signed char)(numStreams - 2);
        streams[1].dependencies[2] = (signed char)(numStreams - 1);
        streams[1].dependencies[3] = -1;
        streams[numStreams - 2].dependencies[0] = 0;
        streams[numStreams - 2].dependencies[1] = 1;
        streams[numStreams - 2].dependencies[2] =
            (signed char)(numStreams - 1);
        streams[numStreams - 2].dependencies[3] = -1;
        streams[numStreams - 1].dependencies[0] = 0;
        streams[numStreams - 1].dependencies[1] = 1;
        streams[numStreams - 1].dependencies[2] =
            (signed char)(numStreams - 2);
        streams[numStreams - 1].dependencies[3] = -1;
    } else {
        streams[0].dependencies[0] = (signed char)(numStreams - 2);
        streams[0].dependencies[1] = (signed char)(numStreams - 1);
        streams[0].dependencies[2] = -1;
        streams[numStreams - 2].dependencies[0] = 0;
        streams[numStreams - 2].dependencies[1] =
            (signed char)(numStreams - 1);
        streams[numStreams - 2].dependencies[2] = -1;
        streams[numStreams - 1].dependencies[0] = 0;
        streams[numStreams - 1].dependencies[1] =
            (signed char)(numStreams - 2);
        streams[numStreams - 1].dependencies[2] = -1;
    }

    maps = VertexDataCreateMaps(streams, numStreams, geometry->numVertices);
    if (maps == NULL)
        return FALSE;

    *remappedVertices = VertexDataCreateRemapped(
        maps, streams, numStreams, geometry->numVertices);
    if (*remappedVertices == NULL) {
        RwEngineInstance->fpFree(maps);
        return FALSE;
    }

    sourceIndices = RwEngineInstance->fpMalloc(
        numIndexStreams * geometry->meshHeader->numMeshes *
            sizeof(*sourceIndices),
        0x10116);
    if (sourceIndices == NULL) {
        RwEngineInstance->fpFree(*remappedVertices);
        RwEngineInstance->fpFree(maps);
        return FALSE;
    }
    memset(sourceIndices, 0,
           numIndexStreams * geometry->meshHeader->numMeshes *
               sizeof(*sourceIndices));
    for (meshIndex = 0; meshIndex < geometry->meshHeader->numMeshes;
         meshIndex++) {
        RpMesh* mesh = (RpMesh*)((RwUInt8*)geometry->meshHeader +
                                geometry->meshHeader->firstMeshOffset) +
                       meshIndex;
        for (streamIndex = 0; streamIndex < numIndexStreams; streamIndex++)
            sourceIndices[meshIndex * numIndexStreams + streamIndex] =
                mesh->indices;
    }

    *remappedIndices = RwEngineInstance->fpMalloc(
        geometry->meshHeader->numMeshes * sizeof(**remappedIndices), 0x30116);
    if (*remappedIndices == NULL) {
        RwEngineInstance->fpFree(sourceIndices);
        RwEngineInstance->fpFree(*remappedVertices);
        RwEngineInstance->fpFree(maps);
        return FALSE;
    }

    for (meshIndex = 0; meshIndex < geometry->meshHeader->numMeshes;
         meshIndex++) {
        RpMesh* mesh = (RpMesh*)((RwUInt8*)geometry->meshHeader +
                                geometry->meshHeader->firstMeshOffset) +
                       meshIndex;
        (*remappedIndices)[meshIndex] = IndexDataCreateRemapped(
            maps, (const RwUInt16* const*)&sourceIndices[meshIndex *
                                                        numIndexStreams],
            numIndexStreams, mesh->numIndices);
        if ((*remappedIndices)[meshIndex] == NULL) {
            while (--meshIndex > 0)
                RwEngineInstance->fpFree((*remappedIndices)[meshIndex]);
            return FALSE;
        }
    }

    RwEngineInstance->fpFree(sourceIndices);
    RwEngineInstance->fpFree(maps);
    return TRUE;
}

static const RwGameCubeVertexDescriptor* VtxDescInitOptimized(
    RpGeometry* geometry, const GeomCondMap* streams)
{
    RpSkin* skin = RpSkinGeometryGetSkin(geometry);
    RpGameCubeVtxFmt* format =
        *(RpGameCubeVtxFmt**)((RwUInt8*)geometry + _rpDlGeomVtxFmtOffset);
    RwUInt8 streamIndex = 1;
    RwInt32 texCoord;

    if (format == NULL)
        format = _rpGameCubeVtxFmtGetDefault();
    _rwVertexDescriptorInit(&VtxDesc);
    _rwGCNVertexDescSetVAT(&VtxDesc, 0);
    if (skin->maxNumWeights == 1)
        _rwGCNVertexDescSetElementDesc(&VtxDesc, 0, 1);
    _rwGCNVertexDescSetElementAttr(
        &VtxDesc, 9, 1, format->positionType, format->positionFraction);
    _rwGCNVertexDescSetElementDesc(
        &VtxDesc, 9, streams[0].count >= 0xFF ? 3 : 2);

    if ((geometry->flags & 0x10) != 0) {
        if (format->normalMode != 0) {
            _rwGCNVertexDescSetElementAttr(
                &VtxDesc, 0x19, 1, format->normalType, 0);
            _rwGCNVertexDescSetElementDesc(
                &VtxDesc, 0x19, streams[1].count >= 0xFF ? 3 : 2);
        } else {
            _rwGCNVertexDescSetElementAttr(
                &VtxDesc, 0xA, 0, format->normalType, 0);
            _rwGCNVertexDescSetElementDesc(
                &VtxDesc, 0xA, streams[1].count >= 0xFF ? 3 : 2);
        }
        streamIndex = 2;
    }
    if ((geometry->flags & 8) != 0) {
        _rwGCNVertexDescSetElementAttr(
            &VtxDesc, 0xB, format->colorType > 2, format->colorType, 0);
        _rwGCNVertexDescSetElementDesc(
            &VtxDesc, 0xB,
            streams[streamIndex].count >= 0xFF ? 3 : 2);
        streamIndex++;
    }
    if ((geometry->flags & 0x84) != 0) {
        for (texCoord = 0; texCoord < geometry->numTexCoordSets; texCoord++) {
            _rwGCNVertexDescSetElementAttr(
                &VtxDesc, texCoord + 0xD, 1,
                format->texCoordType[texCoord],
                format->texCoordFraction[texCoord]);
            _rwGCNVertexDescSetElementDesc(
                &VtxDesc, texCoord + 0xD,
                streams[streamIndex].count >= 0xFF ? 3 : 2);
            streamIndex++;
        }
    }
    _rwGCNVertexDescSetNumIndexedAttr(&VtxDesc, streamIndex);
    return &VtxDesc;
}

static void VertexDataSetupOptimized(RwGameCubeVertexData* vertexData,
                                     const GeomCondMap* streams,
                                     const RpGeometry* geometry)
{
    RwUInt32 streamIndex = 1;
    RwUInt32 texCoord;

    if (*(RpGameCubeVtxFmt**)((RwUInt8*)geometry + _rpDlGeomVtxFmtOffset) ==
        NULL) {
        _rpGameCubeVtxFmtGetDefault();
    }
    vertexData->counts[9] = streams[0].count;
    vertexData->source[9] = streams[0].data;
    if ((geometry->flags & 0x10) != 0) {
        vertexData->counts[10] = streams[1].count;
        vertexData->source[10] = streams[1].data;
        streamIndex = 2;
    }
    if ((geometry->flags & 8) != 0) {
        vertexData->counts[11] = streams[streamIndex].count;
        vertexData->source[11] = streams[streamIndex].data;
        streamIndex++;
    }
    if ((geometry->flags & 0x84) != 0) {
        for (texCoord = 0; texCoord < (RwUInt32)geometry->numTexCoordSets;
             texCoord++) {
            vertexData->counts[texCoord + 13] = streams[streamIndex].count;
            vertexData->source[texCoord + 13] = streams[streamIndex].data;
            streamIndex++;
        }
    }
}

static void IndexDataSetupOptimized(RwGameCubeIndexData* indexData,
                                    RwUInt16* const* indices,
                                    const RpGeometry* geometry,
                                    RwUInt16* matrixIndices)
{
    RwUInt32 streamIndex = 1;
    RwUInt32 texCoord;

    if (*(RpGameCubeVtxFmt**)((RwUInt8*)geometry + _rpDlGeomVtxFmtOffset) ==
        NULL) {
        _rpGameCubeVtxFmtGetDefault();
    }
    if (matrixIndices != NULL)
        indexData->attributes[0] = matrixIndices;
    indexData->attributes[9] = indices[0];
    if ((geometry->flags & 0x10) != 0) {
        indexData->attributes[10] = indices[1];
        streamIndex = 2;
    }
    if ((geometry->flags & 8) != 0) {
        indexData->attributes[11] = indices[streamIndex];
        streamIndex++;
    }
    if ((geometry->flags & 0x84) != 0) {
        for (texCoord = 0; texCoord < (RwUInt32)geometry->numTexCoordSets;
             texCoord++) {
            indexData->attributes[texCoord + 13] = indices[streamIndex];
            streamIndex++;
        }
    }
}

static RwUInt16* CreateMatrixIndexListOptimized(
    const RpSkin* skin, const RwUInt16* primitiveIndices,
    const RwUInt8* boneIndices, RwInt32 numIndices, RwUInt32 meshIndex)
{
    RwUInt8 meshBones[10];
    RwUInt16* matrixIndices;
    RwUInt32 index;

    if (skin->splitData.numMeshes != 0) {
        const RpSkinRLECount* count = &skin->splitData.rleCount[meshIndex];
        RwUInt8* current = meshBones;
        RwUInt32 run;
        for (run = 0; run < count->size; run++) {
            const RpSkinRLE* rle =
                &skin->splitData.rle[count->start + run];
            RwUInt8 bone;
            for (bone = 0; bone < rle->count; bone++)
                *current++ = rle->startBone + bone;
        }
        matrixIndices = RwEngineInstance->fpMalloc(
            numIndices * sizeof(*matrixIndices), 0x30116);
        for (index = 0; index < (RwUInt32)numIndices; index++) {
            RwUInt32 matrixIndex;
            RwUInt8 vertexBone = boneIndices[primitiveIndices[index] * 4];
            for (matrixIndex = 0; matrixIndex < 10; matrixIndex++) {
                if (meshBones[matrixIndex] == vertexBone) {
                    matrixIndices[index] = (RwUInt16)(matrixIndex * 3);
                    break;
                }
            }
        }
    } else {
        matrixIndices = RwEngineInstance->fpMalloc(
            numIndices * sizeof(*matrixIndices), 0x30116);
        for (index = 0; index < (RwUInt32)numIndices; index++) {
            RwUInt32 matrixIndex;
            RwUInt8 vertexBone = boneIndices[primitiveIndices[index] * 4];
            for (matrixIndex = 0; matrixIndex < skin->numUsedBones;
                 matrixIndex++) {
                if (skin->usedBoneList[matrixIndex] == vertexBone) {
                    matrixIndices[index] = (RwUInt16)(matrixIndex * 3);
                    break;
                }
            }
        }
    }
    return matrixIndices;
}

static const RwGameCubeVertexDescriptor* VtxDescInitFast(
    const RpGeometry* geometry)
{
    RpGameCubeVtxFmt* format =
        *(RpGameCubeVtxFmt**)((RwUInt8*)geometry + _rpDlGeomVtxFmtOffset);
    RwUInt32 numVertices = (RwUInt32)geometry->numVertices;
    RwUInt8 indexedCount = 1;
    RwInt32 texCoord;

    if (format == NULL)
        format = _rpGameCubeVtxFmtGetDefault();
    _rwVertexDescriptorInit(&VtxDesc);
    _rwGCNVertexDescSetVAT(&VtxDesc, 0);
    _rwGCNVertexDescSetElementDesc(&VtxDesc, 0, 1);
    _rwGCNVertexDescSetElementAttr(
        &VtxDesc, 9, 1, format->positionType, format->positionFraction);
    _rwGCNVertexDescSetElementDesc(
        &VtxDesc, 9, numVertices >= 0xFF ? 3 : 2);
    if ((geometry->flags & 0x10) != 0) {
        if (format->normalMode != 0) {
            _rwGCNVertexDescSetElementAttr(
                &VtxDesc, 0x19, 1, format->normalType, 0);
            _rwGCNVertexDescSetElementDesc(
                &VtxDesc, 0x19, numVertices >= 0xFF ? 3 : 2);
        } else {
            _rwGCNVertexDescSetElementAttr(
                &VtxDesc, 0xA, 0, format->normalType, 0);
            _rwGCNVertexDescSetElementDesc(
                &VtxDesc, 0xA, numVertices >= 0xFF ? 3 : 2);
        }
        indexedCount = 2;
    }
    if ((geometry->flags & 8) != 0) {
        _rwGCNVertexDescSetElementAttr(
            &VtxDesc, 0xB, format->colorType > 2, format->colorType, 0);
        _rwGCNVertexDescSetElementDesc(
            &VtxDesc, 0xB, numVertices >= 0xFF ? 3 : 2);
        indexedCount++;
    }
    if ((geometry->flags & 0x84) != 0) {
        for (texCoord = 0; texCoord < geometry->numTexCoordSets; texCoord++) {
            _rwGCNVertexDescSetElementAttr(
                &VtxDesc, texCoord + 0xD, 1,
                format->texCoordType[texCoord],
                format->texCoordFraction[texCoord]);
            _rwGCNVertexDescSetElementDesc(
                &VtxDesc, texCoord + 0xD, numVertices >= 0xFF ? 3 : 2);
            indexedCount++;
        }
    }
    _rwGCNVertexDescSetNumIndexedAttr(&VtxDesc, indexedCount);
    return &VtxDesc;
}

static void VertexDataFastSetup(RwGameCubeVertexData* vertexData,
                                const RpGeometry* geometry)
{
    const RpMorphTarget* morphTarget = geometry->morphTarget;
    RwInt32 texCoord;

    if (*(RpGameCubeVtxFmt**)((RwUInt8*)geometry + _rpDlGeomVtxFmtOffset) ==
        NULL) {
        _rpGameCubeVtxFmtGetDefault();
    }
    vertexData->counts[9] = geometry->numVertices;
    vertexData->source[9] = morphTarget->verts;
    if ((geometry->flags & 0x10) != 0) {
        vertexData->counts[10] = geometry->numVertices;
        vertexData->source[10] = morphTarget->normals;
    }
    if ((geometry->flags & 8) != 0) {
        vertexData->counts[11] = geometry->numVertices;
        vertexData->source[11] = geometry->preLitLum;
    }
    if ((geometry->flags & 0x84) != 0) {
        for (texCoord = 0; texCoord < geometry->numTexCoordSets; texCoord++) {
            vertexData->counts[texCoord + 13] = geometry->numVertices;
            vertexData->source[texCoord + 13] = geometry->texCoords[texCoord];
        }
    }
}

static void IndexDataSetupFast(RwGameCubeIndexData* indexData,
                               RwUInt16* primitiveIndices,
                               RwUInt16* matrixIndices,
                               const RpGeometry* geometry)
{
    RwInt32 texCoord;

    if (*(RpGameCubeVtxFmt**)((RwUInt8*)geometry + _rpDlGeomVtxFmtOffset) ==
        NULL) {
        _rpGameCubeVtxFmtGetDefault();
    }
    indexData->attributes[0] = matrixIndices;
    indexData->attributes[9] = primitiveIndices;
    if ((geometry->flags & 0x10) != 0)
        indexData->attributes[10] = primitiveIndices;
    if ((geometry->flags & 8) != 0)
        indexData->attributes[11] = primitiveIndices;
    if ((geometry->flags & 0x84) != 0) {
        for (texCoord = 0; texCoord < geometry->numTexCoordSets; texCoord++)
            indexData->attributes[texCoord + 13] = primitiveIndices;
    }
}

static RwUInt16* CreateMatrixIndexList(const RpSkin* skin,
                                       const RpMesh* mesh,
                                       RwUInt32 meshIndex)
{
    RwUInt8 meshBones[10];
    RwUInt16* matrixIndices;
    RwUInt32 index;

    if (skin->splitData.numMeshes != 0) {
        const RpSkinRLECount* count = &skin->splitData.rleCount[meshIndex];
        RwUInt8* current = meshBones;
        RwUInt32 run;
        for (run = 0; run < count->size; run++) {
            const RpSkinRLE* rle =
                &skin->splitData.rle[count->start + run];
            RwUInt8 bone;
            for (bone = 0; bone < rle->count; bone++)
                *current++ = rle->startBone + bone;
        }
        matrixIndices = RwEngineInstance->fpMalloc(
            mesh->numIndices * sizeof(*matrixIndices), 0x30116);
        for (index = 0; index < mesh->numIndices; index++) {
            RwUInt32 matrixIndex;
            RwUInt8 vertexBone = ((const RwUInt8*)skin->vertexBoneIndices)
                [mesh->indices[index] * 4];
            for (matrixIndex = 0; matrixIndex < 10; matrixIndex++) {
                if (meshBones[matrixIndex] == vertexBone) {
                    matrixIndices[index] = (RwUInt16)(matrixIndex * 3);
                    break;
                }
            }
        }
    } else {
        matrixIndices = RwEngineInstance->fpMalloc(
            mesh->numIndices * sizeof(*matrixIndices), 0x30116);
        for (index = 0; index < mesh->numIndices; index++) {
            RwUInt32 matrixIndex;
            RwUInt8 vertexBone = ((const RwUInt8*)skin->vertexBoneIndices)
                [mesh->indices[index] * 4];
            for (matrixIndex = 0; matrixIndex < skin->numUsedBones;
                 matrixIndex++) {
                if (skin->usedBoneList[matrixIndex] == vertexBone) {
                    matrixIndices[index] = (RwUInt16)(matrixIndex * 3);
                    break;
                }
            }
        }
    }
    return matrixIndices;
}

RwResEntry* _rwDlGeometrySkinInstanceOptimized(RpGeometry* geometry,
                                                void* owner,
                                                RwResEntry** ownerRef)
{
    GeomCondMap* remappedVertices;
    RwUInt16*** remappedIndices;
    RwGameCubeVertexData vertexData;
    RwGameCubeIndexData indexData;
    const RwGameCubeVertexDescriptor* descriptor;
    RwResEntry* entry;
    RwGameCubeVertexBuffer* vertexBuffer;
    RwGameCubeDisplayList* displayLists;
    RpSkin* skin;
    RwUInt32 headerSize;
    RwUInt32 displayArraySize;
    RwUInt32 displaySize;
    RwUInt32 vertexSize;
    RwUInt32 totalSize;
    RwUInt32 dataOffset;
    RwUInt32 meshIndex;
    RwUInt8 primitive;

    ReconditionVertexIndexData(geometry, &remappedVertices,
                               &remappedIndices);
    descriptor = VtxDescInitOptimized(geometry, remappedVertices);
    VertexDataSetupOptimized(&vertexData, remappedVertices, geometry);
    headerSize = _rwGCNVertexBufferHeaderGetSize(descriptor);
    displayArraySize = geometry->meshHeader->numMeshes *
                       sizeof(RwGameCubeDisplayList);
    displaySize = headerSize + displayArraySize + 0x1F;
    primitive = (geometry->flags & 1) != 0 ? 0x98 : 0x90;

    for (meshIndex = 0; meshIndex < geometry->meshHeader->numMeshes;
         meshIndex++) {
        RpMesh* mesh = (RpMesh*)((RwUInt8*)geometry->meshHeader +
                                geometry->meshHeader->firstMeshOffset) +
                       meshIndex;
        if ((geometry->flags & 1) != 0) {
            RwUInt32 numStrips;
            RwUInt32 stripIndices;
            _rwGCNTriStripGetStats(remappedIndices[meshIndex][0],
                                   mesh->numIndices, &numStrips,
                                   &stripIndices, TRUE);
            displaySize += _rwGCNDisplayListGetSize(
                descriptor, numStrips, stripIndices);
        } else {
            displaySize += _rwGCNDisplayListGetSize(
                descriptor, 1, mesh->numIndices);
        }
    }
    vertexSize = _rwGCNVertexBufferGetSize(descriptor, vertexData.counts);
    totalSize = displaySize + vertexSize;
    entry = RwEngineInstance->fpMalloc(
        totalSize + sizeof(*entry), 0x30116);
    entry->link.next = NULL;
    entry->link.prev = NULL;
    entry->owner = owner;
    entry->size = totalSize;
    entry->ownerRef = ownerRef;
    entry->destroyNotify = _rxGCResEntryWaitDone;
    *ownerRef = entry;
    vertexBuffer = (RwGameCubeVertexBuffer*)(entry + 1);
    memset(vertexBuffer, 0, totalSize);
    ((RwUInt16*)vertexBuffer)[0] = _RwDlTokenLastSeen;
    ((RwUInt16*)vertexBuffer)[1] = geometry->meshHeader->serialNum;
    vertexBuffer->reserved_0x00[1] = 0;
    if ((geometry->flags & 8) != 0) {
        RpGameCubeVtxFmt* format = *(RpGameCubeVtxFmt**)(
            (RwUInt8*)geometry + _rpDlGeomVtxFmtOffset);
        if (format == NULL) {
            RwInt32 vertex;
            vertexBuffer->reserved_0x00[1] &= ~1U;
            for (vertex = 0; vertex < geometry->numVertices; vertex++) {
                if (((const RwRGBA*)geometry->preLitLum)[vertex].alpha < 0xFF) {
                    vertexBuffer->reserved_0x00[1] |= 1;
                    break;
                }
            }
        } else if (format->colorType > 2) {
            vertexBuffer->reserved_0x00[1] |= 1;
        } else {
            vertexBuffer->reserved_0x00[1] &= ~1U;
        }
    }
    displayLists = (RwGameCubeDisplayList*)((RwUInt8*)vertexBuffer +
                                           headerSize);
    dataOffset = (RwUInt32)((RwUInt8*)displayLists + displayArraySize + 0x1F) &
                 ~0x1FU;
    skin = RpSkinGeometryGetSkin(geometry);

    for (meshIndex = 0; meshIndex < geometry->meshHeader->numMeshes;
         meshIndex++) {
        RpMesh* mesh = (RpMesh*)((RwUInt8*)geometry->meshHeader +
                                geometry->meshHeader->firstMeshOffset) +
                       meshIndex;
        RwUInt32 numStrips;
        RwUInt32 stripIndices;
        RwUInt32 listSize;
        RwUInt16* matrixIndices = NULL;
        RwUInt32 stride;
        RwUInt32 attributeStreams;

        if ((geometry->flags & 1) != 0) {
            _rwGCNTriStripGetStats(remappedIndices[meshIndex][0],
                                   mesh->numIndices, &numStrips,
                                   &stripIndices, TRUE);
        } else {
            stripIndices = mesh->numIndices;
            numStrips = 1;
        }
        listSize = _rwGCNDisplayListGetSize(
            descriptor, numStrips, stripIndices);
        _rwGCNDisplayListInitialize(&displayLists[meshIndex], meshIndex,
                                    listSize, (void*)dataOffset);

        if (skin->maxNumWeights > 1) {
            IndexDataSetupOptimized(&indexData,
                                    remappedIndices[meshIndex], geometry,
                                    NULL);
        } else {
            attributeStreams = 1;
            if ((geometry->flags & 0x10) != 0)
                attributeStreams = 2;
            if ((geometry->flags & 8) != 0)
                attributeStreams++;
            if ((geometry->flags & 0x84) != 0)
                attributeStreams += geometry->numTexCoordSets;
            matrixIndices = CreateMatrixIndexListOptimized(
                skin, remappedIndices[meshIndex][0],
                remappedVertices[attributeStreams].data,
                mesh->numIndices, meshIndex);
            IndexDataSetupOptimized(&indexData,
                                    remappedIndices[meshIndex], geometry,
                                    matrixIndices);
        }
        stride = _rwGCNDisplayListGetStride(descriptor);
        _rwGCNDisplayListFill(
            descriptor, &displayLists[meshIndex], &indexData,
            mesh->numIndices, (geometry->flags & 1) != 0, stride,
            TRUE, primitive, NULL);
        if (matrixIndices != NULL)
            RwEngineInstance->fpFree(matrixIndices);
        dataOffset += listSize;
    }

    _rwGCNVertexBufferInitialize(descriptor, vertexBuffer,
                                 vertexData.counts, (void*)dataOffset);
    _rwGCNVertexBufferFill(
        descriptor, (const RwGameCubeVertexStreams*)vertexBuffer,
        &vertexData, FALSE, NULL);
    if (skin->maxNumWeights > 1) {
        RwUInt32 stream = 1;
        if ((geometry->flags & 0x10) != 0)
            stream = 2;
        if ((geometry->flags & 8) != 0)
            stream++;
        if ((geometry->flags & 0x84) != 0)
            stream += geometry->numTexCoordSets;
        memcpy(skin->platformIndices, remappedVertices[stream].data,
               skin->maxNumWeights * remappedVertices[stream].count);
        memcpy(skin->platformWeights, remappedVertices[stream + 1].data,
               skin->maxNumWeights * remappedVertices[stream + 1].count);
    }
    geometry->numVertices = remappedVertices[0].count;
    DCFlushRange(vertexBuffer, totalSize);
    GXInvalidateVtxCache();
    for (meshIndex = 0; meshIndex < geometry->meshHeader->numMeshes;
         meshIndex++)
        RwEngineInstance->fpFree(remappedIndices[meshIndex]);
    RwEngineInstance->fpFree(remappedIndices);
    RwEngineInstance->fpFree(remappedVertices);
    return entry;
}

RwResEntry* _rwDlGeometrySkinInstanceFast(RpGeometry* geometry,
                                           void* owner,
                                           RwResEntry** ownerRef)
{
    RwGameCubeVertexData vertexData;
    RwGameCubeIndexData indexData;
    const RwGameCubeVertexDescriptor* descriptor;
    RwResEntry* entry;
    RwGameCubeVertexBuffer* vertexBuffer;
    RwGameCubeDisplayList* displayLists;
    RpSkin* skin;
    RwUInt32 headerSize;
    RwUInt32 displayArraySize;
    RwUInt32 displaySize;
    RwUInt32 vertexSize;
    RwUInt32 totalSize;
    RwUInt32 dataOffset;
    RwUInt32 meshIndex;
    RwUInt8 primitive;

    skin = RpSkinGeometryGetSkin(geometry);
    descriptor = VtxDescInitFast(geometry);
    VertexDataFastSetup(&vertexData, geometry);
    headerSize = _rwGCNVertexBufferHeaderGetSize(descriptor);
    displayArraySize = geometry->meshHeader->numMeshes *
                       sizeof(RwGameCubeDisplayList);
    displaySize = headerSize + displayArraySize + 0x1F;
    primitive = (geometry->flags & 1) != 0 ? 0x98 : 0x90;
    for (meshIndex = 0; meshIndex < geometry->meshHeader->numMeshes;
         meshIndex++) {
        RpMesh* mesh = (RpMesh*)((RwUInt8*)geometry->meshHeader +
                                geometry->meshHeader->firstMeshOffset) +
                       meshIndex;
        displaySize += _rwGCNDisplayListGetSize(
            descriptor, 1, mesh->numIndices);
    }
    vertexSize = _rwGCNVertexBufferGetSize(descriptor, vertexData.counts);
    totalSize = displaySize + vertexSize;
    if ((geometry->flags & 0x02000000) != 0) {
        entry = RwEngineInstance->fpMalloc(
            totalSize + sizeof(*entry), 0x30116);
        entry->link.next = NULL;
        entry->link.prev = NULL;
        entry->owner = owner;
        entry->size = totalSize;
        entry->ownerRef = ownerRef;
        entry->destroyNotify = _rxGCResEntryWaitDone;
        *ownerRef = entry;
    } else {
        entry = RwResourcesAllocateResEntry(
            owner, ownerRef, totalSize, _rxGCResEntryWaitDone);
    }
    vertexBuffer = (RwGameCubeVertexBuffer*)(entry + 1);
    memset(vertexBuffer, 0, totalSize);
    ((RwUInt16*)vertexBuffer)[0] = _RwDlTokenLastSeen;
    ((RwUInt16*)vertexBuffer)[1] = geometry->meshHeader->serialNum;
    vertexBuffer->reserved_0x00[1] = 0;
    if ((geometry->flags & 8) != 0) {
        RpGameCubeVtxFmt* format = *(RpGameCubeVtxFmt**)(
            (RwUInt8*)geometry + _rpDlGeomVtxFmtOffset);
        if (format == NULL) {
            RwInt32 vertex;
            vertexBuffer->reserved_0x00[1] &= ~1U;
            for (vertex = 0; vertex < geometry->numVertices; vertex++) {
                if (((const RwRGBA*)geometry->preLitLum)[vertex].alpha < 0xFF) {
                    vertexBuffer->reserved_0x00[1] |= 1;
                    break;
                }
            }
        } else if (format->colorType > 2) {
            vertexBuffer->reserved_0x00[1] |= 1;
        } else {
            vertexBuffer->reserved_0x00[1] &= ~1U;
        }
    }
    displayLists = (RwGameCubeDisplayList*)((RwUInt8*)vertexBuffer +
                                           headerSize);
    dataOffset = (RwUInt32)((RwUInt8*)displayLists + displayArraySize + 0x1F) &
                 ~0x1FU;

    for (meshIndex = 0; meshIndex < geometry->meshHeader->numMeshes;
         meshIndex++) {
        RpMesh* mesh = (RpMesh*)((RwUInt8*)geometry->meshHeader +
                                geometry->meshHeader->firstMeshOffset) +
                       meshIndex;
        RwUInt32 listSize = _rwGCNDisplayListGetSize(
            descriptor, 1, mesh->numIndices);
        RwUInt16* matrixIndices;
        RwUInt32 stride;

        _rwGCNDisplayListInitialize(&displayLists[meshIndex], meshIndex,
                                    listSize, (void*)dataOffset);
        matrixIndices = CreateMatrixIndexList(skin, mesh, meshIndex);
        IndexDataSetupFast(&indexData, mesh->indices, matrixIndices, geometry);
        stride = _rwGCNDisplayListGetStride(descriptor);
        _rwGCNDisplayListFill(descriptor, &displayLists[meshIndex],
                              &indexData, mesh->numIndices, FALSE, stride,
                              TRUE, primitive, NULL);
        RwEngineInstance->fpFree(matrixIndices);
        dataOffset += listSize;
    }
    _rwGCNVertexBufferInitialize(descriptor, vertexBuffer,
                                 vertexData.counts, (void*)dataOffset);
    _rwGCNVertexBufferFill(
        descriptor, (const RwGameCubeVertexStreams*)vertexBuffer,
        &vertexData, FALSE, NULL);
    DCFlushRange(vertexBuffer, totalSize);
    GXInvalidateVtxCache();
    return entry;
}
