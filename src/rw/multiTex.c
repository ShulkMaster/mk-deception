#include "libmkparticle/rw_engine.h"
#include "rw/rpmatfx.h"
#include "rw/rwerror.h"
#include "rw/rwfreelist.h"
#include "rw/rwstream_internal.h"

extern void* memset(void*, RwInt32, RwUInt32);
extern RwInt32 RwEngineGetVersion(void);
extern RwInt32 RpMaterialRegisterPlugin(
    RwInt32, RwUInt32, RwPluginObjectConstructor,
    RwPluginObjectDestructor, RwPluginObjectCopy);
extern RwInt32 RpMaterialRegisterPluginStream(
    RwUInt32, RwPluginDataChunkReadCallBack,
    RwPluginDataChunkWriteCallBack, RwPluginDataChunkGetSizeCallBack);
extern RwInt32 _rpMatFXStreamSizeTexture(RwTexture*);
extern RwStream* _rpMatFXStreamWriteTexture(RwStream*, RwTexture*);
extern RwTexture* _rpMatFXStreamReadTexture(RwStream*, RwTexture**);

static void* MultiTextureOpen(void*, RwInt32, RwInt32);
static void* MultiTextureClose(void*, RwInt32, RwInt32);
static RpMultiTexture* MultiTextureCreate(RpMultiTextureRegEntry*, RwUInt32);
static void MultiTextureDestroy(RpMultiTexture*);
static void* MultiTextureConstructor(void*, RwInt32, RwInt32);
static void* MultiTextureDestructor(void*, RwInt32, RwInt32);
static void* MultiTextureCopy(void*, const void*, RwInt32, RwInt32);
static RwInt32 MultiTextureStreamGetSize(const void*, RwInt32, RwInt32);
static RwStream* MultiTextureStreamWrite(RwStream*, RwInt32, const void*,
                                         RwInt32, RwInt32);
static RwStream* MultiTextureStreamRead(RwStream*, RwInt32, void*, RwInt32,
                                        RwInt32);

static RpMultiTextureRegEntry RegEntries[10];
RwModuleInfo _rpMultiTextureModule = { 0, 0 };

static void* MultiTextureOpen(void* instance, RwInt32 offset, RwInt32 size)
{
    _rpMultiTextureModule.numInstances++;
    _rpMTEffectOpen();
    return instance;
}

static void* MultiTextureClose(void* instance, RwInt32 offset, RwInt32 size)
{
    _rpMTEffectClose();
    _rpMultiTextureModule.numInstances--;
    return instance;
}

static RpMultiTexture* MultiTextureCreate(RpMultiTextureRegEntry* registration,
                                          RwUInt32 numTextures)
{

    RwUInt32 size = registration->platformDataSize + 0x38;
    RpMultiTexture* multiTexture =
        RwEngineInstance->fpMalloc(size, 0x0103012C);
    if (!multiTexture) {
        RwError error;
        error.pluginID = 0x120;
        error.errorCode = _rwerror(0x80000013, size);
        RwErrorSet(&error);
        return 0;
    }
    memset(multiTexture, 0, size);
    multiTexture->registration = registration;
    multiTexture->numTextures = numTextures;
    if (registration->platformDataSize)
        multiTexture->platformData = (RwUInt8*)multiTexture + 0x38;
    return multiTexture;
}

static void MultiTextureDestroy(RpMultiTexture* multiTexture)
{
    RwUInt32 i;
    for (i = 0; i < multiTexture->numTextures; i++) {
        if (multiTexture->textures[i]) {
            RwTextureDestroy(multiTexture->textures[i]);
            multiTexture->textures[i] = 0;
        }
    }
    if (multiTexture->effect) {
        RpMTEffectDestroy(multiTexture->effect);
        multiTexture->effect = 0;
    }
    RwEngineInstance->fpFree(multiTexture);
}

static void* MultiTextureConstructor(void* object, RwInt32 offset, RwInt32 size)
{
    RpMultiTexture** slot =
        (RpMultiTexture**)((RwUInt8*)object + offset);
    *slot = 0;
    return object;
}

