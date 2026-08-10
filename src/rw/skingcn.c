#include "dolphin/gx.h"
#include "dolphin/os.h"
#include "libmkparticle/rw_engine.h"
#include "rw/alphapass.h"
#include "rw/gamecube.h"
#include "rw/rphanim.h"
#include "rw/rpskin.h"
#include "rw/rtquat.h"
#include "rw/rwresources.h"
#include "rw/rwvector.h"

typedef struct RpSkinAtomicData {
    RpHAnimHierarchy* hierarchy;
    void* positions;
    void* normals;
} RpSkinAtomicData;

typedef struct RwGameCubeResEntryHeader {
    RwResEntry entry;
    RwUInt16 token;
    RwUInt16 meshSerialNum;
} RwGameCubeResEntryHeader;

typedef struct RwResourcesGlobalsPrefix {
    RwUInt32 arenaSize;
    RwUInt32 arenaUsage;
    RwUInt32 arenaReusage;
    void* arena;
    RwLinkList entriesA;
    RwLinkList entriesB;
    RwLLLink* activeList;
} RwResourcesGlobalsPrefix;

typedef struct RxGameCubeAllInOneInstanceData {
    RwResEntry* resourceEntry;
    RpMeshHeader* meshHeader;
    RwInt32 geometryFlags;
    RwRGBAReal ambient;
    RwBool hasAmbient;
    RwUInt32 lightMask;
    RwInt32 lightIndex;
    void* morphData;
} RxGameCubeAllInOneInstanceData;

typedef union SpecularMaterialFlags {
    RwUInt8 value;
    struct {
        signed char hidden : 1;
        signed char reflectionPass : 1;
        signed char cullFront : 1;
        signed char swapMode : 1;
        unsigned char reserved : 4;
    } bits;
} SpecularMaterialFlags;

typedef struct SpecularMaterialData {
    RwUInt8 reserved_0x00[0x2C];
    SpecularMaterialFlags flags;
    RwUInt8 reserved_0x2D[3];
} SpecularMaterialData;

typedef struct RpSkinBlendPositionData {
    RwUInt8* destination;
    RwUInt8* source;
    RwUInt32 stride;
    RwUInt32 numVertices;
} RpSkinBlendPositionData;

typedef struct RpSkinBlendPositionNormalData {
    RwUInt8* destinationPositions;
    RwUInt8* destinationNormals;
    RwUInt8* sourcePositions;
    RwUInt8* sourceNormals;
    RwUInt32 positionStride;
    RwUInt32 normalStride;
    RwUInt32 nbtStride;
    RwUInt32 numVertices;
} RpSkinBlendPositionNormalData;

extern RwModuleInfo resourcesModule;
extern RwUInt16 _RwDlTokenCurrent;
extern RwInt32 _RwDlPreInstanceOptimize;
extern RwInt32 _rpDlGeomVtxFmtOffset;
extern RwInt32 _RwGameCubeRasterExtOffset;
extern RwInt32 SpecularMaterialOffset;
extern RwMatrix _RwDlInvCamLTM;
extern RwInt32 _rwDlTokenQueryDone(RwUInt16 token);
extern void _rxGCAtomicDefaultReinstanceCallback(void*, RwResEntry**);
extern RwMatrix* RwMatrixInvert(RwMatrix*, const RwMatrix*);
extern RwMatrix* RwMatrixMultiply(RwMatrix*, const RwMatrix*,
                                  const RwMatrix*);
extern void _rwDlSkinUpdate2WeightsP(const RwMatrix*, const RpSkin*,
                                     const RpSkinBlendPositionData*);
extern void _rwDlSkinUpdate3WeightsP(const RwMatrix*, const RpSkin*,
                                     const RpSkinBlendPositionData*);
extern void _rwDlSkinUpdate4WeightsP(const RwMatrix*, const RpSkin*,
                                     const RpSkinBlendPositionData*);
extern void _rwDlSkinUpdate2WeightsPN(
    const RwMatrix*, const RpSkin*, const RpSkinBlendPositionNormalData*);
extern void _rwDlSkinUpdate3WeightsPN(
    const RwMatrix*, const RpSkin*, const RpSkinBlendPositionNormalData*);
extern void _rwDlSkinUpdate4WeightsPN(
    const RwMatrix*, const RpSkin*, const RpSkinBlendPositionNormalData*);

