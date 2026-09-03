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

extern MkVtable5 vtbl_mkx_mem;
extern MkVtable5 vtbl_mkx_rplight;
extern MkVtableMkproc vtbl_mkproc_nostack;
extern MkVtableMkproc vtbl_mkproc_tinystack;
extern MkVtableMkproc vtbl_mkproc_bigstack;
extern MkVtableMksobj vtbl_mksobj;
extern MkVtable5 vtbl_mkobj;
extern MkVtable5 vtbl_mkpdata_screen_obj;
extern MkVtable5 vtbl_mkpdata_string_obj;
extern MkVtable5 vtbl_pebble;
extern MkVtable5 vtbl_pfx;
extern MkVtable5 vtbl_pfx_clone;
extern MkVtable5 vtbl_mkpdata_anim;
extern MkVtable5 vtbl_ani_texture_control;
extern MkVtable5 vtbl_trigger_struct;
extern MkVtable5 vtbl_mkpdata_plyr;
extern MkVtable5 vtbl_mkpdata_camera;
extern MkVtable5 vtbl_cloth_coll;
extern MkVtable5 vtbl_cloth_coll_plane;
extern MkVtable5 vtbl_cloth_coll_volume;
extern MkVtable5 vtbl_cmdscript;
extern MkVtable5 vtbl_screen_engine;
extern MkVtable5 vtbl_mkpdata_generic;
extern MkVtable5 vtbl_mkhdr_generic;

#ifdef __cplusplus
}
#endif

#endif
