#ifndef MK_VTBL_H
#define MK_VTBL_H

#ifndef MK_VTABLE5_TYPE
#define MK_VTABLE5_TYPE
typedef int (*MkVtblFn)(void);
struct MkProc;
struct MkSobj;
typedef void (*MkProcDestroyFn)(struct MkProc* proc);
typedef void (*MkSobjFn)(struct MkSobj* sobj);
typedef void (*MkProcFn)(void);
typedef void (*MkProcJumpFn)(float (*entry)(void), float ticks);

typedef struct MkVtable5 {
    MkVtblFn fn0;
    MkVtblFn fn1;
    MkVtblFn fn2;
    MkVtblFn fn3;
    MkVtblFn destroy;
} MkVtable5;
#endif

typedef struct MkVtableMkproc {
    MkVtblFn fn0;
    MkVtblFn fn1;
    MkVtblFn fn2;
    MkVtblFn fn3;
    MkProcDestroyFn destroy;
    MkProcFn dispatch;
    MkProcFn sleep;
    MkProcFn system_stack;
    MkProcFn local_stack;
    MkProcJumpFn jump_sleep;
} MkVtableMkproc;

typedef struct MkVtableMksobj {
    MkVtblFn fn0;
    MkVtblFn fn1;
    MkVtblFn fn2;
    MkVtblFn fn3;
    MkSobjFn destroy;
    MkSobjFn update;
} MkVtableMksobj;

int not_mkmaterial(void);
int not_mksobj(void);
int is_mksobj(void);
int not_mkpdata(void);
int is_mkpdata(void);
int not_mkproc(void);
int is_mkproc(void);

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

#endif
