#ifndef MKD_BGND_TYPES_H
#define MKD_BGND_TYPES_H

/*
 * Background load / data-table types (get_data_table -> GameInfo.section).
 * Offsets from load_background / bgnd_anim_camera_* (Ghidra + bgnd.c).
 */

typedef struct BgndMisc {
    char pad00[0x48];
    void* lights_bgnd;     /* +0x48 */
    void* lights_spec;     /* +0x4C */
    void* lights_plyr;     /* +0x50 */
    float shadow_strength; /* +0x54 */
    char shadow_cam_light; /* +0x58 - address taken for UpdateShadowCameraLightSource */
    char pad59[0xB];
    void* script; /* +0x64 */
    float mirror_plane_offset; /* +0x68 */
} BgndMisc;

typedef struct BgndObstacleData BgndObstacleData;

/*
 * Per-arena data table from cmdscript (retail get_data_table).
 * display uses flags70 + far_clip @ +0x90; load_background uses fog/clip head.
 */
typedef struct BgndDataTable {
    char* art_name;     /* +0x00 */
    float near_clip;    /* +0x04 - RwCameraSetNearClipPlane */
    float far_clip_cam; /* +0x08 - RwCameraSetFarClipPlane in load_background */
    float fog_r;        /* +0x0C */
    float fog_g;        /* +0x10 */
    float fog_b;        /* +0x14 */
    float fog_a;        /* +0x18 */
    float fog_density;  /* +0x1C */
    float fog_distance; /* +0x20 */
    int fog_enable;     /* +0x24 */
    float bg_r;         /* +0x28 */
    float bg_g;         /* +0x2C */
    float bg_b;         /* +0x30 */
    float bg_a;         /* +0x34 */
    int sound_bank_0;                    /* +0x38 */
    int sound_bank_1;                    /* +0x3C */
    int sound_bank_2;                    /* +0x40 */
    int sound_bank_3;                    /* +0x44 */
    int music_id_round_0_1;             /* +0x48 */
    int music_id_round_2_plus;          /* +0x4C */
    int secondary_music_id;             /* +0x50 */
    int finish_music_id;                /* +0x54 */
    int end_music_id;                   /* +0x58 */
    void (*start_music_callback)(void); /* +0x5C */
    void (*end_music_callback)(void);   /* +0x60 */
    void* load_script;                  /* +0x64 - post-load cmdscript */
    char pad68[8];
    unsigned int flags70; /* +0x70 - bit0 shadow cam light */
    BgndObstacleData* obstacle_data; /* +0x74 - arena constrain/collision definitions */
    char pad78[0x10];
    unsigned int flags88; /* +0x88 - bit0 early-out / locked */
    char* sky_name;       /* +0x8C */
    float far_clip;       /* +0x90 - display far plane */
    char pad94[4];
    void* anims;              /* +0x98 */
    int* effect_banks;        /* +0x9C - 0-terminated bank ids */
    void* cam_setup_script;   /* +0xA0 */
    void* cam_ended_script;   /* +0xA4 */
    BgndMisc* misc;           /* +0xA8 */
} BgndDataTable;

/* Legacy name used by display / older call sites. */
typedef BgndDataTable ArtSection;

typedef struct GlobalBackgroundEntry {
    void* ssf_entry; /* +0x00 */
    char* script_name; /* +0x04 */
    int field8;        /* +0x08 */
    unsigned int flags; /* +0x0C - bit3 konquest / bit4 krypt mode gates */
} GlobalBackgroundEntry; /* stride 0x10 */

#endif
