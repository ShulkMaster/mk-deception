#include "math/mk_math.h"
#include "math/gxMath.h"
#include "runtime/cmath.h"

/*
 * Soft ceilings (Wave D polish -- NonMatching; ASM still linked):
 *   ang_sub_ang ~98.5% -- ops match; sdata2 reloc/pool leftover
 *   normalize_xz / normalize_v3 ~93-94% -- invsqrt f6/cror schedule
 *   normalize_v3_length ~83% -- sqrt-table + 1/len schedule
 *   leaf schedule: parametric_ray / scale_xz / v3_sub/add* (~40-50%)
 *   xz_to_y_ang ~79% -- mtlr spill schedule
 *   ray_cyl / interp_quat / YXZ_angles_to_quat / mat_x_mat -- algorithmically
 *     reconstructed; remaining differences are source shape and FP scheduling
 */

Vec Xaxis = {1.0f, 0.0f, 0.0f};
Vec Yaxis = {0.0f, 1.0f, 0.0f};
Vec Zaxis = {0.0f, 0.0f, 1.0f};
Quat identity_quat = {0.0f, 0.0f, 0.0f, 1.0f};
MKMATRIX tmp_matrix;

static const float kZero = 0.0f;
static const float kHalf = 0.5f;
static const float kOne = 1.0f;
static const float kTwo = 2.0f;
static const float kThree = 3.0f;
static const float kNegOne = -1.0f;
static const float kEps = 0.001f;
static const float kTiny = 0.00001f;
static const float kInvSqrtScale = 0.0625f;
static const float kNewton12 = 12.0f;
static const float kPi = 3.1415927f;
static const float kTwoPi = 6.2831855f;
static const float kNegPi = -3.1415927f;
static const float kHalfPi = 1.5707964f;
static const float kNegHalfPi = -1.5707964f;
static const float kRadToDeg = 57.29578f;
static const float kAngToFixed = 166886.1f;
static const float kFixedToAng = 0.000005992112f;
static const float kSlerpDotThresh = 0.999f;
static const float kSlerpNormDotThresh = 1.001f;
static const float kV3ToQuatParallel = 0.9999f;
static const float kV3ToQuatAntiParallel = -0.9999f;
static const float kHugeNeg = -1.0e21f;
static const float kHugePos = 1.0e21f;

/* Fast reciprocal sqrt (Quake-style) used by normalize_* / unit helpers. */
static float mk_inv_sqrt(float x) {
    union {
        float f;
        unsigned int u;
    } pun;
    float guess;
    float t1;
    float t3;

    if (!(kZero < x)) {
        return kZero;
    }
    pun.f = x;
    pun.u = 0x5F375A00U - (pun.u >> 1);
    guess = pun.f;
    t1 = guess * x * guess;
    t3 = kThree - t1;
    return kInvSqrtScale * guess * t3 * (kNewton12 - (t1 * t3 * t3));
}

/* GXMathSqrtTable sqrt used by length_* / dist_* (same pattern as cam.c). */
static float mk_sqrt_table(float x) {
    union {
        float f;
        unsigned int u;
    } pun;
    unsigned int bits;
    unsigned int mantissa_exp;
    float guess;

    if (!(kZero < x)) {
        return kZero;
    }
    pun.f = x;
    bits = pun.u;
    mantissa_exp = (unsigned int)GXMathSqrtTable[(bits >> 10) & 0x3FFE] << 8;
    mantissa_exp |= (((bits & 0x7F800000U) + 0x3F800000U) >> 1) & 0x7F800000U;
    pun.u = mantissa_exp;
    guess = pun.f;
    return kHalf * guess * (kThree - (guess * guess) / x);
}

int intersect_xz_lines(const Vec* p, const Vec* dir, Vec* out, float a, float b) {
    float dx;
    float dz;
    float cross;
    float t;
    float bx;
    float bz;

    dx = dir->x;
    dz = dir->z;
    cross = p->x * dz + p->z * (-dx);
    if (cross == kZero) {
        return 0;
    }
    bx = dx * b;
    bz = dz * b;
    t = -(-a + (p->x * bx + p->z * bz)) / cross;
    out->x = dz * t + bx;
    out->z = (-dx) * t + bz;
    out->y = kZero;
    return 1;
}

