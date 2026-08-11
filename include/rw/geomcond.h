#ifndef RW_GEOMCOND_H
#define RW_GEOMCOND_H

#include "rw/rwcore_types.h"

typedef struct GeomCondVertexData {
    void* data;
    RwUInt32 type;
    signed char dependencies[16];
} GeomCondVertexData;

typedef struct GeomCondMap {
    void* data;
    RwUInt32 count;
} GeomCondMap;

GeomCondMap* VertexDataCreateMaps(const GeomCondVertexData* streams,
                                  RwUInt32 numStreams,
                                  RwUInt32 numVertices);
GeomCondMap* VertexDataCreateRemapped(const GeomCondMap* maps,
                                      const GeomCondVertexData* streams,
                                      RwUInt32 numStreams,
                                      RwUInt32 numVertices);
RwUInt16** IndexDataCreateRemapped(const GeomCondMap* maps,
                                   const RwUInt16* const* input,
                                   RwUInt32 numArrays,
                                   RwUInt32 numIndices);

#endif
