#ifndef DOLPHIN_GX_INTERNAL_H
#define DOLPHIN_GX_INTERNAL_H

#include "dolphin/gx.h"
#include "dolphin/gx_fifo.h"

#define GX_MT_XF_FLUSH 1
#define GX_MT_DL_SAVE_CONTEXT 2
#define GX_MT_ABORT_WAIT_COPYOUT 3
#define GX_MT_NULL 0
#define GX_TLUT_256 16
#define GX_TLUT_1K 64
#define GX_TO_ZERO 0
#define GX_PNMTX0 0
#define GX_IDENTITY 60
#define GX_PTIDENTITY 125
#define GX_CULL_BACK 2
#define GX_CLIP_ENABLE 0
#define GX_DF_NONE 0
#define GX_AF_NONE 2
#define GX_LIGHT_NULL 0
#define GX_TEV_KCSEL_1_4 6
#define GX_TEV_KASEL_1 0
#define GX_ITS_1 0
#define GX_FOG_NONE 0
#define GX_CLAMP_TOP 1
#define GX_CLAMP_BOTTOM 2
#define GX_COPY_PROGRESSIVE 0
#define GX_READ_FF 1
#define GX_DISABLE 0
#define GX_ENABLE 1
#define GX_SRC_REG 0
#define GX_SRC_VTX 1
#define GX_CLAMP 0
#define GX_TEVSTAGE1 1
#define GX_TEVSTAGE2 2
#define GX_TEVSTAGE3 3
#define GX_TEVSTAGE4 4
#define GX_TEVSTAGE5 5
#define GX_TEVSTAGE6 6
#define GX_TEVSTAGE7 7
#define GX_TEVSTAGE8 8
#define GX_TEVSTAGE9 9
#define GX_TEVSTAGE10 10
#define GX_TEVSTAGE11 11
#define GX_TEVSTAGE12 12
#define GX_TEVSTAGE13 13
#define GX_TEVSTAGE14 14
#define GX_TEVSTAGE15 15
#define GX_REPLACE 3
#define GX_ALWAYS 7
#define GX_AOP_AND 0
#define GX_ZT_DISABLE 0
#define GX_MAX_TEVSTAGE 16
#define GX_TEV_SWAP0 0
#define GX_TEV_SWAP1 1
#define GX_TEV_SWAP2 2
#define GX_TEV_SWAP3 3
#define GX_CH_RED 0
#define GX_CH_GREEN 1
#define GX_CH_BLUE 2
#define GX_CH_ALPHA 3
#define GX_BM_NONE 0
#define GX_BL_ZERO 0
#define GX_BL_ONE 1
#define GX_BL_SRCALPHA 4
#define GX_BL_INVSRCALPHA 5
#define GX_LO_CLEAR 0
#define GX_LO_SET 15
#define GX_LEQUAL 3
#define GX_PF_RGB8_Z24 0
#define GX_ZC_LINEAR 0
#define GX_GM_1_0 0

typedef signed char s8;
typedef unsigned char u8;
typedef signed short s16;
typedef unsigned short u16;
typedef signed long s32;
typedef unsigned long u32;
typedef float f32;
typedef int BOOL;

#ifndef FALSE
#define FALSE 0
#define TRUE 1
#endif

#define GX_WRITE_U8(value) (*(volatile u8*)GXFIFO_ADDR = (u8)(value))
#define GX_WRITE_U16(value) (*(volatile u16*)GXFIFO_ADDR = (u16)(value))
#define GX_WRITE_U32(value) (*(volatile u32*)GXFIFO_ADDR = (u32)(value))
#define GX_WRITE_F32(value) (*(volatile f32*)GXFIFO_ADDR = (f32)(value))

#define GX_WRITE_RAS_REG(value) \
    do { \
        GX_WRITE_U8(0x61); \
        GX_WRITE_U32(value); \
    } while (0)

#define SET_REG_FIELD(line, reg, size, shift, value) \
    do { \
        (reg) = (u32)__rlwimi((u32)(reg), (value), (shift), \
                              32 - (shift) - (size), 31 - (shift)); \
    } while (0)

#define CHECK_GXBEGIN(line, name) ((void)0)
#define ASSERTMSGLINE(line, condition, message) ((void)0)
#define ASSERTMSGLINEV(line, condition, format, value) ((void)0)
#define ASSERTMSG1LINE(line, condition, format, value) ((void)0)
#define ASSERTLINE(line, condition) ((void)0)
#define GET_REG_FIELD(reg, size, shift) \
    (((reg) >> (shift)) & ((1 << (size)) - 1))