/* Soft ceiling: parametric_ray_to_point ~50% -- MWCC -O4,p interleaves loads vs retail
 * sequential fmadds/stfs; algo matches ASM. */
void parametric_ray_to_point(Vec* out, const Vec* origin, const Vec* dir, float t) {
    out->x = dir->x * t + origin->x;
    out->y = dir->y * t + origin->y;
    out->z = dir->z * t + origin->z;
}

/* Retail infinite-cylinder/ray solver. */
int ray_cyl_intersection(const Vec* origin, const Vec* dir, const Vec* cylPos, const Vec* cylAxis,
                         float radius, float* tNear, float* tFar) {
    float ax = cylAxis->x;
    float ay = cylAxis->y;
    float az = cylAxis->z;
    float dx = dir->x;
    float dy = dir->y;
    float dz = dir->z;
    float ox = origin->x - cylPos->x;
    float oy = origin->y - cylPos->y;
    float oz = origin->z - cylPos->z;
    /* N is perpendicular to both the ray direction and cylinder axis. */
    float nx = dy * az - dz * ay;
    float ny = dz * ax - dx * az;
    float nz = dx * ay - dy * ax;
    float lenN = nx * nx + ny * ny + nz * nz;
    float invLen = kZero;
    float dist;
    int hit;
    float fx;
    float fy;
    float fz;
    float lenQ;
    float invQ;
    float tMid;
    float halfChord;
    float denom;
    float chordOffset;
    float lenO;

    if (kZero < lenN) {
        invLen = mk_sqrt_table(lenN);
    }

    if (kTiny <= invLen) {
        invLen = kOne / invLen;
        nx *= invLen;
        ny *= invLen;
        nz *= invLen;
        dist = ox * nx + oy * ny + oz * nz;
        if (dist < kZero) {
            dist = -dist;
        }
        hit = (dist <= radius);
        if (hit) {
            /* F lies in the ray/axis plane and is perpendicular to the axis. */
            fx = ny * az - nz * ay;
            fy = nz * ax - nx * az;
            fz = nx * ay - ny * ax;
            lenQ = fx * fx + fy * fy + fz * fz;
            invQ = kZero;
            if (kZero < lenQ) {
                invQ = mk_inv_sqrt(lenQ);
            }
            /* Closest approach parameter along ray (retail cross form). */
            tMid = -((ox * ay - oy * ax) * nz + (oy * az - oz * ay) * nx +
                     (oz * ax - ox * az) * ny);
            tMid *= invLen;
            halfChord = radius * radius - dist * dist;
            if (kZero < halfChord) {
                halfChord = mk_sqrt_table(halfChord);
            } else {
                halfChord = kZero;
            }
            denom = dx * (fx * invQ) + dy * (fy * invQ) + dz * (fz * invQ);
            chordOffset = halfChord / denom;
            if (chordOffset < kZero) {
                chordOffset = -chordOffset;
            }
            *tNear = tMid - chordOffset;
            *tFar = tMid + chordOffset;
        }
        return hit;
    }

    /* Degenerate parallel case: compare the ray's radial distance to the cylinder. */
    dist = -(ox * ax + oy * ay + oz * az);
    ox = ax * dist + ox;
    oy = ay * dist + oy;
    oz = az * dist + oz;
    lenO = ox * ox + oy * oy + oz * oz;
    dist = kZero;
    if (kZero < lenO) {
        dist = mk_sqrt_table(lenO);
    }
    *tNear = kHugeNeg;
    *tFar = kHugePos;
    return dist <= radius;
}

float dist2_xz_to_xz(const Vec* a, const Vec* b) {
    float dx = b->x - a->x;
    float dz = b->z - a->z;
    return dx * dx + dz * dz;
}

float dist_xz_to_xz(const Vec* a, const Vec* b) {
    return mk_sqrt_table(dist2_xz_to_xz(a, b));
}

