#include "platform/gcdisplay.h"
#include "runtime/cstring.h"
#include "libmkparticle/gc_state.h"
#include "platform/display_metrics.h"
#include "platform/gcio.h"
#include "platform/gprofile_gcn.h"
#include "runtime/mk_obj.h"

#include "dolphin/cache.h"
#include "dolphin/gx.h"
#include "dolphin/os.h"
#include "dolphin/pad.h"
#include "dolphin/vi.h"
#include "mw/mwMemHeap.h"
#include "platform/gcutils.h"
#include "runtime/mk_proc.h"
#include "runtime/mk_struct.h"
#include "rw/rwengine.h"
#include "rw/gamecube.h"

struct GcNativeDisplay {
    GXRenderModeObj* rmode;
    void* fifo;
    void* xfbDisp;
    void* xfbCopy;
};

typedef void (*NativeRenderCb)(void* arg);

unsigned short loading_palette[0x100] = {
#include "platform/gcdisplay_loading_palette.inc"
};
unsigned char loading_image[0x10000] = {
#include "platform/gcdisplay_loading_image.inc"
};

GXTexObj feedbackTex;
static struct GcNativeDisplay gc_native_display;
static GXColor Black = {0, 0, 0, 0xFF};
static const GXColor kTextWhite = {0xFF, 0xFF, 0xFF, 0xFF};
static const GXColor kRenderBlack = {0, 0, 0, 0xFF};
static const GXColor kMovieRenderBlack = {0, 0, 0, 0xFF};
static const GXColor kFeedbackColor = {0xC4, 0xC4, 0xC4, 0xFF};
static int uFrameBlastCount = 0xF;
static void* LastSheet;
static OSFontHeader* FontData;
static short FontSize;
static short FontSpace;
void* feedbackTexPixels;
static unsigned short* pal_565;
static int progscan_mode;

static volatile unsigned short* const wgPipe = (volatile unsigned short*)GXFIFO_ADDR;

static void gc_native_display_render(NativeRenderCb cb, void* arg);
static void render_text(void* text);
static void render_text_without_clear(char* text, int x, int y);
static void render_image(void* unused);
static void CheckFor480PMode(void);
static void displayContinueMessage(PADStatus* pads, char* msg);
static void gcSetup480P(void);
static int gc_prompt_for_480P(PADStatus* pads);
static void display_dragon_with_text(void* arg);
static void display_image(void);

static inline int timed_out_10s(OSTime start) {
    OSTime now;
    OSTime diff;
    unsigned long limit;

    now = OSGetTime();
    diff = now - start;
    limit = (__OSBusClock >> 2) * 10;
    return diff > (OSTime)limit;
}

static inline int font_string_width(char* s) {
    int lineW;
    int maxW;
    int charW;

    if (FontData == 0) {
        return 0;
    }
    maxW = 0;
    lineW = 0;
    while (*s != '\0') {
        if (*s == '\n') {
            if (maxW < lineW) {
                maxW = lineW;
            }
            lineW = 0;
        }
        s = OSGetFontWidth(s, &charW);
        lineW += FontSpace + (FontSize * charW) / FontData->cellWidth;
    }
    if (maxW < lineW) {
        maxW = lineW;
    }
    return (maxW + 0xF) / 0x10;
}

static inline int font_string_height(char* s) {
    int lines;
    OSFontHeader* font;

    font = FontData;
    if (font == 0) {
        return 0;
    }
    lines = 1;
    for (; *s != '\0'; s++) {
        if (*s == '\n') {
            lines++;
        }
    }
    lines *= (font->leading * FontSize) / font->cellWidth;
    return (lines + 0xF) / 0x10;
}

