#ifndef SHADOW_H
#define SHADOW_H

typedef struct ShadowObject ShadowObject;
typedef struct ShadowboxObject ShadowboxObject;

int init_shadow(void* shadow, void* clump);
void UpdateShadow(void* fighter, void* shadow, void* ltm);
int UpdateShadowCameraLightSource(const float* angles);
void destroy_shadow_system(void);
void TearDownShadow(ShadowObject* shadow);
void shadow_set_new_ground_plane(ShadowObject* shadow, ShadowboxObject* ground,
                                 float y);
int SetupShadow(void* shadow);
int init_shadow_system(void);

void ShadowRasterBlur(void* src_raster, void* dst_raster, void* ip_camera);
void ShadowCameraUpdate(void* camera, void* clump, int clear);

extern float ShadowStrength;

#endif
