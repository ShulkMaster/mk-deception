#ifndef DOLPHIN_GX_H
#define DOLPHIN_GX_H

#include "dolphin/mtx.h"
#include "dolphin/gx_fifo.h"

typedef struct GXColor {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
} GXColor;

typedef struct GXColorS10 {
    short r;
    short g;
    short b;
    short a;
} GXColorS10;

typedef unsigned char GXBool;

typedef enum GXSpotFn {
    GX_SP_OFF = 0, GX_SP_FLAT, GX_SP_COS, GX_SP_COS2,
    GX_SP_SHARP, GX_SP_RING1, GX_SP_RING2
} GXSpotFn;

typedef enum GXDistAttnFn {
    GX_DA_OFF = 0, GX_DA_GENTLE, GX_DA_MEDIUM, GX_DA_STEEP
} GXDistAttnFn;

#ifdef GX_ATTR_INTERNAL_TYPES
typedef enum GXVtxFmt { GX_VTXFMT0, GX_VTXFMT1, GX_VTXFMT2, GX_VTXFMT3,
    GX_VTXFMT4, GX_VTXFMT5, GX_VTXFMT6, GX_VTXFMT7, GX_MAX_VTXFMT } GXVtxFmt;
typedef enum GXAttr { GX_VA_PNMTXIDX, GX_VA_TEX0MTXIDX, GX_VA_TEX1MTXIDX,
    GX_VA_TEX2MTXIDX, GX_VA_TEX3MTXIDX, GX_VA_TEX4MTXIDX, GX_VA_TEX5MTXIDX,
    GX_VA_TEX6MTXIDX, GX_VA_TEX7MTXIDX, GX_VA_POS, GX_VA_NRM, GX_VA_CLR0,
    GX_VA_CLR1, GX_VA_TEX0, GX_VA_TEX1, GX_VA_TEX2, GX_VA_TEX3, GX_VA_TEX4,
    GX_VA_TEX5, GX_VA_TEX6, GX_VA_TEX7, GX_POS_MTX_ARRAY, GX_NRM_MTX_ARRAY,
    GX_TEX_MTX_ARRAY, GX_LIGHT_ARRAY, GX_VA_NBT, GX_VA_MAX_ATTR,
    GX_VA_NULL=0xFF } GXAttr;
typedef enum GXAttrType { GX_NONE, GX_DIRECT, GX_INDEX8, GX_INDEX16 } GXAttrType;
typedef enum GXCompCnt { GX_POS_XY=0, GX_POS_XYZ=1, GX_NRM_XYZ=0,
    GX_NRM_NBT=1, GX_NRM_NBT3=2, GX_CLR_RGB=0, GX_CLR_RGBA=1,
    GX_TEX_S=0, GX_TEX_ST=1 } GXCompCnt;
typedef enum GXCompType { GX_U8=0, GX_S8=1, GX_U16=2, GX_S16=3, GX_F32=4,
    GX_RGB565=0, GX_RGB8=1, GX_RGBX8=2, GX_RGBA4=3, GX_RGBA6=4,
    GX_RGBA8=5 } GXCompType;
typedef enum GXTexCoordID { GX_TEXCOORD0, GX_TEXCOORD1, GX_TEXCOORD2,
    GX_TEXCOORD3, GX_TEXCOORD4, GX_TEXCOORD5, GX_TEXCOORD6, GX_TEXCOORD7,
    GX_MAX_TEXCOORD, GX_TEXCOORD_NULL=0xFF } GXTexCoordID;
typedef enum GXTexGenType { GX_TG_MTX3x4, GX_TG_MTX2x4, GX_TG_BUMP0,
    GX_TG_BUMP1, GX_TG_BUMP2, GX_TG_BUMP3, GX_TG_BUMP4, GX_TG_BUMP5,
    GX_TG_BUMP6, GX_TG_BUMP7, GX_TG_SRTG } GXTexGenType;