void feedback_effect(void) {
    Mtx texMtx;
    Mtx posMtx = {
        {1.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, -1.0f, 0.0f},
    };
    GXColor color = kFeedbackColor;
    GXColor amb;
    GXColor mat;
    int h;
    int w;
    int full_w;
    int full_h;

    RwEngineInstance->dOpenDevice.fpRenderStateSet(0x14, 1);
    RwEngineInstance->dOpenDevice.fpRenderStateSet(6, 0);
    RwEngineInstance->dOpenDevice.fpRenderStateSet(0xA, 5);
    RwEngineInstance->dOpenDevice.fpRenderStateSet(0xB, 2);

    save_projection_matrix();
    set_2d_projection();
    GXLoadPosMtxImm(posMtx, 0);
    GXLoadTexObj(&feedbackTex, 4);
    PSMTXScale(texMtx, 1.0f / (float)screen_width, 1.0f / (float)screen_height, 1.0f);
    GXLoadTexMtxImm(texMtx, 0x21, 1);
    GXSetNumTexGens(1);
    GXSetNumTevStages(1);
    GXSetNumChans(1);

    amb = color;
    GXSetChanAmbColor(4, amb);
    color.a = feedback_blendrate;
    mat = color;
    GXSetChanMatColor(4, mat);
    GXSetChanCtrl(4, 0, 0, 0, 0, 0, 2);
    GXSetTexCoordGen2(0, 1, 4, 0x21, 0, 0x7D);
    GXSetTevOrder(0, 0, 4, 4);
    GXSetTevOp(0, 0);

    if (old_use_feedback_effect == use_feedback_effect) {
        full_w = screen_width;
        full_h = screen_height;
        w = (short)full_w;
        h = (short)full_h;
        GXClearVtxDesc();
        GXSetVtxDesc(9, 1);
        GXSetVtxDesc(0xD, 1);
        GXSetVtxAttrFmt(0, 9, 0, 3, 0);
        GXSetVtxAttrFmt(0, 0xD, 1, 3, 0);
        GXBegin(0x80, 0, 4);
        wgPipe[0] = 0;
        wgPipe[0] = 0;
        wgPipe[0] = 0;
        wgPipe[0] = 0;
        wgPipe[0] = w;
        wgPipe[0] = 0;
        wgPipe[0] = full_w;
        wgPipe[0] = 0;
        wgPipe[0] = w;
        wgPipe[0] = h;
        wgPipe[0] = full_w;
        wgPipe[0] = full_h;
        wgPipe[0] = 0;
        wgPipe[0] = h;
        wgPipe[0] = 0;
        wgPipe[0] = full_h;
    }

    GXSetTexCopySrc(0, 0, screen_width, screen_height);
    GXSetTexCopyDst(screen_width, screen_height, 4, 0);
    GXCopyTex(feedbackTexPixels, 0);
    restore_projection_matrix();

    RwEngineInstance->dOpenDevice.fpRenderStateSet(6, 1);
    RwEngineInstance->dOpenDevice.fpRenderStateSet(0x14, 2);
    RwEngineInstance->dOpenDevice.fpRenderStateSet(0xA, 5);
    RwEngineInstance->dOpenDevice.fpRenderStateSet(0xB, 6);
}

void gc_setup_feedback_buffer_for_konquest(void) {
    feedbackTexPixels =
        _mwMemMalloc(permanent_heap, screen_width * screen_height * 2, 5, 0, 0, 0);
    mk_insert(get_mkx_mem(feedbackTexPixels), &aproc->pdata_list_b);
    if (feedbackTexPixels != 0) {
        GXInitTexObj(&feedbackTex, feedbackTexPixels, screen_width, screen_height, 4, 0, 0,
                     0);
        GXInitTexObjLOD(&feedbackTex, 1, 1, 0.0f, 0.0f, 0.0f, 0, 0, 0);
    }
}

void setup_post_effect_buffers(void) {
}

static inline void romfont_load_sheet(void* sheet, void* previousSheet, GXTexObj* texObj, Mtx texMtx) {
    float invW;
    float invH;

    if (previousSheet != sheet) {
        LastSheet = sheet;
        GXInitTexObj(texObj, sheet, FontData->sheetWidth, FontData->sheetHeight,
                     FontData->sheetFormat, 0, 0, 0);
        GXInitTexObjLOD(texObj, 1, 1, 0.0f, 0.0f, 0.0f, 0, 0, 0);
        GXLoadTexObj(texObj, 0);
        invW = 1.0f / (float)FontData->sheetWidth;
        invH = 1.0f / (float)FontData->sheetHeight;
        PSMTXScale(texMtx, invW, invH, 1.0f);
        GXLoadTexMtxImm(texMtx, 0x1E, 1);
        GXSetNumTexGens(1);
        GXSetTexCoordGen2(0, 1, 4, 0x1E, 0, 0x7D);
    }
}

/* TODO: [near miss] 99.80%; sheet comparison agrees; texture y register allocation remains. */
int romfont_puts(int x, int y, char* text) {
    int charW;
    void* sheet;
    int sheetX;
    int sheetY;
    short x1;
    short x0;
    short y1;
    short y0;
    int u0;
    int u1;
    short v1;
    int v0;
    unsigned short cellW;
    int penX;
    GXTexObj texObj;
    Mtx texMtx;

    LastSheet = 0;
    GXClearVtxDesc();
    GXSetVtxDesc(9, 1);
    GXSetVtxDesc(0xD, 1);
    GXSetVtxAttrFmt(0, 9, 0, 3, 4);
    GXSetVtxAttrFmt(0, 0xD, 1, 3, 0);

    x = (short)(x << 4);
    y <<= 4;
    penX = 0;
    while (*text != '\0') {
        if (*text == '\n') {
            penX = 0;
            text++;
            y += (FontData->leading * FontSize) / FontData->cellWidth;
            continue;
        }

        text = OSGetFontTexture(text, &sheet, &sheetX, &sheetY, &charW);
        romfont_load_sheet(sheet, LastSheet, &texObj, texMtx);

        cellW = FontData->cellWidth;
        x0 = x + penX;
        x1 = x0 + FontSize;
        u0 = sheetX;
        v0 = sheetY;
        y0 = (short)y - (FontData->ascent * FontSize) / cellW;
        y1 = (short)y + (FontData->descent * FontSize) / cellW;
        v1 = sheetY + FontData->cellHeight;

        GXBegin(0x80, 0, 4);
        u1 = u0 + cellW;
        wgPipe[0] = x0;
        wgPipe[0] = y0;
        wgPipe[0] = u0;
        wgPipe[0] = v0;
        wgPipe[0] = x1;
        wgPipe[0] = y0;
        wgPipe[0] = u1;
        wgPipe[0] = v0;
        wgPipe[0] = x1;
        wgPipe[0] = y1;
        wgPipe[0] = u1;
        wgPipe[0] = v1;
        wgPipe[0] = x0;
        wgPipe[0] = y1;
        wgPipe[0] = u0;
        wgPipe[0] = v1;

        penX += FontSpace + (FontSize * charW) / FontData->cellWidth;
    }

    return (penX + 0xF) / 0x10;
}

