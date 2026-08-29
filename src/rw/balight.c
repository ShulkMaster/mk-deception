#include "rw/rwengine.h"
#include "rw/rplight.h"
#include "rw/rwfreelist.h"
#include "rw/rwframe.h"
#include "rw/rwvector.h"
#include "runtime/cmath.h"

static RwPluginRegistry lightTKList = {0x40, 0x40, 0, 0, 0, 0};
static RwFreeList _rpLightFreeList;
static int _rpLightFreeListBlockSize = 0x20;
static int _rpLightFreeListPreallocBlocks = 1;
static RwModuleInfo lightModule;

static void LightTidyDestroyLight(void* light, void*)
{
    RpLightDestroy(light);
}

static RwObjectHasFrame* LightSync(RwObjectHasFrame* object)
{
    return object;
}

RpLight* RpLightSetRadius(RpLight* light, float radius)
{
    RwFrame* frame = light->object.object.parent;
    light->radius = radius;
    if (frame) RwFrameUpdateObjects(frame);
    return light;
}

RpLight* RpLightSetColor(RpLight* light, const RwRGBAReal* color)
{
    light->color = *color;
    if (light->color.red == light->color.green &&
        light->color.red == light->color.blue) {
        light->object.object.privateFlags = 1;
    } else {
        light->object.object.privateFlags = 0;
    }
    return light;
}

float RpLightGetConeAngle(const RpLight* light)
{
    RwSplitBits value;
    RwSplitBits truncated;
    float x = -light->minusCosAngle;
    float z;
    float p;
    float q;
    float r;
    float s;
    float w;
    float correction;
    float high;
    int bits;
    int magnitude;

    value.nReal = x;
    bits = value.nInt;
    magnitude = bits & 0x7fffffff;
    if (magnitude >= 0x3f800000) {
        return bits > 0 ? 0.0f : 3.1415927f;
    }

    if (magnitude < 0x3f000000) {
        if (magnitude <= 0x23000000) {
            return 1.5707964f;
        }
        z = x * x;
        p = z * (z * (z * (z * (z * (0.00003479331f * z +
             0.000791535f) - 0.040055536f) + 0.20121253f) -
             0.32556581f) + 0.16666667f);
        q = z * (z * (z * (0.077038154f * z - 0.688284f) +
             2.0209458f) - 2.403395f) + 1.0f;
        r = p / q;
        return 1.5707963f - (x - (7.5497894e-8f - x * r));
    }

    if (bits < 0) {
        z = 0.5f * (1.0f + x);
        p = z * (z * (z * (z * (z * (0.00003479331f * z +
             0.000791535f) - 0.040055536f) + 0.20121253f) -
             0.32556581f) + 0.16666667f);
        q = z * (z * (z * (0.077038154f * z - 0.688284f) +
             2.0209458f) - 2.403395f) + 1.0f;
        r = p / q;
        s = _rwSqrt(z);
        w = r * s - 7.5497894e-8f;
        return 3.1415925f - 2.0f * (s + w);
    }

    z = 0.5f * (1.0f - x);
    s = _rwSqrt(z);
    truncated.nReal = s;
    truncated.nInt &= 0xfffff000;
    high = truncated.nReal;
    correction = (z - high * high) / (s + high);
    p = z * (z * (z * (z * (z * (0.00003479331f * z +
         0.000791535f) - 0.040055536f) + 0.20121253f) -
         0.32556581f) + 0.16666667f);
    q = z * (z * (z * (0.077038154f * z - 0.688284f) +
         2.0209458f) - 2.403395f) + 1.0f;
    r = p / q;
    w = r * s + correction;
    return 2.0f * (high + w);
}

RpLight* RpLightSetConeAngle(RpLight* light, float angle)
{
    float minusCosAngle;
    if (angle < 0.0f || angle > 1.5707963705062866f) return 0;
    minusCosAngle = -cosf(angle);
    light->minusCosAngle = minusCosAngle;
    return light;
}

int RpLightRegisterPlugin(int size, unsigned int pluginID,
                              RwPluginObjectConstructor constructCB,
                              RwPluginObjectDestructor destructCB,
                              RwPluginObjectCopy copyCB)
{
    int offset = _rwPluginRegistryAddPlugin(
        &lightTKList, size, pluginID, constructCB, destructCB, copyCB);
    return offset;
}

int RpLightDestroy(RpLight* light)
{
    _rwPluginRegistryDeInitObject(&lightTKList, light);
    _rwObjectHasFrameReleaseFrame(light);
    RwEngineInstance->fpFreeListFree(
        *(RwFreeList**)((unsigned char*)RwEngineInstance +
                        lightModule.globalsOffset),
        light);
    return 1;
}


RpLight* RpLightCreate(int type)
{
    RpLight* light;

    light = RwEngineInstance->fpFreeListAlloc(
        *(RwFreeList**)((unsigned char*)RwEngineInstance +
                        lightModule.globalsOffset),
        0x30012);
    if (light == 0) return 0;

    light->object.object.type = 3;
    light->object.object.subType = (unsigned char)type;
    light->object.object.flags = 0;
    light->object.object.privateFlags = 0;
    light->object.object.parent = 0;
    light->object.sync = LightSync;
    light->radius = 0.0f;
    light->minusCosAngle = 0.0f;
    light->color.red = 1.0f;
    light->color.green = 1.0f;
    light->color.blue = 1.0f;
    light->color.alpha = 1.0f;
    light->object.object.privateFlags = 1;
    light->worldSectorsInLight.link.next = &light->worldSectorsInLight.link;
    light->worldSectorsInLight.link.prev = &light->worldSectorsInLight.link;
    light->inWorld.prev = 0;
    light->inWorld.next = 0;
    light->lightFrame = RwEngineInstance->lightFrame - 1;
    light->object.object.flags = 3;
    _rwPluginRegistryInitObject(&lightTKList, light);
    return light;
}

void* _rpLightClose(void* instance, int, int)
{
    RwFreeListForAllUsed(
        *(RwFreeList**)((unsigned char*)RwEngineInstance +
                        lightModule.globalsOffset),
        LightTidyDestroyLight, 0);
    RwFreeListDestroy(
        *(RwFreeList**)((unsigned char*)RwEngineInstance +
                        lightModule.globalsOffset));
    *(RwFreeList**)((unsigned char*)RwEngineInstance +
                    lightModule.globalsOffset) = 0;
    lightModule.numInstances--;
    return instance;
}

void* _rpLightOpen(void* instance, int offset, int)
{
    lightModule.globalsOffset = offset;
    *(RwFreeList**)((unsigned char*)RwEngineInstance +
                    lightModule.globalsOffset) =
        RwFreeListCreateAndPreallocateSpace(
        lightTKList.sizeOfStruct, _rpLightFreeListBlockSize, 4,
        _rpLightFreeListPreallocBlocks, &_rpLightFreeList, 0x40012);
    if (*(RwFreeList**)((unsigned char*)RwEngineInstance +
                        lightModule.globalsOffset) != 0) {
        lightModule.numInstances++;
        return instance;
    }
    return 0;
}
