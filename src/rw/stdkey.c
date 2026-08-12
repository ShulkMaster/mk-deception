#include "rw/rphanim.h"
#include "rw/rwstream.h"

static float HAnimAcosNumerator(float z)
{
    return z * (z * (z * (z * (z * (0.00003479331f * z + 0.000791535f) -
        0.040055536f) + 0.20121253f) - 0.32556581f) + 0.16666667f);
}

static float HAnimAcosDenominator(float z)
{
    return z * (z * (z * (0.077038154f * z - 0.688284f) + 2.0209458f) -
        2.403395f) + 1.0f;
}

static float HAnimSinApprox(float x)
{
    float square = x * x;
    return x + square * x * (-0.16666667f + square *
        (0.008333334f + square * (-0.0001984127f + square *
        (0.0000027557314f + square * (-2.505076e-8f +
        square * 1.589691e-10f)))));
}

void RpHAnimKeyFrameApply(void *matrix, void *voidFrame) {
    RwMatrix *m = matrix;
    RpHAnimKeyFrame *frame = voidFrame;
    const float x = frame->q.imag.x;
    const float y = frame->q.imag.y;
    const float z = frame->q.imag.z;
    const float w = frame->q.real;
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
    float alpha = (time - a->time) / (b->time - a->time);
    out->t.x = a->t.x + alpha * (b->t.x - a->t.x);
    out->t.y = a->t.y + alpha * (b->t.y - a->t.y);
    out->t.z = a->t.z + alpha * (b->t.z - a->t.z);
    {
        float cosTheta = a->q.imag.x * b->q.imag.x +
                          a->q.imag.y * b->q.imag.y +
                          a->q.imag.z * b->q.imag.z +
                          a->q.real * b->q.real;
        float beta = 1.0f - alpha;

        if (cosTheta < 0.0f) {
            cosTheta = -cosTheta;
            b->q.imag.x = -b->q.imag.x;
            b->q.imag.y = -b->q.imag.y;
            b->q.imag.z = -b->q.imag.z;
            b->q.real = -b->q.real;
        }
        if (cosTheta <= 0.999f) {
            float theta;
            float z;

            if (cosTheta < 0.5f) {
                RwSplitBits bits;
                bits.nReal = cosTheta;
                if ((bits.nInt & 0x7fffffff) <= 0x23000000) {
                    theta = 1.5707964f;
                } else {
                    float ratio;
                    z = cosTheta * cosTheta;
                    ratio = HAnimAcosNumerator(z) /
                            HAnimAcosDenominator(z);
                    theta = 1.5707963f -
                            (cosTheta - (7.5497894e-8f - cosTheta * ratio));
                }
            } else {
                RwSplitBits truncated;
                float root;
                float high;
                float correction;
                float ratio;
                z = 0.5f * (1.0f - cosTheta);
                root = _rwSqrt(z);
                truncated.nReal = root;
                truncated.nInt &= 0xfffff000;
                high = truncated.nReal;
                correction = (z - high * high) / (root + high);
                ratio = HAnimAcosNumerator(z) /
                        HAnimAcosDenominator(z);
                theta = 2.0f * (high + ratio * root + correction);
            }
            {
                float reciprocal = 1.0f / HAnimSinApprox(theta);
                beta = HAnimSinApprox(beta * theta) * reciprocal;
                alpha = HAnimSinApprox(alpha * theta) * reciprocal;
            }
        }
        out->q.imag.x = beta * a->q.imag.x + alpha * b->q.imag.x;
        out->q.imag.y = beta * a->q.imag.y + alpha * b->q.imag.y;
        out->q.imag.z = beta * a->q.imag.z + alpha * b->q.imag.z;
        out->q.real = beta * a->q.real + alpha * b->q.real;
    }
}

