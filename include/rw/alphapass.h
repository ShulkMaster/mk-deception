#ifndef RW_ALPHAPASS_H
#define RW_ALPHAPASS_H

#include "rw/rwcore_types.h"
#include "rw/rpworld_types.h"

typedef struct RxGCTevAlphaPass {
    char pad00[0x08];
    int mode;
    char pad0C[0x10];
    int field_0x1C;
    int field_0x20;
    int field_0x24;
    char pad28[0x10];
} RxGCTevAlphaPass;

RwTexture* RpMaterialGetAlphaPassTexture(RpMaterial* material);
RwTexture* RpMaterialSetAlphaPassTexture(RpMaterial* material,
                                         RwTexture* texture);
RwTexture* RpMaterialGetDualAlphaPassTexture(RpMaterial* material);
RwTexture* RpMaterialSetDualAlphaPassTexture(RpMaterial* material,
                                             RwTexture* texture);
void _rxGCTevAlphaPassSetup(RxGCTevAlphaPass* pass);
void _rxGCTevAlphaPassCleanup(RxGCTevAlphaPass* pass);
void SetSingleTextureAlphaPassWithAlphaComp(RwTexture* texture,
                                            RwTexture* alphaTexture,
                                            RxGCTevAlphaPass* pass);
void _rxGCTevAlphaMultiPassSetup(RxGCTevAlphaPass* pass);
void _rxGCTevAlphaMultiPassCleanup(RxGCTevAlphaPass* pass);

#endif
