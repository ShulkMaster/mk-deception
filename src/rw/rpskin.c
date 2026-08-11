#include "libmkparticle/rw_engine.h"
#include "runtime/cstring.h"
#include "rw/rphanim.h"
#include "rw/rpskin.h"
#include "rw/rpworld_types.h"
#include "rw/rwfreelist.h"
#include "rw/rwstream.h"

typedef struct RpSkinAtomicData {
    RpHAnimHierarchy* hierarchy;
    RwUInt32 reserved_0x04[2];
} RpSkinAtomicData;

static RwFreeList _rpSkinFreeList;
static RwInt32 _rpSkinFreeListBlockSize = 0x14;
static RwInt32 _rpSkinFreeListPreallocBlocks = 1;

extern RwInt32 RwEngineGetPluginOffset(RwUInt32 pluginID);
extern RwInt32 RpAtomicRegisterPlugin(
    RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor constructCB,
    RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB);
extern RwInt32 RpAtomicRegisterPluginStream(
    RwUInt32 pluginID, RwPluginDataChunkReadCallBack readCB,
    RwPluginDataChunkWriteCallBack writeCB,
    RwPluginDataChunkGetSizeCallBack getSizeCB);
extern RwInt32 RpAtomicSetStreamAlwaysCallBack(
    RwUInt32 pluginID, RwPluginDataChunkAlwaysCallBack callback);
extern RwInt32 RpAtomicSetStreamRightsCallBack(
    RwUInt32 pluginID, RwPluginDataChunkRightsCallBack callback);
extern RwInt32 RpAtomicGetPluginOffset(RwUInt32 pluginID);
extern RwStream* RwStreamWriteReal(RwStream* stream, const RwReal* values,
                                   RwUInt32 numBytes);
extern RwStream* RwStreamReadReal(RwStream* stream, RwReal* values,
                                  RwUInt32 numBytes);
extern RpGeometry* _rpSkinInitialize(RpGeometry* geometry);
extern RpGeometry* _rpSkinDeinitialize(RpGeometry* geometry);
static RpSkinAtomicData* SkinAtomicData(const void* atomic)
{
    return (RpSkinAtomicData*)((RwUInt8*)atomic +
                               _rpSkinGlobals.atomicOffset);
}

static RpSkin** SkinGeometryData(const void* geometry)
{
    return (RpSkin**)((RwUInt8*)geometry + _rpSkinGlobals.geometryOffset);
}

static RwBool MatfxPluginIsAttached(void)
{
    return RwEngineGetPluginOffset(0x120) != -1;
}

static RwBool ToonPluginIsAttached(void)
{
    return RwEngineGetPluginOffset(0x12E) != -1;
}

static RpAtomic* SkinAtomicAttachBestPipeForAttachedPlugins(
    RpAtomic* atomic, RpSkinType type)
{
    if (!MatfxPluginIsAttached() && type == rpSKINTYPEMATFX) {
        type = rpSKINTYPEGENERIC;
    } else if (!ToonPluginIsAttached() && type == rpSKINTYPETOON) {
        type = rpSKINTYPEGENERIC;
    }
    return _rpSkinPipelinesAttach(atomic, type);
}

static RpAtomic* SkinAtomicSetup(RpAtomic* atomic, RpSkinType type)
{
    RpGeometry* geometry = atomic->geometry;
    RpSkin* skin;

    if (geometry != 0) {
        skin = RpSkinGeometryGetSkin(geometry);
        if (skin != 0) {

            SkinAtomicAttachBestPipeForAttachedPlugins(atomic, type);
        }
    }
    return atomic;
}

static void SkinFindMaxWeights(RpSkin* skin,
                               const RwMatrixWeights* vertexWeights,
                               RwUInt32 numVertices)
{
    RwUInt32 vertex;

    skin->maxNumWeights = 1;
    for (vertex = 0; vertex < numVertices; vertex++) {
        RwUInt32 weight;
        for (weight = skin->maxNumWeights; weight < 4; weight++) {
            if (((const RwUInt32*)&vertexWeights[vertex])[weight] != 0) {
                skin->maxNumWeights++;
                if (skin->maxNumWeights == 4) {
                    return;
                }
            } else {
                break;
            }
        }
    }
}