void gc_native_display_render_text(char* text) {
    if (OSGetFontEncode() == 1) {
        FontData = _mwMemMalloc(wave_heap, OS_FONT_SIZE_SJIS, 5, 0, 0, 0);
    } else {
        FontData = _mwMemMalloc(wave_heap, OS_FONT_SIZE_ANSI, 5, 0, 0, 0);
    }
    if (FontData == 0) {
        OSPanic("gcdisplay.c", 0x50D, "Ins. memory to load ROM font.");
    }
    if (OSInitFont(FontData) == 0) {
        OSPanic("gcdisplay.c", 0x511, "ROM font is available in boot ROM ver 0.8 or later.");
    }
    FontSize = FontData->cellWidth << 4;
    FontSpace = -0x10;
    gc_native_display_render(render_text, text);
    if (FontData != 0) {
        _mwMemFree(FontData, 0, 0);
        FontData = 0;
    }
}

void gc_native_display_render_image(void) {
    gc_native_display_render(render_image, 0);
}

/* TODO: [near miss] 97.78%; clear-color load precedes matrix setup; local literal regresses the exact constant pool. */
void gc_native_display_render_movie(void* ctx) {
    GXColor clearColor = kMovieRenderBlack;
    Mtx posMtx = {
        {1.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, -1.0f, 0.0f},
    };

    GXSetCoPlanar(0);
    GXSetCullMode(0);
    GXSetClipMode(0);
    GXSetScissor(0, 0, gc_native_display.rmode->fbWidth, gc_native_display.rmode->efbHeight);
    GXSetScissorBoxOffset(0, 0);
    GXSetNumIndStages(0);
    GXSetFog(0, 0.0f, 1.0f, 0.1f, 1.0f, clearColor);
    GXSetFogRangeAdj(0, 0, 0);
    GXSetBlendMode(0, 4, 5, 0);
    GXSetColorUpdate(1);
    GXSetAlphaUpdate(1);
    GXSetZMode(1, 3, 1);
    GXSetZCompLoc(1);
    GXSetDither(1);
    GXSetDstAlpha(0, 0);
    GXSetPixelFmt(0, 0);
    GXLoadPosMtxImm(posMtx, 0);
    GXSetCopyClear(clearColor, 0x00FFFFFFu);
    GProfile_GCN_GxDrawDone();
    VIWaitForRetrace();
    VIFlush();
    GXCopyDisp(_RwGCXFBDisp, 1);
}

/* TODO: [near miss] 98.88%; captured black-load schedule remains; literal form regresses constant-pool data. */
static void gc_native_display_render(NativeRenderCb cb, void* arg) {
    GXColor black = kRenderBlack;
    Mtx posMtx = {
        {1.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, -1.0f, 0.0f},
    };
    GXColor clearColor;
    GXColor fogColor;
    void* framebuffers[2];
    void* curXfb;
    int first;
    int i;
    GXRenderModeObj* mode;
    first = 1;

    if (gc_native_display.xfbDisp != 0) {
        framebuffers[0] = gc_native_display.xfbDisp;
        framebuffers[1] = gc_native_display.xfbCopy;
    } else {
        RwGameCubeGetXFBs(&framebuffers[0], &framebuffers[1]);
    }

    GXSetCoPlanar(0);
    GXSetCullMode(2);
    GXSetClipMode(0);
    mode = gc_native_display.rmode;
    GXSetScissor(0, 0, mode->fbWidth, mode->efbHeight);
    GXSetScissorBoxOffset(0, 0);
    GXSetNumIndStages(0);

    fogColor = black;
    GXSetFog(0, 0.0f, 1.0f, 0.1f, 1.0f, fogColor);
    GXSetFogRangeAdj(0, 0, 0);
    GXSetBlendMode(0, 4, 5, 0);
    GXSetColorUpdate(1);
    GXSetAlphaUpdate(1);
    GXSetZMode(0, 3, 0);
    GXSetZCompLoc(1);
    GXSetDither(1);
    GXSetDstAlpha(0, 0);
    GXSetPixelFmt(0, 0);
    GXLoadPosMtxImm(posMtx, 0);
    VISetBlack(0);
    VIFlush();

    clearColor = black;
    curXfb = framebuffers[0];
    GXSetCopyClear(clearColor, 0x00FFFFFFu);
    for (i = 0; i < uFrameBlastCount; i++) {
        mode = gc_native_display.rmode;
        if (mode->field_rendering != 0) {
            GXSetViewportJitter(0.0f, 0.0f, mode->fbWidth, mode->efbHeight, 0.0f, 1.0f,
                                VIGetNextField());
        } else {
            GXSetViewport(0.0f, 0.0f, mode->fbWidth, mode->efbHeight, 0.0f, 1.0f);
        }

        if (cb != 0) {
            cb(arg);
        }

        GXCopyDisp(curXfb, 1);
        GXDrawDone();

        if (first != 0) {
            VISetBlack(0);
            first = 0;
        }

        VISetNextFrameBuffer(curXfb);
        VIFlush();
        VIWaitForRetrace();

        if (curXfb == framebuffers[0]) {
            curXfb = framebuffers[1];
        } else {
            curXfb = framebuffers[0];
        }
    }
}

