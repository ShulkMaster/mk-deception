#include "libmkparticle/rw_engine.h"
#include "runtime/cstring.h"
#include "rw/bamateri.h"
#include "rw/batextur.h"
#include "rw/rpworld_types.h"
#include "rw/rwerror.h"
#include "rw/rwfreelist.h"
#include "rw/rxpipeline.h"
#include "rw/rwstream.h"

typedef struct RpMaterialChunkInfo {
    int flags;
    RpMaterialColor color;
    int unused;
    int textured;
    RpSurfaceProperties surface;
} RpMaterialChunkInfo;

static RwPluginRegistry materialTKList = {0x1C, 0x1C, 0, 0, 0, 0};
static RpSurfaceProperties defaultSurfaceProperties = {1.0f, 1.0f, 1.0f};
static RwFreeList _rpMaterialFreeList;
static int _rpMaterialFreeListBlockSize = 0x100;
static int _rpMaterialFreeListPreallocBlocks = 1;
static int lastSeenExtraData;
static unsigned int lastSeenRightsPluginId;
static RwModuleInfo materialModule;

RwStream* _rpReadMaterialRights(RwStream* stream, int length)
{
    if (!RwStreamReadInt32(stream, (int*)&lastSeenRightsPluginId, 4)) return 0;
    if (length == 8 && !RwStreamReadInt32(stream, &lastSeenExtraData, 4)) return 0;
    return stream;
}

RwStream* _rpWriteMaterialRights(RwStream* stream, int,
                                  const RpMaterial* object)
{
    const RpMaterial* material = object;
    if (!RwStreamWriteInt32(stream,
                            (const int*)&material->pipeline->pluginId, 4)) return 0;
    if (!RwStreamWriteInt32(stream,
                            (const int*)&material->pipeline->pluginData, 4)) return 0;
    return stream;
}

int _rpSizeMaterialRights(const RpMaterial* object)
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

void* _rpMaterialOpen(void* instance, int offset, int)
{
    materialModule.globalsOffset = offset;
    *(RwFreeList**)((unsigned char*)RwEngineInstance +
                    materialModule.globalsOffset) =
        RwFreeListCreateAndPreallocateSpace(
            materialTKList.sizeOfStruct, _rpMaterialFreeListBlockSize, 4,
            _rpMaterialFreeListPreallocBlocks, &_rpMaterialFreeList, 0x40007);
    if (*(RwFreeList**)((unsigned char*)RwEngineInstance +
                        materialModule.globalsOffset) == 0) return 0;
    materialModule.numInstances++;
    return instance;
}

void* _rpMaterialClose(void* instance, int, int)
{
    if (*(RwFreeList**)((unsigned char*)RwEngineInstance +
                        materialModule.globalsOffset) != 0) {
        RwFreeListDestroy(
            *(RwFreeList**)((unsigned char*)RwEngineInstance +
                            materialModule.globalsOffset));
        *(RwFreeList**)((unsigned char*)RwEngineInstance +
                        materialModule.globalsOffset) = 0;
    }
    materialModule.numInstances--;
    return instance;
}



RpMaterial* RpMaterialCreate(void)
{
    RwFreeList* freeList = *(RwFreeList**)((unsigned char*)RwEngineInstance +
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

int RpMaterialDestroy(RpMaterial* material)
{
    if (material->refCount == 1) {
        _rwPluginRegistryDeInitObject(&materialTKList, material);
        RpMaterialSetTexture(material, 0);
        RwEngineInstance->fpFreeListFree(
            *(RwFreeList**)((unsigned char*)RwEngineInstance +
                            materialModule.globalsOffset),
            material);
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

int RpMaterialRegisterPlugin(int size, unsigned int pluginID,
                                 RwPluginObjectConstructor constructCB,
                                 RwPluginObjectDestructor destructCB,
                                 RwPluginObjectCopy copyCB)
{
    int offset = _rwPluginRegistryAddPlugin(
        &materialTKList, size, pluginID, constructCB, destructCB, copyCB);
    return offset;
}

int RpMaterialRegisterPluginStream(unsigned int pluginID,
                                       RwPluginDataChunkReadCallBack readCB,
                                       RwPluginDataChunkWriteCallBack writeCB,
                                       RwPluginDataChunkGetSizeCallBack getSizeCB)
{
    int offset = _rwPluginRegistryAddPluginStream(
        &materialTKList, pluginID, readCB, writeCB, getSizeCB);
    return offset;
}




RpMaterial* RpMaterialStreamRead(RwStream* stream)
{
    unsigned int length;
    unsigned int version;
    RwError error;

    if (!RwStreamFindChunk(stream, 1, &length, &version)) return 0;
    if (version >= 0x34000 && version <= 0x36003) {
        RpMaterialChunkInfo chunk;
        RpMaterial* material;
        unsigned char color[4];

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
