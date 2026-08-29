#include "dolphin/cache.h"
#include "dolphin/gx.h"
#include "rw/rwengine.h"
#include "rw/gamecube.h"
#include "rw/dltoken.h"
#include "rw/native_internal.h"
#include "rw/rpworld_types.h"
#include "rw/rwerror.h"
#include "rw/rwresources.h"
#include "rw/rwstream.h"
#include "rw/rwstream_internal.h"

static void _rpNativePointer2Offset(GameCubeNativeMeshHeader* native,
                                    GameCubeNativeMesh* meshes,
                                    unsigned int numMeshes)
{
    unsigned char* vertexData = (unsigned char*)(((unsigned int)meshes +
        numMeshes * sizeof(GameCubeNativeMesh) + 0x1F) & ~0x1FU);
    unsigned int i;

    for (i = 0; i < native->numMeshes; i++)
        native->meshes[i].displayList.offset =
            (unsigned char*)native->meshes[i].displayList.pointer - vertexData;

    for (i = 0; i < numMeshes; i++)
        meshes[i].displayList.offset =
            (unsigned char*)meshes[i].displayList.pointer - vertexData;
}

static void _rpNativeOffset2Pointer(GameCubeNativeMeshHeader* native,
                                    GameCubeNativeMesh* meshes,
                                    unsigned int numMeshes)
{
    unsigned char* vertexData = (unsigned char*)(((unsigned int)meshes +
        numMeshes * sizeof(GameCubeNativeMesh) + 0x1F) & ~0x1FU);
    unsigned int i;

    for (i = 0; i < native->numMeshes; i++)
        native->meshes[i].displayList.pointer =
            vertexData + native->meshes[i].displayList.offset;

    for (i = 0; i < numMeshes; i++)
        meshes[i].displayList.pointer =
            vertexData + meshes[i].displayList.offset;
}

static int _rpNativeSize(const RwResEntry* entry, unsigned int numMeshes)
{
    int size = 0;
    const GameCubeNativeMeshHeader* native =
        (const GameCubeNativeMeshHeader*)(entry + 1);
    const GameCubeNativeMesh* meshes =
        &native->meshes[native->numMeshes - 1] + 1;

    size = entry->size - 7;
    return size;
}

static void* _rpNativeRead(RwStream* stream, void* owner,
                           RwResEntry** entryRef, unsigned int numMeshes)
{
    unsigned int version;
    unsigned int chunkLength;
    int headerSize;
    int dataSize;
    int platform;
    GameCubeNativeMeshHeader* native;
    GameCubeNativeMesh* meshes;
    unsigned char* vertexData;

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
    if (!RwStreamReadInt32(stream, &platform, 4)) return 0;
    if (platform != 6) return 0;
    if (!RwStreamReadInt32(stream, &headerSize, 4)) return 0;
    if (!RwStreamReadInt32(stream, &dataSize, 4)) return 0;

    *entryRef = RwEngineInstance->fpMalloc(headerSize + dataSize + 0x37,
                                           0x3050D);
    native = (GameCubeNativeMeshHeader*)(*entryRef + 1);
    if (RwStreamRead(stream, native, headerSize) != (unsigned int)headerSize)
        return 0;
    vertexData = (unsigned char*)(((unsigned int)native + headerSize + 0x1F) & ~0x1FU);
    if (RwStreamRead(stream, vertexData, dataSize) != (unsigned int)dataSize)
        return 0;

    meshes = &native->meshes[native->numMeshes - 1] + 1;
    _rpNativeOffset2Pointer(native, meshes, numMeshes);
    (*entryRef)->link.next = 0;
    (*entryRef)->link.prev = 0;
    (*entryRef)->owner = owner;
    (*entryRef)->size = chunkLength;
    (*entryRef)->ownerRef = entryRef;
    (*entryRef)->destroyNotify = _rxGCResEntryWaitDone;
    native->token = _RwDlTokenCurrent;
    DCFlushRange(*entryRef + 1, (*entryRef)->size);
    GXInvalidateVtxCache();
    return owner;
}

static RwStream* _rpNativeWrite(RwStream* stream, RwResEntry* entry,
                                unsigned int numMeshes)
{
    int platform = 6;
    GameCubeNativeMeshHeader* native =
        (GameCubeNativeMeshHeader*)(entry + 1);
    GameCubeNativeMesh* meshes =
        &native->meshes[native->numMeshes - 1] + 1;
    int chunkSize = _rpNativeSize(entry, numMeshes) - 12;
    int headerSize;
    int dataSize;

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
    if (!RwStreamWrite(stream, meshes->displayList.pointer, dataSize)) return 0;

    DCFlushRange(native, entry->size);
    GXInvalidateVtxCache();
    return stream;
}

int _rpGeometryNativeSize(const RpGeometry* geometry)
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
    RpGeometry* result = _rpNativeRead(stream, geometry, &geometry->repEntry,
                                       geometry->meshHeader->numMeshes);
    return result;
}

int _rpWorldSectorNativeSize(const RpWorldSector* sector)
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
    RpWorldSector* result = _rpNativeRead(stream, sector, &sector->repEntry,
                                          sector->mesh->numMeshes);
    return result;
}