static void SkinFindNumUsedBones(RpSkin* skin, const RwUInt32* vertexIndices,
                                 const RwMatrixWeights* vertexWeights,
                                 RwUInt8* usedBoneList,
                                 RwUInt32* numUsedBones,
                                 RwUInt32 numVertices)
{
    RwUInt32 vertex;

    *numUsedBones = 0;
    for (vertex = 0; vertex < numVertices; vertex++) {
        RwUInt32 weight;
        for (weight = 0; weight < skin->maxNumWeights; weight++) {
            if (((const RwUInt32*)&vertexWeights[vertex])[weight] != 0) {
                RwUInt32 bone;
                RwBool unique = 1;
                RwUInt32 index;

                bone = (RwUInt8)(vertexIndices[vertex] >> (weight * 8));
                for (index = 0; index < *numUsedBones; index++) {
                    if (bone == usedBoneList[index]) {
                        unique = 0;
                        break;
                    }
                }
                if (unique) {
                    usedBoneList[*numUsedBones] = bone;
                    (*numUsedBones)++;
                }
            }
        }
    }
}

static RwBool SkinCreateSkinData(
    RpSkin* skin, RwUInt32 numBones, RwUInt32 numUsedBones,
    RwUInt32 numVertices, const RwUInt8* usedBoneList,
    const RwMatrixWeights* vertexWeights, const RwUInt32* vertexIndices,
    const RwMatrix* skinToBoneMatrices)
{
    RwUInt32 allocationSize =
        numVertices * (sizeof(RwUInt32) + sizeof(RwMatrixWeights)) +
        numBones * sizeof(RwMatrix) + numUsedBones + 15;

    skin->skinData = RwEngineInstance->fpMalloc(allocationSize, 0x30116);
    if (skin->skinData == 0) {
        return 0;
    }

    memset(skin->skinData, 0, allocationSize);
    skin->numBones = numBones;
    skin->numUsedBones = numUsedBones;
    skin->usedBoneList = skin->skinData;
    skin->skinToBoneMatrices = (RwMatrix*)
        (((RwUInt32)skin->usedBoneList + numUsedBones + 15) & ~15U);
    skin->vertexBoneIndices =
        (RwUInt32*)((RwUInt8*)skin->skinToBoneMatrices +
                   numBones * sizeof(RwMatrix));
    skin->vertexBoneWeights =
        (RwMatrixWeights*)(skin->vertexBoneIndices + numVertices);

    if (usedBoneList != 0 && numUsedBones != 0) {
        RwUInt32 usedBoneDataSize = numUsedBones * sizeof(RwUInt8);
        memcpy(skin->usedBoneList, usedBoneList, usedBoneDataSize);
    }
    if (skinToBoneMatrices != 0) {
        RwUInt32 bone = numBones;
        while (bone-- != 0) {
            RwUInt32* destination =
                (RwUInt32*)&skin->skinToBoneMatrices[bone];
            const RwUInt32* source =
                (const RwUInt32*)&skinToBoneMatrices[bone];
            RwUInt32 pair = sizeof(RwMatrix) / (2 * sizeof(RwUInt32));



            do {
                *destination++ = *source++;
                *destination++ = *source++;
            } while (--pair != 0);
        }
    }
    if (vertexIndices != 0) {
        RwUInt32 vertexIndicesSize = numVertices * sizeof(RwUInt32);
        memcpy(skin->vertexBoneIndices, vertexIndices,
               vertexIndicesSize);
    }
    if (vertexWeights != 0) {
        RwUInt32 vertexWeightsSize = numVertices * sizeof(RwMatrixWeights);
        memcpy(skin->vertexBoneWeights, vertexWeights,
               vertexWeightsSize);
    }
    return 1;
}

