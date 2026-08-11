#include "dolphin/gx.h"
#include "libmkparticle/rw_engine.h"
#include "platform/gcpipemanager.h"
#include "rw/alphapass.h"
#include "rw/gamecube.h"
#include "rw/nodegamecube.h"
#include "rw/rpmatfx.h"
#include "rw/rwresources.h"

typedef struct RwGameCubeLightingData {
    RwUInt8 reserved_0x00[0x0C];
    RwRGBAReal ambient;
    RwBool hasAmbient;
    RwUInt32 lightMask;
    RwInt32 lightIndex;
} RwGameCubeLightingData;

typedef struct RxGameCubeAllInOneInstanceData {
    RwResEntry* resourceEntry;
    RpMeshHeader* meshHeader;
    RwUInt32 geometryFlags;
    RwRGBAReal ambient;
    RwBool hasAmbient;
    RwUInt32 lightMask;
    RwInt32 lightIndex;
    void* morphData;
} RxGameCubeAllInOneInstanceData;

typedef struct MatFXStateCache {
    RwBool lightingSetup;
    RpMaterial* material;
    RwInt32 effect;
} MatFXStateCache;

typedef struct SpecularMaterialData {
    void* light;
    RwFrame* frame;
    RwTexture* texture;
    RwTexture* savedTexture;
    RpSurfaceProperties savedSurface;
    RwReal clipValue;
    RwReal shininess;
    RwUInt8 tint[4];
    RwReal gloss;
    RwUInt8 flags;
    RwUInt8 reserved_0x2D[3];
} SpecularMaterialData;

typedef struct SpecularLightData {
    RwUInt8 reserved_0x00[0x18];
    RwRGBAReal color;
} SpecularLightData;

typedef struct RwGameCubeDisplayResource {
    RwUInt16 token;
    RwUInt16 reserved_0x02;
    RwUInt32 reserved_0x04;
    RwUInt32 numDisplayLists;
    RwUInt8 reserved_0x0C[8];
    RwGameCubeDisplayList displayLists[1];
} RwGameCubeDisplayResource;

typedef void (*RwRenderStateSetCall)(RwUInt32, RwUInt32, RwGlobals*);
typedef void (*RwRenderStateGetCall)(RwUInt32, void*, RwGlobals*);

extern RwInt32 MatFXMaterialDataOffset;
extern RwInt32 SpecularMaterialOffset;
extern RwInt32 _rpDlGeomVtxFmtOffset;
extern RwUInt16 _RwDlTokenCurrent;
extern RpAtomic* _rxGCAtomicDefaultLightingCallback(
    RpAtomic*, RwGameCubeLightingData*);

extern void SetupAtomicSpecularity(RpAtomic* atomic);
extern void ProcessSpecularity(RpMaterial*, RwTexture*, RwTexture*, RwBool);
extern void CleanupSpecularity(RpMaterial*, RwTexture*, RwTexture*);
extern void SetFirstTextureAlphaPassWithAlphaComp(
    RwTexture*, RwTexture*, RxGCTevAlphaPass*);
extern RwTexture* _rpMatFXTextureMaskCreate(
    const RwTexture* baseTexture, const RwTexture* effectTexture);
extern void* _rxGCAtomicDefaultInstanceCallback(void*, RwResEntry**);
extern void* _rxGCAtomicDefaultReinstanceCallback(void*, RwResEntry**);
extern RxPipeline* _rpDlAtomicPipelineCreate(
    RwUInt32, RwUInt32, void*, void*, void*, void*);
extern RwBool RxGameCubePreInstanceGetOptimize(void);
extern void RxGameCubePreInstanceSetOptimize(RwBool optimize);
extern RwBool _rpGameCubeMTPipeDataQueryNBTs(
    const RxGameCubeAllInOneInstanceData* data);
extern void _rpGameCubeMTPipeDataCalcNBTs(
    RxGameCubeAllInOneInstanceData* data,
    const RpGameCubeVtxFmt* format, RwInt32 numVertices);
extern RwMatrix* RwFrameGetLTM(RwFrame* frame);

