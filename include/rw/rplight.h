#ifndef RW_RPLIGHT_H
#define RW_RPLIGHT_H

#include "rw/rwtypehf.h"

typedef struct RpLight {
    RwObjectHasFrame object; /* +0x00 */
    RwReal radius;           /* +0x14 */
    RwRGBAReal color;        /* +0x18 */
    RwReal minusCosAngle;    /* +0x28 */
    RwLLLink inWorld;        /* +0x2C */
    struct RpWorld* world;   /* +0x34 */
    RwUInt32 frameIndex;     /* +0x38 */
    RwUInt16 renderFrame;    /* +0x3C */
    RwUInt16 reserved3E;
} RpLight;

RpLight* RpLightCreate(RwInt32 type);
RwBool RpLightDestroy(RpLight* light);
RpLight* RpLightSetColor(RpLight* light, const RwRGBAReal* color);
RpLight* RpLightSetRadius(RpLight* light, RwReal radius);
RwReal RpLightGetConeAngle(const RpLight* light);
RpLight* RpLightSetConeAngle(RpLight* light, RwReal angle);
RwInt32 RpLightRegisterPlugin(RwInt32 size, RwUInt32 pluginID,
                              RwPluginObjectConstructor constructCB,
                              RwPluginObjectDestructor destructCB,
                              RwPluginObjectCopy copyCB);
struct RpWorld* RpLightGetWorld(RpLight* light);
void RpWorldAddLight(struct RpWorld* world, RpLight* light);

#endif
