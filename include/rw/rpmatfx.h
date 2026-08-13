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
typedef struct RpGameCubeMTEntry24 RpGameCubeMTEntry24;
typedef struct RpGameCubeMTEntry64 RpGameCubeMTEntry64;
typedef struct RpGameCubeMTEntry60 RpGameCubeMTEntry60;
typedef struct RpGameCubeMTEntry20 RpGameCubeMTEntry20;
typedef struct RpGameCubeMTEntry40 RpGameCubeMTEntry40;

typedef RpMTEffect* (*RpMTEffectStreamReadCallBack)(RwStream* stream,
                                                    int type,
                                                    unsigned int version,
                                                    int length);
typedef const RpMTEffect* (*RpMTEffectStreamWriteCallBack)(
    const RpMTEffect* effect, RwStream* stream);
typedef int (*RpMTEffectStreamGetSizeCallBack)(const RpMTEffect* effect);
typedef void (*RpMTEffectDestroyCallBack)(RpMTEffect* effect);

struct RpMTEffect {
    int type;
    unsigned int refCount;
    char name[32];
    RwLLLink dictLink;
};

struct RpMTEffectDict {
    RwLinkList effects;
    RwLLLink globalLink;
};

struct RpMultiTextureRegEntry {
    int platform;
    unsigned int pluginID;
    int materialOffset;
    unsigned int platformDataSize;
};

struct RpMultiTexture {
    RpMultiTextureRegEntry* registration;
    unsigned int numTextures;
    RwTexture* textures[8];
    unsigned char texCoords[8];
    RpMTEffect* effect;
    void* platformData;
};

struct RpGameCubeMTEntry24 {
    int value[5];
    unsigned short field_0x14;
    unsigned short field_0x16;
};

struct RpGameCubeMTEffectConfig {
    int field_0x00;
    unsigned char allocationCount24;
    unsigned char allocationCount64;
    unsigned char allocationCount60;
    unsigned char allocationCount20;
    unsigned char allocationCount40;
    unsigned char count24;
    unsigned char count64;
    unsigned char count60;
    unsigned char count20;
    unsigned char count40;
    unsigned short values[16];
    unsigned char bytes[16];
    unsigned char reserved_0x3E[2];
    RpGameCubeMTEntry60* entries60;
    RpGameCubeMTEntry24* entries24;
    RpGameCubeMTEntry40* entries40;
    RpGameCubeMTEntry20* entries20;
    RpGameCubeMTEntry64* entries64;
    int field_0x54;
    int field_0x58;
};

int _rpMTEffectSystemInit(void);
int _rpMTEffectRegisterPlatform(
    int type, RpMTEffectStreamReadCallBack read,
    RpMTEffectStreamWriteCallBack write,
    RpMTEffectStreamGetSizeCallBack getSize,
    RpMTEffectDestroyCallBack destroy);
int _rpMTEffectOpen(void);
int _rpMTEffectClose(void);
RpMTEffect* _rpMTEffectInit(RpMTEffect* effect, int type);
RpMTEffectDict* RpMTEffectDictCreate(void);
void RpMTEffectDictDestroy(RpMTEffectDict* dictionary);
RpMTEffectDict* RpMTEffectDictAddEffect(RpMTEffectDict* dictionary,
                                        RpMTEffect* effect);
RpMTEffect* RpMTEffectDictRemoveEffect(RpMTEffect* effect);
RpMTEffect* RpMTEffectDictFindNamedEffect(RpMTEffectDict* dictionary,
                                          const char* name);
RpMTEffect* RpMTEffectCreateDummy(void);
void RpMTEffectDestroy(RpMTEffect* effect);
RpMTEffect* RpMTEffectStreamRead(RwStream* stream);
RpMTEffect* RpMTEffectFind(const char* name);
RpMTEffect* RpMTEffectSetName(RpMTEffect* effect, const char* name);
void RpMTEffectAddRef(RpMTEffect* effect);
RpMTEffect* RpGameCubeMTEffectCreate(
    unsigned int count64, unsigned int count24, unsigned int count60,
    unsigned int count20, unsigned int count40);
RpGameCubeMTEffectConfig* RpGameCubeMTEffectGetConfig(RpMTEffect* effect);
int _rpGameCubeMTDataPluginAttach(void);

int _rpMultiTexturePluginAttach(void);
int _rpMaterialRegisterMultiTexturePlugin(int platform,
                                             unsigned int pluginID,
                                             unsigned int platformDataSize);
RpMultiTexture* RpMultiTextureSetEffect(RpMultiTexture* multiTexture,
                                        RpMTEffect* effect);
RpMTEffect* RpMultiTextureGetEffect(const RpMultiTexture* multiTexture);
RpMultiTexture* RpMultiTextureSetTexture(RpMultiTexture* multiTexture,
                                         unsigned int index,
                                         RwTexture* texture);
RwTexture* RpMultiTextureGetTexture(const RpMultiTexture* multiTexture,
                                    unsigned int index);
void RpMultiTextureSetCoords(RpMultiTexture* multiTexture, unsigned int index,
                             unsigned int coords);
unsigned int RpMultiTextureGetCoords(const RpMultiTexture* multiTexture,
                                 unsigned int index);
RpMultiTexture* RpMaterialGetMultiTexture(RpMaterial* material,
                                          int platform);

int RpMatFXPluginAttach(void);
RpAtomic* _rpMatFXPipelineAtomicSetup(RpAtomic* atomic);
RpWorldSector* _rpMatFXPipelineWorldSectorSetup(RpWorldSector* worldSector);
RpAtomic* RpMatFXAtomicEnableEffects(RpAtomic* atomic);
int RpMatFXAtomicQueryEffects(const RpAtomic* atomic);
RpWorldSector* RpMatFXWorldSectorEnableEffects(RpWorldSector* worldSector);
RpMaterial* RpMatFXMaterialSetEffects(RpMaterial* material,
                                      RpMatFXMaterialFlags effects);
RpMaterial* RpMatFXMaterialSetBumpMapTexture(RpMaterial*, RwTexture*);
RpMaterial* RpMatFXMaterialSetBumpMapFrame(RpMaterial*, RwFrame*);
RpMaterial* RpMatFXMaterialSetBumpMapCoefficient(RpMaterial*, float);
RwFrame* RpMatFXMaterialGetBumpMapFrame(const RpMaterial*);
float RpMatFXMaterialGetBumpMapCoefficient(const RpMaterial*);
RpMaterial* RpMatFXMaterialSetEnvMapTexture(RpMaterial*, RwTexture*);
RpMaterial* RpMatFXMaterialSetEnvMapFrame(RpMaterial*, RwFrame*);
RpMaterial* RpMatFXMaterialSetEnvMapFrameBufferAlpha(RpMaterial*, int);
RpMaterial* RpMatFXMaterialSetEnvMapCoefficient(RpMaterial*, float);
RwTexture* RpMatFXMaterialGetEnvMapTexture(const RpMaterial*);
RwFrame* RpMatFXMaterialGetEnvMapFrame(const RpMaterial*);
int RpMatFXMaterialGetEnvMapFrameBufferAlpha(const RpMaterial*);
float RpMatFXMaterialGetEnvMapCoefficient(const RpMaterial*);
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
