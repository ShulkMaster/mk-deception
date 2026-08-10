#include "libmkparticle/rw_engine.h"
#include "rw/gamecube.h"
#include "rw/rpskin.h"
#include "rw/rpworld_types.h"
#include "rw/rwerror.h"
#include "rw/rwresources.h"
#include "rw/rwstream.h"

typedef struct RpSkinAtomicData {
    RpHAnimHierarchy* hierarchy;
    void* cachedVertexData;
    void* cachedVertexData2;
} RpSkinAtomicData;

typedef struct RwGameCubeVertexDataHeader {
    RwUInt8 reserved_0x00[0x10];
    RwUInt32 serialNumber;
} RwGameCubeVertexDataHeader;

#define SKIN_ATOMIC_DATA(atomic)                                          \
    ((RpSkinAtomicData*)((RwUInt8*)(atomic) + _rpSkinGlobals.atomicOffset))
#define SKIN_GEOMETRY_DATA(geometry)                                      \
    (*(RpSkin**)((RwUInt8*)(geometry) + _rpSkinGlobals.geometryOffset))

RwUInt32 _rpSkinGeometryNativeSize(const RpGeometry* geometry)
{
    /* Retail's functional body is recovered exactly. Its O0 source also emits
     * an overwritten initial zero for size and selects save/restore helpers;
     * the clean ownership form below intentionally omits that dead lifetime. */
    RpSkin* skin = SKIN_GEOMETRY_DATA(geometry);
    RwUInt32 size = 0;

    size = 0x10;
    size += 4;
    size += skin->numUsedBones;
    if (skin->maxNumWeights > 1) {
        size += geometry->numVertices * (skin->maxNumWeights * 2);
    }
    size += skin->numBones * sizeof(RwMatrix);

    if (skin->nativeData != NULL) {
        RwGameCubeVertexBuffer* vertexBuffer =
            (RwGameCubeVertexBuffer*)(geometry->repEntry + 1);

        (*(RwGameCubeVertexDataHeader**)(
             (RwUInt8*)vertexBuffer->arrays[0].data - sizeof(void*)))
            ->serialNumber = 0;
        vertexBuffer->arrays[0].data = skin->nativeData;
        if ((geometry->flags & 0x10) != 0)
            vertexBuffer->arrays[1].data = skin->nativeData2;
        geometry->repEntry->destroyNotify = _rxGCResEntryWaitDone;
        skin->nativeData = NULL;
        skin->nativeData2 = NULL;
    }

    size += _rpSkinSplitDataStreamGetSize(skin);
    return size;
}

RwStream* _rpSkinGeometryNativeWrite(RwStream* stream,
                                     const RpGeometry* geometry)
{
    /* Retail only materializes an otherwise unused native-version value (6)
     * before returning the untouched stream. Do not recreate that dead sink. */
    return stream;
}

