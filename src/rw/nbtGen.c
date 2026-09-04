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

static float rpReadNbtScalar(const unsigned char* address, unsigned char type,
                         unsigned char fraction);
static void rpWriteNbtScalar(unsigned char* address, float value, unsigned char type,
                        unsigned char fraction);

static void CalcNBTSetup(const RpGameCubeVtxFmt* format,
                         unsigned char* positionSize, unsigned char* normalSize,
                         unsigned char* texCoordSize, unsigned int* savedGQR5)
{
    /* TODO: Retail also programs GQR5 for the paired-single CalcNBT kernel;
     * this portable setup preserves its format and element-size semantics. */
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
    /* TODO: Retail restores GQR5 for its paired-single CalcNBT kernel. The
     * portable implementation does not modify or consume GQR state. */
    (void)savedGQR5;
}

#define rpPrepareNbtSourcePointers(data, positionBase, normalBase, texCoordBase, first, \
                         second, third)                                        \
    do {                                                                       \
        (data)->positions[0] = (positionBase) +                                \
            (first) * ((data)->positionSize * 3);                              \
        (data)->positions[1] = (positionBase) +                                \
            (second) * ((data)->positionSize * 3);                             \
        (data)->positions[2] = (positionBase) +                                \
            (third) * ((data)->positionSize * 3);                              \
        (data)->normal = (normalBase) +                                        \
            (first) * ((data)->normalSize * 9);                                \
        (data)->texCoords[0] = (texCoordBase) +                                \
            (first) * ((data)->texCoordSize * 2);                              \
        (data)->texCoords[1] = (texCoordBase) +                                \
            (second) * ((data)->texCoordSize * 2);                             \
        (data)->texCoords[2] = (texCoordBase) +                                \
            (third) * ((data)->texCoordSize * 2);                              \
    } while (0)

static void TriStripNBTDataSetup8(NBTCalcData* data, const unsigned char* indices,
                                  unsigned int stride,
                                  const unsigned char* positionBase,
                                  unsigned char* normalBase,
                                  const unsigned char* texCoordBase,
                                  int vertex)
{
    unsigned char first = indices[stride * vertex];
    unsigned char second;
    unsigned char third;

    if (vertex < 2) {
        if (vertex == 0) {
            second = indices[stride];
            third = indices[2 * stride];
        } else {
            second = indices[2 * stride];
            third = indices[0];
        }
    } else {
        second = indices[stride * (vertex - 2)];
        third = indices[stride * (vertex - 1)];
        if (second == third) {
            unsigned char next = indices[stride * (vertex + 1)];
            if (first == next) {
                vertex++;
                second = indices[stride * (vertex + 1)];
                if (first == second) {
                    vertex++;
                    second = indices[stride * (vertex + 1)];
                }
                third = indices[stride * (vertex + 2)];
            } else {
                second = third;
                third = next;
            }
        }
        if ((vertex & 1) != 0) {
            unsigned char swap = second;
            second = third;
            third = swap;
        }
    }
    rpPrepareNbtSourcePointers(data, positionBase, normalBase, texCoordBase,
                     first, second, third);
}

static void TriStripNBTDataSetup16(NBTCalcData* data, const unsigned char* records,
                                   unsigned int stride,
                                   const unsigned char* positionBase,
                                   unsigned char* normalBase,
                                   const unsigned char* texCoordBase,
                                   int vertex)
{
    unsigned short first =
        *(const unsigned short*)(records + stride * vertex);
    unsigned short second;
    unsigned short third;

    if (vertex < 2) {
        if (vertex == 0) {
            second = *(const unsigned short*)(records + stride);
            third = *(const unsigned short*)(records + 2 * stride);
        } else {
            second = *(const unsigned short*)(records + 2 * stride);
            third = *(const unsigned short*)records;
        }
    } else {
        second = *(const unsigned short*)(records + stride * (vertex - 2));
        third = *(const unsigned short*)(records + stride * (vertex - 1));
        if (second == third) {
            unsigned short next =
                *(const unsigned short*)(records + stride * (vertex + 1));
            if (first == next) {
                vertex++;
                second =
                    *(const unsigned short*)(records + stride * (vertex + 1));
                if (first == second) {
                    vertex++;
                    second =
                        *(const unsigned short*)(records +
                                                 stride * (vertex + 1));
                }
                third =
                    *(const unsigned short*)(records + stride * (vertex + 2));
            } else {
                second = third;
                third = next;
            }
        }
        if ((vertex & 1) != 0) {
            unsigned short swap = second;
            second = third;
            third = swap;
        }
    }
    rpPrepareNbtSourcePointers(data, positionBase, normalBase, texCoordBase,
                     first, second, third);
}

