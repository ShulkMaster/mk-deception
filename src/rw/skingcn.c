#include "dolphin/gx.h"
#include "dolphin/os.h"
#include "rw/rwengine.h"
#include "rw/alphapass.h"
#include "rw/gamecube.h"
#include "rw/dltoken.h"
#include "rw/gamecube_texture.h"
#include "rw/gcspecular.h"
#include "rw/nodegamecube.h"
#include "rw/rphanim.h"
#include "rw/rpskin.h"
#include "rw/rtquat.h"
#include "rw/rwframe.h"
#include "rw/rwresources.h"
#include "rw/rwvector.h"

RpSkinGlobals _rpSkinGlobals = {0, 0, 0, 0, 0, 0, 0, 0, 0,
                                {0, 0, 0, 0, 0, 0}};

static void _rpSkinMainResEntryCB(RwResEntry* entry)
{
    /* Release transient skin buffers after GX finishes using the entry. */
    RwGameCubeVertexBuffer* vertexBuffer =
        (RwGameCubeVertexBuffer*)(entry + 1);
    unsigned int ownerFlags;

    if (vertexBuffer->displayListToken == _RwDlTokenCurrent) {
        GXSetDrawSync(_RwDlTokenCurrent & 0xFFFFU);
        _RwDlTokenCurrent = (_RwDlTokenCurrent + 1) % 57344;
    }
    while (_rwDlTokenQueryDone(vertexBuffer->displayListToken) == 0) {
    }
    if (vertexBuffer->arrays[0].data != 0) {
        RwResourcesFreeResEntry(
            *(RwResEntry**)((unsigned char*)vertexBuffer->arrays[0].data - 4));
    }
    ownerFlags = *(unsigned int*)((unsigned char*)entry->owner + 8);
    if (((RwObject*)entry->owner)->type == 1) {
        SkinAtomicState* atomicData =
            (SkinAtomicState*)((unsigned char*)entry->owner +
                              _rpSkinGlobals.atomicOffset);
        vertexBuffer->arrays[0].data = atomicData->positions;
        if ((ownerFlags & 0x10) != 0)
            vertexBuffer->arrays[1].data = atomicData->normals;
        atomicData->positions = 0;
        atomicData->normals = 0;
    } else {
        RpSkin* skin =
            *(RpSkin**)((unsigned char*)entry->owner +
                        _rpSkinGlobals.geometryOffset);
        vertexBuffer->arrays[0].data = skin->nativeData;
        if ((ownerFlags & 0x10) != 0)
            vertexBuffer->arrays[1].data = skin->nativeData2;
        skin->nativeData = 0;
        skin->nativeData2 = 0;
    }
}

static void _rpSkinResEntryWaitDone(RwResEntry* entry)
{
    /* Wait until GX has consumed the resource entry's display-list token. */
    if (((RwGameCubeResEntryHeader*)entry)->vertexBuffer.displayListToken ==
        _RwDlTokenCurrent) {
        /* TODO: Retail redundantly zero-extends this 16-bit token before the call. */
        GXSetDrawSync(_RwDlTokenCurrent);
        _RwDlTokenCurrent = (_RwDlTokenCurrent + 1) % 57344;
    }
    while (_rwDlTokenQueryDone(
               ((RwGameCubeResEntryHeader*)entry)
                   ->vertexBuffer.displayListToken) == 0) {
    }
}

