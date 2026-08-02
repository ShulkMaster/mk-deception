/*
 * Port readiness:
 *   Structs: CLEAN
 *   Matching: 100.00% (.text)
 *   Linked: YES
 *   Status: READY
 *   Gaps: none
 */
#include "rw/rtquat.h"

/* rtquat.a/rtquat.obj -- RtQuatConvertFromMatrix + diag helpers.
 * Algorithm from GQNE5D ASM; types from BFBB rtquat/rwplcore.
 * PS2 MKD / MKDHook = API only, never EE ASM.
 * Build: -opt off -O0 -inline off (see configure.py). */

static RtQuat* QuatFromPositiveDiagMatrix(RtQuat* qpQuat, const RwMatrix* mpMatrix, RwReal T) {
    RwReal S;

    S = ((RwReal)1) + T;
    S = _rwSqrt(S);
    qpQuat->real = ((RwReal)0.5) * S;
    S = ((RwReal)0.5) / S;

    qpQuat->imag.x = S * (mpMatrix->up.z - mpMatrix->at.y);
    qpQuat->imag.y = S * (mpMatrix->at.x - mpMatrix->right.z);
    qpQuat->imag.z = S * (mpMatrix->right.y - mpMatrix->up.x);

    return qpQuat;
}

static RtQuat* QuatFromXDiagDomMatrix(RtQuat* qpQuat, const RwMatrix* mpMatrix) {
    RwReal S;

    S = ((RwReal)1) + (mpMatrix->right.x - (mpMatrix->up.y + mpMatrix->at.z));
    S = _rwSqrt(S);
    qpQuat->imag.x = ((RwReal)0.5) * S;
    S = ((RwReal)0.5) / S;

    qpQuat->real = S * (mpMatrix->up.z - mpMatrix->at.y);
    qpQuat->imag.y = S * (mpMatrix->right.y + mpMatrix->up.x);
    qpQuat->imag.z = S * (mpMatrix->right.z + mpMatrix->at.x);

    return qpQuat;
}

static RtQuat* QuatFromYDiagDomMatrix(RtQuat* qpQuat, const RwMatrix* mpMatrix) {
    RwReal S;

    S = ((RwReal)1) + (mpMatrix->up.y - (mpMatrix->at.z + mpMatrix->right.x));
    S = _rwSqrt(S);
    qpQuat->imag.y = ((RwReal)0.5) * S;
    S = ((RwReal)0.5) / S;

    qpQuat->real = S * (mpMatrix->at.x - mpMatrix->right.z);
    qpQuat->imag.z = S * (mpMatrix->up.z + mpMatrix->at.y);
    qpQuat->imag.x = S * (mpMatrix->up.x + mpMatrix->right.y);

    return qpQuat;
}

static RtQuat* QuatFromZDiagDomMatrix(RtQuat* qpQuat, const RwMatrix* mpMatrix) {
    RwReal S;

    S = ((RwReal)1) + (mpMatrix->at.z - (mpMatrix->right.x + mpMatrix->up.y));
    S = _rwSqrt(S);
    qpQuat->imag.z = ((RwReal)0.5) * S;
    S = ((RwReal)0.5) / S;

    qpQuat->real = S * (mpMatrix->right.y - mpMatrix->up.x);
    qpQuat->imag.x = S * (mpMatrix->at.x + mpMatrix->right.z);
    qpQuat->imag.y = S * (mpMatrix->at.y + mpMatrix->up.z);

    return qpQuat;
}

typedef RtQuat* (*QuatFromMatrixFn)(RtQuat* qpQuat, const RwMatrix* mpMatrix);

RwBool RtQuatConvertFromMatrix(RtQuat* qpQuat, const RwMatrix* mpMatrix) {
    /* Locals ordered for MWCC -O0 (matches retail stack/register layout). */
    RwBool ok;
    RwBool result;

    result = FALSE;
    if (qpQuat != NULL) {
        if (mpMatrix != NULL) {
            result = TRUE;
        }
    }

    ok = result;
    if (ok) {
        QuatFromMatrixFn tmpX;
        QuatFromMatrixFn func;
        QuatFromMatrixFn tmpY;
        QuatFromMatrixFn call;
        RwReal T;

        T = mpMatrix->at.z + (mpMatrix->right.x + mpMatrix->up.y);

        if (T > ((RwReal)0)) {
            QuatFromPositiveDiagMatrix(qpQuat, mpMatrix, T);
        } else {
            if (mpMatrix->right.x > mpMatrix->up.y) {
                if (mpMatrix->right.x > mpMatrix->at.z) {
                    tmpX = QuatFromXDiagDomMatrix;
                } else {
                    tmpX = QuatFromZDiagDomMatrix;
                }
                func = tmpX;
            } else {
                if (mpMatrix->up.y > mpMatrix->at.z) {
                    tmpY = QuatFromYDiagDomMatrix;
                } else {
                    tmpY = QuatFromZDiagDomMatrix;
                }
                func = tmpY;
            }

            call = func;
            (call)(qpQuat, mpMatrix);
        }
    }

    return ok;
}
