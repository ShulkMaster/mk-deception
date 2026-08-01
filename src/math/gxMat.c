#include "math/gxMat.h"
#include "math/gxQuat.h"

/*
 * Soft ceilings (NonMatching -- clean math preferred over FP schedule):
 *   gxMat33Tx31 / gxMatV3MatAddV3* ~77%/79% -- FP coloring + axis reload CSE; stop.
 *   gxMatScaledByV3 ~92.6% -- first lfs scale->x before stmw (sched off fixes
 *     prologue but scrambles later call setup); stop after budget.
 *   gxMat33x33 ~97% -- retail dual path recovered; remaining mismatch is FPR
 *     allocation/scheduling in the alias-safe path.
 */

void gxMat33Tx31(Vec* out, Vec* v, Mat33* m) {
    float x = v->x;
    float y = v->y;
    float z = v->z;

    out->x = z * m->col2[0] + x * m->col0[0] + y * m->col1[0];
    out->y = z * m->col2[1] + x * m->col0[1] + y * m->col1[1];
    out->z = z * m->col2[2] + x * m->col0[2] + y * m->col1[2];
}

void gxMatScaledByV3(Mat33* out, const Mat33* in, const Vec* scale) {
    /* Soft ceiling ~92.6%: retail lfs scale->x after mrs; MWCC before stmw. */
    PSVECScale((const Vec*)in, scale->x, (Vec*)out);
    PSVECScale((const Vec*)&in->col1[0], scale->y, (Vec*)&out->col1[0]);
    PSVECScale((const Vec*)&in->col2[0], scale->z, (Vec*)&out->col2[0]);
    out->flags &= ~1;
}



void gxMat33x33_Check(Mat33* out, Mat33* a, Mat33* b) {
    gxMat33x33(out, a, b);
}

void gxMatV3MatAddV3_Check(Vec* out, Vec* v, Mat33* m, Vec* add) {
    float x = v->x;
    float y = v->y;
    float z = v->z;

    out->x = add->x + (y * m->col1[0] + x * m->col0[0] + z * m->col2[0]);
    out->y = add->y + (y * m->col1[1] + x * m->col0[1] + z * m->col2[1]);
    out->z = add->z + (y * m->col1[2] + x * m->col0[2] + z * m->col2[2]);
}

void gxMatV3MatAddV3(Vec* out, Vec* v, Mat33* m, Vec* add) {
    float x = v->x;
    float y = v->y;
    float z = v->z;

    out->x = add->x + (y * m->col1[0] + x * m->col0[0] + z * m->col2[0]);
    out->y = add->y + (y * m->col1[1] + x * m->col0[1] + z * m->col2[1]);
    out->z = add->z + (y * m->col1[2] + x * m->col0[2] + z * m->col2[2]);
}

void gxMat33x33(Mat33* out, Mat33* a, Mat33* b) {
    float c00, c01, c02;
    float c10, c11, c12;
    float c20, c21, c22;

    if (out == a || out == b) {
        float a01 = a->col0[1];
        float b10 = b->col1[0];
        float b11 = b->col1[1];
        float a00 = a->col0[0];
        float b00 = b->col0[0];
        float b12 = b->col1[2];
        float a11 = a->col1[1];
        float b01 = b->col0[1];
        float b02 = b->col0[2];
        float a21 = a->col2[1];
        float a02 = a->col0[2];
        float b20 = b->col2[0];
        float b21 = b->col2[1];
        float a10 = a->col1[0];
        float b22 = b->col2[2];
        float a12 = a->col1[2];
        float a22 = a->col2[2];
        float a20 = a->col2[0];

        c00 = a02 * b20 + a00 * b00 + a01 * b10;
        c01 = a02 * b21 + a00 * b01 + a01 * b11;
        c02 = a02 * b22 + a00 * b02 + a01 * b12;
        c10 = a12 * b20 + a10 * b00 + a11 * b10;
        c11 = a12 * b21 + a10 * b01 + a11 * b11;
        c12 = a12 * b22 + a10 * b02 + a11 * b12;
        c20 = a22 * b20 + a20 * b00 + a21 * b10;
        c21 = a22 * b21 + a20 * b01 + a21 * b11;
        c22 = a22 * b22 + a20 * b02 + a21 * b12;
        out->col0[0] = c00;
        out->col0[1] = c01;
        out->col0[2] = c02;
        out->col1[0] = c10;
        out->col1[1] = c11;
        out->col1[2] = c12;
        out->col2[0] = c20;
        out->col2[1] = c21;
        out->col2[2] = c22;
    } else {
        out->col0[0] = a->col0[1] * b->col1[0] + a->col0[0] * b->col0[0] +
                       a->col0[2] * b->col2[0];
        out->col0[1] = a->col0[1] * b->col1[1] + a->col0[0] * b->col0[1] +
                       a->col0[2] * b->col2[1];
        out->col0[2] = a->col0[1] * b->col1[2] + a->col0[0] * b->col0[2] +
                       a->col0[2] * b->col2[2];
        out->col1[0] = a->col1[1] * b->col1[0] + a->col1[0] * b->col0[0] +
                       a->col1[2] * b->col2[0];
        out->col1[1] = a->col1[1] * b->col1[1] + a->col1[0] * b->col0[1] +
                       a->col1[2] * b->col2[1];
        out->col1[2] = a->col1[1] * b->col1[2] + a->col1[0] * b->col0[2] +
                       a->col1[2] * b->col2[2];
        out->col2[0] = a->col2[1] * b->col1[0] + a->col2[0] * b->col0[0] +
                       a->col2[2] * b->col2[0];
        out->col2[1] = a->col2[1] * b->col1[1] + a->col2[0] * b->col0[1] +
                       a->col2[2] * b->col2[1];
        out->col2[2] = a->col2[1] * b->col1[2] + a->col2[0] * b->col0[2] +
                       a->col2[2] * b->col2[2];
        out->flags = a->flags & b->flags;
    }
}
