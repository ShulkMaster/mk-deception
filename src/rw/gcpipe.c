#include "dolphin/gx.h"
#include "libmkparticle/rw_engine.h"
#include "rw/alphapass.h"
#include "rw/gamecube.h"
#include "rw/nodegamecube.h"
#include "rw/rxpipeline.h"

extern RwMatrix* RwFrameGetLTM(RwFrame* frame);

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
    void* light;
    RwFrame* frame;
    RwTexture* texture;
    RwTexture* savedTexture;
    RpSurfaceProperties savedSurface;
    RwReal clipValue;
    RwReal shininess;
    RwUInt8 tint[4];
    RwReal gloss;
    SpecularMaterialFlags flags;
    RwUInt8 reserved_0x2D[3];
} SpecularMaterialData;

typedef void* (*RxGCInstanceCallBack)(void*, RwResEntry**);
typedef void* (*RxGCLightingCallBack)(void*, void*);
typedef void* (*RxGCRenderCallBack)(
    void*, RxGameCubeAllInOneInstanceData*);

typedef struct RxGameCubeAllInOnePrivateData {
    RxGCInstanceCallBack instanceCallback;
    RxGCInstanceCallBack reinstanceCallback;
    RxGCLightingCallBack lightingCallback;
    RxGCRenderCallBack renderCallback;
} RxGameCubeAllInOnePrivateData;

typedef void (*RwRenderStateSetCall)(RwUInt32, RwUInt32, RwGlobals*);
typedef void (*RwRenderStateGetCall)(RwUInt32, void*, RwGlobals*);

extern RwUInt16 _RwDlTokenCurrent;
extern RwInt32 _rpDlGeomVtxFmtOffset;
extern RwInt32 SpecularMaterialOffset;
extern RwBool _rwDlTokenQueryDone(RwUInt16 token);
extern void SetupAtomicSpecularity(RpAtomic* atomic);
extern void ProcessSpecularity(RpMaterial* material, RwTexture* texture,
                               RwTexture* alphaTexture,
                               RwBool useSpecularMap);
extern void CleanupSpecularity(RpMaterial* material, RwTexture* texture,
                               RwTexture* alphaTexture);

RwInt32 _RwDlPreInstanceOptimize = 1;


void _rxGCResEntryWaitDone(RwResEntry* entry)
{
    RwUInt16* token = (RwUInt16*)(entry + 1);

    if (*token == _RwDlTokenCurrent) {
        GXSetDrawSync((RwUInt32)_RwDlTokenCurrent);
        _RwDlTokenCurrent = (_RwDlTokenCurrent + 1) % 57344;
    }
    do {
    } while (_rwDlTokenQueryDone(*token) == 0);
}


