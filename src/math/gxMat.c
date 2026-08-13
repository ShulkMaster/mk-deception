#include "math/gxMat.h"

typedef char Mat33SizeMustBe0x30[(sizeof(Mat33) == 0x30) ? 1 : -1];

/*
 * Non-const pointer params are load-bearing: MWCC 2.4.7 treats loads through
 * pointer-to-const as immune to stores (CSE + store sinking), which breaks the
 * retail per-row reload schedule. Mat33 params reached through union members
 * are conservative either way, so gxMat33x33/_Check keep const.
 *
 * Soft ceiling: gxMat33x33 ~97.43% -- FPR allocation plus load scheduling in
 *   the aliased (out == a || out == b) arm (same op set, partly reordered);
 *   direct arm and prologue/epilogue are exact. The hoisted-initializer order
 *   in that arm is retail-derived (a02 fifth); reordering it shifts the load
 *   schedule. Stop.
 */

void gxMat33Tx31(Vec* out, Vec* v, Mat33* m) {
    out->x = v->x * m->col0[0] + v->y * m->col1[0] + v->z * m->col2[0];
    out->y = v->x * m->col0[1] + v->y * m->col1[1] + v->z * m->col2[1];
    out->z = v->x * m->col0[2] + v->y * m->col1[2] + v->z * m->col2[2];
}

void gxMatScaledByV3(Mat33* out, Mat33* in, Vec* scale) {
    PSVECScale(&in->col0_vec, &out->col0_vec, scale->x);
    PSVECScale(&in->col1_vec, &out->col1_vec, scale->y);
    PSVECScale(&in->col2_vec, &out->col2_vec, scale->z);
    out->flags &= ~1;
}



void gxMat33x33_Check(Mat33* out, const Mat33* a, const Mat33* b) {
    gxMat33x33(out, a, b);
}

void gxMatV3MatAddV3_Check(Vec* out, Vec* v, Mat33* m, Vec* add) {
    out->x = add->x + (v->x * m->col0[0] + v->y * m->col1[0] + v->z * m->col2[0]);
    out->y = add->y + (v->x * m->col0[1] + v->y * m->col1[1] + v->z * m->col2[1]);
    out->z = add->z + (v->x * m->col0[2] + v->y * m->col1[2] + v->z * m->col2[2]);
}

void gxMatV3MatAddV3(Vec* out, Vec* v, Mat33* m, Vec* add) {
    out->x = add->x + (v->x * m->col0[0] + v->y * m->col1[0] + v->z * m->col2[0]);
    out->y = add->y + (v->x * m->col0[1] + v->y * m->col1[1] + v->z * m->col2[1]);
    out->z = add->z + (v->x * m->col0[2] + v->y * m->col1[2] + v->z * m->col2[2]);
}

void gxMat33x33(Mat33* out, const Mat33* a, const Mat33* b) {
    float c00, c01, c02;
    float c10, c11, c12;
    float c20, c21, c22;

    if (out == a || out == b) {
        float a01 = a->col0[1];
        float b10 = b->col1[0];
        float b11 = b->col1[1];
        float a00 = a->col0[0];
        float a02 = a->col0[2];
        float b00 = b->col0[0];
        float b12 = b->col1[2];
        float a11 = a->col1[1];
        float b01 = b->col0[1];
        float b02 = b->col0[2];
        float a21 = a->col2[1];
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
