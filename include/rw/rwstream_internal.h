#ifndef RW_RWSTREAM_INTERNAL_H
#define RW_RWSTREAM_INTERNAL_H

#include "rw/rwcore_types.h"
#include "rw/rwplcore.h"

typedef struct RpGeometry RpGeometry;
typedef struct RpMaterialList RpMaterialList;

typedef struct RwFrameList {
    RwFrame** frames;
    int numFrames;
} RwFrameList;

typedef struct RpGeometryList {
    RpGeometry** geometries;
    int num_geometries;
} RpGeometryList;

RwFrameList* _rwFrameListStreamRead(RwStream* stream, RwFrameList* frameList);
RwFrameList* _rwFrameListDeinitialize(RwFrameList* frameList);
void GeometryListDeinitialize(RpGeometryList* geometry_list);
RpMaterialList* _rpMaterialListStreamRead(RwStream* stream,
                                          RpMaterialList* materialList);
RpMaterialList* _rpMaterialListInitialize(RpMaterialList* materialList);
int _rwStringStreamGetSize(const char* string);
const char* _rwStringStreamWrite(const char* string, RwStream* stream);
char* _rwStringStreamFindAndRead(char* string, RwStream* stream);
RwStream* _rwStreamWriteVersionedChunkHeader(
    RwStream* stream, int type, int size, unsigned int version,
    unsigned int buildNum);
int _rwStreamReadChunkHeader(RwStream* stream, unsigned int* type,
                                unsigned int* length, unsigned int* version,
                                unsigned int* buildNum);
#endif
