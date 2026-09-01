#include "dolphin/mtx.h"
#include "math/gxQuat.h"
#include "runtime/asm_sequences.inc"

float Unit01[2] = { 0.0f, 1.0f };
const float PSMTXOne = 1.0f;
const float PSMTXZero = 0.0f;

asm void PSMTXIdentity(Mtx matrix)
{
    SEQ_PSMTXIdentity();
}

asm void PSMTXConcat(const Mtx a, const Mtx b, Mtx output)
{
    SEQ_PSMTXConcat();
}

asm void PSMTXTrans(Mtx matrix, float x, float y, float z)
{
    SEQ_PSMTXTrans();
}

asm void PSMTXScale(Mtx matrix, float x, float y, float z)
{
    SEQ_PSMTXScale();
}

asm void PSMTXQuat(Mtx matrix, const Quat* quaternion)
{
    SEQ_PSMTXQuat();
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
