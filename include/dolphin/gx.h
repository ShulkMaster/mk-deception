#ifndef DOLPHIN_GX_H
#define DOLPHIN_GX_H

#include "dolphin/mtx.h"

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

typedef struct GXTexObj {
    unsigned char data[0x20];
} GXTexObj;

typedef struct GXTlutObj {
    unsigned char data[0xC];
} GXTlutObj;

typedef struct GXTexRegion GXTexRegion;

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
void GXSetTexCoordGen2(int destination, int function, int source, int matrix,
                       int normalize, int post_matrix);
void GXSetNumTevStages(unsigned char count);
void GXSetTevOrder(int stage, int coordinate, int map, int color);
void GXSetTevColorIn(int stage, int a, int b, int c, int d);
void GXSetTevColorOp(int stage, int op, int bias, int scale, int clamp, int output);
void GXSetTevAlphaIn(int stage, int a, int b, int c, int d);
void GXSetTevAlphaOp(int stage, int op, int bias, int scale, int clamp, int output);
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
                         float reference_brightness, int function);
void GXInitLightPos(GXLightObj* object, float x, float y, float z);
void GXInitLightDir(GXLightObj* object, float x, float y, float z);
void GXInitLightSpot(GXLightObj* object, float cutoff, int function);
void GXInitLightColor(GXLightObj* object, GXColor color);
void GXLoadLightObjImm(GXLightObj* object, unsigned int id);
void GXSetChanCtrl(int channel, int enable, int ambient_source,
                   int material_source, unsigned int light_mask,
                   int diffuse_function, int attenuation_function);
void GXSetChanMatColor(int channel, GXColor color);
void GXSetChanAmbColor(int channel, GXColor color);
void GXSetNumIndStages(unsigned char count);
void GXSetProjection(Mtx44 matrix, int type);
void GXGetProjectionv(float* projection);
void GXSetProjectionv(float* projection);
void GXLoadTexObj(GXTexObj* object, int map_id);
void GXSetBlendMode(int type, int source, int destination, int operation);
void GXSetCullMode(int mode);
void GXSetZMode(int compare, int enable, int update);
void GXSetAlphaCompare(int compare0, int reference0, int operation,
                       int compare1, int reference1);
void GXLoadTexMtxImm(Mtx matrix, int id, int type);
void GXClearVtxDesc(void);
void GXSetVtxDesc(int attribute, int type);
void GXSetVtxAttrFmt(int format, int attribute, int component_count,
                     int component_type, int fraction);
void GXLoadPosMtxImm(Mtx matrix, int id);
void GXBegin(int primitive, int format, int vertex_count);
void* GXInit(void* fifo_base, unsigned long fifo_size);
void GXDrawDone(void);
void GXSetScissor(int left, int top, int width, int height);
void GXSetDispCopySrc(int left, int top, int width, int height);
float GXGetYScaleFactor(unsigned short efb_height, unsigned short xfb_height);
unsigned short GXSetDispCopyYScale(float scale);
void GXSetDispCopyDst(unsigned short width, unsigned short height);
void GXSetCopyFilter(unsigned char antialias,
                     unsigned char sample_pattern[12][2],
                     unsigned char vertical_filter_enable,
                     unsigned char* vertical_filter);
void GXSetPixelFmt(int pixel_format, int z_format);
void GXSetFieldMode(unsigned char field_mode, unsigned char half_aspect_ratio);
void GXSetTevOp(int stage, int mode);
void GXInitTlutObj(GXTlutObj* object, void* image, int format,
                   unsigned short entry_count);
void GXLoadTlut(GXTlutObj* object, unsigned long id);
void GXInitTexObjCI(GXTexObj* object, void* image, unsigned short width,
                    unsigned short height, int format, int wrap_s, int wrap_t,
                    int mipmap, unsigned long tlut);
void GXSetCoPlanar(int enable);
void GXSetClipMode(int mode);
void GXSetScissorBoxOffset(int x, int y);
void GXSetFog(int type, float start, float end, float near_plane,
              float far_plane, GXColor color);
void GXSetFogRangeAdj(int enable, unsigned short center, void* table);
void GXSetColorUpdate(int enable);
void GXSetAlphaUpdate(unsigned char enable);
void GXSetZCompLoc(int before_texture);
void GXSetDither(int enable);
void GXSetDstAlpha(int enable, unsigned char alpha);
void GXSetCopyClear(GXColor color, unsigned long z);
void GXSetViewport(float left, float top, float width, float height,
                   float near_plane, float far_plane);
void GXSetViewportJitter(float left, float top, float width, float height,
                         float near_plane, float far_plane,
                         unsigned long field);
void GXCopyDisp(void* destination, unsigned char clear);
void GXSetTexCopySrc(int left, int top, unsigned short width,
                     unsigned short height);
void GXSetTexCopyDst(unsigned short width, unsigned short height, int format,
                     int mipmap);
void GXCopyTex(void* destination, unsigned char clear);
void GXCallDisplayList(void* list, unsigned long byte_count);
unsigned long GXBeginDisplayList(void* list, unsigned long byte_count);
unsigned long GXEndDisplayList(void);
void GXResetWriteGatherPipe(void);
void GXFlush(void);
void GXInvalidateTexAll(void);
void GXInitTexObj(GXTexObj* object, void* image, unsigned short width,
                  unsigned short height, int format, int wrap_s, int wrap_t,
                  int mipmap);
void GXInitTexObjLOD(GXTexObj* object, int min_filter, int mag_filter,
                     float min_lod, float max_lod, float lod_bias,
                     int bias_clamp, int edge_lod, int max_anisotropy);
int GXGetTexBufferSize(unsigned short width, unsigned short height, int format,
                       int mipmap, unsigned char max_lod);
void GXPixModeSync(void);

#ifdef __cplusplus
}
#endif

#endif