static const GXColor opaqueWhite = {255, 255, 255, 255};
static const GXColor opaqueBlack = {0, 0, 0, 255};
static const GXColor channelMaterialWhite = {255, 255, 255, 255};
static MatFXStateCache FXStateCache;
RxPipeline* _RpMatFXAtomicPipe;
RpGameCubeVtxFmt* _rpGCMatFXVtxFmtNBT;

static RpMatFXMaterialData* MatFXData(const RpMaterial* material)
{
    return *(RpMatFXMaterialData**)((RwUInt8*)material +
                                    MatFXMaterialDataOffset);
}

static SpecularMaterialData* SpecularData(const RpMaterial* material)
{
    return (SpecularMaterialData*)((RwUInt8*)material +
                                   SpecularMaterialOffset);
}

static RpGameCubeVtxFmt* GeometryVertexFormat(const RpGeometry* geometry)
{
    return *(RpGameCubeVtxFmt**)((RwUInt8*)geometry + _rpDlGeomVtxFmtOffset);
}

static void GXSetTexCoordGen(RwInt32 dst, RwInt32 func, RwInt32 src,
                             RwUInt32 matrix);

static void _rxGCChannelLightingSetup(
    const RxGameCubeAllInOneInstanceData* data)
{
    RwUInt32 flags = data->geometryFlags;

    if (data->lightIndex > 0) {
        RwBool ambientSource;
        if ((flags & 8) != 0) {
            ambientSource = 1;
        } else {
            ambientSource = 0;
            if (!data->hasAmbient)
                GXSetChanAmbColor(4, opaqueBlack);
        }
        GXSetNumChans(1);
        GXSetChanCtrl(4, 1, ambientSource, 0, data->lightMask, 2, 1);
        if ((flags & 0x40) == 0)
            GXSetChanMatColor(4, opaqueWhite);
    } else if ((flags & 8) != 0) {
        GXSetNumChans(1);
        if ((flags & 0x40) != 0)
            GXSetChanCtrl(4, 1, 1, 0, 0, 0, 2);
        else
            GXSetChanCtrl(4, 0, 0, 1, 0, 0, 2);
    } else {
        if (!data->hasAmbient && (flags & 0x40) == 0)
            GXSetChanMatColor(4, opaqueBlack);
        GXSetNumChans(1);
        GXSetChanCtrl(4, 0, 0, 0, 0, 0, 2);
    }
}

static void _rxGCChannelMaterialSetup(
    const RxGameCubeAllInOneInstanceData* data, const RpMaterial* material)
{
    RwUInt32 flags = data->geometryFlags;
    GXColor color;

    if (data->lightIndex > 0) {
        if (data->hasAmbient && (flags & 8) == 0) {
            RwReal scale = 255.0f * material->surface.ambient;
            color.r = (RwUInt8)(scale * data->ambient.red);
            color.g = (RwUInt8)(scale * data->ambient.green);
            color.b = (RwUInt8)(scale * data->ambient.blue);
            color.a = 255;
            GXSetChanAmbColor(4, color);
        }
        if ((flags & 0x40) != 0)
            GXSetChanMatColor(4, *(GXColor*)&material->color);
    } else if ((flags & 8) != 0) {
        if ((flags & 0x40) != 0)
            GXSetChanMatColor(4, *(GXColor*)&material->color);
        else
            GXSetChanMatColor(4, channelMaterialWhite);

        if (data->hasAmbient) {
            RwReal scale = 255.0f * material->surface.ambient;
            if ((flags & 0x40) != 0) {
                color.r = (RwUInt8)(material->color.red *
                                     (scale * data->ambient.red));
                color.g = (RwUInt8)(material->color.green *
                                     (scale * data->ambient.green));
                color.b = (RwUInt8)(material->color.blue *
                                     (scale * data->ambient.blue));
                color.a = material->color.alpha;
            } else {
                scale *= 255.0f;
                color.r = (RwUInt8)(scale * data->ambient.red);
                color.g = (RwUInt8)(scale * data->ambient.green);
                color.b = (RwUInt8)(scale * data->ambient.blue);
                color.a = 255;
            }
            GXSetChanAmbColor(4, color);
        }
    } else if (data->hasAmbient) {
        RwReal scale = material->surface.ambient;
        if ((flags & 0x40) != 0) {
            color.r = (RwUInt8)(material->color.red *
                                 (scale * data->ambient.red));
            color.g = (RwUInt8)(material->color.green *
                                 (scale * data->ambient.green));
            color.b = (RwUInt8)(material->color.blue *
                                 (scale * data->ambient.blue));
            color.a = material->color.alpha;
        } else {
            scale *= 255.0f;
            color.r = (RwUInt8)(scale * data->ambient.red);
            color.g = (RwUInt8)(scale * data->ambient.green);
            color.b = (RwUInt8)(scale * data->ambient.blue);
            color.a = 255;
        }
        GXSetChanMatColor(4, color);
    } else if ((flags & 0x40) != 0) {
        color.r = color.g = color.b = 0;
        color.a = material->color.alpha;
        GXSetChanMatColor(4, color);
    }
}

