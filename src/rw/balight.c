#include "libmkparticle/rw_engine.h"
#include "rw/rplight.h"
#include "rw/rwfreelist.h"

extern RwFrame* RwFrameUpdateObjects(RwFrame* frame);
extern float cosf(float);
extern RwReal _rwSqrt(RwReal value);

static RwPluginRegistry lightTKList = {0x40, 0x40, 0, 0, 0, 0};
static RwFreeList _rpLightFreeList;
static RwInt32 _rpLightFreeListBlockSize = 0x20;
static RwInt32 _rpLightFreeListPreallocBlocks = 1;
static RwModuleInfo lightModule;

static void LightTidyDestroyLight(void* light, void*)
{
    RpLightDestroy(light);
}

static RwObjectHasFrame* LightSync(RwObjectHasFrame* object)
{
    return object;
}

RpLight* RpLightSetRadius(RpLight* light, RwReal radius)
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

RwReal RpLightGetConeAngle(const RpLight* light)
{
    RwSplitBits value;
    RwSplitBits truncated;
    RwReal x = -light->minusCosAngle;
    RwReal z;
    RwReal p;
    RwReal q;
    RwReal r;
    RwReal s;
    RwReal w;
    RwReal correction;
    RwReal high;
    RwInt32 bits;
    RwInt32 magnitude;

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

RpLight* RpLightSetConeAngle(RpLight* light, RwReal angle)
{
    RwReal minusCosAngle;
    if (angle < 0.0f || angle > 1.5707963705062866f) return 0;
    minusCosAngle = -cosf(angle);
    light->minusCosAngle = minusCosAngle;
    return light;
}

RwInt32 RpLightRegisterPlugin(RwInt32 size, RwUInt32 pluginID,
                              RwPluginObjectConstructor constructCB,
                              RwPluginObjectDestructor destructCB,
                              RwPluginObjectCopy copyCB)
{
    RwInt32 offset = _rwPluginRegistryAddPlugin(
        &lightTKList, size, pluginID, constructCB, destructCB, copyCB);
    return offset;
}

RwBool RpLightDestroy(RpLight* light)
{
    RwFreeList* freeList = *(RwFreeList**)((RwUInt8*)RwEngineInstance +
                                          lightModule.globalsOffset);
    _rwPluginRegistryDeInitObject(&lightTKList, light);
    _rwObjectHasFrameReleaseFrame(light);
    RwEngineInstance->fpFreeListFree(freeList, light);
    return 1;
}


RpLight* RpLightCreate(RwInt32 type)
{
    RwFreeList* freeList = *(RwFreeList**)((RwUInt8*)RwEngineInstance +
                                          lightModule.globalsOffset);
    RpLight* light;

    light = RwEngineInstance->fpFreeListAlloc(freeList, 0x30012);
    if (light == 0) return 0;

    rwObjectInitialize(light, 3, type);
    light->object.sync = LightSync;
    light->radius = 0.0f;
    light->minusCosAngle = 0.0f;
    light->color.red = 1.0f;
    light->color.green = 1.0f;
    light->color.blue = 1.0f;
    light->color.alpha = 1.0f;
    light->object.object.privateFlags = 1;
    rwLinkListInitialize(&light->worldSectorsInLight);
    light->inWorld.prev = 0;
    light->inWorld.next = 0;
    light->lightFrame = *(RwUInt16*)((RwUInt8*)RwEngineInstance + 0xA) - 1;
    light->object.object.flags = 3;
    _rwPluginRegistryInitObject(&lightTKList, light);
    return light;
}

void* _rpLightClose(void* instance, RwInt32, RwInt32)
{
    RwFreeList** freeList = (RwFreeList**)((RwUInt8*)RwEngineInstance +
                                          lightModule.globalsOffset);
    RwFreeListForAllUsed(*freeList, LightTidyDestroyLight, 0);
    RwFreeListDestroy(*freeList);
    *freeList = 0;
    lightModule.numInstances--;
    return instance;
}

void* _rpLightOpen(void* instance, RwInt32 offset, RwInt32)
{
    RwFreeList** freeList;
    lightModule.globalsOffset = offset;
    freeList = (RwFreeList**)((RwUInt8*)RwEngineInstance + offset);
    *freeList = RwFreeListCreateAndPreallocateSpace(
        lightTKList.sizeOfStruct, _rpLightFreeListBlockSize, 4,
        _rpLightFreeListPreallocBlocks, &_rpLightFreeList, 0x40012);
    if (*freeList != 0) {
        lightModule.numInstances++;
        return instance;
    }
    return 0;
}