static void TriListNBTDataSetup8(NBTCalcData* data, const unsigned char* indices,
                                 unsigned int stride,
                                 const unsigned char* positionBase,
                                 unsigned char* normalBase,
                                 const unsigned char* texCoordBase,
                                 int vertex)
{
    unsigned char first = indices[stride * vertex];
    unsigned char second;
    unsigned char third;
    int verticesPerTriangle = 3;
    switch (vertex % verticesPerTriangle) {
    case 0: second = indices[stride * (vertex + 1)];
            third = indices[stride * (vertex + 2)]; break;
    case 1: second = indices[stride * (vertex + 1)];
            third = indices[stride * (vertex - 1)]; break;
    case 2: second = indices[stride * (vertex - 2)];
            third = indices[stride * (vertex - 1)]; break;
    default: return;
    }
    rpPrepareNbtSourcePointers(data, positionBase, normalBase, texCoordBase,
                     first, second, third);
}

static void TriListNBTDataSetup16(NBTCalcData* data, const unsigned char* records,
                                  unsigned int stride,
                                  const unsigned char* positionBase,
                                  unsigned char* normalBase,
                                  const unsigned char* texCoordBase,
                                  int vertex)
{
    unsigned short first =
        *(const unsigned short*)(records + stride * vertex);
    unsigned short second;
    unsigned short third;
    int verticesPerTriangle = 3;
    switch (vertex % verticesPerTriangle) {
    case 0:
        second = *(const unsigned short*)(records + stride * (vertex + 1));
        third = *(const unsigned short*)(records + stride * (vertex + 2));
        break;
    case 1:
        second = *(const unsigned short*)(records + stride * (vertex + 1));
        third = *(const unsigned short*)(records + stride * (vertex - 1));
        break;
    case 2:
        second = *(const unsigned short*)(records + stride * (vertex - 2));
        third = *(const unsigned short*)(records + stride * (vertex - 1));
        break;
    default: return;
    }
    rpPrepareNbtSourcePointers(data, positionBase, normalBase, texCoordBase,
                     first, second, third);
}

void CalcNBT(NBTCalcData* data);

#define rpWalkNbtCommands(setupFunction16, setupFunction8, tangentType,         \
                             tangentValue, unsetTest16, unsetTest8)             \
    do {                                                                       \
        consumed = 0;                                                          \
        while (consumed < displayList->size) {                                 \
            primitive = *command;                                              \
            command++;                                                         \
            consumed++;                                                        \
            count = *(const unsigned short*)command;                           \
            command += 2;                                                      \
            consumed += 2;                                                     \
            if (vertexBuffer->arrays[0].descriptor == 3) {                     \
                for (i = 0; i < count; i++) {                                 \
                    unsigned short index = *(const unsigned short*)(           \
                        command + recordStride * i);                           \
                    tangentType tangent = (tangentValue);                      \
                    if (unsetTest16) {                                         \
                        setupFunction16(&data, command, recordStride,          \
                            positionBase, normalBase, texCoordBase, i);        \
                        CalcNBT(&data);                                        \
                    }                                                          \
                }                                                              \
            } else {                                                           \
                for (i = 0; i < count; i++) {                                 \
                    unsigned char index = command[recordStride * i];           \
                    tangentType tangent = (tangentValue);                      \
                    if (unsetTest8) {                                          \
                        setupFunction8(&data, command, recordStride,           \
                            positionBase, normalBase, texCoordBase, i);        \
                        CalcNBT(&data);                                        \
                    }                                                          \
                }                                                              \
            }                                                                  \
            command += recordStride * count;                                   \
            consumed += recordStride * count;                                  \
            primitive = *command;                                              \
            if (primitive == 0)                                                \
                break;                                                         \
        }                                                                      \
    } while (0)

