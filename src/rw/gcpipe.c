#include "dolphin/gx.h"
#include "rw/rwengine.h"
#include "rw/alphapass.h"
#include "rw/gamecube.h"
#include "rw/gcspecular.h"
#include "rw/dltoken.h"
#include "rw/nodegamecube.h"
#include "rw/rxpipeline.h"
#include "rw/rwframe.h"

typedef void* (*RxGCInstanceCallBack)(void*, RwResEntry**);
typedef void* (*RxGCLightingCallBack)(void*, void*);
typedef void* (*RxGCRenderCallBack)(
    void*, RxGameCubeAtomicAllInOneInstanceData*);

typedef struct RxGameCubeAllInOneCallbackSlots {
    RxGCInstanceCallBack instanceCallback;
    RxGCInstanceCallBack reinstanceCallback;
    RxGCLightingCallBack lightingCallback;
    RxGCRenderCallBack renderCallback;
} RxGameCubeAllInOneCallbackSlots;

extern void SetupAtomicSpecularity(RpAtomic* atomic);
extern void ProcessSpecularity(RpMaterial* material, RwTexture* texture,
                               RwTexture* alphaTexture,
                               int useSpecularMap);
extern void CleanupSpecularity(RpMaterial* material, RwTexture* texture,
                               RwTexture* alphaTexture);

int _RwDlPreInstanceOptimize = 1;


void _rxGCResEntryWaitDone(RwResEntry* entry)
{
    unsigned short* token = (unsigned short*)(entry + 1);

    if (*token == _RwDlTokenCurrent) {
        GXSetDrawSync((unsigned int)_RwDlTokenCurrent);
        _RwDlTokenCurrent = (_RwDlTokenCurrent + 1) % 57344;
    }
    do {
    } while (_rwDlTokenQueryDone(*token) == 0);
}


/* TODO: [near miss] 98.94422%; stack exact; retail reuses r5 (arg 3) as the base
 * for the callback's ambient arg, and atomic ranks below specular (r22 vs r24). */
