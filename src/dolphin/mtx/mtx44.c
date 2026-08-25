#include "dolphin/mtx.h"

void C_MTXFrustum(Mtx44 matrix, float top, float bottom, float left,
                  float right, float near_plane, float far_plane)
{
    float reciprocal;

    reciprocal = 1.0f / (right - left);
    matrix[0][0] = 2.0f * near_plane * reciprocal;
    matrix[0][1] = 0.0f;
    matrix[0][2] = reciprocal * (right + left);
    matrix[0][3] = 0.0f;

    reciprocal = 1.0f / (top - bottom);
    matrix[1][0] = 0.0f;
    matrix[1][1] = 2.0f * near_plane * reciprocal;
    matrix[1][2] = reciprocal * (top + bottom);
    matrix[1][3] = 0.0f;

    matrix[2][0] = 0.0f;
    matrix[2][1] = 0.0f;
    reciprocal = 1.0f / (far_plane - near_plane);
    matrix[2][2] = -near_plane * reciprocal;
    matrix[2][3] = reciprocal * -(far_plane * near_plane);

    matrix[3][0] = 0.0f;
    matrix[3][1] = 0.0f;
    matrix[3][2] = -1.0f;
    matrix[3][3] = 0.0f;
}

void C_MTXOrtho(Mtx44 matrix, float top, float bottom, float left,
                float right, float near_plane, float far_plane)
{
    float reciprocal;

    reciprocal = 1.0f / (right - left);
    matrix[0][0] = 2.0f * reciprocal;
    matrix[0][1] = 0.0f;
    matrix[0][2] = 0.0f;
    matrix[0][3] = reciprocal * -(right + left);

    reciprocal = 1.0f / (top - bottom);
    matrix[1][0] = 0.0f;
    matrix[1][1] = 2.0f * reciprocal;
    matrix[1][2] = 0.0f;
    matrix[1][3] = reciprocal * -(top + bottom);

    matrix[2][0] = 0.0f;
    matrix[2][1] = 0.0f;
    reciprocal = 1.0f / (far_plane - near_plane);
    matrix[2][2] = -1.0f * reciprocal;
    matrix[2][3] = -far_plane * reciprocal;

    matrix[3][0] = 0.0f;
    matrix[3][1] = 0.0f;
    matrix[3][2] = 0.0f;
    matrix[3][3] = 1.0f;
}
