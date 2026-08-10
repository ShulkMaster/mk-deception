#include "libmkparticle/rw_engine.h"
#include "rw/gamecube.h"
#include "rw/rpskin.h"
#include "rw/rpworld_types.h"
#include "rw/rwerror.h"
#include "rw/rwfreelist.h"
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

RwUInt32 _rpSkinGeometryNativeSize(const RpGeometry* geometry)
{

    RpSkin* skin = *(RpSkin**)((RwUInt8*)geometry +
                               _rpSkinGlobals.geometryOffset);
    RwUInt32 size;

    size = 0x10;
    size += 4;
    size += skin->numUsedBones;
    if (skin->maxNumWeights > 1) {
        size += geometry->numVertices * (skin->maxNumWeights * 2);
    }
    size += skin->numBones * sizeof(RwMatrix);

    if (skin->nativeData != 0) {
        RwGameCubeVertexBuffer* vertexBuffer =
            (RwGameCubeVertexBuffer*)(geometry->repEntry + 1);

        (*(RwGameCubeVertexDataHeader**)(
             (RwUInt8*)vertexBuffer->arrays[0].data - sizeof(void*)))
            ->serialNumber = 0;
        vertexBuffer->arrays[0].data = skin->nativeData;
        if ((geometry->flags & 0x10) != 0)
            vertexBuffer->arrays[1].data = skin->nativeData2;
        geometry->repEntry->destroyNotify = _rxGCResEntryWaitDone;
        skin->nativeData = 0;
        skin->nativeData2 = 0;
    }

    size += _rpSkinSplitDataStreamGetSize(skin);
    return size;
}

RwStream* _rpSkinGeometryNativeWrite(RwStream* stream,
                                     const RpGeometry* geometry)
{


    return stream;
}

RwStream* _rpSkinGeometryNativeRead(RwStream* stream, RpGeometry* geometry)
{



    RwUInt32 version;
    RwUInt32 chunkSize;
    RwInt32 skinHeader;
    RwInt32 nativeVersion;
    RpSkin* skin;
    RwUInt32 numVertices;

    if (!RwStreamFindChunk(stream, 1, &chunkSize, &version))
        return 0;
    if (version < 0x34000 || version > 0x36003) {
        RwError error;
        error.pluginID = 0x116;
        error.errorCode = _rwerror(0x80000004);
        RwErrorSet(&error);
        return 0;
    }
    if (version < 0x34002) {
        RwError error;
        error.pluginID = 0x116;
        error.errorCode = _rwerror(0x80000004);
        RwErrorSet(&error);
        return 0;
    }
    if (RwStreamReadInt32(stream, &nativeVersion,
                          sizeof(nativeVersion)) == 0)
        return 0;
    if (nativeVersion != 6)
        return 0;

    skin = RwEngineInstance->fpFreeListAlloc(
        (RwFreeList*)_rpSkinGlobals.skinFreeList, 0x30116);
    memset(skin, 0, sizeof(*skin));
    if (RwStreamReadInt32(stream, &skinHeader, sizeof(skinHeader)) == 0)
        return 0;

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
            return 0;
        chunkSize = skin->maxNumWeights * numVertices;
        if (RwStreamRead(stream, skin->platformIndices, chunkSize) !=
            chunkSize)
            return 0;
        chunkSize = skin->maxNumWeights * numVertices;
        if (RwStreamRead(stream, skin->platformWeights, chunkSize) !=
            chunkSize)
            return 0;
        chunkSize = skin->numBones * sizeof(RwMatrix);
        if (RwStreamRead(stream, skin->skinToBoneMatrices, chunkSize) !=
            chunkSize)
            return 0;
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
            return 0;
        chunkSize = skin->numBones * sizeof(RwMatrix);
        if (RwStreamRead(stream, skin->skinToBoneMatrices, chunkSize) !=
            chunkSize)
            return 0;
    }

    if (_rpSkinSplitDataStreamRead(stream, skin) == 0)
        return 0;
    RpSkinGeometrySetSkin(geometry, skin);
    return stream;
}

RwUInt32 _rpSkinAtomicNativeSize(const RpAtomic* atomic)
{
    RpSkinAtomicData* skinData =
        (RpSkinAtomicData*)((RwUInt8*)atomic + _rpSkinGlobals.atomicOffset);

    if (skinData->cachedVertexData != 0) {
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
        skinData->cachedVertexData = 0;
        skinData->cachedVertexData2 = 0;
    }
    return 0;
}