typedef int GXTevStageID;
typedef int GXTevMode;
typedef int GXTevColorArg;
typedef int GXTevAlphaArg;
typedef int GXTevOp;
typedef int GXTevBias;
typedef int GXTevScale;
#define GX_FALSE ((GXBool)0)
#define GX_TRUE ((GXBool)1)
typedef int GXTevRegID;
typedef int GXTevKColorID;
typedef unsigned int GXLightID;
typedef int GXChannelID;
typedef int GXColorSrc;
typedef int GXDiffuseFn;
typedef int GXAttnFn;

#define GX_COLOR0 0
#define GX_COLOR1 1
#define GX_ALPHA0 2
#define GX_ALPHA1 3
#define GX_COLOR0A0 4
#define GX_COLOR1A1 5
typedef int GXTevKColorSel;
typedef int GXTevKAlphaSel;
typedef int GXTevSwapSel;
typedef int GXTevColorChan;
typedef int GXCompare;
typedef int GXAlphaOp;
typedef int GXZTexOp;
typedef int GXFogType;
typedef int GXBlendMode;
typedef int GXBlendFactor;
typedef int GXLogicOp;
typedef int GXPixelFmt;
typedef int GXZFmt16;
typedef int GXIndTexStageID;
typedef int GXIndTexFormat;
typedef int GXIndTexBiasSel;
typedef int GXIndTexMtxID;
typedef int GXIndTexWrap;
typedef int GXIndTexAlphaSel;
typedef int GXMiscToken;
typedef int GXAlphaReadMode;
typedef int GXIndTexScale;
typedef int GXPrimitive;
typedef int GXTexOffset;
typedef int GXCullMode;
typedef int GXCopyMode;
typedef int GXFBClamp;
typedef int GXGamma;
typedef int GXProjectionType;
typedef int GXTexMtxType;
typedef int GXClipMode;
#define GX_PERSPECTIVE 0
#define GX_ORTHOGRAPHIC 1
#define GX_MTX3x4 0
#define GX_MTX2x4 1
#define GX_PTTEXMTX0 64
#define GX_NEAR 0
#define GX_LINEAR 1
#define GX_NEAR_MIP_NEAR 2
#define GX_LIN_MIP_NEAR 3
#define GX_NEAR_MIP_LIN 4
#define GX_LIN_MIP_LIN 5
#define GX_TEXCACHE_32K 0
#define GX_TEXCACHE_128K 1
#define GX_TEXCACHE_512K 2
#define GX_TEXCACHE_NONE 3

typedef enum GXPerf0 {
    GX_PERF0_VERTICES,
    GX_PERF0_CLIP_VTX,
    GX_PERF0_CLIP_CLKS,
    GX_PERF0_XF_WAIT_IN,
    GX_PERF0_XF_WAIT_OUT,
    GX_PERF0_XF_XFRM_CLKS,
    GX_PERF0_XF_LIT_CLKS,
    GX_PERF0_XF_BOT_CLKS,
    GX_PERF0_XF_REGLD_CLKS,
    GX_PERF0_XF_REGRD_CLKS,
    GX_PERF0_CLIP_RATIO,
    GX_PERF0_TRIANGLES,
    GX_PERF0_TRIANGLES_CULLED,
    GX_PERF0_TRIANGLES_PASSED,
    GX_PERF0_TRIANGLES_SCISSORED,
    GX_PERF0_TRIANGLES_0TEX,
    GX_PERF0_TRIANGLES_1TEX,
    GX_PERF0_TRIANGLES_2TEX,
    GX_PERF0_TRIANGLES_3TEX,
    GX_PERF0_TRIANGLES_4TEX,
    GX_PERF0_TRIANGLES_5TEX,
    GX_PERF0_TRIANGLES_6TEX,
    GX_PERF0_TRIANGLES_7TEX,
    GX_PERF0_TRIANGLES_8TEX,
    GX_PERF0_TRIANGLES_0CLR,
    GX_PERF0_TRIANGLES_1CLR,
    GX_PERF0_TRIANGLES_2CLR,
    GX_PERF0_QUAD_0CVG,
    GX_PERF0_QUAD_NON0CVG,
    GX_PERF0_QUAD_1CVG,
    GX_PERF0_QUAD_2CVG,
    GX_PERF0_QUAD_3CVG,
    GX_PERF0_QUAD_4CVG,
    GX_PERF0_AVG_QUAD_CNT,
    GX_PERF0_CLOCKS,
    GX_PERF0_NONE
} GXPerf0;