void _rpDlMatFXStateCacheInit(void)
{
    FXStateCache.effect = 0;
    FXStateCache.lightingSetup = 0;
    FXStateCache.material = 0;
}

static void SetupMaterial(RpMaterial* material,
                          RxGameCubeAllInOneInstanceData* data)
{
    RwGameCubeVertexBuffer* vertexBuffer =
        (RwGameCubeVertexBuffer*)((RwUInt8*)data->resourceEntry + 0x18);
    DpMaterialCallback callback;

    if (!FXStateCache.lightingSetup) {
        _rxGCChannelLightingSetup(data);
        FXStateCache.lightingSetup = 1;
    }
    if (FXStateCache.material != material) {
        _rxGCChannelMaterialSetup(data, material);
        FXStateCache.material = material;
    }
    FXStateCache.effect = 0;
    callback = DPObjectRenderSetup(data->geometryFlags, data->lightMask,
                                   data->hasAmbient,
                                   vertexBuffer->reserved_0x00[1] & 1);
    if (callback != 0) {
        callback((RwRGBAReal*)&data->ambient, (GXColor*)&material->color,
                 material, material->surface.ambient);
    }
}

static void MeshRenderStandard(
    RpMesh* mesh, RwGameCubeDisplayList* displayList,
    RxGameCubeAllInOneInstanceData* data)
{
    RpMaterial* material = mesh->material;
    RwTexture* texture = material->texture;
    RwTexture* alphaTexture = RpMaterialGetAlphaPassTexture(material);
    SpecularMaterialData* specular = SpecularData(material);
    RwBool useSpecularMap;

    SetupMaterial(material, data);
    if ((data->geometryFlags & 8) != 0 && data->hasAmbient &&
        (data->geometryFlags & 0x84) != 0)
        useSpecularMap = 1;
    else
        useSpecularMap = 0;
    SetFirstTextureAlphaPassWithAlphaComp(
        texture, alphaTexture, (RxGCTevAlphaPass*)data);
    if (specular->shininess != 0.0f) {
        ProcessSpecularity(material, texture, alphaTexture,
                           useSpecularMap);
        GXCallDisplayList(displayList->data, displayList->size);
        CleanupSpecularity(material, texture, alphaTexture);
    } else {
        GXCallDisplayList(displayList->data, displayList->size);
    }
    if (alphaTexture != 0)
        _rxGCTevAlphaMultiPassCleanup((RxGCTevAlphaPass*)data);
}

static void LoadUVMatrix(const RwMatrix* matrix, RwInt32 id, Mtx texMatrix)
{
    texMatrix[0][0] = matrix->right.x;
    texMatrix[0][1] = matrix->up.x;
    texMatrix[0][2] = 0.0f;
    texMatrix[0][3] = matrix->pos.x;
    texMatrix[1][0] = matrix->right.y;
    texMatrix[1][1] = matrix->up.y;
    texMatrix[1][2] = 0.0f;
    texMatrix[1][3] = matrix->pos.y;
    GXLoadTexMtxImm(texMatrix, id, 1);
}

