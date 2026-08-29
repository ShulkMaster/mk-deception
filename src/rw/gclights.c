#include "dolphin/gx.h"
#include "rw/rwengine.h"
#include "rw/rplight.h"
#include "rw/gamecube.h"
#include "rw/nodegamecube.h"
#include "rw/rpworld_types.h"
#include "rw/rtquat.h"
#include "rw/rwframe.h"
#include "rw/rwvector.h"

typedef struct RwGameCubeLightExt {
    unsigned int useAttenuation;
    float a0;
    float a1;
    float a2;
    float k0;
    float k1;
    float k2;
} RwGameCubeLightExt;

int _RwDlLightExtOffset;
GXLightObj _RwGCLightObjs[8];



static void _rpGCHWLightingApplyDirectionalLight(RpLight* light,
                                                  int index)
{
    RwV3d direction;
    RwGameCubeLightExt* extension;
    RwRGBAReal* lightColor;
    GXColor color;

    RwV3dTransformVector(
        &direction,
        &RwFrameGetLTM((RwFrame*)light->object.object.parent)->at,
        &_RwDlInvCamLTM);
    extension =
        (RwGameCubeLightExt*)((unsigned char*)light + _RwDlLightExtOffset);
    if (extension->useAttenuation == 0) {
        GXInitLightAttn(&_RwGCLightObjs[index], 1.0f, 0.0f, 0.0f, 1.0f,
                        0.0f, 0.0f);
    } else {
        GXInitLightAttn(&_RwGCLightObjs[index], extension->a0, extension->a1,
                        extension->a2, extension->k0, extension->k1,
                        extension->k2);
    }
    GXInitLightPos(&_RwGCLightObjs[index], -1048576.0f * -direction.x,
                   -1048576.0f * direction.y,
                   -1048576.0f * -direction.z);
    lightColor = &light->color;
    color.r = 255.0f * lightColor->red;
    color.g = 255.0f * lightColor->green;
    color.b = 255.0f * lightColor->blue;
    color.a = 0;
    GXInitLightColor(&_RwGCLightObjs[index], color);
    GXLoadLightObjImm(&_RwGCLightObjs[index], 1U << index);
}



void _rwGCLightsGlobalEnable(int flags, RwGameCubeLightingData* lighting)
{
    RpWorld* world = (RpWorld*)RwEngineInstance->curWorld;
    RwLLLink* link = world->directionalLightList.link.next;
    RwLLLink* end = &world->directionalLightList.link;

    while (link != end) {

        RpLight* light = RW_CONTAINER_OF(link, RpLight, inWorld);

        if (light != 0 &&
            (light->object.object.flags & (unsigned char)flags) != 0) {
            if ((int)light->object.object.subType == rpLIGHTDIRECTIONAL) {
                _rpGCHWLightingApplyDirectionalLight(light,
                                                      lighting->lightIndex);
                lighting->lightMask |= 1U << lighting->lightIndex;
                lighting->lightIndex++;
            } else {
                RwRGBAReal* lightColor = &light->color;

                lighting->ambient.red += lightColor->red;
                lighting->ambient.green += lightColor->green;
                lighting->ambient.blue += lightColor->blue;
                lighting->hasAmbient = 1;
            }
        }
        link = link->next;
    }
}




