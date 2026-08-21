#include "rw/rtquat.h"

static Quat* QuatFromPositiveDiagMatrix(Quat* qpQuat, const RwMatrix* mpMatrix, float T) {
    float S;

    S = ((float)1) + T;
    S = _rwSqrt(S);
    qpQuat->w = ((float)0.5) * S;
    S = ((float)0.5) / S;

    qpQuat->x = S * (mpMatrix->up.z - mpMatrix->at.y);
    qpQuat->y = S * (mpMatrix->at.x - mpMatrix->right.z);
    qpQuat->z = S * (mpMatrix->right.y - mpMatrix->up.x);

    return qpQuat;
}

static Quat* QuatFromXDiagDomMatrix(Quat* qpQuat, const RwMatrix* mpMatrix) {
    float S;

    S = ((float)1) + (mpMatrix->right.x - (mpMatrix->up.y + mpMatrix->at.z));
    S = _rwSqrt(S);
    qpQuat->x = ((float)0.5) * S;
    S = ((float)0.5) / S;

    qpQuat->w = S * (mpMatrix->up.z - mpMatrix->at.y);
    qpQuat->y = S * (mpMatrix->right.y + mpMatrix->up.x);
    qpQuat->z = S * (mpMatrix->right.z + mpMatrix->at.x);

    return qpQuat;
}

static Quat* QuatFromYDiagDomMatrix(Quat* qpQuat, const RwMatrix* mpMatrix) {
    float S;

    S = ((float)1) + (mpMatrix->up.y - (mpMatrix->at.z + mpMatrix->right.x));
    S = _rwSqrt(S);
    qpQuat->y = ((float)0.5) * S;
    S = ((float)0.5) / S;

    qpQuat->w = S * (mpMatrix->at.x - mpMatrix->right.z);
    qpQuat->z = S * (mpMatrix->up.z + mpMatrix->at.y);
    qpQuat->x = S * (mpMatrix->up.x + mpMatrix->right.y);

    return qpQuat;
}

static Quat* QuatFromZDiagDomMatrix(Quat* qpQuat, const RwMatrix* mpMatrix) {
    float S;

    S = ((float)1) + (mpMatrix->at.z - (mpMatrix->right.x + mpMatrix->up.y));
    S = _rwSqrt(S);
    qpQuat->z = ((float)0.5) * S;
    S = ((float)0.5) / S;

    qpQuat->w = S * (mpMatrix->right.y - mpMatrix->up.x);
    qpQuat->x = S * (mpMatrix->at.x + mpMatrix->right.z);
    qpQuat->y = S * (mpMatrix->at.y + mpMatrix->up.z);

    return qpQuat;
}

typedef Quat* (*QuatFromMatrixFn)(Quat* qpQuat, const RwMatrix* mpMatrix);

int RtQuatConvertFromMatrix(Quat* qpQuat, const RwMatrix* mpMatrix) {
    int valid;

    valid = qpQuat != 0 && mpMatrix != 0;

    if (valid) {
        QuatFromMatrixFn convert;
        float T;

        T = mpMatrix->at.z + (mpMatrix->right.x + mpMatrix->up.y);

        if (T > ((float)0)) {
            QuatFromPositiveDiagMatrix(qpQuat, mpMatrix, T);
        } else {
            if (mpMatrix->right.x > mpMatrix->up.y) {
                if (mpMatrix->right.x > mpMatrix->at.z) {
                    convert = QuatFromXDiagDomMatrix;
                } else {
                    convert = QuatFromZDiagDomMatrix;
                }
            } else {
                if (mpMatrix->up.y > mpMatrix->at.z) {
                    convert = QuatFromYDiagDomMatrix;
                } else {
                    convert = QuatFromZDiagDomMatrix;
                }
            }
            convert(qpQuat, mpMatrix);
        }
    }

    return valid;
}
