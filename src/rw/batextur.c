#include "rw/rwcore_types.h"
#include "rw/batextur.h"
#include "rw/rwerror.h"
#include "rw/palquant.h"
#include "rw/rwplcore.h"
#include "libmkparticle/rw_engine.h"

typedef struct RwFreeListStorage {
    unsigned char storage[0x24];
} RwFreeListStorage;

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
    RwRaster* (*mipmapGenerationCallback)(RwRaster*, RwImage*);
    int (*mipmapNameCallback)(char*, char*, unsigned char, int);
} RwTextureModuleGlobals;

RwPluginRegistry textureTKList = { 0x58, 0x58, 0, 0, 0, 0 };
static RwPluginRegistry texDictTKList = { 0x18, 0x18, 0, 0, 0, 0 };
static RwFreeListStorage _rwTextureFreeList;
static RwFreeListStorage _rwTexDictionaryFreeList;

static RwModuleInfo textureModule;
static RwTexDictionary* dummyTexDict;
extern RwImage* RwImageCreate(int width, int height, int depth);
extern RwImage* RwImageAllocatePixels(RwImage* image);
extern int RwImageDestroy(RwImage* image);
extern RwImage* RwImageCopy(RwImage* destination, const RwImage* source);
extern const char* RwImageFindFileType(const char* name);
extern RwImage* RwImageReadMaskedImage(const char* name, const char* maskName);
extern RwImage* RwImageResample(RwImage* destination, const RwImage* source);
extern RwImage* RwImageCreateResample(const RwImage* source, int width, int height);
extern RwImage* RwImageGammaCorrect(RwImage* image);
extern void* RwFreeListCreateAndPreallocateSpace(
    int entrySize, int entriesPerBlock, int alignment, int preallocBlocks,
    RwFreeListStorage* freeList, int hint);
extern int RwFreeListDestroy(void* freeList);
extern void* memcpy(void* destination, const void* source, unsigned int size);

RwTexture* RwTexDictionaryFindNamedTexture(RwTexDictionary* dictionary,
                                            const char* name);
int RwTextureGenerateMipmapName(char* name, char* maskName, unsigned char level,
                                int format);

static const char character_25[] = "0123456789abcdef";
static char emptyTextureName[] = "";
static char nullMaskName[] = "(null)";
static int _rwTextureFreeListBlockSize = 0x80;
static int _rwTextureFreeListPreallocBlocks = 1;
static int _rwTexDictionaryFreeListBlockSize = 5;
static int _rwTexDictionaryFreeListPreallocBlocks = 1;

static RwTextureModuleGlobals* TextureGlobals(void)
{
    return (RwTextureModuleGlobals*)((char*)RwEngineInstance +
                                     textureModule.globalsOffset);
}

static char CalculateIndexCharacter(unsigned char level) {
    char result = 0;
    int valid = 0;

    if (level != 0 && level < 16) {
        valid = 1;
    }
    result = valid ? character_25[level] : 0;
    return result;
}

static int TextureDefaultMipmapName(char* name, char* maskName, unsigned char level,
                                    int format) {
    char suffix[3];

    suffix[0] = 'm';
    suffix[1] = CalculateIndexCharacter(level);
    suffix[2] = 0;
    if (suffix[1] != 0) {
        RwEngineInstance->stringFuncs.strcat(name, suffix);
        if (maskName != 0 && maskName[0] != 0) {
            RwEngineInstance->stringFuncs.strcat(maskName, suffix);
        }
    }
    return 1;
}

static int PalettizeImage(RwImage** image, int depth) {
    RwRGBA palette[256];
    RwPalQuant quantizer;
    RwImage* palettized;

    if (RwPalQuantInit(&quantizer) == 0) {
        return 0;
    }
    RwPalQuantAddImage(&quantizer, *image, 1.0f);
    RwPalQuantResolvePalette(palette, 1 << depth, &quantizer);
    palettized = RwImageCreate((*image)->width, (*image)->height, depth);
    if (palettized != 0) {
        RwImageAllocatePixels(palettized);
        RwPalQuantMatchImage(palettized->pixels, palettized->stride,
                             palettized->depth, 0, &quantizer, *image);
        memcpy(palettized->palette, palette, (1 << depth) * sizeof(RwRGBA));
        RwImageDestroy(*image);
        *image = palettized;
    } else {
        return 0;
    }
    RwPalQuantTerm(&quantizer);
    return 1;
}


