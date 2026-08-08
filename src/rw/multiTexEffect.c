#include "libmkparticle/rw_engine.h"
#include "rw/rpmatfx.h"
#include "rw/rwerror.h"
#include "rw/rwstream_internal.h"

typedef struct RpMTEffectRegEntry {
    RwInt32 type;
    RpMTEffectDestroyCallBack destroy;
    RpMTEffectStreamReadCallBack streamRead;
    RpMTEffectStreamWriteCallBack streamWrite;
    RpMTEffectStreamGetSizeCallBack streamGetSize;
} RpMTEffectRegEntry;

typedef struct RpMTEffectGlobals {
    RwLinkList dictionaries;
    RpMTEffectDict* currentDictionary;
    RwInt32 scratchSize;
    RwChar* scratch;
    RwChar* scratchName;
} RpMTEffectGlobals;

extern void* memset(void* destination, RwInt32 value, RwUInt32 size);
extern RwModuleInfo _rpMultiTextureModule;

#define MTEFFECTGLOBALS                                                   \
    ((RpMTEffectGlobals*)((RwUInt8*)RwEngineInstance +                    \
                          _rpMultiTextureModule.globalsOffset))
#define EFFECT_FROM_LINK(link) ((RpMTEffect*)((RwUInt8*)(link) - 0x28))
#define DICT_FROM_LINK(link) ((RpMTEffectDict*)((RwUInt8*)(link) - 8))

static RpMTEffectRegEntry EffectRegEntries[10];
static RpMTEffectDict* DummyDict;

RwBool _rpMTEffectSystemInit(void)
{
    memset(EffectRegEntries, 0, sizeof(EffectRegEntries));
    return TRUE;
}

RwBool _rpMTEffectRegisterPlatform(
    RwInt32 type, RpMTEffectStreamReadCallBack read,
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
    return TRUE;
}

RwBool _rpMTEffectOpen(void)
{
    RpMTEffectGlobals* globals = MTEFFECTGLOBALS;
    rwLinkListInitialize(&globals->dictionaries);
    DummyDict = RpMTEffectDictCreate();
    if (!DummyDict)
        return FALSE;
    globals->currentDictionary = DummyDict;
    globals->scratch = RwEngineInstance->fpMalloc(0x220, 0x4012C);
    if (!globals->scratch) {
        RwError error;
        RpMTEffectDictDestroy(globals->currentDictionary);
        error.pluginID = 0x120;
        error.errorCode = _rwerror(0x80000013, 0x220);
        RwErrorSet(&error);
        return FALSE;
    }
    memset(globals->scratch, 0, 0x220);
    globals->scratchName = globals->scratch + 0x100;
    globals->scratchSize = 0x100;
    return TRUE;
}

RwBool _rpMTEffectClose(void)
{
    RpMTEffectGlobals* globals = MTEFFECTGLOBALS;
    RwLLLink* link;
    RwLLLink* end;
    if (globals->scratch) {
        RwEngineInstance->fpFree(globals->scratch);
        globals->scratch = NULL;
        globals->scratchName = NULL;
        globals->scratchSize = 0;
    }
    end = &globals->dictionaries.link;
    link = globals->dictionaries.link.next;
    while (link != end) {
        if (DICT_FROM_LINK(link) == DummyDict) {
            RpMTEffectDictDestroy(DummyDict);
            DummyDict = NULL;
            break;
        } else {
            link = link->next;
        }
    }
    return globals->dictionaries.link.next == end;
}

RpMTEffect* _rpMTEffectInit(RpMTEffect* effect, RwInt32 type)
{
    memset(effect, 0, sizeof(*effect));
    effect->type = type;
    effect->refCount = 1;
    effect->dictLink.next = NULL;
    effect->dictLink.prev = NULL;
    if (type && MTEFFECTGLOBALS->currentDictionary)
        RpMTEffectDictAddEffect(MTEFFECTGLOBALS->currentDictionary, effect);
    return effect;
}

RpMTEffectDict* RpMTEffectDictCreate(void)
{
    RpMTEffectDict* dictionary =
        RwEngineInstance->fpMalloc(sizeof(*dictionary), 0x3012C);
    if (!dictionary) {
        RwError error;
        error.pluginID = 0x120;
        error.errorCode = _rwerror(0x80000013, sizeof(*dictionary));
        RwErrorSet(&error);
        return NULL;
    }
    rwLinkListInitialize(&dictionary->effects);
    rwLinkListAddLLLink(&MTEFFECTGLOBALS->dictionaries,
                        &dictionary->globalLink);
    return dictionary;
}

