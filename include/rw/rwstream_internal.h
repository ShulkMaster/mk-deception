#ifndef RW_RWSTREAM_INTERNAL_H
#define RW_RWSTREAM_INTERNAL_H

typedef struct RwFrameList {
    RwFrame** frames;
    int num_frames;
} RwFrameList;

typedef struct RpGeometryList {
    RpGeometry** geometries;
    int num_geometries;
} RpGeometryList;

int _rwFrameListStreamRead(RwStream* stream, RwFrameList* frame_list);
void _rwFrameListDeinitialize(RwFrameList* frame_list);
void GeometryListDeinitialize(RpGeometryList* geometry_list);
RwStream* _rpMaterialListStreamRead(RwStream* stream,
                                    RpMaterialList* material_list);
unsigned int _rpMaterialListInitialize(RpMaterialList* material_list);
RwStream* _rwPluginRegistryReadDataChunks(RwPluginRegistry* registry,
                                          RwStream* stream, void* object);
void* _rwPluginRegistryInvokeRights(RwPluginRegistry* registry,
                                    unsigned int plugin_id, void* object,
                                    int extra_data);

#endif