static int PalettizeMipmaps(RwRGBA* palette, RwImage* baseOriginal,
                            RwImage** mipmaps, int mipmapCount, int depth) {
    RwPalQuant quantizer;
    int level;

    if (mipmaps[0]->palette != 0) {
        for (level = 1; level < mipmapCount; level++) {
            RwRGBA* firstPalette = (RwRGBA*)mipmaps[0]->palette;
            RwRGBA* levelPalette = (RwRGBA*)mipmaps[level]->palette;
            int color;

            if (levelPalette == 0) {
                level = 64;
                break;
            }
            for (color = 0; color < (1 << depth); color++) {
                if (((unsigned int*)firstPalette)[color] !=
                    ((unsigned int*)levelPalette)[color]) {
                    level = 64;
                    break;
                }
            }
        }
        if (level == mipmapCount) {
            memcpy(palette, mipmaps[0]->palette,
                   (1 << mipmaps[0]->depth) * sizeof(RwRGBA));
            return 1;
        }
    }

    if (RwPalQuantInit(&quantizer) == 0) {
        return 0;
    }
    for (level = 0; level < mipmapCount; level++) {
        RwPalQuantAddImage(&quantizer, mipmaps[level], 1.0f);
    }
    RwPalQuantResolvePalette(palette, 1 << depth, &quantizer);
    for (level = 0; level < mipmapCount; level++) {
        RwImage* original = mipmaps[level];
        RwImage* palettized =
            RwImageCreate(original->width, original->height, depth);

        if (palettized != 0) {
            RwImageAllocatePixels(palettized);
            RwPalQuantMatchImage(palettized->pixels, palettized->stride,
                                 palettized->depth, 0, &quantizer, original);
            palettized->palette = (unsigned char*)palette;
            mipmaps[level] = palettized;
            if (original != baseOriginal) {
                RwImageDestroy(original);
            }
        } else {
            return 0;
        }
    }
    RwPalQuantTerm(&quantizer);
    return 1;
}

static RwImage* TextureImageReadAndSize(const char* name, const char* maskName,
                                        int rasterFlags, int* width, int* height,
                                        int* depth, int* format) {
    char imageName[256];
    char imageMaskName[256];
    const char* extension;
    const char* maskExtension;
    RwImage* image;

    RwEngineInstance->stringFuncs.strncpy(imageName, name, sizeof(imageName));
    if (RwEngineInstance->stringFuncs.strlen(name) >= sizeof(imageName)) {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x8000001E, name, 256, 255, name[255]);
        RwErrorSet(&error);
        imageName[255] = 0;
    }
    extension = RwImageFindFileType(name);
    if (extension != 0) {
        RwEngineInstance->stringFuncs.strcat(imageName, extension);
    }

    imageMaskName[0] = 0;
    if (maskName != 0 && maskName[0] != 0) {
        RwEngineInstance->stringFuncs.strncpy(imageMaskName, maskName,
                                       sizeof(imageMaskName));
        if (RwEngineInstance->stringFuncs.strlen(maskName) >= sizeof(imageMaskName)) {
            RwError error;
            error.pluginID = 1;
            error.errorCode = _rwerror(0x8000001E, maskName, 256, 255,
                                  maskName[255]);
            RwErrorSet(&error);
            imageMaskName[255] = 0;
        }
        maskExtension = RwImageFindFileType(maskName);
        if (maskExtension != 0) {
            RwEngineInstance->stringFuncs.strcat(imageMaskName, maskExtension);
        }
    }

    image = RwImageReadMaskedImage(imageName, imageMaskName);
    if (image == 0) {
        return 0;
    }
    if ((*width == 0 || *height == 0) &&
        RwImageFindRasterFormat(image, rasterFlags, width, height, depth, format) == 0) {
        RwError error;
        RwImageDestroy(image);
        error.pluginID = 1;
        error.errorCode = _rwerror(0x80000009);
        RwErrorSet(&error);
        return 0;
    }

    if (image->width != *width || image->height != *height) {
        RwImage* resampled;
        int originalDepth = image->depth;
        RwImage* source;

        if (originalDepth != 32) {
            source = image;
            image = RwImageCreate(source->width, source->height, 32);
            if (image == 0) {
                RwImageDestroy(source);
                return 0;
            }
            if (RwImageAllocatePixels(image) == 0) {
                RwImageDestroy(image);
                RwImageDestroy(source);
                return 0;
            }
            RwImageCopy(image, source);
            RwImageDestroy(source);
        }

        resampled = RwImageCreate(*width, *height, 32);
        if (resampled == 0) {
            RwImageDestroy(image);
            return 0;
        }
        if (RwImageAllocatePixels(resampled) == 0) {
            RwImageDestroy(resampled);
            RwImageDestroy(image);
            return 0;
        }
        RwImageResample(resampled, image);
        RwImageDestroy(image);
        image = resampled;
        if (originalDepth == 4) {
            PalettizeImage(&image, originalDepth);
        } else if (originalDepth == 8) {
            PalettizeImage(&image, originalDepth);
        }
    }
    return image;
}


