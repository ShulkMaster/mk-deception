#ifndef GAME_BLOOD_ASSET_H
#define GAME_BLOOD_ASSET_H
#include "math/gxVect.h"
typedef struct BloodSurface BloodSurface;

struct BloodSurfaceEdge {
    Vec normal;
    float plane_distance;
};

typedef struct BloodSurfaceRecord {
    Vec normal;
    float plane_distance;
    int bone;
    int vertex_indices[3];
    Vec points[3];
    int neighbors[3];
    struct BloodSurfaceEdge edges[3];
} BloodSurfaceRecord;

typedef char check_BloodSurfaceRecord_size[(sizeof(BloodSurfaceRecord) == 0x80) ? 1 : -1];

typedef struct BloodSurfaceVertex {
    int tag;
    Vec position;
} BloodSurfaceVertex;

struct BloodSurface {
    int vertex_count;
    BloodSurfaceVertex* vertices;
    int record_count;
    int (*triangles)[3];
    BloodSurfaceRecord* records;
};

typedef struct BloodPath {
    BloodSurface* surface;
    int point_count;
    const int* record_indices;
    int* corner_indices;
    float interpolation_bias;
    float speed_base;
    float speed_scale;
} BloodPath;

typedef struct BloodModelData {
    BloodSurface surface;
    BloodPath paths[10];
} BloodModelData;

typedef struct BloodPathFile {
    BloodSurface surface;
    int relocation_marker;
    BloodPath* paths[10];
} BloodPathFile;

#endif