void rotate_xz(Vec* out, const Vec* v, float ang) {
    float c = gxMathCos(ang);
    float s = gxMathSin(ang);
    float x = v->x;
    float z = v->z;
    out->x = z * s + x * c;
    out->z = z * c - x * s;
}

void xz_x_v_add_xz(Vec* dst, const Vec* v, float s) {
    dst->x = v->x * s + dst->x;
    dst->z = v->z * s + dst->z;
}

/* Soft ceiling: normalize_xz ~93% -- invsqrt inline near-miss (f6 x-keep / cror). */
void normalize_xz(Vec* v) {
    float inv = mk_inv_sqrt(v->x * v->x + v->z * v->z);
    v->x *= inv;
    v->z *= inv;
}

float length_xz(const Vec* v) {
    return mk_sqrt_table(v->x * v->x + v->z * v->z);
}

float xz_dot_xz(const Vec* a, const Vec* b) {
    return a->x * b->x + a->z * b->z;
}

float xz_unit_vector_recip(Vec* out, const Vec* from, const Vec* to) {
    float inv;

    out->y = kZero;
    out->x = to->x - from->x;
    out->z = to->z - from->z;
    inv = mk_inv_sqrt(out->x * out->x + out->z * out->z);
    out->x *= inv;
    out->z *= inv;
    return inv;
}

void xz_unit_vector(Vec* out, const Vec* from, const Vec* to) {
    float inv;

    out->y = kZero;
    out->x = to->x - from->x;
    out->z = to->z - from->z;
    inv = mk_inv_sqrt(out->x * out->x + out->z * out->z);
    out->x *= inv;
    out->z *= inv;
}

/* Soft ceiling: xz_to_y_ang ~79% -- mtlr spill vs load schedule only. */
float xz_to_y_ang(const Vec* v) {
    return gxMathArcTanYX(v->x, v->z);
}

/* Soft ceiling: scale_xz ~41% -- MWCC parallel lfs/fmuls vs retail f0 reuse. */
void scale_xz(Vec* out, const Vec* v, float s) {
    out->x = v->x * s;
    out->z = v->z * s;
}

void midpoint_v3(Vec* out, const Vec* a, const Vec* b) {
    out->x = kHalf * (a->x + b->x);
    out->y = kHalf * (a->y + b->y);
    out->z = kHalf * (a->z + b->z);
}

float dist2_v3_to_v3(const Vec* a, const Vec* b) {
    float dx = b->x - a->x;
    float dy = b->y - a->y;
    float dz = b->z - a->z;
    return dx * dx + dy * dy + dz * dz;
}

float dist_v3_to_v3(const Vec* a, const Vec* b) {
    return mk_sqrt_table(dist2_v3_to_v3(a, b));
}

void uv_from_angle_y(Vec* out, float angY) {
    out->x = gxMathSin(angY);
    out->y = kZero;
    out->z = gxMathCos(angY);
}

void uv_from_angles_xy(Vec* out, float angX, float angY) {
    float cx;
    float sx;
    angX = -angX;
    sx = gxMathSin(angX);
    cx = gxMathCos(angX);
    out->y = sx;
    out->x = cx * gxMathSin(angY);
    out->z = cx * gxMathCos(angY);
}

float uv_v3_to_v3_dist(Vec* out, const Vec* from, const Vec* to) {
    float len;
    float inv;
    out->x = to->x - from->x;
    out->y = to->y - from->y;
    out->z = to->z - from->z;
    len = mk_sqrt_table(out->x * out->x + out->y * out->y + out->z * out->z);
    inv = kZero;
    if (kZero < len) {
        inv = kOne / len;
    }
    out->x *= inv;
    out->y *= inv;
    out->z *= inv;
    return len;
}

void uv_v3_to_v3(Vec* out, const Vec* from, const Vec* to) {
    float inv;

    out->x = to->x - from->x;
    out->y = to->y - from->y;
    out->z = to->z - from->z;
    inv = mk_inv_sqrt(out->x * out->x + out->y * out->y + out->z * out->z);
    out->x *= inv;
    out->y *= inv;
    out->z *= inv;
}

