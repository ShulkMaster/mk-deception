#include "rw/rwengine.h"
#include "runtime/cstring.h"
#include "rw/rphanim.h"
#include "rw/rpskin.h"
#include "rw/rpworld_types.h"
#include "rw/rwfreelist.h"
#include "rw/rwstream.h"

static RwFreeList _rpSkinFreeList;
static int _rpSkinFreeListBlockSize = 0x14;
static int _rpSkinFreeListPreallocBlocks = 1;

extern RpGeometry* _rpSkinInitialize(RpGeometry* geometry);
extern RpGeometry* _rpSkinDeinitialize(RpGeometry* geometry);
static int MatfxPluginIsAttached(void)
{
    return RwEngineGetPluginOffset(0x120) != -1;
}

static int ToonPluginIsAttached(void)
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
                               unsigned int numVertices)
{
    unsigned int vertex;

    skin->maxNumWeights = 1;
    for (vertex = 0; vertex < numVertices; vertex++) {
        unsigned int weight;
        for (weight = skin->maxNumWeights; weight < 4; weight++) {
            if (((const unsigned int*)&vertexWeights[vertex])[weight] != 0) {
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

static void SkinFindNumUsedBones(RpSkin* skin, const unsigned int* vertexIndices,
                                 const RwMatrixWeights* vertexWeights,
                                 unsigned char* usedBoneList,
                                 unsigned int* numUsedBones,
                                 unsigned int numVertices)
{
    unsigned int vertex;

    *numUsedBones = 0;
    for (vertex = 0; vertex < numVertices; vertex++) {
        unsigned int weight;
        for (weight = 0; weight < skin->maxNumWeights; weight++) {
            if (((const unsigned int*)&vertexWeights[vertex])[weight] != 0) {
                unsigned int bone;
                int unique = 1;
                unsigned int index;

                bone = (unsigned char)(vertexIndices[vertex] >> (weight * 8));
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

static int SkinCreateSkinData(
    RpSkin* skin, unsigned int numBones, unsigned int numUsedBones,
    unsigned int numVertices, const unsigned char* usedBoneList,
    const RwMatrixWeights* vertexWeights, const unsigned int* vertexIndices,
    const RwMatrix* skinToBoneMatrices)
{
    unsigned int allocationSize =
        numVertices * (sizeof(unsigned int) + sizeof(RwMatrixWeights)) +
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
        (((unsigned int)skin->usedBoneList + numUsedBones + 15) & ~15U);
    skin->vertexBoneIndices =
        (unsigned int*)((unsigned char*)skin->skinToBoneMatrices +
                   numBones * sizeof(RwMatrix));
    skin->vertexBoneWeights =
        (RwMatrixWeights*)(skin->vertexBoneIndices + numVertices);

    if (usedBoneList != 0 && numUsedBones != 0) {
        unsigned int usedBoneDataSize = numUsedBones * sizeof(unsigned char);
        memcpy(skin->usedBoneList, usedBoneList, usedBoneDataSize);
    }
    if (skinToBoneMatrices != 0) {
        unsigned int bone = numBones;
        while (bone-- != 0) {
            unsigned int* destination =
                (unsigned int*)&skin->skinToBoneMatrices[bone];
            const unsigned int* source =
                (const unsigned int*)&skinToBoneMatrices[bone];
            unsigned int pair = sizeof(RwMatrix) / (2 * sizeof(unsigned int));



            do {
                *destination++ = *source++;
                *destination++ = *source++;
            } while (--pair != 0);
        }
    }
    if (vertexIndices != 0) {
        unsigned int vertexIndicesSize = numVertices * sizeof(unsigned int);
        memcpy(skin->vertexBoneIndices, vertexIndices,
               vertexIndicesSize);
    }
    if (vertexWeights != 0) {
        unsigned int vertexWeightsSize = numVertices * sizeof(RwMatrixWeights);
        memcpy(skin->vertexBoneWeights, vertexWeights,
               vertexWeightsSize);
    }
    return 1;
}

static RpSkin* SkinCreate(unsigned int numVertices, unsigned int numBones,
                          unsigned int numUsedBones, unsigned int maxNumWeights,
                          const RwMatrixWeights* vertexWeights,
                          const unsigned int* vertexIndices,
                          const RwMatrix* skinToBoneMatrices)
{
    unsigned char usedBoneList[256];
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

static void* SkinOpen(void* instance, int offset, int size)
{
    if (_rpSkinGlobals.numInstances == 0) {
        unsigned int pipelineTypes = rpSKINTYPEGENERIC;
        unsigned int scratchSize;

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
            (((unsigned int)_rpSkinGlobals.scratchMemory + 15) & ~15U);
    }
    _rpSkinGlobals.numInstances++;
    return instance;
}

static void* SkinClose(void* instance, int offset, int size)
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

static void* SkinGeometryConstructor(void* object, int offset,
                                     int size)
{
    *(RpSkin**)((unsigned char*)object + _rpSkinGlobals.geometryOffset) = 0;
    return object;
}

static void* SkinGeometryDestructor(void* object, int offset,
                                    int size)
{

    RpGeometry* geometry = object;
    RpSkin* skin = *(RpSkin**)((unsigned char*)geometry +
                               _rpSkinGlobals.geometryOffset);

    if (skin != 0) {
        _rpSkinDeinitialize(geometry);
        *(RpSkin**)((unsigned char*)geometry + _rpSkinGlobals.geometryOffset) =
            RpSkinDestroy(skin);
    }
    return object;
}

static void* SkinGeometryCopy(void* destination, const void* source,
                              int offset, int size)
{
    return destination;
}

static void* SkinAtomicConstructor(void* object, int offset, int size)
{
    SkinAtomicState* data =
        (SkinAtomicState*)((unsigned char*)object + _rpSkinGlobals.atomicOffset);

    memset(data, 0, sizeof(SkinAtomicState));
    return object;
}

static void* SkinAtomicDestructor(void* object, int offset, int size)
{
    RpAtomic* atomic = object;
    SkinAtomicState* data =
        (SkinAtomicState*)((unsigned char*)atomic + _rpSkinGlobals.atomicOffset);
    if (data->hierarchy != 0) {
        data->hierarchy = 0;
    }
    return object;
}

static void* SkinAtomicCopy(void* destination, const void* source,
                            int offset, int size)
{
    const SkinAtomicState* sourceData =
        (const SkinAtomicState*)((const unsigned char*)source +
                                  _rpSkinGlobals.atomicOffset);
    SkinAtomicState* destinationData =
        (SkinAtomicState*)((unsigned char*)destination +
                            _rpSkinGlobals.atomicOffset);

    destinationData->hierarchy = sourceData->hierarchy;
    return destination;
}

static int SkinAtomicAlways(void* object, int offset, int size)
{
    RpAtomic* atomic;
    RpSkinType type = rpSKINTYPEGENERIC;
    atomic = object;

    if (MatfxPluginIsAttached() &&
        *(unsigned char*)((unsigned char*)atomic + RpAtomicGetPluginOffset(0x120)) != 0) {
        type = rpSKINTYPEMATFX;
    }
    SkinAtomicSetup(atomic, type);
    return 1;
}

static int SkinAtomicRights(void* object, int offset, int size,
                               unsigned int extraData)
{
    RpAtomic* atomic = object;
    RpSkinType type = (RpSkinType)extraData;

    SkinAtomicSetup(atomic, type);
    return 1;
}

static int SkinGeometrySize(const void* object, int offset,
                                int size)
{

    int result = 0;
    const RpGeometry* geometry = object;
    RpSkin* skin = *(RpSkin**)((unsigned char*)geometry +
                               _rpSkinGlobals.geometryOffset);

    if (skin != 0) {
        if (!(geometry->flags & 0x01000000)) {
            int numVertices = geometry->numVertices;
            result = 4;
            result += skin->numUsedBones;
            result += numVertices * sizeof(unsigned int);
            result += numVertices * sizeof(RwMatrixWeights);
            result += skin->numBones * sizeof(RwMatrix);
            result += _rpSkinSplitDataStreamGetSize(skin);
        } else {
            result = _rpSkinGeometryNativeSize(geometry);
        }
    }
    return result;
}

static RwStream* SkinGeometryWrite(RwStream* stream, int binaryLength,
                                   const void* object, int offset,
                                   int size)
{
    RwStream* result;
    const RpGeometry* geometry = object;
    const RpSkin* skin = *(RpSkin* const*)((const unsigned char*)geometry +
                                           _rpSkinGlobals.geometryOffset);

    if (skin != 0) {
        if (!(geometry->flags & 0x01000000)) {
            int numVertices = geometry->numVertices;
            int header = (unsigned char)skin->numBones |
                             (((skin->maxNumWeights << 16) & 0xFF0000) |
                              ((skin->numUsedBones << 8) & 0xFF00));

            result = RwStreamWriteInt32(stream, &header, 4);
            if (result == 0) return 0;
            result = RwStreamWrite(stream, skin->usedBoneList,
                                   skin->numUsedBones);
            if (result == 0) return 0;
            result = RwStreamWriteInt32(
                stream, (const int*)skin->vertexBoneIndices,
                numVertices * 4);
            if (result == 0) return 0;
            result = RwStreamWriteReal(
                stream, (const float*)skin->vertexBoneWeights,
                numVertices * 0x10);
            if (result == 0) return 0;
            result = RwStreamWriteReal(
                stream, (const float*)skin->skinToBoneMatrices,
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

static RwStream* SkinGeometryRead(RwStream* stream, int binaryLength,
                                  void* object, int offset, int size)
{
    RwStream* result;
    RpGeometry* geometry = object;
    RpSkin* skin;

    if (!(geometry->flags & 0x01000000)) {
        unsigned int packed;
        unsigned int numBones;
        unsigned int numUsedBones;
        unsigned int maxWeights;
        int numVertices;
        unsigned int bytesToRead;

        result = RwStreamReadInt32(stream, (int*)&packed, 4);
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
                                   (int*)skin->vertexBoneIndices,
                                   numVertices * 4);
        if (result == 0) return 0;
        result = RwStreamReadReal(stream, (float*)skin->vertexBoneWeights,
                                  numVertices * 0x10);
        if (result == 0) return 0;
        if (maxWeights == 0) {
            unsigned int bone;
            for (bone = 0; bone < skin->numBones; bone++) {
                result = RwStreamSkip(stream, 4);
                if (result == 0) return 0;
                result = RwStreamReadReal(
                    stream, (float*)&skin->skinToBoneMatrices[bone], 0x40);
                if (result == 0) return 0;
            }
            SkinFindMaxWeights(skin, skin->vertexBoneWeights, numVertices);
            SkinFindNumUsedBones(skin, skin->vertexBoneIndices,
                                 skin->vertexBoneWeights, skin->usedBoneList,
                                 &skin->numUsedBones, numVertices);
        } else {
            skin->maxNumWeights = maxWeights;
            result = RwStreamReadReal(stream,
                                      (float*)skin->skinToBoneMatrices,
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

static RwStream* SkinAtomicRead(RwStream* stream, int binaryLength,
                                void* object, int offset, int size)
{

    RwStream* result;
    RpAtomic* atomic = object;
    RpGeometry* geometry = atomic->geometry;
    RpSkin* skin = RpSkinGeometryGetSkin(geometry);

    if (skin == 0) {
        int numBones;
        int numVertices;
        unsigned int bone;

        result = RwStreamReadInt32(stream, &numBones, 4);
        if (result == 0) return 0;
        numVertices = geometry->numVertices;
        skin = SkinCreate(numVertices, numBones, numBones, 4,
                          0, 0, 0);
        result = RwStreamSkip(stream, 4);
        if (result == 0) return 0;
        result = RwStreamReadInt32(stream,
                                   (int*)skin->vertexBoneIndices,
                                   numVertices * 4);
        if (result == 0) return 0;
        result = RwStreamReadReal(stream, (float*)skin->vertexBoneWeights,
                                  numVertices * 0x10);
        if (result == 0) return 0;
        for (bone = 0; bone < skin->numBones; bone++) {
            result = RwStreamSkip(stream, 0xC);
            if (result == 0) return 0;
            result = RwStreamReadReal(
                stream, (float*)&skin->skinToBoneMatrices[bone], 0x40);
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

static RwStream* SkinAtomicWrite(RwStream* stream, int binaryLength,
                                 const void* object, int offset,
                                 int size)
{
    return stream;
}

static int SkinAtomicGetSize(const void* object, int offset,
                                 int size)
{
    int result = 0;
    result += _rpSkinAtomicNativeSize(object);
    return result;
}

int RpSkinPluginAttach(void)
{

    _rpSkinGlobals.engineOffset =
        RwEngineRegisterPlugin(0, 0x116, SkinOpen, SkinClose);
    _rpSkinGlobals.atomicOffset = RpAtomicRegisterPlugin(
        sizeof(SkinAtomicState), 0x116, SkinAtomicConstructor,
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
    SkinAtomicState* data = (SkinAtomicState*)((unsigned char*)atomic +
                                                 _rpSkinGlobals.atomicOffset);
    data->hierarchy = hierarchy;
    return atomic;
}

RpSkin* RpSkinGeometryGetSkin(RpGeometry* geometry)
{
    RpSkin* skin = *(RpSkin**)((unsigned char*)geometry +
                               _rpSkinGlobals.geometryOffset);
    return skin;
}

RpGeometry* RpSkinGeometrySetSkin(RpGeometry* geometry, RpSkin* skin)
{
    RpSkin* oldSkin = *(RpSkin**)((unsigned char*)geometry +
                                  _rpSkinGlobals.geometryOffset);

    if (skin != oldSkin) {
        if (oldSkin != 0) {
            _rpSkinDeinitialize(geometry);
        }
        *(RpSkin**)((unsigned char*)geometry + _rpSkinGlobals.geometryOffset) = skin;
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
