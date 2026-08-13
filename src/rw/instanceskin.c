#include "libmkparticle/rw_engine.h"
#include "rw/gamecube.h"
#include "rw/dltoken.h"
#include "rw/geomcond.h"
#include "rw/rpskin.h"
#include "rw/rwresources.h"
#include "rw/rxpipeline.h"

extern void* memset(void* destination, int value, unsigned int size);
extern void* memcpy(void* destination, const void* source, unsigned int size);
extern void DCFlushRange(void* start, unsigned int length);
extern void GXInvalidateVtxCache(void);


static RwGameCubeVertexDescriptor VtxDesc;

static int ReconditionVertexIndexData(RpGeometry* geometry,
                                         GeomCondMap** remappedVertices,
                                         unsigned short**** remappedIndices)
{

    GeomCondVertexData streams[13];
    RpMorphTarget* morphTarget;
    RpSkin* skin;
    GeomCondMap* maps;
    unsigned short** sourceIndices;
    unsigned int numStreams;
    int meshIndex;
    unsigned int streamIndex;
    unsigned short numMeshes;

    if (*(RpGameCubeVtxFmt**)((unsigned char*)geometry + _rpDlGeomVtxFmtOffset) ==
        0) {
        _rpGameCubeVtxFmtGetDefault();
    }

    morphTarget = geometry->morphTarget;
    numStreams = 0;
    streams[numStreams].data = morphTarget->verts;
    streams[numStreams].type = 8;
    streams[numStreams].dependencies[0] = -1;
    numStreams++;

    if ((geometry->flags & 0x10) != 0) {
        streams[numStreams].data = morphTarget->normals;
        streams[numStreams].type = 8;
        streams[numStreams].dependencies[0] = -1;
        numStreams++;
    }
    if ((geometry->flags & 8) != 0) {
        streams[numStreams].data = geometry->preLitLum;
        streams[numStreams].type = 4;
        streams[numStreams].dependencies[0] = -1;
        numStreams++;
    }
    if ((geometry->flags & 0x84) != 0) {
        int texCoord;
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
    numStreams++;

    if (skin->maxNumWeights > 1) {
        streams[numStreams].data = skin->platformWeights;
        streams[numStreams].type = streams[numStreams - 1].type;
        streams[numStreams].dependencies[0] = -1;
        numStreams++;
    } else {
        streams[numStreams].data = skin->vertexBoneWeights;
        streams[numStreams].type = 9;
        streams[numStreams].dependencies[0] = -1;
        numStreams++;
    }
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
    if (maps == 0)
        return 0;

    *remappedVertices = VertexDataCreateRemapped(
        maps, streams, numStreams, geometry->numVertices);
    if (*remappedVertices == 0) {
        RwEngineInstance->fpFree(maps);
        return 0;
    }

    numStreams -= 2;
    numMeshes = geometry->meshHeader->numMeshes;
    sourceIndices = RwEngineInstance->fpMalloc(
        numStreams * numMeshes * sizeof(*sourceIndices),
        0x10116);
    if (sourceIndices == 0) {
        RwEngineInstance->fpFree(*remappedVertices);
        RwEngineInstance->fpFree(maps);
        return 0;
    }
    memset(sourceIndices, 0,
           numStreams * numMeshes * sizeof(*sourceIndices));
    for (meshIndex = 0; meshIndex < numMeshes; meshIndex++) {
        RpMesh* mesh = (RpMesh*)(geometry->meshHeader + 1) + meshIndex;
        for (streamIndex = 0; streamIndex < numStreams; streamIndex++)
            sourceIndices[meshIndex * numStreams + streamIndex] =
                mesh->indices;
    }

    *remappedIndices = RwEngineInstance->fpMalloc(
        numMeshes * sizeof(**remappedIndices), 0x30116);
    if (*remappedIndices == 0) {
        RwEngineInstance->fpFree(sourceIndices);
        RwEngineInstance->fpFree(*remappedVertices);
        RwEngineInstance->fpFree(maps);
        return 0;
    }

    for (meshIndex = 0; meshIndex < numMeshes; meshIndex++) {
        RpMesh* mesh = (RpMesh*)(geometry->meshHeader + 1) + meshIndex;
        (*remappedIndices)[meshIndex] = IndexDataCreateRemapped(
            maps, (const unsigned short* const*)&sourceIndices[meshIndex *
                                                        numStreams],
            numStreams, mesh->numIndices);
        if ((*remappedIndices)[meshIndex] == 0) {
            while (--meshIndex > 0)
                RwEngineInstance->fpFree((*remappedIndices)[meshIndex]);
            return 0;
        }
    }

    RwEngineInstance->fpFree(sourceIndices);
    RwEngineInstance->fpFree(maps);
    return 1;
}

static const RwGameCubeVertexDescriptor* VtxDescInitOptimized(
    RpGeometry* geometry, const GeomCondMap* streams)
{

    unsigned int streamIndex = 0;
    RpSkin* skin = RpSkinGeometryGetSkin(geometry);
    RpGameCubeVtxFmt* format =
        *(RpGameCubeVtxFmt**)((unsigned char*)geometry + _rpDlGeomVtxFmtOffset);
    int texCoord;

    if (format == 0)
        format = _rpGameCubeVtxFmtGetDefault();
    _rwVertexDescriptorInit(&VtxDesc);
    _rwGCNVertexDescSetVAT(&VtxDesc, 0);
    if (skin->maxNumWeights == 1)
        _rwGCNVertexDescSetElementDesc(&VtxDesc, 0, 1);
    _rwGCNVertexDescSetElementAttr(
        &VtxDesc, 9, 1, format->positionType, format->positionFraction);
    _rwGCNVertexDescSetElementDesc(
        &VtxDesc, 9, streams[streamIndex].count >= 0xFF ? 3 : 2);
    streamIndex++;

    if ((geometry->flags & 0x10) != 0) {
        if (format->normalMode != 0) {
            _rwGCNVertexDescSetElementAttr(
                &VtxDesc, 0x19, 1, format->normalType, 0);
            _rwGCNVertexDescSetElementDesc(
                &VtxDesc, 0x19,
                streams[streamIndex].count >= 0xFF ? 3 : 2);
        } else {
            _rwGCNVertexDescSetElementAttr(
                &VtxDesc, 0xA, 0, format->normalType, 0);
            _rwGCNVertexDescSetElementDesc(
                &VtxDesc, 0xA,
                streams[streamIndex].count >= 0xFF ? 3 : 2);
        }
        streamIndex++;
    }
    if ((geometry->flags & 8) != 0) {
        int componentCount;
        if (format->colorType > 2)
            componentCount = 1;
        else
            componentCount = 0;
        _rwGCNVertexDescSetElementAttr(
            &VtxDesc, 0xB, componentCount, format->colorType, 0);
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

    unsigned int streamIndex = 0;
    unsigned int flags = geometry->flags;
    unsigned char numTexCoordSets = (unsigned char)geometry->numTexCoordSets;
    unsigned int texCoord;

    if (*(RpGameCubeVtxFmt**)((unsigned char*)geometry + _rpDlGeomVtxFmtOffset) ==
        0) {
        _rpGameCubeVtxFmtGetDefault();
    }
    vertexData->counts[9] = streams[streamIndex].count;
    vertexData->source[9] = streams[streamIndex].data;
    streamIndex++;
    if ((flags & 0x10) != 0) {
        vertexData->counts[10] = streams[streamIndex].count;
        vertexData->source[10] = streams[streamIndex].data;
        streamIndex++;
    }
    if ((flags & 8) != 0) {
        vertexData->counts[11] = streams[streamIndex].count;
        vertexData->source[11] = streams[streamIndex].data;
        streamIndex++;
    }
    if ((flags & 0x84) != 0) {
        for (texCoord = 0; texCoord < numTexCoordSets; texCoord++) {
            vertexData->counts[texCoord + 13] = streams[streamIndex].count;
            vertexData->source[texCoord + 13] = streams[streamIndex].data;
            streamIndex++;
        }
    }
}

static void IndexDataSetupOptimized(RwGameCubeIndexData* indexData,
                                    unsigned short* const* indices,
                                    const RpGeometry* geometry,
                                    unsigned short* matrixIndices)
{

    unsigned int streamIndex = 0;
    unsigned int flags = geometry->flags;
    unsigned char numTexCoordSets = (unsigned char)geometry->numTexCoordSets;
    unsigned int texCoord;

    if (*(RpGameCubeVtxFmt**)((unsigned char*)geometry + _rpDlGeomVtxFmtOffset) ==
        0) {
        _rpGameCubeVtxFmtGetDefault();
    }
    if (matrixIndices != 0)
        indexData->attributes[0] = matrixIndices;
    indexData->attributes[9] = indices[streamIndex];
    streamIndex++;
    if ((flags & 0x10) != 0) {
        indexData->attributes[10] = indices[streamIndex];
        streamIndex++;
    }
    if ((flags & 8) != 0) {
        indexData->attributes[11] = indices[streamIndex];
        streamIndex++;
    }
    if ((flags & 0x84) != 0) {
        for (texCoord = 0; texCoord < numTexCoordSets; texCoord++) {
            indexData->attributes[texCoord + 13] = indices[streamIndex];
            streamIndex++;
        }
    }
}

static unsigned short* CreateMatrixIndexListOptimized(
    const RpSkin* skin, const unsigned short* primitiveIndices,
    const unsigned int* boneIndices, int numIndices, unsigned int meshIndex)
{

    unsigned char meshBones[10];
    unsigned short* matrixIndices;
    int index;

    if (skin->splitData.numMeshes != 0) {
        const RpSkinRLECount* count = &skin->splitData.rleCount[meshIndex];
        unsigned char* current = meshBones;
        unsigned int run;
        for (run = 0; run < count->size; run++) {
            const RpSkinRLE* rle =
                &skin->splitData.rle[count->start + run];
            unsigned char bone;
            for (bone = 0; bone < rle->count; bone++)
                *current++ = rle->startBone + bone;
        }
        matrixIndices = RwEngineInstance->fpMalloc(
            numIndices * sizeof(*matrixIndices), 0x30116);
        for (index = 0; index < numIndices; index++) {
            unsigned int matrixIndex;
            for (matrixIndex = 0; matrixIndex < 10; matrixIndex++) {
                if (meshBones[matrixIndex] ==
                    (unsigned char)boneIndices[primitiveIndices[index]]) {
                    matrixIndices[index] = (unsigned short)(matrixIndex * 3);
                    break;
                }
            }
        }
    } else {
        matrixIndices = RwEngineInstance->fpMalloc(
            numIndices * sizeof(*matrixIndices), 0x30116);
        for (index = 0; index < numIndices; index++) {
            unsigned int matrixIndex;
            for (matrixIndex = 0; matrixIndex < skin->numUsedBones;
                 matrixIndex++) {
                if (skin->usedBoneList[matrixIndex] ==
                    (unsigned char)boneIndices[primitiveIndices[index]]) {
                    matrixIndices[index] = (unsigned short)(matrixIndex * 3);
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

    unsigned char indexedCount = 0;
    unsigned int numVertices = (unsigned int)geometry->numVertices;
    RpGameCubeVtxFmt* format =
        *(RpGameCubeVtxFmt**)((unsigned char*)geometry + _rpDlGeomVtxFmtOffset);
    int texCoord;

    if (format == 0)
        format = _rpGameCubeVtxFmtGetDefault();
    _rwVertexDescriptorInit(&VtxDesc);
    _rwGCNVertexDescSetVAT(&VtxDesc, 0);
    _rwGCNVertexDescSetElementDesc(&VtxDesc, 0, 1);
    _rwGCNVertexDescSetElementAttr(
        &VtxDesc, 9, 1, format->positionType, format->positionFraction);
    _rwGCNVertexDescSetElementDesc(
        &VtxDesc, 9, numVertices >= 0xFF ? 3 : 2);
    indexedCount++;
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
        int componentCount;
        if (format->colorType > 2)
            componentCount = 1;
        else
            componentCount = 0;
        _rwGCNVertexDescSetElementAttr(
            &VtxDesc, 0xB, componentCount, format->colorType, 0);
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

    const RpMorphTarget* morphTarget;
    int texCoord;

    if (*(RpGameCubeVtxFmt**)((unsigned char*)geometry + _rpDlGeomVtxFmtOffset) ==
        0) {
        _rpGameCubeVtxFmtGetDefault();
    }
    morphTarget = geometry->morphTarget;
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
                               unsigned short* primitiveIndices,
                               unsigned short* matrixIndices,
                               const RpGeometry* geometry)
{

    int texCoord;

    if (*(RpGameCubeVtxFmt**)((unsigned char*)geometry + _rpDlGeomVtxFmtOffset) ==
        0) {
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

static unsigned short* CreateMatrixIndexList(const RpSkin* skin,
                                       const RpMesh* mesh,
                                       unsigned int meshIndex)
{

    unsigned char meshBones[10];
    unsigned short* matrixIndices;
    unsigned int index;

    if (skin->splitData.numMeshes != 0) {
        const RpSkinRLECount* count = &skin->splitData.rleCount[meshIndex];
        unsigned char* current = meshBones;
        unsigned int run;
        for (run = 0; run < count->size; run++) {
            const RpSkinRLE* rle =
                &skin->splitData.rle[count->start + run];
            unsigned char bone;
            for (bone = 0; bone < rle->count; bone++)
                *current++ = rle->startBone + bone;
        }
        matrixIndices = RwEngineInstance->fpMalloc(
            mesh->numIndices * sizeof(*matrixIndices), 0x30116);
        for (index = 0; index < mesh->numIndices; index++) {
            unsigned int matrixIndex;
            for (matrixIndex = 0; matrixIndex < 10; matrixIndex++) {
                if (meshBones[matrixIndex] ==
                    (unsigned char)skin->vertexBoneIndices[mesh->indices[index]]) {
                    matrixIndices[index] = (unsigned short)(matrixIndex * 3);
                    break;
                }
            }
        }
    } else {
        matrixIndices = RwEngineInstance->fpMalloc(
            mesh->numIndices * sizeof(*matrixIndices), 0x30116);
        for (index = 0; index < mesh->numIndices; index++) {
            unsigned int matrixIndex;
            for (matrixIndex = 0; matrixIndex < skin->numUsedBones;
                 matrixIndex++) {
                if (skin->usedBoneList[matrixIndex] ==
                    (unsigned char)skin->vertexBoneIndices[mesh->indices[index]]) {
                    matrixIndices[index] = (unsigned short)(matrixIndex * 3);
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
    unsigned short*** remappedIndices;
    RwGameCubeVertexData vertexData;
    RwGameCubeIndexData indexData;
    const RwGameCubeVertexDescriptor* descriptor;
    RwResEntry* entry;
    RwGameCubeVertexBuffer* vertexBuffer;
    RwGameCubeDisplayList* displayLists;
    RpGameCubeVtxFmt* format;
    RpSkin* skin;
    unsigned int headerSize;
    unsigned int displayArraySize;
    unsigned int vertexSize;
    unsigned int totalSize;
    unsigned int dataOffset;
    unsigned int meshIndex;
    unsigned char primitive;

    totalSize = 0;
    ReconditionVertexIndexData(geometry, &remappedVertices,
                               &remappedIndices);
    descriptor = VtxDescInitOptimized(geometry, remappedVertices);
    VertexDataSetupOptimized(&vertexData, remappedVertices, geometry);
    headerSize = _rwGCNVertexBufferHeaderGetSize(descriptor);
    totalSize += headerSize;
    displayArraySize = geometry->meshHeader->numMeshes *
                       sizeof(RwGameCubeDisplayList);
    totalSize += displayArraySize;
    totalSize += 0x1F;
    if ((geometry->flags & 1) != 0)
        primitive = 0x98;
    else
        primitive = 0x90;

    for (meshIndex = 0; meshIndex < geometry->meshHeader->numMeshes;
         meshIndex++) {
        RpMesh* mesh = (RpMesh*)(geometry->meshHeader + 1) + meshIndex;
        if ((geometry->flags & 1) != 0) {
            unsigned int numStrips;
            unsigned int stripIndices;
            _rwGCNTriStripGetStats(remappedIndices[meshIndex][0],
                                   mesh->numIndices, &numStrips,
                                   &stripIndices, 1);
            totalSize += _rwGCNDisplayListGetSize(
                descriptor, numStrips, stripIndices);
        } else {
            unsigned int numIndices = mesh->numIndices;
            totalSize += _rwGCNDisplayListGetSize(
                descriptor, 1, numIndices);
        }
    }
    vertexSize = _rwGCNVertexBufferGetSize(descriptor, vertexData.counts);
    totalSize += vertexSize;
    entry = RwEngineInstance->fpMalloc(
        totalSize + sizeof(*entry), 0x30116);
    entry->link.next = 0;
    entry->link.prev = 0;
    entry->owner = owner;
    entry->size = totalSize;
    entry->ownerRef = ownerRef;
    entry->destroyNotify = _rxGCResEntryWaitDone;
    *ownerRef = entry;
    dataOffset = (unsigned int)(entry + 1);
    memset((void*)dataOffset, 0, totalSize);
    vertexBuffer = (RwGameCubeVertexBuffer*)dataOffset;
    ((unsigned short*)vertexBuffer)[0] = _RwDlTokenLastSeen;
    ((unsigned short*)vertexBuffer)[1] = geometry->meshHeader->serialNum;
    vertexBuffer->reserved_0x00[1] = 0;
    if ((geometry->flags & 8) != 0) {
        format = *(RpGameCubeVtxFmt**)(
            (unsigned char*)geometry + _rpDlGeomVtxFmtOffset);
        if (format == 0) {
            int vertex;
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
    dataOffset += headerSize;
    displayLists = (RwGameCubeDisplayList*)dataOffset;
    dataOffset += displayArraySize;
    dataOffset += 0x1F;
    dataOffset &= ~0x1FU;
    skin = RpSkinGeometryGetSkin(geometry);

    for (meshIndex = 0; meshIndex < geometry->meshHeader->numMeshes;
         meshIndex++) {
        RpMesh* mesh = (RpMesh*)(geometry->meshHeader + 1) + meshIndex;
        unsigned int numStrips;
        unsigned int stripIndices;
        unsigned int listSize;
        unsigned short* matrixIndices = 0;
        unsigned int stride;
        unsigned int attributeStreams;
        int isStrip;

        if ((geometry->flags & 1) != 0) {
            _rwGCNTriStripGetStats(remappedIndices[meshIndex][0],
                                   mesh->numIndices, &numStrips,
                                   &stripIndices, 1);
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
                                    0);
        } else {
            attributeStreams = 0;
            attributeStreams++;
            if ((geometry->flags & 0x10) != 0)
                attributeStreams = 2;
            if ((geometry->flags & 8) != 0)
                attributeStreams++;
            if ((geometry->flags & 0x84) != 0) {
                int texCoord;
                for (texCoord = 0; texCoord < geometry->numTexCoordSets;
                     texCoord++)
                    attributeStreams++;
            }
            matrixIndices = CreateMatrixIndexListOptimized(
                skin, remappedIndices[meshIndex][0],
                remappedVertices[attributeStreams].data,
                mesh->numIndices, meshIndex);
            IndexDataSetupOptimized(&indexData,
                                    remappedIndices[meshIndex], geometry,
                                    matrixIndices);
        }
        stride = _rwGCNDisplayListGetStride(descriptor);
        if ((geometry->flags & 1) != 0)
            isStrip = 1;
        else
            isStrip = 0;
        _rwGCNDisplayListFill(
            descriptor, &displayLists[meshIndex], &indexData,
            mesh->numIndices, isStrip, stride, 1, primitive, 0);
        if (matrixIndices != 0)
            RwEngineInstance->fpFree(matrixIndices);
        dataOffset += listSize;
    }

    _rwGCNVertexBufferInitialize(descriptor, vertexBuffer,
                                 vertexData.counts, (void*)dataOffset);
    _rwGCNVertexBufferFill(
        descriptor, (const RwGameCubeVertexStreams*)vertexBuffer,
        &vertexData, 0, 0);
    if (skin->maxNumWeights > 1) {
        unsigned int stream = 0;
        stream++;
        if ((geometry->flags & 0x10) != 0)
            stream = 2;
        if ((geometry->flags & 8) != 0)
            stream++;
        if ((geometry->flags & 0x84) != 0) {
            int texCoord;
            for (texCoord = 0; texCoord < geometry->numTexCoordSets;
                 texCoord++)
                stream++;
        }
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
    unsigned int headerSize;
    unsigned int displayArraySize;
    unsigned int vertexSize;
    unsigned int totalSize;
    unsigned int dataOffset;
    unsigned int meshIndex;
    unsigned int primitive;

    totalSize = 0;
    skin = RpSkinGeometryGetSkin(geometry);
    descriptor = VtxDescInitFast(geometry);
    VertexDataFastSetup(&vertexData, geometry);
    headerSize = _rwGCNVertexBufferHeaderGetSize(descriptor);
    totalSize += headerSize;
    displayArraySize = geometry->meshHeader->numMeshes *
                       sizeof(RwGameCubeDisplayList);
    totalSize += displayArraySize;
    totalSize += 0x1F;
    primitive = (geometry->flags & 1) != 0 ? 0x98 : 0x90;
    for (meshIndex = 0; meshIndex < geometry->meshHeader->numMeshes;
         meshIndex++) {
        unsigned int numIndices =
            ((RpMesh*)(geometry->meshHeader + 1) + meshIndex)->numIndices;
        totalSize += _rwGCNDisplayListGetSize(
            descriptor, 1, numIndices);
    }
    vertexSize = _rwGCNVertexBufferGetSize(descriptor, vertexData.counts);
    totalSize += vertexSize;
    if ((geometry->flags & 0x02000000) != 0) {
        entry = RwEngineInstance->fpMalloc(
            totalSize + sizeof(*entry), 0x30116);
        entry->link.next = 0;
        entry->link.prev = 0;
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
    ((unsigned short*)vertexBuffer)[0] = _RwDlTokenLastSeen;
    ((unsigned short*)vertexBuffer)[1] = geometry->meshHeader->serialNum;
    vertexBuffer->reserved_0x00[1] = 0;
    if ((geometry->flags & 8) != 0) {
        RpGameCubeVtxFmt* format = *(RpGameCubeVtxFmt**)(
            (unsigned char*)geometry + _rpDlGeomVtxFmtOffset);
        if (format == 0) {
            int vertex;
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
    displayLists = (RwGameCubeDisplayList*)((unsigned char*)vertexBuffer +
                                           headerSize);
    dataOffset = (unsigned int)displayLists;
    dataOffset += displayArraySize;
    dataOffset += 0x1F;
    dataOffset &= ~0x1FU;

    for (meshIndex = 0; meshIndex < geometry->meshHeader->numMeshes;
         meshIndex++) {
        RpMesh* mesh = (RpMesh*)(geometry->meshHeader + 1) + meshIndex;
        unsigned int listSize = _rwGCNDisplayListGetSize(
            descriptor, 1, mesh->numIndices);
        unsigned short* matrixIndices;
        unsigned int stride;

        _rwGCNDisplayListInitialize(&displayLists[meshIndex], meshIndex,
                                    listSize, (void*)dataOffset);
        matrixIndices = CreateMatrixIndexList(skin, mesh, meshIndex);
        IndexDataSetupFast(&indexData, mesh->indices, matrixIndices, geometry);
        stride = _rwGCNDisplayListGetStride(descriptor);
        _rwGCNDisplayListFill(descriptor, &displayLists[meshIndex],
                              &indexData, mesh->numIndices, 0, stride,
                              1, primitive, 0);
        RwEngineInstance->fpFree(matrixIndices);
        dataOffset += listSize;
    }
    _rwGCNVertexBufferInitialize(descriptor, vertexBuffer,
                                 vertexData.counts, (void*)dataOffset);
    _rwGCNVertexBufferFill(
        descriptor, (const RwGameCubeVertexStreams*)vertexBuffer,
        &vertexData, 0, 0);
    DCFlushRange(vertexBuffer, totalSize);
    GXInvalidateVtxCache();
    return entry;
}
