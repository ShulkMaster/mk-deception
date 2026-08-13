#include "dolphin/cache.h"
#include "dolphin/gx.h"
#include "rw/gamecube.h"
#include "rw/nodegamecube.h"
#include "rw/rpmatfx.h"
#include "rw/rpworld_types.h"
#include "rw/rtquat.h"
#include "rw/rwresources.h"

typedef struct NBTResourceEntry {
    RwResEntry entry;
    unsigned short token;
    unsigned short meshSerialNum;
    RwGameCubeVertexBuffer vertexBuffer;
} NBTResourceEntry;

typedef struct NBTCalcData {
    const unsigned char* positions[3];
    unsigned char* normal;
    const unsigned char* texCoords[3];
    unsigned char positionSize;
    unsigned char normalSize;
    unsigned char texCoordSize;
    unsigned char reserved;
} NBTCalcData;

typedef char NBTCalcDataSizeCheck[sizeof(NBTCalcData) == 0x20 ? 1 : -1];

static unsigned char nbtPositionType;
static unsigned char nbtNormalType;
static unsigned char nbtTexCoordType;
static unsigned char nbtPositionFraction;
static unsigned char nbtNormalFraction;
static unsigned char nbtTexCoordFraction;

static float ReadScalar(const unsigned char* address, unsigned char type,
                         unsigned char fraction);
static void WriteScalar(unsigned char* address, float value, unsigned char type,
                        unsigned char fraction);

static void CalcNBTSetup(const RpGameCubeVtxFmt* format,
                         unsigned char* positionSize, unsigned char* normalSize,
                         unsigned char* texCoordSize, unsigned int* savedGQR5)
{
    static const unsigned char elementSize[5] = {1, 1, 2, 2, 4};
    static const unsigned char normalFraction[5] = {0, 6, 0, 14, 0};

    *savedGQR5 = 0;
    if (format != 0) {
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
        *positionSize = *normalSize = *texCoordSize = sizeof(float);
    }
}

static void CalcNBTRestore(unsigned int savedGQR5)
{
    (void)savedGQR5;
}

static void SetNBTPointers(NBTCalcData* data, const unsigned char* positionBase,
                           unsigned char* normalBase,
                           const unsigned char* texCoordBase, unsigned int first,
                           unsigned int second, unsigned int third);

static void TriStripNBTDataSetup8(NBTCalcData* data, const unsigned char* indices,
                                  unsigned int stride,
                                  const unsigned char* positionBase,
                                  unsigned char* normalBase,
                                  const unsigned char* texCoordBase,
                                  unsigned int vertex)
{
    unsigned int first = indices[vertex * stride];
    unsigned int second;
    unsigned int third;

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
            unsigned int next = indices[(vertex + 1) * stride];
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
            unsigned int swap = second;
            second = third;
            third = swap;
        }
    }
    SetNBTPointers(data, positionBase, normalBase, texCoordBase,
                   first, second, third);
}

static void TriStripNBTDataSetup16(NBTCalcData* data, const unsigned char* records,
                                   unsigned int stride,
                                   const unsigned char* positionBase,
                                   unsigned char* normalBase,
                                   const unsigned char* texCoordBase,
                                   unsigned int vertex)
{
    unsigned int first = *(const unsigned short*)(records + vertex * stride);
    unsigned int second;
    unsigned int third;

    if (vertex == 0) {
        second = *(const unsigned short*)(records + stride);
        third = *(const unsigned short*)(records + 2 * stride);
    } else if (vertex == 1) {
        second = *(const unsigned short*)(records + 2 * stride);
        third = *(const unsigned short*)records;
    } else {
        second = *(const unsigned short*)(records + (vertex - 2) * stride);
        third = *(const unsigned short*)(records + (vertex - 1) * stride);
        if (second == third) {
            unsigned int next =
                *(const unsigned short*)(records + (vertex + 1) * stride);
            if (first == next) {
                vertex++;
                second =
                    *(const unsigned short*)(records + (vertex + 1) * stride);
                if (first == second) {
                    vertex++;
                    second =
                        *(const unsigned short*)(records + (vertex + 1) * stride);
                }
                third =
                    *(const unsigned short*)(records + (vertex + 2) * stride);
            } else {
                second = third; third = next;
            }
        }
        if ((vertex & 1) != 0) {
            unsigned int swap = second; second = third; third = swap;
        }
    }
    SetNBTPointers(data, positionBase, normalBase, texCoordBase,
                   first, second, third);
}

