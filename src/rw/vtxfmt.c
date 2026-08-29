#include "dolphin/gx.h"
#include "rw/rwengine.h"
#include "rw/gamecube.h"
#include "rw/rwplcore.h"
#include "rw/rwresentry.h"

static RpGameCubeVtxFmt _RpDlVtxFmtDefault;
static RwModuleInfo _RpVtxFmtModule;
int _rpDlWorldVtxFmtOffset;
int _rpDlGeomVtxFmtOffset;

void _rwDlVtxFmtSetup(RpGameCubeVtxFmt* format,
                      RpGameCubeVtxFmtSetupData* setupData)
{
    RwGameCubeVertexBuffer* resource;
    unsigned int arrayIndex = 0;
    unsigned int attribute;
    int colorCount;

    if (format == 0) format = &_RpDlVtxFmtDefault;
    resource = (RwGameCubeVertexBuffer*)(setupData->resourceEntry + 1);

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

static void* _rxDlVertexFmtConst(void* object, int offset, int size)
{
    *(RpGameCubeVtxFmt**)((unsigned char*)object + offset) = 0;
    return object;
}

static void* _rxDlVertexFmtDest(void* object, int offset, int size)
{
    RpGameCubeVtxFmt** format =
        (RpGameCubeVtxFmt**)((unsigned char*)object + offset);
    if (*format != 0) RpGameCubeVtxFmtDestroy(*format);
    return object;
}

static void* _rpDlVtxFmtOpen(void* instance, int offset, int size)
{
    _RpVtxFmtModule.numInstances++;
    if (_RpVtxFmtModule.numInstances == 1)
        RpGameCubeVtxFmtInit(&_RpDlVtxFmtDefault);
    return instance;
}

static void* _rpDlVtxFmtClose(void* instance, int offset, int size)
{
    _RpVtxFmtModule.numInstances--;
    return instance;
}

int _rpDlVtxFmtPluginAttach(void)
{
    int result = RwEngineRegisterPlugin(
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

void RpGameCubeVtxFmtSetPosition(RpGameCubeVtxFmt* format, unsigned int type,
                                 unsigned char fraction)
{
    format->positionType = (unsigned char)type;
    format->positionFraction = fraction;
}

void RpGameCubeVtxFmtSetNormal(RpGameCubeVtxFmt* format, unsigned int type,
                               unsigned int mode)
{
    format->normalType = (unsigned char)type;
    format->normalMode = (unsigned char)mode;
}

void RpGameCubeVtxFmtSetTexCoord(RpGameCubeVtxFmt* format, int index,
                                 unsigned int type, unsigned char fraction)
{
    format->texCoordType[index - 1] = (unsigned char)type;
    format->texCoordFraction[index - 1] = fraction;
}

void RpGameCubeVtxFmtInit(RpGameCubeVtxFmt* format)
{
    int i;

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
        (unsigned char*)geometry + _rpDlGeomVtxFmtOffset);
    if (*current != 0) RpGameCubeVtxFmtDestroy(*current);
    *current = format;
    (*current)->refCount++;
}
