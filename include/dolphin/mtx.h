#ifndef DOLPHIN_MTX_H
#define DOLPHIN_MTX_H

typedef float Mtx[3][4];
typedef float Mtx44[4][4];
typedef float (*MtxPtr)[4];

#include "math/gxVect.h"

typedef Vec Point3d;
typedef struct Quat Quat;

#ifdef __cplusplus
extern "C" {
#endif

void C_MTXFrustum(Mtx44 matrix, float top, float bottom, float left,
                  float right, float near_plane, float far_plane);
void C_MTXOrtho(Mtx44 matrix, float top, float bottom, float left,
                float right, float near_plane, float far_plane);
void C_MTXLookAt(Mtx matrix, Vec* camera_position, Vec* up, Vec* target);
void PSMTXIdentity(Mtx matrix);
void PSMTXTrans(Mtx matrix, float x, float y, float z);
void PSMTXScale(Mtx matrix, float x, float y, float z);
void PSMTXConcat(Mtx a, Mtx b, Mtx output);
void PSMTXQuat(Mtx matrix, const Quat* quaternion);
void PSMTXMultVec(const Mtx matrix, const Vec* source, Vec* destination);

#ifdef __cplusplus
}
#endif

#endif