static void TriListNBTDataSetup8(NBTCalcData* data, const unsigned char* indices,
                                 unsigned int stride,
                                 const unsigned char* positionBase,
                                 unsigned char* normalBase,
                                 const unsigned char* texCoordBase,
                                 unsigned int vertex)
{
    unsigned int first = indices[vertex * stride];
    unsigned int second;
    unsigned int third;
    switch (vertex % 3) {
    case 0: second = indices[(vertex + 1) * stride];
            third = indices[(vertex + 2) * stride]; break;
    case 1: second = indices[(vertex + 1) * stride];
            third = indices[(vertex - 1) * stride]; break;
    default: second = indices[(vertex - 2) * stride];
             third = indices[(vertex - 1) * stride]; break;
    }
    SetNBTPointers(data, positionBase, normalBase, texCoordBase,
                   first, second, third);
}

static void TriListNBTDataSetup16(NBTCalcData* data, const unsigned char* records,
                                  unsigned int stride,
                                  const unsigned char* positionBase,
                                  unsigned char* normalBase,
                                  const unsigned char* texCoordBase,
                                  unsigned int vertex)
{
    unsigned int first = *(const unsigned short*)(records + vertex * stride);
    unsigned int second;
    unsigned int third;
    switch (vertex % 3) {
    case 0:
        second = *(const unsigned short*)(records + (vertex + 1) * stride);
        third = *(const unsigned short*)(records + (vertex + 2) * stride);
        break;
    case 1:
        second = *(const unsigned short*)(records + (vertex + 1) * stride);
        third = *(const unsigned short*)(records + (vertex - 1) * stride);
        break;
    default:
        second = *(const unsigned short*)(records + (vertex - 2) * stride);
        third = *(const unsigned short*)(records + (vertex - 1) * stride);
        break;
    }
    SetNBTPointers(data, positionBase, normalBase, texCoordBase,
                   first, second, third);
}

void CalcNBT(NBTCalcData* data);

