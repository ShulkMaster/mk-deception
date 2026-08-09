#include "dolphin/gx.h"
#include "libmkparticle/rw_engine.h"
#include "rw/rplight.h"
#include "rw/rpworld_types.h"
#include "rw/rtquat.h"
#include "rw/rwvector.h"

typedef struct RwGameCubeLightExt {
    RwUInt32 useAttenuation;
    RwReal a0;
    RwReal a1;
    RwReal a2;
    RwReal k0;
    RwReal k1;
    RwReal k2;
} RwGameCubeLightExt;

typedef struct RwGameCubeLightingData {
    RwUInt8 reserved_0x00[0x0C];
    RwRGBAReal ambient;
    RwBool hasAmbient;
    RwUInt32 lightMask;
    RwInt32 lightIndex;
} RwGameCubeLightingData;

RwInt32 _RwDlLightExtOffset;
GXLightObj _RwGCLightObjs[8];
extern RwMatrix _RwDlInvCamLTM;

#define LIGHT_EXTENSION(light)                                              \
    ((RwGameCubeLightExt*)((RwUInt8*)(light) + _RwDlLightExtOffset))
#define LIGHT_OBJECT(index) (&_RwGCLightObjs[(index)])

#define SET_LIGHT_COLOR(color, light)                                      \
    do {                                                                   \
        (color).r = (signed char)(255.0f * (light)->color.red);            \
        (color).g = (signed char)(255.0f * (light)->color.green);          \
        (color).b = (signed char)(255.0f * (light)->color.blue);           \
        (color).a = 0;                                                     \
    } while (0)

#define APPLY_CUSTOM_ATTENUATION(object, extension)                        \
    GXInitLightAttn((object), (extension)->a0, (extension)->a1,            \
                    (extension)->a2, (extension)->k0, (extension)->k1,     \
                    (extension)->k2)

/* Exact algorithm and size; remaining diff is GX macro register scheduling. */
static void _rpGCHWLightingApplyDirectionalLight(RpLight* light,
                                                  RwInt32 index)
{
    RwV3d direction;
    RwGameCubeLightExt* extension;
    GXColor color;

    RwV3dTransformVector(&direction, &RwFrameGetLTM(RpLightGetFrame(light))->at,
                         &_RwDlInvCamLTM);
    extension = LIGHT_EXTENSION(light);
    if (extension->useAttenuation == 0) {
        GXInitLightAttn(LIGHT_OBJECT(index), 1.0f, 0.0f, 0.0f, 1.0f,
                        0.0f, 0.0f);
    } else {
        APPLY_CUSTOM_ATTENUATION(LIGHT_OBJECT(index), extension);
    }
    GXInitLightPos(LIGHT_OBJECT(index), -1048576.0f * -direction.x,
                   -1048576.0f * direction.y,
                   -1048576.0f * -direction.z);
    SET_LIGHT_COLOR(color, light);
    GXInitLightColor(LIGHT_OBJECT(index), color);
    GXLoadLightObjImm(LIGHT_OBJECT(index), 1U << index);
}

/* Exact traversal/ambient ownership; remaining diff is iterator coloring. */
void _rwGCLightsGlobalEnable(RwInt32 flags, RwGameCubeLightingData* lighting)
{
    RpWorld* world = RwEngineInstance->curWorld;
    RwLLLink* link = world->directionalLightList.link.next;
    RwLLLink* end = &world->directionalLightList.link;

    while (link != end) {
        /* RpLight::inWorld is the canonical intrusive link at +0x34. */
        RpLight* light = (RpLight*)((RwUInt8*)link - 0x34);

        if (light != NULL && (light->object.object.flags & flags) != 0) {
            if (RpLightGetType(light) == rpLIGHTDIRECTIONAL) {
                _rpGCHWLightingApplyDirectionalLight(light,
                                                      lighting->lightIndex);
                lighting->lightMask |= 1U << lighting->lightIndex;
                lighting->lightIndex++;
            } else {
                lighting->ambient.red += light->color.red;
                lighting->ambient.green += light->color.green;
                lighting->ambient.blue += light->color.blue;
                lighting->hasAmbient = TRUE;
            }
        }
        link = link->next;
    }
}