static void* MultiTextureDestructor(void* object, RwInt32 offset, RwInt32 size)
{
    RpMultiTexture** slot = (RpMultiTexture**)((RwUInt8*)object + offset);
    if (*slot) {
        MultiTextureDestroy(*slot);
        *slot = 0;
    }
    return object;
}

static void* MultiTextureCopy(void* destination, const void* source,
                              RwInt32 offset, RwInt32 size)
{
    RpMultiTexture* sourceMulti =
        *(RpMultiTexture* const*)((const RwUInt8*)source + offset);
    RpMultiTexture* destinationMulti;
    RwUInt32 i;
    if (!sourceMulti)
        return destination;
    destinationMulti = MultiTextureCreate(sourceMulti->registration,
                                          sourceMulti->numTextures);
    if (!destinationMulti)
        return 0;
    *(RpMultiTexture**)((RwUInt8*)destination + offset) = destinationMulti;
    for (i = 0; i < sourceMulti->numTextures; i++) {
        RpMultiTextureSetTexture(destinationMulti, i,
            RpMultiTextureGetTexture(sourceMulti, i));
        RpMultiTextureSetCoords(destinationMulti, i,
            RpMultiTextureGetCoords(sourceMulti, i));
    }
    RpMultiTextureSetEffect(destinationMulti,
                            RpMultiTextureGetEffect(sourceMulti));
    return destination;
}


static RwInt32 MultiTextureStreamGetSize(const void* object, RwInt32 offset,
                                         RwInt32 size)
{
    const RpMultiTexture* multiTexture =
        *(RpMultiTexture* const*)((const RwUInt8*)object + offset);
    RwInt32 streamSize = 0;
    RwUInt32 i;
    if (multiTexture) {
        streamSize += 8;
        streamSize += multiTexture->numTextures;
        for (i = 0; i < multiTexture->numTextures; i++)
            streamSize += _rpMatFXStreamSizeTexture(multiTexture->textures[i]);
        if (multiTexture->effect)
            streamSize += _rwStringStreamGetSize(multiTexture->effect->name) + 12;
    }
    return streamSize;
}


static RwStream* MultiTextureStreamWrite(RwStream* stream, RwInt32 length,
                                         const void* object, RwInt32 offset,
                                         RwInt32 size)
{
    const RpMultiTexture* multiTexture =
        *(RpMultiTexture* const*)((const RwUInt8*)object + offset);
    RwInt32 version;
    RwBool hasEffect;
    struct { RwUInt8 platform, count, hasEffect, pad; } header;
    RwUInt32 i;
    if (multiTexture) {
        version = RwEngineGetVersion();
        if (!RwStreamWriteInt32(stream, &version, 4)) return 0;
        header.platform = (RwUInt8)multiTexture->registration->platform;
        header.count = (RwUInt8)multiTexture->numTextures;
        if (multiTexture->effect)
            hasEffect = 1;
        else
            hasEffect = 0;
        header.hasEffect = hasEffect;
        header.pad = 0;
        if (!RwStreamWrite(stream, &header, 4)) return 0;
        if (multiTexture->numTextures) {
            if (!RwStreamWrite(stream, multiTexture->texCoords,
                               multiTexture->numTextures)) return 0;
            for (i = 0; i < multiTexture->numTextures; i++)
                _rpMatFXStreamWriteTexture(stream, multiTexture->textures[i]);
        }
        if (multiTexture->effect &&
            !_rwStringStreamWrite(multiTexture->effect->name, stream))
            return 0;
    }
    return stream;
}

