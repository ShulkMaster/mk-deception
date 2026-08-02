#ifndef RW_RPMATFX_TYPES_H
#define RW_RPMATFX_TYPES_H

#include "rw/rpworld_types.h"

/*
 * Partial MatFX material payloads used by Midway texture ownership helpers.
 * MatFXGetData selects one of these payloads by effect type.
 */
typedef enum RpMatFXMaterialFlags {
    rpMATFXEFFECTNULL = 0,
    rpMATFXEFFECTBUMPMAP = 1,
    rpMATFXEFFECTENVMAP = 4,
    rpMATFXEFFECTDUAL = 5,
    rpMATFXEFFECTBUMPENVMAP = 6
} RpMatFXMaterialFlags;

typedef struct RpMatFXEnvMapData {
    RwTexture* texture; /* +0x00 */
} RpMatFXEnvMapData;

typedef struct RpMatFXBumpMapData {
    char pad00[4];
    RwTexture* texture;       /* +0x04 */
    RwTexture* bumped_texture; /* +0x08 */
    char pad0C[4];
    float coefficient;        /* +0x10 */
} RpMatFXBumpMapData;

int RpMatFXMaterialGetEffects(RpMaterial* material);
void* MatFXGetData(RpMaterial* material, int effect);

#endif
