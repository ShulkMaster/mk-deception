#include "rw/rphanim.h"
#include "rw/rwstream.h"

/* Criterion SDK fdlibm expansions from rwplcore.h. */
#define RW_ACOS_POLY(v, result)                                                \
    do {                                                                       \
        p = (v) *                                                              \
            ((v) *                                                             \
                 ((v) * ((v) * ((v) * (0.00003479331f * (v) + 0.000791535f) -  \
                                0.040055536f) +                                \
                         0.20121253f) -                                        \
                  0.32556581f) +                                               \
             0.16666667f);                                                     \
        q = (v) *                                                              \
                ((v) * ((v) * (0.077038154f * (v) - 0.688284f) + 2.0209458f) - \
                 2.403395f) +                                                  \
            1.0f;                                                              \
        (result) = p / q;                                                      \
    } while (0)

#define RW_ACOS(result, input)                                                 \
    do {                                                                       \
        RwSplitBits value, truncated;                                          \
        RwReal x = (input);                                                    \
        RwReal z, p, q, r, s, w, c, df;                                        \
        RwInt32 hx, ix;                                                        \
        value.nReal = x;                                                       \
        hx = value.nInt;                                                       \
        ix = hx & 0x7fffffff;                                                  \
        if (ix >= 0x3f800000) {                                                \
            (result) = hx > 0 ? 0.0f : 3.1415927f;                             \
        } else if (ix < 0x3f000000) {                                          \
            if (ix <= 0x23000000)                                              \
                (result) = 1.5707964f;                                         \
            else {                                                             \
                z = x * x;                                                     \
                RW_ACOS_POLY(z, r);                                            \
                (result) = 1.5707963f - (x - (7.5497894e-8f - x * r));         \
            }                                                                  \
        } else if (hx < 0) {                                                   \
            z = 0.5f * (1.0f + x);                                             \
            RW_ACOS_POLY(z, r);                                                \
            s = _rwSqrt(z);                                                    \
            w = r * s - 7.5497894e-8f;                                         \
            (result) = 3.1415925f - 2.0f * (s + w);                            \
        } else {                                                               \
            z = 0.5f * (1.0f - x);                                             \
            s = _rwSqrt(z);                                                    \
            truncated.nReal = s;                                               \
            truncated.nInt &= 0xfffff000;                                      \
            df = truncated.nReal;                                              \
            c = (z - df * df) / (s + df);                                      \
            RW_ACOS_POLY(z, r);                                                \
            w = r * s + c;                                                     \
            (result) = 2.0f * (df + w);                                        \
        }                                                                      \
    } while (0)

#define RW_SIN(result, x)                                                      \
    do {                                                                       \
        const RwReal z = (x) * (x);                                            \
        const RwReal v = z * (x);                                              \
        const RwReal r = 0.008333334f +                                        \
                         z * (-0.0001984127f +                                 \
                              z * (0.0000027557314f +                          \
                                   z * (-2.505076e-8f + z * 1.589691e-10f)));  \
        (result) = (x) + v * (-0.16666667f + z * r);                           \
    } while (0)

