#include "rw/rwengine.h"
#include "runtime/cstring.h"
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

extern RwModuleInfo _rpMultiTextureModule;

static RpMTEffectGlobals* MultiTextureEffectGlobals(void)
{
    return (RpMTEffectGlobals*)((unsigned char*)RwEngineInstance +
                                _rpMultiTextureModule.globalsOffset);
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

    RpMTEffectGlobals* globals;
    char* scratch;
    int allocationSize;
    int scratchSize;

    globals = (RpMTEffectGlobals*)((unsigned char*)RwEngineInstance +
                                   _rpMultiTextureModule.globalsOffset);
    globals->dictionaries.link.next = &globals->dictionaries.link;
    globals->dictionaries.link.prev = &globals->dictionaries.link;
    DummyDict = RpMTEffectDictCreate();
    if (!DummyDict)
        return 0;
    globals->currentDictionary = DummyDict;
    scratchSize = 0x100;
    allocationSize = scratchSize * 2 + 0x20;
    scratch = RwEngineInstance->fpMalloc(allocationSize, 0x4012C);
    if (!scratch) {
        RwError error;
        RpMTEffectDictDestroy(globals->currentDictionary);
        error.pluginID = 0x120;
        error.errorCode = _rwerror(0x80000013, allocationSize);
        RwErrorSet(&error);
        return 0;
    }
    memset(scratch, 0, allocationSize);
    globals->scratch = scratch;
    globals->scratchName = scratch + 0x100;
    globals->scratchSize = scratchSize;
    return 1;
}

int _rpMTEffectClose(void)
{

    RwLinkList* dictionaries;
    RwLLLink* link;
    RwLLLink* end;
    {
        RpMTEffectGlobals* globals = (RpMTEffectGlobals*)(
            (unsigned char*)RwEngineInstance +
            _rpMultiTextureModule.globalsOffset);

        if (globals->scratch) {
            RwEngineInstance->fpFree(globals->scratch);
            globals->scratch = 0;
            globals->scratchName = 0;
            globals->scratchSize = 0;
        }
    }
    dictionaries = (RwLinkList*)((unsigned char*)RwEngineInstance +
                                 _rpMultiTextureModule.globalsOffset);
    end = &dictionaries->link;
    link = dictionaries->link.next;
    while (link != end) {
        RpMTEffectDict* dictionary = (RpMTEffectDict*)(
            (unsigned char*)link - 8);

        if (dictionary == DummyDict) {
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
    RpMTEffectGlobals* globals;
    RwLLLink* dictionaryLink;
    RwLLLink* listHead;
    RpMTEffectDict* dictionary =
        RwEngineInstance->fpMalloc(size, 0x3012C);
    if (!dictionary) {
        RwError error;
        error.pluginID = 0x120;
        error.errorCode = _rwerror(0x80000013, size);
        RwErrorSet(&error);
        return 0;
    }
    dictionary->effects.link.next = &dictionary->effects.link;
    dictionary->effects.link.prev = &dictionary->effects.link;
    globals = (RpMTEffectGlobals*)((unsigned char*)RwEngineInstance +
                                   _rpMultiTextureModule.globalsOffset);
    dictionaryLink = &dictionary->globalLink;
    listHead = &globals->dictionaries.link;
    dictionaryLink->next = listHead->next;
    dictionaryLink->prev = listHead;
    listHead->next->prev = dictionaryLink;
    listHead->next = dictionaryLink;
    return dictionary;
}

void RpMTEffectDictDestroy(RpMTEffectDict* dictionary)
{

    RwLLLink* end;
    RwLLLink* link;
    RpMTEffectGlobals* globals = (RpMTEffectGlobals*)(
        (unsigned char*)RwEngineInstance + _rpMultiTextureModule.globalsOffset);

    if (dictionary == globals->currentDictionary)
        globals->currentDictionary = 0;
    end = &dictionary->effects.link;
    link = end->next;
    while (link != end) {
        RpMTEffect* effect = (RpMTEffect*)(
            (unsigned char*)link - 0x28);
        link = link->next;
        RpMTEffectDictRemoveEffect(effect);
    }
    dictionary->globalLink.prev->next = dictionary->globalLink.next;
    dictionary->globalLink.next->prev = dictionary->globalLink.prev;
    RwEngineInstance->fpFree(dictionary);
}

RpMTEffectDict* RpMTEffectDictAddEffect(RpMTEffectDict* dictionary,
                                        RpMTEffect* effect)
{

    if (effect->dictLink.next) {
        effect->dictLink.prev->next = effect->dictLink.next;
        effect->dictLink.next->prev = effect->dictLink.prev;
        RpMTEffectDestroy(effect);
    }
    effect->dictLink.next = dictionary->effects.link.next;
    effect->dictLink.prev = &dictionary->effects.link;
    dictionary->effects.link.next->prev = &effect->dictLink;
    dictionary->effects.link.next = &effect->dictLink;
    RpMTEffectAddRef(effect);
    return dictionary;
}

RpMTEffect* RpMTEffectDictRemoveEffect(RpMTEffect* effect)
{
    if (effect->dictLink.next) {
        RwLLLink* previous;

        effect->dictLink.prev->next = effect->dictLink.next;
        previous = effect->dictLink.prev;
        effect->dictLink.next->prev = previous;
        RpMTEffectDestroy(effect);
    }
    return effect;
}

RpMTEffect* RpMTEffectDictFindNamedEffect(RpMTEffectDict* dictionary,
                                          const char* name)
{
    RwLLLink* link;
    RpMTEffect* effect;

    link = dictionary->effects.link.next;
    while (link != &dictionary->effects.link) {
        effect = (RpMTEffect*)(
            (unsigned char*)link - 0x28);
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
            RpMTEffectRegEntry* entry = &EffectRegEntries[effect->type];

            if (entry->destroy) {
                entry->destroy(effect);
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
            RpMTEffectDict* dictionary = (RpMTEffectDict*)(
                (unsigned char*)link - 8);

            effect = RpMTEffectDictFindNamedEffect(dictionary, name);
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
