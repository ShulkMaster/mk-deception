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

#define rpLIGHTPOSITIONINGSTART 0x80
#define RpLightGetType(light) ((light)->object.object.subType)
#define RpLightGetFrame(light) ((RwFrame*)(light)->object.object.parent)

typedef struct RpLight {
    RwObjectHasFrame object; /* +0x00 */
    RwReal radius;           /* +0x14 */
    RwRGBAReal color;        /* +0x18 */
    RwReal minusCosAngle;    /* +0x28 */
    RwLinkList worldSectorsInLight; /* +0x2C */
    RwLLLink inWorld;               /* +0x34 */
    RwUInt16 lightFrame;            /* +0x3C */
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
struct RpWorld* RpLightGetWorld(const RpLight* light);
struct RpWorld* RpWorldAddLight(struct RpWorld* world, RpLight* light);

#endif