RpSkinGlobals _rpSkinGlobals = {0, 0, 0, NULL, NULL, 0, NULL, 0, 0,
                                {NULL, NULL, NULL, NULL, NULL, NULL}};

static const RwUInt8 skinVertexSizes[5] = {3, 3, 6, 6, 12};

#define SKIN_ATOMIC_DATA(atomic)                                          \
    ((RpSkinAtomicData*)((RwUInt8*)(atomic) + _rpSkinGlobals.atomicOffset))
#define SKIN_GEOMETRY_DATA(geometry)                                      \
    (*(RpSkin**)((RwUInt8*)(geometry) + _rpSkinGlobals.geometryOffset))
#define GEOMETRY_VTX_FORMAT(geometry)                                     \
    (*(RpGameCubeVtxFmt**)((RwUInt8*)(geometry) + _rpDlGeomVtxFmtOffset))
#define SPECULAR_DATA(material)                                           \
    ((SpecularMaterialData*)((RwUInt8*)(material) + SpecularMaterialOffset))
#define RESOURCES_GLOBALS                                                 \
    (*(RwResourcesGlobalsPrefix*)((RwUInt8*)RwEngineInstance +            \
                                  resourcesModule.globalsOffset))

static void ActivateResourceEntry(RwResEntry* entry)
{
    RwLLLink* link = &entry->link;
    RwLLLink* head = RESOURCES_GLOBALS.activeList;

    if (link->next != NULL) {
        link->prev->next = link->next;
        link->next->prev = link->prev;
        link->next = head->next;
        link->prev = head;
        head->next->prev = link;
        head->next = link;
    }
}

static void _rpSkinMainResEntryCB(RwResEntry* entry)
{
    RwGameCubeResEntryHeader* header = (RwGameCubeResEntryHeader*)entry;
    RwGameCubeVertexBuffer* vertexBuffer =
        (RwGameCubeVertexBuffer*)(header + 1);
    RwObject* owner;
    RwUInt32 ownerFlags;

    owner = (RwObject*)entry->owner;
    ownerFlags = *(RwUInt32*)((RwUInt8*)owner + 8);

    if (header->token == _RwDlTokenCurrent) {
        GXSetDrawSync(_RwDlTokenCurrent);
        _RwDlTokenCurrent = (_RwDlTokenCurrent + 1) % 57344;
    }
    while (_rwDlTokenQueryDone(header->token) == FALSE) {
    }
    if (vertexBuffer->arrays[0].data != NULL) {
        RwResourcesFreeResEntry(
            *(RwResEntry**)((RwUInt8*)vertexBuffer->arrays[0].data - 4));
    }
    if (owner->type == 1) {
        RpSkinAtomicData* atomicData =
            (RpSkinAtomicData*)((RwUInt8*)owner + _rpSkinGlobals.atomicOffset);
        vertexBuffer->arrays[0].data = atomicData->positions;
        if ((ownerFlags & 0x10) != 0)
            vertexBuffer->arrays[1].data = atomicData->normals;
        atomicData->positions = NULL;
        atomicData->normals = NULL;
    } else {
        RpSkin* skin =
            *(RpSkin**)((RwUInt8*)owner + _rpSkinGlobals.geometryOffset);
        vertexBuffer->arrays[0].data = skin->nativeData;
        if ((ownerFlags & 0x10) != 0)
            vertexBuffer->arrays[1].data = skin->nativeData2;
        skin->nativeData = NULL;
        skin->nativeData2 = NULL;
    }
}

static void _rpSkinResEntryWaitDone(RwResEntry* entry)
{
    RwGameCubeResEntryHeader* header = (RwGameCubeResEntryHeader*)entry;

    if (header->token == _RwDlTokenCurrent) {
        GXSetDrawSync(_RwDlTokenCurrent);
        _RwDlTokenCurrent = (_RwDlTokenCurrent + 1) % 57344;
    }
    while (_rwDlTokenQueryDone(header->token) == FALSE) {
    }
}

