#include "math/mk_math.h"
#include "math/gxMath.h"
#include "runtime/cmath.h"

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
static const float kSlerpDotThresh = 0.999f;
static const float kSlerpNormDotThresh = 1.001f;
static const float kV3ToQuatParallel = 0.9999f;
static const float kV3ToQuatAntiParallel = -0.9999f;
static const float kHugeNeg = -1.0e21f;
static const float kHugePos = 1.0e21f;

static float mk_inv_sqrt(float x) {
    union {
        float f;
        unsigned int u;
    } pun;
    float guess;
    float scaled_input;
    float correction;

    if (x <= kZero) {
        return kZero;
    }
    pun.f = x;
    pun.u = 0x5F375A00U - (pun.u >> 1);
    guess = pun.f;
    scaled_input = guess * (x * guess);
    correction = kThree - scaled_input;
    guess = kInvSqrtScale * guess;
    return guess * correction * (kNewton12 - (scaled_input * correction * correction));
}

/* TODO: [near miss] 97.59%; coefficient preparation and FP operations agree; input/component register coloring remains. */
int intersect_xz_lines(const Vec* p, const Vec* dir, Vec* out, float a, float b) {
    float dx;
    float dz;
    float neg_dx;
    float bx;
    float bz;
    float neg_a;
    float cross;
    float t;

    dx = dir->x;
    dz = dir->z;
    neg_a = -a;
    neg_dx = -dx;
    bx = dx * b;
    bz = dz * b;
    cross = p->x * dz + p->z * neg_dx;
    if (cross == kZero) {
        return 0;
    }
    t = -(neg_a + (p->x * bx + p->z * bz)) / cross;
    out->x = dz * t + bx;
    out->z = neg_dx * t + bz;
    out->y = kZero;
    return 1;
}

#pragma push
#pragma scheduling off
void parametric_ray_to_point(Vec* out, const Vec* origin, const Vec* dir, float t) {
    out->x = dir->x * t + origin->x;
    out->y = dir->y * t + origin->y;
    out->z = dir->z * t + origin->z;
}
#pragma pop

/* TODO: [breakthrough] 74.06%; ordered sqrt guards and parallel early exit recovered; Newton/geometry scheduling and FPR frame remain. */
int ray_cyl_intersection(const Vec* origin, const Vec* dir, const Vec* cylPos, const Vec* cylAxis,
                         float radius, float* tNear, float* tFar) {
    float nx = dir->y * cylAxis->z - dir->z * cylAxis->y;
    float ny = dir->z * cylAxis->x - dir->x * cylAxis->z;
    float nz = dir->x * cylAxis->y - dir->y * cylAxis->x;
    float lenN = nx * nx + ny * ny + nz * nz;
    float ox = origin->x - cylPos->x;
    float oy = origin->y - cylPos->y;
    float oz = origin->z - cylPos->z;
    float invLen = gxMathFastSqrt(lenN);
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

    if (invLen < kTiny) {
        dist = -(ox * cylAxis->x + oy * cylAxis->y + oz * cylAxis->z);
        ox = cylAxis->x * dist + ox;
        oy = cylAxis->y * dist + oy;
        oz = cylAxis->z * dist + oz;
        lenO = ox * ox + oy * oy + oz * oz;
        dist = gxMathFastSqrt(lenO);
        *tNear = kHugeNeg;
        *tFar = kHugePos;
        return dist <= radius;
    }

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
        fx = ny * cylAxis->z - nz * cylAxis->y;
        fy = nz * cylAxis->x - nx * cylAxis->z;
        fz = nx * cylAxis->y - ny * cylAxis->x;
        lenQ = fx * fx + fy * fy + fz * fz;
        invQ = mk_inv_sqrt(lenQ);
        tMid = -((ox * cylAxis->y - oy * cylAxis->x) * nz + (oy * cylAxis->z - oz * cylAxis->y) * nx +
                 (oz * cylAxis->x - ox * cylAxis->z) * ny);
        tMid *= invLen;
        halfChord = radius * radius - dist * dist;
        halfChord = gxMathFastSqrt(halfChord);
        denom = dir->x * (fx * invQ) + dir->y * (fy * invQ) + dir->z * (fz * invQ);
        chordOffset = halfChord / denom;
        if (chordOffset < kZero) {
            chordOffset = -chordOffset;
        }
        *tNear = tMid - chordOffset;
        *tFar = tMid + chordOffset;
    }
    return hit;
}