int _rpSkinVertexBuffersUpdate(RpSkin* skin, RpAtomic* atomic,
                                  RwGameCubeVertexBuffer* vertexBuffer,
                                  RwResEntry** resourceEntry)
{
    /* Preserve or allocate the skinned position and normal vertex arrays. */
    RpGeometry* geometry = atomic->geometry;
    RpGameCubeVtxFmt* format;
    unsigned int size;
    RwResEntry* sourceEntry;
    RwResEntry* entry;

    {
        if ((*resourceEntry)->link.next != 0) {
            (*resourceEntry)->link.prev->next =
                (*resourceEntry)->link.next;
            (*resourceEntry)->link.next->prev =
                (*resourceEntry)->link.prev;
            (*resourceEntry)->link.next =
                ((RwResourcesGlobals*)((unsigned char*)RwEngineInstance +
                                       resourcesModule.globalsOffset))
                    ->activeList->next;
            (*resourceEntry)->link.prev =
                ((RwResourcesGlobals*)((unsigned char*)RwEngineInstance +
                                       resourcesModule.globalsOffset))
                    ->activeList;
            ((RwResourcesGlobals*)((unsigned char*)RwEngineInstance +
                                   resourcesModule.globalsOffset))
                ->activeList->next->prev = &(*resourceEntry)->link;
            ((RwResourcesGlobals*)((unsigned char*)RwEngineInstance +
                                   resourcesModule.globalsOffset))
                ->activeList->next = &(*resourceEntry)->link;
        }
    }
    (*resourceEntry)->destroyNotify = _rpSkinMainResEntryCB;
    if (((RwObject*)(*resourceEntry)->owner)->type == 1) {
        SkinAtomicState* atomicData =
            (SkinAtomicState*)((unsigned char*)atomic +
                                _rpSkinGlobals.atomicOffset);
        if (atomicData->positions == 0) {
            atomicData->positions = vertexBuffer->arrays[0].data;
            vertexBuffer->arrays[0].data = 0;
            if ((geometry->flags & 0x10) != 0) {
                atomicData->normals = vertexBuffer->arrays[1].data;
                vertexBuffer->arrays[1].data = 0;
            }
        }
    } else if (skin->nativeData == 0) {
        skin->nativeData = vertexBuffer->arrays[0].data;
        vertexBuffer->arrays[0].data = 0;
        if ((geometry->flags & 0x10) != 0) {
            skin->nativeData2 = vertexBuffer->arrays[1].data;
            vertexBuffer->arrays[1].data = 0;
        }
    }
    if (vertexBuffer->arrays[0].data != 0) {
        unsigned short token =
            ((RwGameCubeResEntryHeader*)*(RwResEntry**)(
                 (unsigned char*)vertexBuffer->arrays[0].data - 4))
                ->vertexBuffer.displayListToken;

        if (_rwDlTokenQueryDone(token) == 0) {
            (*(RwResEntry**)((unsigned char*)vertexBuffer->arrays[0].data - 4))
                ->ownerRef = 0;
            vertexBuffer->arrays[0].data = 0;
            if ((geometry->flags & 0x10) != 0)
                vertexBuffer->arrays[1].data = 0;
        }
    }
    if (vertexBuffer->arrays[0].data == 0) {
        format = *(RpGameCubeVtxFmt**)((unsigned char*)geometry +
                                       _rpDlGeomVtxFmtOffset);
        size = 6;
        if (format != 0) {
            const unsigned int vertexSizes[5] = {3, 3, 6, 6, 12};

            size += (geometry->numVertices *
                     vertexSizes[format->positionType] + 31) & ~31U;
            if ((geometry->flags & 0x10) != 0) {
                unsigned int components;

                if (format->normalMode != 0)
                    components = 3;
                else
                    components = 1;
                size += (geometry->numVertices *
                         vertexSizes[format->normalType] * components + 31) &
                        ~31U;
            }
        } else {
            size += (geometry->numVertices * sizeof(RwV3d) + 31) & ~31U;
            if ((geometry->flags & 0x10) != 0)
                size += (geometry->numVertices * sizeof(RwV3d) + 31) & ~31U;
        }
        entry = RwResourcesAllocateResEntry(
            atomic, (RwResEntry**)&vertexBuffer->arrays[0].data, size,
            _rpSkinResEntryWaitDone);
        if (geometry->numMorphTargets == 1)
            sourceEntry = geometry->repEntry;
        else
            sourceEntry = atomic->repEntry;
        if (entry == 0 || sourceEntry == 0) {
            if (entry != 0)
                RwResourcesFreeResEntry(entry);
            return 0;
        }
        vertexBuffer->arrays[0].data = (void*)
            (((unsigned int)vertexBuffer->arrays[0].data + 0x3D) & ~31U);
        if ((geometry->flags & 0x10) != 0) {
            vertexBuffer->arrays[1].data =
                (unsigned char*)vertexBuffer->arrays[0].data +
                ((geometry->numVertices * vertexBuffer->arrays[0].stride + 31) &
                 ~31U);
        }
        *(RwResEntry**)((unsigned char*)vertexBuffer->arrays[0].data - 4) = entry;
    } else {
        RwResEntry** storedEntry = (RwResEntry**)(
            (unsigned char*)vertexBuffer->arrays[0].data - 4);

        if ((*storedEntry)->link.next != 0) {
            (*storedEntry)->link.prev->next = (*storedEntry)->link.next;
            (*storedEntry)->link.next->prev = (*storedEntry)->link.prev;
            (*storedEntry)->link.next =
                ((RwResourcesGlobals*)((unsigned char*)RwEngineInstance +
                                       resourcesModule.globalsOffset))
                    ->activeList->next;
            (*storedEntry)->link.prev =
                ((RwResourcesGlobals*)((unsigned char*)RwEngineInstance +
                                       resourcesModule.globalsOffset))
                    ->activeList;
            ((RwResourcesGlobals*)((unsigned char*)RwEngineInstance +
                                   resourcesModule.globalsOffset))
                ->activeList->next->prev = &(*storedEntry)->link;
            ((RwResourcesGlobals*)((unsigned char*)RwEngineInstance +
                                   resourcesModule.globalsOffset))
                ->activeList->next = &(*storedEntry)->link;
        }
    }
    ((RwGameCubeResEntryHeader*)*(RwResEntry**)(
         (unsigned char*)vertexBuffer->arrays[0].data - 4))
        ->vertexBuffer.displayListToken = _RwDlTokenCurrent;
    return 1;
}