void v3_blend3(Vec* out, const Vec* weights, const Vec* a, const Vec* b, const Vec* c) {
    out->x = weights->z * c->x + weights->x * a->x + weights->y * b->x;
    out->y = weights->z * c->y + weights->x * a->y + weights->y * b->y;
    out->z = weights->z * c->z + weights->x * a->z + weights->y * b->z;
}

/* Returns the pre-normalization length, as required by retail callers. */
float normalize_v3_length(Vec* v) {
    float len = mk_sqrt_table(v->x * v->x + v->y * v->y + v->z * v->z);
    float inv = kZero;
    if (kZero < len) {
        inv = kOne / len;
    }
    v->x *= inv;
    v->y *= inv;
    v->z *= inv;
    return len;
}

/* Soft ceiling: normalize_v3 ~94% -- invsqrt inline near-miss (f6 x-keep / cror). */
void normalize_v3(Vec* v) {
    float inv = mk_inv_sqrt(v->x * v->x + v->y * v->y + v->z * v->z);
    v->x *= inv;
    v->y *= inv;
    v->z *= inv;
}

void zero_v3(Vec* v) {
    v->z = kZero;
    v->y = kZero;
    v->x = kZero;
}

float length_v3(const Vec* v) {
    return mk_sqrt_table(v->x * v->x + v->y * v->y + v->z * v->z);
}

void v3_cross_v3(Vec* out, const Vec* a, const Vec* b) {
    out->x = a->y * b->z - a->z * b->y;
    out->y = a->z * b->x - a->x * b->z;
    out->z = a->x * b->y - a->y * b->x;
}

float v3_dot_v3(const Vec* a, const Vec* b) {
    return a->x * b->x + a->y * b->y + a->z * b->z;
}

/* Soft ceiling: v3_sub_v3 / v3_add_v3 / v3_add_v3_scaled ~50% -- schedule only;
 * arg/store order matches retail ASM. */
void v3_sub_v3(Vec* out, const Vec* a, const Vec* b) {
    out->x = a->x - b->x;
    out->y = a->y - b->y;
    out->z = a->z - b->z;
}

void v3_add_v3_scaled(Vec* out, const Vec* a, const Vec* b, float s) {
    out->x = b->x * s + a->x;
    out->y = b->y * s + a->y;
    out->z = b->z * s + a->z;
}

void v3_add_v3(Vec* out, const Vec* a, const Vec* b) {
    out->x = a->x + b->x;
    out->y = a->y + b->y;
    out->z = a->z + b->z;
}

void v3_x_v_add_v3(Vec* dst, const Vec* v, float s) {
    dst->x = v->x * s + dst->x;
    dst->y = v->y * s + dst->y;
    dst->z = v->z * s + dst->z;
}

void scale_v3(Vec* out, const Vec* v, float s) {
    out->x = v->x * s;
    out->y = v->y * s;
    out->z = v->z * s;
}

void interp_v3(Vec* out, const Vec* a, const Vec* b, float t) {
    float s = kOne - t;
    out->x = a->x * t + b->x * s;
    out->y = a->y * t + b->y * s;
    out->z = a->z * t + b->z * s;
}

void norm_angles_v3(Vec* ang) {
    ang->x = norm_angle(ang->x);
    ang->y = norm_angle(ang->y);
    ang->z = norm_angle(ang->z);
}

float norm_angle(float ang) {
    return ((int)(ang * kAngToFixed) & 0xFFFFF) * kFixedToAng;
}

void v3_to_xz_ang(Vec* ang, const Vec* v) {
    float len;
    ang->z = gxMathArcTanYX(v->y, v->x);
    ang->y = kZero;
    len = mk_sqrt_table(v->x * v->x + v->y * v->y);
    ang->x = gxMathArcTanYX(v->z, len);
}