static RpSkin* SkinCreate(RwUInt32 numVertices, RwUInt32 numBones,
                          RwUInt32 numUsedBones, RwUInt32 maxNumWeights,
                          const RwMatrixWeights* vertexWeights,
                          const RwUInt32* vertexIndices,
                          const RwMatrix* skinToBoneMatrices)
{
    RwUInt8 usedBoneList[256];
    RpSkin* skin = RwEngineInstance->fpFreeListAlloc(
        (RwFreeList*)_rpSkinGlobals.skinFreeList, 0x30116);

    memset(skin, 0, sizeof(RpSkin));
    if (maxNumWeights == 0) {
        SkinFindMaxWeights(skin, vertexWeights, numVertices);
    }
    if (numUsedBones == 0) {
        SkinFindNumUsedBones(skin, vertexIndices, vertexWeights, usedBoneList,
                             &numUsedBones, numVertices);
    }
    if (!SkinCreateSkinData(skin, numBones, numUsedBones, numVertices,
                            usedBoneList, vertexWeights, vertexIndices,
                            skinToBoneMatrices)) {
        RwEngineInstance->fpFreeListFree(
            (RwFreeList*)_rpSkinGlobals.skinFreeList, skin);
        return 0;
    }
    return skin;
}

static void* SkinOpen(void* instance, RwInt32 offset, RwInt32 size)
{
    if (_rpSkinGlobals.numInstances == 0) {
        RwUInt32 pipelineTypes = rpSKINTYPEGENERIC;
        RwUInt32 scratchSize;

        if (MatfxPluginIsAttached()) {
            pipelineTypes |= rpSKINTYPEMATFX;
        }
        if (ToonPluginIsAttached()) {
            pipelineTypes |= 4;
        }

        _rpSkinPipelinesCreate(pipelineTypes);
        _rpSkinGlobals.skinFreeList = RwFreeListCreateAndPreallocateSpace(
            sizeof(RpSkin), _rpSkinFreeListBlockSize, 4,
            _rpSkinFreeListPreallocBlocks, &_rpSkinFreeList, 0x40116);
        scratchSize = 0x400F;
        _rpSkinGlobals.scratchMemory =
            RwEngineInstance->fpMalloc(scratchSize, 0x40116);
        memset(_rpSkinGlobals.scratchMemory, 0, scratchSize);
        _rpSkinGlobals.alignedScratchMemory = (void*)
            (((RwUInt32)_rpSkinGlobals.scratchMemory + 15) & ~15U);
    }
    _rpSkinGlobals.numInstances++;
    return instance;
}

static void* SkinClose(void* instance, RwInt32 offset, RwInt32 size)
{
    _rpSkinGlobals.numInstances--;
    if (_rpSkinGlobals.numInstances == 0) {

        _rpSkinPipelinesDestroy();
        RwFreeListDestroy((RwFreeList*)_rpSkinGlobals.skinFreeList);
        _rpSkinGlobals.skinFreeList = 0;
        RwEngineInstance->fpFree(_rpSkinGlobals.scratchMemory);
        _rpSkinGlobals.scratchMemory = 0;
    }
    return instance;
}

static void* SkinGeometryConstructor(void* object, RwInt32 offset,
                                     RwInt32 size)
{
    *SkinGeometryData(object) = 0;
    return object;
}

static void* SkinGeometryDestructor(void* object, RwInt32 offset,
                                    RwInt32 size)
{

    RpGeometry* geometry = object;
    RpSkin* skin = *SkinGeometryData(geometry);

    if (skin != 0) {
        _rpSkinDeinitialize(geometry);
        *SkinGeometryData(geometry) = RpSkinDestroy(skin);
    }
    return object;
}

static void* SkinGeometryCopy(void* destination, const void* source,
                              RwInt32 offset, RwInt32 size)
{
    return destination;
}

static void* SkinAtomicConstructor(void* object, RwInt32 offset, RwInt32 size)
{

    memset(SkinAtomicData(object), 0, sizeof(RpSkinAtomicData));
    return object;
}

