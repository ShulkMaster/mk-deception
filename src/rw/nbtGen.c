#include "dolphin/cache.h"
#include "dolphin/gx.h"
#include "rw/gamecube.h"
#include "rw/rpmatfx.h"
#include "rw/rpworld_types.h"
#include "rw/rtquat.h"
#include "rw/rwresources.h"

typedef struct NBTCalcData {
    const RwUInt8* positions[3];
    RwUInt8* normal;
    const RwUInt8* texCoords[3];
    RwUInt8 positionSize;
    RwUInt8 normalSize;
    RwUInt8 texCoordSize;
    RwUInt8 reserved;
} NBTCalcData;

typedef struct NBTInstanceData {
    RwResEntry* resourceEntry;
    RpMeshHeader* meshHeader;
} NBTInstanceData;

typedef struct NBTResourceEntry {
    RwResEntry entry;
    RwUInt16 token;
    RwUInt16 meshSerialNum;
    RwGameCubeVertexBuffer vertexBuffer;
} NBTResourceEntry;

typedef struct RpGameCubeMTEntry24Private {
    RwInt32 value[5];
    RwUInt16 field_0x14;
    RwUInt16 field_0x16;
} RpGameCubeMTEntry24Private;

typedef struct RpGameCubeMTEffectConfigPrivate {
    RwUInt8 reserved_0x00[9];
    RwUInt8 count24;
    RwUInt8 reserved_0x0A[0x3A];
    RpGameCubeMTEntry24Private* entries24;
} RpGameCubeMTEffectConfigPrivate;

typedef char NBTCalcDataSizeCheck[sizeof(NBTCalcData) == 0x20 ? 1 : -1];
typedef char NBTInstanceDataSizeCheck[sizeof(NBTInstanceData) == 8 ? 1 : -1];
typedef char RpGameCubeMTEntry24PrivateSizeCheck[
    sizeof(RpGameCubeMTEntry24Private) == 0x18 ? 1 : -1];

static RwUInt8 nbtPositionType;
static RwUInt8 nbtNormalType;
static RwUInt8 nbtTexCoordType;
static RwUInt8 nbtPositionFraction;
static RwUInt8 nbtNormalFraction;
static RwUInt8 nbtTexCoordFraction;

#define READ_SCALAR(_result, _address, _type, _fraction)                    \
    do {                                                                    \
        RwReal _scale = (RwReal)(1U << (_fraction));                        \
        switch (_type) {                                                    \
        case 0: (_result) = *(const RwUInt8*)(_address) / _scale; break;    \
        case 1: (_result) = *(const signed char*)(_address) / _scale; break; \
        case 2: (_result) = *(const RwUInt16*)(_address) / _scale; break;   \
        case 3: (_result) = *(const RwInt16*)(_address) / _scale; break;    \
        default: (_result) = *(const RwReal*)(_address); break;             \
        }                                                                   \
    } while (0)

#define WRITE_SCALAR(_address, _value, _type, _fraction)                   \
    do {                                                                    \
        RwReal _scaled = (_value) * (RwReal)(1U << (_fraction));            \
        switch (_type) {                                                    \
        case 0: *(RwUInt8*)(_address) = (RwUInt8)_scaled; break;            \
        case 1: *(signed char*)(_address) = (signed char)_scaled; break;    \
        case 2: *(RwUInt16*)(_address) = (RwUInt16)_scaled; break;          \
        case 3: *(RwInt16*)(_address) = (RwInt16)_scaled; break;            \
        default: *(RwReal*)(_address) = (_value); break;                    \
        }                                                                   \
    } while (0)

static void CalcNBTSetup(const RpGameCubeVtxFmt* format,
                         RwUInt8* positionSize, RwUInt8* normalSize,
                         RwUInt8* texCoordSize, RwUInt32* savedGQR5)
{
    static const RwUInt8 elementSize[5] = {1, 1, 2, 2, 4};
    static const RwUInt8 normalFraction[5] = {0, 6, 0, 14, 0};

    *savedGQR5 = 0;
    if (format != NULL) {
        nbtPositionType = format->positionType;
        nbtNormalType = format->normalType;
        nbtTexCoordType = format->texCoordType[0];
        nbtPositionFraction = format->positionFraction;
        nbtNormalFraction = normalFraction[format->normalType];
        nbtTexCoordFraction = format->texCoordFraction[0];
        *positionSize = elementSize[format->positionType];
        *normalSize = elementSize[format->normalType];
        *texCoordSize = elementSize[format->texCoordType[0]];
    } else {
        nbtPositionType = nbtNormalType = nbtTexCoordType = 4;
        nbtPositionFraction = nbtNormalFraction = nbtTexCoordFraction = 0;
        *positionSize = *normalSize = *texCoordSize = sizeof(RwReal);
    }
}

