typedef void* (*RwFreeListAllocCall)(void* freelist, int hint);
typedef void (*RwStringCopyCall)(char* destination, const char* source, unsigned int size);
typedef unsigned int (*RwStringLengthCall)(const char* string);

typedef struct RwTexturePrefix {
    void* raster;
    void* dictionary;
    struct RwTexturePrefix** next_link;
    struct RwTexturePrefix** prev_link;
    char name[32];
    char mask[32];
    unsigned int filter_flags;
    int ref_count;
} RwTexturePrefix;

typedef struct RwErrorPair {
    int plugin;
    int code;
} RwErrorPair;

extern void* RwEngineInstance;
extern int textureModule;
extern char textureTKList[];
extern void* _rwPluginRegistryInitObject(void* registry, void* object);
extern int TextureAnnihilate(void* texture);
extern int _rwerror(void* code, ...);
extern void RwErrorSet(RwErrorPair* error);

void* RwTextureCreate(void* raster) {
    RwTexturePrefix* volatile texture;
    void* freelist;

    freelist = *(void**)((char*)RwEngineInstance + textureModule + 8);
    texture = (RwTexturePrefix*)
        (*(RwFreeListAllocCall*)((char*)RwEngineInstance + 0x144))(freelist, 0x30006);
    if (texture != 0) {
        texture->dictionary = 0;
        texture->name[0] = 0;
        texture->mask[0] = 0;
        texture->raster = raster;
        texture->ref_count = 1;
        texture->filter_flags = 0;
        texture->filter_flags = (texture->filter_flags & 0xFFFF00FF) | 0x1100;
        texture->filter_flags = (texture->filter_flags & 0xFFFFFF00) | 1;
        _rwPluginRegistryInitObject(textureTKList, (void*)texture);
    }
    return (void*)texture;
}

int RwTextureDestroy(RwTexturePrefix* texture) {
    int result;

    result = 1;
    texture->ref_count--;
    if (texture->ref_count <= 0) {
        result = TextureAnnihilate(texture);
    }
    return result;
}

void* RwTextureSetName(RwTexturePrefix* texture, const char* name) {
    RwErrorPair error;

    (*(RwStringCopyCall*)((char*)RwEngineInstance + 0xFC))(texture->name, name, 32);
    if ((*(RwStringLengthCall*)((char*)RwEngineInstance + 0x120))(name) >= 32) {
        error.plugin = 1;
        error.code = _rwerror((void*)0x8000001E, name, 32, 31, name[31]);
        RwErrorSet(&error);
        texture->name[31] = 0;
    }
    return texture;
}

void RwTexDictionaryRemoveTexture(RwTexturePrefix* texture) {
    RwTexturePrefix** previous;

    if (texture->dictionary != 0) {
        texture->dictionary = 0;
        previous = texture->prev_link;
        *previous = (RwTexturePrefix*)texture->next_link;
        texture->next_link[1] = (RwTexturePrefix*)previous;
    }
}
