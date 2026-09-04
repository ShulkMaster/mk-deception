#include "movie/MovieManagerGC_Disp.h"

#include "dolphin/gx.h"
#include "dolphin/cache.h"
#include "dolphin/mtx.h"
#include "dolphin/os.h"
#include "dolphin/vi.h"
#include "math/gxVect.h"
#include "movie/MovieConfig.h"
#include "movie/mwsfx.h"
#include "platform/display_metrics.h"
#include "runtime/cstring.h"

typedef struct CameraVectors {
    Vec up;
    Vec position;
    Vec target;
} CameraVectors;

typedef struct UsrCamObj {
    Mtx view;
    char field_0x30[0x40];
} UsrCamObj;

typedef struct UsrTexObj {
    GXTexObj tex0;
    GXTexObj tex1;
    void* bufY;
    void* bufC;
    int sizeY;
    int sizeC;
    int width;
    int height;
} UsrTexObj;

typedef struct SceneCtrl {
    UsrCamObj cam;
    UsrTexObj tob0;
    UsrTexObj tob1;
} SceneCtrl;

typedef struct NativeMovieProcessCtx {
    int handle;
    int field_0x04;
    MwsFrameInfo frame;
} NativeMovieProcessCtx;

typedef char UsrCamObjSizeCheck[sizeof(UsrCamObj) == 0x70 ? 1 : -1];
typedef char UsrTexObjSizeCheck[sizeof(UsrTexObj) == 0x58 ? 1 : -1];
typedef char SceneCtrlSizeCheck[sizeof(SceneCtrl) == 0x120 ? 1 : -1];
typedef char NativeMovieProcessCtxSizeCheck[
    sizeof(NativeMovieProcessCtx) == 0x0C ? 1 : -1];

static SceneCtrl scn_ctrl;
static unsigned char is_set_black;
static GXRenderModeObj* rmode = &GXNtsc480IntDf;
static const CameraVectors camera_vectors = {
    {0.0f, 1.0f, 0.0f},
    {0.0f, 0.0f, 400.0f},
    {0.0f, 0.0f, 0.0f},
};
static const char allocation_error[] = "can't allocate tex buf.\n";

#define WGPIPE_S16 (*(volatile short*)GXFIFO_ADDR)
#define WGPIPE_F32 (*(volatile float*)GXFIFO_ADDR)

/* Retail uses compact nonvolatile saves for this setup routine. */
#pragma optimize_for_size on
static void setTevPrm(GXTexMapID map0, GXTexMapID map1) {
    GXSetNumTexGens(2);
    GXSetTexCoordGen2(0, 1, 4, 0x3C, 0, 0x7D);
    GXSetTexCoordGen2(1, 1, 4, 0x3C, 0, 0x7D);
    GXSetNumTevStages(4);
    GXSetTevOrder(0, 0, map1, 0xFF);
    GXSetTevColorIn(0, 0xF, 8, 0xE, 2);
    GXSetTevColorOp(0, 0, 0, 0, 0, 0);
    GXSetTevAlphaIn(0, 7, 4, 6, 1);
    GXSetTevAlphaOp(0, 1, 0, 0, 0, 0);
    GXSetTevKColorSel(0, 0xC);
    GXSetTevKAlphaSel(0, 0x1C);
    GXSetTevSwapMode(0, 0, 1);
    GXSetTevOrder(1, 1, map0, 0xFF);
    GXSetTevColorIn(1, 0xF, 8, 0xE, 0);
    GXSetTevColorOp(1, 0, 0, 1, 0, 0);
    GXSetTevAlphaIn(1, 7, 4, 6, 0);
    GXSetTevAlphaOp(1, 0, 0, 1, 0, 0);
    GXSetTevKColorSel(1, 0xD);
    GXSetTevKAlphaSel(1, 0x1D);
    GXSetTevSwapMode(1, 0, 0);
    GXSetTevOrder(2, 0, map1, 0xFF);
    GXSetTevColorIn(2, 0xF, 8, 0xE, 0);
    GXSetTevColorOp(2, 0, 0, 0, 1, 0);
    GXSetTevAlphaIn(2, 7, 4, 6, 0);
    GXSetTevAlphaOp(2, 1, 0, 0, 1, 0);
    GXSetTevKColorSel(2, 0xE);
    GXSetTevKAlphaSel(2, 0x1E);
    GXSetTevSwapMode(2, 0, 2);
    GXSetTevOrder(3, 0xFF, 0xFF, 0xFF);
    GXSetTevColorIn(3, 0, 1, 0xE, 0xF);
    GXSetTevColorOp(3, 0, 0, 0, 1, 0);
    GXSetTevAlphaIn(3, 7, 7, 7, 7);
    GXSetTevAlphaOp(3, 0, 0, 0, 1, 0);
    GXSetTevSwapMode(3, 0, 0);
    GXSetTevKColorSel(3, 0xF);
    GXColorS10 color_s10 = {-111, 0, -138, 68};
    GXSetTevColorS10(1, color_s10);
    GXColor color0 = {0x66, 0x00, 0xFF, 0x32};
    GXSetTevKColor(0, color0);
    GXColor color1 = {0x94, 0x00, 0x94, 0x94};
    GXSetTevKColor(1, color1);
    GXColor color2 = {0xCB, 0x00, 0x05, 0xCF};
    GXSetTevKColor(2, color2);
    GXColor color3 = {0x00, 0xFF, 0x00, 0x00};
    GXSetTevKColor(3, color3);
    GXSetTevSwapModeTable(0, 0, 1, 2, 3);
    GXSetTevSwapModeTable(1, 0, 3, 3, 3);
    GXSetTevSwapModeTable(2, 0, 0, 3, 0);
    GXSetNumChans(0);
    GXSetNumIndStages(0);
}

