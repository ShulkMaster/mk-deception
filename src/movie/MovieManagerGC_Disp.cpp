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

typedef enum _GXTexMapID {
    GX_TEXMAP0 = 0,
    GX_TEXMAP1 = 1
} GXTexMapID;
typedef struct CameraVectors {
    Vec up;
    Vec position;
    Vec target;
} CameraVectors;
typedef struct UsrCamObj {
    char pad[0x70];
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
    int unk4;
    MwsFrameInfo frame;
} NativeMovieProcessCtx;

static SceneCtrl scn_ctrl;

/* MWCC emits .sbss in reverse declaration order. */
int gap_08_805108BC_sbss;
char init_399;
char pad_init399[3];
void* last_tob;
char is_set_black;
char pad_is_set_black[3];

int gap_07_8050FA3C_sdata;
GXRenderModeObj* rmode = &GXNtsc480IntDf;

static const GXColorS10 tev_color_s10 = {0xFF91, 0xFF76, 0x0044, 0x0000};
static const GXColor tev_kcolor0 = {0x66, 0x00, 0xFF, 0x32};
static const GXColor tev_kcolor1 = {0x94, 0x00, 0x94, 0x94};
static const GXColor tev_kcolor2 = {0xCB, 0x00, 0x05, 0xCF};
static const GXColor tev_kcolor3 = {0x00, 0xFF, 0x00, 0x00};
static const float flt_490 = 0.0f;
static const float flt_491 = 1.0f;
static const float flt_532 = 400.0f;
static const float flt_533 = 3000.0f;
static const double dbl_535 = 4503601774854144.0;

static const CameraVectors camera_vectors = {
    {0.0f, 1.0f, 0.0f},
    {0.0f, 0.0f, 100.0f},
    {0.0f, 0.0f, 0.0f},
};

static const char stringBase0[] = "can't allocate tex buf.\n";

static GXColor copy_clear_color;

#define WGPIPE_S16 (*(volatile short*)0xCC008000)
#define WGPIPE_F32 (*(volatile float*)0xCC008000)

static void setTevPrm(GXTexMapID map0, GXTexMapID map1) {
    GXColorS10 local_s10;
    GXColor local_color;

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
    local_s10 = tev_color_s10;
    GXSetTevColorS10(1, local_s10);
    local_color = tev_kcolor0;
    GXSetTevKColor(0, local_color);
    local_color = tev_kcolor1;
    GXSetTevKColor(1, local_color);
    local_color = tev_kcolor2;
    GXSetTevKColor(2, local_color);
    local_color = tev_kcolor3;
    GXSetTevKColor(3, local_color);
    GXSetTevSwapModeTable(0, 0, 1, 2, 3);
    GXSetTevSwapModeTable(1, 0, 3, 3, 3);
    GXSetTevSwapModeTable(2, 0, 0, 3, 0);
    GXSetNumChans(0);
    GXSetNumIndStages(0);
}

/* Soft ceiling: drawTex ~80.3% -- local aggregate/register scheduling; stop. */
static void drawTex(UsrCamObj* cam, UsrTexObj* tex) {
    Mtx tex_mtx;
    Mtx44 proj;
    Mtx tmp;
    Mtx model;
    CameraVectors vectors;
    GXRenderModeObj* mode;
    float half_width;
    float half_height;
    short halfW;
    short halfH;
    short negW;
    short negH;

    if (tex->bufY == 0) {
        return;
    }
    vectors = camera_vectors;
    mode = rmode;
    half_height = (float)((unsigned int)mode->xfbHeight >> 1);
    half_width = (float)((unsigned int)mode->fbWidth >> 1);
    C_MTXFrustum(proj, half_height, -half_height, -half_width,
                 half_width, flt_532, flt_533);
    GXSetProjection(proj, 0);
    C_MTXLookAt((MtxPtr)cam, &vectors.position, &vectors.up, &vectors.target);
    GXLoadTexObj(&tex->tex0, 0);
    GXLoadTexObj(&tex->tex1, 1);
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
    PSMTXTrans(model, flt_490, flt_490, flt_490);
    PSMTXConcat((MtxPtr)cam, model, tmp);
    GXLoadPosMtxImm(tmp, 0);
    GXBegin(0x80, 0, 4);
    WGPIPE_S16 = halfW;
    WGPIPE_S16 = halfH;
    WGPIPE_S16 = 0;
    WGPIPE_F32 = flt_491;
    WGPIPE_F32 = flt_490;
    WGPIPE_S16 = halfW;
    WGPIPE_S16 = negH;
    WGPIPE_S16 = 0;
    WGPIPE_F32 = flt_491;
    WGPIPE_F32 = flt_491;
    WGPIPE_S16 = negW;
    WGPIPE_S16 = negH;
    WGPIPE_S16 = 0;
    WGPIPE_F32 = flt_490;
    WGPIPE_F32 = flt_491;
    WGPIPE_S16 = negW;
    WGPIPE_S16 = halfH;
    WGPIPE_S16 = 0;
    WGPIPE_F32 = flt_490;
    WGPIPE_F32 = flt_490;
    GXSetCullMode(2);
}

