#include "platform/display.h"

#include "dolphin/gx.h"
#include "game/game_info.h"
#include "libmkparticle/pfxfont.h"
#include "libmkparticle/particle.h"
#include "math/mk_math.h"
#include "platform/fast_rw.h"
#include "platform/gcInit.h"
#include "platform/gcutils.h"
#include "platform/main.h"
#include "runtime/image.h"
#include "runtime/cstdio.h"
#include "runtime/mk_obj.h"
#include "runtime/mk_render.h"
#include "runtime/mk_proc.h"
#include "runtime/mk_plugins.h"
#include "runtime/mk_pdata.h"
#include "runtime/mk_struct.h"
#include "runtime/shadow.h"
#include "runtime/mtRand2.h"
#include "runtime/tga.h"
#include "runtime/mk_vtbl.h"
#include "runtime/utils.h"
#include "rw/rwcore_types.h"
#include "rw/rwcamera_internal.h"
#include "rw/rwdevice.h"
#include "rw/rwframe.h"
#include "rw/rphanim.h"
#include "rw/rplight.h"

extern void destroy_fade_box(void);
extern void GProfile_GCN_GxDrawDone(void);
extern RpWorld* RpClumpGetWorld(RpClump* clump);
extern RpWorld* RpWorldRemoveClump(RpWorld* world, RpClump* clump);
extern RpWorld* RpWorldAddClump(RpWorld* world, RpClump* clump);
extern int RwTextureDestroy(RwTexture* texture);
extern void RwGameCubeCameraTextureFlush(RwRaster* raster, int generate_mipmaps);
extern RwImage* RwImageCreate(int width, int height, int depth);
extern RwImage* RwImageAllocatePixels(RwImage* image);
extern RwImage* RwImageSetFromRaster(RwImage* image, RwRaster* raster);
extern int RwImageDestroy(RwImage* image);
extern void create_fade_box(void);
extern void CameraDestroy(RwCamera* camera);
extern int RpWorldDestroy(RpWorld* world);
extern void update_fog_render_states(void);
extern void init_debug_message_handler(void);
extern int select_display_device(void);
extern int fog_on;
extern int RpWorldPluginAttach(void);
extern int RpSkinPluginAttach(void);
extern int RpSpecularPluginAttach(void);
extern int specskin_plugin_attach(void);
extern int RpMatFXPluginAttach(void);
extern void debug_error_message(const char* message);
extern int __mini_game_display_ctrl;
extern int curr_pipeline_used;
extern int last_pipeline_used;
extern int uploaded_light_state;
extern int reseed_rnd_tbl;
extern void force_rw_lights(void);
extern int get_bgnd_flags(void);
extern void render_konquest_shadows(void);
extern void render_minigame_list(void);
extern void render_fgnd_mkobjs(void);
extern void insert_PFXlist_in_transl_tree(void);
extern void mkpfx_camera_begin(void);
extern void mkpfx_camera_end(void);
extern void render_collision_regions(void);
extern void mirror_guy(MkObj* source, MkObj* mirror, PlyrPdata* pdata);
extern void plyr_turn_off_mirrorguy(PlyrInfo* player);
extern void del_string_obj_by_id(int id);
static const char display_text[] =                                             \
    "/hostwrite/%03d.tga\0"                                                   \
    "Tick = %02d\0"                                                           \
    "FR = %02d\0"                                                             \
    "RpWorldPluginAttach failed.\0"                                           \
    "RpSkinPluginAttach failed.\0"                                            \
    "RpHAnimPluginAttach failed.\0"                                           \
    "RpSpecularPluginAttach failed.\0"                                        \
    "specskin_plugin_attach failed.\0"                                        \
    "RpMatFXPluginAttach failed.\0"                                           \
    "RpClumpMkobjPluginAttach failed.\0"                                      \
    "RpAtomicMksobjPluginAttach failed.\0"                                    \
    "RpMaterialMkmaterialPluginAttach failed.\0"                              \
    "RpColorSetPluginAttach failed.";

