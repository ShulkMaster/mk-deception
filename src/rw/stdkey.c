#include "rw/rphanim.h"
#include "rw/rwstream.h"

#define rpHAnimAcosNumerator(z)                                             \
    ((z) * (0.16666667f + (z) * (-0.32556581f + (z) *                     \
        (0.20121253f + (z) * (-0.040055536f + (z) *                       \
        (0.000791535f + (z) * 0.00003479331f))))))
#define rpHAnimAcosDenominator(z)                                           \
    (1.0f + (z) * (-2.403395f + (z) * (2.0209458f + (z) *                 \
        (-0.688284f + (z) * 0.077038154f))))

typedef union HAnimIEEEFloatShape {
    float value;
    unsigned int word;
} HAnimIEEEFloatShape;

/* Intentional IEEE-754 bit view used by the retail fdlibm-style acos. */
typedef char HAnimIEEEFloatShapeSizeCheck[
    sizeof(HAnimIEEEFloatShape) == 0x04 ? 1 : -1];

#define rpHAnimReadFloatWord(wordOut, realValue)                            \
    do {                                                                    \
        HAnimIEEEFloatShape shape;                                          \
        shape.value = (realValue);                                          \
        (wordOut) = shape.word;                                             \
    } while (0)

#define rpHAnimWriteFloatWord(realOut, wordValue)                           \
    do {                                                                    \
        HAnimIEEEFloatShape shape;                                          \
        shape.word = (unsigned int)(wordValue);                             \
        (realOut) = shape.value;                                            \
    } while (0)

#define rpHAnimApproximateSine(result, x)                                   \
    do {                                                                    \
        const float z = (x) * (x);                                          \
        const float v = z * (x);                                            \
        const float r = 0.008333334f + z * (-0.0001984127f + z *           \
            (0.0000027557314f + z * (-2.505076e-8f +                       \
            z * 1.589691e-10f)));                                           \
        (result) = (x) + v * (-0.16666667f + z * r);                        \
    } while (0)

#define rpHAnimApproximateAcos(result, x)                                   \
    do {                                                                    \
        float z, p, q, r, w, s, c, df;                                      \
        int hx, ix;                                                         \
        rpHAnimReadFloatWord(hx, (x));                                      \
        ix = hx & 0x7fffffff;                                                \
        if (ix >= 0x3f800000) {                                              \
            if (hx > 0) {                                                   \
                (result) = 0.0f;                                            \
            } else {                                                        \
                (result) = 3.1415925f + 2.0f * 7.5497894e-8f;              \
            }                                                               \
        } else if (ix < 0x3f000000) {                                       \
            if (ix <= 0x23000000) {                                         \
                (result) = 1.5707964f;                                      \
            } else {                                                        \
                z = (x) * (x);                                              \
                p = rpHAnimAcosNumerator(z);                                \
                q = rpHAnimAcosDenominator(z);                              \
                r = p / q;                                                  \
                (result) = 1.5707963f -                                     \
                    ((x) - (7.5497894e-8f - (x) * r));                      \
            }                                                               \
        } else if (hx < 0) {                                                \
            z = 0.5f * (1.0f + (x));                                        \
            p = rpHAnimAcosNumerator(z);                                    \
            q = rpHAnimAcosDenominator(z);                                  \
            s = _rwSqrt(z);                                                 \
            r = p / q;                                                      \
            w = r * s - 7.5497894e-8f;                                     \
            (result) = 3.1415925f - 2.0f * (s + w);                         \
        } else {                                                            \
            int idf;                                                        \
            z = 0.5f * (1.0f - (x));                                        \
            s = _rwSqrt(z);                                                 \
            df = s;                                                         \
            rpHAnimReadFloatWord(idf, df);                                  \
            rpHAnimWriteFloatWord(df, idf & 0xfffff000);                    \
            c = (z - df * df) / (s + df);                                   \
            p = rpHAnimAcosNumerator(z);                                    \
            q = rpHAnimAcosDenominator(z);                                  \
            r = p / q;                                                      \
            w = r * s + c;                                                  \
            (result) = 2.0f * (df + w);                                     \
        }                                                                   \
    } while (0)

