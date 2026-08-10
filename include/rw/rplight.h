#ifndef RW_RPLIGHT_H
#define RW_RPLIGHT_H

#include "rw/rwtypehf.h"

typedef enum RpLightType {
    rpNALIGHTTYPE = 0,
    rpLIGHTDIRECTIONAL = 1,
    rpLIGHTAMBIENT = 2,
    rpLIGHTPOINT = 0x80,
    rpLIGHTSPOT = 0x81,
    rpLIGHTSPOTSOFT = 0x82
} RpLightType;

typedef struct RpLight {
    RwObjectHasFrame object;
    RwReal radius;
    RwRGBAReal color;
    RwReal minusCosAngle;
    RwLinkList worldSectorsInLight;
    RwLLLink inWorld;
    RwUInt16 lightFrame;
    RwUInt16 reserved3E;
} RpLight;

static inline RwInt32 RpLightGetType(const RpLight* light)
{
    return light->object.object.subType;
}

static inline RwFrame* RpLightGetFrame(const RpLight* light)
{
    return (RwFrame*)light->object.object.parent;
}

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
struct RpWorld* RpLightGetWorld(const RpLight* light);
struct RpWorld* RpWorldAddLight(struct RpWorld* world, RpLight* light);

#endif