static float halt_during_screen_save(void);
static float _print_screen_to_tga(void);
static void render_konquest_sky(void);
static void render_sky(void);
static void setup_default_render_state(void);

typedef struct DisplayCameraItem {
    MkObj* object;
    unsigned int instance;
} DisplayCameraItem;

extern DisplayCameraItem camera_item;

typedef struct DisplayEngineVtable {
    void* slots_00[8];
    int (*render_state_set)(int state, int value, void* engine);
} DisplayEngineVtable;

extern DisplayEngineVtable* RwEngineInstance;

FadingScreen fading_screen = {0};
MKMATRIX camera_facing_matrix_ay;
RwRGBA background_color = {0, 0, 0, 0};
RwRGBA load_meter_bgnd_color = {0, 0, 0, 0};
int capture_num = 1;
int pal50_video_frame_dropping;
RpWorld* World;
int show_ticks;
int save_screen;
unsigned long f_render_all_atomics;
unsigned long renderware_initialized;
int display_render__pal50_frame_ctr;
static int cached_rs_src_blend;
int screen_height;
int screen_width;

const unsigned int rgba_white = 0xFFFFFFFFu;
const unsigned int rgba_red = 0xFF0000FFu;
const unsigned int rgba_green = 0x00FF00FFu;
const unsigned int rgba_blue = 0x0000FFFFu;
const unsigned int rgba_cyan = 0x00FFFFFFu;
const unsigned int rgba_yellow = 0xFFFF00FFu;

void end_first_pass_render(void) {
    f_render_all_atomics = 0;
    destroy_fade_box();
}

static inline MkObj* display_camera_live_object(DisplayCameraItem* owner) {
    MkObj* object = owner->object;
    if (object != 0) {
        if (object->hdr.instance == owner->instance) {
            return object;
        }
        object = 0;
    } else {
        object = 0;
    }
    return object;
}

void start_first_pass_render(void) {
    MkObj* camera_object = display_camera_live_object(&camera_item);

    if (camera_object != 0) {
        camera_object->pos.value.x = 0.0f;
        camera_object->pos.value.y = 0.0f;
        camera_object->pos.value.z = -50.0f;
        camera_object->ang.x = 0.0f;
        camera_object->ang.y = 0.0f;
        camera_object->ang.z = 0.0f;
        update_mkobj(camera_object);
    }
    f_render_all_atomics = 1;
    create_fade_box();
}

void DeleteCameraSnapShot(void) {
    if (fading_screen.snapshotTex != 0) {
        RwTextureDestroy(fading_screen.snapshotTex);
        fading_screen.snapshotTex = 0;
    }
    if (fading_screen.fade_obj != 0) {
        if (fading_screen.fade_obj->instance != 0) {
            fading_screen.fade_obj->vtbl->destroy();
        }
        fading_screen.fade_obj = 0;
    }
}

