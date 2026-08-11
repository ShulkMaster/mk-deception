#include "libmkparticle/rw_engine.h"
#include "runtime/cstring.h"
#include "rw/batextur.h"
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

RwStream* _rpReadMaterialRights(RwStream* stream, RwInt32 length)
{
    if (!RwStreamReadInt32(stream, (RwInt32*)&lastSeenRightsPluginId, 4)) return 0;
    if (length == 8 && !RwStreamReadInt32(stream, &lastSeenExtraData, 4)) return 0;
    return stream;
}

RwStream* _rpWriteMaterialRights(RwStream* stream, RwInt32,
                                  const RpMaterial* object)
{
    const RpMaterial* material = object;
    if (!RwStreamWriteInt32(stream,
                            (const RwInt32*)&material->pipeline->pluginId, 4)) return 0;
    if (!RwStreamWriteInt32(stream,
                            (const RwInt32*)&material->pipeline->pluginData, 4)) return 0;
    return stream;
}

RwInt32 _rpSizeMaterialRights(const RpMaterial* object)
{
    const RpMaterial* material = object;
    if (material->pipeline != 0 && material->pipeline->pluginId != 0) return 8;
    return 0;
}

void _rpMaterialSetDefaultSurfaceProperties(const RpSurfaceProperties* surface)
{
    if (surface == 0) {
        defaultSurfaceProperties.ambient = 1.0f;
        defaultSurfaceProperties.diffuse = 1.0f;
        defaultSurfaceProperties.specular = 1.0f;
    } else {
        defaultSurfaceProperties = *surface;
    }
}

void* _rpMaterialOpen(void* instance, RwInt32 offset, RwInt32)
{
    RwFreeList** freeList;
    materialModule.globalsOffset = offset;
    freeList = (RwFreeList**)((RwUInt8*)RwEngineInstance + offset);
    *freeList = RwFreeListCreateAndPreallocateSpace(
        materialTKList.sizeOfStruct, _rpMaterialFreeListBlockSize, 4,
        _rpMaterialFreeListPreallocBlocks, &_rpMaterialFreeList, 0x40007);
    if (*freeList == 0) return 0;
    materialModule.numInstances++;
    return instance;
}

void* _rpMaterialClose(void* instance, RwInt32, RwInt32)
{
    RwFreeList** freeList = (RwFreeList**)((RwUInt8*)RwEngineInstance +
                                          materialModule.globalsOffset);
    if (*freeList != 0) {
        RwFreeListDestroy(*freeList);
        *freeList = 0;
    }
    materialModule.numInstances--;
    return instance;
}



RpMaterial* RpMaterialCreate(void)
{
    RwFreeList* freeList = *(RwFreeList**)((RwUInt8*)RwEngineInstance +
                                          materialModule.globalsOffset);
    RpMaterial* material;
    RpMaterialColor color;

    material = RwEngineInstance->fpFreeListAlloc(freeList, 0x30007);
    if (material == 0) return 0;
    material->refCount = 1;
    color.red = 0xFF;
    color.green = 0xFF;
    color.blue = 0xFF;
    color.alpha = 0xFF;
    material->color.red = color.red;
    material->color.green = color.green;
    material->color.blue = color.blue;
    material->color.alpha = color.alpha;
    material->texture = 0;
    material->pipeline = 0;
    material->surface = defaultSurfaceProperties;
    _rwPluginRegistryInitObject(&materialTKList, material);
    return material;
}

RwBool RpMaterialDestroy(RpMaterial* material)
{
    RwFreeList* freeList = *(RwFreeList**)((RwUInt8*)RwEngineInstance +
                                          materialModule.globalsOffset);
    if (material->refCount == 1) {
        _rwPluginRegistryDeInitObject(&materialTKList, material);
        RpMaterialSetTexture(material, 0);
        RwEngineInstance->fpFreeListFree(freeList, material);
    } else {
        material->refCount--;
    }
    return 1;
}



RpMaterial* RpMaterialSetTexture(RpMaterial* material, RwTexture* texture)
{
    RwTexture* newTexture = texture;
    if (newTexture != 0) newTexture->ref_count++;
    if (material->texture != 0) RwTextureDestroy(material->texture);
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




RpMaterial* RpMaterialStreamRead(RwStream* stream)
{
    RwUInt32 length;
    RwUInt32 version;
    RwError error;

    if (!RwStreamFindChunk(stream, 1, &length, &version)) return 0;
    if (version >= 0x34000 && version <= 0x36003) {
        RpMaterialChunkInfo chunk;
        RpMaterial* material;
        RwUInt8 color[4];

        memset(&chunk, 0, sizeof(chunk));
        if (RwStreamRead(stream, &chunk, length) != length) return 0;
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
        if (material == 0) return 0;
        material->color.red = chunk.color.red;
        material->color.green = chunk.color.green;
        material->color.blue = chunk.color.blue;
        material->color.alpha = chunk.color.alpha;
        material->surface = chunk.surface;
        material->texture = 0;
        if (chunk.textured != 0) {
            if (!RwStreamFindChunk(stream, 6, 0, &version)) {
                RpMaterialDestroy(material);
                return 0;
            }
            if (version >= 0x34000 && version <= 0x36003) {
                material->texture = RwTextureStreamRead(stream);
            } else {
                RpMaterialDestroy(material);
                error.pluginID = 2;
                error.errorCode = _rwerror(0x80000004);
                RwErrorSet(&error);
                return 0;
            }
        }
        lastSeenRightsPluginId = 0;
        lastSeenExtraData = 0;
        if (!_rwPluginRegistryReadDataChunks(&materialTKList, stream,
                                             material)) {
            RpMaterialDestroy(material);
            return 0;
        }
        if (lastSeenRightsPluginId != 0) {
            _rwPluginRegistryInvokeRights(&materialTKList,
                                          lastSeenRightsPluginId, material,
                                          lastSeenExtraData);
        }
        return material;
    }
    error.pluginID = 2;
    error.errorCode = _rwerror(0x80000004);
    RwErrorSet(&error);
    return 0;
}
