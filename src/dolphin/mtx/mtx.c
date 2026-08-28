#include "dolphin/mtx.h"
#include "math/gxQuat.h"

void PSMTXIdentity(Mtx matrix)
{
    matrix[0][0] = 1.0f;
    matrix[0][1] = 0.0f;
    matrix[0][2] = 0.0f;
    matrix[0][3] = 0.0f;
    matrix[1][0] = 0.0f;
    matrix[1][1] = 1.0f;
    matrix[1][2] = 0.0f;
    matrix[1][3] = 0.0f;
    matrix[2][0] = 0.0f;
    matrix[2][1] = 0.0f;
    matrix[2][2] = 1.0f;
    matrix[2][3] = 0.0f;
}

void PSMTXConcat(Mtx a, Mtx b, Mtx output)
{
    Mtx temporary;
    MtxPtr result = output;
    int row;

    if (output == a || output == b) {
        result = temporary;
    }
    for (row = 0; row < 3; ++row) {
        result[row][0] = a[row][0] * b[0][0] + a[row][1] * b[1][0] + a[row][2] * b[2][0];
        result[row][1] = a[row][0] * b[0][1] + a[row][1] * b[1][1] + a[row][2] * b[2][1];
        result[row][2] = a[row][0] * b[0][2] + a[row][1] * b[1][2] + a[row][2] * b[2][2];
        result[row][3] = a[row][0] * b[0][3] + a[row][1] * b[1][3] + a[row][2] * b[2][3] + a[row][3];
    }
    if (result == temporary) {
        for (row = 0; row < 3; ++row) {
            output[row][0] = temporary[row][0];
            output[row][1] = temporary[row][1];
            output[row][2] = temporary[row][2];
            output[row][3] = temporary[row][3];
        }
    }
}

void PSMTXTrans(Mtx matrix, float x, float y, float z)
{
    PSMTXIdentity(matrix);
    matrix[0][3] = x;
    matrix[1][3] = y;
    matrix[2][3] = z;
}

void PSMTXScale(Mtx matrix, float x, float y, float z)
{
    matrix[0][0] = x;
    matrix[0][1] = 0.0f;
    matrix[0][2] = 0.0f;
    matrix[0][3] = 0.0f;
    matrix[1][0] = 0.0f;
    matrix[1][1] = y;
    matrix[1][2] = 0.0f;
    matrix[1][3] = 0.0f;
    matrix[2][0] = 0.0f;
    matrix[2][1] = 0.0f;
    matrix[2][2] = z;
    matrix[2][3] = 0.0f;
}

void PSMTXQuat(Mtx matrix, const Quat* quaternion)
{
    float scale = 2.0f / (quaternion->x * quaternion->x +
                          quaternion->y * quaternion->y +
                          quaternion->z * quaternion->z +
                          quaternion->w * quaternion->w);
    float xs = quaternion->x * scale;
    float ys = quaternion->y * scale;
    float zs = quaternion->z * scale;
    float wx = quaternion->w * xs;
    float wy = quaternion->w * ys;
    float wz = quaternion->w * zs;
    float xx = quaternion->x * xs;
    float xy = quaternion->x * ys;
    float xz = quaternion->x * zs;
    float yy = quaternion->y * ys;
    float yz = quaternion->y * zs;
    float zz = quaternion->z * zs;

    matrix[0][0] = 1.0f - (yy + zz);
    matrix[0][1] = xy - wz;
    matrix[0][2] = xz + wy;
    matrix[0][3] = 0.0f;
    matrix[1][0] = xy + wz;
    matrix[1][1] = 1.0f - (xx + zz);
    matrix[1][2] = yz - wx;
    matrix[1][3] = 0.0f;
    matrix[2][0] = xz - wy;
    matrix[2][1] = yz + wx;
    matrix[2][2] = 1.0f - (xx + yy);
    matrix[2][3] = 0.0f;
}

void C_MTXLookAt(Mtx matrix, const Point3d* cameraPosition,
                 const Vec* cameraUp, const Point3d* target)
{
    Vec look;
    Vec right;
    Vec up;

    look.x = cameraPosition->x - target->x;
    look.y = cameraPosition->y - target->y;
    look.z = cameraPosition->z - target->z;
    PSVECNormalize(&look, &look);
    PSVECCrossProduct(cameraUp, &look, &right);
    PSVECNormalize(&right, &right);
    PSVECCrossProduct(&look, &right, &up);

    matrix[0][0] = right.x;
    matrix[0][1] = right.y;
    matrix[0][2] = right.z;
    matrix[0][3] = -(cameraPosition->z * right.z +
                     (cameraPosition->x * right.x + cameraPosition->y * right.y));
    matrix[1][0] = up.x;
    matrix[1][1] = up.y;
    matrix[1][2] = up.z;
    matrix[1][3] = -(cameraPosition->z * up.z +
                     (cameraPosition->x * up.x + cameraPosition->y * up.y));
    matrix[2][0] = look.x;
    matrix[2][1] = look.y;
    matrix[2][2] = look.z;
    matrix[2][3] = -(cameraPosition->z * look.z +
                     (cameraPosition->x * look.x + cameraPosition->y * look.y));
}