RwBool _rpSkinVertexBuffersUpdate(RpSkin* skin, RpAtomic* atomic,
                                  RwGameCubeVertexBuffer* vertexBuffer,
                                  RwResEntry** resourceEntry)
{
    RpGeometry* geometry = atomic->geometry;
    RpGameCubeVtxFmt* format;
    RwUInt32 size;
    RwUInt32 numVertices;
    RwResEntry* entry;

    ActivateResourceEntry(*resourceEntry);
    (*resourceEntry)->destroyNotify = _rpSkinMainResEntryCB;
    if ((*resourceEntry)->owner != NULL &&
        ((RwObject*)(*resourceEntry)->owner)->type == 1) {
        RpSkinAtomicData* atomicData = SKIN_ATOMIC_DATA(atomic);
        if (atomicData->positions == NULL) {
            atomicData->positions = vertexBuffer->arrays[0].data;
            vertexBuffer->arrays[0].data = NULL;
            if ((geometry->flags & 0x10) != 0) {
                atomicData->normals = vertexBuffer->arrays[1].data;
                vertexBuffer->arrays[1].data = NULL;
            }
        }
    } else if (skin->nativeData == NULL) {
        skin->nativeData = vertexBuffer->arrays[0].data;
        vertexBuffer->arrays[0].data = NULL;
        if ((geometry->flags & 0x10) != 0) {
            skin->nativeData2 = vertexBuffer->arrays[1].data;
            vertexBuffer->arrays[1].data = NULL;
        }
    }
    if (vertexBuffer->arrays[0].data != NULL) {
        RwResEntry* oldEntry =
            *(RwResEntry**)((RwUInt8*)vertexBuffer->arrays[0].data - 4);
        if (_rwDlTokenQueryDone(
                ((RwGameCubeResEntryHeader*)oldEntry)->token) == FALSE) {
            oldEntry->ownerRef = NULL;
            vertexBuffer->arrays[0].data = NULL;
            if ((geometry->flags & 0x10) != 0)
                vertexBuffer->arrays[1].data = NULL;
        }
    }
    if (vertexBuffer->arrays[0].data == NULL) {
        format = GEOMETRY_VTX_FORMAT(geometry);
        if (format != NULL) {
            size = (geometry->numVertices *
                    skinVertexSizes[format->positionType] + 31) & ~31U;
            size += 6;
            if ((geometry->flags & 0x10) != 0) {
                RwUInt32 components = format->normalMode != 0 ? 3 : 1;
                size += (geometry->numVertices *
                         skinVertexSizes[format->normalType] * components + 31) &
                        ~31U;
            }
        } else {
            size = (geometry->numVertices * sizeof(RwV3d) + 31) & ~31U;
            size += 6;
            if ((geometry->flags & 0x10) != 0)
                size += (geometry->numVertices * sizeof(RwV3d) + 31) & ~31U;
        }
        entry = RwResourcesAllocateResEntry(
            atomic, (RwResEntry**)&vertexBuffer->arrays[0].data, size,
            _rpSkinResEntryWaitDone);
        numVertices = geometry->numMorphTargets == 1
                          ? (RwUInt32)geometry->numVertices
                          : (RwUInt32)atomic->interpolator.position;
        if (entry == NULL || numVertices == 0) {
            if (entry != NULL)
                RwResourcesFreeResEntry(entry);
            return FALSE;
        }
        vertexBuffer->arrays[0].data = (void*)
            (((RwUInt32)vertexBuffer->arrays[0].data + 0x3D) & ~31U);
        if ((geometry->flags & 0x10) != 0) {
            vertexBuffer->arrays[1].data =
                (RwUInt8*)vertexBuffer->arrays[0].data +
                ((geometry->numVertices * vertexBuffer->arrays[0].stride + 31) &
                 ~31U);
        }
        *(RwResEntry**)((RwUInt8*)vertexBuffer->arrays[0].data - 4) = entry;
    } else {
        entry = *(RwResEntry**)((RwUInt8*)vertexBuffer->arrays[0].data - 4);
        ActivateResourceEntry(entry);
    }
    ((RwGameCubeResEntryHeader*)entry)->token = _RwDlTokenCurrent;
    return TRUE;
}

