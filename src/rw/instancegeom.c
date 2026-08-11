#include "libmkparticle/rw_engine.h"
#include "rw/gamecube.h"
#include "rw/geomcond.h"
#include "rw/rwresources.h"
#include "rw/rxpipeline.h"

extern void* memset(void* destination, RwInt32 value, RwUInt32 size);
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
    RpMorphTarget* morphTarget;
    GeomCondMap* maps;
    RwUInt16** sourceIndices;
    RwUInt32 numStreams;
    RwInt32 meshIndex;
    RwUInt32 streamIndex;
    RwUInt16 numMeshes;

    if (*(RpGameCubeVtxFmt**)((RwUInt8*)geometry + _rpDlGeomVtxFmtOffset) ==
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
        RwInt32 texCoord;
        for (texCoord = 0; texCoord < geometry->numTexCoordSets; texCoord++) {
            streams[numStreams].data = geometry->texCoords[texCoord];
            streams[numStreams].type = 7;
            streams[numStreams].dependencies[0] = -1;
            numStreams++;
        }
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

    numMeshes = geometry->meshHeader->numMeshes;
    sourceIndices = RwEngineInstance->fpMalloc(
        numStreams * numMeshes * sizeof(*sourceIndices), 0x1050D);
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
        numMeshes * sizeof(**remappedIndices), 0x1050D);
    if (*remappedIndices == 0) {
        RwEngineInstance->fpFree(sourceIndices);
        RwEngineInstance->fpFree(*remappedVertices);
        RwEngineInstance->fpFree(maps);
        return 0;
    }

    for (meshIndex = 0; meshIndex < numMeshes; meshIndex++) {
        RpMesh* mesh = (RpMesh*)(geometry->meshHeader + 1) + meshIndex;
        (*remappedIndices)[meshIndex] = IndexDataCreateRemapped(
            maps, (const RwUInt16* const*)&sourceIndices[meshIndex *
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


    RwUInt8 streamIndex = 0;
    RpGameCubeVtxFmt* format =
        *(RpGameCubeVtxFmt**)((RwUInt8*)geometry + _rpDlGeomVtxFmtOffset);
    RwInt32 texCoord;

    if (format == 0)
        format = _rpGameCubeVtxFmtGetDefault();
    _rwVertexDescriptorInit(&VtxDesc);
    _rwGCNVertexDescSetVAT(&VtxDesc, 0);
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
        RwInt32 componentCount;
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


    RwUInt32 streamIndex = 0;
    RwUInt32 flags = geometry->flags;
    RwUInt8 numTexCoordSets = (RwUInt8)geometry->numTexCoordSets;
    RwUInt32 texCoord;

    if (*(RpGameCubeVtxFmt**)((RwUInt8*)geometry + _rpDlGeomVtxFmtOffset) ==
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
            vertexData->counts[texCoord + 13] =
                streams[streamIndex].count;
            vertexData->source[texCoord + 13] = streams[streamIndex].data;
            streamIndex++;
        }
    }
}

static void IndexDataSetupOptimized(RwGameCubeIndexData* indexData,
                                    RwUInt16* const* indices,
                                    const RpGeometry* geometry)
{


    RwUInt32 streamIndex = 0;
    RwUInt32 flags = geometry->flags;
    RwUInt8 numTexCoordSets = (RwUInt8)geometry->numTexCoordSets;
    RwUInt32 texCoord;

    if (*(RpGameCubeVtxFmt**)((RwUInt8*)geometry + _rpDlGeomVtxFmtOffset) ==
        0) {
        _rpGameCubeVtxFmtGetDefault();
    }
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

RwResEntry* _rwDlGeometryInstanceOptimized(RpGeometry* geometry,
                                           void* owner,
                                           RwResEntry** ownerRef)
{



    GeomCondMap* remappedVertices;
    RwUInt16*** remappedIndices;
    RwGameCubeVertexData vertexData;
    RwGameCubeIndexData indexData;
    const RwGameCubeVertexDescriptor* descriptor;
    RwGameCubeVertexBuffer* vertexBuffer;
    RwGameCubeDisplayList* displayLists;
    RpGameCubeVtxFmt* format;
    RwUInt32 headerSize;
    RwUInt32 displayArraySize;
    RwUInt32 vertexSize;
    RwUInt32 totalSize;
    RwResEntry* entry;
    RwUInt32 dataOffset;
    RwUInt32 meshIndex;
    RwUInt8 primitive;

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
        if ((geometry->flags & 1) != 0) {
            RwUInt32 numStrips;
            RwUInt32 stripIndices;
            _rwGCNTriStripGetStats(remappedIndices[meshIndex][0],
                                   ((RpMesh*)(geometry->meshHeader + 1) +
                                    meshIndex)->numIndices,
                                   &numStrips,
                                   &stripIndices, 1);
            totalSize += _rwGCNDisplayListGetSize(
                descriptor, numStrips, stripIndices);
        } else {
            RwUInt32 numIndices =
                ((RpMesh*)(geometry->meshHeader + 1) + meshIndex)
                    ->numIndices;
            totalSize += _rwGCNDisplayListGetSize(descriptor, 1, numIndices);
        }
    }
    vertexSize = _rwGCNVertexBufferGetSize(descriptor, vertexData.counts);
    totalSize += vertexSize;
    entry = RwEngineInstance->fpMalloc(totalSize + sizeof(*entry), 0x3050D);
    entry->link.next = 0;
    entry->link.prev = 0;
    entry->owner = owner;
    entry->size = totalSize;
    entry->ownerRef = ownerRef;
    entry->destroyNotify = _rxGCResEntryWaitDone;
    *ownerRef = entry;
    dataOffset = (RwUInt32)(entry + 1);
    memset((void*)dataOffset, 0, totalSize);
    vertexBuffer = (RwGameCubeVertexBuffer*)dataOffset;
    ((RwUInt16*)vertexBuffer)[0] = _RwDlTokenLastSeen;
    ((RwUInt16*)vertexBuffer)[1] = geometry->meshHeader->serialNum;
    vertexBuffer->reserved_0x00[1] = 0;
    if ((geometry->flags & 8) != 0) {
        format = *(RpGameCubeVtxFmt**)(
            (RwUInt8*)geometry + _rpDlGeomVtxFmtOffset);
        if (format == 0) {
            RwInt32 vertex;
            vertexBuffer->reserved_0x00[1] &= ~1U;
            for (vertex = 0; vertex < geometry->numVertices; vertex++) {
                if (((const RwRGBA*)geometry->preLitLum)[vertex].alpha <
                    0xFF) {
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

    for (meshIndex = 0; meshIndex < geometry->meshHeader->numMeshes;
         meshIndex++) {
        RpMesh* mesh = (RpMesh*)(geometry->meshHeader + 1) + meshIndex;
        RwUInt32 numStrips;
        RwUInt32 stripIndices;
        RwUInt32 listSize;
        RwUInt32 stride;
        RwBool isStrip;

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
        IndexDataSetupOptimized(&indexData, remappedIndices[meshIndex],
                                geometry);
        stride = _rwGCNDisplayListGetStride(descriptor);
        if ((geometry->flags & 1) != 0)
            isStrip = 1;
        else
            isStrip = 0;
        _rwGCNDisplayListFill(
            descriptor, &displayLists[meshIndex], &indexData,
            mesh->numIndices, isStrip, stride, 1, primitive, 0);
        dataOffset += listSize;
    }

    _rwGCNVertexBufferInitialize(descriptor, vertexBuffer,
                                 vertexData.counts, (void*)dataOffset);
    _rwGCNVertexBufferFill(
        descriptor, (const RwGameCubeVertexStreams*)vertexBuffer,
        &vertexData, 0, 0);
    DCFlushRange(vertexBuffer, totalSize);
    GXInvalidateVtxCache();
    for (meshIndex = 0; meshIndex < geometry->meshHeader->numMeshes;
         meshIndex++)
        RwEngineInstance->fpFree(remappedIndices[meshIndex]);
    RwEngineInstance->fpFree(remappedIndices);
    RwEngineInstance->fpFree(remappedVertices);
    return entry;
}

static const RwGameCubeVertexDescriptor* VtxDescInitFast(
    const RpGeometry* geometry)
{


    RwUInt8 indexedCount = 0;
    RwUInt32 numVertices = (RwUInt32)geometry->numVertices;
    RpGameCubeVtxFmt* format =
        *(RpGameCubeVtxFmt**)((RwUInt8*)geometry + _rpDlGeomVtxFmtOffset);
    RwInt32 texCoord;

    if (format == 0)
        format = _rpGameCubeVtxFmtGetDefault();
    _rwVertexDescriptorInit(&VtxDesc);
    _rwGCNVertexDescSetVAT(&VtxDesc, 0);
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
        RwInt32 componentCount;
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
    RwInt32 texCoord;

    if (*(RpGameCubeVtxFmt**)((RwUInt8*)geometry + _rpDlGeomVtxFmtOffset) ==
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
                               RwUInt16* primitiveIndices,
                               const RpGeometry* geometry)
{


    RwInt32 texCoord;

    if (*(RpGameCubeVtxFmt**)((RwUInt8*)geometry + _rpDlGeomVtxFmtOffset) ==
        0) {
        _rpGameCubeVtxFmtGetDefault();
    }
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

RwResEntry* _rwDlGeometryInstanceFast(RpGeometry* geometry, void* owner,
                                      RwResEntry** ownerRef)
{



    RwGameCubeVertexData vertexData;
    RwGameCubeIndexData indexData;
    const RwGameCubeVertexDescriptor* descriptor;
    RwGameCubeVertexBuffer* vertexBuffer;
    RwGameCubeDisplayList* displayLists;
    RpGameCubeVtxFmt* format;
    RwUInt32 headerSize;
    RwUInt32 displayArraySize;
    RwUInt32 vertexSize;
    RwUInt32 totalSize;
    RwResEntry* entry;
    RwUInt32 dataOffset;
    RwUInt32 meshIndex;
    RwUInt8 primitive;

    totalSize = 0;
    descriptor = VtxDescInitFast(geometry);
    VertexDataFastSetup(&vertexData, geometry);
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
        RwUInt32 numIndices =
            ((RpMesh*)(geometry->meshHeader + 1) + meshIndex)->numIndices;
        totalSize += _rwGCNDisplayListGetSize(descriptor, 1, numIndices);
    }
    vertexSize = _rwGCNVertexBufferGetSize(descriptor, vertexData.counts);
    totalSize += vertexSize;
    if ((geometry->flags & 0x02000000) != 0) {
        entry = RwEngineInstance->fpMalloc(totalSize + sizeof(*entry),
                                           0x3050D);
        entry->link.next = 0;
        entry->link.prev = 0;
        entry->owner = owner;
        entry->size = totalSize;
        entry->ownerRef = ownerRef;
        entry->destroyNotify = _rxGCResEntryWaitDone;
        *ownerRef = entry;
    } else {
        entry = RwResourcesAllocateResEntry(owner, ownerRef, totalSize,
                                            _rxGCResEntryWaitDone);
    }
    dataOffset = (RwUInt32)(entry + 1);
    memset((void*)dataOffset, 0, totalSize);
    vertexBuffer = (RwGameCubeVertexBuffer*)dataOffset;
    ((RwUInt16*)vertexBuffer)[0] = _RwDlTokenLastSeen;
    ((RwUInt16*)vertexBuffer)[1] = geometry->meshHeader->serialNum;
    vertexBuffer->reserved_0x00[1] = 0;
    if ((geometry->flags & 8) != 0) {
        format = *(RpGameCubeVtxFmt**)(
            (RwUInt8*)geometry + _rpDlGeomVtxFmtOffset);
        if (format == 0) {
            RwInt32 vertex;
            vertexBuffer->reserved_0x00[1] &= ~1U;
            for (vertex = 0; vertex < geometry->numVertices; vertex++) {
                if (((const RwRGBA*)geometry->preLitLum)[vertex].alpha <
                    0xFF) {
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

    for (meshIndex = 0; meshIndex < geometry->meshHeader->numMeshes;
         meshIndex++) {
        RpMesh* mesh = (RpMesh*)(geometry->meshHeader + 1) + meshIndex;
        RwUInt32 listSize = _rwGCNDisplayListGetSize(
            descriptor, 1, mesh->numIndices);
        RwUInt32 stride;

        _rwGCNDisplayListInitialize(&displayLists[meshIndex], meshIndex,
                                    listSize, (void*)dataOffset);
        IndexDataSetupFast(&indexData, mesh->indices, geometry);
        stride = _rwGCNDisplayListGetStride(descriptor);
        _rwGCNDisplayListFill(
            descriptor, &displayLists[meshIndex], &indexData,
            mesh->numIndices, 0, stride, 1, primitive, 0);
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