float dist2_xz_to_xz(const Vec* a, const Vec* b) {
    float dx = b->x - a->x;
    float dz = b->z - a->z;
    return dx * dx + dz * dz;
}

/* TODO: [near miss] 62.16%; body agrees; frame setup scheduled late; scoped scheduler control regresses. */
float dist_xz_to_xz(const Vec* a, const Vec* b) {
    return gxMathFastSqrt(dist2_xz_to_xz(a, b));
}

void rotate_xz(Vec* out, const Vec* v, float ang)
{
    float x_component;
    float z = v->z;
    float x = v->x;
    x_component = x * gxMathCos(ang);
    out->x = z * gxMathSin(ang) + x_component;
    x_component = x * gxMathSin(ang);
    out->z = z * gxMathCos(ang) - x_component;
}

#pragma push
#pragma scheduling off
void xz_x_v_add_xz(Vec* dst, const Vec* v, float s) {
    dst->x = v->x * s + dst->x;
    dst->z = v->z * s + dst->z;
}
#pragma pop

void normalize_xz(Vec* v) {
    float inv = mk_inv_sqrt(v->x * v->x + v->z * v->z);
    v->x = v->x * inv;
    v->z *= inv;
}

/* TODO: [breakthrough] 70.59%; sqrt table indexing corrected; inlined sqrt-table scheduling differs. */
float length_xz(const Vec* v) {
    return gxMathFastSqrt(v->x * v->x + v->z * v->z);
}

float xz_dot_xz(const Vec* a, const Vec* b) {
    return a->x * b->x + a->z * b->z;
}

/* TODO: [near miss] 99.46%; rounded squares/refinement agree; three square-register rows remain; stop at coloring. */
float xz_unit_vector_recip(Vec* out, Vec* from, Vec* to) {
    float inv;
    float x;
    float z_squared;
    float x_squared;

    out->y = kZero;
    out->x = to->x - from->x;
    out->z = to->z - from->z;
    x = out->x;
    x_squared = x * x;
    z_squared = out->z * out->z;
    inv = mk_inv_sqrt(z_squared + x_squared);
    out->x = x * inv;
    out->z *= inv;
    return inv;
}

/* TODO: [near miss] 93.67%; retained X and rounded squares agree;
 * read-only input load scheduling and inverse-square-root FPR homes remain. */
void xz_unit_vector(Vec* out, Vec* from, const Vec* to) {
    float inv;
    float x;
    float x_squared;
    float z_squared;

    out->y = kZero;
    out->x = to->x - from->x;
    out->z = to->z - from->z;
    x = out->x;
    x_squared = x * x;
    z_squared = out->z * out->z;
    inv = mk_inv_sqrt(x_squared + z_squared);
    out->x = x * inv;
    out->z *= inv;
}

#pragma push
#pragma scheduling off
float xz_to_y_ang(const Vec* v) {
    return gxMathArcTanYX(v->x, v->z);
}
#pragma pop

#pragma push
#pragma scheduling off
void scale_xz(Vec* out, const Vec* v, float s) {
    out->x = v->x * s;
    out->z = v->z * s;
}
#pragma pop

#pragma push
#pragma scheduling off
/* TODO: [near miss] 92.35294%; only half-factor load precedes the x input loads. */
void midpoint_v3(Vec* out, const Vec* a, const Vec* b) {
    out->x = kHalf * (a->x + b->x);
    out->y = kHalf * (a->y + b->y);
    out->z = kHalf * (a->z + b->z);
}
#pragma pop

float dist2_v3_to_v3(const Vec* a, const Vec* b) {
    float dx = b->x - a->x;
    float dy = b->y - a->y;
    float dz = b->z - a->z;
    return dx * dx + dy * dy + dz * dz;
}

/* TODO: [near miss] 46.34%; body exact; frame setup delayed past eleven FP instructions; scheduling-off changes body. */
float dist_v3_to_v3(const Vec* a, const Vec* b) {
    return gxMathFastSqrt(dist2_v3_to_v3(a, b));
}

void uv_from_angle_y(Vec* out, float angY) {
    out->x = gxMathSin(angY);
    out->y = kZero;
    out->z = gxMathCos(angY);
}

void uv_from_angles_xy(Vec* out, float angX, float angY) {
    float cx;
    angX = -angX;
    out->y = gxMathSin(angX);
    cx = gxMathCos(angX);
    out->x = cx * gxMathSin(angY);
    out->z = cx * gxMathCos(angY);
}