void _rpSkinMatrixBlendUpdate(RwMatrix* destination, RpSkin* skin,
                              const RwMatrix* ltm,
                              RpHAnimHierarchy* hierarchy)
{
    RwMatrix inverse;
    RwMatrix temporary;
    const RwMatrix* transform;
    RwUInt32 i;

    if (hierarchy == NULL)
        return;
    if ((hierarchy->flags & rpHANIMHIERARCHYNOMATRICES) != 0) {
        if (skin->maxNumWeights > 1 || skin->splitData.numMeshes == 3) {
            RwMatrixInvert(&inverse, ltm);
            transform = &inverse;
        } else {
            transform = &_RwDlInvCamLTM;
        }
        for (i = 0; i < skin->numUsedBones; i++) {
            RwUInt32 bone = skin->usedBoneList[i];
            RwFrame* frame = hierarchy->pNodeInfo[bone].pFrame;
            RwMatrixMultiply(&temporary, &skin->skinToBoneMatrices[bone],
                             RwFrameGetLTM(frame));
            RwMatrixMultiply(&destination[bone], &temporary, transform);
        }
    } else if ((hierarchy->flags & rpHANIMHIERARCHYLOCALSPACEMATRICES) != 0) {
        if (skin->maxNumWeights > 1 || skin->splitData.numMeshes == 3) {
            for (i = 0; i < skin->numUsedBones; i++) {
                RwUInt32 bone = skin->usedBoneList[i];
                RwMatrixMultiply(&destination[bone],
                                 &skin->skinToBoneMatrices[bone],
                                 &hierarchy->pMatrixArray[bone]);
            }
        } else {
            RwMatrixMultiply(&temporary, ltm, &_RwDlInvCamLTM);
            _rpSkinMatrixBlendUpdateASM(
                destination, skin->skinToBoneMatrices,
                hierarchy->pMatrixArray, &temporary, skin->usedBoneList,
                skin->numUsedBones);
        }
    } else {
        if (skin->maxNumWeights > 1 || skin->splitData.numMeshes == 3) {
            RwMatrixInvert(&inverse, ltm);
            transform = &inverse;
        } else {
            transform = &_RwDlInvCamLTM;
        }
        _rpSkinMatrixBlendUpdateASM(destination, skin->skinToBoneMatrices,
                                    hierarchy->pMatrixArray, transform,
                                    skin->usedBoneList, skin->numUsedBones);
    }
}

static RwUInt32 SkinVertexScalarSize(RwUInt8 type)
{
    static const RwUInt8 sizes[5] = {4, 1, 1, 2, 2};
    return sizes[type];
}

void _rpSkinBlendBodyP(RpSkin* skin, const RwMatrix* matrices,
                       RwUInt8* source, RwUInt8* destination,
                       const RpGameCubeVtxFmt* format,
                       RwUInt32 numVertices)
{
    RpSkinBlendPositionData data;
    RwUInt32 stride = format != NULL
                          ? SkinVertexScalarSize(format->positionType) * 3
                          : sizeof(RwReal);
    RwUInt32 i;

    data.destination = destination;
    data.source = source;
    data.stride = stride;
    data.numVertices = numVertices;
    switch (skin->maxNumWeights) {
    case 1:
        for (i = 0; i < numVertices; i++) {
            RwUInt8 bone = (RwUInt8)skin->vertexBoneIndices[i];
            RwV3dTransformPoint((RwV3d*)destination, (RwV3d*)source,
                                &matrices[bone]);
            source += stride;
            destination += stride;
        }
        break;
    case 2:
        _rwDlSkinUpdate2WeightsP(matrices, skin, &data);
        break;
    case 3:
        _rwDlSkinUpdate3WeightsP(matrices, skin, &data);
        break;
    case 4:
        _rwDlSkinUpdate4WeightsP(matrices, skin, &data);
        break;
    }
    DCFlushRange(data.destination,
                 (numVertices * stride * 3 + 31) & ~31U);
    GXInvalidateVtxCache();
}

