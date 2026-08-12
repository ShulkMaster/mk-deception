#include "rw/rwplcore.h"

RwBBox* RwBBoxCalculate(RwBBox* boundBox, const RwV3d* verts, int numVerts) {
    boundBox->inf = *verts;
    boundBox->sup = *verts;
    verts++;
    numVerts--;

    while (numVerts--) {
        if (boundBox->inf.x > verts->x) boundBox->inf.x = verts->x;
        if (boundBox->inf.y > verts->y) boundBox->inf.y = verts->y;
        if (boundBox->inf.z > verts->z) boundBox->inf.z = verts->z;
        if (boundBox->sup.x < verts->x) boundBox->sup.x = verts->x;
        if (boundBox->sup.y < verts->y) boundBox->sup.y = verts->y;
        if (boundBox->sup.z < verts->z) boundBox->sup.z = verts->z;
        verts++;
    }

    return boundBox;
}
