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
RwStream* _rpMaterialListStreamRead(RwStream* stream,
                                    RpMaterialList* material_list);
unsigned int _rpMaterialListInitialize(RpMaterialList* material_list);
#endif
