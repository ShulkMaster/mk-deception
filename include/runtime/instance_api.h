#ifndef RUNTIME_INSTANCE_API_H
#define RUNTIME_INSTANCE_API_H

typedef struct RpClump RpClump;
typedef struct RpGeometry RpGeometry;
typedef struct RwStream RwStream;
typedef struct RwTexture RwTexture;

RpClump* inplaceClumpStreamRead(RwStream* stream);
RpGeometry* inplaceGeometryCreate_80056E98(int num_vertices, int num_triangles,
                                           unsigned int format);
unsigned int PadSize32(unsigned int value);
int inplaceNativeTextureRead(RwStream* stream, RwTexture** texture);
void render_collision_regions(void);

#endif