static void* SkinAtomicDestructor(void* object, RwInt32 offset, RwInt32 size)
{
    RpAtomic* atomic = object;
    RpSkinAtomicData* data = SkinAtomicData(atomic);
    if (data->hierarchy != 0) {
        data->hierarchy = 0;
    }
    return object;
}

static void* SkinAtomicCopy(void* destination, const void* source,
                            RwInt32 offset, RwInt32 size)
{
    const RpSkinAtomicData* sourceData = SkinAtomicData(source);
    RpSkinAtomicData* destinationData = SkinAtomicData(destination);

    destinationData->hierarchy = sourceData->hierarchy;
    return destination;
}

static RwBool SkinAtomicAlways(void* object, RwInt32 offset, RwInt32 size)
{
    RpAtomic* atomic;
    RpSkinType type = rpSKINTYPEGENERIC;
    atomic = object;

    if (MatfxPluginIsAttached() &&
        *(RwUInt8*)((RwUInt8*)atomic + RpAtomicGetPluginOffset(0x120)) != 0) {
        type = rpSKINTYPEMATFX;
    }
    SkinAtomicSetup(atomic, type);
    return 1;
}

static RwBool SkinAtomicRights(void* object, RwInt32 offset, RwInt32 size,
                               RwUInt32 extraData)
{
    RpAtomic* atomic = object;
    RpSkinType type = (RpSkinType)extraData;

    SkinAtomicSetup(atomic, type);
    return 1;
}

static RwInt32 SkinGeometrySize(const void* object, RwInt32 offset,
                                RwInt32 size)
{

    RwInt32 result = 0;
    const RpGeometry* geometry = object;
    RpSkin* skin = *SkinGeometryData(geometry);

    if (skin != 0) {
        if (!(geometry->flags & 0x01000000)) {
            RwInt32 numVertices = geometry->numVertices;
            result = 4;
            result += skin->numUsedBones;
            result += numVertices * sizeof(RwUInt32);
            result += numVertices * sizeof(RwMatrixWeights);
            result += skin->numBones * sizeof(RwMatrix);
            result += _rpSkinSplitDataStreamGetSize(skin);
        } else {
            result = _rpSkinGeometryNativeSize(geometry);
        }
    }
    return result;
}

static RwStream* SkinGeometryWrite(RwStream* stream, RwInt32 binaryLength,
                                   const void* object, RwInt32 offset,
                                   RwInt32 size)
{
    RwStream* result;
    const RpGeometry* geometry = object;
    const RpSkin* skin = *SkinGeometryData(geometry);

    if (skin != 0) {
        if (!(geometry->flags & 0x01000000)) {
            RwInt32 numVertices = geometry->numVertices;
            RwInt32 header = (RwUInt8)skin->numBones |
                             (((skin->maxNumWeights << 16) & 0xFF0000) |
                              ((skin->numUsedBones << 8) & 0xFF00));

            result = RwStreamWriteInt32(stream, &header, 4);
            if (result == 0) return 0;
            result = RwStreamWrite(stream, skin->usedBoneList,
                                   skin->numUsedBones);
            if (result == 0) return 0;
            result = RwStreamWriteInt32(
                stream, (const RwInt32*)skin->vertexBoneIndices,
                numVertices * 4);
            if (result == 0) return 0;
            result = RwStreamWriteReal(
                stream, (const RwReal*)skin->vertexBoneWeights,
                numVertices * 0x10);
            if (result == 0) return 0;
            result = RwStreamWriteReal(
                stream, (const RwReal*)skin->skinToBoneMatrices,
                skin->numBones * 0x40);
            if (result == 0) return 0;
            result = _rpSkinSplitDataStreamWrite(stream, skin);
            if (result == 0) return 0;
        } else {
            if (_rpSkinGeometryNativeWrite(stream, geometry) == 0) {
                return 0;
            }
        }
    }
    return stream;
}

