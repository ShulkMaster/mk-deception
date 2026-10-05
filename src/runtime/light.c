#include "runtime/light.h"
#include "runtime/mk_obj.h"
#include "runtime/mk_pdata.h"
#include "runtime/mk_proc.h"
#include "runtime/mk_struct.h"
#include "platform/display.h"
#include "rw/rplight.h"
#include "rw/rwframe.h"

struct SpecularLightDef {
    int type;
    MkProcEntryFn procFn;
    int flags;
    RwRGBAReal color;
    float field1C;
    float field20;
    float field24;
};

LightPdata* light_pdata;
MkObj* light_obj;

static int main_plyr_light_created;

static struct SpecularLightDef default_specular_light_def = {
    3, 0, 1, {0.75f, 0.75f, 0.75f, 1.0f}, 0.9f, 2.87f, 0.0f,
};

static struct SpecularLightDef default_bgnd_specular_light_def = {
    3, 0, 2, {0.75f, 0.75f, 0.75f, 1.0f}, 0.1f, 0.27f, 0.8f,
};

static void pre_light(void);
static void post_light(void);
static MkxRpLight* fetch_light(MkPtr** list, unsigned int type, unsigned int index);

static inline MkxRpLight* probe_mkx(MkHdr* hdr) {
    int ok;

    ok = 0;
    if (hdr != 0) {
        if (hdr->light_vtbl->destroy == vdestroy_mkx_rplight) {
            ok = 1;
        }
    }
    if (ok != 0) {
        return MKX_RPLIGHT_FROM_HDR(hdr);
    }
    return 0;
}

static inline MkObj* valid_linked_obj(MkxRpLight* mkx) {
    MkObj* obj = mkx->obj;

    if (obj != 0) {
        obj = obj->hdr.instance == mkx->obj_instance ? obj : 0;
    } else {
        obj = 0;
    }
    return obj;
}

static inline unsigned char rp_light_type(RpLight* light) {
    return light->object.object.subType;
}

static inline void mkobj_or_flag(MkObj* obj, unsigned char bit) {
    obj->flags_08 = obj->flags_08 | bit;
}

static inline void clear_light_low_flags(RpLight* light) {
    light->object.object.flags =
        light->object.object.flags & 0xFC;
}