/* TODO: [near miss] 99.87395%; cleanup frame register differs; 5040 declaration permutations found no exact candidate. */
void TakeCameraSnapShot(void) {
    RwRaster* raster;
    RwRaster* z_raster;
    RwFrame* frame;
    RwCamera* camera;
    RwTexture* texture;
    RwCamera* saved_camera;
    RpWorld* camera_world;

    if (fading_screen.snapshotTex != 0) {
        RwTextureDestroy(fading_screen.snapshotTex);
        fading_screen.snapshotTex = 0;
    }
    do {
        raster = RwRasterCreate(0x100, 0x100, 0x20, 5);
        if (raster != 0) {
            z_raster = RwRasterCreate(0x100, 0x100, 0, 1);
            if (z_raster != 0) {
                frame = RwFrameCreate();
                if (frame != 0) {
                    camera = RwCameraCreate();
                    RwFrameTransform(
                        frame,
                        &((RwFrame*)Camera->object.object.parent)->modelling, 0);
                    if (camera != 0) {
                        camera->frameBuffer = raster;
                        camera->zBuffer = z_raster;
                        _rwObjectHasFrameSetFrame(camera, frame);
                        RwCameraSetNearClipPlane(camera, Camera->nearPlane);
                        RwCameraSetFarClipPlane(camera, Camera->farPlane);
                        RwCameraSetViewWindow(camera, &Camera->viewWindow);
                        RpWorldAddCamera(World, camera);
                        break;
                    }
                    RwFrameDestroy(frame);
                }
                RwRasterDestroy(z_raster);
            }
            RwRasterDestroy(raster);
        }
        camera = 0;
    } while (0);
    if (camera != 0) {
        texture = RwTextureCreate(camera->frameBuffer);
        if (texture != 0) {
            raster = camera->frameBuffer;
            saved_camera = Camera;
            Camera = camera;
            Render();
            RwGameCubeCameraTextureFlush(raster, 0);
            GProfile_GCN_GxDrawDone();
            Camera = saved_camera;
        }
        if (camera != 0) {
            frame = (RwFrame*)camera->object.object.parent;
            if (frame != 0) {
                _rwObjectHasFrameSetFrame(camera, 0);
                RwFrameDestroy(frame);
            }
            if (camera->zBuffer != 0) {
                z_raster = camera->zBuffer;
                camera->zBuffer = 0;
                RwRasterDestroy(z_raster);
            }
            if (camera->frameBuffer != 0) {
                camera->frameBuffer = 0;
            }
            camera_world = RwCameraGetWorld(camera);
            if (camera_world != 0) {
                RpWorldRemoveCamera(camera_world, camera);
            }
            RwCameraDestroy(camera);
        }
    } else {
        texture = 0;
    }
    fading_screen.snapshotTex = texture;
}

RpLight* destroy_light(RpLight* light, void* data) {
    RpWorld* world = data;
    RpWorldRemoveLight(world, light);
    if (light->object.object.parent != 0) {
        RwFrameDestroy((RwFrame*)light->object.object.parent);
    }
    RpLightDestroy(light);
    return light;
}

void display_shutdown(void) {
    destroy_list(&master_clean_up_list);
    pfxfont_system_shutdown();
    CameraDestroy(Camera);
    Camera = 0;
    camera_item.object = 0;
    camera_item.instance = 0;
    destroy_shadow_system();
    DeleteCameraSnapShot();
    if (World != 0) {
        RpWorldForAllLights(World, destroy_light, World);
        RpWorldDestroy(World);
        World = 0;
    }
}

static float halt_during_screen_save(void) {
    set_game_speed(0.0f);
    pause_procs(1);
    while (find_mkproc_pid(0x7005) != 0) {
        _mkproc_sleep_ticks = 1.0f;
        aproc->vtbl->sleep();
    }
    set_game_speed(1.0f);
    pause_procs(0);
    return -1.0f;
}

