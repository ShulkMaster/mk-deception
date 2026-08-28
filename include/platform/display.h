#ifndef PLATFORM_DISPLAY_H
#define PLATFORM_DISPLAY_H
#include "rw/rplight.h"
#include "rw/rwcamera_internal.h"
#include "rw/rwdevice.h"
typedef struct RwTexture RwTexture;
typedef struct ScreenObj ScreenObj;
typedef struct RwRaster RwRaster;
typedef struct FadingScreen {
    unsigned long field_00; unsigned long field_04; RwTexture* snapshotTex;
    int fade_active; float alpha; ScreenObj* fade_obj;
} FadingScreen;
typedef struct RpWorld RpWorld; typedef struct RpClump RpClump;
typedef struct RwEngineInstanceType {
    char pad00[0x1C];
    float im2d_depth;
    int (*fpRenderStateSet)(int, int);
    char pad24[0x0C];
    void (*fpIm2DRenderIndexedPrimitive)(int, void*, int);
    char pad34[0x100];
    void* (*fpMalloc)(unsigned long, unsigned long); void (*fpFree)(void*);
} RwEngineInstanceType;
extern FadingScreen fading_screen;
extern RwRGBA background_color;
extern RwRGBA load_meter_bgnd_color;
extern RwCamera* Camera; extern RpWorld* World;
extern unsigned long f_render_all_atomics, display_off, renderware_initialized;
int init_display(void); int AttachPlugins(void); void Render(void); void display_shutdown(void);
void turn_display_on(void); void turn_display_off(void);
void set_background_color(unsigned char, unsigned char, unsigned char);
void wait_for_display_to_flush(void); void update_camera_facing_matrix(void);
int set_render_state(int, int); void start_first_pass_render(void); void end_first_pass_render(void);
void TakeCameraSnapShot(void); void DeleteCameraSnapShot(void);
int add_clump_to_world(RpWorld*, RpClump*); void pull_clump_from_world(RpClump*);
#endif
