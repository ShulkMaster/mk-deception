#include "libmkparticle/rw_engine.h"
#include "rw/rpmatfx.h"
#include "rw/rwerror.h"
#include "rw/rwstream_internal.h"

typedef struct RpMTEffectRegEntry {
    int type;
    RpMTEffectDestroyCallBack destroy;
    RpMTEffectStreamReadCallBack streamRead;
    RpMTEffectStreamWriteCallBack streamWrite;
    RpMTEffectStreamGetSizeCallBack streamGetSize;
} RpMTEffectRegEntry;

typedef struct RpMTEffectGlobals {
    RwLinkList dictionaries;
    RpMTEffectDict* currentDictionary;
    int scratchSize;
    char* scratch;
    char* scratchName;
} RpMTEffectGlobals;

extern void* memset(void* destination, int value, unsigned int size);
extern RwModuleInfo _rpMultiTextureModule;

static RpMTEffectGlobals* MultiTextureEffectGlobals(void)
{
    return (RpMTEffectGlobals*)((unsigned char*)RwEngineInstance +
                                _rpMultiTextureModule.globalsOffset);
}

static RpMTEffect* EffectFromLink(RwLLLink* link)
{
    return (RpMTEffect*)((unsigned char*)link - 0x28);
}

static RpMTEffectDict* DictionaryFromLink(RwLLLink* link)
{
    return (RpMTEffectDict*)((unsigned char*)link - 8);
}

static RpMTEffectRegEntry EffectRegEntries[10];
static RpMTEffectDict* DummyDict;

int _rpMTEffectSystemInit(void)
{
    memset(EffectRegEntries, 0, sizeof(EffectRegEntries));
    return 1;
}

int _rpMTEffectRegisterPlatform(
    int type, RpMTEffectStreamReadCallBack read,
    RpMTEffectStreamWriteCallBack write,
    RpMTEffectStreamGetSizeCallBack getSize,
    RpMTEffectDestroyCallBack destroy)
{
    RpMTEffectRegEntry* entry = &EffectRegEntries[type];
    entry->type = type;
    entry->streamRead = read;
    entry->streamWrite = write;
    entry->streamGetSize = getSize;
    entry->destroy = destroy;
    return 1;
}

int _rpMTEffectOpen(void)
{

    char* scratch;
    int allocationSize;
    int scratchSize;

    rwLinkListInitialize(&MultiTextureEffectGlobals()->dictionaries);
    DummyDict = RpMTEffectDictCreate();
    if (!DummyDict)
        return 0;
    MultiTextureEffectGlobals()->currentDictionary = DummyDict;
    scratchSize = 0x100;
    allocationSize = scratchSize * 2 + 0x20;
    scratch = RwEngineInstance->fpMalloc(allocationSize, 0x4012C);
    if (!scratch) {
        RwError error;
        RpMTEffectDictDestroy(MultiTextureEffectGlobals()->currentDictionary);
        error.pluginID = 0x120;
        error.errorCode = _rwerror(0x80000013, allocationSize);
        RwErrorSet(&error);
        return 0;
    }
    memset(scratch, 0, allocationSize);
    MultiTextureEffectGlobals()->scratch = scratch;
    MultiTextureEffectGlobals()->scratchName = scratch + 0x100;
    MultiTextureEffectGlobals()->scratchSize = scratchSize;
    return 1;
}

int _rpMTEffectClose(void)
{

    RwLLLink* link;
    RwLLLink* end;
    if (MultiTextureEffectGlobals()->scratch) {
        RwEngineInstance->fpFree(MultiTextureEffectGlobals()->scratch);
        MultiTextureEffectGlobals()->scratch = 0;
        MultiTextureEffectGlobals()->scratchName = 0;
        MultiTextureEffectGlobals()->scratchSize = 0;
    }
    end = &MultiTextureEffectGlobals()->dictionaries.link;
    link = MultiTextureEffectGlobals()->dictionaries.link.next;
    while (link != end) {
        if (DictionaryFromLink(link) == DummyDict) {
            RpMTEffectDictDestroy(DummyDict);
            DummyDict = 0;
            break;
        } else {
            link = link->next;
        }
    }
    return 1;
}

RpMTEffect* _rpMTEffectInit(RpMTEffect* effect, int type)
{

    memset(effect, 0, sizeof(*effect));
    effect->type = type;
    effect->refCount = 1;
    effect->dictLink.prev = 0;
    effect->dictLink.next = 0;
    if (type && MultiTextureEffectGlobals()->currentDictionary)
        RpMTEffectDictAddEffect(MultiTextureEffectGlobals()->currentDictionary, effect);
    return effect;
}

RpMTEffectDict* RpMTEffectDictCreate(void)
{

    unsigned int size = sizeof(RpMTEffectDict);
    RpMTEffectDict* dictionary =
        RwEngineInstance->fpMalloc(size, 0x3012C);
    if (!dictionary) {
        RwError error;
        error.pluginID = 0x120;
        error.errorCode = _rwerror(0x80000013, size);
        RwErrorSet(&error);
        return 0;
    }
    rwLinkListInitialize(&dictionary->effects);
    rwLinkListAddLLLink(&MultiTextureEffectGlobals()->dictionaries,
                        &dictionary->globalLink);
    return dictionary;
}