/* TODO: [near miss] 76.03%; sqrt and normalization agree; input-load scheduling remains. */
float uv_v3_to_v3_dist(Vec* out, const Vec* from, const Vec* to) {
    float len;
    float inv;
    float x_squared;
    float y_squared;
    float z_squared;

    out->x = to->x - from->x;
    out->y = to->y - from->y;
    out->z = to->z - from->z;
    x_squared = out->x * out->x;
    y_squared = out->y * out->y;
    z_squared = out->z * out->z;
    len = gxMathFastSqrt(z_squared + (x_squared + y_squared));
    if (len > 0.0f) {
        inv = 1.0f / len;
    } else {
        inv = len;
    }
    out->x *= inv;
    out->y *= inv;
    out->z *= inv;
    return len;
}


/* TODO: [breakthrough] 70.74545%; ordered guard corrected; reciprocal-square-root FP scheduling remains. */
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

#pragma push
#pragma scheduling off
/* TODO: [near miss] 74.65%; rounded seed and nested FMAs agree; input-load and FPR scheduling remain. */
void v3_blend3(Vec* out, Vec* weights, const Vec* a, const Vec* b, const Vec* c) {
    out->x = weights->z * c->x + (weights->x * a->x + weights->y * b->x);
    out->y = weights->z * c->y + (weights->x * a->y + weights->y * b->y);
    out->z = weights->z * c->z + (weights->x * a->z + weights->y * b->z);
}
#pragma pop

float normalize_v3_length(Vec* v) {
    float len = gxMathFastSqrt(v->x * v->x + v->y * v->y + v->z * v->z);
    float inv;
    if (len > kZero) {
        inv = kOne / len;
    } else {
        inv = len;
    }
    v->x *= inv;
    v->y *= inv;
    v->z *= inv;
    return len;
}

void normalize_v3(Vec* v) {
    float inv = mk_inv_sqrt(v->x * v->x + v->y * v->y + v->z * v->z);
    v->x = v->x * inv;
    v->y *= inv;
    v->z *= inv;
}

void zero_v3(Vec* v) {
    v->z = kZero;
    v->y = kZero;
    v->x = kZero;
}

/* TODO: [near miss] 62.16%; body agrees; frame setup scheduled late; scoped scheduler control regresses. */
float length_v3(const Vec* v) {
    return gxMathFastSqrt(v->x * v->x + v->y * v->y + v->z * v->z);
}

void v3_cross_v3(Vec* out, Vec* a, Vec* b) {
    out->x = a->y * b->z - a->z * b->y;
    out->y = a->z * b->x - a->x * b->z;
    out->z = a->x * b->y - a->y * b->x;
}

float v3_dot_v3(const Vec* a, const Vec* b) {
    return a->x * b->x + a->y * b->y + a->z * b->z;
}

#pragma push
#pragma scheduling off
void v3_sub_v3(Vec* out, const Vec* a, const Vec* b) {
    out->x = a->x - b->x;
    out->y = a->y - b->y;
    out->z = a->z - b->z;
}
#pragma pop

#pragma push
#pragma scheduling off
void v3_add_v3_scaled(Vec* out, const Vec* a, const Vec* b, float s) {
    out->x = b->x * s + a->x;
    out->y = b->y * s + a->y;
    out->z = b->z * s + a->z;
}
#pragma pop

#pragma push
#pragma scheduling off
void v3_add_v3(Vec* out, const Vec* a, const Vec* b) {
    out->x = a->x + b->x;
    out->y = a->y + b->y;
    out->z = a->z + b->z;
}
#pragma pop

void v3_x_v_add_v3(Vec* dst, const Vec* v, float s) {
    dst->x = v->x * s + dst->x;
    dst->y = v->y * s + dst->y;
    dst->z = v->z * s + dst->z;
}

#pragma push
#pragma scheduling off
void scale_v3(Vec* out, const Vec* v, float s) {
    out->x = v->x * s;
    out->y = v->y * s;
    out->z = v->z * s;
}
#pragma pop

#pragma push
#pragma scheduling off
/* TODO: [near miss] 97.22%; unity FPR and y/z input-load order differ;
 * arithmetic agrees; new source lifetime evidence needed. */
void interp_v3(Vec* out, const Vec* a, const Vec* b, float t) {
    float s;
    float component;

    s = kOne;
    component = b->x;
    s -= t;
    out->x = a->x * t + component * s;
    component = b->y;
    out->y = a->y * t + component * s;
    component = b->z;
    out->z = a->z * t + component * s;
}
#pragma pop