static void CalcNBTRestore(RwUInt32 savedGQR5)
{
    (void)savedGQR5;
}

#define SET_NBT_POINTERS(_data, _positionBase, _normalBase, _texCoordBase,    \
                         _first, _second, _third)                            \
    do {                                                                     \
        (_data)->positions[0] = (_positionBase) +                            \
            (_first) * (_data)->positionSize * 3;                            \
        (_data)->positions[1] = (_positionBase) +                            \
            (_second) * (_data)->positionSize * 3;                           \
        (_data)->positions[2] = (_positionBase) +                            \
            (_third) * (_data)->positionSize * 3;                            \
        (_data)->normal = (_normalBase) +                                    \
            (_first) * (_data)->normalSize * 9;                              \
        (_data)->texCoords[0] = (_texCoordBase) +                            \
            (_first) * (_data)->texCoordSize * 2;                            \
        (_data)->texCoords[1] = (_texCoordBase) +                            \
            (_second) * (_data)->texCoordSize * 2;                           \
        (_data)->texCoords[2] = (_texCoordBase) +                            \
            (_third) * (_data)->texCoordSize * 2;                            \
    } while (0)

static void TriStripNBTDataSetup8(NBTCalcData* data, const RwUInt8* indices,
                                  RwUInt32 stride,
                                  const RwUInt8* positionBase,
                                  RwUInt8* normalBase,
                                  const RwUInt8* texCoordBase,
                                  RwUInt32 vertex)
{
    RwUInt32 first = indices[vertex * stride];
    RwUInt32 second;
    RwUInt32 third;

    if (vertex == 0) {
        second = indices[stride];
        third = indices[2 * stride];
    } else if (vertex == 1) {
        second = indices[2 * stride];
        third = indices[0];
    } else {
        second = indices[(vertex - 2) * stride];
        third = indices[(vertex - 1) * stride];
        if (second == third) {
            RwUInt32 next = indices[(vertex + 1) * stride];
            if (first == next) {
                vertex++;
                second = indices[(vertex + 1) * stride];
                if (first == second) {
                    vertex++;
                    second = indices[(vertex + 1) * stride];
                }
                third = indices[(vertex + 2) * stride];
            } else {
                second = third;
                third = next;
            }
        }
        if ((vertex & 1) != 0) {
            RwUInt32 swap = second;
            second = third;
            third = swap;
        }
    }
    SET_NBT_POINTERS(data, positionBase, normalBase, texCoordBase,
                     first, second, third);
}

static void TriStripNBTDataSetup16(NBTCalcData* data, const RwUInt8* records,
                                   RwUInt32 stride,
                                   const RwUInt8* positionBase,
                                   RwUInt8* normalBase,
                                   const RwUInt8* texCoordBase,
                                   RwUInt32 vertex)
{
#define INDEX16(_vertex) (*(const RwUInt16*)(records + (_vertex) * stride))
    RwUInt32 first = INDEX16(vertex);
    RwUInt32 second;
    RwUInt32 third;

    if (vertex == 0) {
        second = INDEX16(1); third = INDEX16(2);
    } else if (vertex == 1) {
        second = INDEX16(2); third = INDEX16(0);
    } else {
        second = INDEX16(vertex - 2); third = INDEX16(vertex - 1);
        if (second == third) {
            RwUInt32 next = INDEX16(vertex + 1);
            if (first == next) {
                vertex++;
                second = INDEX16(vertex + 1);
                if (first == second) {
                    vertex++;
                    second = INDEX16(vertex + 1);
                }
                third = INDEX16(vertex + 2);
            } else {
                second = third; third = next;
            }
        }
        if ((vertex & 1) != 0) {
            RwUInt32 swap = second; second = third; third = swap;
        }
    }
    SET_NBT_POINTERS(data, positionBase, normalBase, texCoordBase,
                     first, second, third);
#undef INDEX16
}

