#include "platform/gcpipemanager.h"

#include "runtime/mk_obj.h"
#include "runtime/mk_plugins.h"
#include "runtime/utils.h"
#include "rw/rpmatfx.h"
#include "rw/rpskin.h"

typedef struct RpGameCubeVtxFmt {
    unsigned char data[0x18];
} RpGameCubeVtxFmt;

static void MatFunc6(RwRGBAReal* color, GXColor* material, void*, float intensity);
static void MatFunc5(RwRGBAReal* color, GXColor* material, void*, float intensity);
static void MatFunc4(RwRGBAReal* color, GXColor* material, void*, float intensity);
static void MatFunc3(RwRGBAReal* color, GXColor* material, void*, float intensity);
static void MatFunc2(RwRGBAReal* color, GXColor* material, void*, float intensity);
static void MatFunc1(RwRGBAReal* color, GXColor* material, void*, float intensity);
static void SetupMKPipelinesOnAtomic(RpAtomic* atomic, void* owner);

void RpGameCubeVtxFmtInit(RpGameCubeVtxFmt* format);
void RpGameCubeVtxFmtSetNormal(RpGameCubeVtxFmt* format, int count, int type);
void RpGameCubeVtxFmtSetPosition(RpGameCubeVtxFmt* format, int count, int type);
void RpGameCubeVtxFmtSetTexCoord(RpGameCubeVtxFmt* format, int index,
                                 int count, int type);
void RpGameCubeGeometrySetVtxFmt(RpGeometry* geometry,
                                 RpGameCubeVtxFmt* format);
extern void* SpecSkinAtomicPipeline;
extern int _rxPipelineGlobalsOffset;

static const GXColor OpaqueWhite = {255, 255, 255, 255};
static const GXColor OpaqueBlack = {0, 0, 0, 255};

RpGameCubeVtxFmt gamecube_vtxfmt_skinned;
RpGameCubeVtxFmt gamecube_vtxfmt_skinned2;
RpGameCubeVtxFmt gamecube_vtxfmt_generic;
static int bInitVtxFmts;

