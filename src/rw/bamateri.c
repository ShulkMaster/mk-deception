#include "libmkparticle/rw_engine.h"
#include "runtime/cstring.h"
#include "rw/rpworld_types.h"
#include "rw/rwerror.h"
#include "rw/rwfreelist.h"
#include "rw/rxpipeline.h"
#include "rw/rwstream.h"

typedef struct RpMaterialChunkInfo {
    RwInt32 flags;
    RpMaterialColor color;
    RwInt32 unused;
    RwInt32 textured;
    RpSurfaceProperties surface;
} RpMaterialChunkInfo;

static RwPluginRegistry materialTKList = {0x1C, 0x1C, 0, 0, 0, 0};
static RpSurfaceProperties defaultSurfaceProperties = {1.0f, 1.0f, 1.0f};
static RwFreeList _rpMaterialFreeList;
static RwInt32 _rpMaterialFreeListBlockSize = 0x100;
static RwInt32 _rpMaterialFreeListPreallocBlocks = 1;
static RwInt32 lastSeenExtraData;
static RwUInt32 lastSeenRightsPluginId;
static RwModuleInfo materialModule;

#define MATERIALFREELIST \
    RWPLUGINOFFSET(RwFreeList*, RwEngineInstance, materialModule.globalsOffset)

RwStream* _rpReadMaterialRights(RwStream* stream, RwInt32 length)
{
    if (!RwStreamReadInt32(stream, (RwInt32*)&lastSeenRightsPluginId, 4)) return NULL;
    if (length == 8 && !RwStreamReadInt32(stream, &lastSeenExtraData, 4)) return NULL;
    return stream;
}

RwStream* _rpWriteMaterialRights(RwStream* stream, RwInt32,
                                  const RpMaterial* object)
{
    const RpMaterial* material = object;
    if (!RwStreamWriteInt32(stream,
                            (const RwInt32*)&material->pipeline->pluginId, 4)) return NULL;
    if (!RwStreamWriteInt32(stream,
                            (const RwInt32*)&material->pipeline->pluginData, 4)) return NULL;
    return stream;
}

RwInt32 _rpSizeMaterialRights(const RpMaterial* object)
{
    const RpMaterial* material = object;
    if (material->pipeline != NULL && material->pipeline->pluginId != 0) return 8;
    return 0;
}

void _rpMaterialSetDefaultSurfaceProperties(const RpSurfaceProperties* surface)
{
    if (surface == NULL) {
        defaultSurfaceProperties.ambient = 1.0f;
        defaultSurfaceProperties.diffuse = 1.0f;
        defaultSurfaceProperties.specular = 1.0f;
    } else {
        defaultSurfaceProperties = *surface;
    }
}

void* _rpMaterialOpen(void* instance, RwInt32 offset, RwInt32)
{
    materialModule.globalsOffset = offset;
    MATERIALFREELIST = RwFreeListCreateAndPreallocateSpace(
        materialTKList.sizeOfStruct, _rpMaterialFreeListBlockSize, 4,
        _rpMaterialFreeListPreallocBlocks, &_rpMaterialFreeList, 0x40007);
    if (MATERIALFREELIST == NULL) return NULL;
    materialModule.numInstances++;
    return instance;
}

void* _rpMaterialClose(void* instance, RwInt32, RwInt32)
{
    if (MATERIALFREELIST != NULL) {
        RwFreeListDestroy(MATERIALFREELIST);
        MATERIALFREELIST = NULL;
    }
    materialModule.numInstances--;
    return instance;
}

/* Near miss: retail retains the allocated pointer on the stack through SDK
 * debug macros; initialization, plugin setup, and ownership are identical. */
RpMaterial* RpMaterialCreate(void)
{
    RpMaterial* material;
    RpMaterialColor color;

    material = RwEngineInstance->fpFreeListAlloc(MATERIALFREELIST, 0x30007);
    if (material == NULL) return NULL;
    material->refCount = 1;
    color.red = 0xFF;
    color.green = 0xFF;
    color.blue = 0xFF;
    color.alpha = 0xFF;
    material->color = color;
    material->texture = NULL;
    material->pipeline = NULL;
    material->surface = defaultSurfaceProperties;
    _rwPluginRegistryInitObject(&materialTKList, material);
    return material;
}