static void CalcMeshNBTs(RwGameCubeVertexBuffer* vertexBuffer,
                         const RwGameCubeDisplayList* displayList,
                         const RpGameCubeVtxFmt* format)
{
    unsigned int savedGQR5;
    unsigned int recordStride;
    unsigned int i;
    unsigned int consumed;
    unsigned char primitive;
    unsigned short count;
    const unsigned char* positionBase = vertexBuffer->arrays[0].data;
    unsigned char* normalBase = vertexBuffer->arrays[1].data;
    const unsigned char* texCoordBase;
    const unsigned char* command;
    NBTCalcData data;

    for (i = 2; vertexBuffer->arrays[i].attribute != 13; i++) {
    }
    texCoordBase = vertexBuffer->arrays[i].data;

    recordStride = 0;
    for (i = 0; i < vertexBuffer->numArrays; i++) {
        if (vertexBuffer->arrays[i].descriptor == 3)
            recordStride += 2;
        else
            recordStride++;
    }
    CalcNBTSetup(format, &data.positionSize, &data.normalSize,
                 &data.texCoordSize, &savedGQR5);
    command = displayList->data;

    if (*command == 0x98) {
        if (format != 0) {
            if (format->normalType == 1) {
                rpWalkNbtCommands(TriStripNBTDataSetup16,
                    TriStripNBTDataSetup8, unsigned char,
                    normalBase[index * 9 + 3], tangent == 0xFF,
                    tangent == 0xFF);
            } else if (format->normalType == 3) {
                rpWalkNbtCommands(TriStripNBTDataSetup16,
                    TriStripNBTDataSetup8, unsigned short,
                    *(unsigned short*)(normalBase + index * 18 + 6),
                    tangent == 0xFFFF,
                    (float)tangent == 3.4028235e38f);
            } else {
                rpWalkNbtCommands(TriStripNBTDataSetup16,
                    TriStripNBTDataSetup8, float,
                    *(float*)(normalBase + index * 36 + 12),
                    tangent == 3.4028235e38f, tangent == 3.4028235e38f);
            }
        } else {
            rpWalkNbtCommands(TriStripNBTDataSetup16,
                TriStripNBTDataSetup8, float,
                *(float*)(normalBase + index * 36 + 12),
                tangent == 3.4028235e38f, tangent == 3.4028235e38f);
        }
    } else {
        if (format != 0) {
            if (format->normalType == 1) {
                rpWalkNbtCommands(TriListNBTDataSetup16,
                    TriListNBTDataSetup8, unsigned char,
                    normalBase[index * 9 + 3], tangent == 0xFF,
                    tangent == 0xFF);
            } else if (format->normalType == 3) {
                rpWalkNbtCommands(TriListNBTDataSetup16,
                    TriListNBTDataSetup8, unsigned short,
                    *(unsigned short*)(normalBase + index * 18 + 6),
                    tangent == 0xFFFF,
                    (float)tangent == 3.4028235e38f);
            } else {
                rpWalkNbtCommands(TriListNBTDataSetup16,
                    TriListNBTDataSetup8, float,
                    *(float*)(normalBase + index * 36 + 12),
                    tangent == 3.4028235e38f, tangent == 3.4028235e38f);
            }
        } else {
            rpWalkNbtCommands(TriListNBTDataSetup16,
                TriListNBTDataSetup8, float,
                *(float*)(normalBase + index * 36 + 12),
                tangent == 3.4028235e38f, tangent == 3.4028235e38f);
        }
    }
    CalcNBTRestore(savedGQR5);
}

#undef rpWalkNbtCommands

void _rpGameCubeMTPipeDataCalcNBTs(
    RxGameCubeAtomicAllInOneInstanceData* instanceData,
    const RpGameCubeVtxFmt* format, int numVertices)
{
    /* TODO: This initializes uncomputed tangents, calculates NBT data for
     * material effects that consume it, then flushes the updated normals.
     * The remaining retail diff is only the local label assigned to the two
     * identical max-float constant relocations. */
    RwGameCubeVertexBuffer* vertexBuffer =
        &((NBTResourceEntry*)instanceData->resourceEntry)->vertexBuffer;
    RwGameCubeDisplayList* displayList =
        (RwGameCubeDisplayList*)((unsigned char*)(vertexBuffer + 1) +
            (vertexBuffer->numArrays - 1) * sizeof(RwGameCubeVertexArray));
    RpMeshHeader* meshHeader = instanceData->meshHeader;
    RpMesh* mesh;
    unsigned char* byteTangent;
    unsigned short* shortTangent;
    float* floatTangent;
    float* defaultTangent;
    int i;

    if (format != 0) {
        if (format->normalType == 1) {
            byteTangent = vertexBuffer->arrays[1].data;
            byteTangent += 3;

            for (i = 0; i < numVertices; i++) {
                *byteTangent = 0xFF;
                byteTangent += 9;
            }
        } else if (format->normalType == 3) {
            shortTangent = vertexBuffer->arrays[1].data;
            shortTangent = (unsigned short*)((unsigned char*)shortTangent + 6);

            for (i = 0; i < numVertices; i++) {
                *shortTangent = 0xFFFF;
                shortTangent =
                    (unsigned short*)((unsigned char*)shortTangent + 18);
            }
        } else {
            floatTangent = vertexBuffer->arrays[1].data;
            floatTangent = (float*)((unsigned char*)floatTangent + 12);

            for (i = 0; i < numVertices; i++) {
                *floatTangent = 3.4028235e38f;
                floatTangent += 9;
            }
        }
    } else {
        defaultTangent = vertexBuffer->arrays[1].data;
        defaultTangent = (float*)((unsigned char*)defaultTangent + 12);

        for (i = 0; i < numVertices; i++) {
            *defaultTangent = 3.4028235e38f;
            defaultTangent += 9;
        }
    }

    mesh = (RpMesh*)(meshHeader + 1);
    for (i = 0; i < meshHeader->numMeshes; i++) {
        RpMultiTexture* multiTexture =
            RpMaterialGetMultiTexture(mesh->material, 6);
        if (multiTexture != 0 && multiTexture->effect != 0 &&
            multiTexture->effect->type == 6) {
            RpGameCubeMTEffectConfig* config =
                RpGameCubeMTEffectGetConfig(multiTexture->effect);
            int entryIndex;
            for (entryIndex = 0; entryIndex < config->count24; entryIndex++) {
                RpGameCubeMTEntry24* entry =
                    &config->entries24[entryIndex];
                if (entry->value[2] == 2 || entry->value[2] == 3 ||
                    (entry->value[1] >= 2 && entry->value[1] <= 9)) {
                    CalcMeshNBTs(vertexBuffer, displayList, format);
                    break;
                }
            }
        }
        displayList++;
        mesh++;
    }
    DCFlushRange(vertexBuffer->arrays[1].data,
                 numVertices * vertexBuffer->arrays[1].stride);
    GXInvalidateVtxCache();
}