#define HANIM_SLERP(out, a, b, alpha)                                          \
    do {                                                                       \
        RwReal cosTheta =                                                      \
            (a)->q.imag.x * (b)->q.imag.x + (a)->q.imag.y * (b)->q.imag.y +    \
            (a)->q.imag.z * (b)->q.imag.z + (a)->q.real * (b)->q.real;         \
        RwReal beta = 1.0f - (alpha);                                          \
        if (cosTheta < 0.0f) {                                                 \
            cosTheta = -cosTheta;                                              \
            (b)->q.imag.x = -(b)->q.imag.x;                                    \
            (b)->q.imag.y = -(b)->q.imag.y;                                    \
            (b)->q.imag.z = -(b)->q.imag.z;                                    \
            (b)->q.real = -(b)->q.real;                                        \
        }                                                                      \
        if (cosTheta <= 0.999f) {                                              \
            RwReal theta, reciprocal, sinBeta, sinAlpha;                       \
            RW_ACOS(theta, cosTheta);                                          \
            RW_SIN(reciprocal, theta);                                         \
            reciprocal = 1.0f / reciprocal;                                    \
            RW_SIN(sinBeta, beta * theta);                                     \
            beta = sinBeta * reciprocal;                                       \
            RW_SIN(sinAlpha, (alpha) * theta);                                 \
            (alpha) = sinAlpha * reciprocal;                                   \
        }                                                                      \
        (out)->q.imag.x = beta * (a)->q.imag.x + (alpha) * (b)->q.imag.x;      \
        (out)->q.imag.y = beta * (a)->q.imag.y + (alpha) * (b)->q.imag.y;      \
        (out)->q.imag.z = beta * (a)->q.imag.z + (alpha) * (b)->q.imag.z;      \
        (out)->q.real = beta * (a)->q.real + (alpha) * (b)->q.real;            \
    } while (0)

void RpHAnimKeyFrameApply(void *matrix, void *voidFrame) {
    RwMatrix *m = matrix;
    RpHAnimKeyFrame *frame = voidFrame;
    const RwReal x = frame->q.imag.x;
    const RwReal y = frame->q.imag.y;
    const RwReal z = frame->q.imag.z;
    const RwReal w = frame->q.real;
    RwV3d square;
    RwV3d cross;
    RwV3d wimag;

    square.x = x * x;
    square.y = y * y;
    square.z = z * z;
    cross.x = y * z;
    cross.y = z * x;
    cross.z = x * y;
    wimag.x = w * x;
    wimag.y = w * y;
    wimag.z = w * z;
    m->right.x = 1 - 2 * (square.y + square.z);
    m->right.y = 2 * (cross.z + wimag.z);
    m->right.z = 2 * (cross.y - wimag.y);
    m->up.x = 2 * (cross.z - wimag.z);
    m->up.y = 1 - 2 * (square.x + square.z);
    m->up.z = 2 * (cross.x + wimag.x);
    m->at.x = 2 * (cross.y + wimag.y);
    m->at.y = 2 * (cross.x - wimag.x);
    m->at.z = 1 - 2 * (square.x + square.y);
    m->pos.x = 0.0f;
    m->pos.y = 0.0f;
    m->pos.z = 0.0f;
    m->flags = 3;
    m->pos = frame->t;
}

void RpHAnimKeyFrameInterpolate(void *vout, void *va, void *vb, RwReal time,
                                void *customData) {
    RpHAnimKeyFrame *out = vout;
    RpHAnimKeyFrame *a = va;
    RpHAnimKeyFrame *b = vb;
    RwReal alpha = (time - a->time) / (b->time - a->time);
    out->t.x = a->t.x + alpha * (b->t.x - a->t.x);
    out->t.y = a->t.y + alpha * (b->t.y - a->t.y);
    out->t.z = a->t.z + alpha * (b->t.z - a->t.z);
    HANIM_SLERP(out, a, b, alpha);
}

void RpHAnimKeyFrameBlend(void *vout, void *va, void *vb, RwReal alpha) {
    RpHAnimKeyFrame *out = vout;
    RpHAnimKeyFrame *a = va;
    RpHAnimKeyFrame *b = vb;
    out->t.x = a->t.x + alpha * (b->t.x - a->t.x);
    out->t.y = a->t.y + alpha * (b->t.y - a->t.y);
    out->t.z = a->t.z + alpha * (b->t.z - a->t.z);
    HANIM_SLERP(out, a, b, alpha);
}