/* TODO: [breakthrough] 83.45%; linked-object latch matches; search loop and return/frame CFG remain. */
int adjust_point_light_associated_with_obj_radius(MkObj* obj, float delta) {
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

/* TODO: [breakthrough needed] 81.79487%; frame and light-owner CFG differ;
 * recover retail creation and lifetime boundaries. */
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

static inline RpLight* find_specular_light(MkPtr** list, LightDef* def) {
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

/* TODO: [near miss] 85.11%; positive validated-hdr selection restored;
 * loop/owner staging and GPR scheduling remain. */
RpLight* create_default_bgnd_specular_light(void) {
    return find_specular_light(&bgnd_spec_light_list, (LightDef*)&default_bgnd_specular_light_def);
}

/* TODO: [near miss] 85.11%; positive validated-hdr selection restored;
 * loop/owner staging and GPR scheduling remain. */
RpLight* create_default_specular_light(void) {
    return find_specular_light(&plyr_light_list, (LightDef*)&default_specular_light_def);
}

RpLight* get_bgnd_specular_light(void) {
    MkPtr* node = bgnd_spec_light_list;
    MkxRpLight* mkx;
    RpLight* light;
    MkHdr* hdr;

    while (node != 0) {
        hdr = node->hdr;
        mkx = probe_mkx(hdr);
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
    MkPtr* node = plyr_light_list;
    MkxRpLight* mkx;
    RpLight* light;
    MkHdr* hdr;

    while (node != 0) {
        hdr = node->hdr;
        mkx = probe_mkx(hdr);
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

static inline void restore_light_world(RpLight* light, const LightDef* def) {
    RpLightSetColor(light, &def->color);
    if (RpLightGetWorld(light) == 0) {
        RpWorldAddLight(World, light);
    }
}

void load_back_in_lights(LightDef** defs, MkPtr** list) {
    const LightDef* def;
    MkxRpLight* entry;
    RpLight* spot_light;
    MkObj* obj;
    RpLight* ambient_light;
    LightDef** cursor;
    unsigned int spotIndex;
    int index;

    cursor = defs;
    spotIndex = 0;
    for (index = 0; index < 3; index++) {
        def = *cursor;
        if (def != 0) {
            switch (def->type) {
            case 0:
                break;
            case 2:
            case 4:
            case 5:
                continue;
            case 1:
                ambient_light = fetch_light(list, 1, 0)->light;
                if (ambient_light == 0) {
                    continue;
                }
                restore_light_world(ambient_light, def);
                break;
            case 3:
                entry = fetch_light(list, 3, spotIndex);
                spot_light = entry->light;
                if (spot_light == 0) {
                    continue;
                }
                spotIndex++;
                restore_light_world(spot_light, def);
                obj = valid_linked_obj(entry);
                if (obj != 0) {
                    obj->dir_x = def->field1C;
                    obj->dir_y = def->field20;
                    obj->dir_z = def->field24;
                    update_mkobj(obj);
                }
                break;
            }
        }
        cursor++;
    }
}

static MkxRpLight* fetch_light(MkPtr** list, unsigned int type, unsigned int index) {
    MkPtr* node;
    MkPtr* next;
    MkxRpLight* mkx;
    int ok;
    MkHdr* hdr;
    int lightType;

    if (list != 0) {
        node = *list;
        while (node != 0) {
            hdr = node->hdr;
            if (node->instance != hdr->instance) {
                next = node->next;
                discard_stale_mkptr(node);
                node = next;
            } else {
                ok = 0;
                if (hdr != 0) {
                    if (hdr->light_vtbl->destroy == vdestroy_mkx_rplight) {
                        ok = 1;
                    }
                }
                if (ok != 0) {
                    mkx = MKX_RPLIGHT_FROM_HDR(hdr);
                } else {
                    mkx = 0;
                }
                if (mkx != 0) {
                    lightType = mkx->light->object.object.subType;
                    switch (lightType) {
                    case 1:
                        if (type == 3) {
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
                    case 0x80:
                        if (type == 2) {
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
    MkPtr* next;
    MkPtr* node;

    RpLight* light;
    MkxRpLight* mkx;
    MkxRpLight* validated;

    if (list == 0) {
        return;
    }
    node = *list;
    while (node != 0) {
        mkx = MKX_RPLIGHT_FROM_HDR(node->hdr);
        if (node->instance != mkx->hdr.instance) {
            next = node->next;
            discard_stale_mkptr(node);
            node = next;
            continue;
        }
        validated = probe_mkx(node->hdr);
        if (validated != 0) {
            light = validated->light;
            if (light != 0 && RpLightGetWorld(light) != 0) {
                RpWorldRemoveLight(World, light);
            }
        }
        node = node->next;
    }
}

void load_lights(LightDef** defs, MkPtr** list) {
    LightDef** cur;
    int index;

    main_plyr_light_created = 0;
    cur = defs;
    index = 0;
    while (index < 3) {
        if (cur[0] != 0) {
            load_light(cur[0], list, 0);
        }
        index += 1;
        cur += 1;
    }
}

static inline RpLight* create_type5_spot(MkObj* parent, LightDef* def) {
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
    mkobj->flags_08_bits.transform_dirty = 1;
    mkobj->flags_08_bits.bit7 = 1;
    mkobj->light_flags = def->flags;
    mkobj->pos.value.x = def->field1C;
    mkobj->pos.value.y = def->field20;
    mkobj->pos.value.z = def->field24;
    mkobj->ang_row.value.x = def->field28;
    mkobj->ang_row.value.y = def->field2C;
    mkobj->ang_row.value.z = def->field30;
    insert_fgnd_mkobj(mkobj);
    update_mkobj(mkobj);
    RpWorldAddLight(World, light);
    return light;
}

/* TODO: [near miss] 99.57%; linked-object latch homes and owner-load folding remain. */
MkObj* load_light(LightDef* def, MkPtr** list, MkObj* parent) {
    RpLight* light;
    RwFrame* frame;
    MkObj* mkobj;
    MkxRpLight* mkx;
    MkPtr* node;
    MkxRpLight* headMkx;
    MkObj* linked;
    LightPdata* lp;
    MkProc* mkproc;
    int count;
    int procId;

    frame = 0;
    mkobj = parent;

    switch (def->type) {
    case 1:
        light = RpLightCreate(2);
        if (light == 0) {
            goto failed;
        }
        RpLightSetColor(light, &def->color);
        RpWorldAddLight(World, light);
        goto created;

    case 2:
        count = 0;
        if (list != 0) {
            node = *list;
            while (node != 0) {
                if (node->instance != node->hdr->instance) {
                    MkPtr* next = node->next;
                    discard_stale_mkptr(node);
                    node = next;
                } else {
                    node = node->next;
                    count++;
                }
            }
        }
        if (count > 1) {
            headMkx = MKX_RPLIGHT_FROM_HDR(point_light_list->hdr);
            linked = MK_HDR_LIVE(headMkx->obj, headMkx->obj_instance);

            if (linked != 0) {
                if (linked->hdr.instance != 0) {
                    ((MkVtableMkobj*)linked->hdr.vtbl)->destroy(linked);
                }
                headMkx->obj = 0;
                headMkx->obj_instance = 0;
            }
        }
        if (parent == 0) {
            frame = RwFrameCreate();
            if (frame == 0) {
                goto failed;
            }
            mkobj = get_mkobj_frame(0x2001, frame);
            if (mkobj == 0) {
                goto failed;
            }
        } else {
            frame = parent->frame;
        }
        light = RpLightCreate(0x80);
        if (light == 0) {
            goto failed;
        }
        _rwObjectHasFrameSetFrame(light, frame);
        RpLightSetColor(light, &def->color);
        RpLightSetRadius(light, def->field1C);
        RpWorldAddLight(World, light);
        mkobj->light_flags = def->flags;
        mkobj->flags_08_bits.airborne = 1;
        if (parent == 0) {
            mkobj->pos.value.x = def->field20;
            mkobj->pos.value.y = def->field24;
            mkobj->pos.value.z = def->field28;
            insert_fgnd_mkobj(mkobj);
            update_mkobj(mkobj);
        }
        goto created;

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
                goto failed;
            }
            mkobj = get_mkobj_frame(procId, frame);
            if (mkobj == 0) {
                goto failed;
            }
        } else {
            frame = parent->frame;
        }
        light = RpLightCreate(1);
        if (light == 0) {
            goto failed;
        }
        _rwObjectHasFrameSetFrame(light, frame);
        RpLightSetColor(light, &def->color);
        RpWorldAddLight(World, light);
        mkobj->light_flags = def->flags;
        mkobj->flags_08_bits.airborne = 1;
        mkobj->flags_08_bits.angular_velocity_enabled = 1;
        if (parent == 0) {
            mkobj->ang_row.value.x = def->field1C;
            mkobj->ang_row.value.y = def->field20;
            mkobj->ang_row.value.z = def->field24;
            insert_fgnd_mkobj(mkobj);
            update_mkobj(mkobj);
        }
        goto created;

    case 4:
        light = create_spot_light(parent, def);
        if (light != 0) {
            goto created;
        }
        goto failed;

    case 5:
        light = create_type5_spot(parent, def);
        if (light == 0) {
            goto failed;
        }
        goto created;

    default:
        goto failed;
    }

failed:
    if (mkobj != 0 && parent == 0) {
        if (mkobj->hdr.instance != 0) {
            ((MkVtableMkobj*)mkobj->hdr.vtbl)->destroy(mkobj);
        }
        mkobj = 0;
    }
    if (frame != 0) {
        RwFrameDestroy(frame);
    }
    goto done;

created:
    clear_light_low_flags(light);
    if (def->procFn != 0) {
        mkproc = _create_mkproc_generic_tinystack(
            0x5009, 0x28, def->procFn, sizeof(LightPdata), (MkHdr**)&lp);
        if (mkproc != 0) {
            lp->light = light;
            mkproc->pre_destroy = pre_light;
            mkproc->destroy_cb = post_light;
            if (mkobj == 0) {
                mkobj = get_mkobj_frame(0x2003, 0);
                if (mkobj == 0) {
                    goto failed;
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
done:
    return mkobj;
}

static void post_light(void) {
    light_pdata = 0;
    light_obj = 0;
}

static void pre_light(void) {
    light_pdata = LIGHT_PDATA_FROM_HDR(apdata);
    light_obj = MK_HDR_LIVE(light_pdata->obj, light_pdata->obj_instance);
}