typedef enum GXTexGenSrc { GX_TG_POS, GX_TG_NRM, GX_TG_BINRM, GX_TG_TANGENT,
    GX_TG_TEX0, GX_TG_TEX1, GX_TG_TEX2, GX_TG_TEX3, GX_TG_TEX4, GX_TG_TEX5,
    GX_TG_TEX6, GX_TG_TEX7, GX_TG_TEXCOORD0, GX_TG_TEXCOORD1,
    GX_TG_TEXCOORD2, GX_TG_TEXCOORD3, GX_TG_TEXCOORD4, GX_TG_TEXCOORD5,
    GX_TG_TEXCOORD6, GX_TG_COLOR0, GX_TG_COLOR1 } GXTexGenSrc;
#else
typedef int GXVtxFmt;
enum { GX_VTXFMT0, GX_VTXFMT1, GX_VTXFMT2, GX_VTXFMT3,
    GX_VTXFMT4, GX_VTXFMT5, GX_VTXFMT6, GX_VTXFMT7, GX_MAX_VTXFMT };
typedef int GXAttr;
enum { GX_VA_PNMTXIDX, GX_VA_TEX0MTXIDX, GX_VA_TEX1MTXIDX,
    GX_VA_TEX2MTXIDX, GX_VA_TEX3MTXIDX, GX_VA_TEX4MTXIDX, GX_VA_TEX5MTXIDX,
    GX_VA_TEX6MTXIDX, GX_VA_TEX7MTXIDX, GX_VA_POS, GX_VA_NRM, GX_VA_CLR0,
    GX_VA_CLR1, GX_VA_TEX0, GX_VA_TEX1, GX_VA_TEX2, GX_VA_TEX3, GX_VA_TEX4,
    GX_VA_TEX5, GX_VA_TEX6, GX_VA_TEX7, GX_POS_MTX_ARRAY, GX_NRM_MTX_ARRAY,
    GX_TEX_MTX_ARRAY, GX_LIGHT_ARRAY, GX_VA_NBT, GX_VA_MAX_ATTR,
    GX_VA_NULL = 0xFF };
typedef int GXAttrType;
enum { GX_NONE, GX_DIRECT, GX_INDEX8, GX_INDEX16 };
typedef int GXCompCnt;
enum { GX_POS_XY=0, GX_POS_XYZ=1, GX_NRM_XYZ=0,
    GX_NRM_NBT=1, GX_NRM_NBT3=2, GX_CLR_RGB=0, GX_CLR_RGBA=1,
    GX_TEX_S=0, GX_TEX_ST=1 };
typedef int GXCompType;
enum { GX_U8=0, GX_S8=1, GX_U16=2, GX_S16=3, GX_F32=4,
    GX_RGB565=0, GX_RGB8=1, GX_RGBX8=2, GX_RGBA4=3, GX_RGBA6=4,
    GX_RGBA8=5 };
typedef int GXTexCoordID;
enum { GX_TEXCOORD0, GX_TEXCOORD1, GX_TEXCOORD2,
    GX_TEXCOORD3, GX_TEXCOORD4, GX_TEXCOORD5, GX_TEXCOORD6, GX_TEXCOORD7,
    GX_MAX_TEXCOORD, GX_TEXCOORD_NULL=0xFF };
typedef int GXTexGenType;
enum { GX_TG_MTX3x4, GX_TG_MTX2x4, GX_TG_BUMP0,
    GX_TG_BUMP1, GX_TG_BUMP2, GX_TG_BUMP3, GX_TG_BUMP4, GX_TG_BUMP5,
    GX_TG_BUMP6, GX_TG_BUMP7, GX_TG_SRTG };
typedef int GXTexGenSrc;
enum { GX_TG_POS, GX_TG_NRM, GX_TG_BINRM, GX_TG_TANGENT,
    GX_TG_TEX0, GX_TG_TEX1, GX_TG_TEX2, GX_TG_TEX3, GX_TG_TEX4, GX_TG_TEX5,
    GX_TG_TEX6, GX_TG_TEX7, GX_TG_TEXCOORD0, GX_TG_TEXCOORD1,
    GX_TG_TEXCOORD2, GX_TG_TEXCOORD3, GX_TG_TEXCOORD4, GX_TG_TEXCOORD5,
    GX_TG_TEXCOORD6, GX_TG_COLOR0, GX_TG_COLOR1 };