void _rpSkinBlendBodyPN(RpSkin* skin, const RwMatrix* matrices,
                        RwUInt8* sourcePositions, RwUInt8* sourceNormals,
                        RwUInt8* destinationPositions,
                        RwUInt8* destinationNormals,
                        const RpGameCubeVtxFmt* format,
                        RwUInt32 numVertices)
{
    RpSkinBlendPositionNormalData data;
    RwUInt32 posStride = format != NULL
                            ? SkinVertexScalarSize(format->positionType) * 3
                            : sizeof(RwReal);
    RwUInt32 normalStride = format != NULL
                               ? SkinVertexScalarSize(format->normalType) * 3
                               : sizeof(RwReal);
    RwUInt32 nbtStride = format != NULL && format->normalMode != 0
                            ? normalStride * 2
                            : 0;
    RwUInt32 i;

    data.destinationPositions = destinationPositions;
    data.destinationNormals = destinationNormals;
    data.sourcePositions = sourcePositions;
    data.sourceNormals = sourceNormals;
    data.positionStride = posStride;
    data.normalStride = normalStride;
    data.nbtStride = nbtStride;
    data.numVertices = numVertices;
    switch (skin->maxNumWeights) {
    case 1:
        for (i = 0; i < numVertices; i++) {
            RwUInt8 bone = (RwUInt8)skin->vertexBoneIndices[i];
            RwV3dTransformPoint((RwV3d*)destinationPositions,
                                (RwV3d*)sourcePositions, &matrices[bone]);
            RwV3dTransformVector((RwV3d*)destinationNormals,
                                 (RwV3d*)sourceNormals, &matrices[bone]);
            sourcePositions += posStride;
            sourceNormals += normalStride + nbtStride;
            destinationPositions += posStride;
            destinationNormals += normalStride + nbtStride;
        }
        break;
    case 2:
        _rwDlSkinUpdate2WeightsPN(matrices, skin, &data);
        break;
    case 3:
        _rwDlSkinUpdate3WeightsPN(matrices, skin, &data);
        break;
    case 4:
        _rwDlSkinUpdate4WeightsPN(matrices, skin, &data);
        break;
    }
    DCFlushRange(data.destinationPositions,
                 ((numVertices * posStride * 3 + 31) & ~31U) +
                     numVertices * (normalStride * 3 + nbtStride));
    GXInvalidateVtxCache();
}

void* _rpSkinInstanceCallback(void* object, RwResEntry** resourceEntry)
{
    RpAtomic* atomic = (RpAtomic*)object;
    RpGeometry* geometry = atomic->geometry;
    RpSkin* skin = RpSkinGeometryGetSkin(geometry);
    void* owner;
    RwResEntry** ownerRef;

    if (geometry->numMorphTargets != 1) {
        owner = atomic;
        ownerRef = (RwResEntry**)&atomic->repEntry;
    } else {
        owner = geometry;
        ownerRef = &geometry->repEntry;
    }
    if (skin->maxNumWeights > 1 || skin->splitData.numMeshes == 3) {
        if ((geometry->flags & 0x02000000) != 0 &&
            _RwDlPreInstanceOptimize == TRUE)
            *resourceEntry = _rwDlGeometrySkinInstanceOptimized(
                geometry, owner, ownerRef);
        else
            *resourceEntry = _rwDlGeometryInstanceFast(geometry, owner,
                                                        ownerRef);
    } else if ((geometry->flags & 0x02000000) != 0 &&
               _RwDlPreInstanceOptimize == TRUE) {
        *resourceEntry = _rwDlGeometrySkinInstanceOptimized(
            geometry, owner, ownerRef);
    } else {
        *resourceEntry = _rwDlGeometrySkinInstanceFast(geometry, owner,
                                                        ownerRef);
    }
    geometry->lockedSinceLastInst = 0;
    return object;
}

