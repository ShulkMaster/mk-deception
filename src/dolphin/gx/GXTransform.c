#include <dolphin/gx.h>
#include "__gx.h"
#include "runtime/asm_sequences.inc"

static const f32 GXProjectionZero = 0.0f;
static const f32 GXProjectionOne = 1.0f;

#pragma push
asm void GXSetProjection(const Mtx44 mtx, GXProjectionType type) {
    SEQ_GXSetProjection();
}

asm void GXSetProjectionv(const f32* ptr) {
    SEQ_GXSetProjectionv();
}

asm void GXGetProjectionv(f32* ptr) {
    SEQ_GXGetProjectionv();
}

asm void GXLoadPosMtxImm(const Mtx mtx, u32 id) {
    SEQ_GXLoadPosMtxImm();
}

asm void GXLoadNrmMtxImm(const Mtx mtx, u32 id) {
    SEQ_GXLoadNrmMtxImm();
}
#pragma pop

void GXSetCurrentMtx(u32 id) {
    CHECK_GXBEGIN(708, "GXSetCurrentMtx");
    SET_REG_FIELD(712, __GXData->matIdxA, 6, 0, id);
    __GXSetMatrixIndex(GX_VA_PNMTXIDX);
}

#pragma push
asm void GXLoadTexMtxImm(const f32 mtx[][4], u32 id, GXTexMtxType type) {
    SEQ_GXLoadTexMtxImm();
}
#pragma pop

void __GXSetViewport(void) {
    f32 sx;
    f32 sy;
    f32 sz;
    f32 ox;
    f32 oy;
    f32 oz;
    f32 zmin;
    f32 zmax;
    u32 reg;

    sx = __GXData->vpWd / 2.0f;
    sy = -__GXData->vpHt / 2.0f;
    ox = 342.0f + (__GXData->vpLeft + (__GXData->vpWd / 2.0f));
    oy = 342.0f + (__GXData->vpTop + (__GXData->vpHt / 2.0f));

    zmin = __GXData->vpNearz * __GXData->zScale;
    zmax = __GXData->vpFarz * __GXData->zScale;

    sz = zmax - zmin;
    oz = zmax + __GXData->zOffset;

    reg = 0x5101A;
    GX_WRITE_U8(0x10);
    GX_WRITE_U32(reg);
    GX_WRITE_XF_REG_F(26, sx);
    GX_WRITE_XF_REG_F(27, sy);
    GX_WRITE_XF_REG_F(28, sz);
    GX_WRITE_XF_REG_F(29, ox);
    GX_WRITE_XF_REG_F(30, oy);
    GX_WRITE_XF_REG_F(31, oz);
}

void GXSetViewportJitter(f32 left, f32 top, f32 wd, f32 ht, f32 nearz, f32 farz, u32 field) {
    CHECK_GXBEGIN(903, "GXSetViewport");  // not the correct function name

    if (field == 0) {
        top -= 0.5f;
    }

    __GXData->vpLeft = left;
    __GXData->vpTop = top;
    __GXData->vpWd = wd;
    __GXData->vpHt = ht;
    __GXData->vpNearz = nearz;
    __GXData->vpFarz = farz;

    __GXSetViewport();
    __GXData->bpSentNot = 1;
}

void GXSetViewport(f32 left, f32 top, f32 wd, f32 ht, f32 nearz, f32 farz) {
    GXSetViewportJitter(left, top, wd, ht, nearz, farz, 1);
}

#pragma push
asm void GXGetViewportv(f32* vp) {
    SEQ_GXGetViewportv();
}
#pragma pop

void GXSetScissor(u32 left, u32 top, u32 wd, u32 ht) {
    u32 tp;
    u32 lf;
    u32 bm;
    u32 rt;

    CHECK_GXBEGIN(1048, "GXSetScissor");
    ASSERTMSGLINE(1049, left < 1706, "GXSetScissor: Left origin > 1708");
    ASSERTMSGLINE(1050, top < 1706, "GXSetScissor: top origin > 1708");
    ASSERTMSGLINE(1051, left + wd < 1706, "GXSetScissor: right edge > 1708");
    ASSERTMSGLINE(1052, top + ht < 1706, "GXSetScissor: bottom edge > 1708");

    tp = top + 342;
    lf = left + 342;
    bm = tp + ht - 1;
    rt = lf + wd - 1;

    SET_REG_FIELD(1059, __GXData->suScis0, 11, 0, tp);
    SET_REG_FIELD(1060, __GXData->suScis0, 11, 12, lf);
    SET_REG_FIELD(1062, __GXData->suScis1, 11, 0, bm);
    SET_REG_FIELD(1063, __GXData->suScis1, 11, 12, rt);

    GX_WRITE_RAS_REG(__GXData->suScis0);
    GX_WRITE_RAS_REG(__GXData->suScis1);
    __GXData->bpSentNot = 0;
}

void GXSetScissorBoxOffset(s32 x_off, s32 y_off) {
    u32 reg = 0;
    u32 hx;
    u32 hy;

    CHECK_GXBEGIN(1119, "GXSetScissorBoxOffset");

    ASSERTMSGLINE(1122, (u32)(x_off + 342) < 2048, "GXSetScissorBoxOffset: Invalid X offset");
    ASSERTMSGLINE(1124, (u32)(y_off + 342) < 2048, "GXSetScissorBoxOffset: Invalid Y offset");

    hx = (u32)(x_off + 342) >> 1;
    hy = (u32)(y_off + 342) >> 1;

    SET_REG_FIELD(1129, reg, 10, 0, hx);
    SET_REG_FIELD(1130, reg, 10, 10, hy);
    SET_REG_FIELD(1131, reg, 8, 24, 0x59);
    GX_WRITE_RAS_REG(reg);
    __GXData->bpSentNot = 0;
}

void GXSetClipMode(GXClipMode mode) {
    CHECK_GXBEGIN(1151, "GXSetClipMode");
    GX_WRITE_XF_REG(5, mode);
    __GXData->bpSentNot = 1;
}

void __GXSetMatrixIndex(GXAttr matIdxAttr) {
    if (matIdxAttr < GX_VA_TEX4MTXIDX) {
        GX_WRITE_SOME_REG4(8, 0x30, __GXData->matIdxA, -12);
        GX_WRITE_XF_REG(24, __GXData->matIdxA);
    } else {
        GX_WRITE_SOME_REG4(8, 0x40, __GXData->matIdxB, -12);
        GX_WRITE_XF_REG(25, __GXData->matIdxB);
    }
    __GXData->bpSentNot = 1;
}
