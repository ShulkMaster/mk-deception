#ifndef SPECULAR_H
#define SPECULAR_H

typedef struct RpAtomic RpAtomic;
typedef struct RpClump RpClump;

RpAtomic* force_specular_texture_atomic_callback(RpAtomic* atomic,
                                                 void* texture);
RpAtomic* restore_specular_texture_atomic_callback(RpAtomic* atomic,
                                                   void* data);
RpAtomic* swap_specular_texture_atomic_callback(RpAtomic* atomic,
                                                void* texture);
void SpecularMaterialCalcMatrix(void* material);
void specskin_initialize_clump(void* clump);
void specskin_force_clipping_clump(void* clump, int value);
void* specskin_material_setup(void* material, unsigned int is_player);
void specular_condition_clump(void* clump);
int specskin_plugin_attach(void);
void SetupShadowPlayerPipeline(RpClump* clump);

#endif
