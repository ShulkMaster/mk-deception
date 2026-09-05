#include "dolphin/gx.h"
#include "rw/alphapass.h"
#include "rw/bamateri.h"
#include "rw/batextur.h"
#include "rw/dltextur.h"
#include "rw/gamecube.h"
#include "rw/gamecube_texture.h"
#include "rw/rpworld_types.h"
#include "rw/rwplcore.h"
#include "rw/rwstream.h"

typedef struct RpMaterialAlphaPass {
    RwTexture* texture;
    RwTexture* dualTexture;
} RpMaterialAlphaPass;

extern void _rwDlRenderStateSetZCompLoc(int beforeTexture);

unsigned int alphaPassPluginOffset = -1;

static int ca_350 = 15;
static int cb_351 = 15;
static int cc_352 = 15;
static int aa_354 = 7;
static int ac_356 = 4;
static int ad_357 = 7;
static int ca_367 = 15;
static int cb_368 = 15;
static int cc_369 = 15;
static int aa_371 = 7;
static int ac_373 = 4;
static int ad_374 = 7;
static int renderMe_397 = 1;
static int renderMe_414 = 1;

static RwModuleInfo AlphaPassModule;
static int noTex_415;
static int noTex_398;
static int ab_372;
static int cd_370;
static int ab_355;
static int cd_353;

static void* AlphaPassOpen(void* instance, int offset, int size) {
    AlphaPassModule.numInstances++;
    return instance;
}

static void* AlphaPassClose(void* instance, int offset, int size) {
    AlphaPassModule.numInstances--;
    return instance;
}

static void* AlphaPassConstructor(void* object, int offset, int size) {
    if ((unsigned int)alphaPassPluginOffset != 0) {
        RpMaterialAlphaPass* plugin = (RpMaterialAlphaPass*)(
            ((unsigned long)object + alphaPassPluginOffset + 15) & ~15UL);
        plugin->texture = 0;
        plugin->dualTexture = 0;
    }
    return object;
}

static void* AlphaPassDestructor(void* object, int offset, int size) {
    return object;
}

static void* AlphaPassCopy(void* destination, const void* source, int offset, int size) {
    RpMaterialSetAlphaPassTexture(destination,
        RpMaterialGetAlphaPassTexture(source));
    return destination;
}

#pragma dont_inline on
static int AlphaPassStreamWriteTexture(RwStream* stream, RwTexture* texture) {
    int size = 0;
    int present = texture != 0;
    int wordSize = 4;

    RwStreamWrite(stream, &present, wordSize);
    size += wordSize;
    if (present != 0) {
        int textureSize = RwTextureStreamGetSize(texture) + 12;
        RwTextureStreamWrite(texture, stream);
        size += textureSize;
    }
    return size;
}

static int AlphaPassStreamReadTexture(RwStream* stream, RwTexture** texture) {
    int present;
    unsigned int chunkSize;
    int size = 0;
    int wordSize = 4;

    RwStreamRead(stream, &present, wordSize);
    size += wordSize;
    if (present != 0) {
        if (RwStreamFindChunk(stream, 6, &chunkSize, 0) != 0) {
            *texture = RwTextureStreamRead(stream);
        }
        size += chunkSize;
        size += 12;
    } else {
        *texture = 0;
    }
    return size;
}

static int AlphaPassStreamSizeTexture(RwTexture* texture) {
    int size = 0;
    size += 4;
    if (texture != 0) {
        int textureSize = RwTextureStreamGetSize(texture) + 12;
        size += textureSize;
    }
    return size;
}
#pragma dont_inline reset

static RwStream* AlphaPassMaterialStreamWrite(RwStream* stream, int length,
                                               const void* object,
                                               int offset,
                                               int sizeOfObject) {
    const RpMaterial* material = object;
    RwTexture* texture;
    (void)length;

    texture = RpMaterialGetAlphaPassTexture(material);
    AlphaPassStreamWriteTexture(stream, texture);
    texture = RpMaterialGetDualAlphaPassTexture(material);
    AlphaPassStreamWriteTexture(stream, texture);
    return stream;
}