static float _print_screen_to_tga(void) {
    char filename[64];
    int width;
    int height;
    RwRaster* raster;
    RwRaster* z_raster;
    RwFrame* frame;
    RwCamera* camera;
    RwTexture* texture;
    RwCamera* saved_camera;
    RwImage* image;
    RpWorld* camera_world;

    if (Camera->frameBuffer != 0) {
        width = Camera->frameBuffer->width;
        height = Camera->frameBuffer->height;
        fading_screen.fade_active = 1;
        save_screen = 0;
        do {
            raster = RwRasterCreate(0x280, 0x1E0, 0x20, 5);
            if (raster != 0) {
                z_raster = RwRasterCreate(0x280, 0x1E0, 0, 1);
                if (z_raster != 0) {
                    frame = RwFrameCreate();
                    if (frame != 0) {
                        camera = RwCameraCreate();
                        RwFrameTransform(
                            frame,
                            &((RwFrame*)Camera->object.object.parent)->modelling,
                            0);
                        if (camera != 0) {
                            camera->frameBuffer = raster;
                            camera->zBuffer = z_raster;
                            _rwObjectHasFrameSetFrame(camera, frame);
                            RwCameraSetNearClipPlane(camera, Camera->nearPlane);
                            RwCameraSetFarClipPlane(camera, Camera->farPlane);
                            RwCameraSetViewWindow(camera, &Camera->viewWindow);
                            RpWorldAddCamera(World, camera);
                            break;
                        }
                        RwFrameDestroy(frame);
                    }
                    RwRasterDestroy(z_raster);
                }
                RwRasterDestroy(raster);
            }
            camera = 0;
        } while (0);
        if (camera != 0) {
            texture = RwTextureCreate(camera->frameBuffer);
            if (texture != 0) {
                raster = camera->frameBuffer;
                saved_camera = Camera;
                Camera = camera;
                Render();
                RwGameCubeCameraTextureFlush(raster, 0);
                GProfile_GCN_GxDrawDone();
                Camera = saved_camera;
            }
            if (camera != 0) {
                frame = (RwFrame*)camera->object.object.parent;
                if (frame != 0) {
                    _rwObjectHasFrameSetFrame(camera, 0);
                    RwFrameDestroy(frame);
                }
                if (camera->zBuffer != 0) {
                    z_raster = camera->zBuffer;
                    camera->zBuffer = 0;
                    RwRasterDestroy(z_raster);
                }
                if (camera->frameBuffer != 0) {
                    camera->frameBuffer = 0;
                }
                camera_world = RwCameraGetWorld(camera);
                if (camera_world != 0) {
                    RpWorldRemoveCamera(camera_world, camera);
                }
                RwCameraDestroy(camera);
            }
        } else {
            texture = 0;
        }
        raster = texture->raster;
        texture->raster = 0;
        RwTextureDestroy(texture);
        fading_screen.fade_active = 0;
        if (RwRasterLock(raster, 0, 2) == 0) {
            return -1.0f;
        }
        image = RwImageCreate(width, height, 0x20);
        if (image != 0) {
            sprintf(filename, display_text, capture_num);
            capture_num++;
            RwImageAllocatePixels(image);
            RwImageSetFromRaster(image, raster);
            RwRasterDestroy(raster);
            ImageWriteTGA(image, filename);
            RwImageDestroy(image);
        } else {
            RwRasterUnlock(raster);
        }
    }
    return -1.0f;
}

void pull_clump_from_world(RpClump* clump) {
    if (clump != 0 && RpClumpGetWorld(clump) != 0) {
        RpWorldRemoveClump(World, clump);
    }
}

int add_clump_to_world(RpWorld* world, RpClump* clump) {
    if (world != 0 && clump != 0 && RpWorldAddClump(world, clump) != 0) {
        return 1;
    }
    return 0;
}

int AttachPlugins(void) {
    if (!RpWorldPluginAttach()) {
        debug_error_message(display_text + 0x2A);
        return 0;
    }
    if (!RpSkinPluginAttach()) {
        debug_error_message(display_text + 0x46);
        return 0;
    }
    RtAnimInitialize();
    if (!RpHAnimPluginAttach()) {
        debug_error_message(display_text + 0x61);
        return 0;
    }
    if (!RpSpecularPluginAttach()) {
        debug_error_message(display_text + 0x7D);
        return 0;
    }
    if (!specskin_plugin_attach()) {
        debug_error_message(display_text + 0x9C);
        return 0;
    }
    if (!RpMatFXPluginAttach()) {
        debug_error_message(display_text + 0xBB);
        return 0;
    }
    if (!RpClumpMkobjPluginAttach()) {
        debug_error_message(display_text + 0xD7);
        return 0;
    }
    if (!RpAtomicMksobjPluginAttach()) {
        debug_error_message(display_text + 0xF8);
        return 0;
    }
    if (!RpMaterialMkmaterialPluginAttach()) {
        debug_error_message(display_text + 0x11B);
        return 0;
    }
    if (!RpColorSetPluginAttach()) {
        debug_error_message(display_text + 0x144);
        return 0;
    }
    return 1;
}