DpMaterialCallback DPObjectRenderSetup(int flags, unsigned int light_mask,
                                       int use_matfx, int use_alpha) {
    DpMaterialCallback callback = 0;
    unsigned int textured = flags & 0x84;
    unsigned int vertex_alpha = flags & 8;
    int color_enable;
    int color_ambient;
    int color_material;
    int color_light;
    int alpha_enable;
    int alpha_ambient;
    int alpha_material;
    unsigned char tev_stages;
    GXColor color;

    if (textured != 0) {
        if (vertex_alpha != 0 && use_matfx == 1) {
            if (light_mask != 0) {
                if (flags & 0x40) {
                    callback = MatFunc1;
                } else {
                    color = OpaqueWhite;
                    GXSetTevColor(3, color);
                    callback = MatFunc2;
                }
                color_light = 0;
                color_material = 0;
                color_enable = 1;
                alpha_enable = 1;
                if (use_alpha == 1) {
                    alpha_ambient = 1;
                    alpha_material = 1;
                } else {
                    alpha_ambient = 0;
                    color = OpaqueBlack;
                    GXSetChanAmbColor(2, color);
                    alpha_material = 0;
                }
            } else {
                if (flags & 0x40) {
                    callback = MatFunc1;
                } else {
                    color = OpaqueWhite;
                    GXSetTevColor(3, color);
                    callback = MatFunc2;
                }
                alpha_enable = 0;
                alpha_ambient = 0;
                color_light = 1;
                if (use_alpha == 1) {
                    color_material = 1;
                } else {
                    color_material = 0;
                    color = OpaqueBlack;
                    GXSetChanMatColor(2, color);
                }
                color_enable = 0;
                alpha_material = 0;
            }
            tev_stages = 2;
            GXSetTevKColorSel(0, 13);
            GXSetTevKAlphaSel(0, 29);
            GXSetTevColorIn(0, 15, 10, 6, 14);
            GXSetTevAlphaIn(0, 7, 5, 3, 6);
        } else {
            if (light_mask != 0) {
                if (flags & 0x40) {
                    color_light = 0;
                    color_material = 0;
                    color_enable = 1;
                    if (vertex_alpha != 0) {
                        alpha_enable = 1;
                        if (use_alpha == 1) {
                            alpha_ambient = 1;
                            alpha_material = 1;
                        } else {
                            alpha_ambient = 0;
                            color = OpaqueBlack;
                            GXSetChanAmbColor(2, color);
                            alpha_material = 0;
                        }
                        callback = MatFunc5;
                    } else {
                        alpha_enable = 0;
                        alpha_ambient = 0;
                        color = OpaqueBlack;
                        alpha_material = 0;
                        GXSetChanAmbColor(2, color);
                        callback = MatFunc4;
                    }
                } else {
                    color_light = 0;
                    color_material = 0;
                    color = OpaqueWhite;
                    GXSetChanMatColor(4, color);
                    color_enable = 1;
                    if (vertex_alpha != 0) {
                        alpha_enable = 1;
                        if (use_alpha == 1) {
                            alpha_ambient = 1;
                            alpha_material = 1;
                        } else {
                            alpha_ambient = 0;
                            alpha_material = 0;
                        }
                    } else {
                        alpha_enable = 0;
                        alpha_ambient = 0;
                        callback = MatFunc3;
                        alpha_material = 0;
                    }
                }
            } else if (flags & 0x40) {
                color_light = 0;
                color_material = 0;
                color_enable = 1;
                if (vertex_alpha != 0) {
                    alpha_enable = 1;
                    if (use_alpha == 1) {
                        alpha_ambient = 1;
                        alpha_material = 1;
                    } else {
                        alpha_ambient = 0;
                        color = OpaqueBlack;
                        GXSetChanAmbColor(2, color);
                        alpha_material = 0;
                    }
                    callback = MatFunc5;
                } else {
                    alpha_enable = 0;
                    alpha_ambient = 0;
                    color = OpaqueBlack;
                    alpha_material = 0;
                    GXSetChanAmbColor(2, color);
                    callback = MatFunc4;
                }
            } else {
                if (vertex_alpha != 0) {
                    color_light = 1;
                    if (use_alpha == 1) {
                        color_material = 1;
                    } else {
                        color_material = 0;
                        color = OpaqueBlack;
                        GXSetChanMatColor(2, color);
                    }
                } else {
                    color_light = 0;
                    color_material = 0;
                    color = OpaqueBlack;
                    GXSetChanMatColor(2, color);
                    callback = MatFunc6;
                }
                color_enable = 0;
                alpha_material = 0;
                alpha_enable = 0;
                alpha_ambient = 0;
            }
            tev_stages = 1;
            GXSetTevColorIn(0, 15, 10, 8, 15);
            GXSetTevAlphaIn(0, 7, 5, 4, 7);
        }
    } else {
        if (light_mask != 0) {
            color_light = 0;
            color_material = 0;
            color = OpaqueWhite;
            GXSetChanMatColor(4, color);
            color_enable = 1;
            if (flags & 8) {
                alpha_enable = 1;
                if (use_alpha == 1) {
                    alpha_ambient = 1;
                    alpha_material = 1;
                } else {
                    alpha_ambient = 0;
                    alpha_material = 0;
                }
            } else {
                alpha_enable = 0;
                alpha_ambient = 0;
                color = OpaqueBlack;
                alpha_material = 0;
                GXSetChanAmbColor(4, color);
            }
        } else {
            if (flags & 8) {
                color_light = 1;
                if (use_alpha == 1) {
                    color_material = 1;
                } else {
                    color_material = 0;
                    color = OpaqueBlack;
                    GXSetChanMatColor(2, color);
                }
            } else {
                color_light = 0;
                color_material = 0;
                color = OpaqueBlack;
                GXSetChanMatColor(4, color);
            }
            alpha_enable = 0;
            alpha_ambient = 0;
            color_enable = 0;
            alpha_material = 0;
        }
        tev_stages = 1;
        if (flags & 0x40) {
            callback = MatFunc1;
        } else {
            color = OpaqueWhite;
            GXSetTevColor(3, color);
            callback = MatFunc2;
        }
        GXSetTevKColorSel(0, 13);
        GXSetTevKAlphaSel(0, 29);
        GXSetTevColorIn(0, 15, 10, 6, 14);
        GXSetTevAlphaIn(0, 7, 5, 3, 6);
    }

    GXSetNumTevStages(tev_stages);
    GXSetTevColorOp(0, 0, 0, 0, 1, 0);
    GXSetTevAlphaOp(0, 0, 0, 0, 1, 0);
    if (tev_stages > 1) {
        GXSetTevColorIn(1, 15, 0, 8, 15);
        GXSetTevColorOp(1, 0, 0, 0, 1, 0);
        GXSetTevAlphaIn(1, 7, 0, 4, 7);
        GXSetTevAlphaOp(1, 0, 0, 0, 1, 0);
        GXSetTevOrder(0, 255, 255, 4);
        GXSetTevOrder(1, 0, 0, 255);
        GXSetNumTexGens(1);
        GXSetTexCoordGen2(0, 1, 4, 60, 0, 125);
    } else if (textured != 0) {
        GXSetTevOrder(0, 0, 0, 4);
        GXSetNumTexGens(1);
        GXSetTexCoordGen2(0, 1, 4, 60, 0, 125);
    } else {
        GXSetNumTexGens(0);
        GXSetTevOrder(0, 255, 255, 4);
    }
    GXSetNumChans(1);
    GXSetChanCtrl(0, color_enable, alpha_enable, color_light, light_mask, 2, 1);
    GXSetChanCtrl(2, alpha_material, alpha_ambient, color_material, 0, 0, 2);
    GXSetChanCtrl(1, 0, 0, 0, 0, 0, 2);
    GXSetChanCtrl(3, 0, 0, 0, 0, 0, 2);
    return callback;
}

