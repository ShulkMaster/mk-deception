#include "runtime/mk_vtbl.h"

/*
 * These functions are owned and typed by their implementation modules. This
 * TU only stores their addresses in ABI-erased vtable slots, so keep the
 * declarations private until those owning headers are imported.
 */
int vdestroy_mkx_mem(void);
int vdestroy_mkx_rplight(void);
int vdestroy_mksobj(void);
int vdestroy_mkobj(void);
int vdestroy_screen_obj(void);
int vdestroy_string_obj(void);
int vdestroy_pebble(void);
int vdestroy_pfx(void);
int vdestroy_pfx_clone(void);
int vdestroy_mkpdata_anim(void);
int vdestroy_ani_texture_control(void);
int vdestroy_trigger_struct(void);
int vdestroy_mkpdata_plyr(void);
int vdestroy_mkpdata_camera(void);
int vdestroy_cloth_coll(void);
int vdestroy_cloth_coll_plane(void);
int vdestroy_cloth_coll_volume(void);
int vdestroy_cmdscript(void);
int vdestroy_screen_engine(void);
int vdestroy_mkpdata_generic(void);
int vdestroy_mkhdr_generic(void);

void vdestroy_mkproc_nostack(struct MkProc* proc);
void vdestroy_mkproc_tinystack(struct MkProc* proc);
void vdestroy_mkproc_bigstack(struct MkProc* proc);
void dispatch_nostack(void);
void sleep_nostack(void);
void system_stack_nostack(void);
void local_stack_nostack(void);
void jump_sleep_nostack(int return_address);
void dispatch_tinystack(void);
void sleep_tinystack(void);
void system_stack_tinystack(void);
void local_stack_tinystack(void);
void jump_sleep_tinystack(int return_address);
void dispatch_bigstack(void);
void sleep_bigstack(void);
void system_stack_bigstack(void);
void local_stack_bigstack(void);
void jump_sleep_bigstack(int return_address);

int update_mksobj(void);

int not_mkmaterial(void) {
    return 0;
}

int not_mksobj(void) {
    return 0;
}

int is_mksobj(void) {
}

int not_mkpdata(void) {
    return 0;
}

int is_mkpdata(void) {
}

int not_mkproc(void) {
    return 0;
}

int is_mkproc(void) {
}

MkVtable5 vtbl_mkx_mem = {
    not_mkproc,
    not_mkpdata,
    not_mksobj,
    not_mkmaterial,
    vdestroy_mkx_mem,
};

MkVtable5 vtbl_mkx_rplight = {
    not_mkproc,
    not_mkpdata,
    not_mksobj,
    not_mkmaterial,
    vdestroy_mkx_rplight,
};

MkVtableMkproc vtbl_mkproc_nostack = {
    is_mkproc,
    not_mkpdata,
    not_mksobj,
    not_mkmaterial,
    vdestroy_mkproc_nostack,
    dispatch_nostack,
    sleep_nostack,
    system_stack_nostack,
    local_stack_nostack,
    jump_sleep_nostack,
};

MkVtableMkproc vtbl_mkproc_tinystack = {
    is_mkproc,
    not_mkpdata,
    not_mksobj,
    not_mkmaterial,
    vdestroy_mkproc_tinystack,
    dispatch_tinystack,
    sleep_tinystack,
    system_stack_tinystack,
    local_stack_tinystack,
    jump_sleep_tinystack,
};

MkVtableMkproc vtbl_mkproc_bigstack = {
    is_mkproc,
    not_mkpdata,
    not_mksobj,
    not_mkmaterial,
    vdestroy_mkproc_bigstack,
    dispatch_bigstack,
    sleep_bigstack,
    system_stack_bigstack,
    local_stack_bigstack,
    jump_sleep_bigstack,
};

MkVtableMksobj vtbl_mksobj = {
    not_mkproc,
    not_mkpdata,
    is_mksobj,
    not_mkmaterial,
    vdestroy_mksobj,
    update_mksobj,
};

MkVtable5 vtbl_mkobj = {
    not_mkproc,
    is_mkpdata,
    not_mksobj,
    not_mkmaterial,
    vdestroy_mkobj,
};

MkVtable5 vtbl_mkpdata_screen_obj = {
    not_mkproc,
    is_mkpdata,
    not_mksobj,
    not_mkmaterial,
    vdestroy_screen_obj,
};

MkVtable5 vtbl_mkpdata_string_obj = {
    not_mkproc,
    is_mkpdata,
    not_mksobj,
    not_mkmaterial,
    vdestroy_string_obj,
};

MkVtable5 vtbl_pebble = {
    not_mkproc,
    is_mkpdata,
    not_mksobj,
    not_mkmaterial,
    vdestroy_pebble,
};

MkVtable5 vtbl_pfx = {
    not_mkproc,
    is_mkpdata,
    not_mksobj,
    not_mkmaterial,
    vdestroy_pfx,
};

MkVtable5 vtbl_pfx_clone = {
    not_mkproc,
    is_mkpdata,
    not_mksobj,
    not_mkmaterial,
    vdestroy_pfx_clone,
};

MkVtable5 vtbl_mkpdata_anim = {
    not_mkproc,
    is_mkpdata,
    not_mksobj,
    not_mkmaterial,
    vdestroy_mkpdata_anim,
};

MkVtable5 vtbl_ani_texture_control = {
    not_mkproc,
    is_mkpdata,
    not_mksobj,
    not_mkmaterial,
    vdestroy_ani_texture_control,
};

MkVtable5 vtbl_trigger_struct = {
    not_mkproc,
    is_mkpdata,
    not_mksobj,
    not_mkmaterial,
    vdestroy_trigger_struct,
};

MkVtable5 vtbl_mkpdata_plyr = {
    not_mkproc,
    is_mkpdata,
    not_mksobj,
    not_mkmaterial,
    vdestroy_mkpdata_plyr,
};

MkVtable5 vtbl_mkpdata_camera = {
    not_mkproc,
    is_mkpdata,
    not_mksobj,
    not_mkmaterial,
    vdestroy_mkpdata_camera,
};

MkVtable5 vtbl_cloth_coll = {
    not_mkproc,
    is_mkpdata,
    not_mksobj,
    not_mkmaterial,
    vdestroy_cloth_coll,
};

MkVtable5 vtbl_cloth_coll_plane = {
    not_mkproc,
    is_mkpdata,
    not_mksobj,
    not_mkmaterial,
    vdestroy_cloth_coll_plane,
};

MkVtable5 vtbl_cloth_coll_volume = {
    not_mkproc,
    is_mkpdata,
    not_mksobj,
    not_mkmaterial,
    vdestroy_cloth_coll_volume,
};

MkVtable5 vtbl_cmdscript = {
    not_mkproc,
    is_mkpdata,
    not_mksobj,
    not_mkmaterial,
    vdestroy_cmdscript,
};

MkVtable5 vtbl_screen_engine = {
    not_mkproc,
    not_mkpdata,
    not_mksobj,
    not_mkmaterial,
    vdestroy_screen_engine,
};

MkVtable5 vtbl_mkpdata_generic = {
    not_mkproc,
    is_mkpdata,
    not_mksobj,
    not_mkmaterial,
    vdestroy_mkpdata_generic,
};

MkVtable5 vtbl_mkhdr_generic = {
    not_mkproc,
    not_mkpdata,
    not_mksobj,
    not_mkmaterial,
    vdestroy_mkhdr_generic,
};