void _rwGCLightsLocalEnable(RpLight* light,
                            RwGameCubeLightingData* lighting)
{
    if (lighting->lightIndex < 8) {
        RwMatrix* lightLTM;
        RwV3d position;
        RwGameCubeLightExt* extension;
        RwRGBAReal* lightColor;
        GXColor color;

        lightColor = &light->color;
        color.r = 255.0f * lightColor->red;
        color.g = 255.0f * lightColor->green;
        color.b = 255.0f * lightColor->blue;
        color.a = 0;
        GXInitLightColor(&_RwGCLightObjs[lighting->lightIndex], color);
        lightLTM =
            RwFrameGetLTM((RwFrame*)light->object.object.parent);
        RwV3dTransformPoint(&position, &lightLTM->pos, &_RwDlInvCamLTM);
        GXInitLightPos(&_RwGCLightObjs[lighting->lightIndex], -position.x,
                       position.y, -position.z);
        extension =
            (RwGameCubeLightExt*)((unsigned char*)light + _RwDlLightExtOffset);

        switch (light->object.object.subType) {
        case rpLIGHTPOINT:
            if (extension->useAttenuation == 0) {
                GXInitLightAttnA(&_RwGCLightObjs[lighting->lightIndex], 1.0f,
                                 0.0f, 0.0f);
                GXInitLightDistAttn(&_RwGCLightObjs[lighting->lightIndex],
                                    0.5f * light->radius, 0.5f, 2);
            } else {
                GXInitLightAttn(
                    &_RwGCLightObjs[lighting->lightIndex], extension->a0,
                    extension->a1, extension->a2, extension->k0,
                    extension->k1, extension->k2);
            }
            break;

        case rpLIGHTSPOT: {
            RwV3d direction;

            RwV3dTransformVector(&direction, &lightLTM->at,
                                 &_RwDlInvCamLTM);
            GXInitLightDir(&_RwGCLightObjs[lighting->lightIndex], -direction.x,
                           direction.y, -direction.z);
            if (extension->useAttenuation == 0) {
                GXInitLightSpot(&_RwGCLightObjs[lighting->lightIndex],
                                57.29578f * RpLightGetConeAngle(light),
                                1);
                GXInitLightDistAttn(&_RwGCLightObjs[lighting->lightIndex],
                                    0.5f * light->radius, 0.5f, 2);
            } else {
                GXInitLightAttn(
                    &_RwGCLightObjs[lighting->lightIndex], extension->a0,
                    extension->a1, extension->a2, extension->k0,
                    extension->k1, extension->k2);
            }
            break;
        }

        case rpLIGHTSPOTSOFT: {
            RwV3d direction;

            RwV3dTransformVector(&direction, &lightLTM->at,
                                 &_RwDlInvCamLTM);
            GXInitLightDir(&_RwGCLightObjs[lighting->lightIndex], -direction.x,
                           direction.y, -direction.z);
            if (extension->useAttenuation == 0) {
                GXInitLightSpot(&_RwGCLightObjs[lighting->lightIndex],
                                57.29578f * RpLightGetConeAngle(light),
                                2);
                GXInitLightDistAttn(&_RwGCLightObjs[lighting->lightIndex],
                                    0.5f * light->radius, 0.5f, 2);
            } else {
                GXInitLightAttn(
                    &_RwGCLightObjs[lighting->lightIndex], extension->a0,
                    extension->a1, extension->a2, extension->k0,
                    extension->k1, extension->k2);
            }
            break;
        }
        }

        GXLoadLightObjImm(&_RwGCLightObjs[lighting->lightIndex],
                          1U << lighting->lightIndex);
        lighting->lightMask |= 1U << lighting->lightIndex;
        lighting->lightIndex++;
    }
}

static void rwDlLightExtCnst(RpLight* light)
{
    RwGameCubeLightExt* extension =
        (RwGameCubeLightExt*)((unsigned char*)light + _RwDlLightExtOffset);

    extension->useAttenuation = 0;
}

static void rwDlLightExtDest(void)
{
}

static void rwDlLightExtCopy(void)
{
}

int _rpDlLightPluginAttach(void)
{
    _RwDlLightExtOffset = RpLightRegisterPlugin(
        sizeof(RwGameCubeLightExt), 0x505,
        (RwPluginObjectConstructor)rwDlLightExtCnst,
        (RwPluginObjectDestructor)rwDlLightExtDest,
        (RwPluginObjectCopy)rwDlLightExtCopy);
    if (_RwDlLightExtOffset < 0) {
        return 0;
    }
    return 1;
}