void turn_display_on(void) {
    display_off = 0;
}

void turn_display_off(void) {
    display_off = 1;
}

static inline MkObj* fighter_live_shadow_obj(FighterMirror* owner) {
    MkObj* object = owner->shadow_obj;
    if (object != 0) {
        if (object->hdr.instance == owner->shadow_obj_instance) {
            return object;
        }
        object = 0;
    } else {
        object = 0;
    }
    return object;
}

/* TODO: [near miss] 99.709680%; register coloring; one-trial ceiling. */
void Render(void) {
    MkProc* capture_proc;
    MkProc* halt_proc;
    char tick_text[80];

    gc_setup_render_mode(mode_of_play == 7);
    if (pal50_video_frame_dropping && ++display_render__pal50_frame_ctr > 5) {
        display_render__pal50_frame_ctr = 0;
        pfxsystem_frame_begin();
        pfxsystem_skip_render_frame();
        if (!g_game_info.flag_bits.high_res_path &&
            __mini_game_display_ctrl != 0) {
            render_minigame_list();
        }
    } else if ((int)display_off != 0) {
        pfxsystem_frame_begin();
        if (Camera != 0) {
            RwCameraClear(Camera, &load_meter_bgnd_color, 7);
            RwCameraBeginUpdate(Camera);
            RwCameraEndUpdate(Camera);
            RwCameraShowRaster(Camera, 0, 1);
        }
    } else {
        RwFrameOrthoNormalize((RwFrame*)Camera->object.object.parent);
        force_rw_lights();
        if (reseed_rnd_tbl != 0) {
            reload_rnd_tbl();
        }
        pfxsystem_frame_begin();
        last_pipeline_used = 0;
        curr_pipeline_used = 0;
        uploaded_light_state = 0;
        if (!g_game_info.flag_bits.high_res_path) {
            if (mode_of_play == 7 || mode_of_play == 8) {
                render_konquest_shadows();
            }
            if (get_bgnd_flags() & 1) {
                FighterMirror* fighter = g_game_info.plyr0.slot.fighter;
                if (fighter != 0 && g_game_info.plyr0.slot.mirror_a != 0 &&
                    g_game_info.plyr0.slot.mirror_b != 0 &&
                    fighter->flag_obj != 0) {
                    MkObj* shadow_obj = fighter_live_shadow_obj(fighter);

                    if (shadow_obj == 0) {
                        plyr_turn_off_mirrorguy(&g_game_info.plyr0);
                    } else if (g_game_info.plyr0.slot.mirror_b->hdr.instance != 0 &&
                               !fighter->flag_obj->hide_flag_bits.hidden) {
                        UpdateShadow(g_game_info.plyr0.slot.mirror_a,
                                     (ShadowObject*)fighter,
                                     g_game_info.plyr0.slot.mirror_b);
                        if (g_game_info.section->flags70 & 8) {
                            mirror_guy(g_game_info.plyr0.slot.mirror_a,
                                       g_game_info.plyr0.slot.mirror_b,
                                       g_game_info.plyr0.slot.pdata);
                        }
                    }
                }
                fighter = g_game_info.plyr1.slot.fighter;
                if (fighter != 0 && g_game_info.plyr1.slot.mirror_a != 0 &&
                    g_game_info.plyr1.slot.mirror_b != 0 &&
                    fighter->flag_obj != 0) {
                    MkObj* shadow_obj = fighter_live_shadow_obj(fighter);

                    if (shadow_obj == 0) {
                        plyr_turn_off_mirrorguy(&g_game_info.plyr1);
                    } else if (g_game_info.plyr1.slot.mirror_b->hdr.instance != 0 &&
                               !fighter->flag_obj->hide_flag_bits.hidden) {
                        UpdateShadow(g_game_info.plyr1.slot.mirror_a,
                                     (ShadowObject*)fighter,
                                     g_game_info.plyr1.slot.mirror_b);
                        if (g_game_info.section->flags70 & 8) {
                            mirror_guy(g_game_info.plyr1.slot.mirror_a,
                                       g_game_info.plyr1.slot.mirror_b,
                                       g_game_info.plyr1.slot.pdata);
                        }
                    }
                }
            }
            last_pipeline_used = 0;
            curr_pipeline_used = 0;
            uploaded_light_state = 0;
        }
        GXSetAlphaUpdate(1);
        if (g_game_info.flag_bits.high_res_path) {
            RwCameraClear(Camera, &load_meter_bgnd_color, 7);
        } else {
            RwCameraClear(Camera, &background_color, 7);
        }
        GXSetAlphaUpdate(0);
        if (!g_game_info.flag_bits.high_res_path && g_game_info.sky != 0) {
            if (mode_of_play == 7) {
                render_konquest_sky();
            } else {
                render_sky();
            }
        }
        RwCameraBeginUpdate(Camera);
        setup_default_render_state();
        render_startup();
        mkpfx_camera_begin();
        if (!g_game_info.flag_bits.high_res_path) {
            if (__mini_game_display_ctrl != 0) {
                render_2d_objs(1);
                render_minigame_list();
            }
            render_fgnd_mkobjs();
            insert_PFXlist_in_transl_tree();
            render_transl_atomics();
            setup_default_render_state();
        }
        RwEngineInstance->render_state_set(0xE, 0, RwEngineInstance);
        render_post_3D_effect();
        if (!g_game_info.flag_bits.high_res_path &&
            fading_screen.fade_active == 0) {
            render_collision_regions();
            if (show_ticks != 0) {
                sprintf(tick_text, display_text + 0x14, exec_tick_ctr);
            }
        } else {
            del_string_obj_by_id(0x900E);
        }
        render_2d_objs(0);
        mkpfx_camera_end();
        RwCameraEndUpdate(Camera);
        adjust_gamma();
        if (fading_screen.fade_active == 0) {
            RwCameraShowRaster(Camera, 0, 1);
        }
        if (save_screen != 0) {
            save_screen = 0;
            GProfile_GCN_GxDrawDone();
            if (find_mkproc_pid(0x7005) == 0) {
                capture_proc = _create_mkproc_generic_bigstack(
                    0x7005, 0x1F, _print_screen_to_tga, 0, 0);
                if (capture_proc != 0) {
                    capture_proc->flags_bits.skip_if_paused = 1;
                    halt_proc = proc_create(halt_during_screen_save, 0x208A);
                    if (halt_proc != 0) {
                        halt_proc->flags_bits.skip_if_paused = 1;
                    }
                }
            }
        }
    }
}

