#ifndef GX_VECT_H
#define GX_VECT_H

typedef struct Vec {
    float x;
    float y;
    float z;
} Vec;

void PSVECSubtract(const Vec* a, const Vec* b, Vec* dst);
void PSVECNormalize(const Vec* src, Vec* dst);

float gxVectAngleZX(const Vec* v);
void gxVectUVV3ToV3(Vec* v, const Vec* u, const Vec* w);

#endif