void MovieManager_Default_ProcessFrame(void* ctx, int unused, int width, int height) {
    NativeMovieProcessCtx* proc;
    UsrTexObj* tex;
    int alignedW;
    int alignedH;
    int halfW;
    int halfH;
    void* yBuf;
    void* cBuf;

    proc = (NativeMovieProcessCtx*)ctx;
    (void)unused;
    if (init_399 == 0) {
        last_tob = 0;
        init_399 = 1;
    }
    tex = &scn_ctrl.tob0;
    if (last_tob == &scn_ctrl.tob0) {
        tex = &scn_ctrl.tob1;
    } else {
        tex = &scn_ctrl.tob0;
    }
    last_tob = tex;
    if (tex->bufY == 0) {
        halfW = width >> 1;
        halfH = height >> 1;
        alignedW = ((halfW + 0x1F) & ~0x1F) + 0x1F;
        alignedW &= ~0x3FF;
        alignedH = halfH & 0xFFFF;
        tex->width = alignedW;
        tex->height = alignedH;
        tex->sizeY = GXGetTexBufferSize(tex->width, tex->height, 3, 0, 0);
        tex->sizeC = GXGetTexBufferSize(alignedW & 0xFFFF, halfH & 0xFFFF, 3, 0, 0);
        yBuf = mwMovMalloc(tex->sizeY);
        tex->bufY = yBuf;
        cBuf = mwMovMalloc(tex->sizeC);
        tex->bufC = cBuf;
        if (yBuf == 0 || cBuf == 0) {
            OSReport(stringBase0);
        } else {
            if (yBuf != 0) {
                memset(yBuf, 0, tex->sizeY);
                memset(cBuf, 0x80, tex->sizeC);
            }
            GXInitTexObj(&tex->tex0, tex->bufY, tex->width, tex->height, 3, 0, 0, 1);
            GXInitTexObj(&tex->tex1, tex->bufC, alignedW & 0xFFFF, halfH & 0xFFFF, 3, 0, 0, 0);
            GXInitTexObjLOD(&tex->tex0, 0, 0, flt_490, flt_490, flt_490, 0, 0, 0);
            GXInitTexObjLOD(&tex->tex1, 0, 0, flt_490, flt_490, flt_490, 0, 0, 0);
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
    SceneCtrl* ctrl;

    ctrl = &scn_ctrl;
    if (ctrl->tob0.bufY != 0) {
        mwMovFree(ctrl->tob0.bufY);
        mwMovFree(ctrl->tob0.bufC);
        ctrl->tob0.bufY = 0;
        ctrl->tob0.bufC = 0;
    }
    if (ctrl->tob1.bufY != 0) {
        mwMovFree(ctrl->tob1.bufY);
        mwMovFree(ctrl->tob1.bufC);
        ctrl->tob1.bufY = 0;
        ctrl->tob1.bufC = 0;
    }
    GXSetTevSwapModeTable(0, 0, 1, 2, 3);
    GXSetTevSwapModeTable(1, 0, 1, 2, 3);
    GXSetTevSwapModeTable(2, 0, 1, 2, 3);
}

void MovieManager_Default_StartVideo(void) {
    SceneCtrl* ctrl;
    Mtx44 proj;
    Vec camPos;
    Vec up;
    Vec target;

    GXSetCopyClear(copy_clear_color, 0xFFFFFF);
    ctrl = &scn_ctrl;
    memset(ctrl, 0, 0x120);
    GXInvalidateTexAll();
    GXSetTexCoordGen2(0, 1, 4, 0x1E, 0, 0x7D);
    GXSetNumTexGens(1);
    C_MTXFrustum(proj, (float)(rmode->efbHeight >> 1), (float)(-(rmode->efbHeight >> 1)),
                 (float)(rmode->fbWidth >> 1), (float)(-(rmode->fbWidth >> 1)), flt_532,
                 flt_533);
    GXSetProjection(proj, 0);
    camPos.x = camera_vectors.up.x;
    camPos.y = camera_vectors.up.y;
    camPos.z = camera_vectors.up.z;
    up.x = camera_vectors.position.x;
    up.y = camera_vectors.position.y;
    up.z = camera_vectors.position.z;
    target.x = camera_vectors.target.x;
    target.y = camera_vectors.target.y;
    target.z = camera_vectors.target.z;
    C_MTXLookAt((MtxPtr)&scn_ctrl.cam, &camPos, &up, &target);
    ctrl->tob0.bufY = 0;
    ctrl->tob0.bufC = 0;
    ctrl->tob1.bufY = 0;
    ctrl->tob1.bufC = 0;
    VISetBlack(1);
    is_set_black = 1;
    if (ctrl->tob0.bufY != 0) {
        memset(ctrl->tob0.bufY, 0, ctrl->tob0.sizeY);
        memset(ctrl->tob0.bufC, 0x80, ctrl->tob0.sizeC);
    }
    if (ctrl->tob1.bufY != 0) {
        memset(ctrl->tob1.bufY, 0, ctrl->tob1.sizeY);
        memset(ctrl->tob1.bufC, 0x80, ctrl->tob1.sizeC);
    }
}