#endif
typedef struct GXVtxAttrFmtList {
    GXAttr attr; GXCompCnt cnt; GXCompType type; unsigned char frac;
} GXVtxAttrFmtList;

typedef struct GXFogAdjTable {
    unsigned short r[10];
} GXFogAdjTable;

typedef struct GXTexObj {
    unsigned char data[0x20];
} GXTexObj;

typedef struct GXTlutObj {
    unsigned char data[0xC];
} GXTlutObj;

typedef struct GXTexRegion GXTexRegion;
typedef struct GXTlutRegion GXTlutRegion;

typedef int GXTexFmt;
enum {
    GX_TF_I4 = 0, GX_TF_I8 = 1, GX_TF_IA4 = 2, GX_TF_IA8 = 3,
    GX_TF_RGB565 = 4, GX_TF_RGB5A3 = 5, GX_TF_RGBA8 = 6,
    GX_TF_C4 = 8, GX_TF_C8 = 9, GX_TF_C14X2 = 10, GX_TF_CMPR = 14,
    GX_TF_Z8 = 0x11, GX_TF_Z16 = 0x13, GX_TF_Z24X8 = 0x16,
    GX_CTF_R4 = 0x20, GX_CTF_RA4 = 0x22, GX_CTF_RA8 = 0x23,
    GX_CTF_YUVA8 = 0x26, GX_CTF_A8 = 0x27, GX_CTF_R8 = 0x28,
    GX_CTF_G8 = 0x29, GX_CTF_B8 = 0x2A, GX_CTF_RG8 = 0x2B,
    GX_CTF_GB8 = 0x2C, GX_CTF_Z4 = 0x30, GX_CTF_Z8M = 0x39,
    GX_CTF_Z8L = 0x3A, GX_CTF_Z16L = 0x3C,
    GX_TF_A8 = GX_CTF_A8
};
typedef int GXCITexFmt;
typedef int GXTexWrapMode;
typedef int GXTexFilter;
typedef int GXAnisotropy;
typedef int GXTexMapID;
enum { GX_TEXMAP0, GX_TEXMAP1, GX_TEXMAP2, GX_TEXMAP3,
       GX_TEXMAP4, GX_TEXMAP5, GX_TEXMAP6, GX_TEXMAP7,
       GX_MAX_TEXMAP, GX_TEXMAP_NULL = 0xFF };
typedef int GXTlutFmt;
enum { GX_TL_IA8 = 0, GX_TL_RGB565 = 1, GX_TL_RGB5A3 = 2 };
typedef int GXTexCacheSize;
typedef int GXTlutSize;
typedef GXTexRegion* (*GXTexRegionCallback)(GXTexObj*, GXTexMapID);
typedef GXTlutRegion* (*GXTlutRegionCallback)(unsigned long);

typedef struct GXLightObj {
    unsigned char data[0x40];
} GXLightObj;

typedef struct GXRenderModeObj {
    int viTVmode;
    unsigned short fbWidth;
    unsigned short efbHeight;
    unsigned short xfbHeight;
    unsigned short viXOrigin;
    unsigned short viYOrigin;
    unsigned short viWidth;
    unsigned short viHeight;
    unsigned long xFBmode;
    unsigned char field_rendering;
    unsigned char aa;
    unsigned char sample_pattern[12][2];
    unsigned char vfilter[7];
} GXRenderModeObj;

