#ifndef GAME_GCSPECSKIN_H
#define GAME_GCSPECSKIN_H

typedef struct RxPipeline RxPipeline;
typedef struct RwMatrix RwMatrix;
typedef struct RpMaterial RpMaterial;
typedef struct RwTexture RwTexture;

extern RxPipeline* SpecSkinAtomicPipeline;
extern RxPipeline* SpecSkinMaterialPipeline;
extern RwMatrix SpecularMatrix;

int specskin_plugin_attach(void);
void CleanupSpecularity(RpMaterial* material, RwTexture* base_texture,
                        RwTexture* alpha_texture);
void ProcessSpecularity(RpMaterial* material, RwTexture* base_texture,
                        RwTexture* alpha_texture, int has_specular_map);

#endif
