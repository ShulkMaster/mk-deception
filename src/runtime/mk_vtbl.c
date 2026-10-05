#include "runtime/mk_vtbl.h"
#include "runtime/mk_struct.h"
#include "runtime/mk_proc.h"
#include "runtime/mk_obj.h"
#include "runtime/light.h"
#include "runtime/image.h"
#include "runtime/fonts.h"
#include "runtime/mk_pebble.h"
#include "runtime/mk_particle.h"
#include "runtime/anim_pdata.h"
#include "runtime/mk_cmdscript.h"
#include "runtime/mk_pdata.h"
#include "runtime/cam.h"
#include "game/plyr.h"
#include "game/cloth.h"
#include "game/konquest.h"
#include "mw/mwScreenEngineGlue.h"

int not_mkmaterial(void) {
    return 0;
}

struct MkHdr* not_mksobj(struct MkHdr* hdr) {
    return 0;
}

struct MkHdr* is_mksobj(struct MkHdr* hdr) { return hdr; }

struct MkHdr* not_mkpdata(struct MkHdr* hdr) {
    return 0;
}

struct MkHdr* is_mkpdata(struct MkHdr* hdr) { return hdr; }

struct MkHdr* not_mkproc(struct MkHdr* hdr) {
    return 0;
}

struct MkHdr* is_mkproc(struct MkHdr* hdr) { return hdr; }

MkVtableMkxMem vtbl_mkx_mem = {
    not_mkproc,
    not_mkpdata,
    not_mksobj,
    not_mkmaterial,
    vdestroy_mkx_mem,
};

MkVtableMkxRpLight vtbl_mkx_rplight = {
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

MkVtableMkobj vtbl_mkobj = {
    not_mkproc,
    is_mkpdata,
    not_mksobj,
    not_mkmaterial,
    vdestroy_mkobj,
};

ScreenObjVtable vtbl_mkpdata_screen_obj = {
    not_mkproc,
    is_mkpdata,
    not_mksobj,
    not_mkmaterial,
    vdestroy_screen_obj,
};

StringObjVtable vtbl_mkpdata_string_obj = {
    not_mkproc,
    is_mkpdata,
    not_mksobj,
    not_mkmaterial,
    vdestroy_string_obj,
};

MkVtablePebble vtbl_pebble = {
    not_mkproc,
    is_mkpdata,
    not_mksobj,
    not_mkmaterial,
    vdestroy_pebble,
};

MkVtablePfx vtbl_pfx = {
    not_mkproc,
    is_mkpdata,
    not_mksobj,
    not_mkmaterial,
    vdestroy_pfx,
};

MkVtablePfxClone vtbl_pfx_clone = {
    not_mkproc,
    is_mkpdata,
    not_mksobj,
    not_mkmaterial,
    vdestroy_pfx_clone,
};

MkVtableAnim vtbl_mkpdata_anim = {
    not_mkproc,
    is_mkpdata,
    not_mksobj,
    not_mkmaterial,
    vdestroy_mkpdata_anim,
};

MkVtableAniTextureControl vtbl_ani_texture_control = {
    not_mkproc,
    is_mkpdata,
    not_mksobj,
    not_mkmaterial,
    vdestroy_ani_texture_control,
};

MkVtableTrigger vtbl_trigger_struct = {
    not_mkproc,
    is_mkpdata,
    not_mksobj,
    not_mkmaterial,
    vdestroy_trigger_struct,
};

MkVtablePlyr vtbl_mkpdata_plyr = {
    not_mkproc,
    is_mkpdata,
    not_mksobj,
    not_mkmaterial,
    vdestroy_mkpdata_plyr,
};

MkHdrVtable vtbl_mkpdata_camera = {
    not_mkproc,
    is_mkpdata,
    not_mksobj,
    not_mkmaterial,
    vdestroy_mkpdata_camera,
};

MkHdrVtable vtbl_cloth_coll = {
    not_mkproc,
    is_mkpdata,
    not_mksobj,
    not_mkmaterial,
    vdestroy_cloth_coll,
};

MkHdrVtable vtbl_cloth_coll_plane = {
    not_mkproc,
    is_mkpdata,
    not_mksobj,
    not_mkmaterial,
    vdestroy_cloth_coll_plane,
};

MkHdrVtable vtbl_cloth_coll_volume = {
    not_mkproc,
    is_mkpdata,
    not_mksobj,
    not_mkmaterial,
    vdestroy_cloth_coll_volume,
};

MkVtableCmdscript vtbl_cmdscript = {
    not_mkproc,
    is_mkpdata,
    not_mksobj,
    not_mkmaterial,
    vdestroy_cmdscript,
};

MkHdrVtable vtbl_screen_engine = {
    not_mkproc,
    not_mkpdata,
    not_mksobj,
    not_mkmaterial,
    vdestroy_screen_engine,
};

MkHdrVtable vtbl_mkpdata_generic = {
    not_mkproc,
    is_mkpdata,
    not_mksobj,
    not_mkmaterial,
    vdestroy_mkpdata_generic,
};

MkHdrVtable vtbl_mkhdr_generic = {
    not_mkproc,
    not_mkpdata,
    not_mksobj,
    not_mkmaterial,
    vdestroy_mkhdr_generic,
};
