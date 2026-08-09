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
extern void _rpSkinDeinitialize(RpGeometry* geometry);
#define SKIN_ATOMIC_DATA(atomic)                                          \
    ((RpSkinAtomicData*)((RwUInt8*)(atomic) + _rpSkinGlobals.atomicOffset))
#define SKIN_GEOMETRY_DATA(geometry)                                      \
    ((RpSkin**)((RwUInt8*)(geometry) + _rpSkinGlobals.geometryOffset))

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

    if (geometry != NULL) {
        skin = RpSkinGeometryGetSkin(geometry);
        if (skin != NULL) {
            /* Retail retains the otherwise unused pipeline result. */
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
                RwBool unique = TRUE;
                RwUInt32 bone =
                    (RwUInt8)(vertexIndices[vertex] >> (weight * 8));
                RwUInt32 index;

                for (index = 0; index < *numUsedBones; index++) {
                    if (bone == usedBoneList[index]) {
                        unique = FALSE;
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
    if (skin->skinData == NULL) {
        return FALSE;
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

    if (usedBoneList != NULL && numUsedBones != 0) {
        RwUInt32 usedBoneDataSize = numUsedBones * sizeof(RwUInt8);
        memcpy(skin->usedBoneList, usedBoneList, usedBoneDataSize);
    }
    if (skinToBoneMatrices != NULL) {
        RwUInt32 bone = numBones;
        while (bone-- != 0) {
            RwUInt32* destination =
                (RwUInt32*)&skin->skinToBoneMatrices[bone];
            const RwUInt32* source =
                (const RwUInt32*)&skinToBoneMatrices[bone];
            RwUInt32 pair = sizeof(RwMatrix) / (2 * sizeof(RwUInt32));

            /* Retail selects mtctr/bdnz for this fixed eight-pair copy; the
             * clean O0 loop retains an explicit counter comparison. */
            do {
                *destination++ = *source++;
                *destination++ = *source++;
            } while (--pair != 0);
        }
    }
    if (vertexIndices != NULL) {
        RwUInt32 vertexIndicesSize = numVertices * sizeof(RwUInt32);
        memcpy(skin->vertexBoneIndices, vertexIndices,
               vertexIndicesSize);
    }
    if (vertexWeights != NULL) {
        RwUInt32 vertexWeightsSize = numVertices * sizeof(RwMatrixWeights);
        memcpy(skin->vertexBoneWeights, vertexWeights,
               vertexWeightsSize);
    }
    return TRUE;
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
        return NULL;
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
        /* Retail stores the unused result of this lifecycle side effect. */
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
        _rpSkinGlobals.skinFreeList = NULL;
        RwEngineInstance->fpFree(_rpSkinGlobals.scratchMemory);
        _rpSkinGlobals.scratchMemory = NULL;
    }
    return instance;
}

static void* SkinGeometryConstructor(void* object, RwInt32 offset,
                                     RwInt32 size)
{
    *SKIN_GEOMETRY_DATA(object) = NULL;
    return object;
}

static void* SkinGeometryDestructor(void* object, RwInt32 offset,
                                    RwInt32 size)
{
    /* The functional body matches retail; only save-helper emission differs. */
    RpGeometry* geometry = object;
    RpSkin* skin = *SKIN_GEOMETRY_DATA(geometry);

    if (skin != NULL) {
        _rpSkinDeinitialize(geometry);
        *SKIN_GEOMETRY_DATA(geometry) = RpSkinDestroy(skin);
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
    /* Retail's checked plugin-access macro widens the saved-register set. */
    memset(SKIN_ATOMIC_DATA(object), 0, sizeof(RpSkinAtomicData));
    return object;
}

static void* SkinAtomicDestructor(void* object, RwInt32 offset, RwInt32 size)
{
    RpAtomic* atomic = object;
    RpSkinAtomicData* data = SKIN_ATOMIC_DATA(atomic);
    if (data->hierarchy != NULL) {
        data->hierarchy = NULL;
    }
    return object;
}

static void* SkinAtomicCopy(void* destination, const void* source,
                            RwInt32 offset, RwInt32 size)
{
    const RpSkinAtomicData* sourceData = SKIN_ATOMIC_DATA(source);
    RpSkinAtomicData* destinationData = SKIN_ATOMIC_DATA(destination);

    destinationData->hierarchy = sourceData->hierarchy;
    return destination;
}

static RwBool SkinAtomicAlways(void* object, RwInt32 offset, RwInt32 size)
{
    RpAtomic* atomic;
    RpSkinType type = rpSKINTYPEGENERIC;
    atomic = object;

    if (MatfxPluginIsAttached() &&
        RWPLUGINOFFSET(RwUInt8, atomic, RpAtomicGetPluginOffset(0x120)) != 0) {
        type = rpSKINTYPEMATFX;
    }
    SkinAtomicSetup(atomic, type);
    return TRUE;
}

static RwBool SkinAtomicRights(void* object, RwInt32 offset, RwInt32 size,
                               RwUInt32 extraData)
{
    RpAtomic* atomic = object;
    RpSkinType type = (RpSkinType)extraData;

    SkinAtomicSetup(atomic, type);
    return TRUE;
}

static RwInt32 SkinGeometrySize(const void* object, RwInt32 offset,
                                RwInt32 size)
{
    /* The size calculation matches retail; only save-helper emission differs. */
    RwInt32 result = 0;
    const RpGeometry* geometry = object;
    RpSkin* skin = *SKIN_GEOMETRY_DATA(geometry);

    if (skin != NULL) {
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
    const RpSkin* skin = *SKIN_GEOMETRY_DATA(geometry);

    if (skin != NULL) {
        if (!(geometry->flags & 0x01000000)) {
            RwInt32 numVertices = geometry->numVertices;
            RwInt32 header = (RwUInt8)skin->numBones |
                             (((skin->maxNumWeights << 16) & 0xFF0000) |
                              ((skin->numUsedBones << 8) & 0xFF00));

            result = RwStreamWriteInt32(stream, &header, 4);
            if (result == NULL) return NULL;
            result = RwStreamWrite(stream, skin->usedBoneList,
                                   skin->numUsedBones);
            if (result == NULL) return NULL;
            result = RwStreamWriteInt32(
                stream, (const RwInt32*)skin->vertexBoneIndices,
                numVertices * 4);
            if (result == NULL) return NULL;
            result = RwStreamWriteReal(
                stream, (const RwReal*)skin->vertexBoneWeights,
                numVertices * 0x10);
            if (result == NULL) return NULL;
            result = RwStreamWriteReal(
                stream, (const RwReal*)skin->skinToBoneMatrices,
                skin->numBones * 0x40);
            if (result == NULL) return NULL;
            result = _rpSkinSplitDataStreamWrite(stream, skin);
            if (result == NULL) return NULL;
        } else {
            if (_rpSkinGeometryNativeWrite(stream, geometry) == NULL) {
                return NULL;
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
        if (result == NULL) return NULL;
        numBones = packed & 0xFF;
        numUsedBones = (packed >> 8) & 0xFF;
        maxWeights = (packed >> 16) & 0xFF;
        numVertices = geometry->numVertices;
        if (maxWeights == 0) {
            skin = SkinCreate(numVertices, numBones, numBones, 4,
                              NULL, NULL, NULL);
            if (skin == NULL) return NULL;
        } else {
            skin = SkinCreate(numVertices, numBones, numUsedBones, maxWeights,
                              NULL, NULL, NULL);
            if (skin == NULL) return NULL;
            bytesToRead = numUsedBones;
            if (bytesToRead !=
                RwStreamRead(stream, skin->usedBoneList, bytesToRead)) {
                return NULL;
            }
        }
        result = RwStreamReadInt32(stream,
                                   (RwInt32*)skin->vertexBoneIndices,
                                   numVertices * 4);
        if (result == NULL) return NULL;
        result = RwStreamReadReal(stream, (RwReal*)skin->vertexBoneWeights,
                                  numVertices * 0x10);
        if (result == NULL) return NULL;
        if (maxWeights == 0) {
            RwUInt32 bone;
            for (bone = 0; bone < skin->numBones; bone++) {
                result = RwStreamSkip(stream, 4);
                if (result == NULL) return NULL;
                result = RwStreamReadReal(
                    stream, (RwReal*)&skin->skinToBoneMatrices[bone], 0x40);
                if (result == NULL) return NULL;
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
            if (result == NULL) return NULL;
            result = _rpSkinSplitDataStreamRead(stream, skin);
            if (result == NULL) return NULL;
        }
        RpSkinGeometrySetSkin(geometry, skin);
    } else if (_rpSkinGeometryNativeRead(stream, geometry) == NULL) {
        return NULL;
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

    if (skin == NULL) {
        RwInt32 numBones;
        RwInt32 numVertices;
        RwUInt32 bone;

        result = RwStreamReadInt32(stream, &numBones, 4);
        if (result == NULL) return NULL;
        numVertices = geometry->numVertices;
        skin = SkinCreate(numVertices, numBones, numBones, 4,
                          NULL, NULL, NULL);
        result = RwStreamSkip(stream, 4);
        if (result == NULL) return NULL;
        result = RwStreamReadInt32(stream,
                                   (RwInt32*)skin->vertexBoneIndices,
                                   numVertices * 4);
        if (result == NULL) return NULL;
        result = RwStreamReadReal(stream, (RwReal*)skin->vertexBoneWeights,
                                  numVertices * 0x10);
        if (result == NULL) return NULL;
        for (bone = 0; bone < skin->numBones; bone++) {
            result = RwStreamSkip(stream, 0xC);
            if (result == NULL) return NULL;
            result = RwStreamReadReal(
                stream, (RwReal*)&skin->skinToBoneMatrices[bone], 0x40);
            if (result == NULL) return NULL;
        }
        SkinFindMaxWeights(skin, skin->vertexBoneWeights, numVertices);
        SkinFindNumUsedBones(skin, skin->vertexBoneIndices,
                             skin->vertexBoneWeights, skin->usedBoneList,
                             &skin->numUsedBones, numVertices);
        RpSkinGeometrySetSkin(geometry, skin);
    } else {
        result = RwStreamSkip(stream, size);
        if (result == NULL) return NULL;
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
    /* Retail repeatedly overwrites an unused registration result. */
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
    return TRUE;
}

RpAtomic* RpSkinAtomicSetHAnimHierarchy(RpAtomic* atomic,
                                        RpHAnimHierarchy* hierarchy)
{
    RpSkinAtomicData* data = SKIN_ATOMIC_DATA(atomic);
    data->hierarchy = hierarchy;
    return atomic;
}

RpSkin* RpSkinGeometryGetSkin(RpGeometry* geometry)
{
    RpSkin* skin = *SKIN_GEOMETRY_DATA(geometry);
    return skin;
}

RpGeometry* RpSkinGeometrySetSkin(RpGeometry* geometry, RpSkin* skin)
{
    RpSkin* oldSkin = *SKIN_GEOMETRY_DATA(geometry);

    if (skin != oldSkin) {
        if (oldSkin != NULL) {
            _rpSkinDeinitialize(geometry);
        }
        *SKIN_GEOMETRY_DATA(geometry) = skin;
        if (skin != NULL && !_rpSkinInitialize(geometry)) {
            return NULL;
        }
    }
    return geometry;
}

RpSkin* RpSkinDestroy(RpSkin* skin)
{
    if (skin->skinData != NULL) {
        RwEngineInstance->fpFree(skin->skinData);
    }
    _rpSkinSplitDataDestroy(skin);
    RwEngineInstance->fpFreeListFree(
        (RwFreeList*)_rpSkinGlobals.skinFreeList, skin);
    skin = NULL;
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
