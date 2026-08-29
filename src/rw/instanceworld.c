#include "dolphin/gx.h"
#include "runtime/cstring.h"
#include "rw/rwengine.h"
#include "rw/gamecube.h"
#include "rw/dltoken.h"
#include "rw/geomcond.h"
#include "rw/rwresources.h"
#include "rw/rxpipeline.h"

extern void DCFlushRange(void* start, unsigned int length);

static RwGameCubeVertexDescriptor VtxDesc;

static int ReconditionVertexIndexData(RpWorld* world,
                                         RpWorldSector* sector,
                                         GeomCondMap** remappedVertices,
                                         unsigned short**** remappedIndices)
{

    GeomCondVertexData streams[13];
    GeomCondMap* maps;
    unsigned short** sourceIndices;
    unsigned int numStreams;
    int meshIndex;
    unsigned int streamIndex;
    unsigned short numMeshes;

    if (*(RpGameCubeVtxFmt**)((unsigned char*)world + _rpDlWorldVtxFmtOffset) ==
        0) {
        _rpGameCubeVtxFmtGetDefault();
    }

    numStreams = 0;
    streams[numStreams].data = sector->vertices;
    streams[numStreams].type = 8;
    streams[numStreams].dependencies[0] = -1;
    numStreams++;

    if ((world->flags & 0x10) != 0) {
        int vertex;
        for (vertex = 0; vertex < sector->numVertices; vertex++)
            sector->normals[vertex].pad = 0;
        streams[numStreams].data = sector->normals;
        streams[numStreams].type = 4;
        streams[numStreams].dependencies[0] = -1;
        numStreams++;
    }
    if ((world->flags & 8) != 0) {
        streams[numStreams].data = sector->preLitLum;
        streams[numStreams].type = 4;
        streams[numStreams].dependencies[0] = -1;
        numStreams++;
    }
    if ((world->flags & 0x84) != 0) {
        int texCoord;
        for (texCoord = 0; texCoord < world->numTexCoordSets; texCoord++) {
            streams[numStreams].data = sector->texCoords[texCoord];
            streams[numStreams].type = 7;
            streams[numStreams].dependencies[0] = -1;
            numStreams++;
        }
    }

    maps = VertexDataCreateMaps(streams, numStreams, sector->numVertices);
    if (maps == 0)
        return 0;

    *remappedVertices = VertexDataCreateRemapped(
        maps, streams, numStreams, sector->numVertices);
    if (*remappedVertices == 0) {
        RwEngineInstance->fpFree(maps);
        return 0;
    }

    numMeshes = sector->mesh->numMeshes;
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
        RpMesh* mesh = (RpMesh*)(sector->mesh + 1) + meshIndex;
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
        RpMesh* mesh = (RpMesh*)(sector->mesh + 1) + meshIndex;
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
    RpWorld* world, const GeomCondMap* streams)
{

    unsigned char streamIndex = 0;
    RpGameCubeVtxFmt* format =
        *(RpGameCubeVtxFmt**)((unsigned char*)world + _rpDlWorldVtxFmtOffset);
    int texCoord;

    if (format == 0)
        format = _rpGameCubeVtxFmtGetDefault();
    _rwVertexDescriptorInit(&VtxDesc);
    _rwGCNVertexDescSetVAT(&VtxDesc, 0);
    _rwGCNVertexDescSetElementAttr(
        &VtxDesc, 9, 1, format->positionType, format->positionFraction);
    _rwGCNVertexDescSetElementDesc(
        &VtxDesc, 9, streams[streamIndex].count >= 0xFF ? 3 : 2);
    streamIndex++;

    if ((world->flags & 0x10) != 0) {
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
    if ((world->flags & 8) != 0) {
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
    if ((world->flags & 0x84) != 0) {
        for (texCoord = 0; texCoord < world->numTexCoordSets; texCoord++) {
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
                                     const RpWorld* world)
{

    unsigned int streamIndex = 0;
    unsigned int flags = world->flags;
    unsigned char numTexCoordSets = (unsigned char)world->numTexCoordSets;
    unsigned int texCoord;

    if (*(RpGameCubeVtxFmt**)((unsigned char*)world + _rpDlWorldVtxFmtOffset) ==
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
                                    unsigned short* const* indices,
                                    const RpWorld* world)
{

    unsigned int streamIndex = 0;
    unsigned int flags = world->flags;
    unsigned char numTexCoordSets = (unsigned char)world->numTexCoordSets;
    unsigned int texCoord;

    if (*(RpGameCubeVtxFmt**)((unsigned char*)world + _rpDlWorldVtxFmtOffset) ==
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

RwResEntry* _rwDlWorldSectorInstanceOptimized(RpWorld* world,
                                              RpWorldSector* sector)
{

    GeomCondMap* remappedVertices;
    unsigned short*** remappedIndices;
    RwGameCubeVertexData vertexData;
    RwGameCubeIndexData indexData;
    const RwGameCubeVertexDescriptor* descriptor;
    RwGameCubeVertexBuffer* vertexBuffer;
    RwGameCubeDisplayList* displayLists;
    RpGameCubeVtxFmt* format;
    unsigned int headerSize;
    unsigned int displayArraySize;
    unsigned int vertexSize;
    unsigned int totalSize;
    RwResEntry* entry;
    unsigned int dataOffset;
    unsigned int meshIndex;
    unsigned char primitive;

    totalSize = 0;
    ReconditionVertexIndexData(world, sector, &remappedVertices,
                               &remappedIndices);
    descriptor = VtxDescInitOptimized(world, remappedVertices);
    VertexDataSetupOptimized(&vertexData, remappedVertices, world);
    headerSize = _rwGCNVertexBufferHeaderGetSize(descriptor);
    totalSize += headerSize;
    displayArraySize = sector->mesh->numMeshes *
                       sizeof(RwGameCubeDisplayList);
    totalSize += displayArraySize;
    totalSize += 0x1F;
    if ((world->flags & 1) != 0)
        primitive = 0x98;
    else
        primitive = 0x90;

    for (meshIndex = 0; meshIndex < sector->mesh->numMeshes;
         meshIndex++) {
        if ((world->flags & 1) != 0) {
            unsigned int numStrips;
            unsigned int stripIndices;
            _rwGCNTriStripGetStats(remappedIndices[meshIndex][0],
                                   ((RpMesh*)(sector->mesh + 1) +
                                    meshIndex)->numIndices,
                                   &numStrips,
                                   &stripIndices, 1);
            totalSize += _rwGCNDisplayListGetSize(
                descriptor, numStrips, stripIndices);
        } else {
            unsigned int numIndices =
                ((RpMesh*)(sector->mesh + 1) + meshIndex)
                    ->numIndices;
            totalSize += _rwGCNDisplayListGetSize(descriptor, 1, numIndices);
        }
    }
    vertexSize = _rwGCNVertexBufferGetSize(descriptor, vertexData.counts);
    totalSize += vertexSize;
    entry = RwEngineInstance->fpMalloc(totalSize + sizeof(*entry), 0x3050D);
    entry->link.next = 0;
    entry->link.prev = 0;
    entry->owner = sector;
    entry->size = totalSize;
    entry->ownerRef = &sector->repEntry;
    entry->destroyNotify = _rxGCResEntryWaitDone;
    sector->repEntry = entry;
    dataOffset = (unsigned int)(entry + 1);
    memset((void*)dataOffset, 0, totalSize);
    vertexBuffer = (RwGameCubeVertexBuffer*)dataOffset;
    vertexBuffer->displayListToken = _RwDlTokenLastSeen;
    vertexBuffer->meshSerialNum = sector->mesh->serialNum;
    vertexBuffer->flags = 0;
    if ((world->flags & 8) != 0) {
        format = *(RpGameCubeVtxFmt**)(
            (unsigned char*)world + _rpDlWorldVtxFmtOffset);
        if (format == 0) {
            int vertex;
            vertexBuffer->flags &= ~1U;
            for (vertex = 0; vertex < sector->numVertices; vertex++) {
                if (sector->preLitLum[vertex].alpha <
                    0xFF) {
                    vertexBuffer->flags |= 1;
                    break;
                }
            }
        } else if (format->colorType > 2) {
            vertexBuffer->flags |= 1;
        } else {
            vertexBuffer->flags &= ~1U;
        }
    }
    dataOffset += headerSize;
    displayLists = (RwGameCubeDisplayList*)dataOffset;
    dataOffset += displayArraySize;
    dataOffset += 0x1F;
    dataOffset &= ~0x1FU;

    for (meshIndex = 0; meshIndex < sector->mesh->numMeshes;
         meshIndex++) {
        RpMesh* mesh = (RpMesh*)(sector->mesh + 1) + meshIndex;
        unsigned int numStrips;
        unsigned int stripIndices;
        unsigned int listSize;
        unsigned int stride;
        int isStrip;

        if ((world->flags & 1) != 0) {
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
                                world);
        stride = _rwGCNDisplayListGetStride(descriptor);
        if ((world->flags & 1) != 0)
            isStrip = 1;
        else
            isStrip = 0;
        _rwGCNDisplayListFill(
            descriptor, &displayLists[meshIndex], &indexData,
            mesh->numIndices, isStrip, stride, 1, primitive,
            &sector->tightBoundingBox.inf);
        dataOffset += listSize;
    }

    _rwGCNVertexBufferInitialize(descriptor, vertexBuffer,
                                 vertexData.counts, (void*)dataOffset);
    _rwGCNVertexBufferFill(
        descriptor, vertexBuffer,
        &vertexData, 1, &sector->tightBoundingBox.inf);
    DCFlushRange(vertexBuffer, totalSize);
    GXInvalidateVtxCache();
    for (meshIndex = 0; meshIndex < sector->mesh->numMeshes;
         meshIndex++)
        RwEngineInstance->fpFree(remappedIndices[meshIndex]);
    RwEngineInstance->fpFree(remappedIndices);
    RwEngineInstance->fpFree(remappedVertices);
    return entry;
}

static const RwGameCubeVertexDescriptor* VtxDescInitFast(
    const RpWorld* world, const RpWorldSector* sector)
{

    unsigned char indexedCount = 0;
    unsigned int numVertices = sector->numVertices;
    RpGameCubeVtxFmt* format =
        *(RpGameCubeVtxFmt**)((unsigned char*)world + _rpDlWorldVtxFmtOffset);
    int texCoord;

    if (format == 0)
        format = _rpGameCubeVtxFmtGetDefault();
    _rwVertexDescriptorInit(&VtxDesc);
    _rwGCNVertexDescSetVAT(&VtxDesc, 0);
    _rwGCNVertexDescSetElementAttr(
        &VtxDesc, 9, 1, format->positionType, format->positionFraction);
    _rwGCNVertexDescSetElementDesc(
        &VtxDesc, 9, numVertices >= 0xFF ? 3 : 2);
    indexedCount++;
    if ((world->flags & 0x10) != 0) {
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
    if ((world->flags & 8) != 0) {
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
    if ((world->flags & 0x84) != 0) {
        for (texCoord = 0; texCoord < world->numTexCoordSets; texCoord++) {
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
                                const RpWorld* world,
                                const RpWorldSector* sector)
{

    int texCoord;

    if (*(RpGameCubeVtxFmt**)((unsigned char*)world + _rpDlWorldVtxFmtOffset) ==
        0) {
        _rpGameCubeVtxFmtGetDefault();
    }
    vertexData->counts[9] = sector->numVertices;
    vertexData->source[9] = sector->vertices;
    if ((world->flags & 0x10) != 0) {
        vertexData->counts[10] = sector->numVertices;
        vertexData->source[10] = sector->normals;
    }
    if ((world->flags & 8) != 0) {
        vertexData->counts[11] = sector->numVertices;
        vertexData->source[11] = sector->preLitLum;
    }
    if ((world->flags & 0x84) != 0) {
        for (texCoord = 0; texCoord < world->numTexCoordSets; texCoord++) {
            vertexData->counts[texCoord + 13] = sector->numVertices;
            vertexData->source[texCoord + 13] = sector->texCoords[texCoord];
        }
    }
}

static void IndexDataSetupFast(RwGameCubeIndexData* indexData,
                               unsigned short* primitiveIndices,
                               const RpWorld* world)
{

    int texCoord;

    if (*(RpGameCubeVtxFmt**)((unsigned char*)world + _rpDlWorldVtxFmtOffset) ==
        0) {
        _rpGameCubeVtxFmtGetDefault();
    }
    indexData->attributes[9] = primitiveIndices;
    if ((world->flags & 0x10) != 0)
        indexData->attributes[10] = primitiveIndices;
    if ((world->flags & 8) != 0)
        indexData->attributes[11] = primitiveIndices;
    if ((world->flags & 0x84) != 0) {
        for (texCoord = 0; texCoord < world->numTexCoordSets; texCoord++)
            indexData->attributes[texCoord + 13] = primitiveIndices;
    }
}

RwResEntry* _rwDlWorldSectorInstanceFast(RpWorld* world,
                                         RpWorldSector* sector,
                                         void* owner,
                                         RwResEntry** ownerRef)
{

    RwGameCubeVertexData vertexData;
    RwGameCubeIndexData indexData;
    const RwGameCubeVertexDescriptor* descriptor;
    RwGameCubeVertexBuffer* vertexBuffer;
    RwGameCubeDisplayList* displayLists;
    RpGameCubeVtxFmt* format;
    unsigned int headerSize;
    unsigned int displayArraySize;
    unsigned int vertexSize;
    unsigned int totalSize;
    RwResEntry* entry;
    unsigned int dataOffset;
    unsigned int meshIndex;
    unsigned char primitive;

    descriptor = VtxDescInitFast(world, sector);
    VertexDataFastSetup(&vertexData, world, sector);
    headerSize = _rwGCNVertexBufferHeaderGetSize(descriptor);
    displayArraySize = sector->mesh->numMeshes *
                       sizeof(RwGameCubeDisplayList);
    totalSize = headerSize + displayArraySize + 0x1F;
    if ((world->flags & 1) != 0)
        primitive = 0x98;
    else
        primitive = 0x90;
    for (meshIndex = 0; meshIndex < sector->mesh->numMeshes;
         meshIndex++) {
        unsigned int numIndices =
            ((RpMesh*)(sector->mesh + 1) + meshIndex)->numIndices;
        totalSize += _rwGCNDisplayListGetSize(descriptor, 1, numIndices);
    }
    vertexSize = _rwGCNVertexBufferGetSize(descriptor, vertexData.counts);
    totalSize += vertexSize;
    if ((world->flags & 0x02000000) != 0) {
        entry = RwEngineInstance->fpMalloc(totalSize + sizeof(*entry),
                                           0x3050D);
        entry->link.next = 0;
        entry->link.prev = 0;
        entry->owner = sector;
        entry->size = totalSize;
        entry->ownerRef = &sector->repEntry;
        entry->destroyNotify = _rxGCResEntryWaitDone;
        sector->repEntry = entry;
    } else {
        entry = RwResourcesAllocateResEntry(owner, ownerRef, totalSize,
                                            _rxGCResEntryWaitDone);
    }
    dataOffset = (unsigned int)(entry + 1);
    memset((void*)dataOffset, 0, totalSize);
    vertexBuffer = (RwGameCubeVertexBuffer*)dataOffset;
    vertexBuffer->displayListToken = _RwDlTokenLastSeen;
    vertexBuffer->meshSerialNum = sector->mesh->serialNum;
    vertexBuffer->flags = 0;
    if ((world->flags & 8) != 0) {
        format = *(RpGameCubeVtxFmt**)(
            (unsigned char*)world + _rpDlWorldVtxFmtOffset);
        if (format == 0) {
            int vertex;
            vertexBuffer->flags &= ~1U;
            for (vertex = 0; vertex < sector->numVertices; vertex++) {
                if (sector->preLitLum[vertex].alpha <
                    0xFF) {
                    vertexBuffer->flags |= 1;
                    break;
                }
            }
        } else if (format->colorType > 2) {
            vertexBuffer->flags |= 1;
        } else {
            vertexBuffer->flags &= ~1U;
        }
    }
    dataOffset += headerSize;
    displayLists = (RwGameCubeDisplayList*)dataOffset;
    dataOffset += displayArraySize;
    dataOffset += 0x1F;
    dataOffset &= ~0x1FU;

    for (meshIndex = 0; meshIndex < sector->mesh->numMeshes;
         meshIndex++) {
        RpMesh* mesh = (RpMesh*)(sector->mesh + 1) + meshIndex;
        unsigned int listSize = _rwGCNDisplayListGetSize(
            descriptor, 1, mesh->numIndices);
        unsigned int stride;

        _rwGCNDisplayListInitialize(&displayLists[meshIndex], meshIndex,
                                    listSize, (void*)dataOffset);
        IndexDataSetupFast(&indexData, mesh->indices, world);
        stride = _rwGCNDisplayListGetStride(descriptor);
        _rwGCNDisplayListFill(
            descriptor, &displayLists[meshIndex], &indexData,
            mesh->numIndices, 0, stride, 1, primitive,
            &sector->tightBoundingBox.inf);
        dataOffset += listSize;
    }
    _rwGCNVertexBufferInitialize(descriptor, vertexBuffer,
                                 vertexData.counts, (void*)dataOffset);
    _rwGCNVertexBufferFill(
        descriptor, vertexBuffer,
        &vertexData, 1, &sector->tightBoundingBox.inf);
    DCFlushRange(vertexBuffer, totalSize);
    GXInvalidateVtxCache();
    return entry;
}