#pragma optimize_for_size reset

static void drawTex(UsrCamObj* cam, UsrTexObj* tex) {
    Mtx tex_mtx;
    Mtx44 proj;
    Mtx tmp;
    Mtx model;
    Vec up;
    Vec position;
    Vec target;
    float half_width;
    float half_height;
    float tex_zero = 0.0f;
    float tex_one = 1.0f;
    short halfW;
    short halfH;
    short negW;
    short negH;

    if (tex->bufY == 0) {
        return;
    }
    up = camera_vectors.up;
    position = camera_vectors.position;
    target = camera_vectors.target;
    half_height = (float)(rmode->xfbHeight >> 1);
    half_width = (float)(rmode->fbWidth >> 1);
    C_MTXFrustum(proj, half_height, -half_height, -half_width,
                 half_width, 400.0f, 3000.0f);
    GXSetProjection(proj, 0);
    C_MTXLookAt(cam->view, &position, &up, &target);
    GXLoadTexObj(&tex->tex0, GX_TEXMAP0);
    GXLoadTexObj(&tex->tex1, GX_TEXMAP1);
    setTevPrm(GX_TEXMAP0, GX_TEXMAP1);
    GXSetBlendMode(1, 1, 0, 0);
    GXSetCullMode(0);
    GXSetZMode(0, 7, 0);
    GXSetAlphaCompare(7, 0, 1, 7, 0);
    PSMTXIdentity(tex_mtx);
    GXLoadTexMtxImm(tex_mtx, 0x1E, 1);
    GXClearVtxDesc();
    GXSetVtxDesc(9, 1);
    GXSetVtxDesc(0xD, 1);
    GXSetVtxAttrFmt(0, 9, 1, 3, 0);
    GXSetVtxAttrFmt(0, 0xD, 1, 4, 0);
    halfW = (short)(screen_width / 2);
    halfH = (short)(screen_height / 2);
    negW = (short)-halfW;
    negH = (short)-halfH;
    PSMTXTrans(model, 0.0f, 0.0f, 0.0f);
    PSMTXConcat(cam->view, model, tmp);
    GXLoadPosMtxImm(tmp, 0);
    GXBegin(0x80, 0, 4);
    WGPIPE_S16 = halfW;
    WGPIPE_S16 = halfH;
    WGPIPE_S16 = 0;
    WGPIPE_F32 = tex_one;
    WGPIPE_F32 = tex_zero;
    WGPIPE_S16 = halfW;
    WGPIPE_S16 = negH;
    WGPIPE_S16 = 0;
    WGPIPE_F32 = tex_one;
    WGPIPE_F32 = tex_one;
    WGPIPE_S16 = negW;
    WGPIPE_S16 = negH;
    WGPIPE_S16 = 0;
    WGPIPE_F32 = tex_zero;
    WGPIPE_F32 = tex_one;
    WGPIPE_S16 = negW;
    WGPIPE_S16 = halfH;
    WGPIPE_S16 = 0;
    WGPIPE_F32 = tex_zero;
    WGPIPE_F32 = tex_zero;
    GXSetCullMode(2);
}