static RwTexture* TextureDefaultNormalRead(const char* name,
                                           const char* maskName) {
    char imageName[256];
    char imageMaskName[256];
    RwRGBA palette[256];
    RwImage* image;
    RwRaster* raster;
    RwTexture* texture;
    int width;
    int height;
    int depth;
    int format;

    RwEngineInstance->stringFuncs.strncpy(imageName, name, sizeof(imageName));
    if (RwEngineInstance->stringFuncs.strlen(name) >= sizeof(imageName)) {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x8000001E, name, 256, 255, name[255]);
        RwErrorSet(&error);
        imageName[255] = 0;
    }
    imageMaskName[0] = 0;
    if (maskName != 0 && maskName[0] != 0) {
        RwEngineInstance->stringFuncs.strncpy(imageMaskName, maskName,
                                       sizeof(imageMaskName));
        if (RwEngineInstance->stringFuncs.strlen(maskName) >= sizeof(imageMaskName)) {
            RwError error;
            error.pluginID = 1;
            error.errorCode = _rwerror(0x8000001E, maskName, 256, 255,
                                  maskName[255]);
            RwErrorSet(&error);
            imageMaskName[255] = 0;
        }
    }

    RwTextureGenerateMipmapName(imageName, imageMaskName, 0, 4);
    width = 0;
    height = 0;
    image = TextureImageReadAndSize(imageName, imageMaskName, 4, &width, &height,
                                    &depth, &format);
    if (image == 0) {
        return 0;
    }
    raster = RwRasterCreate(width, height, depth, format);
    if (raster == 0) {
        RwImageDestroy(image);
        return 0;
    }

    if ((((raster->format & 0xFF) << 8) & 0x6000) != 0) {
        if ((((raster->format & 0xFF) << 8) & 0x4000) != 0) {
            PalettizeMipmaps(palette, 0, &image, 1, 4);
        } else {
            PalettizeMipmaps(palette, 0, &image, 1, 8);
        }
        image->palette = (unsigned char*)palette;
    }
    RwImageGammaCorrect(image);
    if (RwRasterSetFromImage(raster, image) == 0) {
        RwRasterDestroy(raster);
        RwImageDestroy(image);
        return 0;
    }
    RwImageDestroy(image);
    texture = RwTextureCreate(raster);
    if (texture == 0) {
        RwRasterDestroy(raster);
        return 0;
    }
    RwTextureSetName(texture, name);
    if (maskName != 0) {
        RwTextureSetMaskName(texture, maskName);
    } else {
        RwTextureSetMaskName(texture, emptyTextureName);
    }
    return texture;
}

