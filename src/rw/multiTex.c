#include "rw/rwengine.h"
#include "runtime/cstring.h"
#include "rw/bamateri.h"
#include "rw/rpmatfx.h"
#include "rw/rwerror.h"
#include "rw/rwfreelist.h"
#include "rw/rwstream_internal.h"

extern int RwEngineGetVersion(void);
extern int _rpMatFXStreamSizeTexture(RwTexture*);
extern RwStream* _rpMatFXStreamWriteTexture(RwStream*, RwTexture*);
extern RwTexture* _rpMatFXStreamReadTexture(RwStream*, RwTexture**);

static void* MultiTextureOpen(void*, int, int);
static void* MultiTextureClose(void*, int, int);
static RpMultiTexture* MultiTextureCreate(RpMultiTextureRegEntry*, unsigned int);
static void MultiTextureDestroy(RpMultiTexture*);
static void* MultiTextureConstructor(void*, int, int);
static void* MultiTextureDestructor(void*, int, int);
static void* MultiTextureCopy(void*, const void*, int, int);
static int MultiTextureStreamGetSize(const void*, int, int);
static RwStream* MultiTextureStreamWrite(RwStream*, int, const void*,
                                         int, int);
static RwStream* MultiTextureStreamRead(RwStream*, int, void*, int,
                                        int);

static RpMultiTextureRegEntry RegEntries[10];
RwModuleInfo _rpMultiTextureModule = { 0, 0 };

static void* MultiTextureOpen(void* instance, int offset, int size)
{
    _rpMultiTextureModule.numInstances++;
    _rpMTEffectOpen();
    return instance;
}

static void* MultiTextureClose(void* instance, int offset, int size)
{
    _rpMTEffectClose();
    _rpMultiTextureModule.numInstances--;
    return instance;
}

static RpMultiTexture* MultiTextureCreate(RpMultiTextureRegEntry* registration,
                                          unsigned int numTextures)
{

    unsigned int size = registration->platformDataSize + 0x38;
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
        multiTexture->platformData = (unsigned char*)multiTexture + 0x38;
    return multiTexture;
}

static void MultiTextureDestroy(RpMultiTexture* multiTexture)
{
    unsigned int i;
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

static void* MultiTextureConstructor(void* object, int offset, int size)
{
    RpMultiTexture** slot =
        (RpMultiTexture**)((unsigned char*)object + offset);
    *slot = 0;
    return object;
}

static void* MultiTextureDestructor(void* object, int offset, int size)
{
    RpMultiTexture** slot = (RpMultiTexture**)((unsigned char*)object + offset);
    if (*slot) {
        MultiTextureDestroy(*slot);
        *slot = 0;
    }
    return object;
}

static void* MultiTextureCopy(void* destination, const void* source,
                              int offset, int size)
{
    RpMultiTexture* sourceMulti =
        *(RpMultiTexture* const*)((const unsigned char*)source + offset);
    RpMultiTexture* destinationMulti;
    unsigned int i;
    if (!sourceMulti)
        return destination;
    destinationMulti = MultiTextureCreate(sourceMulti->registration,
                                          sourceMulti->numTextures);
    if (!destinationMulti)
        return 0;
    *(RpMultiTexture**)((unsigned char*)destination + offset) = destinationMulti;
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


static int MultiTextureStreamGetSize(const void* object, int offset,
                                         int size)
{
    const RpMultiTexture* multiTexture =
        *(RpMultiTexture* const*)((const unsigned char*)object + offset);
    int streamSize = 0;
    unsigned int i;
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


static RwStream* MultiTextureStreamWrite(RwStream* stream, int length,
                                         const void* object, int offset,
                                         int size)
{
    const RpMultiTexture* multiTexture =
        *(RpMultiTexture* const*)((const unsigned char*)object + offset);
    int version;
    int hasEffect;
    struct { unsigned char platform, count, hasEffect, pad; } header;
    unsigned int i;
    if (multiTexture) {
        version = RwEngineGetVersion();
        if (!RwStreamWriteInt32(stream, &version, 4)) return 0;
        header.platform = (unsigned char)multiTexture->registration->platform;
        header.count = (unsigned char)multiTexture->numTextures;
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

static RwStream* MultiTextureStreamRead(RwStream* stream, int length,
                                        void* object, int offset,
                                        int size)
{
    int version;
    struct { unsigned char platform, count, hasEffect, pad; } header;
    RpMultiTextureRegEntry* registration;
    RpMultiTexture* multiTexture;
    unsigned int i;
    char name[32];
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
    *(RpMultiTexture**)((unsigned char*)object + offset) = multiTexture;
    return stream;
}

int _rpMultiTexturePluginAttach(void)
{
    if (!_rpMTEffectSystemInit()) return 0;
    memset(RegEntries, 0, sizeof(RegEntries));
    _rpMultiTextureModule.globalsOffset =
        RwEngineRegisterPlugin(0x18, 0x12C, MultiTextureOpen, MultiTextureClose);
    if (_rpMultiTextureModule.globalsOffset < 0) return 0;
    return 1;
}


int _rpMaterialRegisterMultiTexturePlugin(int platform,
                                             unsigned int pluginID,
                                             unsigned int platformDataSize)
{
    RpMultiTextureRegEntry* entry;
    int offset = RpMaterialRegisterPlugin(4, pluginID,
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
                                         unsigned int index, RwTexture* texture)
{

    if (multiTexture->textures[index])
        RwTextureDestroy(multiTexture->textures[index]);
    multiTexture->textures[index] = texture;
    if (texture)
        texture->ref_count++;
    return multiTexture;
}

RwTexture* RpMultiTextureGetTexture(const RpMultiTexture* multiTexture,
                                    unsigned int index)
{ return multiTexture->textures[index]; }

void RpMultiTextureSetCoords(RpMultiTexture* multiTexture, unsigned int index,
                             unsigned int coords)
{ multiTexture->texCoords[index] = coords; }

unsigned int RpMultiTextureGetCoords(const RpMultiTexture* multiTexture,
                                 unsigned int index)
{ return multiTexture->texCoords[index]; }

RpMultiTexture* RpMaterialGetMultiTexture(RpMaterial* material,
                                          int platform)
{
    RpMultiTextureRegEntry* entry = &RegEntries[platform];
    if (entry->pluginID) {
        RpMultiTexture** slot =
            (RpMultiTexture**)((unsigned char*)material + entry->materialOffset);
        return *slot;
    }
    return 0;
}