static RwStream* SkinGeometryRead(RwStream* stream, RwInt32 binaryLength,
                                  void* object, RwInt32 offset, RwInt32 size)
{
    RwStream* result;
    RpGeometry* geometry = object;
    RpSkin* skin;

    if (!(geometry->flags & 0x01000000)) {
        RwUInt32 packed;
        RwUInt32 numBones;
        RwUInt32 numUsedBones;
        RwUInt32 maxWeights;
        RwInt32 numVertices;
        RwUInt32 bytesToRead;

        result = RwStreamReadInt32(stream, (RwInt32*)&packed, 4);
        if (result == 0) return 0;
        numBones = packed & 0xFF;
        numUsedBones = (packed >> 8) & 0xFF;
        maxWeights = (packed >> 16) & 0xFF;
        numVertices = geometry->numVertices;
        if (maxWeights == 0) {
            skin = SkinCreate(numVertices, numBones, numBones, 4,
                              0, 0, 0);
            if (skin == 0) return 0;
        } else {
            skin = SkinCreate(numVertices, numBones, numUsedBones, maxWeights,
                              0, 0, 0);
            if (skin == 0) return 0;
            bytesToRead = numUsedBones;
            if (bytesToRead !=
                RwStreamRead(stream, skin->usedBoneList, bytesToRead)) {
                return 0;
            }
        }
        result = RwStreamReadInt32(stream,
                                   (RwInt32*)skin->vertexBoneIndices,
                                   numVertices * 4);
        if (result == 0) return 0;
        result = RwStreamReadReal(stream, (RwReal*)skin->vertexBoneWeights,
                                  numVertices * 0x10);
        if (result == 0) return 0;
        if (maxWeights == 0) {
            RwUInt32 bone;
            for (bone = 0; bone < skin->numBones; bone++) {
                result = RwStreamSkip(stream, 4);
                if (result == 0) return 0;
                result = RwStreamReadReal(
                    stream, (RwReal*)&skin->skinToBoneMatrices[bone], 0x40);
                if (result == 0) return 0;
            }
            SkinFindMaxWeights(skin, skin->vertexBoneWeights, numVertices);
            SkinFindNumUsedBones(skin, skin->vertexBoneIndices,
                                 skin->vertexBoneWeights, skin->usedBoneList,
                                 &skin->numUsedBones, numVertices);
        } else {
            skin->maxNumWeights = maxWeights;
            result = RwStreamReadReal(stream,
                                      (RwReal*)skin->skinToBoneMatrices,
                                      skin->numBones * 0x40);
            if (result == 0) return 0;
            result = _rpSkinSplitDataStreamRead(stream, skin);
            if (result == 0) return 0;
        }

        RpSkinGeometrySetSkin(geometry, skin);
    } else if (_rpSkinGeometryNativeRead(stream, geometry) == 0) {
        return 0;
    }
    return stream;
}

static RwStream* SkinAtomicRead(RwStream* stream, RwInt32 binaryLength,
                                void* object, RwInt32 offset, RwInt32 size)
{

    RwStream* result;
    RpAtomic* atomic = object;
    RpGeometry* geometry = atomic->geometry;
    RpSkin* skin = RpSkinGeometryGetSkin(geometry);

    if (skin == 0) {
        RwInt32 numBones;
        RwInt32 numVertices;
        RwUInt32 bone;

        result = RwStreamReadInt32(stream, &numBones, 4);
        if (result == 0) return 0;
        numVertices = geometry->numVertices;
        skin = SkinCreate(numVertices, numBones, numBones, 4,
                          0, 0, 0);
        result = RwStreamSkip(stream, 4);
        if (result == 0) return 0;
        result = RwStreamReadInt32(stream,
                                   (RwInt32*)skin->vertexBoneIndices,
                                   numVertices * 4);
        if (result == 0) return 0;
        result = RwStreamReadReal(stream, (RwReal*)skin->vertexBoneWeights,
                                  numVertices * 0x10);
        if (result == 0) return 0;
        for (bone = 0; bone < skin->numBones; bone++) {
            result = RwStreamSkip(stream, 0xC);
            if (result == 0) return 0;
            result = RwStreamReadReal(
                stream, (RwReal*)&skin->skinToBoneMatrices[bone], 0x40);
            if (result == 0) return 0;
        }
        SkinFindMaxWeights(skin, skin->vertexBoneWeights, numVertices);
        SkinFindNumUsedBones(skin, skin->vertexBoneIndices,
                             skin->vertexBoneWeights, skin->usedBoneList,
                             &skin->numUsedBones, numVertices);
        RpSkinGeometrySetSkin(geometry, skin);
    } else {
        result = RwStreamSkip(stream, size);
        if (result == 0) return 0;
    }
    return stream;
}