void* _rpSkinAtomicReinstanceCallBack(void* object,
                                      RwResEntry** resourceEntry)
{
    RpAtomic* atomic = (RpAtomic*)object;
    RpGeometry* geometry = atomic->geometry;
    RpSkin* skin = SKIN_GEOMETRY_DATA(geometry);

    if (skin->maxNumWeights > 1 || skin->splitData.numMeshes == 3) {
        RwGameCubeVertexBuffer* vertexBuffer =
            (RwGameCubeVertexBuffer*)((RwUInt8*)*resourceEntry + 0x18);
        void** cachedPositions;
        void** cachedNormals;

        if ((*resourceEntry)->owner != NULL &&
            ((RwObject*)(*resourceEntry)->owner)->type == 1) {
            RpSkinAtomicData* atomicData = SKIN_ATOMIC_DATA(atomic);
            cachedPositions = &atomicData->positions;
            cachedNormals = &atomicData->normals;
        } else {
            cachedPositions = &skin->nativeData;
            cachedNormals = &skin->nativeData2;
        }
        if (*cachedPositions == NULL) {
            if ((geometry->flags & 0x01000000) == 0)
                _rxGCAtomicDefaultReinstanceCallback(object, resourceEntry);
            *cachedPositions = vertexBuffer->arrays[0].data;
            vertexBuffer->arrays[0].data = NULL;
            if ((geometry->flags & 0x10) != 0) {
                *cachedNormals = vertexBuffer->arrays[1].data;
                vertexBuffer->arrays[1].data = NULL;
            }
        } else if ((geometry->flags & 0x01000000) == 0) {
            void* position = vertexBuffer->arrays[0].data;
            void* normal = vertexBuffer->arrays[1].data;
            vertexBuffer->arrays[0].data = *cachedPositions;
            if ((geometry->flags & 0x10) != 0)
                vertexBuffer->arrays[1].data = *cachedNormals;
            _rxGCAtomicDefaultReinstanceCallback(object, resourceEntry);
            vertexBuffer->arrays[0].data = position;
            if ((geometry->flags & 0x10) != 0)
                vertexBuffer->arrays[1].data = normal;
        }
        if (!_rpSkinVertexBuffersUpdate(skin, atomic, vertexBuffer,
                                        resourceEntry))
            return NULL;
        _rpSkinMatrixBlendUpdate(_rpSkinGlobals.alignedScratchMemory, skin,
                                 RwFrameGetLTM(atomic->object.parent),
                                 SKIN_ATOMIC_DATA(atomic)->hierarchy);
        if ((geometry->flags & 0x10) != 0)
            _rpSkinBlendBodyPN(
                skin, _rpSkinGlobals.alignedScratchMemory, *cachedPositions,
                *cachedNormals, vertexBuffer->arrays[0].data,
                vertexBuffer->arrays[1].data, GEOMETRY_VTX_FORMAT(geometry),
                geometry->numVertices);
        else
            _rpSkinBlendBodyP(
                skin, _rpSkinGlobals.alignedScratchMemory, *cachedPositions,
                vertexBuffer->arrays[0].data, GEOMETRY_VTX_FORMAT(geometry),
                geometry->numVertices);
    } else {
        _rpSkinMatrixBlendUpdate(_rpSkinGlobals.alignedScratchMemory, skin,
                                 RwFrameGetLTM(atomic->object.parent),
                                 SKIN_ATOMIC_DATA(atomic)->hierarchy);
    }
    return object;
}

void _rpSkinLoadMatrix(const RwMatrix* matrix, RwUInt32 index,
                       RwBool normals)
{
    Mtx gxMatrix;

    gxMatrix[0][0] = -matrix->right.x;
    gxMatrix[0][1] = -matrix->up.x;
    gxMatrix[0][2] = -matrix->at.x;
    gxMatrix[0][3] = -matrix->pos.x;
    gxMatrix[1][0] = matrix->right.y;
    gxMatrix[1][1] = matrix->up.y;
    gxMatrix[1][2] = matrix->at.y;
    gxMatrix[1][3] = matrix->pos.y;
    gxMatrix[2][0] = -matrix->right.z;
    gxMatrix[2][1] = -matrix->up.z;
    gxMatrix[2][2] = -matrix->at.z;
    gxMatrix[2][3] = -matrix->pos.z;
    GXLoadPosMtxImm(gxMatrix, index);
    if (normals)
        GXLoadNrmMtxImm(gxMatrix, index);
}

void _rpSkinLoadMatrixPalette(const RpSkin* skin, RwUInt32 meshIndex,
                              RwBool normals)
{
    const RpSkinRLECount* count = &skin->splitData.rleCount[meshIndex];
    RwUInt32 matrixIndex = 0;
    RwUInt32 run;

    for (run = 0; run < count->size; run++) {
        const RpSkinRLE* rle =
            &skin->splitData.rle[count->start + run];
        RwUInt32 i;
        for (i = 0; i < rle->count; i++) {
            _rpSkinLoadMatrix(
                &((RwMatrix*)_rpSkinGlobals.alignedScratchMemory)
                     [rle->startBone + i],
                matrixIndex, normals);
            matrixIndex += 3;
        }
    }
}