void MovieManager_Default_ProcessFrame(void* ctx, int unused, int width, int height) {
    static UsrTexObj* last_tob;
    static char init;
    NativeMovieProcessCtx* proc;
    UsrTexObj* tex;
    int padded_half_width;
    unsigned short chroma_width;
    unsigned short chroma_height;
    void* y_buffer;

    proc = (NativeMovieProcessCtx*)ctx;
    (void)unused;
    if (init == 0) {
        last_tob = 0;
        init = 1;
    }
    if (last_tob == &scn_ctrl.tob0) {
        tex = &scn_ctrl.tob1;
    } else {
        tex = &scn_ctrl.tob0;
    }
    last_tob = tex;
    if (tex->bufY == 0) {
        padded_half_width = width / 2 + 0x1F;
        chroma_width = (unsigned short)(padded_half_width & 0xFFE0);
        chroma_height = (unsigned short)(height / 2);
        tex->width =
            ((((padded_half_width * 2) & 0x1FFC0) + 0x1F) & 0xFFE0);
        tex->height = (unsigned short)height;
        tex->sizeY = GXGetTexBufferSize(
            (unsigned short)tex->width, (unsigned short)tex->height,
            GX_TF_I8, 0, 0);
        tex->sizeC = GXGetTexBufferSize(
            chroma_width, chroma_height, GX_TF_IA8, 0, 0);
        y_buffer = mwMovMalloc(tex->sizeY);
        tex->bufY = y_buffer;
        tex->bufC = mwMovMalloc(tex->sizeC);
        if (y_buffer == 0 || tex->bufC == 0) {
            OSReport(allocation_error);
        } else {
            if (y_buffer != 0) {
                memset(y_buffer, 0, tex->sizeY);
                memset(tex->bufC, 0x80, tex->sizeC);
            }
            GXInitTexObj(&tex->tex0, tex->bufY,
                         (unsigned short)tex->width,
                         (unsigned short)tex->height,
                         GX_TF_I8, 0, 0, 0);
            GXInitTexObj(&tex->tex1, tex->bufC,
                         chroma_width, chroma_height,
                         GX_TF_IA8, 0, 0, 0);
            GXInitTexObjLOD(&tex->tex0, 0, 0, 0.0f, 0.0f, 0.0f,
                            0, 0, 0);
            GXInitTexObjLOD(&tex->tex1, 0, 0, 0.0f, 0.0f, 0.0f,
                            0, 0, 0);
        }
    }
    mwPlyFxSetOutBufPitchHeight(proc->handle, tex->width, tex->height);
    mwPlyFxCnvFrmY84C44(proc->handle, &proc->frame, tex->bufY, tex->bufC);
    DCStoreRange(tex->bufY, tex->sizeY);
    DCStoreRange(tex->bufC, tex->sizeC);
    GXPixModeSync();
    drawTex(&scn_ctrl.cam, tex);
    GXPixModeSync();
    if (is_set_black != 0) {
        VISetBlack(0);
        is_set_black = 0;
    }
}

void MovieManager_Default_VSync(void) {}

void MovieManager_Default_StopVideo(void) {
    if (scn_ctrl.tob0.bufY != 0) {
        mwMovFree(scn_ctrl.tob0.bufY);
        mwMovFree(scn_ctrl.tob0.bufC);
        scn_ctrl.tob0.bufY = 0;
        scn_ctrl.tob0.bufC = 0;
    }
    if (scn_ctrl.tob1.bufY != 0) {
        mwMovFree(scn_ctrl.tob1.bufY);
        mwMovFree(scn_ctrl.tob1.bufC);
        scn_ctrl.tob1.bufY = 0;
        scn_ctrl.tob1.bufC = 0;
    }
    GXSetTevSwapModeTable(0, 0, 1, 2, 3);
    GXSetTevSwapModeTable(1, 0, 1, 2, 3);
    GXSetTevSwapModeTable(2, 0, 1, 2, 3);
}

void MovieManager_Default_StartVideo(void) {
    GXColor copy_clear_color = {0, 0, 0, 0};
    Mtx44 proj;
    Vec up;
    Vec position;
    Vec target;
    float half_height;
    float half_width;

    GXSetCopyClear(copy_clear_color, 0xFFFFFF);
    memset(&scn_ctrl, 0, sizeof(scn_ctrl));
    GXInvalidateTexAll();
    GXSetTexCoordGen2(0, 1, 4, 0x1E, 0, 0x7D);
    GXSetNumTexGens(1);
    up = camera_vectors.up;
    position = camera_vectors.position;
    target = camera_vectors.target;
    half_height = (float)(rmode->efbHeight >> 1);
    half_width = (float)(rmode->fbWidth >> 1);
    C_MTXFrustum(proj, half_height, -half_height, -half_width,
                 half_width, 400.0f, 3000.0f);
    GXSetProjection(proj, 0);
    C_MTXLookAt(scn_ctrl.cam.view, &position, &up, &target);
    scn_ctrl.tob0.bufY = 0;
    scn_ctrl.tob0.bufC = 0;
    scn_ctrl.tob1.bufY = 0;
    scn_ctrl.tob1.bufC = 0;
    VISetBlack(1);
    is_set_black = 1;
    if (scn_ctrl.tob0.bufY != 0) {
        memset(scn_ctrl.tob0.bufY, 0, scn_ctrl.tob0.sizeY);
        memset(scn_ctrl.tob0.bufC, 0x80, scn_ctrl.tob0.sizeC);
    }
    if (scn_ctrl.tob1.bufY != 0) {
        memset(scn_ctrl.tob1.bufY, 0, scn_ctrl.tob1.sizeY);
        memset(scn_ctrl.tob1.bufC, 0x80, scn_ctrl.tob1.sizeC);
    }
}

int gap_08_805108BC_sbss;