static RwTexture* TextureDefaultMipmapRead(const char* name,
                                           const char* maskName) {
    char imageName[256];
    char imageMaskName[256];
    RwRGBA palette[256];
    RwImage* mipmaps[16];
    RwRaster* raster;
    RwTexture* texture;
    int width;
    int height;
    int depth;
    int format;
    int rasterFlags;
    int level;
    int levelCount;

    RwEngineInstance->stringFuncs.strncpy(imageName, name, sizeof(imageName));
    if (RwEngineInstance->stringFuncs.strlen(name) >= sizeof(imageName)) {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x8000001E, name, 256, 255, name[255]);
        RwErrorSet(&error);
        imageName[255] = 0;
    }
    imageMaskName[0] = 0;
    if (maskName != 0 && maskName[0] != 0) {
        RwEngineInstance->stringFuncs.strncpy(imageMaskName, maskName,
                                       sizeof(imageMaskName));
        if (RwEngineInstance->stringFuncs.strlen(maskName) >= sizeof(imageMaskName)) {
            RwError error;
            error.pluginID = 1;
            error.errorCode = _rwerror(0x8000001E, maskName, 256, 255,
                                  maskName[255]);
            RwErrorSet(&error);
            imageMaskName[255] = 0;
        }
    }

    rasterFlags = 4;
    if (TextureGlobals()->mipmapping != 0) {
        rasterFlags |= 0x8000;
        if (TextureGlobals()->autoMipmapping != 0) {
            rasterFlags |= 0x1000;
        }
    }
    RwTextureGenerateMipmapName(imageName, imageMaskName, 0, rasterFlags);
    width = 0;
    height = 0;
    mipmaps[0] = TextureImageReadAndSize(imageName, imageMaskName, rasterFlags,
                                         &width, &height, &depth, &format);
    if (mipmaps[0] == 0) {
        return 0;
    }
    raster = RwRasterCreate(width, height, depth, format);
    if (raster == 0) {
        RwImageDestroy(mipmaps[0]);
        return 0;
    }

    if ((format & 0x8000) != 0) {
        if ((format & 0x1000) != 0) {
            if (RwRasterSetFromImage(raster, mipmaps[0]) == 0) {
                RwRasterDestroy(raster);
                RwImageDestroy(mipmaps[0]);
                return 0;
            }
            RwImageDestroy(mipmaps[0]);
        } else {
            levelCount = RwRasterGetNumLevels(raster);
            for (level = 1; level < levelCount; level++) {
                RwEngineInstance->stringFuncs.strncpy(imageName, name,
                                               sizeof(imageName));
                if (RwEngineInstance->stringFuncs.strlen(name) >= sizeof(imageName)) {
                    RwError error;
                    error.pluginID = 1;
                    error.errorCode = _rwerror(0x8000001E, name, 256, 255,
                                          name[255]);
                    RwErrorSet(&error);
                    imageName[255] = 0;
                }
                imageMaskName[0] = 0;
                if (maskName != 0 && maskName[0] != 0) {
                    RwEngineInstance->stringFuncs.strncpy(imageMaskName, maskName,
                                                   sizeof(imageMaskName));
                    if (RwEngineInstance->stringFuncs.strlen(maskName) >=
                        sizeof(imageMaskName)) {
                        RwError error;
                        error.pluginID = 1;
                        error.errorCode = _rwerror(0x8000001E, maskName, 256, 255,
                                              maskName[255]);
                        RwErrorSet(&error);
                        imageMaskName[255] = 0;
                    }
                }
                RwTextureGenerateMipmapName(imageName, imageMaskName,
                                            (unsigned char)level, rasterFlags);
                RwRasterLock(raster, (unsigned char)level, 5);
                width = raster->width;
                height = raster->height;
                depth = raster->depth;
                format = ((raster->format & 0xFF) << 8) | raster->type;
                RwRasterUnlock(raster);
                mipmaps[level] = TextureImageReadAndSize(
                    imageName, imageMaskName, rasterFlags, &width, &height,
                    &depth, &format);
                if (mipmaps[level] == 0) {
                    while (--level >= 0) {
                        RwImageDestroy(mipmaps[level]);
                    }
                    RwRasterDestroy(raster);
                    return 0;
                }
            }

            if ((((raster->format & 0xFF) << 8) & 0x6000) != 0) {
                if ((((raster->format & 0xFF) << 8) & 0x4000) != 0) {
                    PalettizeMipmaps(palette, 0, mipmaps, levelCount, 4);
                } else {
                    PalettizeMipmaps(palette, 0, mipmaps, levelCount, 8);
                }
                RwImageGammaCorrect(mipmaps[0]);
            } else {
                for (level = 0; level < levelCount; level++) {
                    RwImageGammaCorrect(mipmaps[level]);
                }
            }

            for (level = 0; level < levelCount; level++) {
                if (RwRasterLock(raster, (unsigned char)level, 5) != 0) {
                    if (RwRasterSetFromImage(raster, mipmaps[level]) == 0) {
                        while (level < levelCount) {
                            RwImageDestroy(mipmaps[level]);
                            level++;
                        }
                        RwRasterDestroy(raster);
                        return 0;
                    }
                    RwRasterUnlock(raster);
                }
                RwImageDestroy(mipmaps[level]);
            }
        }
    } else {
        RwImageGammaCorrect(mipmaps[0]);
        if (RwRasterSetFromImage(raster, mipmaps[0]) == 0) {
            RwRasterDestroy(raster);
            RwImageDestroy(mipmaps[0]);
            return 0;
        }
        RwImageDestroy(mipmaps[0]);
    }

    texture = RwTextureCreate(raster);
    if (texture == 0) {
        RwRasterDestroy(raster);
        return 0;
    }
    RwTextureSetName(texture, name);
    if (maskName != 0) {
        RwTextureSetMaskName(texture, maskName);
    } else {
        RwTextureSetMaskName(texture, emptyTextureName);
    }
    return texture;
}


