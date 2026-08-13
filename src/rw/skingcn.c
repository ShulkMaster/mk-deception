#include "dolphin/gx.h"
#include "dolphin/os.h"
#include "libmkparticle/rw_engine.h"
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

static const unsigned char skinVertexSizes[5] = {3, 3, 6, 6, 12};

static void ActivateResourceEntry(RwResEntry* entry)
{
    RwLLLink* link = &entry->link;
    RwResourcesGlobals* resources =
        (RwResourcesGlobals*)((unsigned char*)RwEngineInstance +
                                    resourcesModule.globalsOffset);
    RwLLLink* head = resources->activeList;

    if (link->next != 0) {
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
    unsigned int ownerFlags;

    owner = (RwObject*)entry->owner;
    ownerFlags = *(unsigned int*)((unsigned char*)owner + 8);

    if (header->data.sync.token == _RwDlTokenCurrent) {
        GXSetDrawSync(_RwDlTokenCurrent);
        _RwDlTokenCurrent = (_RwDlTokenCurrent + 1) % 57344;
    }
    while (_rwDlTokenQueryDone(header->data.sync.token) == 0) {
    }
    if (vertexBuffer->arrays[0].data != 0) {
        RwResourcesFreeResEntry(
            *(RwResEntry**)((unsigned char*)vertexBuffer->arrays[0].data - 4));
    }
    if (owner->type == 1) {
        SkinAtomicState* atomicData =
            (SkinAtomicState*)((unsigned char*)owner + _rpSkinGlobals.atomicOffset);
        vertexBuffer->arrays[0].data = atomicData->positions;
        if ((ownerFlags & 0x10) != 0)
            vertexBuffer->arrays[1].data = atomicData->normals;
        atomicData->positions = 0;
        atomicData->normals = 0;
    } else {
        RpSkin* skin =
            *(RpSkin**)((unsigned char*)owner + _rpSkinGlobals.geometryOffset);
        vertexBuffer->arrays[0].data = skin->nativeData;
        if ((ownerFlags & 0x10) != 0)
            vertexBuffer->arrays[1].data = skin->nativeData2;
        skin->nativeData = 0;
        skin->nativeData2 = 0;
    }
}

static void _rpSkinResEntryWaitDone(RwResEntry* entry)
{
    RwGameCubeResEntryHeader* header = (RwGameCubeResEntryHeader*)entry;

    if (header->data.sync.token == _RwDlTokenCurrent) {
        GXSetDrawSync(_RwDlTokenCurrent);
        _RwDlTokenCurrent = (_RwDlTokenCurrent + 1) % 57344;
    }
    while (_rwDlTokenQueryDone(header->data.sync.token) == 0) {
    }
}

int _rpSkinVertexBuffersUpdate(RpSkin* skin, RpAtomic* atomic,
                                  RwGameCubeVertexBuffer* vertexBuffer,
                                  RwResEntry** resourceEntry)
{
    RpGeometry* geometry = atomic->geometry;
    RpGameCubeVtxFmt* format;
    unsigned int size;
    unsigned int numVertices;
    RwResEntry* entry;

    ActivateResourceEntry(*resourceEntry);
    (*resourceEntry)->destroyNotify = _rpSkinMainResEntryCB;
    if ((*resourceEntry)->owner != 0 &&
        ((RwObject*)(*resourceEntry)->owner)->type == 1) {
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
        RwResEntry* oldEntry =
            *(RwResEntry**)((unsigned char*)vertexBuffer->arrays[0].data - 4);
        if (_rwDlTokenQueryDone(
                ((RwGameCubeResEntryHeader*)oldEntry)->data.sync.token) == 0) {
            oldEntry->ownerRef = 0;
            vertexBuffer->arrays[0].data = 0;
            if ((geometry->flags & 0x10) != 0)
                vertexBuffer->arrays[1].data = 0;
        }
    }
    if (vertexBuffer->arrays[0].data == 0) {
        format = *(RpGameCubeVtxFmt**)((unsigned char*)geometry +
                                       _rpDlGeomVtxFmtOffset);
        if (format != 0) {
            size = (geometry->numVertices *
                    skinVertexSizes[format->positionType] + 31) & ~31U;
            size += 6;
            if ((geometry->flags & 0x10) != 0) {
                unsigned int components = format->normalMode != 0 ? 3 : 1;
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
                          ? (unsigned int)geometry->numVertices
                          : (unsigned int)atomic->interpolator.position;
        if (entry == 0 || numVertices == 0) {
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
        entry = *(RwResEntry**)((unsigned char*)vertexBuffer->arrays[0].data - 4);
        ActivateResourceEntry(entry);
    }
    ((RwGameCubeResEntryHeader*)entry)->data.sync.token = _RwDlTokenCurrent;
    return 1;
}

void _rpSkinMatrixBlendUpdate(RwMatrix* destination, RpSkin* skin,
                              const RwMatrix* ltm,
                              RpHAnimHierarchy* hierarchy)
{
    RwMatrix inverse;
    RwMatrix temporary;
    const RwMatrix* transform;
    unsigned int i;

    if (hierarchy == 0)
        return;
    if ((hierarchy->flags & rpHANIMHIERARCHYNOMATRICES) != 0) {
        if (skin->maxNumWeights > 1 || skin->splitData.numMeshes == 3) {
            RwMatrixInvert(&inverse, ltm);
            transform = &inverse;
        } else {
            transform = &_RwDlInvCamLTM;
        }
        for (i = 0; i < skin->numUsedBones; i++) {
            unsigned int bone = skin->usedBoneList[i];
            RwFrame* frame = hierarchy->pNodeInfo[bone].pFrame;
            RwMatrixMultiply(&temporary, &skin->skinToBoneMatrices[bone],
                             RwFrameGetLTM(frame));
            RwMatrixMultiply(&destination[bone], &temporary, transform);
        }
    } else if ((hierarchy->flags & rpHANIMHIERARCHYLOCALSPACEMATRICES) != 0) {
        if (skin->maxNumWeights > 1 || skin->splitData.numMeshes == 3) {
            for (i = 0; i < skin->numUsedBones; i++) {
                unsigned int bone = skin->usedBoneList[i];
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

static unsigned int SkinVertexScalarSize(unsigned char type)
{
    static const unsigned char sizes[5] = {4, 1, 1, 2, 2};
    return sizes[type];
}

void _rpSkinBlendBodyP(RpSkin* skin, const RwMatrix* matrices,
                       unsigned char* source, unsigned char* destination,
                       const RpGameCubeVtxFmt* format,
                       unsigned int numVertices)
{
    RpSkinBlendPositionData data;
    unsigned int stride = format != 0
                          ? SkinVertexScalarSize(format->positionType) * 3
                          : sizeof(float);
    unsigned int i;

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
                        unsigned char* sourcePositions, unsigned char* sourceNormals,
                        unsigned char* destinationPositions,
                        unsigned char* destinationNormals,
                        const RpGameCubeVtxFmt* format,
                        unsigned int numVertices)
{
    RpSkinBlendPositionNormalData data;
    unsigned int posStride = format != 0
                            ? SkinVertexScalarSize(format->positionType) * 3
                            : sizeof(float);
    unsigned int normalStride = format != 0
                               ? SkinVertexScalarSize(format->normalType) * 3
                               : sizeof(float);
    unsigned int nbtStride = format != 0 && format->normalMode != 0
                            ? normalStride * 2
                            : 0;
    unsigned int i;

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
        if ((geometry->flags & 0x02000000) != 0 &&
            _RwDlPreInstanceOptimize == 1)
            *resourceEntry = _rwDlGeometrySkinInstanceOptimized(
                geometry, owner, ownerRef);
        else
            *resourceEntry = _rwDlGeometryInstanceFast(geometry, owner,
                                                        ownerRef);
    } else if ((geometry->flags & 0x02000000) != 0 &&
               _RwDlPreInstanceOptimize == 1) {
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
    RpSkin* skin = *(RpSkin**)((unsigned char*)geometry +
                               _rpSkinGlobals.geometryOffset);

    if (skin->maxNumWeights > 1 || skin->splitData.numMeshes == 3) {
        RwGameCubeResEntryHeader* header =
            (RwGameCubeResEntryHeader*)*resourceEntry;
        RwGameCubeVertexBuffer* vertexBuffer =
            (RwGameCubeVertexBuffer*)(header + 1);
        void** cachedPositions;
        void** cachedNormals;

        if ((*resourceEntry)->owner != 0 &&
            ((RwObject*)(*resourceEntry)->owner)->type == 1) {
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
            return 0;
        {
            SkinAtomicState* atomicData =
                (SkinAtomicState*)((unsigned char*)atomic +
                                    _rpSkinGlobals.atomicOffset);
            _rpSkinMatrixBlendUpdate(_rpSkinGlobals.alignedScratchMemory,
                                     skin,
                                     RwFrameGetLTM(atomic->object.parent),
                                     atomicData->hierarchy);
        }
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
        SkinAtomicState* atomicData =
            (SkinAtomicState*)((unsigned char*)atomic +
                                _rpSkinGlobals.atomicOffset);
        _rpSkinMatrixBlendUpdate(_rpSkinGlobals.alignedScratchMemory, skin,
                                 RwFrameGetLTM(atomic->object.parent),
                                 atomicData->hierarchy);
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
    const RpSkinRLECount* count = &skin->splitData.rleCount[meshIndex];
    unsigned int matrixIndex = 0;
    unsigned int run;

    for (run = 0; run < count->size; run++) {
        const RpSkinRLE* rle =
            &skin->splitData.rle[count->start + run];
        unsigned int i;
        for (i = 0; i < rle->count; i++) {
            _rpSkinLoadMatrix(
                &((RwMatrix*)_rpSkinGlobals.alignedScratchMemory)
                     [rle->startBone + i],
                matrixIndex, normals);
            matrixIndex += 3;
        }
    }
}

void* _rpSkinRenderCallback(
    void* object, RxGameCubeAtomicAllInOneInstanceData* instanceData)
{
    RpAtomic* atomic = (RpAtomic*)object;
    RpGeometry* geometry = atomic->geometry;
    RpSkin* skin = RpSkinGeometryGetSkin(geometry);
    RwGameCubeResEntryHeader* header =
        (RwGameCubeResEntryHeader*)instanceData->resourceEntry;
    RwGameCubeVertexBuffer* vertexBuffer =
        (RwGameCubeVertexBuffer*)(header + 1);
    RwGameCubeDisplayList* displayList =
        (RwGameCubeDisplayList*)&vertexBuffer->arrays[vertexBuffer->numArrays];
    RpMesh* mesh = (RpMesh*)(instanceData->meshHeader + 1);
    RwDlObjectRenderCallBack materialCallback;
    RpGameCubeVtxFmt* format =
        *(RpGameCubeVtxFmt**)((unsigned char*)geometry +
                              _rpDlGeomVtxFmtOffset);
    unsigned int meshCount = instanceData->meshHeader->numMeshes;
    unsigned int meshIndex = 0;

    ((RwGameCubeResEntryHeader*)instanceData->resourceEntry)->data.sync.token =
        _RwDlTokenCurrent;
    _rwDlVtxFmtSetup(format, (RpGameCubeVtxFmtSetupData*)instanceData);
    if (skin->maxNumWeights > 1 || skin->splitData.numMeshes == 3)
        _rwDlTransformSetup(RwFrameGetLTM(atomic->object.parent),
                            (instanceData->geometryFlags & 0x10) != 0);
    else
        GXSetVtxDesc(0, 1);
    if (format == 0)
        format = _rpGameCubeVtxFmtGetDefault();
    materialCallback = _rwDlObjectRenderSetup(
        instanceData->geometryFlags, instanceData->lightMask,
        instanceData->hasAmbient, vertexBuffer->reserved_0x00[1] & 1);
    if (skin->maxNumWeights == 1 && skin->splitData.numMeshes == 0) {
        unsigned int i;
        for (i = 0; i < skin->numUsedBones; i++)
            _rpSkinLoadMatrix(
                &((RwMatrix*)_rpSkinGlobals.alignedScratchMemory)
                     [skin->usedBoneList[i]],
                i * 3, (instanceData->geometryFlags & 0x10) != 0);
    }
    if ((instanceData->geometryFlags & 0x84) != 0) {
        RpMesh* deferredMesh[64];
        RwGameCubeDisplayList* deferredList[64];
        unsigned int deferredIndex[64];
        unsigned int deferredCount = 0;

        while (meshIndex < meshCount) {
            RpMaterial* material = mesh->material;
            SpecularMaterialPluginData* specular =
                (SpecularMaterialPluginData*)((unsigned char*)material +
                                              SpecularMaterialOffset);
            if (!specular->flags.bits.hidden) {
                if (specular->flags.bits.reflectionPass) {
                    deferredMesh[deferredCount] = mesh;
                    deferredList[deferredCount] = displayList;
                    deferredIndex[deferredCount++] = meshIndex;
                } else {
                    RwTexture* texture = material->texture;
                    RwTexture* alpha =
                        RpMaterialGetAlphaPassTexture(material);
                    if (texture != 0 && texture->raster != 0) {
                        RwGameCubeRasterExt* extension =
                            (RwGameCubeRasterExt*)((unsigned char*)texture->raster +
                                                  _RwGameCubeRasterExtOffset);
                        _rwDlRenderStateSetZCompLoc(
                            (extension->hasAlpha & 1) == 0);
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
            SpecularMaterialPluginData* specular =
                (SpecularMaterialPluginData*)((unsigned char*)material +
                                              SpecularMaterialOffset);
            if (!specular->flags.bits.hidden) {
                RwTexture* texture = material->texture;
                RwTexture* alpha = RpMaterialGetAlphaPassTexture(material);
                if (texture != 0 && texture->raster != 0) {
                    RwGameCubeRasterExt* extension =
                        (RwGameCubeRasterExt*)((unsigned char*)texture->raster +
                                              _RwGameCubeRasterExtOffset);
                    _rwDlRenderStateSetZCompLoc(
                        (extension->hasAlpha & 1) == 0);
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
                    skin->splitData.numMeshes != 0)
                    _rpSkinLoadMatrixPalette(
                        skin, deferredIndex[meshIndex],
                        (instanceData->geometryFlags & 0x10) != 0);
                GXCallDisplayList(deferredList[meshIndex]->data,
                                  deferredList[meshIndex]->size);
                _rxGCTevAlphaPassCleanup((RxGCTevAlphaPass*)instanceData);
            }
        }
    } else {
        _rwDlRenderStateSetZCompLoc(1);
        while (meshIndex < meshCount) {
            RpMaterial* material = mesh->material;
            if (materialCallback != 0)
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

static RpSkin* _rpSkinCreate(RpSkin* skin, unsigned int numVertices)
{
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
                float* weights = (float*)&skin->vertexBoneWeights[vertex];
                if (weights[position + candidate] > weights[position]) {
                    float weight = weights[position];
                    unsigned int indices = skin->vertexBoneIndices[vertex];
                    unsigned char* bytes = (unsigned char*)&indices;
                    unsigned char index = bytes[position];
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
        unsigned char* weights;
        unsigned char* indices;
        skin->platformWeights = RwEngineInstance->fpMalloc(
            numVertices * skin->maxNumWeights * 2 + 3, 0x30116);
        if (skin->platformWeights == 0)
            return 0;
        weights = skin->platformWeights;
        for (vertex = 0; vertex < numVertices; vertex++) {
            unsigned int weight;
            unsigned int sum = 0;
            for (weight = 0; weight < skin->maxNumWeights; weight++) {
                *weights = (unsigned char)(128.0f *
                    ((float*)&skin->vertexBoneWeights[vertex])[weight]);
                sum += *weights++;
            }
            for (weight = 0; sum < 128 && weight < skin->maxNumWeights;
                 weight++) {
                weights[weight - skin->maxNumWeights]++;
                sum++;
            }
        }
        skin->platformIndices = (void*)
            (((unsigned int)weights + 3) & ~3U);
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