/* TODO: [near miss] 99.47%; GXColor copy slots and font-width registers differ; stop at coloring. */
static void render_text(void* text) {
    char* s;
    GXColor black;
    int w;
    int msgW;
    int msgH;
    int x;
    int y;

    s = text;
    save_projection_matrix();
    set_2d_projection();

    black = Black;
    w = screen_width;

    GXSetNumChans(1);
    GXSetChanCtrl(0, 0, 0, 0, 0, 0, 2);
    GXSetChanMatColor(0, black);
    GXSetChanAmbColor(0, black);
    GXSetNumTexGens(0);
    GXSetNumTevStages(1);
    GXSetTevOrder(0, 0xFF, 0xFF, 4);
    GXSetTevOp(0, 4);
    GXClearVtxDesc();
    GXSetVtxDesc(9, 1);
    GXSetVtxAttrFmt(0, 9, 0, 3, 0);
    GXBegin(0x80, 0, 4);
    wgPipe[0] = 0;
    wgPipe[0] = 0;
    wgPipe[0] = w;
    wgPipe[0] = 0;
    wgPipe[0] = w;
    wgPipe[0] = 0x1E0;
    wgPipe[0] = 0;
    wgPipe[0] = 0x1E0;

    GXSetNumChans(0);
    GXSetNumTexGens(0);
    GXSetNumTevStages(1);
    GXSetTevOp(0, 3);
    GXSetTevOrder(0, 0, 0, 0xFF);

    msgW = font_string_width(s);
    x = (screen_width - msgW) / 2;
    msgH = font_string_height(s);
    y = (screen_height - msgH) / 2;
    romfont_puts(x, y, s);
    restore_projection_matrix();
}

#pragma dont_inline on
/* TODO: [near miss] 92.98%; named white-color copy prologue differs; anonymous initializer needs TU data reconciliation. */
static void render_text_without_clear(char* text, int x, int y) {
    char* walk;
    GXColor color;

    walk = text;
    color = kTextWhite;
    if (FontData != 0) {
        for (; *walk != '\0'; walk++) {
        }
    }

    GXSetNumChans(1);
    GXSetChanCtrl(0, 0, 0, 0, 0, 0, 2);
    GXSetNumTevStages(1);
    GXSetNumTexGens(1);
    GXSetTexCoordGen2(0, 1, 4, 0x3C, 0, 0x7D);
    GXSetChanMatColor(0, color);
    GXSetChanAmbColor(0, color);
    GXSetTevOrder(0, 0, 0, 4);
    GXSetTevColorIn(0, 0xF, 0xA, 8, 0xF);
    GXSetTevColorOp(0, 0, 0, 0, 1, 0);
    GXSetTevAlphaIn(0, 7, 5, 4, 7);
    GXSetTevAlphaOp(0, 0, 0, 0, 1, 0);
    romfont_puts(x, y, text);
}
#pragma dont_inline reset

static inline void set_chan0_color(GXColor color)
{
    GXSetChanMatColor(0, color);
    GXSetChanAmbColor(0, color);
}

/* TODO: [near miss] 99.954025%; by-value GXColor copy slots remain reversed; named
 * per-call locals coalesce, and a shared inline with display_dragon_with_text regresses. */
static void render_image(void* unused) {
    int w;
    int h;
    GXColor black;

    save_projection_matrix();
    set_2d_projection();

    black = Black;
    h = screen_height;
    w = screen_width;

    GXSetNumChans(1);
    GXSetChanCtrl(0, 0, 0, 0, 0, 0, 2);
    set_chan0_color(black);
    GXSetNumTexGens(0);
    GXSetNumTevStages(1);
    GXSetTevOrder(0, 0xFF, 0xFF, 4);
    GXSetTevOp(0, 4);
    GXClearVtxDesc();
    GXSetVtxDesc(9, 1);
    GXSetVtxAttrFmt(0, 9, 0, 3, 0);
    GXBegin(0x80, 0, 4);
    wgPipe[0] = 0;
    wgPipe[0] = 0;
    wgPipe[0] = w;
    wgPipe[0] = 0;
    wgPipe[0] = w;
    wgPipe[0] = h;
    wgPipe[0] = 0;
    wgPipe[0] = h;

    GXSetNumChans(0);
    GXSetNumTexGens(0);
    GXSetNumTevStages(1);
    GXSetTevOp(0, 3);
    GXSetTevOrder(0, 0, 0, 0xFF);
    display_image();
    restore_projection_matrix();
}

void gc_native_display_pass_to_RW(void) {
    GXDrawDone();
    if (pal_565 != 0) {
        _mwMemFree(pal_565, 0, 0);
        pal_565 = 0;
    }
    _RwGCXFBDisp = gc_native_display.xfbDisp;
    _RwGCXFBCopy = gc_native_display.xfbCopy;
    _RwDlDefaultFifo = gc_native_display.fifo;
}

