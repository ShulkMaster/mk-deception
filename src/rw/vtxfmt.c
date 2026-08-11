#include "dolphin/gx.h"
#include "libmkparticle/rw_engine.h"
#include "rw/gamecube.h"
#include "rw/rwplcore.h"

extern RwInt32 RpGeometryRegisterPlugin(
    RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor constructCB,
    RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB);
extern RwInt32 RpWorldRegisterPlugin(
    RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor constructCB,
    RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB);

static RpGameCubeVtxFmt _RpDlVtxFmtDefault;
static RwModuleInfo _RpVtxFmtModule;
RwInt32 _rpDlWorldVtxFmtOffset;
RwInt32 _rpDlGeomVtxFmtOffset;

void _rwDlVtxFmtSetup(RpGameCubeVtxFmt* format,
                      RpGameCubeVtxFmtSetupData* setupData)
{
    RwGameCubeVertexBuffer* resource;
    RwUInt32 arrayIndex = 0;
    RwUInt32 attribute;
    RwInt32 colorCount;

    if (format == 0) format = &_RpDlVtxFmtDefault;
    resource = (RwGameCubeVertexBuffer*)((RwUInt8*)setupData->resourceEntry + 0x18);

    GXClearVtxDesc();
    GXSetVtxDesc(9, resource->arrays[arrayIndex].descriptor);
    GXSetVtxAttrFmt(0, 9, 1, format->positionType,
                    format->positionFraction);
    GXSetArray(9, resource->arrays[arrayIndex].data,
               resource->arrays[arrayIndex].stride);
    arrayIndex++;

    if (setupData->flags & 0x10) {
        if (format->normalMode != 0) {
            GXSetVtxDesc(0x19, resource->arrays[arrayIndex].descriptor);
            GXSetVtxAttrFmt(0, 0x19, 1, format->normalType, 0);
            GXSetArray(0x19, resource->arrays[arrayIndex].data,
                       resource->arrays[arrayIndex].stride);
            arrayIndex++;
        } else {
            GXSetVtxDesc(0xA, resource->arrays[arrayIndex].descriptor);
            GXSetVtxAttrFmt(0, 0xA, 0, format->normalType, 0);
            GXSetArray(0xA, resource->arrays[arrayIndex].data,
                       resource->arrays[arrayIndex].stride);
            arrayIndex++;
        }
    }

    if (setupData->flags & 8) {
        GXSetVtxDesc(0xB, resource->arrays[arrayIndex].descriptor);
        if (format->colorType > 2)
            colorCount = 1;
        else
            colorCount = 0;
        GXSetVtxAttrFmt(0, 0xB, colorCount,
                        format->colorType, 0);
        GXSetArray(0xB, resource->arrays[arrayIndex].data,
                   resource->arrays[arrayIndex].stride);
        arrayIndex++;
    }

    if (setupData->flags & 0x84) {
        attribute = 0xD;
        while (arrayIndex < resource->numArrays) {
            GXSetVtxDesc(attribute, resource->arrays[arrayIndex].descriptor);
            GXSetVtxAttrFmt(0, attribute, 1,
                            format->texCoordType[attribute - 0xD],
                            format->texCoordFraction[attribute - 0xD]);
            GXSetArray(attribute, resource->arrays[arrayIndex].data,
                       resource->arrays[arrayIndex].stride);
            arrayIndex++;
            attribute++;
        }
    }
}

RpGameCubeVtxFmt* _rpGameCubeVtxFmtGetDefault(void)
{
    return &_RpDlVtxFmtDefault;
}

static void* _rxDlVertexFmtConst(void* object, RwInt32 offset, RwInt32 size)
{
    *(RpGameCubeVtxFmt**)((RwUInt8*)object + offset) = 0;
    return object;
}

static void* _rxDlVertexFmtDest(void* object, RwInt32 offset, RwInt32 size)
{
    RpGameCubeVtxFmt** format =
        (RpGameCubeVtxFmt**)((RwUInt8*)object + offset);
    if (*format != 0) RpGameCubeVtxFmtDestroy(*format);
    return object;
}

static void* _rpDlVtxFmtOpen(void* instance, RwInt32 offset, RwInt32 size)
{
    _RpVtxFmtModule.numInstances++;
    if (_RpVtxFmtModule.numInstances == 1)
        RpGameCubeVtxFmtInit(&_RpDlVtxFmtDefault);
    return instance;
}

static void* _rpDlVtxFmtClose(void* instance, RwInt32 offset, RwInt32 size)
{
    _RpVtxFmtModule.numInstances--;
    return instance;
}

RwBool _rpDlVtxFmtPluginAttach(void)
{
    RwInt32 result = RwEngineRegisterPlugin(
        0, 0x511, _rpDlVtxFmtOpen, _rpDlVtxFmtClose);
    if (result < 0) return 0;

    _rpDlGeomVtxFmtOffset = RpGeometryRegisterPlugin(
        4, 0x511, _rxDlVertexFmtConst, _rxDlVertexFmtDest, 0);
    if (_rpDlGeomVtxFmtOffset < 0) return 0;

    _rpDlWorldVtxFmtOffset = RpWorldRegisterPlugin(
        4, 0x511, _rxDlVertexFmtConst, _rxDlVertexFmtDest, 0);
    if (_rpDlWorldVtxFmtOffset < 0) return 0;
    return 1;
}

void RpGameCubeVtxFmtSetPosition(RpGameCubeVtxFmt* format, RwUInt32 type,
                                 RwUInt8 fraction)
{
    format->positionType = (RwUInt8)type;
    format->positionFraction = fraction;
}

void RpGameCubeVtxFmtSetNormal(RpGameCubeVtxFmt* format, RwUInt32 type,
                               RwUInt32 mode)
{
    format->normalType = (RwUInt8)type;
    format->normalMode = (RwUInt8)mode;
}

void RpGameCubeVtxFmtSetTexCoord(RpGameCubeVtxFmt* format, RwInt32 index,
                                 RwUInt32 type, RwUInt8 fraction)
{
    format->fields[index + 1] = (RwUInt8)type;
    format->fields[index + 0xD] = fraction;
}

void RpGameCubeVtxFmtInit(RpGameCubeVtxFmt* format)
{
    RwInt32 i;

    format->positionType = 4;
    format->normalType = 4;
    for (i = 0; i < 8; i++) format->texCoordType[i] = 4;
    format->colorType = 5;
    format->field_0x0B = 1;
    format->positionFraction = 0;
    for (i = 0; i < 8; i++) format->texCoordFraction[i] = 0;
    format->normalMode = 0;
    format->refCount = 1;
}

RpGameCubeVtxFmt* RpGameCubeVtxFmtCreate(void)
{
    RpGameCubeVtxFmt* format =
        RwEngineInstance->fpMalloc(sizeof(*format), 0x3050D);
    RpGameCubeVtxFmtInit(format);
    return format;
}

void RpGameCubeVtxFmtDestroy(RpGameCubeVtxFmt* format)
{
    if (format->refCount == 1)
        RwEngineInstance->fpFree(format);
    else
        format->refCount--;
}

void RpGameCubeGeometrySetVtxFmt(RpGeometry* geometry,
                                 RpGameCubeVtxFmt* format)
{
    RpGameCubeVtxFmt** current = (RpGameCubeVtxFmt**)(
        (RwUInt8*)geometry + _rpDlGeomVtxFmtOffset);
    if (*current != 0) RpGameCubeVtxFmtDestroy(*current);
    *current = format;
    (*current)->refCount++;
}
