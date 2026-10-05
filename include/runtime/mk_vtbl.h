#ifndef MK_VTBL_H
#define MK_VTBL_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MK_VTABLE5_TYPE
#define MK_VTABLE5_TYPE
typedef int (*MkVtblFn)(void);
struct MkHdr;
struct MkProc;
struct MkSobj;
typedef struct MkHdr* (*MkVtableCastFn)(struct MkHdr* hdr);
typedef void (*MkProcDestroyFn)(struct MkProc* proc);
typedef void (*MkSobjFn)(struct MkSobj* sobj);
typedef void (*MkProcFn)(void);
typedef void (*MkProcJumpFn)(float (*entry)(void), float ticks);

typedef struct MkVtable5 {
    MkVtableCastFn fn0;
    MkVtableCastFn fn1;
    MkVtableCastFn fn2;
    MkVtblFn fn3;
    MkVtblFn destroy;
} MkVtable5;
#endif

struct MkxMem;
#define MK_VTABLE_ADDRESS(table) ((void*)&(table))

typedef struct MkVtableMkxMem {
    MkVtableCastFn fn0;
    MkVtableCastFn fn1;
    MkVtableCastFn fn2;
    MkVtblFn fn3;
    void (*destroy)(struct MkxMem* owner);
} MkVtableMkxMem;
typedef char check_MkVtableMkxMem_size[(sizeof(MkVtableMkxMem) == sizeof(MkVtable5)) ? 1 : -1];

struct MkxRpLight;
typedef struct MkVtableMkxRpLight {
    MkVtableCastFn fn0;
    MkVtableCastFn fn1;
    MkVtableCastFn fn2;
    MkVtblFn fn3;
    void (*destroy)(struct MkxRpLight* owner);
} MkVtableMkxRpLight;
typedef char check_MkVtableMkxRpLight_size[(sizeof(MkVtableMkxRpLight) == sizeof(MkVtable5)) ? 1 : -1];

struct MkObj;
typedef struct MkVtableMkobj {
    MkVtableCastFn fn0;
    MkVtableCastFn fn1;
    MkVtableCastFn fn2;
    MkVtblFn fn3;
    void (*destroy)(struct MkObj* owner);
} MkVtableMkobj;
typedef char check_MkVtableMkobj_size[(sizeof(MkVtableMkobj) == sizeof(MkVtable5)) ? 1 : -1];

struct PebbleData;
typedef struct MkVtablePebble {
    MkVtableCastFn fn0;
    MkVtableCastFn fn1;
    MkVtableCastFn fn2;
    MkVtblFn fn3;
    void (*destroy)(struct PebbleData* owner);
} MkVtablePebble;
typedef char check_MkVtablePebble_size[(sizeof(MkVtablePebble) == sizeof(MkVtable5)) ? 1 : -1];

struct MkPfx;
typedef struct MkVtablePfx {
    MkVtableCastFn fn0;
    MkVtableCastFn fn1;
    MkVtableCastFn fn2;
    MkVtblFn fn3;
    void (*destroy)(struct MkPfx* owner);
} MkVtablePfx;
typedef char check_MkVtablePfx_size[(sizeof(MkVtablePfx) == sizeof(MkVtable5)) ? 1 : -1];

struct PfxClone;
typedef struct MkVtablePfxClone {
    MkVtableCastFn fn0;
    MkVtableCastFn fn1;
    MkVtableCastFn fn2;
    MkVtblFn fn3;
    void (*destroy)(struct PfxClone* owner);
} MkVtablePfxClone;
typedef char check_MkVtablePfxClone_size[(sizeof(MkVtablePfxClone) == sizeof(MkVtable5)) ? 1 : -1];

struct AnimPdata;
typedef struct MkVtableAnim {
    MkVtableCastFn fn0;
    MkVtableCastFn fn1;
    MkVtableCastFn fn2;
    MkVtblFn fn3;
    void (*destroy)(struct AnimPdata* owner);
} MkVtableAnim;
typedef char check_MkVtableAnim_size[(sizeof(MkVtableAnim) == sizeof(MkVtable5)) ? 1 : -1];

struct AniTextureControl;
typedef struct MkVtableAniTextureControl {
    MkVtableCastFn fn0;
    MkVtableCastFn fn1;
    MkVtableCastFn fn2;
    MkVtblFn fn3;
    void (*destroy)(struct AniTextureControl* owner);
} MkVtableAniTextureControl;
typedef char check_MkVtableAniTextureControl_size[(sizeof(MkVtableAniTextureControl) == sizeof(MkVtable5)) ? 1 : -1];

