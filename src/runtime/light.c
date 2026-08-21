#include "runtime/light.h"
#include "runtime/mk_obj.h"
#include "runtime/mk_proc.h"
#include "rw/rplight.h"
#include "rw/rwframe.h"

typedef int (*MkObjDestroyFn)(MkObj* obj);

/* Retail .data objects are 0x28 bytes each. */
typedef struct SpecularLightDef {
    int type;
    MkProcEntryFn procFn;
    int flags;
    RwRGBAReal color;
    float field1C;
    float field20;
    float field24;
} SpecularLightDef;

extern MkPtr* point_light_list;
extern MkPtr* skinned_obj_light_list;
extern MkPtr* bgnd_light_list;
extern MkPtr* bgnd_spec_light_list;
extern MkPtr* plyr_light_list;
extern MkPtr* master_clean_up_list;
extern RpWorld* World;

extern int vdestroy_mkx_rplight(MkxRpLight* light);

MkObj* get_mkobj_frame(int type, RwFrame* frame);
MkxRpLight* get_mkx_rplight(RpLight* light);
void bind_rplight_to_obj(RpLight* light, MkObj* obj);

/* Retail leaves create_mkproc's return (mkproc*) in r3. */
MkProc* _create_mkproc_generic_tinystack(int proc_id, int priority, MkProcEntryFn proc_fn,
                                         int pdata_size, LightPdata** pdata_out);

MkObj* light_obj;
LightPdata* light_pdata;

static int main_plyr_light_created;

static SpecularLightDef default_specular_light_def = {
    3, 0, 1, {0.75f, 0.75f, 0.75f, 1.0f}, 0.9f, 2.87f, 0.0f,
};

static SpecularLightDef default_bgnd_specular_light_def = {
    3, 0, 2, {0.75f, 0.75f, 0.75f, 1.0f}, 0.1f, 0.27f, 0.8f,
};

static void pre_light(void);
static void post_light(void);
static MkxRpLight* fetch_light(MkPtr** list, unsigned int type, unsigned int index);

static MkxRpLight* probe_mkx(MkHdr* hdr) {
    int ok;

    ok = 0;
    if (hdr != 0) {
        if (hdr->vtbl->destroy == (MkVtblFn)vdestroy_mkx_rplight) {
            ok = 1;
        }
    }
    if (ok == 0) {
        return 0;
    }
    return MKX_RPLIGHT_FROM_HDR(hdr);
}

static MkObj* valid_linked_obj(MkxRpLight* mkx) {
    MkObj* obj;

    obj = mkx->obj;
    if (obj == 0) {
        return 0;
    }
    if (obj->hdr.instance != mkx->obj_instance) {
        return 0;
    }
    return obj;
}

static unsigned char rp_light_type(RpLight* light) {
    return light->object.object.subType;
}

static void mkobj_or_flag(MkObj* obj, unsigned char bit) {
    obj->flags_08 = (unsigned char)(obj->flags_08 | bit);
}

static void clear_light_low_flags(RpLight* light) {
    light->object.object.flags =
        (unsigned char)(light->object.object.flags & 0xFC);
}

static void destroy_owned_mkobj(MkObj* mkobj, MkObj* parent) {
    if (mkobj == 0 || parent != 0) {
        return;
    }
    if (mkobj->hdr.instance == 0) {
        return;
    }
    ((MkObjDestroyFn)mkobj->hdr.vtbl->destroy)(mkobj);
}

int adjust_point_light_associated_with_obj_radius(float delta, MkObj* obj) {
    MkxRpLight* found;
    MkxRpLight* entry;
    RpLight* light;
    float radius;
    unsigned int index;

    found = 0;
    for (index = 0; index < 2; index++) {
        entry = fetch_light(&point_light_list, 2, index);
        if (entry == 0) {
            continue;
        }
        if (valid_linked_obj(entry) != obj) {
            continue;
        }
        found = entry;
        break;
    }
    if (found == 0) {
        return 1;
    }
    light = found->light;
    if (light == 0) {
        return 1;
    }
    radius = light->radius + delta;
    if (radius < 0.0f) {
        radius = 0.01f;
    }
    RpLightSetRadius(light, radius);
    light = found->light;
    if (light->radius < 0.05f) {
        return 1;
    }
    return 0;
}