void wait_for_display_to_flush(void) {
    GProfile_GCN_GxDrawDone();
}

static void render_konquest_sky(void) {
    int saved_fog = fog_on;
    float saved_far_plane = Camera->farPlane;
    RwCameraSetFarClipPlane(Camera, 2000.0f);
    RwCameraBeginUpdate(Camera);
    fog_on = 0;
    setup_default_render_state();
    fog_on = saved_fog;
    render_mkobj(g_game_info.sky);
    render_transl_atomics();
    RwCameraEndUpdate(Camera);
    RwCameraSetFarClipPlane(Camera, saved_far_plane);
}

static void render_sky(void) {
    int saved_fog = fog_on;
    float saved_far_plane = Camera->farPlane;
    if (g_game_info.section != 0) {
        RwCameraSetFarClipPlane(Camera, g_game_info.section->far_clip);
    } else {
        RwCameraSetFarClipPlane(Camera, 2000.0f);
    }
    RwCameraBeginUpdate(Camera);
    fog_on = 0;
    setup_default_render_state();
    fog_on = saved_fog;
    RpClumpRender(g_game_info.sky->clump);
    RwCameraEndUpdate(Camera);
    RwCameraSetFarClipPlane(Camera, saved_far_plane);
}

static void setup_default_render_state(void) {
    RwRenderStateSet_rwRENDERSTATECULLMODE(2);
    RwRenderStateSet_rwRENDERSTATEZWRITEENABLE(1);
    RwRenderStateSet_rwRENDERSTATEZTESTENABLE(1);
    RwEngineInstance->render_state_set(7, 2, RwEngineInstance);
    cached_rs_src_blend = 5;
    RwRenderStateSet_SRCBLEND_DESTBLEND(5, 6);
    update_fog_render_states();
}

