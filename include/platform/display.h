#ifndef PLATFORM_DISPLAY_H
#define PLATFORM_DISPLAY_H
typedef struct RwFrame RwFrame;
typedef struct RwTexture RwTexture;
typedef struct ScreenObj ScreenObj;
typedef struct RwRaster RwRaster;
#ifndef RW_MEMORY_FUNCTIONS_DEFINED
#define RW_MEMORY_FUNCTIONS_DEFINED
typedef struct RwMemoryFunctions {
    void* (*alloc)(unsigned long); void (*free)(void*);
    void* (*realloc)(void*, unsigned long); void* (*calloc)(unsigned long, unsigned long);
} RwMemoryFunctions;
#endif
typedef struct FadingScreen {
    unsigned long field_00; unsigned long field_04; RwTexture* snapshotTex;
    int fade_active; float alpha; ScreenObj* fade_obj;
} FadingScreen;
typedef struct RwCamera {
    char object[4]; RwFrame* frame; char pad08[0x58]; void* frameBuffer; void* zBuffer;
    float viewWindow[2]; char pad70[0x10]; float nearPlane; float farPlane; float fogPlane;
} RwCamera;
typedef struct RpWorld RpWorld; typedef struct RpClump RpClump;
typedef struct RpLight {
    unsigned char type, subType, flags, privateFlags; RwFrame* frame;
} RpLight;
typedef struct RwEngineInstanceType {
    char pad00[0x20]; int (*fpRenderStateSet)(int, int); char pad24[0x110];
    void* (*fpMalloc)(unsigned long, unsigned long); void (*fpFree)(void*);
} RwEngineInstanceType;
extern FadingScreen fading_screen;
extern unsigned char background_color[4], load_meter_bgnd_color[4];
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