void obj_add_to_skinned_obj_light_list_with_ambient(MkObj* obj, LightDef* def) {
    first_mkhdr(&skinned_obj_light_list);
    load_light(def, &skinned_obj_light_list, 0);
    if (obj != 0) {
        obj->light_flags = 0x400;
    }
}

void obj_change_to_bgnd_obj_light_list(MkObj* obj, LightDef* def) {
    if (first_mkhdr(&bgnd_light_list) == 0) {
        load_light(def, &bgnd_light_list, 0);
    }
    if (obj != 0) {
        obj->light_flags = 1;
    }
}

void obj_change_to_skinned_obj_light_list(MkObj* obj, LightDef* def) {
    if (first_mkhdr(&skinned_obj_light_list) == 0) {
        load_light(def, &skinned_obj_light_list, 0);
    }
    if (obj != 0) {
        obj->light_flags = 0x400;
    }
}

RpLight* create_spot_light(MkObj* parent, LightDef* def) {
    RpLight* light;
    RwFrame* frame;
    MkObj* mkobj;

    light = RpLightCreate(0x81);
    if (light == 0) {
        return 0;
    }
    RpLightSetColor(light, &def->color);
    RpLightSetConeAngle(light, def->coneAngle);
    RpLightSetRadius(light, def->spotRadius);

    if (parent != 0) {
        frame = parent->frame;
        bind_rplight_to_obj(light, parent);
        mkobj = parent;
    } else {
        frame = RwFrameCreate();
        mkobj = get_mkobj_frame(0x2009, frame);
        parent = mkobj;
    }

    if (frame == 0) {
        RpLightDestroy(light);
        return 0;
    }

    _rwObjectHasFrameSetFrame(light, frame);
    mkobj_or_flag(parent, 0x10);
    mkobj_or_flag(parent, 0x80);
    parent->light_flags = def->flags;
    parent->pos.value.x = def->field1C;
    parent->pos.value.y = def->field20;
    parent->pos.value.z = def->field24;
    parent->dir_x = def->field28;
    parent->dir_y = def->field2C;
    parent->dir_z = def->field30;
    insert_fgnd_mkobj(parent);
    update_mkobj(parent);
    RpWorldAddLight(World, light);
    return light;
}

static RpLight* find_specular_light(MkPtr** list, LightDef* def) {
    MkPtr* node;
    MkxRpLight* mkx;
    RpLight* light;

    if (load_light(def, list, 0) == 0) {
        return 0;
    }
    node = *list;
    while (node != 0) {
        mkx = probe_mkx(node->hdr);
        if (mkx != 0) {
            light = mkx->light;
            if (rp_light_type(light) == 1) {
                return light;
            }
        }
        node = next_mkptr(node);
    }
    return 0;
}

RpLight* create_default_bgnd_specular_light(void) {
    return find_specular_light(&bgnd_spec_light_list, (LightDef*)&default_bgnd_specular_light_def);
}

RpLight* create_default_specular_light(void) {
    return find_specular_light(&plyr_light_list, (LightDef*)&default_specular_light_def);
}

RpLight* get_bgnd_specular_light(void) {
    MkPtr* node;
    MkxRpLight* mkx;
    RpLight* light;

    node = bgnd_spec_light_list;
    while (node != 0) {
        mkx = probe_mkx(node->hdr);
        if (mkx != 0) {
            light = mkx->light;
            if ((int)light->object.object.subType == 1) {
                return light;
            }
        }
        node = next_mkptr(node);
    }
    return 0;
}

RpLight* get_specular_light(void) {
    MkPtr* node;
    MkxRpLight* mkx;
    RpLight* light;

    node = plyr_light_list;
    while (node != 0) {
        mkx = probe_mkx(node->hdr);
        if (mkx != 0) {
            light = mkx->light;
            if ((int)light->object.object.subType == 1) {
                return light;
            }
        }
        node = next_mkptr(node);
    }
    return 0;
}