void* _rpSkinRenderCallback(void* object,
                            RxGameCubeAllInOneInstanceData* instanceData)
{
    RpAtomic* atomic = (RpAtomic*)object;
    RpGeometry* geometry = atomic->geometry;
    RpSkin* skin = RpSkinGeometryGetSkin(geometry);
    RwGameCubeVertexBuffer* vertexBuffer =
        (RwGameCubeVertexBuffer*)((RwUInt8*)instanceData->resourceEntry + 0x18);
    RwGameCubeDisplayList* displayList =
        (RwGameCubeDisplayList*)&vertexBuffer->arrays[vertexBuffer->numArrays];
    RpMesh* mesh = (RpMesh*)((RwUInt8*)instanceData->meshHeader + 0x10);
    RwDlObjectRenderCallBack materialCallback;
    RpGameCubeVtxFmt* format = GEOMETRY_VTX_FORMAT(geometry);
    RwUInt32 meshCount = instanceData->meshHeader->numMeshes;
    RwUInt32 meshIndex = 0;

    ((RwGameCubeResEntryHeader*)instanceData->resourceEntry)->token =
        _RwDlTokenCurrent;
    _rwDlVtxFmtSetup(format, (RpGameCubeVtxFmtSetupData*)instanceData);
    if (skin->maxNumWeights > 1 || skin->splitData.numMeshes == 3)
        _rwDlTransformSetup(RwFrameGetLTM(atomic->object.parent),
                            (instanceData->geometryFlags & 0x10) != 0);
    else
        GXSetVtxDesc(0, 1);
    if (format == NULL)
        format = _rpGameCubeVtxFmtGetDefault();
    materialCallback = _rwDlObjectRenderSetup(
        instanceData->geometryFlags, instanceData->lightMask,
        instanceData->hasAmbient, vertexBuffer->reserved_0x00[1] & 1);
    if (skin->maxNumWeights == 1 && skin->splitData.numMeshes == 0) {
        RwUInt32 i;
        for (i = 0; i < skin->numUsedBones; i++)
            _rpSkinLoadMatrix(
                &((RwMatrix*)_rpSkinGlobals.alignedScratchMemory)
                     [skin->usedBoneList[i]],
                i * 3, (instanceData->geometryFlags & 0x10) != 0);
    }
    if ((instanceData->geometryFlags & 0x84) != 0) {
        RpMesh* deferredMesh[64];
        RwGameCubeDisplayList* deferredList[64];
        RwUInt32 deferredIndex[64];
        RwUInt32 deferredCount = 0;

        while (meshIndex < meshCount) {
            RpMaterial* material = mesh->material;
            SpecularMaterialData* specular = SPECULAR_DATA(material);
            if (!specular->flags.bits.hidden) {
                if (specular->flags.bits.reflectionPass) {
                    deferredMesh[deferredCount] = mesh;
                    deferredList[deferredCount] = displayList;
                    deferredIndex[deferredCount++] = meshIndex;
                } else {
                    RwTexture* texture = material->texture;
                    RwTexture* alpha =
                        RpMaterialGetAlphaPassTexture(material);
                    if (texture != NULL && texture->raster != NULL) {
                        RwUInt8* ext = (RwUInt8*)texture->raster +
                                      _RwGameCubeRasterExtOffset;
                        _rwDlRenderStateSetZCompLoc(
                            (*(RwUInt32*)(ext + 0x14) & 1) == 0);
                    } else {
                        _rwDlRenderStateSetZCompLoc(TRUE);
                    }
                    if (materialCallback != NULL)
                        materialCallback(&instanceData->ambient,
                                         (GXColor*)&material->color, material,
                                         material->surface.ambient);
                    SetSingleTextureAlphaPassWithAlphaComp(
                        texture, alpha, (RxGCTevAlphaPass*)instanceData);
                    if (skin->maxNumWeights == 1 &&
                        skin->splitData.numMeshes != 0)
                        _rpSkinLoadMatrixPalette(
                            skin, meshIndex,
                            (instanceData->geometryFlags & 0x10) != 0);
                    GXCallDisplayList(displayList->data, displayList->size);
                    _rxGCTevAlphaPassCleanup(
                        (RxGCTevAlphaPass*)instanceData);
                }
            }
            mesh++;
            displayList++;
            meshIndex++;
        }
        for (meshIndex = 0; meshIndex < deferredCount; meshIndex++) {
            RpMaterial* material = deferredMesh[meshIndex]->material;
            RwTexture* texture = material->texture;
            RwTexture* alpha = RpMaterialGetAlphaPassTexture(material);
            if (materialCallback != NULL)
                materialCallback(&instanceData->ambient,
                                 (GXColor*)&material->color, material,
                                 material->surface.ambient);
            SetSingleTextureAlphaPassWithAlphaComp(
                texture, alpha, (RxGCTevAlphaPass*)instanceData);
            if (skin->maxNumWeights == 1 && skin->splitData.numMeshes != 0)
                _rpSkinLoadMatrixPalette(
                    skin, deferredIndex[meshIndex],
                    (instanceData->geometryFlags & 0x10) != 0);
            GXCallDisplayList(deferredList[meshIndex]->data,
                              deferredList[meshIndex]->size);
            _rxGCTevAlphaPassCleanup((RxGCTevAlphaPass*)instanceData);
        }
    } else {
        _rwDlRenderStateSetZCompLoc(TRUE);
        while (meshIndex < meshCount) {
            RpMaterial* material = mesh->material;
            if (materialCallback != NULL)
                materialCallback(&instanceData->ambient,
                                 (GXColor*)&material->color, material,
                                 material->surface.ambient);
            if (skin->maxNumWeights == 1 && skin->splitData.numMeshes != 0)
                _rpSkinLoadMatrixPalette(
                    skin, meshIndex,
                    (instanceData->geometryFlags & 0x10) != 0);
            GXCallDisplayList(displayList->data, displayList->size);
            mesh++;
            displayList++;
            meshIndex++;
        }
    }
    return object;
}

