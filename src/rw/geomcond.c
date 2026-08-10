#include "libmkparticle/rw_engine.h"
#include "runtime/cstring.h"
#include "rw/geomcond.h"

static RwUInt32 TypeGetSize(RwUInt32 type)
{
    switch (type) {
    case 1:
        return 1;
    case 2:
        return 2;
    case 3:
        return 3;
    case 4:
        return 4;
    case 6:
        return 4;
    case 7:
        return 8;
    case 8:
        return 12;
    case 9:
        return 16;
    case 10:
        return 4;
    default:
        return 0;
    }
}

static RwBool TypeCheckEqual(RwUInt32 type, const void* first,
                             const void* second)
{
    switch (type) {
    case 1:
        return *(const RwUInt8*)first == *(const RwUInt8*)second;
    case 2:
        return *(const RwUInt16*)first == *(const RwUInt16*)second;
    case 3:
        return ((const RwUInt8*)first)[0] == ((const RwUInt8*)second)[0] &&
               ((const RwUInt8*)first)[1] == ((const RwUInt8*)second)[1] &&
               ((const RwUInt8*)first)[2] == ((const RwUInt8*)second)[2];
    case 4:
        return *(const RwUInt32*)first == *(const RwUInt32*)second;
    case 6:
        return *(const RwUInt32*)first == *(const RwUInt32*)second;
    case 7:
        return ((const RwUInt32*)first)[0] == ((const RwUInt32*)second)[0] &&
               ((const RwUInt32*)first)[1] == ((const RwUInt32*)second)[1];
    case 8:
        return ((const RwUInt32*)first)[0] == ((const RwUInt32*)second)[0] &&
               ((const RwUInt32*)first)[1] == ((const RwUInt32*)second)[1] &&
               ((const RwUInt32*)first)[2] == ((const RwUInt32*)second)[2];
    case 9:
        return ((const RwUInt32*)first)[0] == ((const RwUInt32*)second)[0] &&
               ((const RwUInt32*)first)[1] == ((const RwUInt32*)second)[1] &&
               ((const RwUInt32*)first)[2] == ((const RwUInt32*)second)[2] &&
               ((const RwUInt32*)first)[3] == ((const RwUInt32*)second)[3];
    case 10:
        return *(const RwUInt32*)first == *(const RwUInt32*)second;
    default:
        return FALSE;
    }
}

static void IndicesRemap(RwUInt16* output, const RwUInt16* input,
                         const RwInt32* map, RwUInt32 numIndices)
{
    RwUInt32 index;

    for (index = 0; index < numIndices; index++) {
        RwInt32 removed = 0;
        RwUInt16 representative;
        RwUInt16 vertex;

        if (map[input[index]] != -1)
            representative = (RwUInt16)map[input[index]];
        else
            representative = input[index];

        for (vertex = 0; vertex < representative; vertex++) {
            if (map[vertex] != -1)
                removed++;
        }

        output[index] = representative - removed;
    }
}

RwUInt16** IndexDataCreateRemapped(const GeomCondMap* maps,
                                   const RwUInt16* const* input,
                                   RwUInt32 numArrays, RwUInt32 numIndices)
{
    RwUInt16** output;
    RwUInt32 size = numArrays * sizeof(*output) +
                    numArrays * (numIndices * sizeof(**output));
    RwUInt32 offset;
    RwUInt32 index;

    output = RwEngineInstance->fpMalloc(size, 0x3050D);
    if (output == NULL)
        return NULL;

    memset(output, 0, numArrays * sizeof(*output));
    offset = numArrays * sizeof(*output);
    for (index = 0; index < numArrays; index++) {
        output[index] = (RwUInt16*)((RwUInt8*)output + offset);
        offset += numIndices * sizeof(**output);
    }

    for (index = 0; index < numArrays; index++)
        IndicesRemap(output[index], input[index], maps[index].data, numIndices);

    return output;
}