int _rpGameCubeMTPipeDataQueryNBTs(
    const RxGameCubeAtomicAllInOneInstanceData* instanceData)
{
    const RpMeshHeader* meshHeader = instanceData->meshHeader;
    const RpMesh* mesh = (const RpMesh*)(meshHeader + 1);
    int i;

    for (i = 0; i < meshHeader->numMeshes; i++, mesh++) {
        RpMultiTexture* multiTexture =
            RpMaterialGetMultiTexture(mesh->material, 6);
        if (multiTexture != 0 && multiTexture->effect != 0 &&
            multiTexture->effect->type == 6) {
            RpGameCubeMTEffectConfig* config =
                RpGameCubeMTEffectGetConfig(multiTexture->effect);
            int entryIndex;
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
    /* TODO: Retail implements this tangent-space calculation with paired-single
     * quantized loads, stores, and arithmetic; retain this portable semantic
     * implementation until those instructions have a supported C lowering. */
    RwV3d positions[3];
    RwTexCoords texCoords[3];
    RwV3d normal;
    RwV3d edge1, edge2, tangent, binormal;
    unsigned char* output;
    float du1, dv1, du2, dv2, projection, length;
    unsigned int i;

    for (i = 0; i < 3; i++) {
        positions[i].x = rpReadNbtScalar(data->positions[i], nbtPositionType,
                                    nbtPositionFraction);
        positions[i].y = rpReadNbtScalar(data->positions[i] + data->positionSize,
                                    nbtPositionType, nbtPositionFraction);
        positions[i].z = rpReadNbtScalar(
            data->positions[i] + data->positionSize * 2, nbtPositionType,
            nbtPositionFraction);
        texCoords[i].u = rpReadNbtScalar(data->texCoords[i], nbtTexCoordType,
                                    nbtTexCoordFraction);
        texCoords[i].v = rpReadNbtScalar(data->texCoords[i] + data->texCoordSize,
                                    nbtTexCoordType, nbtTexCoordFraction);
    }
    normal.x = rpReadNbtScalar(data->normal, nbtNormalType, nbtNormalFraction);
    normal.y = rpReadNbtScalar(data->normal + data->normalSize, nbtNormalType,
                          nbtNormalFraction);
    normal.z = rpReadNbtScalar(data->normal + data->normalSize * 2, nbtNormalType,
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
        tangent.x /= length;
        tangent.y /= length;
        tangent.z /= length;
    }
    binormal.x = normal.y * tangent.z - normal.z * tangent.y;
    binormal.y = normal.z * tangent.x - normal.x * tangent.z;
    binormal.z = normal.x * tangent.y - normal.y * tangent.x;
    if (du1 * dv2 - dv1 * du2 < 0.0f) {
        tangent.x = -tangent.x;
        tangent.y = -tangent.y;
        tangent.z = -tangent.z;
    }

    output = data->normal + data->normalSize * 3;
    rpWriteNbtScalar(output, tangent.x, nbtNormalType, nbtNormalFraction);
    rpWriteNbtScalar(output + data->normalSize, tangent.y, nbtNormalType,
                nbtNormalFraction);
    rpWriteNbtScalar(output + data->normalSize * 2, tangent.z, nbtNormalType,
                nbtNormalFraction);
    output += data->normalSize * 3;
    rpWriteNbtScalar(output, binormal.x, nbtNormalType, nbtNormalFraction);
    rpWriteNbtScalar(output + data->normalSize, binormal.y, nbtNormalType,
                nbtNormalFraction);
    rpWriteNbtScalar(output + data->normalSize * 2, binormal.z, nbtNormalType,
                nbtNormalFraction);
}

static float rpReadNbtScalar(const unsigned char* address, unsigned char type,
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

static void rpWriteNbtScalar(unsigned char* address, float value, unsigned char type,
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