void RpMTEffectDictDestroy(RpMTEffectDict* dictionary)
{

    RwLLLink* end;
    RwLLLink* link;
    if (dictionary == MultiTextureEffectGlobals()->currentDictionary)
        MultiTextureEffectGlobals()->currentDictionary = 0;
    end = &dictionary->effects.link;
    link = end->next;
    while (link != end) {
        RpMTEffect* effect = EffectFromLink(link);
        link = link->next;
        RpMTEffectDictRemoveEffect(effect);
    }
    rwLinkListRemoveLLLink(&dictionary->globalLink);
    RwEngineInstance->fpFree(dictionary);
}

RpMTEffectDict* RpMTEffectDictAddEffect(RpMTEffectDict* dictionary,
                                        RpMTEffect* effect)
{

    if (effect->dictLink.next) {
        rwLinkListRemoveLLLink(&effect->dictLink);
        RpMTEffectDestroy(effect);
    }
    rwLinkListAddLLLink(&dictionary->effects, &effect->dictLink);
    RpMTEffectAddRef(effect);
    return dictionary;
}

RpMTEffect* RpMTEffectDictRemoveEffect(RpMTEffect* effect)
{
    if (effect->dictLink.next) {
        rwLinkListRemoveLLLink(&effect->dictLink);
        RpMTEffectDestroy(effect);
    }
    return effect;
}

RpMTEffect* RpMTEffectDictFindNamedEffect(RpMTEffectDict* dictionary,
                                          const char* name)
{

    RwLLLink* link = dictionary->effects.link.next;
    while (link != &dictionary->effects.link) {
        RpMTEffect* effect = EffectFromLink(link);
        if (!RwEngineInstance->stringFuncs.strcmp(effect->name, name))
            return effect;
        link = link->next;
    }
    return 0;
}

RpMTEffect* RpMTEffectCreateDummy(void)
{
    unsigned int size = sizeof(RpMTEffect);
    RpMTEffect* effect = RwEngineInstance->fpMalloc(size, 0x3012C);
    if (!effect) {
        RwError error;
        error.pluginID = 0x120;
        error.errorCode = _rwerror(0x80000013, size);
        RwErrorSet(&error);
        return 0;
    }
    _rpMTEffectInit(effect, 0);
    return effect;
}

void RpMTEffectDestroy(RpMTEffect* effect)
{

    effect->refCount--;
    if (!effect->refCount) {
        RpMTEffectDictRemoveEffect(effect);
        if (effect->type) {
            RpMTEffectDestroyCallBack destroy =
                EffectRegEntries[effect->type].destroy;
            if (destroy) {
                destroy(effect);
                return;
            }
        }
        RwEngineInstance->fpFree(effect);
    }
}

RpMTEffect* RpMTEffectStreamRead(RwStream* stream)
{

    int type;
    unsigned int version;
    unsigned int length;
    char name[32];
    RpMTEffect* effect;
    RpMTEffectRegEntry* entry;
    if (!RwStreamFindChunk(stream, 1, 0, 0) ||
        !RwStreamRead(stream, &type, sizeof(type)))
        return 0;
    RwMemNative32(&type, sizeof(type));
    entry = &EffectRegEntries[type];
    if (!entry->streamRead)
        return 0;
    if (!_rwStringStreamFindAndRead(name, stream))
        return 0;
    if (!RwStreamFindChunk(stream, 3, &length, &version))
        return 0;
    effect = entry->streamRead(stream, type, version, length);
    if (!effect)
        return 0;
    RpMTEffectSetName(effect, name);
    return effect;
}

RpMTEffect* RpMTEffectFind(const char* name)
{

    RpMTEffect* effect = 0;
    char* path;
    RwStream* stream;
    if (MultiTextureEffectGlobals()->currentDictionary) {
        effect = RpMTEffectDictFindNamedEffect(
            MultiTextureEffectGlobals()->currentDictionary, name);
    } else {
        RwLLLink* link = MultiTextureEffectGlobals()->dictionaries.link.next;
        while (link != &MultiTextureEffectGlobals()->dictionaries.link) {
            effect = RpMTEffectDictFindNamedEffect(DictionaryFromLink(link), name);
            if (effect)
                break;
            link = link->next;
        }
    }
    if (effect) {
        RpMTEffectAddRef(effect);
        return effect;
    }
    path = MultiTextureEffectGlobals()->scratchName;
    RwEngineInstance->stringFuncs.strcpy(path, MultiTextureEffectGlobals()->scratch);
    RwEngineInstance->stringFuncs.strncat(path, name, 0x1F);
    stream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMREAD, path);
    if (!stream)
        return 0;
    if (!RwStreamFindChunk(stream, 0x20, 0, 0)) {
        RwStreamClose(stream, 0);
        return 0;
    }
    effect = RpMTEffectStreamRead(stream);
    RwStreamClose(stream, 0);
    return effect;
}

RpMTEffect* RpMTEffectSetName(RpMTEffect* effect, const char* name)
{
    RwEngineInstance->stringFuncs.strncpy(effect->name, name, 0x1F);
    return effect;
}

void RpMTEffectAddRef(RpMTEffect* effect)
{
    effect->refCount++;
}