static RpSkin* _rpSkinCreate(RpSkin* skin, RwUInt32 numVertices)
{
    RwUInt32 vertex;

    skin->nativeData = NULL;
    skin->nativeData2 = NULL;
    skin->platformWeights = NULL;
    skin->platformIndices = NULL;
    for (vertex = 0; vertex < numVertices; vertex++) {
        RwUInt32 position;
        for (position = 0; position < 3; position++) {
            RwUInt32 candidate;
            for (candidate = 1; candidate < 4 - position; candidate++) {
                RwReal* weights = (RwReal*)&skin->vertexBoneWeights[vertex];
                if (weights[position + candidate] > weights[position]) {
                    RwReal weight = weights[position];
                    RwUInt32 indices = skin->vertexBoneIndices[vertex];
                    RwUInt8* bytes = (RwUInt8*)&indices;
                    RwUInt8 index = bytes[position];
                    weights[position] = weights[position + candidate];
                    weights[position + candidate] = weight;
                    bytes[position] = bytes[position + candidate];
                    bytes[position + candidate] = index;
                    skin->vertexBoneIndices[vertex] = indices;
                }
            }
        }
    }
    if (skin->maxNumWeights > 1) {
        RwUInt8* weights;
        RwUInt8* indices;
        skin->platformWeights = RwEngineInstance->fpMalloc(
            numVertices * skin->maxNumWeights * 2 + 3, 0x30116);
        if (skin->platformWeights == NULL)
            return NULL;
        weights = skin->platformWeights;
        for (vertex = 0; vertex < numVertices; vertex++) {
            RwUInt32 weight;
            RwUInt32 sum = 0;
            for (weight = 0; weight < skin->maxNumWeights; weight++) {
                *weights = (RwUInt8)(128.0f *
                    ((RwReal*)&skin->vertexBoneWeights[vertex])[weight]);
                sum += *weights++;
            }
            for (weight = 0; sum < 128 && weight < skin->maxNumWeights;
                 weight++) {
                weights[weight - skin->maxNumWeights]++;
                sum++;
            }
        }
        skin->platformIndices = (void*)
            (((RwUInt32)weights + 3) & ~3U);
        indices = skin->platformIndices;
        for (vertex = 0; vertex < numVertices; vertex++) {
            RwUInt32 weight;
            for (weight = 0; weight < skin->maxNumWeights; weight++)
                *indices++ = (RwUInt8)(skin->vertexBoneIndices[vertex] >>
                                       (weight * 8));
        }
    }
    return skin;
}

RpGeometry* _rpSkinInitialize(RpGeometry* geometry)
{
    RpSkin* skin = SKIN_GEOMETRY_DATA(geometry);

    if (skin != NULL) {
        if ((geometry->flags & 0x01000000) != 0) {
            if ((geometry->flags & 0x10) != 0)
                geometry->lockedSinceLastInst =
                    (RwUInt16)(geometry->lockedSinceLastInst | 6);
            else
                geometry->lockedSinceLastInst =
                    (RwUInt16)(geometry->lockedSinceLastInst | 2);
            skin->nativeData = NULL;
            skin->nativeData2 = NULL;
        } else if (_rpSkinCreate(skin, geometry->numVertices) == NULL) {
            return NULL;
        }
    }
    return geometry;
}

RpGeometry* _rpSkinDeinitialize(RpGeometry* geometry)
{
    RpSkin* skin = SKIN_GEOMETRY_DATA(geometry);

    if (skin->platformWeights != NULL) {
        RwEngineInstance->fpFree(skin->platformWeights);
        skin->platformWeights = NULL;
        skin->platformIndices = NULL;
    }
    skin->nativeData = NULL;
    skin->nativeData2 = NULL;
    return geometry;
}
