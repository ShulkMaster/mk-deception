#include "dolphin/cache.h"
#include "libmkparticle/rw_engine.h"
#include "rw/rpworld_types.h"
#include "rw/rwerror.h"
#include "rw/rwresources.h"
#include "rw/rwstream.h"
#include "rw/rwstream_internal.h"

typedef struct RpGameCubeNativeMesh {
    void* data;
    RwUInt32 size;
} RpGameCubeNativeMesh;

typedef struct RpGameCubeNativeData {
    RwUInt16 token;
    RwUInt16 reserved_0x02;
    RwUInt32 reserved_0x04;
    RwUInt32 numMeshes;
    RpGameCubeNativeMesh meshes[1];
} RpGameCubeNativeData;

extern RwUInt16 _RwDlTokenCurrent;
extern void _rxGCResEntryWaitDone(RwResEntry* entry);
extern void GXInvalidateVtxCache(void);

static RpGameCubeNativeMesh* NativeMeshTable(RpGameCubeNativeData* native)
{
    return (RpGameCubeNativeMesh*)((RwUInt8*)native +
        (native->numMeshes - 1) * sizeof(RpGameCubeNativeMesh) + 0x14);
}

static RwUInt8* NativeVertexData(RpGameCubeNativeMesh* meshes,
                                 RwUInt32 count)
{
    return (RwUInt8*)(((RwUInt32)meshes +
        count * sizeof(RpGameCubeNativeMesh) + 0x1F) & ~0x1FU);
}

static void _rpNativePointer2Offset(RpGameCubeNativeData* native,
                                    RpGameCubeNativeMesh* meshes,
                                    RwUInt32 numMeshes)
{
    RwUInt8* vertexData = NativeVertexData(meshes, numMeshes);
    RwUInt32 i;

    for (i = 0; i < native->numMeshes; i++)
        native->meshes[i].data = (void*)(
            (RwUInt8*)native->meshes[i].data - vertexData);

    for (i = 0; i < numMeshes; i++)
        meshes[i].data = (void*)((RwUInt8*)meshes[i].data - vertexData);
}

static void _rpNativeOffset2Pointer(RpGameCubeNativeData* native,
                                    RpGameCubeNativeMesh* meshes,
                                    RwUInt32 numMeshes)
{
    RwUInt8* vertexData = NativeVertexData(meshes, numMeshes);
    RwUInt32 i;

    for (i = 0; i < native->numMeshes; i++)
        native->meshes[i].data = vertexData + (RwUInt32)native->meshes[i].data;

    for (i = 0; i < numMeshes; i++)
        meshes[i].data = vertexData + (RwUInt32)meshes[i].data;
}

static RwInt32 _rpNativeSize(const RwResEntry* entry, RwUInt32 numMeshes)
{
    RwInt32 size = 0;

    size = entry->size - 7;
    return size;
}

static void* _rpNativeRead(RwStream* stream, void* owner,
                           RwResEntry** entryRef, RwUInt32 numMeshes)
{
    RwUInt32 chunkLength;
    RwUInt32 version;
    RwInt32 platform;
    RwInt32 headerSize;
    RwInt32 dataSize;
    RwResEntry* entry;
    RpGameCubeNativeData* native;
    RpGameCubeNativeMesh* meshes;
    RwUInt8* vertexData;

    if (!RwStreamFindChunk(stream, 1, &chunkLength, &version)) return 0;
    if (version < 0x34000 || version > 0x36003) {
        RwError error;
        error.pluginID = 2;
        error.errorCode = _rwerror(0x80000004);
        RwErrorSet(&error);
        return 0;
    }
    if (version <= 0x34004) {
        RwError error;
        error.pluginID = 2;
        error.errorCode = _rwerror(0x80000004);
        RwErrorSet(&error);
        return 0;
    }
    if (!RwStreamReadInt32(stream, &platform, 4) || platform != 6)
        return 0;
    if (!RwStreamReadInt32(stream, &headerSize, 4)) return 0;
    if (!RwStreamReadInt32(stream, &dataSize, 4)) return 0;

    entry = RwEngineInstance->fpMalloc(headerSize + dataSize + 0x37,
                                       0x3050D);
    *entryRef = entry;
    native = (RpGameCubeNativeData*)(entry + 1);
    if (RwStreamRead(stream, native, headerSize) != (RwUInt32)headerSize)
        return 0;
    vertexData = (RwUInt8*)(((RwUInt32)native + headerSize + 0x1F) & ~0x1FU);
    if (RwStreamRead(stream, vertexData, dataSize) != (RwUInt32)dataSize)
        return 0;

    meshes = NativeMeshTable(native);
    _rpNativeOffset2Pointer(native, meshes, numMeshes);
    entry->link.next = 0;
    entry->link.prev = 0;
    entry->owner = owner;
    entry->size = chunkLength;
    entry->ownerRef = entryRef;
    entry->destroyNotify = _rxGCResEntryWaitDone;
    native->token = _RwDlTokenCurrent;
    DCFlushRange(native, entry->size);
    GXInvalidateVtxCache();
    return owner;
}