RwStream* _rpSkinGeometryNativeRead(RwStream* stream, RpGeometry* geometry)
{
    /* The stream CFG, allocations, pointer layout, reads, and leak-on-failure
     * behavior match retail. Residue is byte-extraction and save-helper
     * emission, not a missing native-data operation. */
    RwUInt32 version;
    RwUInt32 chunkSize;
    RwInt32 skinHeader;
    RwInt32 nativeVersion;
    RpSkin* skin;
    RwUInt32 numVertices;

    if (!RwStreamFindChunk(stream, 1, &chunkSize, &version))
        return NULL;
    if (version < 0x34000 || version > 0x36003) {
        RwError error;
        error.pluginID = 0x116;
        error.errorCode = _rwerror(0x80000004);
        RwErrorSet(&error);
        return NULL;
    }
    if (version < 0x34002) {
        RwError error;
        error.pluginID = 0x116;
        error.errorCode = _rwerror(0x80000004);
        RwErrorSet(&error);
        return NULL;
    }
    if (RwStreamReadInt32(stream, &nativeVersion,
                          sizeof(nativeVersion)) == NULL)
        return NULL;
    if (nativeVersion != 6)
        return NULL;

    skin = RwEngineInstance->fpFreeListAlloc(
        (RwFreeList*)_rpSkinGlobals.skinFreeList, 0x30116);
    memset(skin, 0, sizeof(*skin));
    if (RwStreamReadInt32(stream, &skinHeader, sizeof(skinHeader)) == NULL)
        return NULL;

    skin->numBones = (RwUInt8)skinHeader;
    skin->numUsedBones = (RwUInt8)((RwUInt32)skinHeader >> 8);
    skin->maxNumWeights = (RwUInt8)((RwUInt32)skinHeader >> 16);
    numVertices = geometry->numVertices;
    chunkSize -= 8;

    if (skin->maxNumWeights > 1) {
        skin->platformWeights = RwEngineInstance->fpMalloc(
            chunkSize + 5, 0x30116);
        skin->platformIndices = (RwUInt8*)skin->platformWeights +
            skin->maxNumWeights * numVertices;
        skin->platformIndices = (void*)
            (((RwUInt32)skin->platformIndices + 3) & ~3U);
        skin->skinToBoneMatrices = (RwMatrix*)((RwUInt8*)skin->platformIndices +
            skin->maxNumWeights * numVertices);
        skin->skinToBoneMatrices = (RwMatrix*)
            (((RwUInt32)skin->skinToBoneMatrices + 3) & ~3U);
        skin->usedBoneList = (RwUInt8*)skin->skinToBoneMatrices +
            skin->numBones * sizeof(RwMatrix);

        chunkSize = skin->numUsedBones;
        if (RwStreamRead(stream, skin->usedBoneList, chunkSize) != chunkSize)
            return NULL;
        chunkSize = skin->maxNumWeights * numVertices;
        if (RwStreamRead(stream, skin->platformIndices, chunkSize) !=
            chunkSize)
            return NULL;
        chunkSize = skin->maxNumWeights * numVertices;
        if (RwStreamRead(stream, skin->platformWeights, chunkSize) !=
            chunkSize)
            return NULL;
        chunkSize = skin->numBones * sizeof(RwMatrix);
        if (RwStreamRead(stream, skin->skinToBoneMatrices, chunkSize) !=
            chunkSize)
            return NULL;
    } else {
        skin->platformWeights = RwEngineInstance->fpMalloc(
            chunkSize + 3, 0x30116);
        skin->skinToBoneMatrices = (RwMatrix*)skin->platformWeights;
        skin->skinToBoneMatrices = (RwMatrix*)
            (((RwUInt32)skin->skinToBoneMatrices + 3) & ~3U);
        skin->usedBoneList = (RwUInt8*)skin->skinToBoneMatrices +
            skin->numBones * sizeof(RwMatrix);

        chunkSize = skin->numUsedBones;
        if (RwStreamRead(stream, skin->usedBoneList, chunkSize) != chunkSize)
            return NULL;
        chunkSize = skin->numBones * sizeof(RwMatrix);
        if (RwStreamRead(stream, skin->skinToBoneMatrices, chunkSize) !=
            chunkSize)
            return NULL;
    }

    if (_rpSkinSplitDataStreamRead(stream, skin) == NULL)
        return NULL;
    RpSkinGeometrySetSkin(geometry, skin);
    return stream;
}

RwUInt32 _rpSkinAtomicNativeSize(const RpAtomic* atomic)
{
    RpSkinAtomicData* skinData = SKIN_ATOMIC_DATA(atomic);

    if (skinData->cachedVertexData != NULL) {
        RwGameCubeVertexBuffer* vertexBuffer =
            (RwGameCubeVertexBuffer*)((RwResEntry*)atomic->repEntry + 1);

        (*(RwGameCubeVertexDataHeader**)(
             (RwUInt8*)vertexBuffer->arrays[0].data - sizeof(void*)))
            ->serialNumber = 0;
        vertexBuffer->arrays[0].data = skinData->cachedVertexData;
        if ((atomic->geometry->flags & 0x10) != 0)
            vertexBuffer->arrays[1].data = skinData->cachedVertexData2;
        ((RwResEntry*)atomic->repEntry)->destroyNotify =
            _rxGCResEntryWaitDone;
        skinData->cachedVertexData = NULL;
        skinData->cachedVertexData2 = NULL;
    }
    return 0;
}