void v3_to_xy_ang_high_freq(Vec* ang, const Vec* v) {
    float len;
    ang->z = kZero;
    len = mk_sqrt_table(v->x * v->x + v->z * v->z);
    ang->y = (float)atan2((double)v->x, (double)v->z);
    ang->x = -(float)atan2((double)v->y, (double)len);
}

void v3_to_xy_ang(Vec* ang, const Vec* v) {
    float len;
    ang->z = kZero;
    ang->y = gxMathArcTanYX(v->x, v->z);
    len = mk_sqrt_table(v->x * v->x + v->z * v->z);
    ang->x = -gxMathArcTanYX(v->y, len);
}

void mat_scaled_by_v3(MKMATRIX* out, const MKMATRIX* m, const Vec* scale) {
    out->right.x = m->right.x * scale->x;
    out->right.y = m->right.y * scale->x;
    out->right.z = m->right.z * scale->x;
    out->up.x = m->up.x * scale->y;
    out->up.y = m->up.y * scale->y;
    out->up.z = m->up.z * scale->y;
    out->at.x = m->at.x * scale->z;
    out->at.y = m->at.y * scale->z;
    out->at.z = m->at.z * scale->z;
    out->flags &= ~1U;
}

void v3_x_mat_sub_v3(Vec* out, const Vec* v, const MKMATRIX* m, const Vec* sub) {
    out->x = (v->z * m->at.x + v->x * m->right.x + v->y * m->up.x) - sub->x;
    out->y = (v->z * m->at.y + v->x * m->right.y + v->y * m->up.y) - sub->y;
    out->z = (v->z * m->at.z + v->x * m->right.z + v->y * m->up.z) - sub->z;
}

void v3_x_mat_add_v3(Vec* out, const Vec* v, const MKMATRIX* m, const Vec* add) {
    out->x = add->x + v->z * m->at.x + v->x * m->right.x + v->y * m->up.x;
    out->y = add->y + v->z * m->at.y + v->x * m->right.y + v->y * m->up.y;
    out->z = add->z + v->z * m->at.z + v->x * m->right.z + v->y * m->up.z;
}

void v3_x_mat(Vec* out, const Vec* v, const MKMATRIX* m) {
    out->x = v->z * m->at.x + v->x * m->right.x + v->y * m->up.x;
    out->y = v->z * m->at.y + v->x * m->right.y + v->y * m->up.y;
    out->z = v->z * m->at.z + v->x * m->right.z + v->y * m->up.z;
}

void p3_x_mat(Vec* out, const Vec* p, const MKMATRIX* m) {
    out->x = m->pos.x + p->z * m->at.x + p->x * m->right.x + p->y * m->up.x;
    out->y = m->pos.y + p->z * m->at.y + p->x * m->right.y + p->y * m->up.y;
    out->z = m->pos.z + p->z * m->at.z + p->x * m->right.z + p->y * m->up.z;
}

void mat_x_mat(MKMATRIX* out, const MKMATRIX* a, const MKMATRIX* b) {
    out->right.x = a->right.z * b->at.x + a->right.x * b->right.x + a->right.y * b->up.x;
    out->right.y = a->right.z * b->at.y + a->right.x * b->right.y + a->right.y * b->up.y;
    out->right.z = a->right.z * b->at.z + a->right.x * b->right.z + a->right.y * b->up.z;
    out->up.x = a->up.z * b->at.x + a->up.x * b->right.x + a->up.y * b->up.x;
    out->up.y = a->up.z * b->at.y + a->up.x * b->right.y + a->up.y * b->up.y;
    out->up.z = a->up.z * b->at.z + a->up.x * b->right.z + a->up.y * b->up.z;
    out->at.x = a->at.z * b->at.x + a->at.x * b->right.x + a->at.y * b->up.x;
    out->at.y = a->at.z * b->at.y + a->at.x * b->right.y + a->at.y * b->up.y;
    out->at.z = a->at.z * b->at.z + a->at.x * b->right.z + a->at.y * b->up.z;
    out->flags = a->flags & b->flags;
}

void set_mat(MKMATRIX* dst, const MKMATRIX* src) {
    *dst = *src;
}

