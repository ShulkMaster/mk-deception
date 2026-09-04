#include "rw/rwengine.h"
#include "rw/gamecube.h"
#include "rw/rpskin.h"
#include "rw/rpworld_types.h"
#include "rw/rwerror.h"
#include "rw/rwfreelist.h"
#include "rw/rwresources.h"
#include "rw/rwstream.h"

typedef struct RwGameCubeVertexDataHeader {
    unsigned char reserved_0x00[0x10];
    unsigned int serialNumber;
} RwGameCubeVertexDataHeader;

unsigned int _rpSkinGeometryNativeSize(const RpGeometry* geometry)
{
    RpSkin* skin = *(RpSkin**)((unsigned char*)geometry +
                               _rpSkinGlobals.geometryOffset);
    unsigned int size;

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
             (unsigned char*)vertexBuffer->arrays[0].data - sizeof(void*)))
            ->serialNumber = 0;
        vertexBuffer->arrays[0].data = skin->nativeData;
        if ((geometry->flags & 0x10) != 0)
            vertexBuffer->arrays[1].data = skin->nativeData2;
        geometry->repEntry->destroyNotify = _rxGCResEntryWaitDone;
        skin->nativeData = 0;
        skin->nativeData2 = 0;
    }

    size += _rpSkinSplitDataStreamGetSize(skin);
    /* TODO: Retail emits an overwritten zero initialization and shared GPR
     * save helpers; keep the semantic calculation without dead source. */
    return size;
}

RwStream* _rpSkinGeometryNativeWrite(RwStream* stream,
                                     const RpGeometry* geometry)
{
    int platform = 6;

    return stream;
}

RwStream* _rpSkinGeometryNativeRead(RwStream* stream, RpGeometry* geometry)
{
    unsigned int version;
    unsigned int chunkSize;
    int skinHeader;
    int nativeVersion;
    RpSkin* skin;
    unsigned int numVertices;

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

    skin->numBones = (unsigned char)skinHeader;
    skin->numUsedBones = ((unsigned int)skinHeader >> 8) & 0xff;
    skin->maxNumWeights = ((unsigned int)skinHeader >> 16) & 0xff;
    numVertices = geometry->numVertices;
    chunkSize -= 8;

    if (skin->maxNumWeights > 1) {
        skin->platformWeights = RwEngineInstance->fpMalloc(
            chunkSize + 5, 0x30116);
        skin->platformIndices = (unsigned char*)skin->platformWeights +
            skin->maxNumWeights * numVertices;
        skin->platformIndices = (void*)
            (((unsigned int)skin->platformIndices + 3) & ~3U);
        skin->skinToBoneMatrices = (RwMatrix*)((unsigned char*)skin->platformIndices +
            skin->maxNumWeights * numVertices);
        skin->skinToBoneMatrices = (RwMatrix*)
            (((unsigned int)skin->skinToBoneMatrices + 3) & ~3U);
        skin->usedBoneList = (unsigned char*)skin->skinToBoneMatrices +
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
            (((unsigned int)skin->skinToBoneMatrices + 3) & ~3U);
        skin->usedBoneList = (unsigned char*)skin->skinToBoneMatrices +
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
    /* TODO: The retail routine uses shared GPR save/restore helpers; this
     * equivalent reader currently differs only in frame/save lowering and
     * the resulting branch displacements. */
    return stream;
}

unsigned int _rpSkinAtomicNativeSize(const RpAtomic* atomic)
{
    SkinAtomicState* skinData =
        (SkinAtomicState*)((unsigned char*)atomic + _rpSkinGlobals.atomicOffset);

    if (skinData->positions != 0) {
        RwGameCubeVertexBuffer* vertexBuffer =
            (RwGameCubeVertexBuffer*)(atomic->repEntry + 1);

        (*(RwGameCubeVertexDataHeader**)(
             (unsigned char*)vertexBuffer->arrays[0].data - sizeof(void*)))
            ->serialNumber = 0;
        vertexBuffer->arrays[0].data = skinData->positions;
        if ((atomic->geometry->flags & 0x10) != 0)
            vertexBuffer->arrays[1].data = skinData->normals;
        atomic->repEntry->destroyNotify =
            _rxGCResEntryWaitDone;
        skinData->positions = 0;
        skinData->normals = 0;
    }
    return 0;
}