static RwStream* _rpNativeWrite(RwStream* stream, RwResEntry* entry,
                                RwUInt32 numMeshes)
{
    RwInt32 platform = 6;
    RpGameCubeNativeData* native =
        (RpGameCubeNativeData*)((RwUInt8*)entry + sizeof(RwResEntry));
    RpGameCubeNativeMesh* meshes = NativeMeshTable(native);
    RwInt32 chunkSize = _rpNativeSize(entry, numMeshes) - 12;
    RwInt32 headerSize;
    RwInt32 dataSize;

    if (!_rwStreamWriteVersionedChunkHeader(
            stream, 1, chunkSize, 0x36003, 0xFFFF))
        return 0;
    if (!RwStreamWriteInt32(stream, &platform, 4)) return 0;

    headerSize = (native->numMeshes - 1) * 8 + numMeshes * 8 + 0x14;
    if (!RwStreamWriteInt32(stream, &headerSize, 4)) return 0;
    dataSize = entry->size - headerSize - 0x1F;
    if (!RwStreamWriteInt32(stream, &dataSize, 4)) return 0;

    _rpNativePointer2Offset(native, meshes, numMeshes);
    if (!RwStreamWrite(stream, native, headerSize)) return 0;
    _rpNativeOffset2Pointer(native, meshes, numMeshes);
    if (!RwStreamWrite(stream, meshes->data, dataSize)) return 0;

    DCFlushRange(native, entry->size);
    GXInvalidateVtxCache();
    return stream;
}

RwInt32 _rpGeometryNativeSize(const RpGeometry* geometry)
{
    if ((geometry->flags & 0x01000000) && geometry->repEntry != 0)
        return _rpNativeSize(geometry->repEntry,
                             geometry->meshHeader->numMeshes);
    return 0;
}

RwStream* _rpGeometryNativeWrite(RwStream* stream,
                                 const RpGeometry* geometry)
{
    if (geometry->flags & 0x01000000)
        _rpNativeWrite(stream, geometry->repEntry,
                       geometry->meshHeader->numMeshes);
    return stream;
}

RpGeometry* _rpGeometryNativeRead(RwStream* stream, RpGeometry* geometry)
{
    return _rpNativeRead(stream, geometry, &geometry->repEntry,
                         geometry->meshHeader->numMeshes);
}

RwInt32 _rpWorldSectorNativeSize(const RpWorldSector* sector)
{
    if ((RpWorldSectorGetWorld(sector)->flags & 0x01000000) &&
        sector->repEntry != 0)
        return _rpNativeSize(sector->repEntry, sector->mesh->numMeshes);
    return 0;
}

RwStream* _rpWorldSectorNativeWrite(RwStream* stream,
                                    const RpWorldSector* sector)
{
    RpWorld* world = RpWorldSectorGetWorld(sector);
    if (world->flags & 0x01000000)
        _rpNativeWrite(stream, sector->repEntry, sector->mesh->numMeshes);
    return stream;
}

RpWorldSector* _rpWorldSectorNativeRead(RwStream* stream,
                                        RpWorldSector* sector)
{
    return _rpNativeRead(stream, sector, &sector->repEntry,
                         sector->mesh->numMeshes);
}
