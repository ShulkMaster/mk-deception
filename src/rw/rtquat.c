#include "rw/rtquat.h"

static RtQuat* QuatFromPositiveDiagMatrix(RtQuat* qpQuat, const RwMatrix* mpMatrix, float T) {
    float S;

    S = ((float)1) + T;
    S = _rwSqrt(S);
    qpQuat->real = ((float)0.5) * S;
    S = ((float)0.5) / S;

    qpQuat->imag.x = S * (mpMatrix->up.z - mpMatrix->at.y);
    qpQuat->imag.y = S * (mpMatrix->at.x - mpMatrix->right.z);
    qpQuat->imag.z = S * (mpMatrix->right.y - mpMatrix->up.x);

    return qpQuat;
}

static RtQuat* QuatFromXDiagDomMatrix(RtQuat* qpQuat, const RwMatrix* mpMatrix) {
    float S;

    S = ((float)1) + (mpMatrix->right.x - (mpMatrix->up.y + mpMatrix->at.z));
    S = _rwSqrt(S);
    qpQuat->imag.x = ((float)0.5) * S;
    S = ((float)0.5) / S;

    qpQuat->real = S * (mpMatrix->up.z - mpMatrix->at.y);
    qpQuat->imag.y = S * (mpMatrix->right.y + mpMatrix->up.x);
    qpQuat->imag.z = S * (mpMatrix->right.z + mpMatrix->at.x);

    return qpQuat;
}

static RtQuat* QuatFromYDiagDomMatrix(RtQuat* qpQuat, const RwMatrix* mpMatrix) {
    float S;

    S = ((float)1) + (mpMatrix->up.y - (mpMatrix->at.z + mpMatrix->right.x));
    S = _rwSqrt(S);
    qpQuat->imag.y = ((float)0.5) * S;
    S = ((float)0.5) / S;

    qpQuat->real = S * (mpMatrix->at.x - mpMatrix->right.z);
    qpQuat->imag.z = S * (mpMatrix->up.z + mpMatrix->at.y);
    qpQuat->imag.x = S * (mpMatrix->up.x + mpMatrix->right.y);

    return qpQuat;
}

static RtQuat* QuatFromZDiagDomMatrix(RtQuat* qpQuat, const RwMatrix* mpMatrix) {
    float S;

    S = ((float)1) + (mpMatrix->at.z - (mpMatrix->right.x + mpMatrix->up.y));
    S = _rwSqrt(S);
    qpQuat->imag.z = ((float)0.5) * S;
    S = ((float)0.5) / S;

    qpQuat->real = S * (mpMatrix->right.y - mpMatrix->up.x);
    qpQuat->imag.x = S * (mpMatrix->at.x + mpMatrix->right.z);
    qpQuat->imag.y = S * (mpMatrix->at.y + mpMatrix->up.z);

    return qpQuat;
}

typedef RtQuat* (*QuatFromMatrixFn)(RtQuat* qpQuat, const RwMatrix* mpMatrix);

int RtQuatConvertFromMatrix(RtQuat* qpQuat, const RwMatrix* mpMatrix) {
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