void _rpSkinMatrixBlendUpdate(RwMatrix* destination, RpSkin* skin,
                              const RwMatrix* ltm,
                              RpHAnimHierarchy* hierarchy)
{
    /* Build the active bone palette in the hierarchy's matrix space. */
    RwMatrix inverseNoMatrices;
    RwMatrix temporaryNoMatrices;
    RwMatrix temporaryLocal;
    RwMatrix inverseDefault;
    unsigned int i;

    if (hierarchy == 0)
        return;
    if ((hierarchy->flags & rpHANIMHIERARCHYNOMATRICES) != 0) {
        const RwMatrix* transform;

        if (skin->maxNumWeights > 1 || skin->splitData.numMeshes == 3) {
            inverseNoMatrices.flags = 0;
            RwMatrixInvert(&inverseNoMatrices, ltm);
            transform = &inverseNoMatrices;
        } else {
            transform = &_RwDlInvCamLTM;
        }
        for (i = 0; i < skin->numUsedBones; i++) {
            RwFrame* frame =
                hierarchy->pNodeInfo[skin->usedBoneList[i]].pFrame;
            temporaryNoMatrices.flags = 0;
            RwMatrixMultiply(&temporaryNoMatrices,
                             &skin->skinToBoneMatrices[skin->usedBoneList[i]],
                             RwFrameGetLTM(frame));
            RwMatrixMultiply(&destination[skin->usedBoneList[i]],
                             &temporaryNoMatrices, transform);
        }
    } else if ((hierarchy->flags & rpHANIMHIERARCHYLOCALSPACEMATRICES) != 0) {
        if (skin->maxNumWeights > 1 || skin->splitData.numMeshes == 3) {
            for (i = 0; i < skin->numUsedBones; i++) {
                RwMatrixMultiply(
                    &destination[skin->usedBoneList[i]],
                    &skin->skinToBoneMatrices[skin->usedBoneList[i]],
                    &hierarchy->pMatrixArray[skin->usedBoneList[i]]);
            }
        } else {
            temporaryLocal.flags = 0;
            RwMatrixMultiply(&temporaryLocal, ltm, &_RwDlInvCamLTM);
            _rpSkinMatrixBlendUpdateASM(
                destination, skin->skinToBoneMatrices,
                hierarchy->pMatrixArray, &temporaryLocal, skin->usedBoneList,
                skin->numUsedBones);
        }
    } else {
        const RwMatrix* transform;

        if (skin->maxNumWeights > 1 || skin->splitData.numMeshes == 3) {
            inverseDefault.flags = 0;
            RwMatrixInvert(&inverseDefault, ltm);
            transform = &inverseDefault;
        } else {
            transform = &_RwDlInvCamLTM;
        }
        _rpSkinMatrixBlendUpdateASM(destination, skin->skinToBoneMatrices,
                                    hierarchy->pMatrixArray, transform,
                                    skin->usedBoneList, skin->numUsedBones);
    }
}

