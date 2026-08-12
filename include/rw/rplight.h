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
    float radius;
    RwRGBAReal color;
    float minusCosAngle;
    RwLinkList worldSectorsInLight;
    RwLLLink inWorld;
    unsigned short lightFrame;
    unsigned short reserved3E;
} RpLight;

static inline int RpLightGetType(const RpLight* light)
{
    return light->object.object.subType;
}

static inline RwFrame* RpLightGetFrame(const RpLight* light)
{
    return (RwFrame*)light->object.object.parent;
}

RpLight* RpLightCreate(int type);
int RpLightDestroy(RpLight* light);
RpLight* RpLightSetColor(RpLight* light, const RwRGBAReal* color);
RpLight* RpLightSetRadius(RpLight* light, float radius);
float RpLightGetConeAngle(const RpLight* light);
RpLight* RpLightSetConeAngle(RpLight* light, float angle);
int RpLightRegisterPlugin(int size, unsigned int pluginID,
                              RwPluginObjectConstructor constructCB,
                              RwPluginObjectDestructor destructCB,
                              RwPluginObjectCopy copyCB);
struct RpWorld* RpLightGetWorld(const RpLight* light);
struct RpWorld* RpWorldAddLight(struct RpWorld* world, RpLight* light);

#endif