static void TriListNBTDataSetup8(NBTCalcData* data, const RwUInt8* indices,
                                 RwUInt32 stride,
                                 const RwUInt8* positionBase,
                                 RwUInt8* normalBase,
                                 const RwUInt8* texCoordBase,
                                 RwUInt32 vertex)
{
    RwUInt32 first = indices[vertex * stride];
    RwUInt32 second;
    RwUInt32 third;
    switch (vertex % 3) {
    case 0: second = indices[(vertex + 1) * stride];
            third = indices[(vertex + 2) * stride]; break;
    case 1: second = indices[(vertex + 1) * stride];
            third = indices[(vertex - 1) * stride]; break;
    default: second = indices[(vertex - 2) * stride];
             third = indices[(vertex - 1) * stride]; break;
    }
    SET_NBT_POINTERS(data, positionBase, normalBase, texCoordBase,
                     first, second, third);
}

static void TriListNBTDataSetup16(NBTCalcData* data, const RwUInt8* records,
                                  RwUInt32 stride,
                                  const RwUInt8* positionBase,
                                  RwUInt8* normalBase,
                                  const RwUInt8* texCoordBase,
                                  RwUInt32 vertex)
{
#define INDEX16(_vertex) (*(const RwUInt16*)(records + (_vertex) * stride))
    RwUInt32 first = INDEX16(vertex);
    RwUInt32 second;
    RwUInt32 third;
    switch (vertex % 3) {
    case 0: second = INDEX16(vertex + 1); third = INDEX16(vertex + 2); break;
    case 1: second = INDEX16(vertex + 1); third = INDEX16(vertex - 1); break;
    default: second = INDEX16(vertex - 2); third = INDEX16(vertex - 1); break;
    }
    SET_NBT_POINTERS(data, positionBase, normalBase, texCoordBase,
                     first, second, third);
#undef INDEX16
}

void CalcNBT(NBTCalcData* data);

static void CalcMeshNBTs(RwGameCubeVertexBuffer* vertexBuffer,
                         const RwGameCubeDisplayList* displayList,
                         const RpGameCubeVtxFmt* format)
{
    RwUInt8 positionSize, normalSize, texCoordSize;
    RwUInt32 savedGQR5;
    RwUInt32 recordStride = 0;
    RwUInt32 arrayIndex;
    const RwUInt8* command = displayList->data;
    const RwUInt8* end = command + displayList->size;
    const RwUInt8* positionBase = vertexBuffer->arrays[0].data;
    RwUInt8* normalBase = vertexBuffer->arrays[1].data;
    const RwUInt8* texCoordBase = NULL;
    RwBool index16 = vertexBuffer->arrays[0].descriptor == 3;
    NBTCalcData data;

    for (arrayIndex = 0; arrayIndex < vertexBuffer->numArrays; arrayIndex++) {
        const RwGameCubeVertexArray* array = &vertexBuffer->arrays[arrayIndex];
        recordStride += array->descriptor == 3 ? 2 : 1;
        if (array->attribute == 13)
            texCoordBase = array->data;
    }
    CalcNBTSetup(format, &positionSize, &normalSize, &texCoordSize,
                 &savedGQR5);
    data.positionSize = positionSize;
    data.normalSize = normalSize;
    data.texCoordSize = texCoordSize;

    while (command < end && *command != 0) {
        RwUInt8 primitive = *command++;
        RwUInt32 count = *(const RwUInt16*)command;
        const RwUInt8* records;
        RwUInt32 vertex;
        command += 2;
        records = command;
        for (vertex = 0; vertex < count; vertex++) {
            RwUInt32 index = index16
                ? *(const RwUInt16*)(records + vertex * recordStride)
                : records[vertex * recordStride];
            RwUInt8* tangent = normalBase +
                index * normalSize * 9 + normalSize * 3;
            RwBool unset;
            if (normalSize == 1)
                unset = *(RwUInt8*)tangent == 0xFF;
            else if (normalSize == 2)
                unset = *(RwUInt16*)tangent == 0xFFFF;
            else
                unset = *(RwReal*)tangent == 3.4028235e38f;
            if (unset) {
                if (primitive == 0x98) {
                    if (index16)
                        TriStripNBTDataSetup16(&data, records, recordStride,
                            positionBase, normalBase, texCoordBase, vertex);
                    else
                        TriStripNBTDataSetup8(&data, records, recordStride,
                            positionBase, normalBase, texCoordBase, vertex);
                } else {
                    if (index16)
                        TriListNBTDataSetup16(&data, records, recordStride,
                            positionBase, normalBase, texCoordBase, vertex);
                    else
                        TriListNBTDataSetup8(&data, records, recordStride,
                            positionBase, normalBase, texCoordBase, vertex);
                }
                CalcNBT(&data);
            }
        }
        command += count * recordStride;
    }
    CalcNBTRestore(savedGQR5);
}