/* Exact switch, GX calls, and size; remaining diff is macro scheduling. */
void _rwGCLightsLocalEnable(RpLight* light,
                            RwGameCubeLightingData* lighting)
{
    if (lighting->lightIndex < 8) {
        RwMatrix* lightLTM;
        RwV3d position;
        RwGameCubeLightExt* extension;
        GXColor color;

        SET_LIGHT_COLOR(color, light);
        GXInitLightColor(LIGHT_OBJECT(lighting->lightIndex), color);
        lightLTM = RwFrameGetLTM(RpLightGetFrame(light));
        RwV3dTransformPoint(&position, &lightLTM->pos, &_RwDlInvCamLTM);
        GXInitLightPos(LIGHT_OBJECT(lighting->lightIndex), -position.x,
                       position.y, -position.z);
        extension = LIGHT_EXTENSION(light);

        switch (RpLightGetType(light)) {
        case rpLIGHTPOINT:
            if (extension->useAttenuation == 0) {
                GXInitLightAttnA(LIGHT_OBJECT(lighting->lightIndex), 1.0f,
                                 0.0f, 0.0f);
                GXInitLightDistAttn(LIGHT_OBJECT(lighting->lightIndex),
                                    0.5f * light->radius, 0.5f, 2);
            } else {
                APPLY_CUSTOM_ATTENUATION(
                    LIGHT_OBJECT(lighting->lightIndex), extension);
            }
            break;

        case rpLIGHTSPOT: {
            RwV3d direction;

            RwV3dTransformVector(&direction, &lightLTM->at,
                                 &_RwDlInvCamLTM);
            GXInitLightDir(LIGHT_OBJECT(lighting->lightIndex), -direction.x,
                           direction.y, -direction.z);
            if (extension->useAttenuation == 0) {
                GXInitLightSpot(LIGHT_OBJECT(lighting->lightIndex),
                                57.29578f * RpLightGetConeAngle(light),
                                1);
                GXInitLightDistAttn(LIGHT_OBJECT(lighting->lightIndex),
                                    0.5f * light->radius, 0.5f, 2);
            } else {
                APPLY_CUSTOM_ATTENUATION(
                    LIGHT_OBJECT(lighting->lightIndex), extension);
            }
            break;
        }

        case rpLIGHTSPOTSOFT: {
            RwV3d direction;

            RwV3dTransformVector(&direction, &lightLTM->at,
                                 &_RwDlInvCamLTM);
            GXInitLightDir(LIGHT_OBJECT(lighting->lightIndex), -direction.x,
                           direction.y, -direction.z);
            if (extension->useAttenuation == 0) {
                GXInitLightSpot(LIGHT_OBJECT(lighting->lightIndex),
                                57.29578f * RpLightGetConeAngle(light),
                                2);
                GXInitLightDistAttn(LIGHT_OBJECT(lighting->lightIndex),
                                    0.5f * light->radius, 0.5f, 2);
            } else {
                APPLY_CUSTOM_ATTENUATION(
                    LIGHT_OBJECT(lighting->lightIndex), extension);
            }
            break;
        }
        }

        GXLoadLightObjImm(LIGHT_OBJECT(lighting->lightIndex),
                          1U << lighting->lightIndex);
        lighting->lightMask |= 1U << lighting->lightIndex;
        lighting->lightIndex++;
    }
}

static void rwDlLightExtCnst(RpLight* light)
{
    RwGameCubeLightExt* extension = LIGHT_EXTENSION(light);

    extension->useAttenuation = 0;
}

static void rwDlLightExtDest(void)
{
}

static void rwDlLightExtCopy(void)
{
}

RwBool _rpDlLightPluginAttach(void)
{
    _RwDlLightExtOffset = RpLightRegisterPlugin(
        sizeof(RwGameCubeLightExt), 0x505,
        (RwPluginObjectConstructor)rwDlLightExtCnst,
        (RwPluginObjectDestructor)rwDlLightExtDest,
        (RwPluginObjectCopy)rwDlLightExtCopy);
    if (_RwDlLightExtOffset < 0) {
        return FALSE;
    }
    return TRUE;
}
