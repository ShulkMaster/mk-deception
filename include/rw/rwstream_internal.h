#ifndef RW_RWSTREAM_INTERNAL_H
#define RW_RWSTREAM_INTERNAL_H

#include "rw/rwcore_types.h"
#include "rw/rwplcore.h"

typedef struct RpGeometry RpGeometry;
typedef struct RpMaterialList RpMaterialList;

typedef struct RwFrameList {
    RwFrame** frames;
    RwInt32 numFrames;
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
RwInt32 _rwStringStreamGetSize(const RwChar* string);
const RwChar* _rwStringStreamWrite(const RwChar* string, RwStream* stream);
RwChar* _rwStringStreamFindAndRead(RwChar* string, RwStream* stream);
RwStream* _rwStreamWriteVersionedChunkHeader(
    RwStream* stream, RwInt32 type, RwInt32 size, RwUInt32 version,
    RwUInt32 buildNum);
RwBool _rwStreamReadChunkHeader(RwStream* stream, RwUInt32* type,
                                RwUInt32* length, RwUInt32* version,
                                RwUInt32* buildNum);
#endif