void RpMTEffectDictDestroy(RpMTEffectDict* dictionary)
{
    RwLLLink* link;
    RwLLLink* next;
    if (dictionary == MTEFFECTGLOBALS->currentDictionary)
        MTEFFECTGLOBALS->currentDictionary = NULL;
    link = dictionary->effects.link.next;
    while (link != &dictionary->effects.link) {
        next = link->next;
        RpMTEffectDictRemoveEffect(EFFECT_FROM_LINK(link));
        link = next;
    }
    rwLinkListRemoveLLLink(&dictionary->globalLink);
    RwEngineInstance->fpFree(dictionary);
}

RpMTEffectDict* RpMTEffectDictAddEffect(RpMTEffectDict* dictionary,
                                        RpMTEffect* effect)
{
    if (effect->dictLink.next)
        RpMTEffectDictRemoveEffect(effect);
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
                                          const RwChar* name)
{
    RwLLLink* link = dictionary->effects.link.next;
    while (link != &dictionary->effects.link) {
        RpMTEffect* effect = EFFECT_FROM_LINK(link);
        if (!RwEngineInstance->stringFuncs.vecStrcmp(effect->name, name))
            return effect;
        link = link->next;
    }
    return NULL;
}

RpMTEffect* RpMTEffectCreateDummy(void)
{
    RpMTEffect* effect = RwEngineInstance->fpMalloc(sizeof(*effect), 0x3012C);
    if (!effect) {
        RwError error;
        error.pluginID = 0x120;
        error.errorCode = _rwerror(0x80000013, sizeof(*effect));
        RwErrorSet(&error);
        return NULL;
    }
    return _rpMTEffectInit(effect, 0);
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
    RwInt32 type;
    RwUInt32 length;
    RwUInt32 version;
    RwChar name[32];
    RpMTEffect* effect;
    RpMTEffectRegEntry* entry;
    if (!RwStreamFindChunk(stream, 1, NULL, NULL) ||
        !RwStreamRead(stream, &type, sizeof(type)))
        return NULL;
    RwMemNative32(&type, sizeof(type));
    entry = &EffectRegEntries[type];
    if (!entry->streamRead)
        return NULL;
    if (!_rwStringStreamFindAndRead(name, stream))
        return NULL;
    if (!RwStreamFindChunk(stream, 3, &length, &version))
        return NULL;
    effect = entry->streamRead(stream, type, version, length);
    if (!effect)
        return NULL;
    RpMTEffectSetName(effect, name);
    return effect;
}

RpMTEffect* RpMTEffectFind(const RwChar* name)
{
    RpMTEffectGlobals* globals = MTEFFECTGLOBALS;
    RpMTEffect* effect = NULL;
    RwStream* stream;
    if (globals->currentDictionary) {
        effect = RpMTEffectDictFindNamedEffect(globals->currentDictionary, name);
    } else {
        RwLLLink* link = globals->dictionaries.link.next;
        while (link != &globals->dictionaries.link) {
            effect = RpMTEffectDictFindNamedEffect(DICT_FROM_LINK(link), name);
            if (effect)
                break;
            link = link->next;
        }
    }
    if (effect) {
        RpMTEffectAddRef(effect);
        return effect;
    }
    RwEngineInstance->stringFuncs.vecStrcpy(globals->scratchName,
                                            globals->scratch);
    RwEngineInstance->stringFuncs.vecStrcat(globals->scratchName, name);
    stream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMREAD,
                          globals->scratchName);
    if (!stream)
        return NULL;
    if (!RwStreamFindChunk(stream, 0x20, NULL, NULL)) {
        RwStreamClose(stream, NULL);
        return NULL;
    }
    effect = RpMTEffectStreamRead(stream);
    RwStreamClose(stream, NULL);
    return effect;
}

RpMTEffect* RpMTEffectSetName(RpMTEffect* effect, const RwChar* name)
{
    RwEngineInstance->stringFuncs.vecStrncpy(effect->name, name, 0x1F);
    return effect;
}

void RpMTEffectAddRef(RpMTEffect* effect)
{
    effect->refCount++;
}