void* _rxGCDefaultRenderCallback(
    void* object, RxGameCubeAllInOneInstanceData* instanceData)
{
    void* result;
    RwGameCubeVertexBuffer* vertexBuffer;
    RwGameCubeVertexArray* vertexArrays;
    RwGameCubeDisplayList* displayList;
    RpMesh* mesh;
    RwDlObjectRenderCallBack materialCallback;
    RwMatrix* ltm;
    RpGameCubeVtxFmt* vertexFormat;
    RwUInt32 numMeshes;

    result = object;
    vertexBuffer =
        (RwGameCubeVertexBuffer*)(instanceData->resourceEntry + 1);
    vertexArrays = vertexBuffer->arrays;
    displayList = (RwGameCubeDisplayList*)&vertexArrays[
        vertexBuffer->numArrays];
    *(RwUInt16*)vertexBuffer = _RwDlTokenCurrent;

    ltm = RwFrameGetLTM((RwFrame*)((RpAtomic*)object)->object.parent);
    vertexFormat = *(RpGameCubeVtxFmt**)(
        (RwUInt8*)((RpAtomic*)object)->geometry + _rpDlGeomVtxFmtOffset);
    _rwDlVtxFmtSetup(vertexFormat,
                     (RpGameCubeVtxFmtSetupData*)instanceData);
    if ((instanceData->geometryFlags & 0x10) != 0) {
        _rwDlTransformSetup(ltm, 1);
    } else {
        _rwDlTransformSetup(ltm, 0);
    }
    materialCallback = _rwDlObjectRenderSetup(
        instanceData->geometryFlags, instanceData->lightMask,
        instanceData->hasAmbient, vertexBuffer->reserved_0x00[1] & 1);

    numMeshes = instanceData->meshHeader->numMeshes;
    mesh = (RpMesh*)(instanceData->meshHeader + 1);
    SetupAtomicSpecularity((RpAtomic*)object);

    if ((instanceData->geometryFlags & 0x84) != 0) {
        while (numMeshes-- != 0) {
            RpMaterial* material = mesh->material;
            SpecularMaterialData* specular = (SpecularMaterialData*)(
                (RwUInt8*)material + SpecularMaterialOffset);
            RwBool restoreState = 0;

            if (specular->flags.bits.hidden == 0) {
                RwUInt32 oldState;
                RwTexture* texture;
                RwTexture* alphaTexture;

                if (specular->flags.bits.cullFront != 0) {
                    restoreState = 1;
                    ((RwRenderStateGetCall)RwEngineInstance->fpRenderStateGet)(
                        0x14, &oldState, RwEngineInstance);
                    ((RwRenderStateSetCall)RwEngineInstance->fpRenderStateSet)(
                        0x14, 1, RwEngineInstance);
                }
                texture = material->texture;
                alphaTexture = RpMaterialGetAlphaPassTexture(material);
                if (0 != materialCallback) {
                    materialCallback(&instanceData->ambient,
                                     (GXColor*)&material->color, material,
                                     material->surface.ambient);
                }
                SetSingleTextureAlphaPassWithAlphaComp(
                    texture, alphaTexture, (RxGCTevAlphaPass*)instanceData);
                if (0.0f != specular->shininess) {
                    RwBool useSpecularMap = 0;
                    RwBool hasLighting = 0;

                    if ((instanceData->geometryFlags & 8) != 0 &&
                        instanceData->hasAmbient != 0) {
                        hasLighting = 1;
                    }
                    if (hasLighting != 0 &&
                        (instanceData->geometryFlags & 0x84) != 0) {
                        useSpecularMap = 1;
                    }
                    ProcessSpecularity(material, texture, alphaTexture,
                                       useSpecularMap);
                    GXCallDisplayList(displayList->data, displayList->size);
                    CleanupSpecularity(material, texture, alphaTexture);
                } else {
                    GXCallDisplayList(displayList->data, displayList->size);
                }
                _rxGCTevAlphaPassCleanup((RxGCTevAlphaPass*)instanceData);
                if (restoreState != 0) {
                    ((RwRenderStateSetCall)RwEngineInstance->fpRenderStateSet)(
                        0x14, oldState, RwEngineInstance);
                }
            }
            displayList++;
            mesh++;
        }
    } else {
        _rwDlRenderStateSetZCompLoc(1);
        while (numMeshes-- != 0) {
            RpMaterial* material = mesh->material;
            SpecularMaterialData* specular = (SpecularMaterialData*)(
                (RwUInt8*)material + SpecularMaterialOffset);

            if (specular->flags.bits.hidden == 0) {
                RwBool restoreState = 0;
                RwUInt32 oldState;

                if (specular->flags.bits.cullFront != 0) {
                    restoreState = 1;
                    ((RwRenderStateGetCall)RwEngineInstance->fpRenderStateGet)(
                        0x14, &oldState, RwEngineInstance);
                    ((RwRenderStateSetCall)RwEngineInstance->fpRenderStateSet)(
                        0x14, 1, RwEngineInstance);
                }
                if (0 != materialCallback) {
                    materialCallback(&instanceData->ambient,
                                     (GXColor*)&material->color, material,
                                     material->surface.ambient);
                }
                if (0.0f != specular->shininess) {
                    ProcessSpecularity(material, 0, 0, 0);
                    GXCallDisplayList(displayList->data, displayList->size);
                    CleanupSpecularity(material, 0, 0);
                } else {
                    GXCallDisplayList(displayList->data, displayList->size);
                }
                if (restoreState != 0) {
                    ((RwRenderStateSetCall)RwEngineInstance->fpRenderStateSet)(
                        0x14, oldState, RwEngineInstance);
                }
            }
            displayList++;
            mesh++;
        }
    }
    return result;
}

RxPipelineNode* _rxGameCubeAllInOneSetInstanceCallBack(
    RxPipelineNode* node, RxGCInstanceCallBack callback)
{
    RxGameCubeAllInOnePrivateData* privateData =
        (RxGameCubeAllInOnePrivateData*)node->privateData;

    privateData->instanceCallback = callback;
    return node;
}

RxPipelineNode* _rxGameCubeAllInOneSetReinstanceCallBack(
    RxPipelineNode* node, RxGCInstanceCallBack callback)
{
    RxGameCubeAllInOnePrivateData* privateData =
        (RxGameCubeAllInOnePrivateData*)node->privateData;

    privateData->reinstanceCallback = callback;
    return node;
}

RxPipelineNode* _rxGameCubeAllInOneSetLightingCallBack(
    RxPipelineNode* node, RxGCLightingCallBack callback)
{
    RxGameCubeAllInOnePrivateData* privateData =
        (RxGameCubeAllInOnePrivateData*)node->privateData;

    privateData->lightingCallback = callback;
    return node;
}

RxPipelineNode* RxGameCubeAllInOneSetRenderCallBack(
    RxPipelineNode* node, RxGCRenderCallBack callback)
{
    RxGameCubeAllInOnePrivateData* privateData =
        (RxGameCubeAllInOnePrivateData*)node->privateData;

    privateData->renderCallback = callback;
    return node;
}

void RxGameCubePreInstanceSetOptimize(RwInt32 optimize)
{
    _RwDlPreInstanceOptimize = optimize;
}

RwInt32 RxGameCubePreInstanceGetOptimize(void)
{
    return _RwDlPreInstanceOptimize;
}