GeomCondMap* VertexDataCreateRemapped(const GeomCondMap* maps,
                                      const GeomCondVertexData* streams,
                                      RwUInt32 numStreams,
                                      RwUInt32 numVertices)
{
    GeomCondMap* output;
    RwUInt32 size = numStreams * sizeof(*output);
    RwUInt32 offset;
    RwUInt32 stream;

    for (stream = 0; stream < numStreams; stream++)
        size += maps[stream].count * TypeGetSize(streams[stream].type);

    output = RwEngineInstance->fpMalloc(size, 0x3050D);
    if (output == NULL)
        return NULL;

    memset(output, 0, numStreams * sizeof(*output));
    offset = numStreams * sizeof(*output);
    for (stream = 0; stream < numStreams; stream++) {
        output[stream].data = (RwUInt8*)output + offset;
        offset += maps[stream].count * TypeGetSize(streams[stream].type);
    }

    for (stream = 0; stream < numStreams; stream++) {
        RwUInt32 elementSize = TypeGetSize(streams[stream].type);
        const RwUInt8* source = streams[stream].data;
        RwUInt8* destination = output[stream].data;
        RwUInt32 vertex;

        for (vertex = 0; vertex < numVertices; vertex++) {
            if (((const RwInt32*)maps[stream].data)[vertex] == -1) {
                memcpy(destination, source, elementSize);
                destination += elementSize;
            }
            source += elementSize;
        }
        output[stream].count = maps[stream].count;
    }

    return output;
}

GeomCondMap* VertexDataCreateMaps(const GeomCondVertexData* streams,
                                  RwUInt32 numStreams,
                                  RwUInt32 numVertices)
{
    RwUInt32 numMapEntries = numStreams * numVertices;
    GeomCondMap* maps = RwEngineInstance->fpMalloc(
        numStreams * sizeof(*maps) + numMapEntries * sizeof(RwInt32),
        0x3050D);
    RwUInt32 offset;
    RwUInt32 index;
    RwUInt32 stream;

    if (maps == NULL)
        return NULL;

    memset(maps, 0, numStreams * sizeof(*maps));
    for (index = 0; index < numMapEntries; index++)
        ((RwInt32*)((RwUInt8*)maps + numStreams * sizeof(*maps)))[index] = -1;

    offset = numStreams * sizeof(*maps);
    for (stream = 0; stream < numStreams; stream++) {
        maps[stream].data = (RwUInt8*)maps + offset;
        maps[stream].count = numVertices;
        offset += numVertices * sizeof(RwInt32);
    }

    for (stream = 0; stream < numStreams; stream++) {
        RwUInt32 elementSize = TypeGetSize(streams[stream].type);
        RwUInt32 sourceVertex;

        for (sourceVertex = 0; sourceVertex < numVertices; sourceVertex++) {
            RwUInt32 candidateVertex;

            if (((RwInt32*)maps[stream].data)[sourceVertex] != -1)
                continue;

            for (candidateVertex = sourceVertex; candidateVertex < numVertices;
                 candidateVertex++) {
                if (sourceVertex != candidateVertex &&
                    TypeCheckEqual(
                        streams[stream].type,
                        (const RwUInt8*)streams[stream].data +
                            elementSize * sourceVertex,
                        (const RwUInt8*)streams[stream].data +
                            elementSize * candidateVertex)) {
                    RwBool equal = TRUE;
                    signed char dependencyIndex = 0;
                    signed char dependency = streams[stream].dependencies[0];
                    RwUInt32 dependencySize =
                        TypeGetSize(streams[dependency].type);

                    while (dependency >= 0) {
                        if (!TypeCheckEqual(
                                streams[dependency].type,
                                (const RwUInt8*)streams[dependency].data +
                                    dependencySize * sourceVertex,
                                (const RwUInt8*)streams[dependency].data +
                                    dependencySize * candidateVertex)) {
                            equal = FALSE;
                        } else {
                            dependencyIndex++;
                            dependency =
                                streams[stream].dependencies[dependencyIndex];
                            dependencySize = TypeGetSize(streams[dependency].type);
                            continue;
                        }
                        break;
                    }

                    if (equal) {
                        ((RwInt32*)maps[stream].data)[candidateVertex] =
                            sourceVertex;
                        maps[stream].count--;
                    }
                }
            }
        }
    }

    return maps;
}
