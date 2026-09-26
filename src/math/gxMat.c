#include "math/gxMat.h"

typedef char Mat33SizeMustBe0x30[(sizeof(Mat33) == 0x30) ? 1 : -1];

/*
 * Non-const pointer params are load-bearing: MWCC 2.4.7 treats loads through
 * pointer-to-const as immune to stores (CSE + store sinking), which breaks the
 * retail per-row reload schedule. Mat33 params reached through union members
 * are conservative either way, so gxMat33x33/_Check keep const.
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
    if (out == a || out == b) {
        float c[3][3];

        c[0][0] = a->col0[0] * b->col0[0] + a->col0[1] * b->col1[0] +
                  a->col0[2] * b->col2[0];
        c[0][1] = a->col0[0] * b->col0[1] + a->col0[1] * b->col1[1] +
                  a->col0[2] * b->col2[1];
        c[0][2] = a->col0[0] * b->col0[2] + a->col0[1] * b->col1[2] +
                  a->col0[2] * b->col2[2];
        c[1][0] = a->col1[0] * b->col0[0] + a->col1[1] * b->col1[0] +
                  a->col1[2] * b->col2[0];
        c[1][1] = a->col1[0] * b->col0[1] + a->col1[1] * b->col1[1] +
                  a->col1[2] * b->col2[1];
        c[1][2] = a->col1[0] * b->col0[2] + a->col1[1] * b->col1[2] +
                  a->col1[2] * b->col2[2];
        c[2][0] = a->col2[0] * b->col0[0] + a->col2[1] * b->col1[0] +
                  a->col2[2] * b->col2[0];
        c[2][1] = a->col2[0] * b->col0[1] + a->col2[1] * b->col1[1] +
                  a->col2[2] * b->col2[1];
        c[2][2] = a->col2[0] * b->col0[2] + a->col2[1] * b->col1[2] +
                  a->col2[2] * b->col2[2];
        out->col0[0] = c[0][0];
        out->col0[1] = c[0][1];
        out->col0[2] = c[0][2];
        out->col1[0] = c[1][0];
        out->col1[1] = c[1][1];
        out->col1[2] = c[1][2];
        out->col2[0] = c[2][0];
        out->col2[1] = c[2][1];
        out->col2[2] = c[2][2];
    } else {
        out->col0[0] = a->col0[0] * b->col0[0] + a->col0[1] * b->col1[0] +
                       a->col0[2] * b->col2[0];
        out->col0[1] = a->col0[0] * b->col0[1] + a->col0[1] * b->col1[1] +
                       a->col0[2] * b->col2[1];
        out->col0[2] = a->col0[0] * b->col0[2] + a->col0[1] * b->col1[2] +
                       a->col0[2] * b->col2[2];
        out->col1[0] = a->col1[0] * b->col0[0] + a->col1[1] * b->col1[0] +
                       a->col1[2] * b->col2[0];
        out->col1[1] = a->col1[0] * b->col0[1] + a->col1[1] * b->col1[1] +
                       a->col1[2] * b->col2[1];
        out->col1[2] = a->col1[0] * b->col0[2] + a->col1[1] * b->col1[2] +
                       a->col1[2] * b->col2[2];
        out->col2[0] = a->col2[0] * b->col0[0] + a->col2[1] * b->col1[0] +
                       a->col2[2] * b->col2[0];
        out->col2[1] = a->col2[0] * b->col0[1] + a->col2[1] * b->col1[1] +
                       a->col2[2] * b->col2[1];
        out->col2[2] = a->col2[0] * b->col0[2] + a->col2[1] * b->col1[2] +
                       a->col2[2] * b->col2[2];
        out->flags = a->flags & b->flags;
    }
}
