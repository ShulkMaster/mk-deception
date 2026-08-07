#include "rw/rwcore_types.h"
#include "rw/rwplcore.h"
#include "libmkparticle/rw_engine.h"

typedef struct RwErrorPair {
    int plugin;
    int code;
} RwErrorPair;

typedef struct RwTextureModuleGlobals {
    RwLLLink dictionaries;
    void* textureFreeList;
    void* dictionaryFreeList;
    RwTexDictionary* currentDictionary;
    RwTexture* (*readCallback)(const char*, const char*);
    RwTexture* (*findCallback)(const char*);
    int mipmapping;
    int autoMipmapping;
    void* workImage;
    unsigned short state;
    unsigned short pad2A;
    int (*mipmapGenerationCallback)(RwRaster*, void*);
    int (*mipmapNameCallback)(char*, char*, unsigned char, int);
} RwTextureModuleGlobals;

typedef struct RwModuleInfoLocal {
    int globalsOffset;
    int numInstances;
} RwModuleInfoLocal;

extern RwModuleInfoLocal textureModule;
extern RwPluginRegistry textureTKList;
extern void* _rwPluginRegistryInitObject(void* registry, void* object);
extern int TextureAnnihilate(RwTexture* texture);
extern int _rwerror(unsigned int code, ...);
extern void RwErrorSet(RwErrorPair* error);

RwTexture* RwTexDictionaryFindNamedTexture(RwTexDictionary* dictionary,
                                            const char* name);

static const char character_25[] = "0123456789abcdef";

#define TEXTURE_GLOBALS \
    ((RwTextureModuleGlobals*)((char*)RwEngineInstance + textureModule.globalsOffset))

static char CalculateIndexCharacter(unsigned char level) {
    char result = 0;
    int valid = 0;

    if (level != 0 && level < 16) {
        valid = 1;
    }
    result = valid ? character_25[level] : 0;
    return result;
}

static int TextureDefaultMipmapName(char* name, char* maskName, unsigned char level) {
    char suffix[3];

    suffix[0] = 'm';
    suffix[1] = CalculateIndexCharacter(level);
    suffix[2] = 0;
    if (suffix[1] != 0) {
        RwEngineInstance->fpStringConcat(name, suffix);
        if (maskName != 0 && maskName[0] != 0) {
            RwEngineInstance->fpStringConcat(maskName, suffix);
        }
    }
    return 1;
}

static int StringCompare(const char* left, const char* right) {
    while (*left != 0 && *right != 0) {
        char leftCharacter = *left;
        char rightCharacter = *right;

        if (leftCharacter >= 'a' && leftCharacter <= 'z') {
            leftCharacter -= 'a' - 'A';
        }
        if (rightCharacter >= 'a' && rightCharacter <= 'z') {
            rightCharacter -= 'a' - 'A';
        }
        if (leftCharacter != rightCharacter) {
            return 0;
        }
        left++;
        right++;
    }
    if (*left == *right) {
        return 1;
    }
    return 0;
}

static RwTexture* TextureDefaultFind(const char* name) {
    RwTexDictionary* dictionary = TEXTURE_GLOBALS->currentDictionary;
    RwLLLink* link;
    RwLLLink* end;

    if (dictionary != 0) {
        return RwTexDictionaryFindNamedTexture(dictionary, name);
    }

    link = TEXTURE_GLOBALS->dictionaries.next;
    end = &TEXTURE_GLOBALS->dictionaries;
    while (link != end) {
        RwTexture* texture;
        dictionary = (RwTexDictionary*)((char*)link - 0x10);
        texture = RwTexDictionaryFindNamedTexture(dictionary, name);
        if (texture != 0) {
            return texture;
        }
        link = link->next;
    }
    return 0;
}

int RwTextureSetFindCallBack(RwTexture* (*callback)(const char*)) {
    TEXTURE_GLOBALS->findCallback = callback;
    return 1;
}

int RwTextureSetReadCallBack(RwTexture* (*callback)(const char*, const char*)) {
    TEXTURE_GLOBALS->readCallback = callback;
    return 1;
}

