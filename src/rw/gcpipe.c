#include "dolphin/gx.h"
#include "libmkparticle/rw_engine.h"
#include "rw/alphapass.h"
#include "rw/gamecube.h"
#include "rw/nodegamecube.h"
#include "rw/rxpipeline.h"

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

RwInt32 _RwDlPreInstanceOptimize = TRUE;

#define SPECULAR_MATERIAL_DATA(material)                                  \
    ((SpecularMaterialData*)((RwUInt8*)(material) +                       \
                             SpecularMaterialOffset))

/* Near match: retail inserts one redundant zero-extension before GXSetDrawSync. */
void _rxGCResEntryWaitDone(RwResEntry* entry)
{
    RwUInt16* token = (RwUInt16*)((RwUInt8*)entry + 0x18);

    if (*token == _RwDlTokenCurrent) {
        GXSetDrawSync((RwUInt32)_RwDlTokenCurrent);
        _RwDlTokenCurrent = (_RwDlTokenCurrent + 1) % 57344;
    }
    do {
    } while (_rwDlTokenQueryDone(*token) == FALSE);
}

/*
 * The renderer's calls, branches, access widths, layouts, display-list order,
 * alpha/specular cleanup, and render-state restoration match retail. The
 * remaining differences are nonvolatile coloring and device-callback address
 * scheduling; retaining extra aliases to force those homes is not justified.
 */
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
    vertexBuffer = (RwGameCubeVertexBuffer*)((RwUInt8*)instanceData->resourceEntry +
                                             0x18);
    vertexArrays = vertexBuffer->arrays;
    displayList = (RwGameCubeDisplayList*)&vertexArrays[
        vertexBuffer->numArrays];
    *(RwUInt16*)((RwUInt8*)instanceData->resourceEntry + 0x18) =
        _RwDlTokenCurrent;

    ltm = RwFrameGetLTM(*(RwFrame**)((RwUInt8*)object + 4));
    vertexFormat = *(RpGameCubeVtxFmt**)(
        (RwUInt8*)((RpAtomic*)object)->geometry + _rpDlGeomVtxFmtOffset);
    _rwDlVtxFmtSetup(vertexFormat,
                     (RpGameCubeVtxFmtSetupData*)instanceData);
    if ((instanceData->geometryFlags & 0x10) != 0) {
        _rwDlTransformSetup(ltm, TRUE);
    } else {
        _rwDlTransformSetup(ltm, FALSE);
    }
    materialCallback = _rwDlObjectRenderSetup(
        instanceData->geometryFlags, instanceData->lightMask,
        instanceData->hasAmbient, vertexBuffer->reserved_0x00[1] & 1);

    numMeshes = instanceData->meshHeader->numMeshes;
    mesh = (RpMesh*)((RwUInt8*)instanceData->meshHeader + 0x10);
    SetupAtomicSpecularity((RpAtomic*)object);

    if ((instanceData->geometryFlags & 0x84) != 0) {
        while (numMeshes-- != 0) {
            RpMaterial* material = mesh->material;
            SpecularMaterialData* specular =
                SPECULAR_MATERIAL_DATA(material);
            RwBool restoreState = FALSE;

            if (specular->flags.bits.hidden == 0) {
                RwUInt32 oldState;
                RwTexture* texture;
                RwTexture* alphaTexture;

                if (specular->flags.bits.cullFront != 0) {
                    restoreState = TRUE;
                    ((RwRenderStateGetCall)RwEngineInstance->fpRenderStateGet)(
                        0x14, &oldState, RwEngineInstance);
                    ((RwRenderStateSetCall)RwEngineInstance->fpRenderStateSet)(
                        0x14, 1, RwEngineInstance);
                }
                texture = material->texture;
                alphaTexture = RpMaterialGetAlphaPassTexture(material);
                if (NULL != materialCallback) {
                    materialCallback(&instanceData->ambient,
                                     (GXColor*)&material->color, material,
                                     material->surface.ambient);
                }
                SetSingleTextureAlphaPassWithAlphaComp(
                    texture, alphaTexture, (RxGCTevAlphaPass*)instanceData);
                if (0.0f != specular->shininess) {
                    RwBool useSpecularMap = FALSE;
                    RwBool hasLighting = FALSE;

                    if ((instanceData->geometryFlags & 8) != 0 &&
                        instanceData->hasAmbient != FALSE) {
                        hasLighting = TRUE;
                    }
                    if (hasLighting != FALSE &&
                        (instanceData->geometryFlags & 0x84) != 0) {
                        useSpecularMap = TRUE;
                    }
                    ProcessSpecularity(material, texture, alphaTexture,
                                       useSpecularMap);
                    GXCallDisplayList(displayList->data, displayList->size);
                    CleanupSpecularity(material, texture, alphaTexture);
                } else {
                    GXCallDisplayList(displayList->data, displayList->size);
                }
                _rxGCTevAlphaPassCleanup((RxGCTevAlphaPass*)instanceData);
                if (restoreState != FALSE) {
                    ((RwRenderStateSetCall)RwEngineInstance->fpRenderStateSet)(
                        0x14, oldState, RwEngineInstance);
                }
            }
            displayList++;
            mesh++;
        }
    } else {
        _rwDlRenderStateSetZCompLoc(TRUE);
        while (numMeshes-- != 0) {
            RpMaterial* material = mesh->material;
            SpecularMaterialData* specular =
                SPECULAR_MATERIAL_DATA(material);

            if (specular->flags.bits.hidden == 0) {
                RwBool restoreState = FALSE;
                RwUInt32 oldState;

                if (specular->flags.bits.cullFront != 0) {
                    restoreState = TRUE;
                    ((RwRenderStateGetCall)RwEngineInstance->fpRenderStateGet)(
                        0x14, &oldState, RwEngineInstance);
                    ((RwRenderStateSetCall)RwEngineInstance->fpRenderStateSet)(
                        0x14, 1, RwEngineInstance);
                }
                if (NULL != materialCallback) {
                    materialCallback(&instanceData->ambient,
                                     (GXColor*)&material->color, material,
                                     material->surface.ambient);
                }
                if (0.0f != specular->shininess) {
                    ProcessSpecularity(material, NULL, NULL, FALSE);
                    GXCallDisplayList(displayList->data, displayList->size);
                    CleanupSpecularity(material, NULL, NULL);
                } else {
                    GXCallDisplayList(displayList->data, displayList->size);
                }
                if (restoreState != FALSE) {
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