static void MeshRenderUVAnim(
    RpMesh* mesh, RwGameCubeDisplayList* displayList,
    RxGameCubeAllInOneInstanceData* data)
{
    RpMaterial* material = mesh->material;
    RwTexture* texture = material->texture;
    RpMatFXMaterialData* matFX = MatFXData(material);
    RwTexture* alphaTexture;
    SpecularMaterialData* specular = SpecularData(material);
    RwBool useSpecularMap;
    RwReal texMatrix[2][4];

    SetupMaterial(material, data);
    if ((data->geometryFlags & 8) != 0 && data->hasAmbient &&
        (data->geometryFlags & 0x84) != 0)
        useSpecularMap = 1;
    else
        useSpecularMap = 0;
    alphaTexture = RpMaterialGetAlphaPassTexture(material);
    SetFirstTextureAlphaPassWithAlphaComp(
        texture, alphaTexture, (RxGCTevAlphaPass*)data);
    if (matFX->slot[0].data.uv.baseTransform != 0) {
        LoadUVMatrix(matFX->slot[0].data.uv.baseTransform, 0x1E,
                       texMatrix);
        GXSetTexCoordGen(0, 1, 4, 0x1E);
    }
    if (specular->shininess != 0.0f) {
        ProcessSpecularity(material, texture, alphaTexture,
                           useSpecularMap);
        GXCallDisplayList(displayList->data, displayList->size);
        CleanupSpecularity(material, texture, alphaTexture);
    } else {
        GXCallDisplayList(displayList->data, displayList->size);
    }
    if (alphaTexture != 0)
        _rxGCTevAlphaMultiPassCleanup((RxGCTevAlphaPass*)data);
}

static void MKMeshRenderDual(RpMesh*, RwGameCubeDisplayList*,
                             RxGameCubeAllInOneInstanceData*);
static void MKMeshRenderUVAnimDual(RpMesh*, RwGameCubeDisplayList*,
                                   RxGameCubeAllInOneInstanceData*);

void _rpDlMatFXMeshRender(
    RpMesh* mesh, RwGameCubeDisplayList* displayList,
    RpAtomic* atomic, RwMatrix* ltm,
    RxGameCubeAllInOneInstanceData* data)
{
    RpMatFXMaterialData* matFX = MatFXData(mesh->material);
    if (matFX != 0) {
        switch (matFX->effects) {
        case rpMATFXEFFECTDUAL:
            MKMeshRenderDual(mesh, displayList, data);
            return;
        case rpMATFXEFFECTUVTRANSFORM:
            MeshRenderUVAnim(mesh, displayList, data);
            return;
        case rpMATFXEFFECTDUALUVTRANSFORM:
            MKMeshRenderUVAnimDual(mesh, displayList, data);
            return;
        default:
            MeshRenderStandard(mesh, displayList, data);
            return;
        }
    } else {
        MeshRenderStandard(mesh, displayList, data);
    }
}

void* _rpGCMatFXAtomicInstanceCallBack(
    RpAtomic* atomic, RxGameCubeAllInOneInstanceData* data)
{

    if (_rpGameCubeMTPipeDataQueryNBTs(data)) {
        RpGeometry* geometry = atomic->geometry;
        RpGameCubeVtxFmt* format = GeometryVertexFormat(geometry);
        RwBool optimize;
        void* result;
        if (format == 0) {
            RpGameCubeGeometrySetVtxFmt(geometry, _rpGCMatFXVtxFmtNBT);
            format = _rpGCMatFXVtxFmtNBT;
        }
        optimize = RxGameCubePreInstanceGetOptimize();
        if (optimize == 1)
            RxGameCubePreInstanceSetOptimize(0);
        result = _rxGCAtomicDefaultInstanceCallback(
            atomic, (RwResEntry**)data);
        if (optimize == 1)
            RxGameCubePreInstanceSetOptimize(1);
        if (result != 0)
            _rpGameCubeMTPipeDataCalcNBTs(
                data, format, geometry->numVertices);
        return result;
    }
    return _rxGCAtomicDefaultInstanceCallback(atomic, (RwResEntry**)data);
}

