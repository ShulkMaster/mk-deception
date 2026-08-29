#ifndef LIGHT_H
#define LIGHT_H

#include "rw/rplight.h"
#include "runtime/mk_proc.h"
#include "runtime/mk_struct.h"

typedef struct MkObj MkObj;
typedef struct LightDef LightDef;
typedef struct LightPdata LightPdata;
typedef struct MkxRpLight MkxRpLight;

struct LightDef {
    int type;              /* +0x00 */
    MkProcEntryFn procFn;  /* +0x04 */
    int flags;             /* +0x08 */
    RwRGBAReal color;      /* +0x0C */
    float field1C;         /* +0x1C */
    float field20;         /* +0x20 */
    float field24;         /* +0x24 */
    float field28;         /* +0x28 */
    float field2C;         /* +0x2C */
    float field30;         /* +0x30 */
    float spotRadius;      /* +0x34 */
    float coneAngle;       /* +0x38 */
};

/* mkproc pdata for light procs (size 0x14). */
struct LightPdata {
    MkHdr hdr;                  /* +0x00 */
    RpLight* light;             /* +0x08 */
    MkObj* obj;                 /* +0x0C */
    unsigned int obj_instance;  /* +0x10 */
};

/* Midway mkx wrapper around RpLight (same trailing fields as LightPdata). */
struct MkxRpLight {
    MkHdr hdr;                  /* +0x00 */
    RpLight* light;             /* +0x08 */
    MkObj* obj;                 /* +0x0C */
    unsigned int obj_instance;  /* +0x10 */
};

typedef char LightDefSizeCheck[sizeof(LightDef) == 0x3C ? 1 : -1];
typedef char LightPdataSizeCheck[sizeof(LightPdata) == 0x14 ? 1 : -1];
typedef char MkxRpLightSizeCheck[sizeof(MkxRpLight) == 0x14 ? 1 : -1];

/* Both wrappers embed MkHdr at offset zero; keep the container conversion local. */
#define MKX_RPLIGHT_FROM_HDR(hdr_) ((MkxRpLight*)(hdr_))
#define LIGHT_PDATA_FROM_HDR(hdr_) ((LightPdata*)(hdr_))

int adjust_point_light_associated_with_obj_radius(MkObj* obj, float delta);
void obj_add_to_skinned_obj_light_list_with_ambient(MkObj* obj, LightDef* def);
void obj_change_to_bgnd_obj_light_list(MkObj* obj, LightDef* def);
void obj_change_to_skinned_obj_light_list(MkObj* obj, LightDef* def);
RpLight* create_spot_light(MkObj* parent, LightDef* def);
RpLight* create_default_bgnd_specular_light(void);
RpLight* create_default_specular_light(void);
RpLight* get_bgnd_specular_light(void);
RpLight* get_specular_light(void);
void load_back_in_lights(LightDef** defs, MkPtr** list);
void clear_all_lights_in(MkPtr** list);
void load_lights(LightDef** defs, MkPtr** list);
MkObj* load_light(LightDef* def, MkPtr** list, MkObj* parent);
MkxRpLight* find_mkx_rplight_in_obj(MkObj* obj);
MkxRpLight* get_mkx_rplight(RpLight* light);
void bind_rplight_to_obj(RpLight* light, MkObj* obj);
void vdestroy_mkx_rplight(MkxRpLight* light);

extern MkObj* light_obj;
extern LightPdata* light_pdata;
extern MkPtr* point_light_list;
extern MkPtr* skinned_obj_light_list;
extern MkPtr* bgnd_light_list;
extern MkPtr* bgnd_spec_light_list;
extern MkPtr* plyr_light_list;

#endif