static RwTexture* TextureDefaultRead(const char* name, const char* maskName) {
    RwTexture* texture;

    if (TextureGlobals()->mipmapping != 0) {
        texture = TextureDefaultMipmapRead(name, maskName);
    } else {
        texture = TextureDefaultNormalRead(name, maskName);
    }
    return texture;
}


static RwRaster* TextureRasterDefaultBuildMipmaps(RwRaster* raster,
                                                   RwImage* baseImage) {
    RwRGBA palette[256];
    RwImage* mipmaps[16];
    int width = raster->width;
    int height = raster->height;
    int depth = raster->depth;
    int formatBit;
    int level;
    int levelCount;

    if (baseImage == 0) {
        mipmaps[0] = RwImageCreate(width, height, 32);
        if (mipmaps[0] != 0) {
            if (RwImageAllocatePixels(mipmaps[0]) == 0) {
                return 0;
            }
            RwImageSetFromRaster(mipmaps[0], raster);
        }
    } else if (baseImage->depth != 32) {
        mipmaps[0] = RwImageCreate(width, height, 32);
        if (mipmaps[0] != 0) {
            if (RwImageAllocatePixels(mipmaps[0]) == 0) {
                return 0;
            }
            RwImageCopy(mipmaps[0], baseImage);
        }
    } else {
        mipmaps[0] = baseImage;
    }
    if (mipmaps[0] == 0) {
        return 0;
    }

    formatBit = raster->format & 0x10;
    raster->format = (unsigned char)(raster->format & ~formatBit);
    levelCount = RwRasterGetNumLevels(raster);
    for (level = 1; level < levelCount; level++) {
        mipmaps[level] = 0;
        if (RwRasterLock(raster, (unsigned char)level, 2) != 0) {
            mipmaps[level] = RwImageCreateResample(
                mipmaps[level - 1], raster->width, raster->height);
            RwRasterUnlock(raster);
        }
        if (mipmaps[level] == 0) {
            while (--level >= 0) {
                if (mipmaps[level] != baseImage) {
                    RwImageDestroy(mipmaps[level]);
                }
            }
            raster->format = (unsigned char)(raster->format | formatBit);
            return 0;
        }
    }

    if ((((raster->format & 0xFF) << 8) & 0x6000) != 0) {
        if ((((raster->format & 0xFF) << 8) & 0x4000) != 0) {
            if (PalettizeMipmaps(palette, baseImage, mipmaps, levelCount, 4) == 0) {
                for (level = 0; level < levelCount; level++) {
                    if (mipmaps[level] != baseImage) {
                        RwImageDestroy(mipmaps[level]);
                    }
                    raster->format = (unsigned char)(raster->format | formatBit);
                    return 0;
                }
            }
        } else {
            if (PalettizeMipmaps(palette, baseImage, mipmaps, levelCount, 8) == 0) {
                for (level = 0; level < levelCount; level++) {
                    if (mipmaps[level] != baseImage) {
                        RwImageDestroy(mipmaps[level]);
                    }
                    raster->format = (unsigned char)(raster->format | formatBit);
                    return 0;
                }
            }
        }
        RwImageGammaCorrect(mipmaps[0]);
    } else {
        for (level = 0; level < levelCount; level++) {
            RwImageGammaCorrect(mipmaps[level]);
        }
    }

    for (level = 0; level < levelCount; level++) {
        if (RwRasterLock(raster, (unsigned char)level, 5) != 0) {
            RwRasterSetFromImage(raster, mipmaps[level]);
            RwRasterUnlock(raster);
        }
        if (mipmaps[level] != baseImage) {
            RwImageDestroy(mipmaps[level]);
        }
    }
    raster->format = (unsigned char)(raster->format | formatBit);
    return raster;
}