/* Soft ceiling: ang_sub_ang ~98.5% -- instruction-identical; sdata2 reloc/pool leftover. */
float ang_sub_ang(float a, float b) {
    float d = a - b;
    if (d > kPi) {
        d -= kTwoPi;
        return d;
    }
    if (d < kNegPi) {
        d += kTwoPi;
        return d;
    }
    return d;
}

float quat_extract_ang_y(const Quat* q) {
    float t = -(kTwo * (q->x * q->x + q->y * q->y) - kOne);
    float s = kTwo * (q->z * q->x + q->w * q->y);
    if (t >= kZero) {
        if (t < kTiny) {
            return kNegHalfPi;
        }
        {
            float ang = gxMathArcTan(s / t);
            if (ang < kZero) {
                ang = kTwoPi + ang;
            }
            return ang;
        }
    } else {
        if (-t < kTiny) {
            return kHalfPi;
        }
        {
            float ang = gxMathArcTan(s / t);
            return kPi + ang;
        }
    }
}

void interp_quat(Quat* out, const Quat* q1, const Quat* q2, float t) {
    float sign = kOne;
    float oneMinusT;
    float dot;
    float invSin;
    float theta;
    float len;
    float inv;

    if (t < kZero) {
        t = kZero;
    }
    if (kOne < t) {
        t = kOne;
    }
    oneMinusT = kOne - t;
    dot = q1->x * q2->x + q1->y * q2->y + q1->z * q2->z + q1->w * q2->w;
    if (dot < kZero) {
        dot = -dot;
        sign = kNegOne;
    }
    if (dot < kSlerpDotThresh) {
        theta = gxMathArcCos(dot);
        invSin = kOne / gxMathSin(theta);
        t = invSin * gxMathSin(t * theta);
        oneMinusT = invSin * gxMathSin(oneMinusT * theta);
    }
    oneMinusT *= sign;
    out->x = t * q1->x + oneMinusT * q2->x;
    out->y = t * q1->y + oneMinusT * q2->y;
    out->z = t * q1->z + oneMinusT * q2->z;
    out->w = t * q1->w + oneMinusT * q2->w;
    if (kSlerpNormDotThresh < dot) {
        len = out->x * out->x + out->y * out->y + out->z * out->z + out->w * out->w;
        if (len < kSlerpDotThresh || kSlerpNormDotThresh < len) {
            inv = mk_inv_sqrt(len);
            out->x *= inv;
            out->y *= inv;
            out->z *= inv;
            out->w *= inv;
        }
    }
}

void quat_x_quat(Quat* out, const Quat* a, const Quat* b) {
    float ax = a->x;
    float ay = a->y;
    float az = a->z;
    float aw = a->w;
    float bx = b->x;
    float by = b->y;
    float bz = b->z;
    float bw = b->w;
    out->x = -(az * by - (ay * bz + aw * bx + ax * bw));
    out->y = -(ax * bz - (az * bx + aw * by + ay * bw));
    out->z = -(ay * bx - (ax * by + aw * bz + az * bw));
    out->w = -(az * bz - -(ay * by - (aw * bw - ax * bx)));
}

void v3_v3_to_quat(Quat* out, const Vec* v1, const Vec* v2) {
    float dot = v1->x * v2->x + v1->y * v2->y + v1->z * v2->z;
    float ax;
    float ay;
    float az;
    float len;
    float inv;
    float half;
    float w;

    if (kV3ToQuatParallel < dot) {
        out->x = kZero;
        out->y = kZero;
        out->z = kZero;
        out->w = kOne;
        return;
    }
    if (dot < kV3ToQuatAntiParallel) {
        ax = kZero;
        ay = -v1->y;
        az = v1->x;
        len = mk_sqrt_table(ax * ax + ay * ay + az * az);
        if (len < kEps) {
            ax = -v1->z;
            ay = kZero;
            az = v1->x;
        }
        inv = mk_inv_sqrt(ax * ax + ay * ay + az * az);
        out->x = ax * inv;
        out->y = ay * inv;
        out->z = az * inv;
        out->w = kZero;
        return;
    }
    ax = v1->y * v2->z - v1->z * v2->y;
    ay = v1->z * v2->x - v1->x * v2->z;
    az = v1->x * v2->y - v1->y * v2->x;
    inv = mk_inv_sqrt(ax * ax + ay * ay + az * az);
    half = kHalf * (kOne - dot);
    w = mk_sqrt_table(half);
    half = kHalf * (kOne + dot);
    out->x = ax * inv * w;
    out->y = ay * inv * w;
    out->z = az * inv * w;
    out->w = mk_sqrt_table(half);
}