/* TODO: [near miss] 99.29%; equivalent TV-format dispatch omits retail
 * fallback jump; copy-height ABI and field predicate agree. */
void gc_native_display_init(void) {
    int tvFormat;
    GXRenderModeObj* mode;
    int xfbBytes;
    int xfbHalf;
    void* raw;
    void* fifo;
    unsigned char* xfb1;
    unsigned char* xfb2;
    float yscale;
    unsigned long copyHeight;
    int pixFmt;

    gc_grab_renderpipe();
    VIInit();
    tvFormat = VIGetTvFormat();
    if (tvFormat != 1 && tvFormat >= 1 && tvFormat < 3) {
        gc_native_display.rmode = &GXMpal480IntDf;
    } else {
        gc_native_display.rmode = &GXNtsc480IntDf;
    }

    mode = gc_native_display.rmode;
    xfbBytes = ((mode->fbWidth + 0xF) & 0xFFF0) * mode->xfbHeight;
    xfbHalf = xfbBytes * 2;
    raw = _mwMemMalloc(permanent_heap, _RwDlFifoSize + (xfbBytes * 4) + 0x1F, 5, 0,
                       0, 0);
    fifo = (void*)(((unsigned long)raw + 0x1F) & ~0x1Fu);
    _RwDl_FIFO_XFB = raw;
    _RwDlDefaultFifo = fifo;
    gc_native_display.fifo = fifo;
    DCInvalidateRange(fifo, _RwDlFifoSize);

    xfb1 = (unsigned char*)_RwDlDefaultFifo + _RwDlFifoSize;
    xfb2 = xfb1 + xfbHalf;
    _RwGCXFBDisp = xfb1;
    _RwGCXFB1 = xfb1;
    gc_native_display.xfbDisp = xfb1;
    _RwGCXFB2 = xfb2;
    gc_native_display.xfbCopy = xfb2;
    _RwGCXFBCopy = xfb2;
    DCFlushRange(xfb1, xfbHalf);
    DCFlushRange(gc_native_display.xfbCopy, xfbHalf);

    VISetBlack(1);
    VIFlush();
    VIWaitForRetrace();
    VIWaitForRetrace();

    GXInit(gc_native_display.fifo, 0x40000);
    VIConfigure(gc_native_display.rmode);

    mode = gc_native_display.rmode;
    GXSetScissor(0, 0, mode->fbWidth, mode->efbHeight);
    mode = gc_native_display.rmode;
    GXSetDispCopySrc(0, 0, mode->fbWidth, mode->efbHeight);

    mode = gc_native_display.rmode;
    if (mode->field_rendering != 0) {
        yscale = 1.0f;
    } else {
        yscale = GXGetYScaleFactor(mode->efbHeight, mode->xfbHeight);
    }
    copyHeight = GXSetDispCopyYScale(yscale);
    GXSetDispCopyDst(gc_native_display.rmode->fbWidth, copyHeight);
    mode = gc_native_display.rmode;
    GXSetCopyFilter(mode->aa, mode->sample_pattern, 1, mode->vfilter);

    pixFmt = 0;
    if (gc_native_display.rmode->aa != 0) {
        pixFmt = 2;
    }
    _RwDlPixelFormat = pixFmt;
    GXSetPixelFmt(pixFmt, 0);

    mode = gc_native_display.rmode;
    GXSetFieldMode(mode->field_rendering,
                   mode->xfbHeight < mode->viHeight);

    gc_release_renderpipe();

    mode = gc_native_display.rmode;
    screen_width = mode->fbWidth;
    screen_height = mode->efbHeight;

    if (VIGetDTVStatus() != 0) {
        CheckFor480PMode();
    }
}

static void CheckFor480PMode(void) {
    PADStatus pads[4];
    unsigned int want480p;
    unsigned char done;
    int i;

    want480p = 0;
    done = 0;
    uFrameBlastCount = 3;
    init_controller();
    PADReset(0xC0000000);

    while (done == 0) {
        done = 1;
        PADRead(pads);
        for (i = 0; i < 2; i++) {
            if (pads[i].err == PAD_ERR_NOT_READY || pads[i].err == PAD_ERR_TRANSFER) {
                done = 0;
            }
        }
    }

    for (i = 0; i < 2; i++) {
        if (pads[i].err == PAD_ERR_NONE &&
            (pads[i].button & PAD_BUTTON_B) == PAD_BUTTON_B) {
            want480p = 1;
        }
    }

    if ((OSGetResetCode() & 1) != 0) {
        want480p = 1;
    }
    if (OSGetProgressiveMode() == 1) {
        want480p = 1;
    }

    if (want480p == 0) {
        uFrameBlastCount = 0xF;
        return;
    }
    if (gc_prompt_for_480P(pads) != 0) {
        gcSetup480P();
        displayContinueMessage(
            pads,
            "The display mode has been switched to\n            Progressive Scan.\n    Press "
            "the A Button to continue.");
    } else {
        OSSetProgressiveMode(0);
        displayContinueMessage(
            pads, "  The display mode has been set to\n            Interlaced  Mode.\n    "
                  "Press the A Button to continue.");
    }
    uFrameBlastCount = 0xF;
}