void load_back_in_lights(LightDef** defs, MkPtr** list) {
    LightDef* def;
    MkxRpLight* entry;
    MkObj* obj;
    RpLight* light;
    int spotIndex;
    int index;

    spotIndex = 0;
    for (index = 0; index < 3;) {
        def = *defs;
        if (def == 0) {
            defs++;
            index++;
            continue;
        }
        if (def->type == 2 || def->type == 4 || def->type == 5) {
            index++;
            continue;
        }
        if (def->type == 0 || def->type >= 6) {
            defs++;
            index++;
            continue;
        }
        if (def->type == 1) {
            entry = fetch_light(list, 1, 0);
            light = entry->light;
            if (light == 0) {
                index++;
                continue;
            }
            RpLightSetColor(light, &def->color);
            if (RpLightGetWorld(light) == 0) {
                RpWorldAddLight(World, light);
            }
            defs++;
            index++;
            continue;
        }
        /* type 3 */
        entry = fetch_light(list, 3, (unsigned int)spotIndex);
        light = entry->light;
        if (light == 0) {
            index++;
            continue;
        }
        spotIndex++;
        RpLightSetColor(light, &def->color);
        if (RpLightGetWorld(light) == 0) {
            RpWorldAddLight(World, light);
        }
        obj = valid_linked_obj(entry);
        if (obj != 0) {
            obj->dir_x = def->field1C;
            obj->dir_y = def->field20;
            obj->dir_z = def->field24;
            update_mkobj(obj);
        }
        defs++;
        index++;
    }
}

static MkxRpLight* fetch_light(MkPtr** list, unsigned int type, unsigned int index) {
    MkPtr* node;
    MkPtr* next;
    MkxRpLight* mkx;
    MkHdr* hdr;
    int ok;
    int lightType;

    if (list != 0) {
        node = *list;
        while (node != 0) {
            hdr = node->hdr;
            if (node->instance != hdr->instance) {
                next = node->next;
                node->hdr = 0;
                destroy_mkptr(node);
                node = next;
            } else {
                ok = 0;
                if (hdr != 0) {
                    if (hdr->vtbl->destroy == (MkVtblFn)vdestroy_mkx_rplight) {
                        ok = 1;
                    }
                }
                if (ok != 0) {
                    mkx = MKX_RPLIGHT_FROM_HDR(hdr);
                } else {
                    mkx = 0;
                }
                if (mkx != 0) {
                    lightType = (int)mkx->light->object.object.subType;
                    switch (lightType) {
                    case 0x80:
                        if (type == 2) {
                            if (index-- == 0) {
                                return mkx;
                            }
                        }
                        break;
                    case 2:
                        if (type == 1) {
                            if (index-- == 0) {
                                return mkx;
                            }
                        }
                        break;
                    case 1:
                        if (type == 3) {
                            if (index-- == 0) {
                                return mkx;
                            }
                        }
                        break;
                    case 0x81:
                        if (type == 4) {
                            if (index-- == 0) {
                                return mkx;
                            }
                        }
                        break;
                    case 0x82:
                        if (type == 5) {
                            if (index-- == 0) {
                                return mkx;
                            }
                        }
                        break;
                    }
                }
                node = node->next;
            }
        }
    }
    return 0;
}

void clear_all_lights_in(MkPtr** list) {
    MkPtr* node;
    MkPtr* next;
    MkxRpLight* mkx;
    RpLight* light;

    if (list == 0) {
        return;
    }
    node = *list;
    while (node != 0) {
        mkx = MKX_RPLIGHT_FROM_HDR(node->hdr);
        if (node->instance != mkx->hdr.instance) {
            next = node->next;
            node->hdr = 0;
            destroy_mkptr(node);
            node = next;
            continue;
        }
        mkx = probe_mkx(node->hdr);
        if (mkx != 0) {
            light = mkx->light;
            if (light != 0 && RpLightGetWorld(light) != 0) {
                RpWorldRemoveLight(World, light);
            }
        }
        node = node->next;
    }
}