static RwStream* MultiTextureStreamRead(RwStream* stream, RwInt32 length,
                                        void* object, RwInt32 offset,
                                        RwInt32 size)
{
    RwInt32 version;
    struct { RwUInt8 platform, count, hasEffect, pad; } header;
    RpMultiTextureRegEntry* registration;
    RpMultiTexture* multiTexture;
    RwUInt32 i;
    RwChar name[32];
    RpMTEffect* effect;
    if (!RwStreamReadInt32(stream, &version, 4)) return 0;
    if (!RwStreamRead(stream, &header, 4)) return 0;
    registration = &RegEntries[header.platform];
    multiTexture = MultiTextureCreate(registration, header.count);
    if (!multiTexture) return 0;
    if (header.count) {
        if (!RwStreamRead(stream, multiTexture->texCoords,
                          multiTexture->numTextures)) {
            MultiTextureDestroy(multiTexture); return 0;
        }
        for (i = 0; i < multiTexture->numTextures; i++) {
            if (!_rpMatFXStreamReadTexture(stream, &multiTexture->textures[i])) {
                MultiTextureDestroy(multiTexture); return 0;
            }
        }
    }
    if (header.hasEffect & 1) {
        if (!_rwStringStreamFindAndRead(name, stream)) {
            MultiTextureDestroy(multiTexture); return 0;
        }
        effect = RpMTEffectFind(name);
        if (!effect) {
            effect = RpMTEffectCreateDummy();
            if (!effect) { MultiTextureDestroy(multiTexture); return 0; }
            RpMTEffectSetName(effect, name);
        }
        RpMultiTextureSetEffect(multiTexture, effect);
        RpMTEffectDestroy(effect);
    }
    *(RpMultiTexture**)((RwUInt8*)object + offset) = multiTexture;
    return stream;
}

RwBool _rpMultiTexturePluginAttach(void)
{
    if (!_rpMTEffectSystemInit()) return 0;
    memset(RegEntries, 0, sizeof(RegEntries));
    _rpMultiTextureModule.globalsOffset =
        RwEngineRegisterPlugin(0x18, 0x12C, MultiTextureOpen, MultiTextureClose);
    if (_rpMultiTextureModule.globalsOffset < 0) return 0;
    return 1;
}


RwBool _rpMaterialRegisterMultiTexturePlugin(RwInt32 platform,
                                             RwUInt32 pluginID,
                                             RwUInt32 platformDataSize)
{
    RpMultiTextureRegEntry* entry;
    RwInt32 offset = RpMaterialRegisterPlugin(4, pluginID,
        MultiTextureConstructor, MultiTextureDestructor, MultiTextureCopy);
    if (offset < 0) return 0;
    entry = &RegEntries[platform];
    entry->materialOffset = offset;
    offset = RpMaterialRegisterPluginStream(pluginID, MultiTextureStreamRead,
        MultiTextureStreamWrite, MultiTextureStreamGetSize);
    if (offset < 0) return 0;
    entry->platform = platform;
    entry->pluginID = pluginID;
    entry->platformDataSize = platformDataSize;
    return 1;
}

RpMultiTexture* RpMultiTextureSetEffect(RpMultiTexture* multiTexture,
                                        RpMTEffect* effect)
{
    if (multiTexture->effect) RpMTEffectDestroy(multiTexture->effect);
    multiTexture->effect = effect;
    if (effect) RpMTEffectAddRef(multiTexture->effect);
    return multiTexture;
}

RpMTEffect* RpMultiTextureGetEffect(const RpMultiTexture* multiTexture)
{ return multiTexture->effect; }

RpMultiTexture* RpMultiTextureSetTexture(RpMultiTexture* multiTexture,
                                         RwUInt32 index, RwTexture* texture)
{

    if (multiTexture->textures[index])
        RwTextureDestroy(multiTexture->textures[index]);
    multiTexture->textures[index] = texture;
    if (texture)
        texture->ref_count++;
    return multiTexture;
}

RwTexture* RpMultiTextureGetTexture(const RpMultiTexture* multiTexture,
                                    RwUInt32 index)
{ return multiTexture->textures[index]; }

void RpMultiTextureSetCoords(RpMultiTexture* multiTexture, RwUInt32 index,
                             RwUInt32 coords)
{ multiTexture->texCoords[index] = coords; }

RwUInt32 RpMultiTextureGetCoords(const RpMultiTexture* multiTexture,
                                 RwUInt32 index)
{ return multiTexture->texCoords[index]; }

RpMultiTexture* RpMaterialGetMultiTexture(RpMaterial* material,
                                          RwInt32 platform)
{
    RpMultiTextureRegEntry* entry = &RegEntries[platform];
    if (entry->pluginID) {
        RpMultiTexture** slot =
            (RpMultiTexture**)((RwUInt8*)material + entry->materialOffset);
        return *slot;
    }
    return 0;
}