void norm_angles_v3(Vec* ang) {
    ang->x = norm_angle_inline(ang->x);
    ang->y = norm_angle_inline(ang->y);
    ang->z = norm_angle_inline(ang->z);
}

float norm_angle(float ang) {
    return norm_angle_inline(ang);
}

void v3_to_xz_ang(Vec* ang, Vec* v) {
    float len;
    float length_squared;
    ang->z = gxMathArcTanYX(v->y, v->x);
    ang->y = kZero;
    length_squared = v->x * v->x + v->y * v->y;
    len = gxMathFastSqrt(length_squared);
    ang->x = gxMathArcTanYX(v->z, len);
}

/* TODO: [near miss] 85.00%; FP load/store scheduling differs; scoped scheduling-off regresses. */
void v3_to_xy_ang_high_freq(Vec* ang, const Vec* v) {
    float len;
    ang->z = kZero;
    len = gxMathFastSqrt(v->x * v->x + v->z * v->z);
    ang->y = atan2(v->x, v->z);
    ang->x = -(float)atan2(v->y, len);
}

void v3_to_xy_ang(Vec* ang, Vec* v) {
    float len;
    ang->z = kZero;
    ang->y = gxMathArcTanYX(v->x, v->z);
    len = gxMathFastSqrt(v->x * v->x + v->z * v->z);
    ang->x = -gxMathArcTanYX(v->y, len);
}

/* TODO: [breakthrough] 71.22%; in-place matrix ownership recovered;
 * factor const ownership through gore2 and repeated scale reloads remain. */
