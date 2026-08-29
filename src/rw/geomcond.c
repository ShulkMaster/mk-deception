#include "rw/rwengine.h"
#include "runtime/cstring.h"
#include "rw/geomcond.h"

static unsigned int TypeGetSize(unsigned int type)
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

static int TypeCheckEqual(unsigned int type, const void* first,
                             const void* second)
{
    switch (type) {
    case 1:
        return *(const unsigned char*)first == *(const unsigned char*)second;
    case 2:
        return *(const unsigned short*)first == *(const unsigned short*)second;
    case 3:
        return ((const unsigned char*)first)[0] == ((const unsigned char*)second)[0] &&
               ((const unsigned char*)first)[1] == ((const unsigned char*)second)[1] &&
               ((const unsigned char*)first)[2] == ((const unsigned char*)second)[2];
    case 4:
        return *(const unsigned int*)first == *(const unsigned int*)second;
    case 6:
        return *(const unsigned int*)first == *(const unsigned int*)second;
    case 7:
        return ((const unsigned int*)first)[0] == ((const unsigned int*)second)[0] &&
               ((const unsigned int*)first)[1] == ((const unsigned int*)second)[1];
    case 8:
        return ((const unsigned int*)first)[0] == ((const unsigned int*)second)[0] &&
               ((const unsigned int*)first)[1] == ((const unsigned int*)second)[1] &&
               ((const unsigned int*)first)[2] == ((const unsigned int*)second)[2];
    case 9:
        return ((const unsigned int*)first)[0] == ((const unsigned int*)second)[0] &&
               ((const unsigned int*)first)[1] == ((const unsigned int*)second)[1] &&
               ((const unsigned int*)first)[2] == ((const unsigned int*)second)[2] &&
               ((const unsigned int*)first)[3] == ((const unsigned int*)second)[3];
    case 10:
        return *(const unsigned int*)first == *(const unsigned int*)second;
    default:
        return 0;
    }
}

static void IndicesRemap(unsigned short* output, const unsigned short* input,
                         const int* map, unsigned int numIndices)
{
    unsigned int index;

    for (index = 0; index < numIndices; index++) {
        int removed = 0;
        unsigned short representative;
        unsigned short vertex;

        if (map[input[index]] != -1)
            representative = (unsigned short)map[input[index]];
        else
            representative = input[index];

        for (vertex = 0; vertex < representative; vertex++) {
            if (map[vertex] != -1)
                removed++;
        }

        output[index] = representative - removed;
    }
}

unsigned short** IndexDataCreateRemapped(const GeomCondMap* maps,
                                   const unsigned short* const* input,
                                   unsigned int numArrays, unsigned int numIndices)
{
    unsigned short** output;
    unsigned int size = numArrays * sizeof(*output) +
                    numArrays * (numIndices * sizeof(**output));
    unsigned int offset;
    unsigned int index;

    output = RwEngineInstance->fpMalloc(size, 0x3050D);
    if (output == 0)
        return 0;

    memset(output, 0, numArrays * sizeof(*output));
    offset = numArrays * sizeof(*output);
    for (index = 0; index < numArrays; index++) {
        output[index] = (unsigned short*)((unsigned char*)output + offset);
        offset += numIndices * sizeof(**output);
    }

    for (index = 0; index < numArrays; index++)
        IndicesRemap(output[index], input[index], maps[index].data, numIndices);

    return output;
}

GeomCondMap* VertexDataCreateRemapped(const GeomCondMap* maps,
                                      const GeomCondVertexData* streams,
                                      unsigned int numStreams,
                                      unsigned int numVertices)
{
    GeomCondMap* output;
    unsigned int size = numStreams * sizeof(*output);
    unsigned int offset;
    unsigned int stream;

    for (stream = 0; stream < numStreams; stream++)
        size += maps[stream].count * TypeGetSize(streams[stream].type);

    output = RwEngineInstance->fpMalloc(size, 0x3050D);
    if (output == 0)
        return 0;

    memset(output, 0, numStreams * sizeof(*output));
    offset = numStreams * sizeof(*output);
    for (stream = 0; stream < numStreams; stream++) {
        output[stream].data = (unsigned char*)output + offset;
        offset += maps[stream].count * TypeGetSize(streams[stream].type);
    }

    for (stream = 0; stream < numStreams; stream++) {
        unsigned int elementSize = TypeGetSize(streams[stream].type);
        const unsigned char* source = streams[stream].data;
        unsigned char* destination = output[stream].data;
        unsigned int vertex;

        for (vertex = 0; vertex < numVertices; vertex++) {
            if (((const int*)maps[stream].data)[vertex] == -1) {
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
                                  unsigned int numStreams,
                                  unsigned int numVertices)
{
    unsigned int numMapEntries = numStreams * numVertices;
    GeomCondMap* maps = RwEngineInstance->fpMalloc(
        numStreams * sizeof(*maps) + numMapEntries * sizeof(int),
        0x3050D);
    unsigned int offset;
    unsigned int index;
    unsigned int stream;

    if (maps == 0)
        return 0;

    memset(maps, 0, numStreams * sizeof(*maps));
    for (index = 0; index < numMapEntries; index++)
        ((int*)((unsigned char*)maps + numStreams * sizeof(*maps)))[index] = -1;

    offset = numStreams * sizeof(*maps);
    for (stream = 0; stream < numStreams; stream++) {
        maps[stream].data = (unsigned char*)maps + offset;
        maps[stream].count = numVertices;
        offset += numVertices * sizeof(int);
    }

    for (stream = 0; stream < numStreams; stream++) {
        unsigned int elementSize = TypeGetSize(streams[stream].type);
        unsigned int sourceVertex;

        for (sourceVertex = 0; sourceVertex < numVertices; sourceVertex++) {
            unsigned int candidateVertex;

            if (((int*)maps[stream].data)[sourceVertex] != -1)
                continue;

            for (candidateVertex = sourceVertex; candidateVertex < numVertices;
                 candidateVertex++) {
                if (sourceVertex != candidateVertex &&
                    TypeCheckEqual(
                        streams[stream].type,
                        (const unsigned char*)streams[stream].data +
                            elementSize * sourceVertex,
                        (const unsigned char*)streams[stream].data +
                            elementSize * candidateVertex)) {
                    int equal = 1;
                    signed char dependencyIndex = 0;
                    signed char dependency = streams[stream].dependencies[0];

                    while (dependency >= 0) {
                        unsigned int dependencySize =
                            TypeGetSize(streams[dependency].type);

                        if (!TypeCheckEqual(
                                streams[dependency].type,
                                (const unsigned char*)streams[dependency].data +
                                    dependencySize * sourceVertex,
                                (const unsigned char*)streams[dependency].data +
                                    dependencySize * candidateVertex)) {
                            equal = 0;
                        } else {
                            dependencyIndex++;
                            dependency =
                                streams[stream].dependencies[dependencyIndex];
                            continue;
                        }
                        break;
                    }

                    if (equal) {
                        ((int*)maps[stream].data)[candidateVertex] =
                            sourceVertex;
                        maps[stream].count--;
                    }
                }
            }
        }
    }

    return maps;
}