/* TODO: [near miss] 99.92%; A-button edge CFG agrees; one XOR source-order row remains after staging checks. */
static void displayContinueMessage(PADStatus* pads, char* msg) {
    DragonTextPrompt prompt;
    char yesBuf[20];
    char noBuf[20];
    PADStatus prev[3];
    OSTime start;
    unsigned char done;
    unsigned char ready;
    int i;
    unsigned short btn;

    done = 0;
    ready = 0;
    strcpy(yesBuf, "");
    strcpy(noBuf, "");
    prompt.message = msg;
    prompt.yes_str = yesBuf;
    prompt.no_str = noBuf;
    start = OSGetTime();

    while (done == 0) {
        PADReset(0xC0000000);
        for (i = 0; i < 3; i++) {
            prev[i] = pads[i];
        }

        while (ready == 0) {
            ready = 1;
            PADRead(pads);
            for (i = 0; i < 2; i++) {
                if (pads[i].err == PAD_ERR_NOT_READY || pads[i].err == PAD_ERR_TRANSFER) {
                    ready = 0;
                }
            }
        }
        ready = 0;

        handle_reset_switch();
        for (i = 0; i < 2; i++) {
            if (pads[i].err == 0) {
                btn = pads[i].button;
                if ((btn & 1) == 1 || (btn & 2) == 2) {
                    break;
                }
                if ((unsigned short)(btn & 0x100 & (prev[i].button ^ btn)) != 0) {
                    done = 1;
                    break;
                }
            }
        }

        gc_grab_renderpipe();
        gc_native_display_render(display_dragon_with_text, &prompt);
        gc_release_renderpipe();

        if (timed_out_10s(start) != 0) {
            done = 1;
        }
    }
}

static void gcSetup480P(void) {
    int xfbBytes;
    int xfbHalf;
    void* raw;
    GXRenderModeObj* mode;
    unsigned char* xfb1;
    unsigned char* xfb2;
    float yscale;
    unsigned long copyHeight;
    int i;

    gc_grab_renderpipe();
    OSSetProgressiveMode(1);
    GXDrawDone();
    VISetBlack(1);
    VIFlush();
    VIWaitForRetrace();

    _mwMemFree(_RwDl_FIFO_XFB, 0, 0);
    gc_native_display.xfbDisp = 0;
    gc_native_display.xfbCopy = 0;
    gc_native_display.fifo = 0;
    if (pal_565 != 0) {
        _mwMemFree(pal_565, 0, 0);
        pal_565 = 0;
    }

    gc_native_display.rmode = &GXNtsc480ProgSoft;
    xfbBytes = ((gc_native_display.rmode->fbWidth + 0xF) & 0xFFF0) *
               gc_native_display.rmode->xfbHeight;
    xfbHalf = xfbBytes * 2;
    raw = _mwMemMalloc(permanent_heap, _RwDlFifoSize + (xfbBytes * 4) + 0x1F, 5, 0,
                       0, 0);
    _RwDl_FIFO_XFB = raw;
    _RwDlDefaultFifo = (void*)(((unsigned long)raw + 0x1F) & ~0x1Fu);
    gc_native_display.fifo = _RwDlDefaultFifo;
    DCInvalidateRange(_RwDlDefaultFifo, _RwDlFifoSize);

    xfb1 = (unsigned char*)_RwDlDefaultFifo + _RwDlFifoSize;
    xfb2 = xfb1 + xfbHalf;
    _RwGCXFBDisp = xfb1;
    _RwGCXFB1 = xfb1;
    gc_native_display.xfbDisp = xfb1;
    _RwGCXFB2 = xfb2;
    gc_native_display.xfbCopy = xfb2;
    _RwGCXFBCopy = xfb2;
    DCFlushRange(xfb1, xfbHalf);
    DCFlushRange(gc_native_display.xfbCopy, xfbHalf);

    VISetBlack(1);
    VIFlush();
    VIWaitForRetrace();
    VIWaitForRetrace();

    GXInit(gc_native_display.fifo, 0x40000);
    VIConfigure(gc_native_display.rmode);

    GXSetScissor(0, 0, gc_native_display.rmode->fbWidth,
                 gc_native_display.rmode->efbHeight);
    GXSetDispCopySrc(0, 0, gc_native_display.rmode->fbWidth,
                     gc_native_display.rmode->efbHeight);
    yscale = GXGetYScaleFactor(gc_native_display.rmode->efbHeight,
                              gc_native_display.rmode->xfbHeight);
    copyHeight = GXSetDispCopyYScale(yscale);
    mode = gc_native_display.rmode;
    GXSetDispCopyDst(mode->fbWidth, copyHeight);
    GXSetCopyFilter(gc_native_display.rmode->aa, gc_native_display.rmode->sample_pattern, 1,
                    gc_native_display.rmode->vfilter);

    if (gc_native_display.rmode->aa != 0) {
        GXSetPixelFmt(2, 0);
    } else {
        GXSetPixelFmt(0, 0);
    }

    GXSetFieldMode(
        gc_native_display.rmode->field_rendering,
        gc_native_display.rmode->xfbHeight < gc_native_display.rmode->viHeight);

    gc_release_renderpipe();

    screen_width = gc_native_display.rmode->fbWidth;
    screen_height = gc_native_display.rmode->efbHeight;

    VISetBlack(1);
    VIFlush();
    VIWaitForRetrace();
    VIWaitForRetrace();
    i = 0;
    do {
        VIWaitForRetrace();
        VIFlush();
        i++;
    } while (i < 0xC);
}