#ifdef __cplusplus
extern "C" {
#endif

void GXSetNumTexGens(unsigned char count);
void GXSetDrawSync(unsigned short token);
unsigned short GXReadDrawSync(void);
void GXSetTexCoordGen2(GXTexCoordID destination, GXTexGenType function,
                       GXTexGenSrc source, unsigned long matrix,
                       GXBool normalize, unsigned long post_matrix);
static inline void GXSetTexCoordGen(GXTexCoordID destination,
                                    GXTexGenType function,
                                    GXTexGenSrc source,
                                    unsigned long matrix)
{
    GXSetTexCoordGen2(destination, function, source, matrix, 0, 125);
}
void GXSetNumTevStages(unsigned char count);
void GXSetTevOrder(int stage, int coordinate, int map, int color);
void GXSetTevColorIn(int stage, int a, int b, int c, int d);
void GXSetTevColorOp(int stage, int op, int bias, int scale, GXBool clamp, int output);
void GXSetTevAlphaIn(int stage, int a, int b, int c, int d);
void GXSetTevAlphaOp(int stage, int op, int bias, int scale, GXBool clamp, int output);
void GXSetTevKColorSel(int stage, int selection);
void GXSetTevKAlphaSel(int stage, int selection);
void GXSetTevSwapMode(int stage, int raster_selection, int texture_selection);
void GXSetTevColorS10(int id, GXColorS10 color);
void GXSetTevColor(int id, GXColor color);
void GXSetTevKColor(int id, GXColor color);
void GXSetTevSwapModeTable(int id, int r, int g, int b, int a);
void GXSetNumChans(unsigned char count);
void GXInitLightAttn(GXLightObj* object, float a0, float a1, float a2,
                     float k0, float k1, float k2);
void GXInitLightAttnA(GXLightObj* object, float a0, float a1, float a2);
void GXInitLightDistAttn(GXLightObj* object, float reference_distance,
                         float reference_brightness, GXDistAttnFn function);
void GXInitLightPos(GXLightObj* object, float x, float y, float z);
void GXInitLightDir(GXLightObj* object, float x, float y, float z);
void GXInitLightSpot(GXLightObj* object, float cutoff, GXSpotFn function);
void GXInitLightColor(GXLightObj* object, GXColor color);
void GXLoadLightObjImm(const GXLightObj* object, unsigned int id);
void GXSetChanCtrl(int channel, GXBool enable, int ambient_source,
                   int material_source, unsigned int light_mask,
                   int diffuse_function, int attenuation_function);
void GXSetChanMatColor(int channel, GXColor color);
void GXSetChanAmbColor(int channel, GXColor color);
void GXSetNumIndStages(unsigned char count);
void GXSetProjection(const Mtx44 matrix, int type);
void GXGetProjectionv(float* projection);
void GXGetViewportv(float* viewport);
void GXSetProjectionv(const float* projection);
void GXLoadTexObjPreLoaded(GXTexObj* object, GXTexRegion* region, GXTexMapID map_id);
void GXLoadTexObj(GXTexObj* object, GXTexMapID map_id);
void GXSetBlendMode(int type, int source, int destination, int operation);
void GXSetCullMode(int mode);
void GXSetZMode(GXBool compare_enable, int function, GXBool update_enable);
void GXSetAlphaCompare(int compare0, unsigned char reference0, int operation,
                       int compare1, unsigned char reference1);
void GXLoadTexMtxImm(const float matrix[][4], unsigned long id, int type);
void GXClearVtxDesc(void);
void GXSetVtxDesc(GXAttr attribute, GXAttrType type);
void GXSetVtxAttrFmt(GXVtxFmt format, GXAttr attribute, GXCompCnt component_count,
                     GXCompType component_type, unsigned char fraction);
void GXSetVtxAttrFmtv(GXVtxFmt format, const GXVtxAttrFmtList* list);
void GXSetArray(GXAttr attribute, void* base, unsigned char stride);
void GXLoadPosMtxImm(const Mtx matrix, unsigned long id);
void GXLoadNrmMtxImm(const Mtx matrix, unsigned long id);
void GXSetCurrentMtx(unsigned long id);
void GXBegin(int primitive, int format, unsigned short vertex_count);
GXFifoObj* GXInit(void* fifo_base, unsigned long fifo_size);
void GXDrawDone(void);
void GXSetScissor(unsigned long left, unsigned long top,
                  unsigned long width, unsigned long height);
void GXSetDispCopySrc(unsigned short left, unsigned short top,
                      unsigned short width, unsigned short height);
float GXGetYScaleFactor(unsigned short efb_height, unsigned short xfb_height);
unsigned long GXSetDispCopyYScale(float scale);
void GXSetDispCopyDst(unsigned short width, unsigned short height);
void GXSetCopyFilter(GXBool antialias,
                     const unsigned char sample_pattern[12][2],
                     GXBool vertical_filter_enable,
                     const unsigned char vertical_filter[7]);
void GXSetPixelFmt(int pixel_format, int z_format);
void GXSetFieldMask(GXBool odd_mask, GXBool even_mask);
void GXSetFieldMode(GXBool field_mode, GXBool half_aspect_ratio);
void GXSetTevOp(int stage, int mode);
void GXInitTlutObj(GXTlutObj* object, void* image, int format,
                   unsigned short entry_count);
void GXLoadTlut(GXTlutObj* object, unsigned long id);
void GXInitTexObjCI(GXTexObj* object, void* image, unsigned short width,
                    unsigned short height, int format, int wrap_s, int wrap_t,
                    GXBool mipmap, unsigned long tlut);
void GXSetCoPlanar(GXBool enable);
void GXSetClipMode(int mode);
void GXSetScissorBoxOffset(long x, long y);
void GXSetFog(int type, float start, float end, float near_plane,
              float far_plane, GXColor color);
void GXSetFogRangeAdj(GXBool enable, unsigned short center,
                      const GXFogAdjTable* table);
void GXSetColorUpdate(GXBool enable);
void GXSetAlphaUpdate(GXBool enable);
void GXSetZCompLoc(GXBool before_texture);
void GXSetDither(GXBool enable);
void GXSetDstAlpha(GXBool enable, unsigned char alpha);
void GXSetCopyClear(GXColor color, unsigned long z);
void GXSetViewport(float left, float top, float width, float height,
                   float near_plane, float far_plane);
void GXSetViewportJitter(float left, float top, float width, float height,
                         float near_plane, float far_plane,
                         unsigned long field);
void GXCopyDisp(void* destination, GXBool clear);
void GXSetTexCopySrc(unsigned short left, unsigned short top,
                     unsigned short width, unsigned short height);
void GXSetTexCopyDst(unsigned short width, unsigned short height, int format,
                     GXBool mipmap);
void GXCopyTex(void* destination, GXBool clear);
void GXCallDisplayList(void* list, unsigned long byte_count);
void GXBeginDisplayList(void* list, unsigned long byte_count);
unsigned long GXEndDisplayList(void);
void GXResetWriteGatherPipe(void);
void GXFlush(void);
void GXInvalidateTexAll(void);
void GXInvalidateTexRegion(GXTexRegion* region);
void GXInitTexObj(GXTexObj* object, void* image, unsigned short width,
                  unsigned short height, int format, int wrap_s, int wrap_t,
                  GXBool mipmap);
void GXInitTexObjLOD(GXTexObj* object, int min_filter, int mag_filter,
                     float min_lod, float max_lod, float lod_bias,
                     unsigned char bias_clamp, unsigned char edge_lod,
                     int max_anisotropy);
unsigned long GXGetTexBufferSize(unsigned short width, unsigned short height,
                                 unsigned long format, GXBool mipmap,
                                 unsigned char max_lod);
GXTexFmt GXGetTexObjFmt(const GXTexObj* object);
GXBool GXGetTexObjMipMap(const GXTexObj* object);
float GXGetTexObjLODBias(const GXTexObj* object);
GXBool GXGetTexObjBiasClamp(const GXTexObj* object);
GXBool GXGetTexObjEdgeLOD(const GXTexObj* object);
GXAnisotropy GXGetTexObjMaxAniso(const GXTexObj* object);
unsigned long GXGetTexObjTlut(const GXTexObj* object);
void GXInitTexCacheRegion(GXTexRegion* region, GXBool is_32b_mipmap,
                          unsigned long tmem_even, GXTexCacheSize size_even,
                          unsigned long tmem_odd, GXTexCacheSize size_odd);
void GXInitTlutRegion(GXTlutRegion* region, unsigned long tmem_addr,
                      GXTlutSize tlut_size);
GXTexRegionCallback GXSetTexRegionCallback(GXTexRegionCallback callback);
GXTlutRegionCallback GXSetTlutRegionCallback(GXTlutRegionCallback callback);
void GXPixModeSync(void);
void GXAbortFrame(void);
void GXInvalidateVtxCache(void);
void GXSetCopyClamp(int clamp);
void GXSetDispCopyGamma(int gamma);

#ifdef __cplusplus
}
#endif

#endif