void load_lights(LightDef** defs, MkPtr** list) {
    int index;

    main_plyr_light_created = 0;
    index = 0;
    while (index < 3) {
        if (defs[0] != 0) {
            load_light(defs[0], list, 0);
        }
        index += 1;
        defs += 1;
    }
}

static RpLight* create_type5_spot(MkObj* parent, LightDef* def) {
    RpLight* light;
    RwFrame* frame;
    MkObj* mkobj;

    light = RpLightCreate(0x82);
    if (light == 0) {
        return 0;
    }
    RpLightSetColor(light, &def->color);
    RpLightSetConeAngle(light, def->coneAngle);
    RpLightSetRadius(light, def->spotRadius);

    if (parent != 0) {
        frame = parent->frame;
        mkobj = parent;
    } else {
        frame = RwFrameCreate();
        if (frame == 0) {
            RpLightDestroy(light);
            return 0;
        }
        mkobj = get_mkobj_frame(0x200A, frame);
    }

    _rwObjectHasFrameSetFrame(light, frame);
    mkobj_or_flag(mkobj, 0x10);
    mkobj_or_flag(mkobj, 0x80);
    mkobj->light_flags = def->flags;
    mkobj->pos.value.x = def->field1C;
    mkobj->pos.value.y = def->field20;
    mkobj->pos.value.z = def->field24;
    mkobj->dir_x = def->field28;
    mkobj->dir_y = def->field2C;
    mkobj->dir_z = def->field30;
    insert_fgnd_mkobj(mkobj);
    update_mkobj(mkobj);
    RpWorldAddLight(World, light);
    return light;
}