int set_render_state(int state, int value) {
    if (state < 10) {
        switch (state) {
        case 6:
            RwRenderStateSet_rwRENDERSTATEZTESTENABLE(value);
            return 1;
        case 8:
            RwRenderStateSet_rwRENDERSTATEZWRITEENABLE(value);
            return 1;
        case 9:
            RwRenderStateSet_rwRENDERSTATETEXTUREFILTER(value);
            return 1;
        default:
            return RwEngineInstance->render_state_set(state, value,
                                                       RwEngineInstance);
        }
    }
    switch (state) {
    case 10:
        cached_rs_src_blend = value;
        return 1;
    case 11:
        return RwRenderStateSet_SRCBLEND_DESTBLEND(cached_rs_src_blend, value);
    case 12:
        RwRenderStateSet_rwRENDERSTATEVERTEXALPHAENABLE(value);
        return 1;
    case 20:
        RwRenderStateSet_rwRENDERSTATECULLMODE(value);
        return 1;
    default:
        return RwEngineInstance->render_state_set(state, value,
                                                   RwEngineInstance);
    }
}



void update_camera_facing_matrix(void) {
    MkObj* camera_object = display_camera_live_object(&camera_item);

    if (camera_object != 0) {
        Vec angles;
        angles.z = 0.0f;
        angles.x = 0.0f;
        angles.y = 3.1415927f + camera_object->ang.y;
        if (angles.y > 6.2831855f) {
            angles.y -= 6.2831855f;
        }
        YXZ_angles_to_MKMATRIX(&angles, &camera_facing_matrix_ay);
    } else {
        camera_facing_matrix_ay.at.z = 1.0f;
        camera_facing_matrix_ay.up.y = 1.0f;
        camera_facing_matrix_ay.right.x = 1.0f;
        camera_facing_matrix_ay.up.x = 0.0f;
        camera_facing_matrix_ay.right.z = 0.0f;
        camera_facing_matrix_ay.right.y = 0.0f;
        camera_facing_matrix_ay.at.y = 0.0f;
        camera_facing_matrix_ay.at.x = 0.0f;
        camera_facing_matrix_ay.up.z = 0.0f;
        camera_facing_matrix_ay.pos.z = 0.0f;
        camera_facing_matrix_ay.pos.y = 0.0f;
        camera_facing_matrix_ay.pos.x = 0.0f;
        camera_facing_matrix_ay.flags |= 0x20003;
    }
}

int init_display(void) {
    RwEngineOpenParams parameters;
    if (!RwEngineInit(setup_memory_functions(), 0, 0x48000)) {
        return 0;
    }
    init_debug_message_handler();
    if (!AttachPlugins()) {
        return 0;
    }
    parameters.displayID = 0;
    if (!RwEngineOpen(&parameters)) {
        RwEngineTerm();
        return 0;
    }
    if (!select_display_device()) {
        return 0;
    }
    if (!RwEngineStart()) {
        RwEngineClose();
        RwEngineTerm();
        return 0;
    }
    renderware_initialized = 1;
    disable_default_filesystem();
    init_mk_render();
    return 1;
}

void set_background_color(unsigned char red, unsigned char green,
                          unsigned char blue) {
    background_color.red = red;
    background_color.green = green;
    background_color.blue = blue;
    background_color.alpha = 0xFF;
}
