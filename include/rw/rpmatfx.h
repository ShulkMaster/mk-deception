#ifndef RW_RPMATFX_H
#define RW_RPMATFX_H

#include "rw/rwcore_types.h"
#include "rw/rwstream.h"

typedef struct RpMTEffect RpMTEffect;
typedef struct RpMTEffectDict RpMTEffectDict;
typedef struct RpMaterial RpMaterial;
typedef struct RpMultiTexture RpMultiTexture;
typedef struct RpMultiTextureRegEntry RpMultiTextureRegEntry;

typedef RpMTEffect* (*RpMTEffectStreamReadCallBack)(RwStream* stream,
                                                    RwInt32 type,
                                                    RwUInt32 version,
                                                    RwInt32 length);
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

struct RpMultiTextureRegEntry {
    RwInt32 platform;
    RwUInt32 pluginID;
    RwInt32 materialOffset;
    RwInt32 platformDataSize;
};

struct RpMultiTexture {
    RpMultiTextureRegEntry* registration;
    RwUInt32 numTextures;
    RwTexture* textures[8];
    RwUInt8 texCoords[8];
    RpMTEffect* effect;
    void* platformData;
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

RwBool _rpMultiTexturePluginAttach(void);
RwBool _rpMaterialRegisterMultiTexturePlugin(RwInt32 platform,
                                             RwUInt32 pluginID,
                                             RwInt32 platformDataSize);
RpMultiTexture* RpMultiTextureSetEffect(RpMultiTexture* multiTexture,
                                        RpMTEffect* effect);
RpMTEffect* RpMultiTextureGetEffect(const RpMultiTexture* multiTexture);
RpMultiTexture* RpMultiTextureSetTexture(RpMultiTexture* multiTexture,
                                         RwUInt32 index,
                                         RwTexture* texture);
RwTexture* RpMultiTextureGetTexture(const RpMultiTexture* multiTexture,
                                    RwUInt32 index);
void RpMultiTextureSetCoords(RpMultiTexture* multiTexture, RwUInt32 index,
                             RwUInt8 coords);
RwUInt8 RpMultiTextureGetCoords(const RpMultiTexture* multiTexture,
                                RwUInt32 index);
RpMultiTexture* RpMaterialGetMultiTexture(RpMaterial* material,
                                          RwInt32 platform);

#endif