typedef enum GXPerf1 {
    GX_PERF1_TEXELS,
    GX_PERF1_TX_IDLE,
    GX_PERF1_TX_REGS,
    GX_PERF1_TX_MEMSTALL,
    GX_PERF1_TC_CHECK1_2,
    GX_PERF1_TC_CHECK3_4,
    GX_PERF1_TC_CHECK5_6,
    GX_PERF1_TC_CHECK7_8,
    GX_PERF1_TC_MISS,
    GX_PERF1_VC_ELEMQ_FULL,
    GX_PERF1_VC_MISSQ_FULL,
    GX_PERF1_VC_MEMREQ_FULL,
    GX_PERF1_VC_STATUS7,
    GX_PERF1_VC_MISSREP_FULL,
    GX_PERF1_VC_STREAMBUF_LOW,
    GX_PERF1_VC_ALL_STALLS,
    GX_PERF1_VERTICES,
    GX_PERF1_FIFO_REQ,
    GX_PERF1_CALL_REQ,
    GX_PERF1_VC_MISS_REQ,
    GX_PERF1_CP_ALL_REQ,
    GX_PERF1_CLOCKS,
    GX_PERF1_NONE
} GXPerf1;

#define GX_TEVSTAGE0 0
#define GX_PASSCLR 4
#define GX_TEX_DISABLE 0x100
#define GX_MAX_TEXMAP 8
#define GX_MAX_TEXCOORD 8
#define GX_TEXCOORD0 0
#define GX_COLOR_NULL 0xFF
#define GX_BM_BLEND 1
#define GX_BM_LOGIC 2
#define GX_BM_SUBTRACT 3
#define GX_PF_RGB8_Z24 0
#define GX_PF_RGB565_Z16 2
#define GX_PF_YUV420 7
#define GX_LOAD_BP_REG 0x61
#define GX_INDTEXSTAGE0 0
#define GX_INDTEXSTAGE1 1
#define GX_INDTEXSTAGE2 2
#define GX_INDTEXSTAGE3 3
#define GX_ITF_8 0
#define GX_ITB_NONE 0
#define GX_ITM_OFF 0
#define GX_ITW_OFF 0
#define GX_ITBA_OFF 0
#define GX_CLAMP_TOP 1
#define GX_CLAMP_BOTTOM 2
#define _GX_TF_ZTF 0x10

typedef struct __GXFifoObj {
    u8* base;
    u8* top;
    u32 size;
    u32 hiWatermark;
    u32 loWatermark;
    void* rdPtr;
    void* wrPtr;
    s32 count;
    u8 bind_cpu;
    u8 bind_gp;
} __GXFifoObj;

struct GXTexRegion {
    u32 data[4];
};

struct GXTlutRegion {
    u32 data[4];
};

typedef struct GXData {
    u16 vNumNot;
    u16 bpSentNot;
    u16 vNum;
    u16 vLim;
    u32 cpEnable;
    u32 cpStatus;
    u32 cpClr;
    u32 vcdLo;
    u32 vcdHi;
    u32 vatA[8];
    u32 vatB[8];
    u32 vatC[8];
    u32 lpSize;
    u32 matIdxA;
    u32 matIdxB;
    u32 indexBase[4];
    u32 indexStride[4];
    u32 ambColor[2];
    u32 matColor[2];
    u32 suTs0[8];
    u32 suTs1[8];
    u32 suScis0;
    u32 suScis1;
    u32 tref[8];
    u32 iref;
    u32 bpMask;
    u32 IndTexScale0;
    u32 IndTexScale1;
    u32 tevc[16];
    u32 teva[16];
    u32 tevKsel[8];
    u32 cmode0;
    u32 cmode1;
    u32 zmode;
    u32 peCtrl;
    u32 cpDispSrc;
    u32 cpDispSize;
    u32 cpDispStride;
    u32 cpDisp;
    u32 cpTexSrc;
    u32 cpTexSize;
    u32 cpTexStride;
    u32 cpTex;
    u8 cpTexZ;
    u32 genMode;
    GXTexRegion TexRegions0[8];
    GXTexRegion TexRegions1[8];
    GXTexRegion TexRegions2[8];
    GXTlutRegion TlutRegions[20];
    GXTexRegion* (*texRegionCallback)(GXTexObj*, int);
    GXTlutRegion* (*tlutRegionCallback)(u32);
    int nrmType;
    u8 hasNrms;
    u8 hasBiNrms;
    u32 projType;
    f32 projMtx[6];
    f32 vpLeft;
    f32 vpTop;
    f32 vpWd;
    f32 vpHt;
    f32 vpNearz;
    f32 vpFarz;
    f32 zOffset;
    f32 zScale;
    u32 tImage0[8];
    u32 tMode0[8];
    u32 texmapId[16];
    u32 tcsManEnab;
    u32 tevTcEnab;
    GXPerf0 perf0;
    GXPerf1 perf1;
    u32 perfSel;
    u8 inDispList;
    u8 dlSaveContext;
    u8 abtWaitPECopy;
    u8 dirtyVAT;
    u32 dirtyState;
} GXData;