static RwStream* SkinAtomicWrite(RwStream* stream, RwInt32 binaryLength,
                                 const void* object, RwInt32 offset,
                                 RwInt32 size)
{
    return stream;
}

static RwInt32 SkinAtomicGetSize(const void* object, RwInt32 offset,
                                 RwInt32 size)
{
    RwInt32 result = 0;
    result += _rpSkinAtomicNativeSize(object);
    return result;
}

RwBool RpSkinPluginAttach(void)
{

    _rpSkinGlobals.engineOffset =
        RwEngineRegisterPlugin(0, 0x116, SkinOpen, SkinClose);
    _rpSkinGlobals.atomicOffset = RpAtomicRegisterPlugin(
        sizeof(RpSkinAtomicData), 0x116, SkinAtomicConstructor,
        SkinAtomicDestructor, SkinAtomicCopy);
    RpAtomicRegisterPluginStream(
        0x116, SkinAtomicRead, SkinAtomicWrite, SkinAtomicGetSize);
    RpAtomicSetStreamAlwaysCallBack(0x116, SkinAtomicAlways);
    RpAtomicSetStreamRightsCallBack(0x116, SkinAtomicRights);
    _rpSkinGlobals.geometryOffset = RpGeometryRegisterPlugin(
        sizeof(RpSkin*), 0x116, SkinGeometryConstructor,
        SkinGeometryDestructor, SkinGeometryCopy);
    RpGeometryRegisterPluginStream(
        0x116, SkinGeometryRead, SkinGeometryWrite, SkinGeometrySize);
    return 1;
}

RpAtomic* RpSkinAtomicSetHAnimHierarchy(RpAtomic* atomic,
                                        RpHAnimHierarchy* hierarchy)
{
    RpSkinAtomicData* data = SkinAtomicData(atomic);
    data->hierarchy = hierarchy;
    return atomic;
}

RpSkin* RpSkinGeometryGetSkin(RpGeometry* geometry)
{
    RpSkin* skin = *SkinGeometryData(geometry);
    return skin;
}

RpGeometry* RpSkinGeometrySetSkin(RpGeometry* geometry, RpSkin* skin)
{
    RpSkin* oldSkin = *SkinGeometryData(geometry);

    if (skin != oldSkin) {
        if (oldSkin != 0) {
            _rpSkinDeinitialize(geometry);
        }
        *SkinGeometryData(geometry) = skin;
        if (skin != 0 && !_rpSkinInitialize(geometry)) {
            return 0;
        }
    }
    return geometry;
}

RpSkin* RpSkinDestroy(RpSkin* skin)
{
    if (skin->skinData != 0) {
        RwEngineInstance->fpFree(skin->skinData);
    }
    _rpSkinSplitDataDestroy(skin);
    RwEngineInstance->fpFreeListFree(
        (RwFreeList*)_rpSkinGlobals.skinFreeList, skin);
    skin = 0;
    return skin;
}

RwMatrix* RpSkinGetSkinToBoneMatrices(RpSkin* skin)
{
    return skin->skinToBoneMatrices;
}

RpAtomic* RpSkinAtomicSetType(RpAtomic* atomic, RpSkinType type)
{
    atomic = SkinAtomicAttachBestPipeForAttachedPlugins(atomic, type);
    return atomic;
}
