#include "libmkparticle/rw_engine.h"
#include "rw/rpskin.h"
#include "rw/rwstream.h"
#include "runtime/cstring.h"

RpSkin* _rpSkinSplitDataCreate(RpSkin* skin, RwUInt32 boneLimit,
                               RwUInt32 numBones, RwUInt32 numMeshes,
                               RwUInt32 rleSize)
{
    RpSkinSplitData* splitData = &skin->splitData;
    RwUInt32 size;

    _rpSkinSplitDataDestroy(skin);
    size = numBones + 2 * numMeshes + 2 * rleSize;
    splitData->remapIndices = RwEngineInstance->fpMalloc(size, 0x30116);
    if (splitData->remapIndices == 0) {
        return 0;
    }

    memset(splitData->remapIndices, 0, size);
    splitData->boneLimit = boneLimit;
    splitData->numMeshes = numMeshes;
    splitData->rleSize = rleSize;
    splitData->rleCount =
        (RpSkinRLECount*)(splitData->remapIndices + numBones);
    splitData->rle = (RpSkinRLE*)(splitData->rleCount + numMeshes);
    return skin;
}

RwBool _rpSkinSplitDataDestroy(RpSkin* skin)
{
    RpSkinSplitData* splitData = &skin->splitData;

    if (splitData->remapIndices != 0) {
        RwEngineInstance->fpFree(splitData->remapIndices);
    }
    splitData->boneLimit = 0;
    splitData->numMeshes = 0;
    splitData->rleSize = 0;
    splitData->remapIndices = 0;
    splitData->rleCount = 0;
    splitData->rle = 0;
    return 1;
}


RwStream* _rpSkinSplitDataStreamWrite(RwStream* stream, const RpSkin* skin)
{
    const RpSkinSplitData* splitData = &skin->splitData;

    if (RwStreamWriteInt32(stream, (const RwInt32*)&splitData->boneLimit,
                           sizeof(splitData->boneLimit)) == 0) {
        return 0;
    }
    if (RwStreamWriteInt32(stream, (const RwInt32*)&splitData->numMeshes,
                           sizeof(splitData->numMeshes)) == 0) {
        return 0;
    }
    if (RwStreamWriteInt32(stream, (const RwInt32*)&splitData->rleSize,
                           sizeof(splitData->rleSize)) == 0) {
        return 0;
    }
    if (splitData->numMeshes != 0) {
        RwUInt32 size = skin->numBones +
                        2 * splitData->numMeshes + 2 * splitData->rleSize;
        if (RwStreamWrite(stream, splitData->remapIndices, size) == 0) {
            return 0;
        }
    }
    return stream;
}

RwStream* _rpSkinSplitDataStreamRead(RwStream* stream, RpSkin* skin)
{
    RpSkinSplitData* splitData = &skin->splitData;
    RwInt32 numMeshes;
    RwInt32 rleSize;
    RwInt32 boneLimit;

    if (RwStreamReadInt32(stream, &boneLimit, sizeof(boneLimit)) == 0) {
        return 0;
    }
    if (RwStreamReadInt32(stream, &numMeshes, sizeof(numMeshes)) == 0) {
        return 0;
    }
    if (RwStreamReadInt32(stream, &rleSize, sizeof(rleSize)) == 0) {
        return 0;
    }
    if (numMeshes > 0) {
        RwUInt32 size;

        if (_rpSkinSplitDataCreate(skin, boneLimit, skin->numBones,
                                   numMeshes, rleSize) == 0) {
            return 0;
        }
        size = skin->numBones +
               2 * splitData->numMeshes + 2 * splitData->rleSize;
        if (RwStreamRead(stream, splitData->remapIndices, size) == 0) {

            RwEngineInstance->fpFree(splitData);
            return 0;
        }
    }
    return stream;
}

RwUInt32 _rpSkinSplitDataStreamGetSize(const RpSkin* skin)
{
    RwUInt32 size = 3 * sizeof(RwUInt32);

    if (skin->splitData.numMeshes != 0) {
        size += skin->numBones + 2 * skin->splitData.numMeshes +
                2 * skin->splitData.rleSize;
    }
    return size;
}