static void CalcMeshNBTs(RwGameCubeVertexBuffer* vertexBuffer,
                         const RwGameCubeDisplayList* displayList,
                         const RpGameCubeVtxFmt* format)
{
    unsigned char positionSize, normalSize, texCoordSize;
    unsigned int savedGQR5;
    unsigned int recordStride = 0;
    unsigned int arrayIndex;
    const unsigned char* command = displayList->data;
    const unsigned char* end = command + displayList->size;
    const unsigned char* positionBase = vertexBuffer->arrays[0].data;
    unsigned char* normalBase = vertexBuffer->arrays[1].data;
    const unsigned char* texCoordBase = 0;
    int index16 = vertexBuffer->arrays[0].descriptor == 3;
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
        unsigned char primitive = *command++;
        unsigned int count = *(const unsigned short*)command;
        const unsigned char* records;
        unsigned int vertex;
        command += 2;
        records = command;
        for (vertex = 0; vertex < count; vertex++) {
            unsigned int index = index16
                ? *(const unsigned short*)(records + vertex * recordStride)
                : records[vertex * recordStride];
            unsigned char* tangent = normalBase +
                index * normalSize * 9 + normalSize * 3;
            int unset;
            if (normalSize == 1)
                unset = *(unsigned char*)tangent == 0xFF;
            else if (normalSize == 2)
                unset = *(unsigned short*)tangent == 0xFFFF;
            else
                unset = *(float*)tangent == 3.4028235e38f;
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

void _rpGameCubeMTPipeDataCalcNBTs(
    RxGameCubeAtomicAllInOneInstanceData* instanceData,
    const RpGameCubeVtxFmt* format, int numVertices)
{
    RwGameCubeVertexBuffer* vertexBuffer =
        &((NBTResourceEntry*)instanceData->resourceEntry)->vertexBuffer;
    RwGameCubeDisplayList* displayLists =
        (RwGameCubeDisplayList*)&vertexBuffer->arrays[vertexBuffer->numArrays];
    RpMesh* mesh = (RpMesh*)(instanceData->meshHeader + 1);
    unsigned int meshIndex;
    unsigned char* normalBase = vertexBuffer->arrays[1].data;

    if (format != 0 && format->normalType == 1) {
        int i;
        for (i = 0; i < numVertices; i++) normalBase[i * 9 + 3] = 0xFF;
    } else if (format != 0 && format->normalType == 3) {
        int i;
        for (i = 0; i < numVertices; i++)
            *(unsigned short*)(normalBase + i * 18 + 6) = 0xFFFF;
    } else {
        int i;
        for (i = 0; i < numVertices; i++)
            *(float*)(normalBase + i * 36 + 12) = 3.4028235e38f;
    }

    for (meshIndex = 0; meshIndex < (unsigned int)instanceData->meshHeader->numMeshes;
         meshIndex++, mesh++) {
        RpMultiTexture* multiTexture =
            RpMaterialGetMultiTexture(mesh->material, 6);
        if (multiTexture != 0 && multiTexture->effect != 0 &&
            multiTexture->effect->type == 6) {
            RpGameCubeMTEffectConfig* config =
                RpGameCubeMTEffectGetConfig(multiTexture->effect);
            unsigned int entryIndex;
            for (entryIndex = 0; entryIndex < config->count24; entryIndex++) {
                RpGameCubeMTEntry24* entry =
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

int _rpGameCubeMTPipeDataQueryNBTs(
    const RxGameCubeAtomicAllInOneInstanceData* instanceData)
{


    const RpMesh* mesh = (const RpMesh*)(instanceData->meshHeader + 1);
    unsigned int i;
    for (i = 0; i < (unsigned int)instanceData->meshHeader->numMeshes; i++, mesh++) {
        RpMultiTexture* multiTexture =
            RpMaterialGetMultiTexture(mesh->material, 6);
        if (multiTexture != 0 && multiTexture->effect != 0 &&
            multiTexture->effect->type == 6) {
            RpGameCubeMTEffectConfig* config =
                RpGameCubeMTEffectGetConfig(multiTexture->effect);
            unsigned int entryIndex;
            for (entryIndex = 0; entryIndex < config->count24; entryIndex++) {
                RpGameCubeMTEntry24* entry =
                    &config->entries24[entryIndex];
                if (entry->value[2] == 2 || entry->value[2] == 3 ||
                    (entry->value[1] >= 2 && entry->value[1] <= 9))
                    return 1;
            }
        }
    }
    return 0;
}

void CalcNBT(NBTCalcData* data)
{


    RwV3d positions[3];
    RwTexCoords texCoords[3];
    RwV3d normal;
    RwV3d edge1, edge2, tangent, binormal;
    unsigned char* output;
    float du1, dv1, du2, dv2, projection, length;
    unsigned int i;

    for (i = 0; i < 3; i++) {
        positions[i].x = ReadScalar(data->positions[i], nbtPositionType,
                                    nbtPositionFraction);
        positions[i].y = ReadScalar(data->positions[i] + data->positionSize,
                                    nbtPositionType, nbtPositionFraction);
        positions[i].z = ReadScalar(
            data->positions[i] + data->positionSize * 2, nbtPositionType,
            nbtPositionFraction);
        texCoords[i].u = ReadScalar(data->texCoords[i], nbtTexCoordType,
                                    nbtTexCoordFraction);
        texCoords[i].v = ReadScalar(data->texCoords[i] + data->texCoordSize,
                                    nbtTexCoordType, nbtTexCoordFraction);
    }
    normal.x = ReadScalar(data->normal, nbtNormalType, nbtNormalFraction);
    normal.y = ReadScalar(data->normal + data->normalSize, nbtNormalType,
                          nbtNormalFraction);
    normal.z = ReadScalar(data->normal + data->normalSize * 2, nbtNormalType,
                          nbtNormalFraction);

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
    WriteScalar(output, tangent.x, nbtNormalType, nbtNormalFraction);
    WriteScalar(output + data->normalSize, tangent.y, nbtNormalType,
                nbtNormalFraction);
    WriteScalar(output + data->normalSize * 2, tangent.z, nbtNormalType,
                nbtNormalFraction);
    output += data->normalSize * 3;
    WriteScalar(output, binormal.x, nbtNormalType, nbtNormalFraction);
    WriteScalar(output + data->normalSize, binormal.y, nbtNormalType,
                nbtNormalFraction);
    WriteScalar(output + data->normalSize * 2, binormal.z, nbtNormalType,
                nbtNormalFraction);
}

static float ReadScalar(const unsigned char* address, unsigned char type,
                         unsigned char fraction)
{
    float scale = (float)(1U << fraction);

    switch (type) {
    case 0: return *(const unsigned char*)address / scale;
    case 1: return *(const signed char*)address / scale;
    case 2: return *(const unsigned short*)address / scale;
    case 3: return *(const short*)address / scale;
    default: return *(const float*)address;
    }
}

static void WriteScalar(unsigned char* address, float value, unsigned char type,
                        unsigned char fraction)
{
    float scaled = value * (float)(1U << fraction);

    switch (type) {
    case 0: *(unsigned char*)address = (unsigned char)scaled; break;
    case 1: *(signed char*)address = (signed char)scaled; break;
    case 2: *(unsigned short*)address = (unsigned short)scaled; break;
    case 3: *(short*)address = (short)scaled; break;
    default: *(float*)address = value; break;
    }
}

static void SetNBTPointers(NBTCalcData* data, const unsigned char* positionBase,
                           unsigned char* normalBase,
                           const unsigned char* texCoordBase, unsigned int first,
                           unsigned int second, unsigned int third)
{
    data->positions[0] = positionBase + first * data->positionSize * 3;
    data->positions[1] = positionBase + second * data->positionSize * 3;
    data->positions[2] = positionBase + third * data->positionSize * 3;
    data->normal = normalBase + first * data->normalSize * 9;
    data->texCoords[0] = texCoordBase + first * data->texCoordSize * 2;
    data->texCoords[1] = texCoordBase + second * data->texCoordSize * 2;
    data->texCoords[2] = texCoordBase + third * data->texCoordSize * 2;
}
