#include "rw/rwcore_types.h"
#include "libmkparticle/rw_engine.h"

typedef struct RwErrorPair {
    int plugin;
    int code;
} RwErrorPair;

typedef struct RwTextureModuleGlobals {
    char pad00[8];
    void* freelist; /* module base +0x08 */
} RwTextureModuleGlobals;

extern int textureModule;
extern char textureTKList[];
extern void* _rwPluginRegistryInitObject(void* registry, void* object);
extern int TextureAnnihilate(RwTexture* texture);
extern int _rwerror(unsigned int code, ...);
extern void RwErrorSet(RwErrorPair* error);

RwTexture* RwTextureCreate(RwRaster* raster) {
    RwTexture* volatile texture;
    void* freelist;

    freelist = ((RwTextureModuleGlobals*)((char*)RwEngineInstance + textureModule))->freelist;
    texture = (RwTexture*)
        RwEngineInstance->fpFreeListAlloc(freelist, 0x30006);
    if (texture != 0) {
        texture->dictionary = 0;
        texture->name[0] = 0;
        texture->mask[0] = 0;
        texture->raster = raster;
        texture->ref_count = 1;
        texture->filter_flags = 0;
        texture->filter_flags = (texture->filter_flags & 0xFFFF00FF) | 0x1100;
        texture->filter_flags = (texture->filter_flags & 0xFFFFFF00) | 1;
        _rwPluginRegistryInitObject(textureTKList, texture);
    }
    return texture;
}

int RwTextureDestroy(RwTexture* texture) {
    int result;

    result = 1;
    texture->ref_count--;
    if (texture->ref_count <= 0) {
        result = TextureAnnihilate(texture);
    }
    return result;
}

RwTexture* RwTextureSetName(RwTexture* texture, const char* name) {
    RwErrorPair error;

    RwEngineInstance->fpStringCopy(texture->name, name, 32);
    if (RwEngineInstance->fpStringLength(name) >= 32) {
        error.plugin = 1;
        error.code = _rwerror(0x8000001E, name, 32, 31, name[31]);
        RwErrorSet(&error);
        texture->name[31] = 0;
    }
    return texture;
}

void RwTexDictionaryRemoveTexture(RwTexture* texture) {
    RwLLLink* previous;

    if (texture->dictionary != 0) {
        texture->dictionary = 0;
        previous = texture->prev_link;
        previous->next = texture->next_link;
        texture->next_link->prev = previous;
    }
}