void pokeFilter(void* vfilter) {
    GXRenderModeObj* mode;

    mode = gc_native_display.rmode;
    GXSetCopyFilter(mode->aa, mode->sample_pattern, 1, vfilter);
}

static inline unsigned int changed_buttons(unsigned short previous, unsigned short current) {
    return previous ^ current;
}

static int gc_prompt_for_480P(PADStatus* pads) {
    DragonTextPrompt prompt;
    char msgBuf[84];
    char yesBuf[20];
    char noBuf[20];
    PADStatus prev[3];
    OSTime start;
    unsigned char yes;
    unsigned char done;
    unsigned char ready;
    int i;
    unsigned short btn;

    yes = 1;
    done = 0;
    ready = 0;
    strcpy(msgBuf, "Do you want to display the game\n    in Progressive Scan mode?");
    strcpy(yesBuf, "YES");
    strcpy(noBuf, "NO");
    prompt.message = msgBuf;
    prompt.yes_str = yesBuf;
    prompt.no_str = noBuf;
    start = OSGetTime();

    while (done == 0) {
        PADReset(0xC0000000);
        for (i = 0; i < 3; i++) {
            prev[i] = pads[i];
        }

        while (ready == 0) {
            ready = 1;
            PADRead(pads);
            for (i = 0; i < 2; i++) {
                if (pads[i].err == PAD_ERR_NOT_READY || pads[i].err == PAD_ERR_TRANSFER) {
                    ready = 0;
                }
            }
        }
        ready = 0;

        handle_reset_switch();
        for (i = 0; i < 2; i++) {
            if (pads[i].err == 0) {
                btn = pads[i].button;
                if ((btn & 1) == 1) {
                    yes = 1;
                    break;
                }
                if ((btn & 2) == 2) {
                    yes = 0;
                    break;
                }
                if ((unsigned short)((btn & 0x100) & changed_buttons(prev[i].button, btn)) != 0) {
                    done = 1;
                    break;
                }
            }
        }

        prompt.yes_hi = yes;
        gc_grab_renderpipe();
        gc_native_display_render(display_dragon_with_text, &prompt);
        gc_release_renderpipe();

        if (timed_out_10s(start) != 0) {
            done = 1;
        }
    }
    if (yes != 0) {
        return 1;
    }
    return 0;
}

int gc_prompt_for_refresh_rate(PADStatus* pads)
{
    DragonTextPrompt prompt;
    char msgBuf[84];
    char yesBuf[20];
    char noBuf[20];

    strcpy(msgBuf, "Run 60Hz?");
    strcpy(yesBuf, "YES");
    strcpy(noBuf, "NO");
    prompt.message = msgBuf;
    prompt.yes_str = yesBuf;
    prompt.no_str = noBuf;
    prompt.yes_hi = 1;
    gc_grab_renderpipe();
    gc_native_display_render(display_dragon_with_text, &prompt);
    gc_release_renderpipe();
    return 1;
}

/* TODO: [near miss] 98.21%; GXColor copy slots recovered; width-loop coloring, masks and y scheduling remain. */
static void display_dragon_with_text(void* arg) {
    DragonTextPrompt* prompt = arg;
    GXColor black;
    int w;
    int h;
    int msgW;
    int msgH;
    int yesNoH;
    int yesW;
    int noW;
    int y;
    unsigned long i;

    save_projection_matrix();
    set_2d_projection();

    black = Black;
    h = screen_height;
    w = screen_width;

    GXSetNumChans(1);
    GXSetChanCtrl(0, 0, 0, 0, 0, 0, 2);
    set_chan0_color(black);
    GXSetNumTexGens(0);
    GXSetNumTevStages(1);
    GXSetTevOrder(0, 0xFF, 0xFF, 4);
    GXSetTevOp(0, 4);
    GXClearVtxDesc();
    GXSetVtxDesc(9, 1);
    GXSetVtxAttrFmt(0, 9, 0, 3, 0);
    GXBegin(0x80, 0, 4);
    wgPipe[0] = 0;
    wgPipe[0] = 0;
    wgPipe[0] = w;
    wgPipe[0] = 0;
    wgPipe[0] = w;
    wgPipe[0] = h;
    wgPipe[0] = 0;
    wgPipe[0] = h;

    GXSetNumChans(0);
    GXSetNumTexGens(0);
    GXSetNumTevStages(1);
    GXSetTevOp(0, 3);
    GXSetTevOrder(0, 0, 0, 0xFF);
    display_image();

    if (OSGetFontEncode() == 1) {
        FontData = _mwMemMalloc(wave_heap, OS_FONT_SIZE_SJIS, 5, 0, 0, 0);
    } else {
        FontData = _mwMemMalloc(wave_heap, OS_FONT_SIZE_ANSI, 5, 0, 0, 0);
    }
    if (FontData == 0) {
        OSPanic("gcdisplay.c", 0x50D, "Ins. memory to load ROM font.");
    }
    if (OSInitFont(FontData) == 0) {
        OSPanic("gcdisplay.c", 0x511, "ROM font is available in boot ROM ver 0.8 or later.");
    }

    FontSize = FontData->cellWidth << 4;
    FontSpace = -0x10;

    msgW = font_string_width(prompt->message);
    render_text_without_clear(prompt->message, (screen_width - msgW) / 2, 0x15E);

    if (prompt->yes_hi != 0) {
        for (i = 0; i < strlen(prompt->no_str); i++) {
            prompt->no_str[i] |= 0x20;
        }
        for (i = 0; i < strlen(prompt->yes_str); i++) {
            prompt->yes_str[i] &= 0xDF;
        }
    } else {
        for (i = 0; i < strlen(prompt->yes_str); i++) {
            prompt->yes_str[i] |= 0x20;
        }
        for (i = 0; i < strlen(prompt->no_str); i++) {
            prompt->no_str[i] &= 0xDF;
        }
    }

    msgH = font_string_height(prompt->message);
    yesNoH = font_string_height("YESNO");
    y = msgH + yesNoH / 2;
    yesW = font_string_width(prompt->yes_str);
    y += 0x15E;
    render_text_without_clear(prompt->yes_str, (screen_width / 2 - 100) - yesW / 2, y);

    noW = font_string_width(prompt->no_str);
    render_text_without_clear(prompt->no_str, (screen_width / 2 + 100) - noW / 2, y);

    if (FontData != 0) {
        _mwMemFree(FontData, 0, 0);
        FontData = 0;
    }
    restore_projection_matrix();
}