static void MatFunc6(RwRGBAReal* color, GXColor* material, void*, float intensity) {
    GXColor output;
    float scale = 255.0f * intensity;
    output.r = color->red * scale;
    output.g = color->green * scale;
    output.b = color->blue * scale;
    output.a = 0;
    GXSetChanMatColor(0, output);
}

static void MatFunc5(RwRGBAReal* color, GXColor* material, void*, float intensity) {
    GXColor output = *material;
    GXSetChanMatColor(4, output);
}

static void MatFunc4(RwRGBAReal* color, GXColor* material, void*, float intensity) {
    GXColor output;
    GXColor ambient;
    float scale = 255.0f * intensity;
    output = *material;
    ambient.r = color->red * scale;
    ambient.g = color->green * scale;
    ambient.b = color->blue * scale;
    ambient.a = 0;
    GXSetChanMatColor(4, output);
    GXSetChanAmbColor(0, ambient);
}

static void MatFunc3(RwRGBAReal* color, GXColor* material, void*, float intensity) {
    GXColor output;
    float scale = 255.0f * intensity;
    output.r = color->red * scale;
    output.g = color->green * scale;
    output.b = color->blue * scale;
    output.a = 0;
    GXSetChanAmbColor(0, output);
}

static void MatFunc2(RwRGBAReal* color, GXColor* material, void*, float intensity) {
    GXColor output;
    float scale = 255.0f * intensity;
    output.r = color->red * scale;
    output.g = color->green * scale;
    output.b = color->blue * scale;
    output.a = 0;
    GXSetTevKColor(1, output);
}

static void MatFunc1(RwRGBAReal* color, GXColor* material, void*, float intensity) {
    GXColor output;
    GXColor konst;
    output.r = material->r * (color->red * intensity);
    output.g = material->g * (color->green * intensity);
    output.b = material->b * (color->blue * intensity);
    output.a = 0;
    GXSetTevColor(3, *material);
    konst = output;
    GXSetTevKColor(1, konst);
}

void GCNSetupNonRenderwarePipeline(RpClump* clump, void* owner) {
    RwLLLink* link = clump->atomicList.next;
    RwLLLink* sentinel = &clump->atomicList;

    while (link != sentinel) {
        RpAtomic* atomic = RpAtomicFromClumpLink(link);
        RpGeometry* geometry = atomic->geometry;
        RwLLLink* next = link->next;
        MksobjPluginData* atomic_data = MK_ATOMIC_PLUGIN(atomic);

        if (atomic_data->field_0C != 0) {
            SetupMKPipelinesOnAtomic(atomic, owner);
        } else if (geometry->numMorphTargets == 1 &&
                   RpSkinGeometryGetSkin(geometry) == 0) {
            unsigned int material_count = geometry->matList.numMaterials;
            unsigned int index;
            int material_offset = 0;
            for (index = 0; index < material_count; index++, material_offset += 4) {
                RpMaterial* material = *(RpMaterial**)(
                    (unsigned char*)geometry->matList.materials + material_offset);
                MkmaterialPluginData* material_data =
                    MK_MATERIAL_PLUGIN(material);
                RpMatFXMaterialGetEffects(material);
                if (material_data->flags & 0x10000000) {
                    float* scroll = material_data->vec4;
                    RpMatFXAtomicEnableEffects(atomic);
                    if (scroll != 0) {
                        float u1 = scroll[0];
                        float v1 = scroll[1];
                        float u2 = scroll[2];
                        float v2 = scroll[3];
                        if (RpMatFXMaterialGetEffects(material) & 4) {
                            RpMatFXMaterialSetEffects(material, 6);
                        } else {
                            RpMatFXMaterialSetEffects(material, 5);
                        }
                        material_start_uv_scroll(owner, material, u1, v1, u2,
                                                 v2);
                    }
                }
            }
        }
        link = next;
    }
}