void _rpGameCubeMTPipeDataCalcNBTs(NBTInstanceData* instanceData,
                                   const RpGameCubeVtxFmt* format,
                                   RwInt32 numVertices)
{
    RwGameCubeVertexBuffer* vertexBuffer =
        &((NBTResourceEntry*)instanceData->resourceEntry)->vertexBuffer;
    RwGameCubeDisplayList* displayLists =
        (RwGameCubeDisplayList*)&vertexBuffer->arrays[vertexBuffer->numArrays];
    RpMesh* mesh = (RpMesh*)(instanceData->meshHeader + 1);
    RwUInt32 meshIndex;
    RwUInt8* normalBase = vertexBuffer->arrays[1].data;

    if (format != NULL && format->normalType == 1) {
        RwInt32 i;
        for (i = 0; i < numVertices; i++) normalBase[i * 9 + 3] = 0xFF;
    } else if (format != NULL && format->normalType == 3) {
        RwInt32 i;
        for (i = 0; i < numVertices; i++)
            *(RwUInt16*)(normalBase + i * 18 + 6) = 0xFFFF;
    } else {
        RwInt32 i;
        for (i = 0; i < numVertices; i++)
            *(RwReal*)(normalBase + i * 36 + 12) = 3.4028235e38f;
    }

    for (meshIndex = 0; meshIndex < (RwUInt32)instanceData->meshHeader->numMeshes;
         meshIndex++, mesh++) {
        RpMultiTexture* multiTexture =
            RpMaterialGetMultiTexture(mesh->material, 6);
        if (multiTexture != NULL && multiTexture->effect != NULL &&
            multiTexture->effect->type == 6) {
            RpGameCubeMTEffectConfigPrivate* config =
                (RpGameCubeMTEffectConfigPrivate*)
                RpGameCubeMTEffectGetConfig(multiTexture->effect);
            RwUInt32 entryIndex;
            for (entryIndex = 0; entryIndex < config->count24; entryIndex++) {
                RpGameCubeMTEntry24Private* entry =
                    &config->entries24[entryIndex];
                if (entry->value[2] == 2 || entry->value[2] == 3 ||
                    (entry->value[1] >= 2 && entry->value[1] <= 9)) {
                    CalcMeshNBTs(vertexBuffer, &displayLists[meshIndex],
                                 format);
                    break;
                }
            }
        }
    }
    DCFlushRange(normalBase, numVertices * vertexBuffer->arrays[1].stride);
    GXInvalidateVtxCache();
}

RwBool _rpGameCubeMTPipeDataQueryNBTs(const NBTInstanceData* instanceData)
{
    /* Retail retains an otherwise-unused copy of the current mesh after each
     * iteration. Clean C omits that dead lifetime and its wider save range. */
    const RpMesh* mesh = (const RpMesh*)(instanceData->meshHeader + 1);
    RwUInt32 i;
    for (i = 0; i < (RwUInt32)instanceData->meshHeader->numMeshes; i++, mesh++) {
        RpMultiTexture* multiTexture =
            RpMaterialGetMultiTexture(mesh->material, 6);
        if (multiTexture != NULL && multiTexture->effect != NULL &&
            multiTexture->effect->type == 6) {
            RpGameCubeMTEffectConfigPrivate* config =
                (RpGameCubeMTEffectConfigPrivate*)
                RpGameCubeMTEffectGetConfig(multiTexture->effect);
            RwUInt32 entryIndex;
            for (entryIndex = 0; entryIndex < config->count24; entryIndex++) {
                RpGameCubeMTEntry24Private* entry =
                    &config->entries24[entryIndex];
                if (entry->value[2] == 2 || entry->value[2] == 3 ||
                    (entry->value[1] >= 2 && entry->value[1] <= 9))
                    return TRUE;
            }
        }
    }
    return FALSE;
}

