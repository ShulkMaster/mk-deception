#ifndef RW_RPMATFX_H
#define RW_RPMATFX_H

#include "rw/rwcore_types.h"
#include "rw/rpmatfx_types.h"
#include "rw/rxpipeline.h"
#include "rw/rwstream.h"

typedef struct RpMTEffect RpMTEffect;
typedef struct RpMTEffectDict RpMTEffectDict;
typedef struct RpMaterial RpMaterial;
typedef struct RpMultiTexture RpMultiTexture;
typedef struct RpMultiTextureRegEntry RpMultiTextureRegEntry;
typedef struct RpGameCubeMTEffectConfig RpGameCubeMTEffectConfig;

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
RpMTEffect* RpGameCubeMTEffectCreate(
    RwUInt32 count64, RwUInt32 count24, RwUInt32 count60,
    RwUInt32 count20, RwUInt32 count40);
RpGameCubeMTEffectConfig* RpGameCubeMTEffectGetConfig(RpMTEffect* effect);
RwBool _rpGameCubeMTDataPluginAttach(void);

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
                             RwUInt32 coords);
RwUInt32 RpMultiTextureGetCoords(const RpMultiTexture* multiTexture,
                                 RwUInt32 index);
RpMultiTexture* RpMaterialGetMultiTexture(RpMaterial* material,
                                          RwInt32 platform);

RwBool RpMatFXPluginAttach(void);
RpAtomic* RpMatFXAtomicEnableEffects(RpAtomic* atomic);
RwBool RpMatFXAtomicQueryEffects(const RpAtomic* atomic);
RpWorldSector* RpMatFXWorldSectorEnableEffects(RpWorldSector* worldSector);
RpMaterial* RpMatFXMaterialSetEffects(RpMaterial* material,
                                      RpMatFXMaterialFlags effects);
RpMaterial* RpMatFXMaterialSetBumpMapTexture(RpMaterial*, RwTexture*);
RpMaterial* RpMatFXMaterialSetBumpMapFrame(RpMaterial*, RwFrame*);
RpMaterial* RpMatFXMaterialSetBumpMapCoefficient(RpMaterial*, RwReal);
RwFrame* RpMatFXMaterialGetBumpMapFrame(const RpMaterial*);
RwReal RpMatFXMaterialGetBumpMapCoefficient(const RpMaterial*);
RpMaterial* RpMatFXMaterialSetEnvMapTexture(RpMaterial*, RwTexture*);
RpMaterial* RpMatFXMaterialSetEnvMapFrame(RpMaterial*, RwFrame*);
RpMaterial* RpMatFXMaterialSetEnvMapFrameBufferAlpha(RpMaterial*, RwBool);
RpMaterial* RpMatFXMaterialSetEnvMapCoefficient(RpMaterial*, RwReal);
RwTexture* RpMatFXMaterialGetEnvMapTexture(const RpMaterial*);
RwFrame* RpMatFXMaterialGetEnvMapFrame(const RpMaterial*);
RwBool RpMatFXMaterialGetEnvMapFrameBufferAlpha(const RpMaterial*);
RwReal RpMatFXMaterialGetEnvMapCoefficient(const RpMaterial*);
RpMaterial* RpMatFXMaterialSetDualTexture(RpMaterial*, RwTexture*);
RpMaterial* RpMatFXMaterialSetDualBlendModes(RpMaterial*, RwBlendFunction,
                                             RwBlendFunction);
RwTexture* RpMatFXMaterialGetDualTexture(const RpMaterial*);
const RpMaterial* RpMatFXMaterialGetDualBlendModes(const RpMaterial*,
                                                   RwBlendFunction*,
                                                   RwBlendFunction*);
RpMaterial* RpMatFXMaterialSetUVTransformMatrices(RpMaterial*, RwMatrix*,
                                                  RwMatrix*);
const RpMaterial* RpMatFXMaterialGetUVTransformMatrices(const RpMaterial*,
                                                        RwMatrix**,
                                                        RwMatrix**);

#endif
