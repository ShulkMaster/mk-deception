#ifndef SHADOW_H
#define SHADOW_H

#include "rw/rwcamera_internal.h"

typedef struct ShadowObject ShadowObject;
typedef struct ShadowboxObject ShadowboxObject;
typedef struct RpClump RpClump;
typedef struct MkObj MkObj;
typedef struct FighterState FighterState;

int init_shadow(ShadowObject* shadow, MkObj* object);
void UpdateShadow(FighterState* fighter, ShadowObject* shadow, MkObj* object);
int UpdateShadowCameraLightSource(const float* angles);
void destroy_shadow_system(void);
void TearDownShadow(ShadowObject* shadow);
void shadow_set_new_ground_plane(ShadowObject* shadow, ShadowboxObject* ground,
                                 float y);
int SetupShadow(void* shadow);
int init_shadow_system(void);

void ShadowRasterBlur(RwRaster* srcRaster, RwRaster* dstRaster,
                      RwCamera* ipCamera);
void ShadowCameraUpdate(RwCamera* camera, RpClump* clump, int clear);

extern float ShadowStrength;

#endif
