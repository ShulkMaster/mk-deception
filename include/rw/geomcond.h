#ifndef RW_GEOMCOND_H
#define RW_GEOMCOND_H

#include "rw/rwcore_types.h"

typedef struct GeomCondVertexData {
    void* data;
    unsigned int type;
    signed char dependencies[16];
} GeomCondVertexData;

typedef struct GeomCondMap {
    void* data;
    unsigned int count;
} GeomCondMap;

GeomCondMap* VertexDataCreateMaps(const GeomCondVertexData* streams,
                                  unsigned int numStreams,
                                  unsigned int numVertices);
GeomCondMap* VertexDataCreateRemapped(const GeomCondMap* maps,
                                      const GeomCondVertexData* streams,
                                      unsigned int numStreams,
                                      unsigned int numVertices);
unsigned short** IndexDataCreateRemapped(const GeomCondMap* maps,
                                   const unsigned short* const* input,
                                   unsigned int numArrays,
                                   unsigned int numIndices);

#endif