static RwStream* AlphaPassMaterialStreamRead(RwStream* stream, int length,
                                              void* object, int offset,
                                              int sizeOfObject) {
    RpMaterial* material = object;
    RpMaterialAlphaPass* plugin = (RpMaterialAlphaPass*)(
        ((unsigned long)material + alphaPassPluginOffset + 15) & ~15UL);
    RwTexture* texture;
    (void)length;

    if (plugin == 0) {
        return 0;
    }
    AlphaPassStreamReadTexture(stream, &texture);
    if (texture != 0) {
        RpMaterialSetAlphaPassTexture(material, texture);
    }
    AlphaPassStreamReadTexture(stream, &texture);
    if (texture != 0) {
        RpMaterialSetDualAlphaPassTexture(material, texture);
    }
    return stream;
}

static int AlphaPassMaterialStreamGetSize(const void* object,
                                               int offset,
                                               int sizeOfObject) {
    const RpMaterial* material = object;
    RwTexture* texture;
    int streamSize = 0;

    texture = RpMaterialGetAlphaPassTexture(material);
    streamSize += AlphaPassStreamSizeTexture(texture);
    texture = RpMaterialGetDualAlphaPassTexture(material);
    streamSize += AlphaPassStreamSizeTexture(texture);
    return streamSize;
}

int RpMatGCAlphaPassAttach(void) {
    int result;
    int pluginSize = 0x17;

    result = RwEngineRegisterPlugin(0, 0xe3, AlphaPassOpen, AlphaPassClose);
    if (result < 0) {
        return 0;
    }
    if (alphaPassPluginOffset == 0xffffffff) {
        alphaPassPluginOffset = RpMaterialRegisterPlugin(
            pluginSize, 0xe3, AlphaPassConstructor, AlphaPassDestructor, AlphaPassCopy);
    }
    if ((unsigned int)alphaPassPluginOffset == 0) {
        return 0;
    }
    result = RpMaterialRegisterPluginStream(
        0xe3, AlphaPassMaterialStreamRead, AlphaPassMaterialStreamWrite,
        AlphaPassMaterialStreamGetSize);
    if (result < 0) {
        return 0;
    }
    return 1;
}

RwTexture* RpMaterialGetAlphaPassTexture(const RpMaterial* material) {
    RpMaterialAlphaPass* plugin = (RpMaterialAlphaPass*)(
        ((unsigned long)material + alphaPassPluginOffset + 15) & ~15UL);
    return plugin->texture;
}

RwTexture* RpMaterialSetAlphaPassTexture(RpMaterial* material, RwTexture* texture) {
    RpMaterialAlphaPass* plugin = (RpMaterialAlphaPass*)(
        ((unsigned long)material + alphaPassPluginOffset + 15) & ~15UL);
    plugin->texture = texture;
    return plugin->texture;
}

RwTexture* RpMaterialGetDualAlphaPassTexture(const RpMaterial* material) {
    RpMaterialAlphaPass* plugin = (RpMaterialAlphaPass*)(
        ((unsigned long)material + alphaPassPluginOffset + 15) & ~15UL);
    return plugin->dualTexture;
}

RwTexture* RpMaterialSetDualAlphaPassTexture(RpMaterial* material, RwTexture* texture) {
    RpMaterialAlphaPass* plugin = (RpMaterialAlphaPass*)(
        ((unsigned long)material + alphaPassPluginOffset + 15) & ~15UL);
    plugin->dualTexture = texture;
    return plugin->dualTexture;
}

#pragma dont_inline on
void _rxGCTevAlphaPassSetup(RxGCTevAlphaPass* state) {
    unsigned char stage;
    if ((state->mode & 8) != 0 && (state->mode & 0x84) != 0 && state->field_0x1C != 0) {
        stage = 2;
    } else {
        stage = 1;
    }
    GXSetNumTevStages((unsigned char)(stage + 1));
    GXSetTevSwapModeTable(3, 0, 3, 2, 1);
    GXSetTevSwapMode(stage, 0, 3);
    GXSetTevOrder(stage, 0, 1, 4);
    GXSetTevColorIn(stage, ca_350, cb_351, cc_352, cd_353);
    GXSetTevColorOp(stage, 0, 0, 0, 1, 0);
    GXSetTevAlphaIn(stage, aa_354, ab_355, ac_356, ad_357);
    GXSetTevAlphaOp(stage, 0, 0, 0, 1, 0);
}

