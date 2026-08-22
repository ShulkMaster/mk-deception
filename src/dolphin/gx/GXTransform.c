#include <dolphin/gx.h>
#include "__gx.h"

/*
 * Soft ceiling: retail uses SDK psq_l/psq_st helpers here. MWCC 1.2.5n
 * exposes no callable paired-single intrinsic, so keep the typed scalar form;
 * the authentic donor's register-qualified inline assembly is prohibited.
 */
static inline void Copy6Floats(const f32* source, f32* destination) {
    destination[0] = source[0]; destination[1] = source[1];
    destination[2] = source[2]; destination[3] = source[3];
    destination[4] = source[4]; destination[5] = source[5];
}

static inline void __GXSetProjection(void) {
    GX_WRITE_U8(0x10);
    GX_WRITE_U32(0x00061020);
    GX_WRITE_F32(__GXData->projMtx[0]); GX_WRITE_F32(__GXData->projMtx[1]);
    GX_WRITE_F32(__GXData->projMtx[2]); GX_WRITE_F32(__GXData->projMtx[3]);
    GX_WRITE_F32(__GXData->projMtx[4]); GX_WRITE_F32(__GXData->projMtx[5]);
    GX_WRITE_U32(__GXData->projType);
}

static inline void WriteMTX4x3(const f32 mtx[3][4]) {
    u32 row, column;
    for (row = 0; row < 3; row++)
        for (column = 0; column < 4; column++) GX_WRITE_F32(mtx[row][column]);
}
static inline void WriteMTX3x3from3x4(const f32 mtx[3][4]) {
    u32 row, column;
    for (row = 0; row < 3; row++)
        for (column = 0; column < 3; column++) GX_WRITE_F32(mtx[row][column]);
}
static inline void WriteMTX4x2(const f32 mtx[3][4]) {
    u32 row, column;
    for (row = 0; row < 2; row++)
        for (column = 0; column < 4; column++) GX_WRITE_F32(mtx[row][column]);
}

void GXSetProjection(const Mtx44 mtx, GXProjectionType type) {
    CHECK_GXBEGIN(295, "GXSetProjection");

    __GXData->projType = type;
    __GXData->projMtx[0] = mtx[0][0];
    __GXData->projMtx[2] = mtx[1][1];
    __GXData->projMtx[4] = mtx[2][2];
    __GXData->projMtx[5] = mtx[2][3];
    if (type == GX_ORTHOGRAPHIC) {
        __GXData->projMtx[1] = mtx[0][3];
        __GXData->projMtx[3] = mtx[1][3];
    } else {
        __GXData->projMtx[1] = mtx[0][2];
        __GXData->projMtx[3] = mtx[1][2];
    }

    __GXSetProjection();
    __GXData->bpSentNot = 1;
}

void GXSetProjectionv(const f32* ptr) {
    CHECK_GXBEGIN(339, "GXSetProjectionv");

    __GXData->projType = ptr[0] == 0.0f ? GX_PERSPECTIVE : GX_ORTHOGRAPHIC;

    Copy6Floats(&ptr[1], __GXData->projMtx);

    __GXSetProjection();
    __GXData->bpSentNot = 1;
}

void GXGetProjectionv(f32* ptr) {
    ASSERTMSGLINE(370, ptr, "GXGet*: invalid null pointer");

    ptr[0] = (u32)__GXData->projType != GX_PERSPECTIVE ? 1.0f : 0.0f;

    Copy6Floats(__GXData->projMtx, &ptr[1]);
}

void GXLoadPosMtxImm(const Mtx mtx, u32 id) {
    u32 reg;
    u32 addr;

    CHECK_GXBEGIN(507, "GXLoadPosMtxImm");

    addr = id * 4;
    reg = addr | 0xB0000;

    GX_WRITE_U8(0x10);
    GX_WRITE_U32(reg);
    WriteMTX4x3(mtx);
}


void GXLoadNrmMtxImm(const Mtx mtx, u32 id) {
    u32 reg;
    u32 addr;

    CHECK_GXBEGIN(588, "GXLoadNrmMtxImm");

    addr = id * 3 + 0x400;
    reg = addr | 0x80000;

    GX_WRITE_U8(0x10);
    GX_WRITE_U32(reg);
    WriteMTX3x3from3x4(mtx);
}


void GXSetCurrentMtx(u32 id) {
    CHECK_GXBEGIN(708, "GXSetCurrentMtx");
    SET_REG_FIELD(712, __GXData->matIdxA, 6, 0, id);
    __GXSetMatrixIndex(GX_VA_PNMTXIDX);
}

void GXLoadTexMtxImm(const f32 mtx[][4], u32 id, GXTexMtxType type) {
    u32 reg;
    u32 addr;
    u32 count;

    CHECK_GXBEGIN(741, "GXLoadTexMtxImm");

    if (id >= GX_PTTEXMTX0) {
        addr = (id - GX_PTTEXMTX0) * 4 + 0x500;
        ASSERTMSGLINE(751, type == GX_MTX3x4, "GXLoadTexMtx: Invalid matrix type");
    } else {
        addr = id * 4;
    }
    count = (type == GX_MTX2x4) ? 8 : 12;
    reg = addr | ((count - 1) << 16);

    GX_WRITE_U8(0x10);
    GX_WRITE_U32(reg);
    if (type == GX_MTX3x4) {
        WriteMTX4x3(mtx);
    } else {
        WriteMTX4x2(mtx);
    }
}


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

void GXGetViewportv(f32* vp) {
    ASSERTMSGLINE(968, vp, "GXGet*: invalid null pointer");

    Copy6Floats(&__GXData->vpLeft, vp);
}


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
    u32 scissor;

    CHECK_GXBEGIN(1119, "GXSetScissorBoxOffset");

    ASSERTMSGLINE(1122, (u32)(x_off + 342) < 2048, "GXSetScissorBoxOffset: Invalid X offset");
    ASSERTMSGLINE(1124, (u32)(y_off + 342) < 2048, "GXSetScissorBoxOffset: Invalid Y offset");

    SET_REG_FIELD(1129, reg, 10, 0, (u32)(x_off + 342) >> 1);
    scissor = reg;
    SET_REG_FIELD(1130, scissor, 10, 10, (u32)(y_off + 342) >> 1);
    SET_REG_FIELD(1131, scissor, 8, 24, 0x59);
    GX_WRITE_RAS_REG(scissor);
    __GXData->bpSentNot = 0;
}

void GXSetClipMode(GXClipMode mode) {
    CHECK_GXBEGIN(1151, "GXSetClipMode");
    GX_WRITE_XF_REG(5, mode);
    __GXData->bpSentNot = 1;
}

void __GXSetMatrixIndex(GXAttr matIdxAttr) {
    u32 value;
    if (matIdxAttr < GX_VA_TEX4MTXIDX) {
        value = __GXData->matIdxA;
        GX_WRITE_SOME_REG4(8, 0x30, value, -12);
        GX_WRITE_XF_REG(24, value);
    } else {
        value = __GXData->matIdxB;
        GX_WRITE_SOME_REG4(8, 0x40, value, -12);
        GX_WRITE_XF_REG(25, value);
    }
    __GXData->bpSentNot = 1;
}
