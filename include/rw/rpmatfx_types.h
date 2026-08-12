#ifndef RW_RPMATFX_TYPES_H
#define RW_RPMATFX_TYPES_H

#include "rw/rpworld_types.h"

typedef enum RpMatFXMaterialFlags {
    rpMATFXEFFECTNULL = 0,
    rpMATFXEFFECTBUMPMAP = 1,
    rpMATFXEFFECTENVMAP = 2,
    rpMATFXEFFECTBUMPENVMAP = 3,
    rpMATFXEFFECTDUAL = 4,
    rpMATFXEFFECTUVTRANSFORM = 5,
    rpMATFXEFFECTDUALUVTRANSFORM = 6,
    rpMATFXEFFECTMAX = 7,
    rpMATFXNUMEFFECTS = 6
} RpMatFXMaterialFlags;

typedef struct RpMatFXEnvMapData {
    RwFrame* frame;
    RwTexture* texture;
    float coefficient;
    int useFrameBufferAlpha;
} RpMatFXEnvMapData;

typedef struct RpMatFXBumpMapData {
    RwFrame* frame;
    RwTexture* texture;
    RwTexture* bumped_texture;
    float storedCoefficient;
    float coefficient;
} RpMatFXBumpMapData;

typedef struct RpMatFXDualData {
    RwTexture* texture;
    int srcBlendMode;
    int dstBlendMode;
} RpMatFXDualData;

typedef struct RpMatFXUVTransformData {
    RwMatrix* baseTransform;
    RwMatrix* dualTransform;
} RpMatFXUVTransformData;

typedef union RpMatFXDataUnion {
    RpMatFXBumpMapData bump;
    RpMatFXEnvMapData env;
    RpMatFXDualData dual;
    RpMatFXUVTransformData uv;
    unsigned char raw[0x14];
} RpMatFXDataUnion;

typedef struct RpMatFXMaterialSlot {
    RpMatFXDataUnion data;
    RpMatFXMaterialFlags type;
} RpMatFXMaterialSlot;

typedef struct RpMatFXMaterialData {
    RpMatFXMaterialSlot slot[2];
    RpMatFXMaterialFlags effects;
} RpMatFXMaterialData;

RpMatFXMaterialFlags RpMatFXMaterialGetEffects(const RpMaterial* material);
void* MatFXGetData(RpMaterial* material, RpMatFXMaterialFlags effect);

#endif
