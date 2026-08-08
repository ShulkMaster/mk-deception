#ifndef RW_RPMATFX_H
#define RW_RPMATFX_H

#include "rw/rwcore_types.h"
#include "rw/rwstream.h"

typedef struct RpMTEffect RpMTEffect;
typedef struct RpMTEffectDict RpMTEffectDict;

typedef RpMTEffect* (*RpMTEffectStreamReadCallBack)(RwStream* stream,
                                                    RwInt32 type,
                                                    RwInt32 length,
                                                    RwUInt32 version);
typedef RwStream* (*RpMTEffectStreamWriteCallBack)(const RpMTEffect* effect,
                                                   RwStream* stream);
typedef RwInt32 (*RpMTEffectStreamGetSizeCallBack)(const RpMTEffect* effect);
typedef void (*RpMTEffectDestroyCallBack)(RpMTEffect* effect);

struct RpMTEffect {
    RwInt32 type;
    RwUInt32 refCount;
    RwChar name[32];
    RwLLLink dictLink;
};

struct RpMTEffectDict {
    RwLinkList effects;
    RwLLLink globalLink;
};

RwBool _rpMTEffectSystemInit(void);
RwBool _rpMTEffectRegisterPlatform(
    RwInt32 type, RpMTEffectStreamReadCallBack read,
    RpMTEffectStreamWriteCallBack write,
    RpMTEffectStreamGetSizeCallBack getSize,
    RpMTEffectDestroyCallBack destroy);
RwBool _rpMTEffectOpen(void);
RwBool _rpMTEffectClose(void);
RpMTEffect* _rpMTEffectInit(RpMTEffect* effect, RwInt32 type);
RpMTEffectDict* RpMTEffectDictCreate(void);
void RpMTEffectDictDestroy(RpMTEffectDict* dictionary);
RpMTEffectDict* RpMTEffectDictAddEffect(RpMTEffectDict* dictionary,
                                        RpMTEffect* effect);
RpMTEffect* RpMTEffectDictRemoveEffect(RpMTEffect* effect);
RpMTEffect* RpMTEffectDictFindNamedEffect(RpMTEffectDict* dictionary,
                                          const RwChar* name);
RpMTEffect* RpMTEffectCreateDummy(void);
void RpMTEffectDestroy(RpMTEffect* effect);
RpMTEffect* RpMTEffectStreamRead(RwStream* stream);
RpMTEffect* RpMTEffectFind(const RwChar* name);
RpMTEffect* RpMTEffectSetName(RpMTEffect* effect, const RwChar* name);
void RpMTEffectAddRef(RpMTEffect* effect);

#endif
