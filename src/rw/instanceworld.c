#include "libmkparticle/rw_engine.h"
#include "rw/gamecube.h"
#include "rw/geomcond.h"
#include "rw/rwresources.h"
#include "rw/rxpipeline.h"

extern void* memset(void* destination, RwInt32 value, RwUInt32 size);
extern void DCFlushRange(void* start, RwUInt32 length);
extern void GXInvalidateVtxCache(void);

extern RwInt32 _rpDlWorldVtxFmtOffset;
extern RwUInt16 _RwDlTokenLastSeen;

static RwGameCubeVertexDescriptor VtxDesc;

static RwBool ReconditionVertexIndexData(RpWorld* world,
                                         RpWorldSector* sector,
                                         GeomCondMap** remappedVertices,
                                         RwUInt16**** remappedIndices)
{
    /* The conditioning, dependency map, allocation, and retail failure leaks
     * are recovered. Retail additionally retains the resolved format value
     * after its NULL check, widening its stack/save shape. */
    GeomCondVertexData streams[13];
    GeomCondMap* maps;
    RwUInt16** sourceIndices;
    RwUInt32 numStreams;
    RwInt32 meshIndex;
    RwUInt32 streamIndex;
    RwUInt16 numMeshes;

    if (*(RpGameCubeVtxFmt**)((RwUInt8*)world + _rpDlWorldVtxFmtOffset) ==
        NULL) {
        _rpGameCubeVtxFmtGetDefault();
    }

    numStreams = 0;
    streams[numStreams].data = sector->vertices;
    streams[numStreams].type = 8;
    streams[numStreams].dependencies[0] = -1;
    numStreams++;

    if ((world->flags & 0x10) != 0) {
        RwInt32 vertex;
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
        RwInt32 texCoord;
        for (texCoord = 0; texCoord < world->numTexCoordSets; texCoord++) {
            streams[numStreams].data = sector->texCoords[texCoord];
            streams[numStreams].type = 7;
            streams[numStreams].dependencies[0] = -1;
            numStreams++;
        }
    }

    maps = VertexDataCreateMaps(streams, numStreams, sector->numVertices);
    if (maps == NULL)
        return FALSE;

    *remappedVertices = VertexDataCreateRemapped(
        maps, streams, numStreams, sector->numVertices);
    if (*remappedVertices == NULL) {
        RwEngineInstance->fpFree(maps);
        return FALSE;
    }

    numMeshes = sector->mesh->numMeshes;
    sourceIndices = RwEngineInstance->fpMalloc(
        numStreams * numMeshes * sizeof(*sourceIndices), 0x1050D);
    if (sourceIndices == NULL) {
        RwEngineInstance->fpFree(*remappedVertices);
        RwEngineInstance->fpFree(maps);
        return FALSE;
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
    if (*remappedIndices == NULL) {
        RwEngineInstance->fpFree(sourceIndices);
        RwEngineInstance->fpFree(*remappedVertices);
        RwEngineInstance->fpFree(maps);
        return FALSE;
    }

    for (meshIndex = 0; meshIndex < numMeshes; meshIndex++) {
        RpMesh* mesh = (RpMesh*)(sector->mesh + 1) + meshIndex;
        (*remappedIndices)[meshIndex] = IndexDataCreateRemapped(
            maps, (const RwUInt16* const*)&sourceIndices[meshIndex *
                                                        numStreams],
            numStreams, mesh->numIndices);
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
    RpWorld* world, const GeomCondMap* streams)
{
    /* Attribute order, formats, index widths, and calls are exact. The
     * remaining diff is O0 conditional-result and register scheduling. */
    RwUInt8 streamIndex = 0;
    RpGameCubeVtxFmt* format =
        *(RpGameCubeVtxFmt**)((RwUInt8*)world + _rpDlWorldVtxFmtOffset);
    RwInt32 texCoord;

    if (format == NULL)
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
    /* Retail keeps the resolved format result after the NULL check even
     * though the subsequent stream stores do not consume it. */
    RwUInt32 streamIndex = 0;
    RwUInt32 flags = world->flags;
    RwUInt8 numTexCoordSets = (RwUInt8)world->numTexCoordSets;
    RwUInt32 texCoord;

    if (*(RpGameCubeVtxFmt**)((RwUInt8*)world + _rpDlWorldVtxFmtOffset) ==
        NULL) {
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
                                    const RpWorld* world)
{
    /* Stream construction is exact; retail's otherwise-unused resolved
     * format value accounts for the residual save/register shape. */
    RwUInt32 streamIndex = 0;
    RwUInt32 flags = world->flags;
    RwUInt8 numTexCoordSets = (RwUInt8)world->numTexCoordSets;
    RwUInt32 texCoord;

    if (*(RpGameCubeVtxFmt**)((RwUInt8*)world + _rpDlWorldVtxFmtOffset) ==
        NULL) {
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
    /* The conditioned data, display-list sizing/fill, resource ownership,
     * alpha flag, cache flush, and cleanup CFG are recovered. Residue is O0
     * stack-slot and nonvolatile-register allocation across the large body. */
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
            RwUInt32 numStrips;
            RwUInt32 stripIndices;
            _rwGCNTriStripGetStats(remappedIndices[meshIndex][0],
                                   ((RpMesh*)(sector->mesh + 1) +
                                    meshIndex)->numIndices,
                                   &numStrips,
                                   &stripIndices, TRUE);
            totalSize += _rwGCNDisplayListGetSize(
                descriptor, numStrips, stripIndices);
        } else {
            RwUInt32 numIndices =
                ((RpMesh*)(sector->mesh + 1) + meshIndex)
                    ->numIndices;
            totalSize += _rwGCNDisplayListGetSize(descriptor, 1, numIndices);
        }
    }
    vertexSize = _rwGCNVertexBufferGetSize(descriptor, vertexData.counts);
    totalSize += vertexSize;
    entry = RwEngineInstance->fpMalloc(totalSize + sizeof(*entry), 0x3050D);
    entry->link.next = NULL;
    entry->link.prev = NULL;
    entry->owner = sector;
    entry->size = totalSize;
    entry->ownerRef = &sector->repEntry;
    entry->destroyNotify = _rxGCResEntryWaitDone;
    sector->repEntry = entry;
    dataOffset = (RwUInt32)(entry + 1);
    memset((void*)dataOffset, 0, totalSize);
    vertexBuffer = (RwGameCubeVertexBuffer*)dataOffset;
    ((RwUInt16*)vertexBuffer)[0] = _RwDlTokenLastSeen;
    ((RwUInt16*)vertexBuffer)[1] = sector->mesh->serialNum;
    vertexBuffer->reserved_0x00[1] = 0;
    if ((world->flags & 8) != 0) {
        format = *(RpGameCubeVtxFmt**)(
            (RwUInt8*)world + _rpDlWorldVtxFmtOffset);
        if (format == NULL) {
            RwInt32 vertex;
            vertexBuffer->reserved_0x00[1] &= ~1U;
            for (vertex = 0; vertex < sector->numVertices; vertex++) {
                if (sector->preLitLum[vertex].alpha <
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

    for (meshIndex = 0; meshIndex < sector->mesh->numMeshes;
         meshIndex++) {
        RpMesh* mesh = (RpMesh*)(sector->mesh + 1) + meshIndex;
        RwUInt32 numStrips;
        RwUInt32 stripIndices;
        RwUInt32 listSize;
        RwUInt32 stride;
        RwBool isStrip;

        if ((world->flags & 1) != 0) {
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
        IndexDataSetupOptimized(&indexData, remappedIndices[meshIndex],
                                world);
        stride = _rwGCNDisplayListGetStride(descriptor);
        if ((world->flags & 1) != 0)
            isStrip = TRUE;
        else
            isStrip = FALSE;
        _rwGCNDisplayListFill(
            descriptor, &displayLists[meshIndex], &indexData,
            mesh->numIndices, isStrip, stride, TRUE, primitive,
            &sector->tightBoundingBox.inf);
        dataOffset += listSize;
    }

    _rwGCNVertexBufferInitialize(descriptor, vertexBuffer,
                                 vertexData.counts, (void*)dataOffset);
    _rwGCNVertexBufferFill(
        descriptor, (const RwGameCubeVertexStreams*)vertexBuffer,
        &vertexData, TRUE, &sector->tightBoundingBox.inf);
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
    /* Retail/current operations and function size agree; only the indexed
     * count and per-attribute temporary registers are permuted. */
    RwUInt8 indexedCount = 0;
    RwUInt32 numVertices = sector->numVertices;
    RpGameCubeVtxFmt* format =
        *(RpGameCubeVtxFmt**)((RwUInt8*)world + _rpDlWorldVtxFmtOffset);
    RwInt32 texCoord;

    if (format == NULL)
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
    /* Retail retains both the resolved format and an indexed-stream count
     * that no later operation consumes. Clean source preserves only the
     * functional vertex stream stores. */
    RwInt32 texCoord;

    if (*(RpGameCubeVtxFmt**)((RwUInt8*)world + _rpDlWorldVtxFmtOffset) ==
        NULL) {
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
                               RwUInt16* primitiveIndices,
                               const RpWorld* world)
{
    /* The index stores are exact. Retail keeps the resolved format solely as
     * an unused nonvolatile lifetime, selecting a wider save helper. */
    RwInt32 texCoord;

    if (*(RpGameCubeVtxFmt**)((RwUInt8*)world + _rpDlWorldVtxFmtOffset) ==
        NULL) {
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
    /* The allocator split, resource header, alpha scan, display lists,
     * vertex buffer, and hardware publication order are recovered. Remaining
     * differences are O0 stack-slot/register coloring across the same CFG. */
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
        RwUInt32 numIndices =
            ((RpMesh*)(sector->mesh + 1) + meshIndex)->numIndices;
        totalSize += _rwGCNDisplayListGetSize(descriptor, 1, numIndices);
    }
    vertexSize = _rwGCNVertexBufferGetSize(descriptor, vertexData.counts);
    totalSize += vertexSize;
    if ((world->flags & 0x02000000) != 0) {
        entry = RwEngineInstance->fpMalloc(totalSize + sizeof(*entry),
                                           0x3050D);
        entry->link.next = NULL;
        entry->link.prev = NULL;
        entry->owner = sector;
        entry->size = totalSize;
        entry->ownerRef = &sector->repEntry;
        entry->destroyNotify = _rxGCResEntryWaitDone;
        sector->repEntry = entry;
    } else {
        entry = RwResourcesAllocateResEntry(owner, ownerRef, totalSize,
                                            _rxGCResEntryWaitDone);
    }
    dataOffset = (RwUInt32)(entry + 1);
    memset((void*)dataOffset, 0, totalSize);
    vertexBuffer = (RwGameCubeVertexBuffer*)dataOffset;
    ((RwUInt16*)vertexBuffer)[0] = _RwDlTokenLastSeen;
    ((RwUInt16*)vertexBuffer)[1] = sector->mesh->serialNum;
    vertexBuffer->reserved_0x00[1] = 0;
    if ((world->flags & 8) != 0) {
        format = *(RpGameCubeVtxFmt**)(
            (RwUInt8*)world + _rpDlWorldVtxFmtOffset);
        if (format == NULL) {
            RwInt32 vertex;
            vertexBuffer->reserved_0x00[1] &= ~1U;
            for (vertex = 0; vertex < sector->numVertices; vertex++) {
                if (sector->preLitLum[vertex].alpha <
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

    for (meshIndex = 0; meshIndex < sector->mesh->numMeshes;
         meshIndex++) {
        RpMesh* mesh = (RpMesh*)(sector->mesh + 1) + meshIndex;
        RwUInt32 listSize = _rwGCNDisplayListGetSize(
            descriptor, 1, mesh->numIndices);
        RwUInt32 stride;

        _rwGCNDisplayListInitialize(&displayLists[meshIndex], meshIndex,
                                    listSize, (void*)dataOffset);
        IndexDataSetupFast(&indexData, mesh->indices, world);
        stride = _rwGCNDisplayListGetStride(descriptor);
        _rwGCNDisplayListFill(
            descriptor, &displayLists[meshIndex], &indexData,
            mesh->numIndices, FALSE, stride, TRUE, primitive,
            &sector->tightBoundingBox.inf);
        dataOffset += listSize;
    }
    _rwGCNVertexBufferInitialize(descriptor, vertexBuffer,
                                 vertexData.counts, (void*)dataOffset);
    _rwGCNVertexBufferFill(
        descriptor, (const RwGameCubeVertexStreams*)vertexBuffer,
        &vertexData, TRUE, &sector->tightBoundingBox.inf);
    DCFlushRange(vertexBuffer, totalSize);
    GXInvalidateVtxCache();
    return entry;
}