typedef char GXData_size_must_be_0x5B0[(sizeof(GXData) == 0x5B0) ? 1 : -1];
typedef char GXFifo_size_must_be_0x24[(sizeof(__GXFifoObj) == 0x24) ? 1 : -1];

extern GXData* const __GXData;
extern void* __piReg;
extern void* __cpReg;
extern void* __memReg;
extern void* __peReg;
#define __PIRegs ((volatile u32*)__piReg)

#define GX_GET_PI_REG(offset) (*(volatile u32*)((volatile u32*)__piReg + (offset)))
#define GX_GET_CP_REG(offset) (*(volatile u16*)((volatile u16*)__cpReg + (offset)))
#define GX_GET_MEM_REG(offset) (*(volatile u16*)((volatile u16*)__memReg + (offset)))
#define GX_GET_PE_REG(offset) (*(volatile u16*)((volatile u16*)__peReg + (offset)))
#define GX_SET_PE_REG(offset, value) \
    (*(volatile u16*)((volatile u16*)__peReg + (offset)) = (u16)(value))
#define GX_SET_CP_REG(offset, value) \
    (*(volatile u16*)((volatile u16*)__cpReg + (offset)) = (u16)(value))

static inline u32 __GXReadMEMCounterU32(u32 low, u32 high) {
    u32 high0 = GX_GET_MEM_REG(high);
    u32 high1;
    u32 lowValue;

    do {
        high1 = high0;
        lowValue = GX_GET_MEM_REG(low);
        high0 = GX_GET_MEM_REG(high);
    } while (high0 != high1);

    return (high0 << 16) | lowValue;
}

#define GX_WRITE_XF_REG(address, value) \
    do { \
        GX_WRITE_U8(0x10); \
        GX_WRITE_U32(0x1000 + (address)); \
        GX_WRITE_U32(value); \
    } while (0)

#define GX_WRITE_XF_REG_F(address, value) \
    do { \
        GX_WRITE_F32(value); \
    } while (0)

#define GX_WRITE_SOME_REG4(command, address, value, index) \
    do { \
        GX_WRITE_U8(command); \
        GX_WRITE_U8(address); \
        GX_WRITE_U32(value); \
    } while (0)

#define GX_WRITE_SOME_REG2(command, address, value, index) \
    GX_WRITE_SOME_REG4(command, address, value, index)
#define GX_WRITE_SOME_REG3(command, address, value, index) \
    GX_WRITE_SOME_REG4(command, address, value, index)

void __GXSetDirtyState(void);
void __GXFlushTextureState(void);
void __GXSetSUTexRegs(void);
void __GXUpdateBPMask(void);
void __GXSetGenMode(void);
void __GXSetVCD(void);
void __GXSetVAT(void);
void __GXSetMatrixIndex(GXAttr attr);
void __GXCalculateVLim(void);
void __GetImageTileCount(GXTexFmt format, u16 width, u16 height,
                         u32* rowTiles, u32* columnTiles, u32* tileCount);
void __GXSendFlushPrim(void);
void __GXSaveCPUFifoAux(__GXFifoObj* fifo);
void __GXInitGX(void);
void GXSaveCPUFifo(GXFifoObj* fifo);
void GXSetCPUFifo(GXFifoObj* fifo);

#endif