void quat_to_mat(MKMATRIX* out, const Quat* q) {
    float x = q->x;
    float y = q->y;
    float z = q->z;
    float w = q->w;
    out->right.x = -(kTwo * (y * y + z * z) - kOne);
    out->right.y = kTwo * (x * y + w * z);
    out->right.z = kTwo * (z * x - w * y);
    out->up.x = kTwo * (x * y - w * z);
    out->up.y = -(kTwo * (x * x + z * z) - kOne);
    out->up.z = kTwo * (y * z + w * x);
    out->at.x = kTwo * (z * x + w * y);
    out->at.y = kTwo * (y * z - w * x);
    out->at.z = -(kTwo * (x * x + y * y) - kOne);
    out->flags = 3;
}

void YXZ_angles_to_quat(const Vec* angles, Quat* out) {
    float cx;
    float sx;
    float cy;
    float sy;
    float cz;
    float sz;
    MKMATRIX m;

    gxMathCosSin(&cx, &sx, angles->x);
    gxMathCosSin(&cy, &sy, angles->y);
    gxMathCosSin(&cz, &sz, angles->z);

    /* Same YXZ rotation matrix as YXZ_angles_to_MKMATRIX, then Quat. */
    m.right.x = sx * sy * sz + cy * cz;
    m.right.y = cx * sz;
    m.right.z = cy * sz * sx - cz * sy;
    m.up.x = sx * cz * sy - cy * sz;
    m.up.y = cx * cz;
    m.up.z = sx * cy * cz + sy * sz;
    m.at.x = sy * cx;
    m.at.y = -sx;
    m.at.z = cy * cx;
    m.flags = 3;
    m.pos.x = kZero;
    m.pos.y = kZero;
    m.pos.z = kZero;
    RtQuatConvertFromMatrix(out, &m);
}

void mat_to_quat(Quat* out, const MKMATRIX* m) {
    RtQuatConvertFromMatrix(out, m);
}

void XYZ_angles_to_MKMATRIX(const Vec* angles, MKMATRIX* m) {
    Vec saved;
    Vec neg;
    saved.x = m->pos.x;
    saved.y = m->pos.y;
    saved.z = m->pos.z;
    neg.x = kNegOne * saved.x;
    neg.y = kNegOne * saved.y;
    neg.z = kNegOne * saved.z;
    RwMatrixTranslate(m, (const RwV3d*)&neg, 2);
    RwMatrixRotate(m, (const RwV3d*)&Xaxis, kRadToDeg * angles->x, 0);
    if (angles->y != kZero) {
        RwMatrixRotate(m, (const RwV3d*)&Yaxis, kRadToDeg * angles->y, 1);
    }
    if (angles->z != kZero) {
        RwMatrixRotate(m, (const RwV3d*)&Zaxis, kRadToDeg * angles->z, 1);
    }
    RwMatrixTranslate(m, (const RwV3d*)&saved, 2);
}