void* _rpGCMatFXAtomicReinstanceCallBack(
    RpAtomic* atomic, RxGameCubeAllInOneInstanceData* data)
{
    RpGeometry* geometry = atomic->geometry;
    RpGameCubeVtxFmt* format = GeometryVertexFormat(geometry);
    RwBool calculateNBTs = 0;

    if (format != 0 && format->normalMode != 0 &&
        (geometry->flags & 0x10) != 0) {
        RwBool morphing = 0;
        RwBool required;
        if (geometry->numMorphTargets != 1 &&
            (atomic->interpolator.flags & 1) != 0)
            morphing = 1;
        required = 1;
        if ((geometry->lockedSinceLastInst & 0x17) == 0 && !morphing)
            required = 0;
        calculateNBTs = required;
    }
    if (_rxGCAtomicDefaultReinstanceCallback(
            atomic, (RwResEntry**)data) == 0)
        return 0;
    if (calculateNBTs)
        _rpGameCubeMTPipeDataCalcNBTs(data, format, geometry->numVertices);
    return atomic;
}

void* _rpGCMatFXRenderCallback(
    RpAtomic* atomic, RxGameCubeAllInOneInstanceData* data)
{
    RwGameCubeDisplayResource* resource =
        (RwGameCubeDisplayResource*)((RwUInt8*)data->resourceEntry + 0x18);
    RwGameCubeDisplayList* displayList =
        &resource->displayLists[resource->numDisplayLists - 1];
    RpMesh* mesh = (RpMesh*)(data->meshHeader + 1);
    RwUInt32 numMeshes = data->meshHeader->numMeshes;
    RwMatrix* ltm = RwFrameGetLTM((RwFrame*)atomic->object.parent);

    resource->token = _RwDlTokenCurrent;
    _rwDlVtxFmtSetup(GeometryVertexFormat(atomic->geometry),
                     (RpGameCubeVtxFmtSetupData*)data);
    _rwDlTransformSetup(ltm, (data->geometryFlags & 0x10) != 0);
    _rpDlMatFXStateCacheInit();
    SetupAtomicSpecularity(atomic);
    while (numMeshes-- != 0) {
        SpecularMaterialData* specular = SpecularData(mesh->material);
        if ((specular->flags & 2) == 0) {
            RwBool restoreCull = 0;
            RwInt32 oldCullMode;
            if ((specular->flags & 4) != 0) {
                restoreCull = 1;
                ((RwRenderStateGetCall)RwEngineInstance->fpRenderStateGet)(
                    0x14, &oldCullMode, RwEngineInstance);
                ((RwRenderStateSetCall)RwEngineInstance->fpRenderStateSet)(
                    0x14, 1, RwEngineInstance);
            }
            _rpDlMatFXMeshRender(mesh, displayList, atomic, ltm, data);
            if (restoreCull)
                ((RwRenderStateSetCall)RwEngineInstance->fpRenderStateSet)(
                    0x14, oldCullMode, RwEngineInstance);
        }
        mesh++;
        displayList++;
    }
    return atomic;
}

RwBool _rpMatFXPipelinesCreate(void)
{
    _rpGCMatFXVtxFmtNBT = RpGameCubeVtxFmtCreate();
    RpGameCubeVtxFmtSetNormal(_rpGCMatFXVtxFmtNBT, 4, 1);
    _RpMatFXAtomicPipe = _rpDlAtomicPipelineCreate(
        0x120, 1, _rpGCMatFXAtomicInstanceCallBack,
        _rpGCMatFXAtomicReinstanceCallBack,
        _rxGCAtomicDefaultLightingCallback, _rpGCMatFXRenderCallback);
    return 1;
}

RwBool _rpMatFXPipelinesDestroy(void)
{

    if (_rpGCMatFXVtxFmtNBT != 0) {
        RpGameCubeVtxFmtDestroy(_rpGCMatFXVtxFmtNBT);
        _rpGCMatFXVtxFmtNBT = 0;
    }
    if (_RpMatFXAtomicPipe != 0) {
        _rxPipelineDestroy(_RpMatFXAtomicPipe);
        _RpMatFXAtomicPipe = 0;
    }
    return 1;
}

RpAtomic* _rpMatFXPipelineAtomicSetup(RpAtomic* atomic)
{

    RpAtomic* result = atomic;
    atomic->pipeline = _RpMatFXAtomicPipe;
    return result;
}

void _rpMatFXPipelineWorldSectorSetup(void)
{
}

RwBool _rpMatFXSetupDualRenderState(
    RpMatFXDualData* data, RwInt32 renderState)
{
    return 1;
}