struct KonquestTriggerStruct;
typedef struct MkVtableTrigger {
    MkVtableCastFn fn0;
    MkVtableCastFn fn1;
    MkVtableCastFn fn2;
    MkVtblFn fn3;
    void (*destroy)(struct KonquestTriggerStruct* owner);
} MkVtableTrigger;
typedef char check_MkVtableTrigger_size[(sizeof(MkVtableTrigger) == sizeof(MkVtable5)) ? 1 : -1];

struct PlyrPdata;
typedef struct MkVtablePlyr {
    MkVtableCastFn fn0;
    MkVtableCastFn fn1;
    MkVtableCastFn fn2;
    MkVtblFn fn3;
    void (*destroy)(struct PlyrPdata* owner);
} MkVtablePlyr;
typedef char check_MkVtablePlyr_size[(sizeof(MkVtablePlyr) == sizeof(MkVtable5)) ? 1 : -1];

struct CmdScript;
typedef struct MkVtableCmdscript {
    MkVtableCastFn fn0;
    MkVtableCastFn fn1;
    MkVtableCastFn fn2;
    MkVtblFn fn3;
    void (*destroy)(struct CmdScript* owner);
} MkVtableCmdscript;
typedef char check_MkVtableCmdscript_size[(sizeof(MkVtableCmdscript) == sizeof(MkVtable5)) ? 1 : -1];

typedef struct MkVtableMkproc {
    MkVtableCastFn fn0;
    MkVtableCastFn fn1;
    MkVtableCastFn fn2;
    MkVtblFn fn3;
    MkProcDestroyFn destroy;
    MkProcFn dispatch;
    MkProcFn sleep;
    MkProcFn system_stack;
    MkProcFn local_stack;
    MkProcJumpFn jump_sleep;
} MkVtableMkproc;

typedef struct MkVtableMksobj {
    MkVtableCastFn fn0;
    MkVtableCastFn fn1;
    MkVtableCastFn fn2;
    MkVtblFn fn3;
    MkSobjFn destroy;
    MkSobjFn update;
} MkVtableMksobj;

int not_mkmaterial(void);
struct MkHdr* not_mksobj(struct MkHdr* hdr);
struct MkHdr* is_mksobj(struct MkHdr* hdr);
struct MkHdr* not_mkpdata(struct MkHdr* hdr);
struct MkHdr* is_mkpdata(struct MkHdr* hdr);
struct MkHdr* not_mkproc(struct MkHdr* hdr);
struct MkHdr* is_mkproc(struct MkHdr* hdr);

extern MkVtableMkxMem vtbl_mkx_mem;
extern MkVtableMkxRpLight vtbl_mkx_rplight;
extern MkVtableMkproc vtbl_mkproc_nostack;
extern MkVtableMkproc vtbl_mkproc_tinystack;
extern MkVtableMkproc vtbl_mkproc_bigstack;
extern MkVtableMksobj vtbl_mksobj;
extern MkVtableMkobj vtbl_mkobj;
extern struct ScreenObjVtable vtbl_mkpdata_screen_obj;
extern struct StringObjVtable vtbl_mkpdata_string_obj;
extern MkVtablePebble vtbl_pebble;
extern MkVtablePfx vtbl_pfx;
extern MkVtablePfxClone vtbl_pfx_clone;
extern MkVtableAnim vtbl_mkpdata_anim;
extern MkVtableAniTextureControl vtbl_ani_texture_control;
extern MkVtableTrigger vtbl_trigger_struct;
extern MkVtablePlyr vtbl_mkpdata_plyr;
extern struct MkHdrVtable vtbl_mkpdata_camera;
extern struct MkHdrVtable vtbl_cloth_coll;
extern struct MkHdrVtable vtbl_cloth_coll_plane;
extern struct MkHdrVtable vtbl_cloth_coll_volume;
extern MkVtableCmdscript vtbl_cmdscript;
extern struct MkHdrVtable vtbl_screen_engine;
extern struct MkHdrVtable vtbl_mkpdata_generic;
struct MkHdrVtable;
extern struct MkHdrVtable vtbl_mkhdr_generic;

#ifdef __cplusplus
}
#endif

#endif