void mat_scaled_by_v3(MKMATRIX* out, MKMATRIX* m, const Vec* scale) {
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

void v3_x_mat_sub_v3(Vec* out, Vec* v, MKMATRIX* m, Vec* sub) {
    out->x = (v->x * m->right.x + v->y * m->up.x + v->z * m->at.x) - sub->x;
    out->y = (v->x * m->right.y + v->y * m->up.y + v->z * m->at.y) - sub->y;
    out->z = (v->x * m->right.z + v->y * m->up.z + v->z * m->at.z) - sub->z;
}

void v3_x_mat_add_v3(Vec* out, Vec* v, MKMATRIX* m, Vec* add) {
    out->x = add->x + (v->x * m->right.x + v->y * m->up.x + v->z * m->at.x);
    out->y = add->y + (v->x * m->right.y + v->y * m->up.y + v->z * m->at.y);
    out->z = add->z + (v->x * m->right.z + v->y * m->up.z + v->z * m->at.z);
}

void v3_x_mat(Vec* out, Vec* v, MKMATRIX* m) {
    out->x = v->x * m->right.x + v->y * m->up.x + v->z * m->at.x;
    out->y = v->x * m->right.y + v->y * m->up.y + v->z * m->at.y;
    out->z = v->x * m->right.z + v->y * m->up.z + v->z * m->at.z;
}

void p3_x_mat(Vec* out, Vec* p, MKMATRIX* m) {
    out->x = m->pos.x + (p->z * m->at.x + (p->x * m->right.x + p->y * m->up.x));
    out->y = m->pos.y + (p->z * m->at.y + (p->x * m->right.y + p->y * m->up.y));
    out->z = m->pos.z + (p->z * m->at.z + (p->x * m->right.z + p->y * m->up.z));
}

#pragma push
#pragma scheduling off
#pragma optimization_level 1
/* TODO: [breakthrough] 75.93684%; X/Y subtotal then Z agrees; verify operand scheduling and leaf optimization profile. */
void mat_x_mat(MKMATRIX* out, const MKMATRIX* a, const MKMATRIX* b) {
    out->right.x = a->right.x * b->right.x + a->right.y * b->up.x + a->right.z * b->at.x;
    out->right.y = a->right.x * b->right.y + a->right.y * b->up.y + a->right.z * b->at.y;
    out->right.z = a->right.x * b->right.z + a->right.y * b->up.z + a->right.z * b->at.z;
    out->up.x = a->up.x * b->right.x + a->up.y * b->up.x + a->up.z * b->at.x;
    out->up.y = a->up.x * b->right.y + a->up.y * b->up.y + a->up.z * b->at.y;
    out->up.z = a->up.x * b->right.z + a->up.y * b->up.z + a->up.z * b->at.z;
    out->at.x = a->at.x * b->right.x + a->at.y * b->up.x + a->at.z * b->at.x;
    out->at.y = a->at.x * b->right.y + a->at.y * b->up.y + a->at.z * b->at.y;
    out->at.z = a->at.x * b->right.z + a->at.y * b->up.z + a->at.z * b->at.z;
    out->flags = a->flags & b->flags;
}
#pragma pop

void set_mat(MKMATRIX* dst, const MKMATRIX* src) {
    *dst = *src;
}

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

/* TODO: [near miss] 51.15%; rounded products and branches agree; FP scheduling/register webs remain. */
float quat_extract_ang_y(const Quat* q) {
    float xx = q->x * q->x;
    float yy = q->y * q->y;
    float zx = q->z * q->x;
    float wy = q->w * q->y;
    float t = -(kTwo * (xx + yy) - kOne);
    float s = kTwo * (zx + wy);
    float ang;

    if (t >= kZero) {
        if (t < kTiny) {
            return kNegHalfPi;
        }
        ang = gxMathArcTan(s / t);
        if (ang < kZero) {
            ang = kTwoPi + ang;
        }
        return ang;
    } else {
        if (-t < kTiny) {
            return kHalfPi;
        }
        return kPi + gxMathArcTan(s / t);
    }
}

static inline void interpolate_quat_components(
    Quat* out, const Quat* q1, const Quat* q2,
    float weight_1, float weight_2) {
    out->x = weight_1 * q1->x + weight_2 * q2->x;
    out->y = weight_1 * q1->y + weight_2 * q2->y;
    out->z = weight_1 * q1->z + weight_2 * q2->z;
    out->w = weight_1 * q1->w + weight_2 * q2->w;
}

/* TODO: [near miss] 89.00%; weight/dot/sign FPR ownership and component load/store scheduling remain. */
void interp_quat(Quat* out, const Quat* q1, const Quat* q2, float t) {
    float sign;
    float oneMinusT;
    float dot;
    float theta;
    float invSin;
    float len;
    float inv;

    if (t < kZero) {
        t = kZero;
    }
    if (t > kOne) {
        t = kOne;
    }
    oneMinusT = kOne - t;
    sign = kOne;
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
    interpolate_quat_components(out, q1, q2, t, oneMinusT * sign);
    if (dot > kSlerpNormDotThresh) {
        len = out->x * out->x + out->y * out->y + out->z * out->z + out->w * out->w;
        if (len < kSlerpDotThresh || len > kSlerpNormDotThresh) {
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
    float x = ax * bw;
    float y = ay * bw;
    float z = az * bw;
    out->x = x + aw * bx + ay * bz - az * by;
    out->y = y + aw * by + az * bx - ax * bz;
    out->z = z + aw * bz + ax * by - ay * bx;
    out->w = aw * bw - ax * bx - ay * by - az * bz;
}

/* TODO: [near miss] 87.76382%; entry scheduling and antiparallel/sqrt FP homes remain. */
void v3_v3_to_quat(Quat* out, const Vec* v1, const Vec* v2) {
    float dot = v1->x * v2->x + v1->y * v2->y + v1->z * v2->z;
    float ax;
    float ay;
    float az;
    float len;
    float inv;
    float half;
    float w;

    if (dot > kV3ToQuatParallel) {
        out->x = kZero;
        out->y = kZero;
        out->z = kZero;
        out->w = kOne;
        return;
    }
    if (dot < kV3ToQuatAntiParallel) {
        ax = kZero;
        ay = v1->x;
        az = -v1->y;
        len = gxMathFastSqrt(ay * ay + az * az);
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
    ax *= inv;
    ay *= inv;
    az *= inv;
    half = kHalf * (kOne - dot);
    w = gxMathFastSqrt(half);
    ax *= w;
    ay *= w;
    az *= w;
    half = kHalf * (kOne + dot);
    out->x = ax;
    out->y = ay;
    out->z = az;
    out->w = gxMathFastSqrt(half);
}

/* TODO: [near miss] 95.82%; products, FP order and frame agree;
 * quaternion component/product FPR coloring remains. */
void quat_to_mat(MKMATRIX* out, const Quat* q) {
    float y = q->y;
    float z = q->z;
    float x = q->x;
    float yy = y * y;
    float w = q->w;
    float zz = z * z;
    float xy = x * y;
    float wz = w * z;
    float zx = z * x;
    float wy = w * y;
    float xx = x * x;
    float yz;
    float wx;

    out->right.x = -(kTwo * (yy + zz) - kOne);
    yz = y * z;
    wx = w * x;
    out->right.y = kTwo * (xy + wz);
    out->right.z = kTwo * (zx - wy);
    out->up.x = kTwo * (xy - wz);
    out->up.y = -(kTwo * (xx + zz) - kOne);
    out->up.z = kTwo * (yz + wx);
    out->at.x = kTwo * (zx + wy);
    out->at.y = kTwo * (yz - wx);
    out->at.z = -(kTwo * (xx + yy) - kOne);
    out->flags = 3;
}

/* TODO: [near miss] 97.91667%; trig slots, product tree and stores agree; seventeen FP register rows remain. */
void YXZ_angles_to_quat(const Vec* angles, Quat* out) {
    float sy;
    float cy;
    float sx;
    float cx;
    float sz;
    float cz;
    MKMATRIX m;
    float cycz;
    float cysz;
    float sysz;
    float czsy;

    gxMathCosSin(&cx, &sx, angles->x);
    gxMathCosSin(&cy, &sy, angles->y);
    gxMathCosSin(&cz, &sz, angles->z);

    cycz = cy * cz;
    cysz = cy * sz;
    czsy = cz * sy;
    sysz = sy * sz;
    m.right.x = sx * sysz + cycz;
    m.right.y = cx * sz;
    m.right.z = cysz * sx - czsy;
    m.up.x = sx * czsy - cysz;
    m.up.y = cx * cz;
    m.up.z = sx * cycz + sysz;
    m.at.x = sy * cx;
    m.at.y = -sx;
    m.at.z = cy * cx;
    m.flags = 3;
    RtQuatConvertFromMatrix(out, &m);
}

void mat_to_quat(Quat* out, const MKMATRIX* m) {
    RtQuatConvertFromMatrix(out, m);
}

/* TODO: [near miss] 99.69%; two symmetric fcmpu operand pairs remain; stop at comparison ordering. */
void XYZ_angles_to_MKMATRIX(const Vec* angles, MKMATRIX* m) {
    RwV3d saved;
    RwV3d neg;
    float y_angle;
    float z_angle;
    saved = m->pos;
    neg.x = kNegOne * saved.x;
    neg.y = kNegOne * saved.y;
    neg.z = kNegOne * saved.z;
    RwMatrixTranslate(m, &neg, 2);
    RwMatrixRotate(m, (const RwV3d*)&Xaxis, kRadToDeg * angles->x, 0);
    y_angle = angles->y;
    if (y_angle != 0.0f) {
        RwMatrixRotate(m, (const RwV3d*)&Yaxis, kRadToDeg * y_angle, 1);
    }
    z_angle = angles->z;
    if (z_angle != 0.0f) {
        RwMatrixRotate(m, (const RwV3d*)&Zaxis, kRadToDeg * z_angle, 1);
    }
    RwMatrixTranslate(m, &saved, 2);
}

void ZYX_angles_to_MKMATRIX(const Vec* angles, MKMATRIX* m) {
    RwV3d saved;
    RwV3d neg;
    float angle_y;
    float angle_x;
    saved = m->pos;
    neg.x = kNegOne * saved.x;
    neg.y = kNegOne * saved.y;
    neg.z = kNegOne * saved.z;
    RwMatrixTranslate(m, &neg, 2);
    RwMatrixRotate(m, (const RwV3d*)&Zaxis, kRadToDeg * angles->z, 0);
    angle_y = angles->y;
    if (angle_y) {
        RwMatrixRotate(m, (const RwV3d*)&Yaxis, kRadToDeg * angle_y, 1);
    }
    angle_x = angles->x;
    if (angle_x) {
        RwMatrixRotate(m, (const RwV3d*)&Xaxis, kRadToDeg * angle_x, 1);
    }
    RwMatrixTranslate(m, &saved, 2);
}

/* TODO: [near miss] 98.828125%; product tree and trig slots agree; eleven FP register rows remain; stop at coloring. */
void YXZ_angles_to_MKMATRIX(const Vec* angles, MKMATRIX* m) {
    float cz;
    float sz;
    float cx;
    float sx;
    float cy;
    float sy;
    float cycz;
    float cysz;
    float sysz;
    float czsy;

    gxMathCosSin(&cx, &sx, angles->x);
    gxMathCosSin(&cy, &sy, angles->y);
    gxMathCosSin(&cz, &sz, angles->z);

    cycz = cy * cz;
    cysz = cy * sz;
    sysz = sy * sz;
    czsy = cz * sy;
    m->right.x = sx * sysz + cycz;
    m->right.y = cx * sz;
    m->right.z = cysz * sx - czsy;
    m->up.x = sx * czsy - cysz;
    m->up.y = cx * cz;
    m->up.z = sx * cycz + sysz;
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