RwTexture* _rpMatFXSetupBumpMapTexture(
    RwTexture* baseTexture, RwTexture* effectTexture)
{
    RwTexture* result =
        _rpMatFXTextureMaskCreate(baseTexture, effectTexture);
    return result;
}

static RwUInt32 SetupAlphaTextureForDualPass(
    RwTexture* texture, RwInt32* tevStage, RwInt32* texMap,
    RwBool secondCoords)
{
    RwInt32 swapStage;
    GXSetTevColorOp(*tevStage, 0, 0, 0, 1, 0);
    GXSetTevAlphaOp(*tevStage, 0, 0, 0, 1, 0);
    (*tevStage)++;
    swapStage = *tevStage;
    GXSetTevSwapMode(*tevStage, 0, 3);
    _rwDlTextureSet(texture, *texMap);
    if (secondCoords)
        GXSetTevOrder(*tevStage, 1, *texMap, 4);
    else
        GXSetTevOrder(*tevStage, 0, *texMap, 4);
    GXSetTevColorIn(*tevStage, 15, 15, 15, 0);
    GXSetTevAlphaIn(*tevStage, 7, 0, 4, 7);
    (*texMap)++;
    return swapStage;
}

static void BlendSetup_Zero_SrcAlpha(RwInt32* tevStage)
{
    GXSetTevOrder(*tevStage, 0xFF, 0xFF, 0xFF);
    GXSetTevColorIn(*tevStage, 15, 5, 2, 15);
    GXSetTevColorOp(*tevStage, 0, 0, 0, 1, 0);
    GXSetTevAlphaIn(*tevStage, 7, 7, 7, 1);
    GXSetTevAlphaOp(*tevStage, 0, 0, 0, 1, 0);
    (*tevStage)++;
}

static void BlendSetup_SrcAlpha_InvSrcAlpha(RwInt32* tevStage)
{
    GXSetTevOrder(*tevStage, 0xFF, 0xFF, 0xFF);
    GXSetTevColorIn(*tevStage, 2, 4, 5, 15);
    GXSetTevColorOp(*tevStage, 0, 0, 0, 1, 0);
    GXSetTevAlphaIn(*tevStage, 7, 7, 7, 1);
    GXSetTevAlphaOp(*tevStage, 0, 0, 0, 1, 0);
    (*tevStage)++;
}

static void BlendSetup_SrcAlpha_One(RwInt32* tevStage)
{
    GXSetTevOrder(*tevStage, 0xFF, 0xFF, 0xFF);
    GXSetTevColorIn(*tevStage, 15, 4, 5, 2);
    GXSetTevColorOp(*tevStage, 0, 0, 0, 1, 0);
    GXSetTevAlphaIn(*tevStage, 7, 7, 7, 1);
    GXSetTevAlphaOp(*tevStage, 0, 0, 0, 1, 0);
    (*tevStage)++;
}

static void BlendSetup_One_One(RwInt32* tevStage)
{
    GXSetTevOrder(*tevStage, 0xFF, 0xFF, 0xFF);
    GXSetTevColorIn(*tevStage, 15, 2, 12, 4);
    GXSetTevColorOp(*tevStage, 0, 0, 0, 1, 0);
    GXSetTevAlphaIn(*tevStage, 7, 7, 7, 1);
    GXSetTevAlphaOp(*tevStage, 0, 0, 0, 1, 0);
    (*tevStage)++;
}

static void MKChooseBlendMode(
    RwInt32 srcBlend, RwInt32 dstBlend, RwInt32* tevStage)
{
    if (srcBlend == 1 && dstBlend == 5)
        BlendSetup_Zero_SrcAlpha(tevStage);
    else if (srcBlend == 5 && dstBlend == 6)
        BlendSetup_SrcAlpha_InvSrcAlpha(tevStage);
    else if (srcBlend == 5 && dstBlend == 2)
        BlendSetup_SrcAlpha_One(tevStage);
    else if (srcBlend == 2 && dstBlend == 2)
        BlendSetup_One_One(tevStage);
}

