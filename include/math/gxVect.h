#ifndef GX_VECT_H
#define GX_VECT_H

typedef struct Vec {
    float x;
    float y;
    float z;
} Vec;
typedef char VecSizeCheck[sizeof(Vec) == 0x0C ? 1 : -1];

static inline void gxVectCopy(Vec* destination, const Vec* source) {
    destination->x = source->x;
    destination->y = source->y;
    destination->z = source->z;
}

static inline void gxVectScale(Vec* destination, const Vec* source, float scale) {
    destination->x = source->x * scale;
    destination->y = source->y * scale;
    destination->z = source->z * scale;
}

void PSVECAdd(const Vec* a, const Vec* b, Vec* dst);
void PSVECSubtract(const Vec* a, const Vec* b, Vec* dst);
void PSVECNormalize(const Vec* src, Vec* dst);
void PSVECScale(const Vec* src, Vec* dst, float scale);
float PSVECMag(const Vec* v);
float PSVECDotProduct(const Vec* a, const Vec* b);
void PSVECCrossProduct(const Vec* a, const Vec* b, Vec* dst);

float gxVectAngleZX(const Vec* v);
void gxVectUVV3ToV3(Vec* v, const Vec* u, const Vec* w);

#endif