void CalcNBT(NBTCalcData* data)
{
    /* Retail is a single paired-single kernel using GQR5/6/7. The scalar body
     * below preserves the same packed conversions and tangent-frame math. */
    RwV3d positions[3];
    RwTexCoords texCoords[3];
    RwV3d normal;
    RwV3d edge1, edge2, tangent, binormal;
    RwUInt8* output;
    RwReal du1, dv1, du2, dv2, projection, length;
    RwUInt32 i;

    for (i = 0; i < 3; i++) {
        READ_SCALAR(positions[i].x, data->positions[i], nbtPositionType,
                    nbtPositionFraction);
        READ_SCALAR(positions[i].y, data->positions[i] + data->positionSize,
                    nbtPositionType, nbtPositionFraction);
        READ_SCALAR(positions[i].z, data->positions[i] + data->positionSize * 2,
                    nbtPositionType, nbtPositionFraction);
        READ_SCALAR(texCoords[i].u, data->texCoords[i], nbtTexCoordType,
                    nbtTexCoordFraction);
        READ_SCALAR(texCoords[i].v, data->texCoords[i] + data->texCoordSize,
                    nbtTexCoordType, nbtTexCoordFraction);
    }
    READ_SCALAR(normal.x, data->normal, nbtNormalType, nbtNormalFraction);
    READ_SCALAR(normal.y, data->normal + data->normalSize,
                nbtNormalType, nbtNormalFraction);
    READ_SCALAR(normal.z, data->normal + data->normalSize * 2,
                nbtNormalType, nbtNormalFraction);

    edge1.x = positions[1].x - positions[0].x;
    edge1.y = positions[1].y - positions[0].y;
    edge1.z = positions[1].z - positions[0].z;
    edge2.x = positions[2].x - positions[0].x;
    edge2.y = positions[2].y - positions[0].y;
    edge2.z = positions[2].z - positions[0].z;
    du1 = texCoords[1].u - texCoords[0].u;
    dv1 = texCoords[1].v - texCoords[0].v;
    du2 = texCoords[2].u - texCoords[0].u;
    dv2 = texCoords[2].v - texCoords[0].v;
    tangent.x = edge2.x * dv1 - edge1.x * dv2;
    tangent.y = edge2.y * dv1 - edge1.y * dv2;
    tangent.z = edge2.z * dv1 - edge1.z * dv2;
    projection = normal.x * tangent.x + normal.y * tangent.y +
                 normal.z * tangent.z;
    tangent.x -= normal.x * projection;
    tangent.y -= normal.y * projection;
    tangent.z -= normal.z * projection;
    length = _rwSqrt(tangent.x * tangent.x + tangent.y * tangent.y +
                     tangent.z * tangent.z);
    if (length > 0.0f) {
        tangent.x /= length; tangent.y /= length; tangent.z /= length;
    }
    binormal.x = normal.y * tangent.z - normal.z * tangent.y;
    binormal.y = normal.z * tangent.x - normal.x * tangent.z;
    binormal.z = normal.x * tangent.y - normal.y * tangent.x;
    if (du1 * dv2 - dv1 * du2 < 0.0f) {
        tangent.x = -tangent.x; tangent.y = -tangent.y;
        tangent.z = -tangent.z;
    }

    output = data->normal + data->normalSize * 3;
    WRITE_SCALAR(output, tangent.x, nbtNormalType, nbtNormalFraction);
    WRITE_SCALAR(output + data->normalSize, tangent.y,
                 nbtNormalType, nbtNormalFraction);
    WRITE_SCALAR(output + data->normalSize * 2, tangent.z,
                 nbtNormalType, nbtNormalFraction);
    output += data->normalSize * 3;
    WRITE_SCALAR(output, binormal.x, nbtNormalType, nbtNormalFraction);
    WRITE_SCALAR(output + data->normalSize, binormal.y,
                 nbtNormalType, nbtNormalFraction);
    WRITE_SCALAR(output + data->normalSize * 2, binormal.z,
                 nbtNormalType, nbtNormalFraction);
}