RwBool RpMaterialDestroy(RpMaterial* material)
{
    if (material->refCount == 1) {
        _rwPluginRegistryDeInitObject(&materialTKList, material);
        RpMaterialSetTexture(material, NULL);
        RwEngineInstance->fpFreeListFree(MATERIALFREELIST, material);
    } else {
        material->refCount--;
    }
    return TRUE;
}

/* Near miss: retail evaluates an unused address check for the texture
 * argument; clean C preserves the exact refcount/destroy/store behavior. */
RpMaterial* RpMaterialSetTexture(RpMaterial* material, RwTexture* texture)
{
    RwTexture* newTexture = texture;
    if (newTexture != NULL) newTexture->ref_count++;
    if (material->texture != NULL) RwTextureDestroy(material->texture);
    material->texture = newTexture;
    return material;
}

RwInt32 RpMaterialRegisterPlugin(RwInt32 size, RwUInt32 pluginID,
                                 RwPluginObjectConstructor constructCB,
                                 RwPluginObjectDestructor destructCB,
                                 RwPluginObjectCopy copyCB)
{
    RwInt32 offset = _rwPluginRegistryAddPlugin(
        &materialTKList, size, pluginID, constructCB, destructCB, copyCB);
    return offset;
}

RwInt32 RpMaterialRegisterPluginStream(RwUInt32 pluginID,
                                       RwPluginDataChunkReadCallBack readCB,
                                       RwPluginDataChunkWriteCallBack writeCB,
                                       RwPluginDataChunkGetSizeCallBack getSizeCB)
{
    RwInt32 offset = _rwPluginRegistryAddPluginStream(
        &materialTKList, pluginID, readCB, writeCB, getSizeCB);
    return offset;
}

/* Near miss: retail SDK debug macros retain the material and stream locals in
 * a larger frame; chunk parsing, cleanup, rights, and plugin flow are exact. */
RpMaterial* RpMaterialStreamRead(RwStream* stream)
{
    RpMaterialChunkInfo chunk;
    RpMaterial* material;
    RwUInt8 color[4];
    RwUInt32 length;
    RwUInt32 version;
    RwError error;

    if (!RwStreamFindChunk(stream, 1, &length, &version)) return NULL;
    if (version < 0x34000 || version > 0x36003) {
        error.pluginID = 2;
        error.errorCode = _rwerror(0x80000004);
        RwErrorSet(&error);
        return NULL;
    }
    memset(&chunk, 0, sizeof(chunk));
    if (RwStreamRead(stream, &chunk, length) != length) return NULL;
    color[0] = chunk.color.red;
    color[1] = chunk.color.green;
    color[2] = chunk.color.blue;
    color[3] = chunk.color.alpha;
    RwMemNative32(&chunk, sizeof(chunk));
    chunk.color.red = color[0];
    chunk.color.green = color[1];
    chunk.color.blue = color[2];
    chunk.color.alpha = color[3];

    material = RpMaterialCreate();
    if (material == NULL) return NULL;
    material->color = chunk.color;
    material->surface = chunk.surface;
    material->texture = NULL;
    if (chunk.textured != 0) {
        if (!RwStreamFindChunk(stream, 6, NULL, &version)) {
            RpMaterialDestroy(material);
            return NULL;
        }
        if (version < 0x34000 || version > 0x36003) {
            RpMaterialDestroy(material);
            error.pluginID = 2;
            error.errorCode = _rwerror(0x80000004);
            RwErrorSet(&error);
            return NULL;
        }
        material->texture = RwTextureStreamRead(stream);
    }
    lastSeenRightsPluginId = 0;
    lastSeenExtraData = 0;
    if (!_rwPluginRegistryReadDataChunks(&materialTKList, stream, material)) {
        RpMaterialDestroy(material);
        return NULL;
    }
    if (lastSeenRightsPluginId != 0) {
        _rwPluginRegistryInvokeRights(&materialTKList, lastSeenRightsPluginId,
                                      material, lastSeenExtraData);
    }
    return material;
}