static void ApplySpecularityToDualPass(
    RpMaterial* material, RwInt32* texCoord, RwInt32* tevStage)
{
    SpecularMaterialData* specular = SpecularData(material);
    SpecularLightData* light = specular->light;
    GXColor color;
    RwReal scale = 2.0f * material->surface.specular;
    RwInt32 coord = (*texCoord)++;

    if (scale > 1.0f)
        scale = 1.0f;
    color.r = (RwUInt8)(specular->tint[0] * (scale * light->color.red));
    color.g = (RwUInt8)(specular->tint[1] * (scale * light->color.green));
    color.b = (RwUInt8)(specular->tint[2] * (scale * light->color.blue));
    color.a = 255;
    GXSetTexCoordGen(coord, 1, 1, 0x39);
    specular->texture->filter_flags =
        (specular->texture->filter_flags & 0xFFFF00FF) | 0x1100;
    _rwDlTextureSet(specular->texture, 4);
    GXSetTevOrder(*tevStage, coord, 4, 0xFF);
    GXSetTevKColor(0, color);
    GXSetTevKColorSel(*tevStage, 0xC);
    GXSetTevColorIn(*tevStage, 15, 8, 14, 0);
    GXSetTevAlphaIn(*tevStage, 7, 7, 7, 0);
    GXSetTevColorOp(*tevStage, 0, 0, 0, 1, 0);
    GXSetTevAlphaOp(*tevStage, 0, 0, 0, 1, 0);
    (*tevStage)++;
}

static void FinishDualPass(RwGameCubeDisplayList* displayList,
                           RwInt32 tevStage, RwInt32 texCoord,
                           RwUInt32 firstSwap, RwUInt32 secondSwap)
{
    GXSetNumTevStages((RwUInt8)tevStage);
    GXSetNumTexGens((RwUInt8)texCoord);
    GXSetAlphaCompare(7, 0, 0, 7, 0);
    GXSetZCompLoc(1);
    GXCallDisplayList(displayList->data, displayList->size);
    if (firstSwap != 0)
        GXSetTevSwapMode(firstSwap, 0, 0);
    if (secondSwap != 0)
        GXSetTevSwapMode(secondSwap, 0, 0);
    FXStateCache.lightingSetup = 0;
    FXStateCache.material = 0;
    FXStateCache.effect = 0;
}

static void MKMeshRenderDual(
    RpMesh* mesh, RwGameCubeDisplayList* displayList,
    RxGameCubeAllInOneInstanceData* data)
{
    RpMaterial* material = mesh->material;
    RpMatFXMaterialData* matFX = MatFXData(material);
    RpMatFXDualData* dual = &matFX->slot[0].data.dual;
    RwTexture* alpha = RpMaterialGetAlphaPassTexture(material);
    RwTexture* dualAlpha = RpMaterialGetDualAlphaPassTexture(material);
    RwInt32 tevStage = 0;
    RwInt32 texMap = 0;
    RwInt32 texCoord = material->texture != 0;
    RwUInt32 firstSwap = 0;
    RwUInt32 secondSwap = 0;

    if (material->texture != 0)
        texMap = 1;
    _rwDlTextureSet(material->texture, 0);
    GXSetTevSwapModeTable(3, 0, 3, 2, 1);
    SetupMaterial(material, data);
    if ((data->geometryFlags & 8) != 0 && data->hasAmbient &&
        (data->geometryFlags & 0x84) != 0)
        tevStage++;
    if (alpha != 0)
        firstSwap = SetupAlphaTextureForDualPass(
            alpha, &tevStage, &texMap, 0);
    GXSetTevColorOp(tevStage, 0, 0, 0, 1, 1);
    GXSetTevAlphaOp(tevStage, 0, 0, 0, 1, 1);
    tevStage++;
    _rwDlTextureSet(dual->texture, texMap);
    if ((data->geometryFlags & 0x80) != 0) {
        GXSetTexCoordGen(1, 1, 5, 0x3C);
        GXSetTevOrder(tevStage, 1, texMap, 4);
        texCoord++;
    } else {
        GXSetTevOrder(tevStage, 0, texMap, 4);
    }
    GXSetTevColorIn(tevStage, 15, 10, 8, 15);
    GXSetTevAlphaIn(tevStage, 7, 7, 7, 4);
    texMap++;
    if (dualAlpha != 0)
        secondSwap = SetupAlphaTextureForDualPass(
            dualAlpha, &tevStage, &texMap,
            (data->geometryFlags & 0x80) != 0);
    GXSetTevColorOp(tevStage, 0, 0, 0, 1, 2);
    GXSetTevAlphaOp(tevStage, 0, 0, 0, 1, 2);
    tevStage++;
    MKChooseBlendMode(dual->srcBlendMode, dual->dstBlendMode, &tevStage);
    if (SpecularData(material)->shininess != 0.0f)
        ApplySpecularityToDualPass(material, &texCoord, &tevStage);
    FinishDualPass(displayList, tevStage, texCoord, firstSwap, secondSwap);
}