RtAnimAnimation *RpHAnimKeyFrameStreamRead(RwStream *stream,
                                           RtAnimAnimation *animation) {
    RpHAnimKeyFrame *frames = animation->pFrames;
    RwUInt32 frameSize = sizeof(RpHAnimKeyFrame);
    RwInt32 i;
    for (i = 0; i < animation->numFrames; i++) {
        RwUInt32 offset;
        if (!RwStreamReadReal(stream, &frames[i].time, 0x20))
            return NULL;
        if (!RwStreamReadInt32(stream, (RwInt32 *)&offset, 4))
            return NULL;
        frames[i].prevFrame =
            (RpHAnimKeyFrame *)((RwUInt8 *)frames +
                                (offset / frameSize) * frameSize);
    }
    return animation;
}
RwBool RpHAnimKeyFrameStreamWrite(RtAnimAnimation *animation,
                                  RwStream *stream) {
    const RpHAnimKeyFrame *frames = animation->pFrames;
    RwInt32 i;
    for (i = 0; i < animation->numFrames; i++) {
        RwInt32 offset;
        if (RwStreamWriteReal(stream, &frames[i].time, 0x20) == NULL)
            return FALSE;
        offset = (RwUInt8 *)frames[i].prevFrame - (RwUInt8 *)frames;
        if (!RwStreamWriteInt32(stream, &offset, 4))
            return FALSE;
    }
    return TRUE;
}
RwInt32 RpHAnimKeyFrameStreamGetSize(RtAnimAnimation *animation) {
    RwInt32 size = sizeof(RpHAnimKeyFrame);
    size *= animation->numFrames;
    return size;
}

#define QUAT_MULTIPLY(out, a, b)                                               \
    do {                                                                       \
        (out)->real = (a)->real * (b)->real - (a)->imag.x * (b)->imag.x -      \
                      (a)->imag.y * (b)->imag.y - (a)->imag.z * (b)->imag.z;   \
        (out)->imag.x = (a)->imag.y * (b)->imag.z - (a)->imag.z * (b)->imag.y; \
        (out)->imag.y = (a)->imag.z * (b)->imag.x - (a)->imag.x * (b)->imag.z; \
        (out)->imag.z = (a)->imag.x * (b)->imag.y - (a)->imag.y * (b)->imag.x; \
        (out)->imag.x += (b)->imag.x * (a)->real;                              \
        (out)->imag.y += (b)->imag.y * (a)->real;                              \
        (out)->imag.z += (b)->imag.z * (a)->real;                              \
        (out)->imag.x += (a)->imag.x * (b)->real;                              \
        (out)->imag.y += (a)->imag.y * (b)->real;                              \
        (out)->imag.z += (a)->imag.z * (b)->real;                              \
    } while (0)
void RpHAnimKeyFrameMulRecip(void *vf, void *vs) {
    RpHAnimKeyFrame *f = vf;
    RpHAnimKeyFrame *s = vs;
    RtQuat q = f->q, inv;
    RwReal n = s->q.real * s->q.real + s->q.imag.x * s->q.imag.x +
               s->q.imag.y * s->q.imag.y + s->q.imag.z * s->q.imag.z;
    if (n > 0) {
        n = 1.0f / n;
        inv.real = s->q.real * n;
        inv.imag.x = -s->q.imag.x * n;
        inv.imag.y = -s->q.imag.y * n;
        inv.imag.z = -s->q.imag.z * n;
    }
    QUAT_MULTIPLY(&f->q, &inv, &q);
    f->t.x -= s->t.x;
    f->t.y -= s->t.y;
    f->t.z -= s->t.z;
}
void RpHAnimKeyFrameAdd(void *vo, void *va, void *vb) {
    RpHAnimKeyFrame *o = vo;
    RpHAnimKeyFrame *a = va;
    RpHAnimKeyFrame *b = vb;
    QUAT_MULTIPLY(&o->q, &a->q, &b->q);
    o->t.x = a->t.x + b->t.x;
    o->t.y = a->t.y + b->t.y;
    o->t.z = a->t.z + b->t.z;
}