void _rxGCTevAlphaMultiPassSetup(RxGCTevAlphaPass* state) {
    unsigned char stage;
    if ((state->mode & 8) != 0 && (state->mode & 0x84) != 0 && state->field_0x1C != 0) {
        stage = 2;
    } else {
        stage = 1;
    }
    GXSetNumTevStages((unsigned char)(stage + 1));
    GXSetTevSwapModeTable(3, 0, 3, 2, 1);
    GXSetTevSwapMode(stage, 0, 3);
    GXSetTevOrder(stage, 0, 1, 4);
    GXSetTevColorIn(stage, ca_367, cb_368, cc_369, cd_370);
    GXSetTevColorOp(stage, 0, 0, 0, 1, 0);
    GXSetTevAlphaIn(stage, aa_371, ab_372, ac_373, ad_374);
    GXSetTevAlphaOp(stage, 0, 0, 0, 1, 0);
}

void _rxGCTevAlphaPassCleanup(RxGCTevAlphaPass* state) {
    unsigned char stage;
    if ((state->mode & 8) != 0 && (state->mode & 0x84) != 0 && state->field_0x1C != 0) {
        stage = 2;
    } else {
        stage = 1;
    }
    GXSetNumTevStages(stage);
    GXSetTevSwapMode(stage - 1, 0, 0);
    GXSetTevSwapMode(stage, 0, 0);
    _rwDlTextureSet(0, 1);
}

void _rxGCTevAlphaMultiPassCleanup(RxGCTevAlphaPass* state) {
    unsigned char stage;
    if ((state->mode & 8) != 0 && (state->mode & 0x84) != 0 && state->field_0x1C != 0) {
        stage = 2;
    } else {
        stage = 1;
    }
    GXSetNumTevStages(stage);
    GXSetTevSwapMode(stage - 1, 0, 0);
    GXSetTevSwapMode(stage, 0, 0);
    _rwDlTextureSet(0, 1);
}
#pragma dont_inline reset

/* TODO: [breakthrough needed] 86.40385%; retain accessor call; macro expansion regressed this consumer. */
void SetSingleTextureAlphaPassWithAlphaComp(RwTexture* texture, RwTexture* alphaTexture,
                                            RxGCTevAlphaPass* state) {
    RwTexture* baseTexture = texture;
    RwGameCubeRasterExt* extension;
    if (noTex_398 != 0) {
        baseTexture = 0;
    }
    _rwDlTextureSet(baseTexture, 0);
    if (alphaTexture != 0 && renderMe_397 != 0) {
        _rxGCTevAlphaPassSetup(state);
        _rwDlTextureSet(alphaTexture, 1);
    }
    if (baseTexture != 0 && baseTexture->raster != 0) {
        extension = rwRasterPlatformData(baseTexture->raster);
        if ((extension->hasAlpha & 1) != 0 || alphaTexture != 0) {
            _rwDlRenderStateSetZCompLoc(0);
            return;
        }
    }
    _rwDlRenderStateSetZCompLoc(1);
}

/* TODO: [near miss] 91.30769%; retain accessor call; macro expansion regressed this consumer. */
void SetFirstTextureAlphaPassWithAlphaComp(RwTexture* texture, RwTexture* alphaTexture,
                                           RxGCTevAlphaPass* state) {
    RwTexture* baseTexture = texture;
    RwGameCubeRasterExt* extension;
    if (noTex_415 != 0) {
        baseTexture = 0;
    }
    _rwDlTextureSet(baseTexture, 0);
    if (alphaTexture != 0 && renderMe_414 != 0) {
        _rxGCTevAlphaMultiPassSetup(state);
        _rwDlTextureSet(alphaTexture, 1);
    }
    if (baseTexture != 0 && baseTexture->raster != 0) {
        extension = rwRasterPlatformData(baseTexture->raster);
        if ((extension->hasAlpha & 1) != 0 || alphaTexture != 0) {
            _rwDlRenderStateSetZCompLoc(0);
        }
    } else {
        _rwDlRenderStateSetZCompLoc(1);
    }
}