static void MKMeshRenderUVAnimDual(
    RpMesh* mesh, RwGameCubeDisplayList* displayList,
    RxGameCubeAllInOneInstanceData* data)
{
    RpMaterial* material = mesh->material;
    RpMatFXMaterialData* matFX = MatFXData(material);
    RpMatFXUVTransformData* uv = &matFX->slot[0].data.uv;
    RpMatFXDualData* dual = &matFX->slot[1].data.dual;
    RwTexture* alpha = RpMaterialGetAlphaPassTexture(material);
    RwTexture* dualAlpha = RpMaterialGetDualAlphaPassTexture(material);
    RwInt32 tevStage = 0;
    RwInt32 texMap = material->texture != 0;
    RwInt32 texCoord = material->texture != 0;
    RwInt32 dualSource;
    RwUInt32 firstSwap = 0;
    RwUInt32 secondSwap = 0;
    RwReal texMatrix[2][4];

    _rwDlTextureSet(material->texture, 0);
    GXSetTevSwapModeTable(3, 0, 3, 2, 1);
    SetupMaterial(material, data);
    if ((data->geometryFlags & 8) != 0 && data->hasAmbient &&
        (data->geometryFlags & 0x84) != 0)
        tevStage++;
    if (uv->baseTransform != 0) {
        LoadUVMatrix(uv->baseTransform, 0x1E, texMatrix);
        GXSetTexCoordGen(0, 1, 4, 0x1E);
    }
    if (alpha != 0)
        firstSwap = SetupAlphaTextureForDualPass(
            alpha, &tevStage, &texMap, 0);
    GXSetTevColorOp(tevStage, 0, 0, 0, 1, 1);
    GXSetTevAlphaOp(tevStage, 0, 0, 0, 1, 1);
    tevStage++;
    _rwDlTextureSet(dual->texture, texMap);
    GXSetTevOrder(tevStage, 1, texMap, 4);
    texCoord++;
    if ((data->geometryFlags & 0x80) != 0)
        dualSource = 5;
    else
        dualSource = 4;
    if (uv->dualTransform != 0) {
        LoadUVMatrix(uv->dualTransform, 0x21, texMatrix);
        GXSetTexCoordGen(1, 1, dualSource, 0x21);
    } else {
        GXSetTexCoordGen(1, 1, dualSource, 0x3C);
    }
    GXSetTevColorIn(tevStage, 15, 10, 8, 15);
    GXSetTevAlphaIn(tevStage, 7, 7, 7, 4);
    texMap++;
    if (dualAlpha != 0)
        secondSwap = SetupAlphaTextureForDualPass(
            dualAlpha, &tevStage, &texMap, 1);
    GXSetTevColorOp(tevStage, 0, 0, 0, 1, 2);
    GXSetTevAlphaOp(tevStage, 0, 0, 0, 1, 2);
    tevStage++;
    MKChooseBlendMode(dual->srcBlendMode, dual->dstBlendMode, &tevStage);
    if (SpecularData(material)->shininess != 0.0f)
        ApplySpecularityToDualPass(material, &texCoord, &tevStage);
    FinishDualPass(displayList, tevStage, texCoord, firstSwap, secondSwap);
}

static void GXSetTexCoordGen(
    RwInt32 dst, RwInt32 func, RwInt32 src, RwUInt32 matrix)
{
    GXSetTexCoordGen2(dst, func, src, matrix, 0, 0x7D);
}