void* _rxGCDefaultRenderCallback(
    void* object, RxGameCubeAtomicAllInOneInstanceData* instanceData)
{
    RwGameCubeVertexBuffer* vertexBuffer;
    RwGameCubeVertexArray* vertexArrays;
    RwGameCubeDisplayList* displayList;
    RpMesh* mesh;
    unsigned int numMeshes;
    RwDlObjectRenderCallBack materialCallback;
    RwMatrix* ltm;
    RpGameCubeVtxFmt* vertexFormat;
    RpAtomic* atomic;
    int restoreState;
    unsigned int oldState;
    int specularMap;
    int restoreState2;
    unsigned int oldState2;

    vertexBuffer =
        (RwGameCubeVertexBuffer*)(instanceData->resourceEntry + 1);
    vertexArrays = vertexBuffer->arrays;
    displayList = (RwGameCubeDisplayList*)&vertexArrays[
        vertexBuffer->numArrays];
    vertexBuffer->displayListToken = (unsigned short)_RwDlTokenCurrent;

    atomic = (RpAtomic*)object;
    ltm = RwFrameGetLTM((RwFrame*)atomic->object.parent);
    vertexFormat = *(RpGameCubeVtxFmt**)(
        (unsigned char*)atomic->geometry + _rpDlGeomVtxFmtOffset);
    _rwDlVtxFmtSetup(vertexFormat,
                     (RpGameCubeVtxFmtSetupData*)instanceData);
    if ((instanceData->geometryFlags & 0x10) != 0) {
        _rwDlTransformSetup(ltm, 1);
    } else {
        _rwDlTransformSetup(ltm, 0);
    }
    materialCallback = _rwDlObjectRenderSetup(
        instanceData->geometryFlags, instanceData->lightMask,
        instanceData->hasAmbient, vertexBuffer->flags & 1);

    numMeshes = instanceData->meshHeader->numMeshes;
    mesh = (RpMesh*)(instanceData->meshHeader + 1);
    SetupAtomicSpecularity((RpAtomic*)object);

    if ((instanceData->geometryFlags & 0x84) != 0) {
        while (numMeshes-- != 0) {
            RpMaterial* material = mesh->material;
            SpecularMaterialPluginData* specular =
                (SpecularMaterialPluginData*)(
                (unsigned char*)material + SpecularMaterialOffset);

            restoreState = 0;
            if (specular->flags.bits.hidden == 0) {
                RwTexture* texture;
                RwTexture* alphaTexture;

                if (specular->flags.bits.cullFront != 0) {
                    restoreState = 1;
                    RwEngineInstance->dOpenDevice.fpRenderStateGet(
                        0x14, &oldState);
                    RwEngineInstance->dOpenDevice.fpRenderStateSet(
                        0x14, 1);
                }
                texture = material->texture;
                alphaTexture = RpMaterialGetAlphaPassTexture(material);
                if ((RwDlObjectRenderCallBack)NULL != materialCallback) {
                    materialCallback(&instanceData->ambient,
                                     (GXColor*)&mesh->material->color,
                                     mesh->material,
                                     mesh->material->surface.ambient);
                }
                SetSingleTextureAlphaPassWithAlphaComp(
                    texture, alphaTexture, (RxGCTevAlphaPass*)instanceData);
                if (specular->shininess) {
                    int useSpecularMap = 0;
                    int hasLighting = 0;

                    if ((instanceData->geometryFlags & 8) != 0 &&
                        instanceData->hasAmbient != 0) {
                        hasLighting = 1;
                    }
                    if (hasLighting != 0 &&
                        (instanceData->geometryFlags & 0x84) != 0) {
                        useSpecularMap = 1;
                    }
                    specularMap = useSpecularMap;
                    ProcessSpecularity(material, texture, alphaTexture,
                                       specularMap);
                    GXCallDisplayList(displayList->data, displayList->size);
                    CleanupSpecularity(material, texture, alphaTexture);
                } else {
                    GXCallDisplayList(displayList->data, displayList->size);
                }
                _rxGCTevAlphaPassCleanup((RxGCTevAlphaPass*)instanceData);
                if (restoreState != 0) {
                    RwEngineInstance->dOpenDevice.fpRenderStateSet(
                        0x14, oldState);
                }
            }
            displayList++;
            mesh++;
        }
    } else {
        _rwDlRenderStateSetZCompLoc(1);
        while (numMeshes-- != 0) {
            RpMaterial* material = mesh->material;
            SpecularMaterialPluginData* specular =
                (SpecularMaterialPluginData*)(
                (unsigned char*)material + SpecularMaterialOffset);

            if (specular->flags.bits.hidden == 0) {
                restoreState2 = 0;
                if (specular->flags.bits.cullFront != 0) {
                    restoreState2 = 1;
                    RwEngineInstance->dOpenDevice.fpRenderStateGet(
                        0x14, &oldState2);
                    RwEngineInstance->dOpenDevice.fpRenderStateSet(
                        0x14, 1);
                }
                if (0 != materialCallback) {
                    materialCallback(&instanceData->ambient,
                                     (GXColor*)&mesh->material->color,
                                     mesh->material,
                                     mesh->material->surface.ambient);
                }
                if (specular->shininess) {
                    ProcessSpecularity(material, 0, 0, 0);
                    GXCallDisplayList(displayList->data, displayList->size);
                    CleanupSpecularity(material, 0, 0);
                } else {
                    GXCallDisplayList(displayList->data, displayList->size);
                }
                if (restoreState2 != 0) {
                    RwEngineInstance->dOpenDevice.fpRenderStateSet(
                        0x14, oldState2);
                }
            }
            displayList++;
            mesh++;
        }
    }
    return object;
}

RxPipelineNode* _rxGameCubeAllInOneSetInstanceCallBack(
    RxPipelineNode* node, RxGCInstanceCallBack callback)
{
    RxGameCubeAllInOneCallbackSlots* privateData =
        (RxGameCubeAllInOneCallbackSlots*)node->privateData;

    privateData->instanceCallback = callback;
    return node;
}

RxPipelineNode* _rxGameCubeAllInOneSetReinstanceCallBack(
    RxPipelineNode* node, RxGCInstanceCallBack callback)
{
    RxGameCubeAllInOneCallbackSlots* privateData =
        (RxGameCubeAllInOneCallbackSlots*)node->privateData;

    privateData->reinstanceCallback = callback;
    return node;
}

RxPipelineNode* _rxGameCubeAllInOneSetLightingCallBack(
    RxPipelineNode* node, RxGCLightingCallBack callback)
{
    RxGameCubeAllInOneCallbackSlots* privateData =
        (RxGameCubeAllInOneCallbackSlots*)node->privateData;

    privateData->lightingCallback = callback;
    return node;
}

RxPipelineNode* RxGameCubeAllInOneSetRenderCallBack(
    RxPipelineNode* node, RxGCRenderCallBack callback)
{
    RxGameCubeAllInOneCallbackSlots* privateData =
        (RxGameCubeAllInOneCallbackSlots*)node->privateData;

    privateData->renderCallback = callback;
    return node;
}

void RxGameCubePreInstanceSetOptimize(int optimize)
{
    _RwDlPreInstanceOptimize = optimize;
}

int RxGameCubePreInstanceGetOptimize(void)
{
    return _RwDlPreInstanceOptimize;
}