int RwTextureSetMipmapping(int enable) {
    TEXTURE_GLOBALS->mipmapping = enable;
    return 1;
}

int RwTextureGetMipmapping(void) {
    return TEXTURE_GLOBALS->mipmapping;
}

int RwTextureSetAutoMipmapping(int enable) {
    TEXTURE_GLOBALS->autoMipmapping = enable;
    return 1;
}

int RwTextureGetAutoMipmapping(void) {
    return TEXTURE_GLOBALS->autoMipmapping;
}

RwTexture* RwTextureCreate(RwRaster* raster) {
    RwTexture* texture;
    void* freelist;

    freelist = TEXTURE_GLOBALS->textureFreeList;
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
        _rwPluginRegistryInitObject(&textureTKList, texture);
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

RwTexture* RwTextureSetMaskName(RwTexture* texture, const char* maskName) {
    RwErrorPair error;

    RwEngineInstance->fpStringCopy(texture->mask, maskName, 32);
    if (RwEngineInstance->fpStringLength(maskName) >= 32) {
        error.plugin = 1;
        error.code = _rwerror(0x8000001E, maskName, 32, 31, maskName[31]);
        RwErrorSet(&error);
        texture->mask[31] = 0;
    }
    return texture;
}

RwTexture* RwTexDictionaryAddTexture(RwTexDictionary* dictionary,
                                     RwTexture* texture) {
    RwLLLink* previous;
    RwLLLink* link;

    if (texture->dictionary != 0) {
        previous = texture->lInDictionary.prev;
        previous->next = texture->lInDictionary.next;
        texture->lInDictionary.next->prev = previous;
    }
    texture->dictionary = dictionary;
    texture->lInDictionary.next = dictionary->textures.next;
    texture->lInDictionary.prev = &dictionary->textures;
    dictionary->textures.next->prev = &texture->lInDictionary;
    link = &texture->lInDictionary;
    dictionary->textures.next = link;
    return texture;
}

RwTexture* RwTexDictionaryRemoveTexture(RwTexture* texture) {
    RwLLLink* previous;

    if (texture->dictionary != 0) {
        texture->dictionary = 0;
        previous = texture->lInDictionary.prev;
        previous->next = texture->lInDictionary.next;
        texture->lInDictionary.next->prev = previous;
    }
    return texture;
}

RwTexture* RwTexDictionaryFindNamedTexture(RwTexDictionary* dictionary,
                                            const char* name) {
    RwLLLink* link = dictionary->textures.next;
    RwLLLink* end = &dictionary->textures;

    while (link != end) {
        RwTexture* texture = (RwTexture*)((char*)link - 8);
        if (texture->name != 0 && StringCompare(texture->name, name)) {
            return texture;
        }
        link = link->next;
    }
    return 0;
}

void RwTexDictionarySetCurrent(RwTexDictionary* dictionary) {
    TEXTURE_GLOBALS->currentDictionary = dictionary;
}

RwTexDictionary* RwTexDictionaryGetCurrent(void) {
    return TEXTURE_GLOBALS->currentDictionary;
}

int RwTextureRegisterPlugin(int size, unsigned int pluginID,
                            RwPluginObjectConstructor constructCB,
                            RwPluginObjectDestructor destructCB,
                            RwPluginObjectCopy copyCB) {
    return _rwPluginRegistryAddPlugin(&textureTKList, size, pluginID, constructCB,
                                      destructCB, copyCB);
}

int RwTextureSetMipmapGenerationCallBack(int (*callback)(RwRaster*, void*)) {
    TEXTURE_GLOBALS->mipmapGenerationCallback = callback;
    return 1;
}

int RwTextureSetMipmapNameCallBack(
    int (*callback)(char*, char*, unsigned char, int)) {
    TEXTURE_GLOBALS->mipmapNameCallback = callback;
    return 1;
}

int RwTextureRasterGenerateMipmaps(RwRaster* raster, void* image) {
    return TEXTURE_GLOBALS->mipmapGenerationCallback(raster, image) != 0;
}