static int TextureAnnihilate(RwTexture* texture) {
    RwLLLink* previous;

    texture->ref_count++;
    _rwPluginRegistryDeInitObject(&textureTKList, texture);
    if (texture->dictionary != 0) {
        texture->lInDictionary.prev->next = texture->lInDictionary.next;
        previous = texture->lInDictionary.prev;
        texture->lInDictionary.next->prev = previous;
    }
    if (texture->raster != 0) {
        RwRasterDestroy(texture->raster);
        texture->raster = 0;
    }
    texture->ref_count--;
    RwEngineInstance->fpFreeListFree(TextureGlobals()->textureFreeList, texture);
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
    RwTexDictionary* dictionary = TextureGlobals()->currentDictionary;
    RwLLLink* link;
    RwLLLink* end;
    RwTexture* texture;

    if (dictionary != 0) {
        texture = RwTexDictionaryFindNamedTexture(dictionary, name);
        return texture;
    }

    link = TextureGlobals()->dictionaries.next;
    end = &TextureGlobals()->dictionaries;
    while (link != end) {
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
    TextureGlobals()->findCallback = callback;
    return 1;
}

int RwTextureSetReadCallBack(RwTexture* (*callback)(const char*, const char*)) {
    TextureGlobals()->readCallback = callback;
    return 1;
}

int RwTextureSetMipmapping(int enable) {
    TextureGlobals()->mipmapping = enable;
    return 1;
}

int RwTextureGetMipmapping(void) {
    return TextureGlobals()->mipmapping;
}

int RwTextureSetAutoMipmapping(int enable) {
    TextureGlobals()->autoMipmapping = enable;
    return 1;
}

int RwTextureGetAutoMipmapping(void) {
    return TextureGlobals()->autoMipmapping;
}

RwTexture* RwTextureSetRaster(RwTexture* texture, RwRaster* raster) {
    if (raster != 0) {
        if (RwEngineInstance->fpTextureSetRaster(texture, raster, 0) != 0) {
            return texture;
        }
        return 0;
    }
    texture->raster = 0;
    return texture;
}

RwTexDictionary* RwTexDictionaryForAllTextures(
    RwTexDictionary* dictionary,
    RwTexture* (*callback)(RwTexture*, void*), void* data);

RwTexDictionary* RwTexDictionaryCreate(void) {
    RwTexDictionary* dictionary;

    dictionary = (RwTexDictionary*)RwEngineInstance->fpFreeListAlloc(
        TextureGlobals()->dictionaryFreeList, 0x30016);
    if (dictionary == 0) {
        return 0;
    }
    dictionary->object.type = 6;
    dictionary->object.subType = 0;
    dictionary->object.flags = 0;
    dictionary->object.privateFlags = 0;
    dictionary->object.parent = 0;

    rwLinkListAddLLLink((RwLinkList*)&TextureGlobals()->dictionaries,
                        &dictionary->lInInstance);
    rwLinkListInitialize((RwLinkList*)&dictionary->textures);
    _rwPluginRegistryInitObject(&texDictTKList, dictionary);
    return dictionary;
}

int RwTexDictionaryDestroy(RwTexDictionary* dictionary) {
    RwLLLink* previous;

    if (TextureGlobals()->currentDictionary == dictionary) {
        TextureGlobals()->currentDictionary = 0;
    }
    RwTexDictionaryForAllTextures(
        dictionary, (RwTexture* (*)(RwTexture*, void*))RwTextureDestroy, 0);
    _rwPluginRegistryDeInitObject(&texDictTKList, dictionary);
    dictionary->lInInstance.prev->next = dictionary->lInInstance.next;
    previous = dictionary->lInInstance.prev;
    dictionary->lInInstance.next->prev = previous;
    RwEngineInstance->fpFreeListFree(TextureGlobals()->dictionaryFreeList, dictionary);
    return 1;
}

RwTexDictionary* RwTexDictionaryForAllTextures(
    RwTexDictionary* dictionary,
    RwTexture* (*callback)(RwTexture*, void*), void* data) {
    RwLLLink* next;
    RwLLLink* end = &dictionary->textures;
    RwLLLink* link = dictionary->textures.next;

    while (link != end) {
        RwTexture* texture;

        next = link->next;
        texture = (RwTexture*)((char*)link - 8);
        if (callback(texture, data) == 0) {
            break;
        }
        link = next;
    }
    return dictionary;
}


RwTexture* RwTextureCreate(RwRaster* raster) {
    RwTexture* texture;
    void* freelist;

    freelist = TextureGlobals()->textureFreeList;
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
    RwError error;

    RwEngineInstance->stringFuncs.strncpy(texture->name, name, 32);
    if (RwEngineInstance->stringFuncs.strlen(name) >= 32) {
        error.pluginID = 1;
        error.errorCode = _rwerror(0x8000001E, name, 32, 31, name[31]);
        RwErrorSet(&error);
        texture->name[31] = 0;
    }
    return texture;
}

RwTexture* RwTextureSetMaskName(RwTexture* texture, const char* maskName) {
    RwError error;

    RwEngineInstance->stringFuncs.strncpy(texture->mask, maskName, 32);
    if (RwEngineInstance->stringFuncs.strlen(maskName) >= 32) {
        error.pluginID = 1;
        error.errorCode = _rwerror(0x8000001E, maskName, 32, 31, maskName[31]);
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
        texture->lInDictionary.prev->next = texture->lInDictionary.next;
        previous = texture->lInDictionary.prev;
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
        texture->lInDictionary.prev->next = texture->lInDictionary.next;
        previous = texture->lInDictionary.prev;
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
        if (StringCompare(texture->name, name)) {
            return texture;
        }
        link = link->next;
    }
    return 0;
}

void RwTexDictionarySetCurrent(RwTexDictionary* dictionary) {
    TextureGlobals()->currentDictionary = dictionary;
}

RwTexDictionary* RwTexDictionaryGetCurrent(void) {
    return TextureGlobals()->currentDictionary;
}

int RwTextureGenerateMipmapName(char* name, char* maskName, unsigned char level,
                                int format) {
    if (TextureGlobals()->mipmapNameCallback != 0) {
        return TextureGlobals()->mipmapNameCallback(name, maskName, level, format);
    }
    return 0;
}


RwTexture* RwTextureRead(const char* name, const char* maskName) {
    RwTexture* texture = TextureGlobals()->findCallback(name);

    if (texture != 0) {
        texture->ref_count++;
        return texture;
    }
    texture = TextureGlobals()->readCallback(name, maskName);
    if (texture == 0) {
        if (maskName != 0) {
            RwError error;
            error.pluginID = 1;
            error.errorCode = _rwerror(0x16, name, maskName);
            RwErrorSet(&error);
        } else {
            RwError error;
            error.pluginID = 1;
            error.errorCode = _rwerror(0x16, name, nullMaskName);
            RwErrorSet(&error);
        }
        return 0;
    }
    if (TextureGlobals()->currentDictionary != 0) {
        RwTexDictionaryAddTexture(TextureGlobals()->currentDictionary, texture);
    }
    return texture;
}

int RwTextureRegisterPlugin(int size, unsigned int pluginID,
                            RwPluginObjectConstructor constructCB,
                            RwPluginObjectDestructor destructCB,
                            RwPluginObjectCopy copyCB) {
    int offset;
    offset = _rwPluginRegistryAddPlugin(&textureTKList, size, pluginID,
                                        constructCB, destructCB, copyCB);
    return offset;
}

int RwTextureSetMipmapGenerationCallBack(
    RwRaster* (*callback)(RwRaster*, RwImage*)) {
    TextureGlobals()->mipmapGenerationCallback = callback;
    return 1;
}

int RwTextureSetMipmapNameCallBack(
    int (*callback)(char*, char*, unsigned char, int)) {
    TextureGlobals()->mipmapNameCallback = callback;
    return 1;
}

int RwTextureRasterGenerateMipmaps(RwRaster* raster, RwImage* image) {
    if (TextureGlobals()->mipmapGenerationCallback(raster, image) != 0) {
        return 1;
    }
    return 0;
}


void* _rwTextureClose(void* instance, int offset, int size) {
    RwLLLink* link;
    RwLLLink* end;

    if (TextureGlobals()->workImage != 0) {
        RwEngineInstance->fpFree(TextureGlobals()->workImage);
        TextureGlobals()->workImage = 0;
        TextureGlobals()->state = 0;
    }
    if (TextureGlobals()->textureFreeList != 0 &&
        TextureGlobals()->dictionaryFreeList != 0) {
        link = TextureGlobals()->dictionaries.next;
        end = &TextureGlobals()->dictionaries;
        while (link != end) {
            RwTexDictionary* dictionary =
                (RwTexDictionary*)((char*)link - 0x10);
            RwLLLink* next = link->next;
            if (dictionary == dummyTexDict) {
                RwTexDictionaryDestroy(dummyTexDict);
                dummyTexDict = 0;
                break;
            }
            link = next;
        }
    }
    if (TextureGlobals()->textureFreeList != 0) {
        RwFreeListDestroy(TextureGlobals()->textureFreeList);
        TextureGlobals()->textureFreeList = 0;
    }
    if (TextureGlobals()->dictionaryFreeList != 0) {
        RwFreeListDestroy(TextureGlobals()->dictionaryFreeList);
        TextureGlobals()->dictionaryFreeList = 0;
    }
    textureModule.numInstances--;
    return instance;
}

void* _rwTextureOpen(void* instance, int offset, int size) {
    textureModule.globalsOffset = offset;
    TextureGlobals()->textureFreeList = RwFreeListCreateAndPreallocateSpace(
        textureTKList.sizeOfStruct, _rwTextureFreeListBlockSize, 4,
        _rwTextureFreeListPreallocBlocks, &_rwTextureFreeList, 0x40006);
    if (TextureGlobals()->textureFreeList == 0) {
        return 0;
    }
    TextureGlobals()->dictionaryFreeList = RwFreeListCreateAndPreallocateSpace(
        texDictTKList.sizeOfStruct, _rwTexDictionaryFreeListBlockSize, 4,
        _rwTexDictionaryFreeListPreallocBlocks, &_rwTexDictionaryFreeList,
        0x40408);
    if (TextureGlobals()->dictionaryFreeList == 0) {
        RwFreeListDestroy(TextureGlobals()->textureFreeList);
        TextureGlobals()->textureFreeList = 0;
        return 0;
    }

    TextureGlobals()->dictionaries.next = &TextureGlobals()->dictionaries;
    {
        RwLLLink* sentinel = &TextureGlobals()->dictionaries;
        TextureGlobals()->dictionaries.prev = sentinel;
    }
    textureModule.numInstances++;
    dummyTexDict = RwTexDictionaryCreate();
    TextureGlobals()->currentDictionary = dummyTexDict;
    if (TextureGlobals()->currentDictionary == 0) {
        RwFreeListDestroy(TextureGlobals()->dictionaryFreeList);
        TextureGlobals()->dictionaryFreeList = 0;
        RwFreeListDestroy(TextureGlobals()->textureFreeList);
        TextureGlobals()->textureFreeList = 0;
        return 0;
    }

    TextureGlobals()->mipmapping = 0;
    TextureGlobals()->autoMipmapping = 0;
    RwTextureSetFindCallBack(TextureDefaultFind);
    RwTextureSetReadCallBack(TextureDefaultRead);
    RwTextureSetMipmapGenerationCallBack(TextureRasterDefaultBuildMipmaps);
    RwTextureSetMipmapNameCallBack(TextureDefaultMipmapName);
    TextureGlobals()->workImage = 0;
    TextureGlobals()->state = 0;
    return instance;
}