void ZYX_angles_to_MKMATRIX(const Vec* angles, MKMATRIX* m) {
    Vec saved;
    Vec neg;
    saved.x = m->pos.x;
    saved.y = m->pos.y;
    saved.z = m->pos.z;
    neg.x = kNegOne * saved.x;
    neg.y = kNegOne * saved.y;
    neg.z = kNegOne * saved.z;
    RwMatrixTranslate(m, (const RwV3d*)&neg, 2);
    RwMatrixRotate(m, (const RwV3d*)&Zaxis, kRadToDeg * angles->z, 0);
    if (angles->y != kZero) {
        RwMatrixRotate(m, (const RwV3d*)&Yaxis, kRadToDeg * angles->y, 1);
    }
    if (angles->x != kZero) {
        RwMatrixRotate(m, (const RwV3d*)&Xaxis, kRadToDeg * angles->x, 1);
    }
    RwMatrixTranslate(m, (const RwV3d*)&saved, 2);
}

void YXZ_angles_to_MKMATRIX(const Vec* angles, MKMATRIX* m) {
    float cx;
    float sx;
    float cy;
    float sy;
    float cz;
    float sz;

    gxMathCosSin(&cx, &sx, angles->x);
    gxMathCosSin(&cy, &sy, angles->y);
    gxMathCosSin(&cz, &sz, angles->z);

    m->right.x = sx * sy * sz + cy * cz;
    m->right.y = cx * sz;
    m->right.z = cy * sz * sx - cz * sy;
    m->up.x = sx * cz * sy - cy * sz;
    m->up.y = cx * cz;
    m->up.z = sx * cy * cz + sy * sz;
    m->at.x = sy * cx;
    m->at.y = -sx;
    m->at.z = cy * cx;
    m->flags = 3;
}

void y_angle_to_MKMATRIX(MKMATRIX* m, float angY) {
    float c;
    float s;
    gxMathCosSin(&c, &s, angY);
    m->right.x = c;
    m->right.y = kZero;
    m->right.z = -s;
    m->up.x = kZero;
    m->up.y = kOne;
    m->up.z = kZero;
    m->at.x = s;
    m->at.y = kZero;
    m->at.z = c;
    m->flags = 3;
}

MKMATRIX* MKMatrixRotateScaleTranslate(MKMATRIX* m, const Vec* axis, float angle, const Vec* scale,
                                       const Vec* translate) {
    RwMatrixRotate(m, (const RwV3d*)axis, angle, 0);
    RwMatrixScale(m, (const RwV3d*)scale, 2);
    RwMatrixTranslate(m, (const RwV3d*)translate, 2);
    return m;
}

MKMATRIX* MKMatrixRotatXZYScaleTranslate(MKMATRIX* m, float angX, float angZ, float angY,
                                         const Vec* scale, const Vec* translate) {
    Vec xax = {1.0f, 0.0f, 0.0f};
    Vec yax = {0.0f, 1.0f, 0.0f};
    Vec zax = {0.0f, 0.0f, 1.0f};
    RwMatrixRotate(m, (const RwV3d*)&xax, angX, 0);
    RwMatrixRotate(m, (const RwV3d*)&zax, angZ, 1);
    RwMatrixRotate(m, (const RwV3d*)&yax, angY, 1);
    RwMatrixScale(m, (const RwV3d*)scale, 1);
    RwMatrixTranslate(m, (const RwV3d*)translate, 2);
    return m;
}

void MKMatrixSetIdentity(MKMATRIX* m) {
    m->at.z = kOne;
    m->up.y = kOne;
    m->right.x = kOne;
    m->up.x = kZero;
    m->right.z = kZero;
    m->right.y = kZero;
    m->at.y = kZero;
    m->at.x = kZero;
    m->up.z = kZero;
    m->pos.z = kZero;
    m->pos.y = kZero;
    m->pos.x = kZero;
    m->flags |= 0x20003U;
}

MKMATRIX* MKMatrixTranslate(MKMATRIX* m, const Vec* delta, int combine) {
    return RwMatrixTranslate(m, (const RwV3d*)delta, combine);
}

MKMATRIX* MKMatrixScale(MKMATRIX* m, const Vec* scale, int combine) {
    return RwMatrixScale(m, (const RwV3d*)scale, combine);
}

MKMATRIX* MKMatrixRotate(MKMATRIX* m, const Vec* axis, float angle, int combine) {
    return RwMatrixRotate(m, (const RwV3d*)axis, angle, combine);
}