void RpHAnimKeyFrameBlend(void *vout, void *va, void *vb, float alpha) {
    RpHAnimKeyFrame *out = vout;
    RpHAnimKeyFrame *a = va;
    RpHAnimKeyFrame *b = vb;
    out->t.x = a->t.x + alpha * (b->t.x - a->t.x);
    out->t.y = a->t.y + alpha * (b->t.y - a->t.y);
    out->t.z = a->t.z + alpha * (b->t.z - a->t.z);
    {
        float cosTheta = a->q.imag.x * b->q.imag.x +
                          a->q.imag.y * b->q.imag.y +
                          a->q.imag.z * b->q.imag.z +
                          a->q.real * b->q.real;
        float beta = 1.0f - alpha;

        if (cosTheta < 0.0f) {
            cosTheta = -cosTheta;
            b->q.imag.x = -b->q.imag.x;
            b->q.imag.y = -b->q.imag.y;
            b->q.imag.z = -b->q.imag.z;
            b->q.real = -b->q.real;
        }
        if (cosTheta <= 0.999f) {
            float theta;
            float z;

            if (cosTheta < 0.5f) {
                RwSplitBits bits;
                bits.nReal = cosTheta;
                if ((bits.nInt & 0x7fffffff) <= 0x23000000) {
                    theta = 1.5707964f;
                } else {
                    float ratio;
                    z = cosTheta * cosTheta;
                    ratio = HAnimAcosNumerator(z) /
                            HAnimAcosDenominator(z);
                    theta = 1.5707963f -
                            (cosTheta - (7.5497894e-8f - cosTheta * ratio));
                }
            } else {
                RwSplitBits truncated;
                float root;
                float high;
                float correction;
                float ratio;
                z = 0.5f * (1.0f - cosTheta);
                root = _rwSqrt(z);
                truncated.nReal = root;
                truncated.nInt &= 0xfffff000;
                high = truncated.nReal;
                correction = (z - high * high) / (root + high);
                ratio = HAnimAcosNumerator(z) /
                        HAnimAcosDenominator(z);
                theta = 2.0f * (high + ratio * root + correction);
            }
            {
                float reciprocal = 1.0f / HAnimSinApprox(theta);
                beta = HAnimSinApprox(beta * theta) * reciprocal;
                alpha = HAnimSinApprox(alpha * theta) * reciprocal;
            }
        }
        out->q.imag.x = beta * a->q.imag.x + alpha * b->q.imag.x;
        out->q.imag.y = beta * a->q.imag.y + alpha * b->q.imag.y;
        out->q.imag.z = beta * a->q.imag.z + alpha * b->q.imag.z;
        out->q.real = beta * a->q.real + alpha * b->q.real;
    }
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
    RtQuat q = f->q, inv;
    float n = s->q.real * s->q.real + s->q.imag.x * s->q.imag.x +
               s->q.imag.y * s->q.imag.y + s->q.imag.z * s->q.imag.z;
    if (n > 0) {
        n = 1.0f / n;
        inv.real = s->q.real * n;
        inv.imag.x = -s->q.imag.x * n;
        inv.imag.y = -s->q.imag.y * n;
        inv.imag.z = -s->q.imag.z * n;
    }
    f->q.real = inv.real * q.real - inv.imag.x * q.imag.x -
                inv.imag.y * q.imag.y - inv.imag.z * q.imag.z;
    f->q.imag.x = inv.imag.y * q.imag.z - inv.imag.z * q.imag.y +
                  q.imag.x * inv.real + inv.imag.x * q.real;
    f->q.imag.y = inv.imag.z * q.imag.x - inv.imag.x * q.imag.z +
                  q.imag.y * inv.real + inv.imag.y * q.real;
    f->q.imag.z = inv.imag.x * q.imag.y - inv.imag.y * q.imag.x +
                  q.imag.z * inv.real + inv.imag.z * q.real;
    f->t.x -= s->t.x;
    f->t.y -= s->t.y;
    f->t.z -= s->t.z;
}
void RpHAnimKeyFrameAdd(void *vo, void *va, void *vb) {
    RpHAnimKeyFrame *o = vo;
    RpHAnimKeyFrame *a = va;
    RpHAnimKeyFrame *b = vb;
    o->q.real = a->q.real * b->q.real - a->q.imag.x * b->q.imag.x -
                a->q.imag.y * b->q.imag.y - a->q.imag.z * b->q.imag.z;
    o->q.imag.x = a->q.imag.y * b->q.imag.z -
                  a->q.imag.z * b->q.imag.y +
                  b->q.imag.x * a->q.real + a->q.imag.x * b->q.real;
    o->q.imag.y = a->q.imag.z * b->q.imag.x -
                  a->q.imag.x * b->q.imag.z +
                  b->q.imag.y * a->q.real + a->q.imag.y * b->q.real;
    o->q.imag.z = a->q.imag.x * b->q.imag.y -
                  a->q.imag.y * b->q.imag.x +
                  b->q.imag.z * a->q.real + a->q.imag.z * b->q.real;
    o->t.x = a->t.x + b->t.x;
    o->t.y = a->t.y + b->t.y;
    o->t.z = a->t.z + b->t.z;
}
