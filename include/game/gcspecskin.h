#ifndef GAME_GCSPECSKIN_H
#define GAME_GCSPECSKIN_H

typedef struct RxPipeline RxPipeline;
typedef struct RwMatrix RwMatrix;

extern RxPipeline* SpecSkinAtomicPipeline;
extern RxPipeline* SpecSkinMaterialPipeline;
extern RwMatrix SpecularMatrix;

int specskin_plugin_attach(void);

#endif