static void display_image(void) {
    int i;
    unsigned short* palette;
    unsigned short src;
    short left;
    int right;
    GXTlutObj tlut;
    GXTexObj tex;
    Mtx texMtx;

    if (pal_565 == 0) {
        palette = _mwMemMalloc(permanent_heap, sizeof(loading_palette), 5, 0, 0, 0);
        if (palette == 0) {
            return;
        }
        pal_565 = palette;
        for (i = 0; i < 0x100; i++) {
            src = loading_palette[i];
            pal_565[i] = ((src * 2) & 0x07C0) | ((src << 11) & 0xF800) |
                         ((src >> 10) & 0x001F);
        }
    }

    DCFlushRange(loading_image, sizeof(loading_image));
    DCFlushRange(pal_565, sizeof(loading_palette));
    GXInitTlutObj(&tlut, pal_565, 1, 0x100);
    GXLoadTlut(&tlut, 0);
    GXInitTexObjCI(&tex, loading_image, 0x100, 0x100, 9, 0, 0, 0, 0);
    GXInitTexObjLOD(&tex, 1, 1, 0.0f, 0.0f, 0.0f, 0, 0, 0);
    GXLoadTexObj(&tex, 1);
    PSMTXScale(texMtx, 0.00390625f, 0.00390625f, 1.0f);
    GXLoadTexMtxImm(texMtx, 0x21, 1);
    GXSetNumTexGens(1);
    GXSetNumTevStages(1);
    GXSetTexCoordGen2(0, 1, 4, 0x21, 0, 0x7D);
    GXSetTevOrder(0, 0, 1, 0xFF);

    left = screen_width / 2 - 0x80;

    GXClearVtxDesc();
    GXSetVtxDesc(9, 1);
    GXSetVtxDesc(0xD, 1);
    GXSetVtxAttrFmt(0, 9, 0, 3, 0);
    GXSetVtxAttrFmt(0, 0xD, 1, 3, 0);
    GXBegin(0x80, 0, 4);
    wgPipe[0] = left;
    right = left;
    right += 0x100;
    wgPipe[0] = 0x70;
    wgPipe[0] = 0;
    wgPipe[0] = 0;
    wgPipe[0] = right;
    wgPipe[0] = 0x70;
    wgPipe[0] = 0x100;
    wgPipe[0] = 0;
    wgPipe[0] = right;
    wgPipe[0] = 0x170;
    wgPipe[0] = 0x100;
    wgPipe[0] = 0x100;
    wgPipe[0] = left;
    wgPipe[0] = 0x170;
    wgPipe[0] = 0;
    wgPipe[0] = 0x100;
}

/* TODO: [near miss] 96.43%; 8x4 tile traversal and instruction order agree;
 * 27 localized GPR operands remain. */
void tile_image(unsigned char* dest) {
    unsigned char* tmp;
    const unsigned char* image;
    const unsigned char* tile_source;
    unsigned char* output;
    int tile;
    int output_offset;
    int pixel;
    int tile_x;
    int tile_y;

    tmp = _mwMemMalloc(wave_heap, sizeof(loading_image), 5, 0, 0, 0);
    image = loading_image;
    output_offset = 0;
    for (tile = 0; tile < 0x800; tile++) {
        tile_x = (tile % 32) * 8;
        tile_y = (tile / 32) * 4;
        tile_source = image + tile_x;
        output = tmp + output_offset;
        for (pixel = 0; pixel < 0x20; pixel++) {
            int y = tile_y + (pixel / 8);
            *output++ = tile_source[y * 0x100 + (pixel % 8)];
        }
        output_offset += 0x20;
    }
    memcpy(dest, tmp, sizeof(loading_image));
    _mwMemFree(tmp, 0, 0);
}

int is_progressive_scan_mode(void) {
    return progscan_mode;
}
