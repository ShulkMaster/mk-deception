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

#define LIGHTFREELIST RWPLUGINOFFSET(RwFreeList*, RwEngineInstance, lightModule.globalsOffset)

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

#define RW_ACOS_POLYNOMIAL(variable, result)                                \
    do {                                                                    \
        p = variable *                                                      \
            (variable *                                                     \
                 (variable *                                                \
                      (variable *                                           \
                           (variable * (0.00003479331f * variable +          \
                                        0.000791535f) +                      \
                            -0.040055536f) +                                 \
                       0.20121253f) +                                        \
                  -0.32556581f) +                                           \
             0.16666667f);                                                  \
        q = variable *                                                      \
                (variable *                                                 \
                     (variable * (0.077038154f * variable + -0.688284f) +   \
                      2.0209458f) +                                         \
                 -2.403395f) +                                              \
            1.0f;                                                           \
        result = p / q;                                                     \
    } while (0)

#define RwACosMacro(result, input)                                          \
    do {                                                                    \
        RwSplitBits value;                                                  \
        RwSplitBits truncated;                                              \
        RwReal x = (input);                                                 \
        RwReal z, p, q, r, s, w, c, df;                                    \
        RwInt32 hx;                                                         \
        RwInt32 ix;                                                         \
        value.nReal = x;                                                    \
        hx = value.nInt;                                                    \
        ix = hx & 0x7fffffff;                                               \
        if (ix >= 0x3f800000) {                                             \
            if (hx > 0)                                                     \
                (result) = 0.0f;                                            \
            else                                                            \
                (result) = 3.1415927f;                                      \
        } else if (ix < 0x3f000000) {                                      \
            if (ix <= 0x23000000) {                                         \
                (result) = 1.5707964f;                                      \
            } else {                                                        \
                z = x * x;                                                  \
                RW_ACOS_POLYNOMIAL(z, r);                                   \
                (result) = 1.5707963f -                                     \
                           (x - (7.5497894e-8f - x * r));                    \
            }                                                               \
        } else if (hx < 0) {                                                \
            z = 0.5f * (1.0f + x);                                         \
            RW_ACOS_POLYNOMIAL(z, r);                                       \
            s = _rwSqrt(z);                                                 \
            w = r * s - 7.5497894e-8f;                                     \
            (result) = 3.1415925f - 2.0f * (s + w);                         \
        } else {                                                            \
            z = 0.5f * (1.0f - x);                                         \
            s = _rwSqrt(z);                                                 \
            truncated.nReal = s;                                           \
            truncated.nInt &= 0xfffff000;                                  \
            df = truncated.nReal;                                          \
            c = (z - df * df) / (s + df);                                  \
            RW_ACOS_POLYNOMIAL(z, r);                                       \
            w = r * s + c;                                                  \
            (result) = 2.0f * (df + w);                                    \
        }                                                                   \
    } while (0)

RwReal RpLightGetConeAngle(const RpLight* light)
{
    RwReal result;
    RwACosMacro(result, -light->minusCosAngle);
    return result;
}

RpLight* RpLightSetConeAngle(RpLight* light, RwReal angle)
{
    RwReal minusCosAngle;
    if (angle < 0.0f || angle > 1.5707963705062866f) return NULL;
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
    _rwPluginRegistryDeInitObject(&lightTKList, light);
    _rwObjectHasFrameReleaseFrame(light);
    RwEngineInstance->fpFreeListFree(LIGHTFREELIST, light);
    return TRUE;
}

RpLight* RpLightCreate(RwInt32 type)
{
    RpLight* light;

    light = RwEngineInstance->fpFreeListAlloc(LIGHTFREELIST, 0x30012);
    if (light == NULL) return NULL;

    rwObjectInitialize(light, 3, type);
    light->object.sync = LightSync;
    light->radius = 0.0f;
    light->minusCosAngle = 0.0f;
    light->color.red = 1.0f;
    light->color.green = 1.0f;
    light->color.blue = 1.0f;
    light->color.alpha = 1.0f;
    light->object.object.privateFlags = 1;
    rwLinkListInitialize((RwLinkList*)&light->inWorld);
    light->frameIndex = 0;
    light->world = NULL;
    light->renderFrame = RWPLUGINOFFSET(RwUInt16, RwEngineInstance, 0xA) - 1;
    light->object.object.flags = 3;
    _rwPluginRegistryInitObject(&lightTKList, light);
    return light;
}

void* _rpLightClose(void* instance, RwInt32, RwInt32)
{
    RwFreeListForAllUsed(LIGHTFREELIST, LightTidyDestroyLight, NULL);
    RwFreeListDestroy(LIGHTFREELIST);
    LIGHTFREELIST = NULL;
    lightModule.numInstances--;
    return instance;
}

void* _rpLightOpen(void* instance, RwInt32 offset, RwInt32)
{
    lightModule.globalsOffset = offset;
    LIGHTFREELIST = RwFreeListCreateAndPreallocateSpace(
        lightTKList.sizeOfStruct, _rpLightFreeListBlockSize, 4,
        _rpLightFreeListPreallocBlocks, &_rpLightFreeList, 0x40012);
    if (LIGHTFREELIST != NULL) {
        lightModule.numInstances++;
        return instance;
    }
    return NULL;
}