MkObj* load_light(LightDef* def, MkPtr** list, MkObj* parent) {
    RpLight* light;
    RwFrame* frame;
    MkObj* mkobj;
    MkxRpLight* mkx;
    MkPtr* node;
    MkPtr* next;
    MkxRpLight* headMkx;
    MkObj* linked;
    LightPdata* lp;
    MkProc* mkproc;
    int count;
    int procId;
    int ok;

    light = 0;
    frame = 0;
    mkobj = parent;
    ok = 0;

    switch (def->type) {
    case 1:
        light = RpLightCreate(2);
        if (light == 0) {
            break;
        }
        RpLightSetColor(light, &def->color);
        RpWorldAddLight(World, light);
        ok = 1;
        break;

    case 2:
        count = 0;
        if (list != 0) {
            node = *list;
            while (node != 0) {
                mkx = MKX_RPLIGHT_FROM_HDR(node->hdr);
                if (node->instance != mkx->hdr.instance) {
                    next = node->next;
                    node->hdr = 0;
                    destroy_mkptr(node);
                    node = next;
                } else {
                    node = node->next;
                    count++;
                }
            }
        }
        if (count > 1) {
            headMkx = MKX_RPLIGHT_FROM_HDR(point_light_list->hdr);
            linked = headMkx->obj;
            if (linked != 0) {
                if (linked->hdr.instance != headMkx->obj_instance) {
                    linked = 0;
                }
            } else {
                linked = 0;
            }
            if (linked != 0) {
                if (linked->hdr.instance != 0) {
                    ((MkObjDestroyFn)linked->hdr.vtbl->destroy)(linked);
                }
                headMkx->obj = 0;
                headMkx->obj_instance = 0;
            }
        }
        if (parent == 0) {
            frame = RwFrameCreate();
            if (frame == 0) {
                break;
            }
            mkobj = get_mkobj_frame(0x2001, frame);
            if (mkobj == 0) {
                break;
            }
        } else {
            frame = parent->frame;
        }
        light = RpLightCreate(0x80);
        if (light == 0) {
            break;
        }
        _rwObjectHasFrameSetFrame(light, frame);
        RpLightSetColor(light, &def->color);
        RpLightSetRadius(light, def->field1C);
        RpWorldAddLight(World, light);
        mkobj->light_flags = def->flags;
        mkobj_or_flag(mkobj, 0x40);
        if (parent == 0) {
            mkobj->pos.value.x = def->field20;
            mkobj->pos.value.y = def->field24;
            mkobj->pos.value.z = def->field28;
            insert_fgnd_mkobj(mkobj);
            update_mkobj(mkobj);
        }
        ok = 1;
        break;

    case 3:
        if (list == &bgnd_spec_light_list) {
            procId = 0x2008;
        } else if (list == &plyr_light_list) {
            if (main_plyr_light_created != 0) {
                procId = 0x2006;
            } else {
                procId = 0x2005;
                main_plyr_light_created = 1;
            }
        } else if (list == &bgnd_light_list) {
            procId = 0x2007;
        } else {
            procId = 0x2002;
        }
        if (parent == 0) {
            frame = RwFrameCreate();
            if (frame == 0) {
                break;
            }
            mkobj = get_mkobj_frame(procId, frame);
            if (mkobj == 0) {
                break;
            }
        } else {
            frame = parent->frame;
        }
        light = RpLightCreate(1);
        if (light == 0) {
            break;
        }
        _rwObjectHasFrameSetFrame(light, frame);
        RpLightSetColor(light, &def->color);
        RpWorldAddLight(World, light);
        mkobj->light_flags = def->flags;
        mkobj_or_flag(mkobj, 0x40);
        mkobj_or_flag(mkobj, 0x08);
        if (parent == 0) {
            mkobj->dir_x = def->field1C;
            mkobj->dir_y = def->field20;
            mkobj->dir_z = def->field24;
            insert_fgnd_mkobj(mkobj);
            update_mkobj(mkobj);
        }
        ok = 1;
        break;

    case 4:
        light = create_spot_light(parent, def);
        if (light != 0) {
            ok = 1;
        }
        break;

    case 5:
        light = create_type5_spot(parent, def);
        if (light != 0) {
            ok = 1;
        }
        break;

    default:
        break;
    }

    if (ok == 0) {
        destroy_owned_mkobj(mkobj, parent);
        mkobj = 0;
        if (frame != 0) {
            RwFrameDestroy(frame);
        }
        return 0;
    }

    clear_light_low_flags(light);
    if (def->procFn != 0) {
        lp = 0;
        mkproc = _create_mkproc_generic_tinystack(0x5009, 0x28, def->procFn, 0x14, &lp);
        if (mkproc != 0) {
            lp->light = light;
            mkproc->pre_destroy = pre_light;
            mkproc->destroy_cb = post_light;
            if (mkobj == 0) {
                mkobj = get_mkobj_frame(0x2003, 0);
                if (mkobj == 0) {
                    if (frame != 0) {
                        RwFrameDestroy(frame);
                    }
                    return 0;
                }
                lp->obj = 0;
                lp->obj_instance = 0;
            }
            lp->obj = mkobj;
            lp->obj_instance = mkobj->hdr.instance;
        }
    }

    mkx = get_mkx_rplight(light);
    mk_append(&mkx->hdr, list);
    if (mkobj != 0) {
        if (mkx != 0) {
            mk_insert(&mkx->hdr, &mkobj->child_list);
            mkx->obj = mkobj;
            mkx->obj_instance = mkobj->hdr.instance;
        } else {
            bind_rplight_to_obj(light, mkobj);
        }
    } else if (mkx != 0) {
        mk_insert(&mkx->hdr, &master_clean_up_list);
    }
    return mkobj;
}

static void post_light(void) {
    light_pdata = 0;
    light_obj = 0;
}

static void pre_light(void) {
    LightPdata* pd;
    MkObj* obj;

    pd = LIGHT_PDATA_FROM_HDR(apdata);
    light_pdata = pd;
    obj = pd->obj;
    if (obj != 0) {
        if (obj->hdr.instance != pd->obj_instance) {
            obj = 0;
        }
    } else {
        obj = 0;
    }
    light_obj = obj;
}
