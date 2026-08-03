#ifndef DOLPHIN_MTX_H
#define DOLPHIN_MTX_H

typedef float Mtx[3][4];
typedef float Mtx44[4][4];
typedef float (*MtxPtr)[4];

typedef struct Vec Vec;

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

#ifdef __cplusplus
}
#endif

#endif