void RpHAnimKeyFrameApply(void *matrix, void *voidFrame) {
    RwMatrix *m = matrix;
    RpHAnimKeyFrame *frame = voidFrame;
    const float x = frame->q.x;
    const float y = frame->q.y;
    const float z = frame->q.z;
    const float w = frame->q.w;
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

void RpHAnimKeyFrameInterpolate(void *vout, void *va, void *vb, float time,
                                void *customData) {
    RpHAnimKeyFrame *out = vout;
    RpHAnimKeyFrame *a = va;
    RpHAnimKeyFrame *b = vb;
    float cosTheta = a->q.w * b->q.w +
                     (a->q.z * b->q.z +
                      (a->q.x * b->q.x + a->q.y * b->q.y));
    float alpha = (time - a->time) / (b->time - a->time);
    float beta;
    int negate;
    int nearlyOne;

    out->t.x = a->t.x + alpha * (b->t.x - a->t.x);
    out->t.y = a->t.y + alpha * (b->t.y - a->t.y);
    out->t.z = a->t.z + alpha * (b->t.z - a->t.z);

    negate = cosTheta < 0.0f;
    if (negate) {
        cosTheta = -cosTheta;
        b->q.x = -b->q.x;
        b->q.y = -b->q.y;
        b->q.z = -b->q.z;
        b->q.w = -b->q.w;
    }
    beta = 1.0f - alpha;
    nearlyOne = cosTheta >= 0.999f;
    if (!nearlyOne) {
        float theta;
        float reciprocal;
        rpHAnimApproximateAcos(theta, cosTheta);
        {
            rpHAnimApproximateSine(reciprocal, theta);
            reciprocal = 1.0f / reciprocal;
            beta *= theta;
            rpHAnimApproximateSine(beta, beta);
            beta *= reciprocal;
            alpha *= theta;
            rpHAnimApproximateSine(alpha, alpha);
            alpha *= reciprocal;
        }
    }
    out->q.x = beta * a->q.x + alpha * b->q.x;
    out->q.y = beta * a->q.y + alpha * b->q.y;
    out->q.z = beta * a->q.z + alpha * b->q.z;
    out->q.w = beta * a->q.w + alpha * b->q.w;
}

void RpHAnimKeyFrameBlend(void *vout, void *va, void *vb, float alpha) {
    RpHAnimKeyFrame *out = vout;
    RpHAnimKeyFrame *a = va;
    RpHAnimKeyFrame *b = vb;
    float theta;
    float cosTheta = a->q.w * b->q.w +
                     (a->q.z * b->q.z +
                      (a->q.x * b->q.x + a->q.y * b->q.y));
    float beta;
    int negate;
    int nearlyOne;

    out->t.x = a->t.x + alpha * (b->t.x - a->t.x);
    out->t.y = a->t.y + alpha * (b->t.y - a->t.y);
    out->t.z = a->t.z + alpha * (b->t.z - a->t.z);

    negate = cosTheta < 0.0f;
    if (negate) {
        cosTheta = -cosTheta;
        b->q.x = -b->q.x;
        b->q.y = -b->q.y;
        b->q.z = -b->q.z;
        b->q.w = -b->q.w;
    }
    beta = 1.0f - alpha;
    nearlyOne = cosTheta >= 0.999f;
    if (!nearlyOne) {
        float reciprocal;
        rpHAnimApproximateAcos(theta, cosTheta);
        {
            rpHAnimApproximateSine(reciprocal, theta);
            reciprocal = 1.0f / reciprocal;
            beta *= theta;
            rpHAnimApproximateSine(beta, beta);
            beta *= reciprocal;
            alpha *= theta;
            rpHAnimApproximateSine(alpha, alpha);
            alpha *= reciprocal;
        }
    }
    out->q.x = beta * a->q.x + alpha * b->q.x;
    out->q.y = beta * a->q.y + alpha * b->q.y;
    out->q.z = beta * a->q.z + alpha * b->q.z;
    out->q.w = beta * a->q.w + alpha * b->q.w;
}

RtAnimAnimation *RpHAnimKeyFrameStreamRead(RwStream *stream,
                                           RtAnimAnimation *animation) {
    RpHAnimKeyFrame *frames = animation->pFrames;
    unsigned int frameSize = sizeof(RpHAnimKeyFrame);
    int i;
    for (i = 0; i < animation->numFrames; i++) {
        unsigned int offset;
        if (!RwStreamReadReal(stream, &frames[i].time, 0x20))
            return 0;
        if (!RwStreamReadInt32(stream, (int *)&offset, 4))
            return 0;
        frames[i].prevFrame =
            (RpHAnimKeyFrame *)((unsigned char *)frames +
                                (offset / frameSize) * frameSize);
    }
    return animation;
}
int RpHAnimKeyFrameStreamWrite(RtAnimAnimation *animation,
                                  RwStream *stream) {
    const RpHAnimKeyFrame *frames = animation->pFrames;
    int i;
    for (i = 0; i < animation->numFrames; i++) {
        int offset;
        if (RwStreamWriteReal(stream, &frames[i].time, 0x20) == 0)
            return 0;
        offset = (unsigned char *)frames[i].prevFrame - (unsigned char *)frames;
        if (!RwStreamWriteInt32(stream, &offset, 4))
            return 0;
    }
    return 1;
}
int RpHAnimKeyFrameStreamGetSize(RtAnimAnimation *animation) {
    int size = sizeof(RpHAnimKeyFrame);
    size *= animation->numFrames;
    return size;
}

void RpHAnimKeyFrameMulRecip(void *vf, void *vs) {
    RpHAnimKeyFrame *f = vf;
    RpHAnimKeyFrame *s = vs;
    Quat q = f->q, inv;
    float n = s->q.w * s->q.w +
              (s->q.z * s->q.z +
               (s->q.x * s->q.x + s->q.y * s->q.y));
    if (n > 0) {
        n = 1.0f / n;
        inv.w = s->q.w * n;
        inv.x = -s->q.x * n;
        inv.y = -s->q.y * n;
        inv.z = -s->q.z * n;
    }
    f->q.w = inv.w * q.w -
             (inv.z * q.z + (inv.x * q.x + inv.y * q.y));
    f->q.x = inv.y * q.z - inv.z * q.y;
    f->q.y = inv.z * q.x - inv.x * q.z;
    f->q.z = inv.x * q.y - inv.y * q.x;
    f->q.x += q.x * inv.w;
    f->q.y += q.y * inv.w;
    f->q.z += q.z * inv.w;
    f->q.x += inv.x * q.w;
    f->q.y += inv.y * q.w;
    f->q.z += inv.z * q.w;
    f->t.x -= s->t.x;
    f->t.y -= s->t.y;
    f->t.z -= s->t.z;
}
void RpHAnimKeyFrameAdd(void *vo, void *va, void *vb) {
    RpHAnimKeyFrame *o = vo;
    RpHAnimKeyFrame *a = va;
    RpHAnimKeyFrame *b = vb;
    o->q.w = a->q.w * b->q.w -
             (a->q.z * b->q.z +
              (a->q.x * b->q.x + a->q.y * b->q.y));
    o->q.x = a->q.y * b->q.z - a->q.z * b->q.y;
    o->q.y = a->q.z * b->q.x - a->q.x * b->q.z;
    o->q.z = a->q.x * b->q.y - a->q.y * b->q.x;
    o->q.x += b->q.x * a->q.w;
    o->q.y += b->q.y * a->q.w;
    o->q.z += b->q.z * a->q.w;
    o->q.x += a->q.x * b->q.w;
    o->q.y += a->q.y * b->q.w;
    o->q.z += a->q.z * b->q.w;
    o->t.x = a->t.x + b->t.x;
    o->t.y = a->t.y + b->t.y;
    o->t.z = a->t.z + b->t.z;
}