void _rpSkinBlendBodyP(RpSkin* skin, const RwMatrix* matrices,
                       const unsigned char* source,
                       unsigned char* destination,
                       const RpGameCubeVtxFmt* format,
                       int numVertices)
{
    /* Blend positions into the instanced vertex stream. */
    /* TODO: Verify the retail GQR5/GQR6 setup; portable C cannot express the
     * required paired-single SPR writes. */
    const unsigned char scalarSizes[5] = {1, 1, 2, 2, 4};
    RpSkinBlendPositionData data;
    unsigned int stride = format != 0
                          ? scalarSizes[format->positionType]
                          : sizeof(float);
    int i;

    data.destination = destination;
    data.source = source;
    data.stride = stride;
    data.numVertices = numVertices;
    switch (skin->maxNumWeights) {
    case 1: {
        const RwV3d* input = (const RwV3d*)skin->nativeData;
        RwV3d* output = (RwV3d*)destination;
        for (i = 0; i < numVertices; i++) {
            unsigned char bone = (unsigned char)skin->vertexBoneIndices[i];
            RwV3dTransformPoint(&output[i], &input[i], &matrices[bone]);
        }
        break;
    }
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
                        const unsigned char* sourcePositions,
                        const unsigned char* sourceNormals,
                        unsigned char* destinationPositions,
                        unsigned char* destinationNormals,
                        const RpGameCubeVtxFmt* format,
                        int numVertices)
{
    /* Blend positions and normals into the instanced vertex streams. */
    /* TODO: Verify the retail GQR5/GQR6/GQR7 setup; portable C cannot express
     * the required paired-single SPR writes. */
    const unsigned char scalarSizes[5] = {1, 1, 2, 2, 4};
    RpSkinBlendPositionNormalData data;
    unsigned int posStride = format != 0
                            ? scalarSizes[format->positionType]
                            : sizeof(float);
    unsigned int normalStride = format != 0
                               ? scalarSizes[format->normalType]
                               : sizeof(float);
    unsigned int nbtStride = format != 0 && format->normalMode != 0
                            ? normalStride * 6
                            : 0;
    int i;

    data.destinationPositions = destinationPositions;
    data.destinationNormals = destinationNormals;
    data.sourcePositions = sourcePositions;
    data.sourceNormals = sourceNormals;
    data.positionStride = posStride;
    data.normalStride = normalStride;
    data.nbtStride = nbtStride;
    data.numVertices = numVertices;
    switch (skin->maxNumWeights) {
    case 1: {
        const RwV3d* inputPositions = (const RwV3d*)sourcePositions;
        const RwV3d* inputNormals = (const RwV3d*)sourceNormals;
        RwV3d* outputPositions = (RwV3d*)destinationPositions;
        RwV3d* outputNormals = (RwV3d*)destinationNormals;
        for (i = 0; i < numVertices; i++) {
            unsigned char bone = (unsigned char)skin->vertexBoneIndices[i];
            RwV3dTransformPoint(&outputPositions[i], &inputPositions[i],
                                &matrices[bone]);
            RwV3dTransformVector(&outputNormals[i], &inputNormals[i],
                                 &matrices[bone]);
        }
        break;
    }
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
    /* Select the rigid or skinned geometry instancer for this atomic. */
    RpAtomic* atomic = (RpAtomic*)object;
    RpGeometry* geometry = atomic->geometry;
    RpSkin* skin = RpSkinGeometryGetSkin(geometry);
    void* owner;
    RwResEntry** ownerRef;

    if (geometry->numMorphTargets != 1) {
        owner = atomic;
        ownerRef = &atomic->repEntry;
    } else {
        owner = geometry;
        ownerRef = &geometry->repEntry;
    }
    if (skin->maxNumWeights > 1 || skin->splitData.numMeshes == 3) {
        if ((geometry->flags & 0x02000000) != 0) {
            if (_RwDlPreInstanceOptimize == 1)
                *resourceEntry = _rwDlGeometrySkinInstanceOptimized(
                    geometry, owner, ownerRef);
            else
                *resourceEntry = _rwDlGeometryInstanceFast(
                    geometry, owner, ownerRef);
        } else {
            *resourceEntry = _rwDlGeometryInstanceFast(geometry, owner,
                                                        ownerRef);
        }
    } else if ((geometry->flags & 0x02000000) != 0) {
        if (_RwDlPreInstanceOptimize == 1)
            *resourceEntry = _rwDlGeometrySkinInstanceOptimized(
                geometry, owner, ownerRef);
        else
            *resourceEntry = _rwDlGeometrySkinInstanceFast(
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
    /* Rebuild skinned vertex buffers while preserving their source arrays. */
    RpAtomic* atomic = (RpAtomic*)object;
    RpGeometry* geometry = atomic->geometry;
    RpSkin* skin = *(RpSkin**)((unsigned char*)geometry +
                               _rpSkinGlobals.geometryOffset);

    if (skin->maxNumWeights > 1 || skin->splitData.numMeshes == 3) {
        RwGameCubeVertexBuffer* vertexBuffer =
            (RwGameCubeVertexBuffer*)(*resourceEntry + 1);
        void** cachedPositions;
        void** cachedNormals;

        if (((RwObject*)(*resourceEntry)->owner)->type == 1) {
            SkinAtomicState* atomicData =
                (SkinAtomicState*)((unsigned char*)atomic +
                                    _rpSkinGlobals.atomicOffset);
            cachedPositions = &atomicData->positions;
            cachedNormals = &atomicData->normals;
        } else {
            cachedPositions = &skin->nativeData;
            cachedNormals = &skin->nativeData2;
        }
        if (*cachedPositions == 0) {
            if ((geometry->flags & 0x01000000) == 0)
                _rxGCAtomicDefaultReinstanceCallback(object, resourceEntry);
            *cachedPositions = vertexBuffer->arrays[0].data;
            vertexBuffer->arrays[0].data = 0;
            if ((geometry->flags & 0x10) != 0) {
                *cachedNormals = vertexBuffer->arrays[1].data;
                vertexBuffer->arrays[1].data = 0;
            }
        } else if ((geometry->flags & 0x01000000) == 0) {
            void* position = vertexBuffer->arrays[0].data;
            vertexBuffer->arrays[0].data = *cachedPositions;
            if ((geometry->flags & 0x10) != 0) {
                void* normal = vertexBuffer->arrays[1].data;

                vertexBuffer->arrays[1].data = *cachedNormals;
                _rxGCAtomicDefaultReinstanceCallback(object, resourceEntry);
                vertexBuffer->arrays[1].data = normal;
            } else {
                _rxGCAtomicDefaultReinstanceCallback(object, resourceEntry);
            }
            vertexBuffer->arrays[0].data = position;
        }
        if (!_rpSkinVertexBuffersUpdate(skin, atomic, vertexBuffer,
                                        resourceEntry))
            return 0;
        _rpSkinMatrixBlendUpdate(
            _rpSkinGlobals.alignedScratchMemory, skin,
            RwFrameGetLTM(atomic->object.parent),
            ((SkinAtomicState*)((unsigned char*)atomic +
                                _rpSkinGlobals.atomicOffset))
                ->hierarchy);
        if ((geometry->flags & 0x10) != 0)
            _rpSkinBlendBodyPN(
                skin, _rpSkinGlobals.alignedScratchMemory, *cachedPositions,
                *cachedNormals, vertexBuffer->arrays[0].data,
                vertexBuffer->arrays[1].data,
                *(RpGameCubeVtxFmt**)((unsigned char*)geometry +
                                      _rpDlGeomVtxFmtOffset),
                geometry->numVertices);
        else
            _rpSkinBlendBodyP(
                skin, _rpSkinGlobals.alignedScratchMemory, *cachedPositions,
                vertexBuffer->arrays[0].data,
                *(RpGameCubeVtxFmt**)((unsigned char*)geometry +
                                      _rpDlGeomVtxFmtOffset),
                geometry->numVertices);
    } else {
        _rpSkinMatrixBlendUpdate(
            _rpSkinGlobals.alignedScratchMemory, skin,
            RwFrameGetLTM(atomic->object.parent),
            ((SkinAtomicState*)((unsigned char*)atomic +
                                _rpSkinGlobals.atomicOffset))
                ->hierarchy);
    }
    return object;
}

void _rpSkinLoadMatrix(const RwMatrix* matrix, unsigned int index,
                       int normals)
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

void _rpSkinLoadMatrixPalette(const RpSkin* skin, unsigned int meshIndex,
                              int normals)
{
    /* Expand the packed split-mesh runs into sequential GX matrix slots. */
    unsigned int matrixIndex = 0;
    unsigned char rleOffset = (unsigned char)(
        ((const unsigned char*)skin->splitData.rleCount)[meshIndex * 2] * 2);
    unsigned char runCount =
        ((const unsigned char*)skin->splitData.rleCount)[meshIndex * 2 + 1];
    unsigned char run;

    for (run = 0; run < runCount; run++) {
        unsigned char startBone =
            ((const unsigned char*)skin->splitData.rle)[rleOffset + run * 2];
        unsigned char boneCount = ((const unsigned char*)skin->splitData.rle)
            [rleOffset + run * 2 + 1];
        unsigned int bone;

        for (bone = 0; bone < boneCount; bone++) {
            _rpSkinLoadMatrix(
                &((RwMatrix*)_rpSkinGlobals.alignedScratchMemory)
                     [startBone + bone],
                matrixIndex, normals);
            matrixIndex += 3;
        }
    }
}

void* _rpSkinRenderCallback(
    void* object, RxGameCubeAtomicAllInOneInstanceData* instanceData)
{
    /* Render an instanced skinned atomic, deferring marked material passes. */
    RpAtomic* atomic = (RpAtomic*)object;
    RpGeometry* geometry = atomic->geometry;
    RwGameCubeVertexBuffer* vertexBuffer =
        (RwGameCubeVertexBuffer*)(instanceData->resourceEntry + 1);
    RwGameCubeDisplayList* displayList =
        (RwGameCubeDisplayList*)&vertexBuffer->arrays[vertexBuffer->numArrays];
    const RwMatrix* ltm;
    RpSkin* skin;
    RpMesh* mesh;
    RwDlObjectRenderCallBack materialCallback;
    RpGameCubeVtxFmt* format;
    unsigned int remaining;
    unsigned int meshIndex;

    vertexBuffer->displayListToken = _RwDlTokenCurrent;
    ltm = RwFrameGetLTM(atomic->object.parent);
    format = *(RpGameCubeVtxFmt**)((unsigned char*)geometry +
                                   _rpDlGeomVtxFmtOffset);
    _rwDlVtxFmtSetup(format, (RpGameCubeVtxFmtSetupData*)instanceData);
    skin = RpSkinGeometryGetSkin(geometry);
    if (skin->maxNumWeights > 1 || skin->splitData.numMeshes == 3) {
        int normals;

        if ((instanceData->geometryFlags & 0x10) != 0)
            normals = 1;
        else
            normals = 0;
        _rwDlTransformSetup(ltm, normals);
    } else {
        GXSetVtxDesc(0, 1);
    }
    if (format == 0)
        format = _rpGameCubeVtxFmtGetDefault();
    materialCallback = _rwDlObjectRenderSetup(
        instanceData->geometryFlags, instanceData->lightMask,
        instanceData->hasAmbient, vertexBuffer->flags & 1);
    remaining = instanceData->meshHeader->numMeshes;
    mesh = (RpMesh*)(instanceData->meshHeader + 1);
    if (skin->maxNumWeights == 1 && skin->splitData.numMeshes == 0) {
        unsigned int i;
        for (i = 0; i < skin->numUsedBones; i++) {
            int normals;

            if ((instanceData->geometryFlags & 0x10) != 0)
                normals = 1;
            else
                normals = 0;
            _rpSkinLoadMatrix(
                &((RwMatrix*)_rpSkinGlobals.alignedScratchMemory)
                     [skin->usedBoneList[i]],
                i * 3, normals);
        }
    }
    if ((instanceData->geometryFlags & 0x84) != 0) {
        RpMesh* deferredMesh[64];
        RwGameCubeDisplayList* deferredList[64];
        unsigned int deferredIndex[64];
        unsigned int deferredCount = 0;

        while (remaining-- != 0) {
            unsigned int currentMeshIndex =
                instanceData->meshHeader->numMeshes - (remaining + 1);
            RpMaterial* material = mesh->material;
            SpecularMaterialPluginData* specular =
                (SpecularMaterialPluginData*)((unsigned char*)material +
                                              SpecularMaterialOffset);
            if (!specular->flags.skinRender.hidden) {
                if (specular->flags.skinRender.deferred) {
                    deferredMesh[deferredCount] = mesh;
                    deferredList[deferredCount] = displayList;
                    deferredIndex[deferredCount++] = currentMeshIndex;
                    mesh++;
                    displayList++;
                    continue;
                } else {
                    RwTexture* texture = material->texture;
                    RwTexture* alpha =
                        RpMaterialGetAlphaPassTexture(material);
                    if (texture != 0 && texture->raster != 0) {
                        RwGameCubeRasterExt* extension =
                            (RwGameCubeRasterExt*)(
                                (unsigned char*)texture->raster->parent +
                                _RwGameCubeRasterExtOffset);
                        int compareBeforeTexture;

                        if ((extension->hasAlpha & 1) != 0)
                            compareBeforeTexture = 0;
                        else
                            compareBeforeTexture = 1;
                        _rwDlRenderStateSetZCompLoc(compareBeforeTexture);
                    } else {
                        _rwDlRenderStateSetZCompLoc(1);
                    }
                    if (materialCallback != 0)
                        materialCallback(&instanceData->ambient,
                                         (GXColor*)&material->color, material,
                                         material->surface.ambient);
                    SetSingleTextureAlphaPassWithAlphaComp(
                        texture, alpha, (RxGCTevAlphaPass*)instanceData);
                    if (skin->maxNumWeights == 1 &&
                        skin->splitData.numMeshes != 0) {
                        int normals;

                        if ((instanceData->geometryFlags & 0x10) != 0)
                            normals = 1;
                        else
                            normals = 0;
                        _rpSkinLoadMatrixPalette(
                            skin, currentMeshIndex, normals);
                    }
                    GXCallDisplayList(displayList->data, displayList->size);
                    _rxGCTevAlphaPassCleanup(
                        (RxGCTevAlphaPass*)instanceData);
                }
            }
            mesh++;
            displayList++;
        }
        for (meshIndex = 0; meshIndex < deferredCount; meshIndex++) {
            RpMaterial* material = deferredMesh[meshIndex]->material;
            SpecularMaterialPluginData* specular =
                (SpecularMaterialPluginData*)((unsigned char*)material +
                                              SpecularMaterialOffset);
            if (!specular->flags.skinRender.hidden) {
                RwTexture* texture = material->texture;
                RwTexture* alpha = RpMaterialGetAlphaPassTexture(material);
                if (texture != 0 && texture->raster != 0) {
                    RwGameCubeRasterExt* extension =
                        (RwGameCubeRasterExt*)(
                            (unsigned char*)texture->raster->parent +
                            _RwGameCubeRasterExtOffset);
                    int compareBeforeTexture;

                    if ((extension->hasAlpha & 1) != 0)
                        compareBeforeTexture = 0;
                    else
                        compareBeforeTexture = 1;
                    _rwDlRenderStateSetZCompLoc(compareBeforeTexture);
                } else {
                    _rwDlRenderStateSetZCompLoc(1);
                }
                if (materialCallback != 0)
                    materialCallback(&instanceData->ambient,
                                     (GXColor*)&material->color, material,
                                     material->surface.ambient);
                SetSingleTextureAlphaPassWithAlphaComp(
                    texture, alpha, (RxGCTevAlphaPass*)instanceData);
                if (skin->maxNumWeights == 1 &&
                    skin->splitData.numMeshes != 0) {
                    int normals;

                    if ((instanceData->geometryFlags & 0x10) != 0)
                        normals = 1;
                    else
                        normals = 0;
                    _rpSkinLoadMatrixPalette(
                        skin, deferredIndex[meshIndex], normals);
                }
                GXCallDisplayList(deferredList[meshIndex]->data,
                                  deferredList[meshIndex]->size);
                _rxGCTevAlphaPassCleanup((RxGCTevAlphaPass*)instanceData);
            }
        }
    } else {
        _rwDlRenderStateSetZCompLoc(1);
        while (remaining-- != 0) {
            unsigned int currentMeshIndex =
                instanceData->meshHeader->numMeshes - (remaining + 1);
            RpMaterial* material = mesh->material;
            if (materialCallback != 0)
                materialCallback(&instanceData->ambient,
                                 (GXColor*)&material->color, material,
                                 material->surface.ambient);
            if (skin->maxNumWeights == 1 &&
                skin->splitData.numMeshes != 0) {
                int normals;

                if ((instanceData->geometryFlags & 0x10) != 0)
                    normals = 1;
                else
                    normals = 0;
                _rpSkinLoadMatrixPalette(
                    skin, currentMeshIndex, normals);
            }
            GXCallDisplayList(displayList->data, displayList->size);
            mesh++;
            displayList++;
        }
    }
    return object;
}

static RpSkin* _rpSkinCreate(RpSkin* skin, unsigned int numVertices)
{
    /* Sort influences and build the packed byte streams used by skinning. */
    unsigned int vertex;

    skin->nativeData = 0;
    skin->nativeData2 = 0;
    skin->platformWeights = 0;
    skin->platformIndices = 0;
    for (vertex = 0; vertex < numVertices; vertex++) {
        unsigned int position;
        for (position = 0; position < 3; position++) {
            unsigned int candidate;
            for (candidate = 1; candidate < 4 - position; candidate++) {
                if (skin->vertexBoneWeights[vertex]
                        .values[position + candidate] >
                    skin->vertexBoneWeights[vertex].values[position]) {
                    float weight =
                        skin->vertexBoneWeights[vertex].values[position];
                    unsigned char firstIndex =
                        (unsigned char)(skin->vertexBoneIndices[vertex] >>
                                        (position * 8));
                    unsigned char secondIndex = (unsigned char)(
                        skin->vertexBoneIndices[vertex] >>
                        ((position + candidate) * 8));

                    skin->vertexBoneWeights[vertex].values[position] =
                        skin->vertexBoneWeights[vertex]
                            .values[position + candidate];
                    skin->vertexBoneWeights[vertex]
                        .values[position + candidate] = weight;
                    skin->vertexBoneIndices[vertex] &=
                        ~(0xFFU << (position * 8));
                    skin->vertexBoneIndices[vertex] |=
                        (unsigned int)secondIndex << (position * 8);
                    skin->vertexBoneIndices[vertex] &=
                        ~(0xFFU << ((position + candidate) * 8));
                    skin->vertexBoneIndices[vertex] |=
                        (unsigned int)firstIndex <<
                        ((position + candidate) * 8);
                }
            }
        }
    }
    if (skin->maxNumWeights > 1) {
        unsigned int allocationSize =
            numVertices * (skin->maxNumWeights * 2) + 3;
        unsigned char* weights;
        unsigned char* indices;
        skin->platformWeights = RwEngineInstance->fpMalloc(
            allocationSize, 0x30116);
        weights = skin->platformWeights;
        for (vertex = 0; vertex < numVertices; vertex++) {
            unsigned int weight;
            unsigned char sum = 0;
            for (weight = 0; weight < skin->maxNumWeights; weight++) {
                *weights = (unsigned char)(int)(
                    128.0f *
                    skin->vertexBoneWeights[vertex].values[weight]);
                sum += *weights++;
            }
            if (sum < 128) {
                for (weight = 0; weight < skin->maxNumWeights; weight++) {
                    weights[weight - skin->maxNumWeights]++;
                    sum++;
                    if (sum == 128)
                        break;
                }
            }
        }
        skin->platformIndices =
            (unsigned char*)skin->platformWeights +
            numVertices * skin->maxNumWeights;
        skin->platformIndices =
            (void*)(((unsigned int)skin->platformIndices + 3) & ~3U);
        indices = skin->platformIndices;
        for (vertex = 0; vertex < numVertices; vertex++) {
            unsigned int weight;
            for (weight = 0; weight < skin->maxNumWeights; weight++)
                *indices++ = (unsigned char)(skin->vertexBoneIndices[vertex] >>
                                       (weight * 8));
        }
    }
    return skin;
}

RpGeometry* _rpSkinInitialize(RpGeometry* geometry)
{
    RpSkin* skin = *(RpSkin**)((unsigned char*)geometry +
                               _rpSkinGlobals.geometryOffset);

    if (skin != 0) {
        if ((geometry->flags & 0x01000000) != 0) {
            if ((geometry->flags & 0x10) != 0)
                geometry->lockedSinceLastInst =
                    (unsigned short)(geometry->lockedSinceLastInst | 6);
            else
                geometry->lockedSinceLastInst =
                    (unsigned short)(geometry->lockedSinceLastInst | 2);
            skin->nativeData = 0;
            skin->nativeData2 = 0;
        } else if (_rpSkinCreate(skin, geometry->numVertices) == 0) {
            return 0;
        }
    }
    return geometry;
}

RpGeometry* _rpSkinDeinitialize(RpGeometry* geometry)
{
    RpSkin* skin = *(RpSkin**)((unsigned char*)geometry +
                               _rpSkinGlobals.geometryOffset);

    if (skin->platformWeights != 0) {
        RwEngineInstance->fpFree(skin->platformWeights);
        skin->platformWeights = 0;
        skin->platformIndices = 0;
    }
    skin->nativeData = 0;
    skin->nativeData2 = 0;
    return geometry;
}
