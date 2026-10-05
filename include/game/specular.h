#ifndef SPECULAR_H
#define SPECULAR_H

typedef struct RpAtomic RpAtomic;
typedef struct RpClump RpClump;
typedef struct RpLight RpLight;
typedef struct RpMaterial RpMaterial;

RpAtomic* force_specular_texture_atomic_callback(RpAtomic* atomic,
                                                 void* texture);
RpAtomic* restore_specular_texture_atomic_callback(RpAtomic* atomic,
                                                   void* data);
RpAtomic* swap_specular_texture_atomic_callback(RpAtomic* atomic,
                                                void* texture);
void SpecularMaterialCalcMatrix(RpMaterial* material);
void SpecularMaterialSetup(RpMaterial* material, RpLight* light, void* frame,
                           void* phong_texture);
void specskin_initialize_clump(void* clump);
void specskin_turn_off_specularity_on_clump(void* clump);
void specskin_force_clipping_clump(void* clump, int value);
RpMaterial* specskin_material_setup(RpMaterial* material, void* is_player);
void specular_condition_clump(void* clump);
int specskin_plugin_attach(void);
void SetupShadowPlayerPipeline(RpClump* clump);

#endif
