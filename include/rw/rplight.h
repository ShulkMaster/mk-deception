#ifndef RW_RPLIGHT_H
#define RW_RPLIGHT_H

#include "rw/rwobject.h"

/* Partial stock RpLight layout through the game-used radius field. */
typedef struct RpLight {
    RwObject object; /* +0x00 */
    char pad08[0x0C];
    float radius;    /* +0x14 */
} RpLight;

RpLight* RpLightCreate(int type);
void RpLightSetColor(RpLight* light, float* color);
void RpLightSetRadius(RpLight* light, float radius);
void RpLightSetConeAngle(RpLight* light, float angle);
struct RpWorld* RpLightGetWorld(RpLight* light);
void RpWorldAddLight(struct RpWorld* world, RpLight* light);

#endif