static void SetupMKPipelinesOnAtomic(RpAtomic* atomic, void* owner) {
    RpGeometry* geometry;
    MksobjPluginData* atomic_data;
    int has_uv_scroll = 0;
    int atomic_effects;
    int material_count;
    int index;

    if (!bInitVtxFmts) {
        RpGameCubeVtxFmtInit(&gamecube_vtxfmt_skinned);
        RpGameCubeVtxFmtInit(&gamecube_vtxfmt_skinned2);
        RpGameCubeVtxFmtInit(&gamecube_vtxfmt_generic);
        RpGameCubeVtxFmtSetNormal(&gamecube_vtxfmt_skinned, 1, 0);
        RpGameCubeVtxFmtSetPosition(&gamecube_vtxfmt_skinned, 3, 12);
        RpGameCubeVtxFmtSetTexCoord(&gamecube_vtxfmt_skinned, 1, 3, 12);
        RpGameCubeVtxFmtSetNormal(&gamecube_vtxfmt_skinned2, 1, 0);
        RpGameCubeVtxFmtSetPosition(&gamecube_vtxfmt_skinned2, 3, 11);
        RpGameCubeVtxFmtSetTexCoord(&gamecube_vtxfmt_skinned2, 1, 3, 12);
        RpGameCubeVtxFmtSetNormal(&gamecube_vtxfmt_generic, 1, 0);
        RpGameCubeVtxFmtSetTexCoord(&gamecube_vtxfmt_generic, 1, 3, 11);
        bInitVtxFmts = 1;
    }

    geometry = atomic->geometry;
    material_count = geometry->matList.numMaterials;
    for (index = 0; index < material_count; index++) {
        MkmaterialPluginData* data =
            MK_MATERIAL_PLUGIN(geometry->matList.materials[index]);
        if (data->flags & 0x10000000) {
            has_uv_scroll = 1;
            break;
        }
    }
    atomic_effects = RpMatFXAtomicQueryEffects(atomic);
    if (has_uv_scroll) {
        RpMatFXAtomicEnableEffects(atomic);
    }

    atomic_data = MK_ATOMIC_PLUGIN(atomic);
    switch (atomic_data->field_0C) {
    case 0x300:
        RpGameCubeGeometrySetVtxFmt(geometry, &gamecube_vtxfmt_skinned);
        atomic->pipeline = SpecSkinAtomicPipeline;
        break;
    case 0x301:
    case 0x302:
        RpGameCubeGeometrySetVtxFmt(geometry, &gamecube_vtxfmt_skinned);
        atomic->pipeline = RpSkinGetGameCubePipeline(1);
        break;
    case 0x303:
        RpGameCubeGeometrySetVtxFmt(geometry, &gamecube_vtxfmt_skinned2);
        atomic->pipeline = RpSkinGetGameCubePipeline(1);
        break;
    case 0x304:
    case 0x305:
    case 0x306:
        if (!has_uv_scroll && atomic_effects == 0) {
            atomic->pipeline = *(void**)((unsigned char*)RwEngineInstance +
                                         _rxPipelineGlobalsOffset + 0x3C);
        }
        RpGameCubeGeometrySetVtxFmt(geometry, &gamecube_vtxfmt_generic);
        break;
    }

    for (index = 0; index < material_count; index++) {
        RpMaterial* material = geometry->matList.materials[index];
        MkmaterialPluginData* data = MK_MATERIAL_PLUGIN(material);
        float* scroll = data->vec4;
        float u1;
        float v1;
        float u2;
        float v2;
        if (scroll != 0) {
            u1 = scroll[0];
            v1 = scroll[1];
            u2 = scroll[2];
            v2 = scroll[3];
        } else {
            u1 = 0.0f;
            v1 = 0.0f;
            u2 = 0.0f;
            v2 = 0.0f;
        }
        /* Retail keeps this legacy UV-scroll guard even though its body is empty. */
        if ((data->flags & 0x10000000) && u1 == 0.0f) {
        }
        switch (data->field_20) {
        case 0x300:
        case 0x307:
            RpMatFXMaterialSetEffects(material, 5);
            material->pipeline = 0;
            break;
        case 0x301:
        case 0x306:
        case 0x308:
            material_start_uv_scroll(owner, material, u1, v1, u2, v2);
            material->pipeline = 0;
            break;
        case 0:
        case 0x302:
        case 0x303:
        case 0x304:
        case 0x305:
            material->pipeline = 0;
            break;
        }
    }
}
