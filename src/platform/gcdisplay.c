#include "platform/gcdisplay.h"

#include "dolphin/cache.h"
#include "dolphin/gx.h"
#include "dolphin/os.h"
#include "dolphin/vi.h"
#include "mw/mwMemHeap.h"
#include "platform/gcutils.h"
#include "runtime/mk_proc.h"
#include "runtime/mk_struct.h"

/*
 * gcdisplay.o - native display + loading dragon image.
 * Soft ceilings (present path): init TV-branch, render mtctr copy, pass_to_RW
 * coloring, display_image rlwimi, tile_image /%; 480p UI / feedback omitted.
 * Function order matches retail ASM.
 */

typedef struct GcNativeDisplay {
    GXRenderModeObj* rmode; /* +0x00 */
    void* fifo;             /* +0x04 */
    void* xfbDisp;          /* +0x08 */
    void* xfbCopy;          /* +0x0C */
} GcNativeDisplay;

/* Partial OSFontHeader - offsets used by romfont / dragon text. */
typedef struct OSFontHeader {
    char pad00[8];
    unsigned short ascent;  /* +0x08 */
    unsigned short descent; /* +0x0A */
    char pad0C[2];
    unsigned short leading; /* +0x0E */
    unsigned short width;   /* +0x10 */
    short maxCharWidth;     /* +0x12 */
    char pad14[4];
    unsigned short format; /* +0x18 */
    char pad1A[4];
    unsigned short sheetWidth;  /* +0x1E */
    unsigned short sheetHeight; /* +0x20 */
} OSFontHeader;

typedef void (*NativeRenderCb)(void* arg);

typedef struct RwEngineInstanceType {
    char pad00[0x20];
    int (*fpRenderStateSet)(int state, int value); /* +0x20 */
} RwEngineInstanceType;

void* memcpy(void* dest, const void* src, unsigned long n);
unsigned long strlen(const char* s);


void RwGameCubeGetXFBs(void** disp, void** copy);


void save_projection_matrix(void);
void set_2d_projection(void);
void restore_projection_matrix(void);

char* strcpy(char* dest, const char* src);
void PADReset(unsigned int mask);
void PADRead(void* status);
int init_controller(void);
void* get_mkx_mem(void* userdata);

/* Dolphin PADStatus - 12 bytes (err @ +0x0A). */
typedef struct PADStatus {
    unsigned short button;
    signed char stickX;
    signed char stickY;
    signed char substickX;
    signed char substickY;
    unsigned char triggerLeft;
    unsigned char triggerRight;
    unsigned char analogA;
    unsigned char analogB;
    signed char err;
} PADStatus;

extern RwEngineInstanceType* RwEngineInstance;
extern int feedback_blendrate;
extern int use_feedback_effect;
extern int old_use_feedback_effect;
extern GXRenderModeObj GXNtsc480ProgSoft;

void GProfile_GCN_GxDrawDone(void);

extern int screen_width;
extern int screen_height;
extern unsigned long _RwDlFifoSize;
extern void* _RwDl_FIFO_XFB;
extern void* _RwDlDefaultFifo;
extern void* _RwGCXFB1;
extern void* _RwGCXFB2;
extern void* _RwGCXFBCopy;
extern void* _RwGCXFBDisp;
extern int _RwDlPixelFormat;


/* ASM-backed .data while NonMatching. */
extern unsigned short loading_palette[0x100];
extern unsigned char loading_image[0x10000];

/* .bss / .sdata / .sbss (this TU) */
static GXTexObj feedbackTex;
static GcNativeDisplay gc_native_display;
static GXColor Black = {0, 0, 0, 0xFF};
static int uFrameBlastCount = 0xF;
static int progscan_mode;
static unsigned short* pal_565;
void* feedbackTexPixels;
static short FontSpace;
static short FontSize;
static OSFontHeader* FontData;
static void* LastSheet;

/* Identity-ish pos mtx (retail @682). */
static const float s_identityPosMtx[12] = {
    1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f,
};

static volatile unsigned short* const wgPipe = (volatile unsigned short*)GXFIFO_ADDR;

static void gc_native_display_render(NativeRenderCb cb, void* arg);
static void render_text(void* text);
static void render_text_without_clear(char* text, int x, int y);
static void render_image(void* unused);
static void CheckFor480PMode(void);
static void displayContinueMessage(PADStatus* pads, char* msg);
static void gcSetup480P(void);
static int gc_prompt_for_480P(PADStatus* pads);

/* 10s timeout using bus clock / 4 (TB freq) - retail OSGetTime compare. */
static int timed_out_10s(unsigned long long start) {
    unsigned long long now;
    unsigned long long diff;
    unsigned long limit;

    now = OSGetTime();
    diff = now - start;
    limit = ((*(unsigned long*)0x800000F8) >> 2) * 10;
    return diff > (unsigned long long)limit;
}

static int font_string_width(char* s) {
    int maxW;
    int lineW;
    int charW;
    OSFontHeader* font;

    font = FontData;
    if (font == 0) {
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
        lineW += FontSpace + (FontSize * charW) / (int)font->width;
    }
    if (maxW < lineW) {
        maxW = lineW;
    }
    return (maxW + 0xF) >> 4;
}

static int font_string_height(char* s) {
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
    return (lines * ((int)font->leading * (int)FontSize) / (int)font->width + 0xF) >> 4;
}

/* ========================================================================= */
/* Retail function order                                                     */
/* ========================================================================= */

void feedback_effect(void) {
    Mtx posMtx;
    Mtx texMtx;
    GXColor color;
    GXColor amb;
    GXColor mat;
    short w;
    short h;

    memcpy(posMtx, s_identityPosMtx, sizeof(s_identityPosMtx));
    color.r = 0xC4;
    color.g = 0xC4;
    color.b = 0xC4;
    color.a = 0xFF;

    RwEngineInstance->fpRenderStateSet(0x14, 1);
    RwEngineInstance->fpRenderStateSet(6, 0);
    RwEngineInstance->fpRenderStateSet(0xA, 5);
    RwEngineInstance->fpRenderStateSet(0xB, 2);

    save_projection_matrix();
    set_2d_projection();
    GXLoadPosMtxImm(posMtx, 0);
    GXLoadTexObj(&feedbackTex, 4);
    PSMTXScale(texMtx, 1.0f / (float)screen_width, 1.0f / (float)screen_height, 0.0f);
    GXLoadTexMtxImm(texMtx, 0x21, 1);
    GXSetNumTexGens(1);
    GXSetNumTevStages(1);
    GXSetNumChans(1);

    amb = color;
    GXSetChanAmbColor(4, amb);
    color.a = (unsigned char)feedback_blendrate;
    mat = color;
    GXSetChanMatColor(4, mat);
    GXSetChanCtrl(4, 0, 0, 0, 0, 0, 2);
    GXSetTexCoordGen2(0, 1, 4, 0x21, 0, 0x7D);
    GXSetTevOrder(0, 0, 4, 4);
    GXSetTevOp(0, 0);

    if (old_use_feedback_effect == use_feedback_effect) {
        w = (short)screen_width;
        h = (short)screen_height;
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
        wgPipe[0] = (unsigned short)w;
        wgPipe[0] = 0;
        wgPipe[0] = (unsigned short)screen_width;
        wgPipe[0] = 0;
        wgPipe[0] = (unsigned short)w;
        wgPipe[0] = (unsigned short)h;
        wgPipe[0] = (unsigned short)screen_width;
        wgPipe[0] = (unsigned short)screen_height;
        wgPipe[0] = 0;
        wgPipe[0] = (unsigned short)h;
        wgPipe[0] = 0;
        wgPipe[0] = (unsigned short)screen_height;
    }

    GXSetTexCopySrc(0, 0, (unsigned short)screen_width, (unsigned short)screen_height);
    GXSetTexCopyDst((unsigned short)screen_width, (unsigned short)screen_height, 4, 0);
    GXCopyTex(feedbackTexPixels, 0);
    restore_projection_matrix();

    RwEngineInstance->fpRenderStateSet(6, 1);
    RwEngineInstance->fpRenderStateSet(0x14, 2);
    RwEngineInstance->fpRenderStateSet(0xA, 5);
    RwEngineInstance->fpRenderStateSet(0xB, 6);
}

void gc_setup_feedback_buffer_for_konquest(void) {
    feedbackTexPixels =
        _mwMemMalloc(permanent_heap, (unsigned long)(screen_width * screen_height * 2), 5, 0, 0, 0);
    mk_insert((MkHdr*)get_mkx_mem(feedbackTexPixels), &aproc->pdata_list_b);
    if (feedbackTexPixels != 0) {
        GXInitTexObj(&feedbackTex, feedbackTexPixels, (unsigned short)screen_width,
                     (unsigned short)screen_height, 4, 0, 0, 0);
        GXInitTexObjLOD(&feedbackTex, 1, 1, 0.0f, 0.0f, 0.0f, 0, 0, 0);
    }
}

void setup_post_effect_buffers(void) {
}

int romfont_puts(short x, int y, char* text) {
    int penX;
    int sheetX;
    int sheetY;
    int charW;
    void* sheet;
    short x0;
    short x1;
    short y0;
    short y1;
    short u0;
    short u1;
    short v0;
    short v1;
    unsigned short cellW;
    OSFontHeader* font;
    GXTexObj texObj;
    Mtx texMtx;
    float invW;
    float invH;

    LastSheet = 0;
    GXClearVtxDesc();
    GXSetVtxDesc(9, 1);
    GXSetVtxDesc(0xD, 1);
    GXSetVtxAttrFmt(0, 9, 0, 3, 4);
    GXSetVtxAttrFmt(0, 0xD, 1, 3, 0);

    y <<= 4;
    penX = 0;
    font = FontData;

    while (*text != '\0') {
        if (*text == '\n') {
            penX = 0;
            text++;
            y += ((int)font->leading * (int)FontSize) / (int)font->width;
            continue;
        }

        text = OSGetFontTexture(text, &sheet, &sheetX, &sheetY, &charW);
        if (LastSheet != sheet) {
            LastSheet = sheet;
            GXInitTexObj(&texObj, sheet, font->sheetWidth, font->sheetHeight, font->format, 0, 0,
                         0);
            GXInitTexObjLOD(&texObj, 1, 1, 0.0f, 0.0f, 0.0f, 0, 0, 0);
            GXLoadTexObj(&texObj, 0);
            invW = 1.0f / (float)font->sheetWidth;
            invH = 1.0f / (float)font->sheetHeight;
            PSMTXScale(texMtx, invW, invH, 1.0f);
            GXLoadTexMtxImm(texMtx, 0x1E, 1);
            GXSetNumTexGens(1);
            GXSetTexCoordGen2(0, 1, 4, 0x1E, 0, 0x7D);
        }

        cellW = font->width;
        x0 = (short)(x * 0x10 + penX);
        x1 = (short)(x0 + FontSize);
        u0 = (short)sheetX;
        u1 = (short)(sheetX + (short)cellW);
        v0 = (short)sheetY;
        v1 = (short)(sheetY + font->maxCharWidth);
        y0 = (short)(y - ((int)font->ascent * (int)FontSize) / (int)cellW);
        y1 = (short)(y + ((int)font->descent * (int)FontSize) / (int)cellW);

        GXBegin(0x80, 0, 4);
        wgPipe[0] = (unsigned short)x0;
        wgPipe[0] = (unsigned short)y0;
        wgPipe[0] = (unsigned short)u0;
        wgPipe[0] = (unsigned short)v0;
        wgPipe[0] = (unsigned short)x1;
        wgPipe[0] = (unsigned short)y0;
        wgPipe[0] = (unsigned short)u1;
        wgPipe[0] = (unsigned short)v0;
        wgPipe[0] = (unsigned short)x1;
        wgPipe[0] = (unsigned short)y1;
        wgPipe[0] = (unsigned short)u1;
        wgPipe[0] = (unsigned short)v1;
        wgPipe[0] = (unsigned short)x0;
        wgPipe[0] = (unsigned short)y1;
        wgPipe[0] = (unsigned short)u0;
        wgPipe[0] = (unsigned short)v1;

        penX += FontSpace + (FontSize * charW) / (int)font->width;
    }

    return (penX + 0xF) >> 4;
}

void gc_native_display_render_text(char* text) {
    if (OSGetFontEncode() == 1) {
        FontData = _mwMemMalloc(wave_heap, 0x120F00, 5, 0, 0, 0);
    } else {
        FontData = _mwMemMalloc(wave_heap, 0x20120, 5, 0, 0, 0);
    }
    if (FontData == 0) {
        OSPanic("gcdisplay.c", 0x50D, "Ins. memory to load ROM font.");
    }
    if (OSInitFont(FontData) == 0) {
        OSPanic("gcdisplay.c", 0x511, "ROM font is available in boot ROM ver 0.8 or later.");
    }
    FontSize = (short)(FontData->width << 4);
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

void gc_native_display_render_movie(void* ctx) {
    Mtx posMtx;
    GXColor clearColor;
    GXColor fogColor;

    memcpy(posMtx, s_identityPosMtx, sizeof(s_identityPosMtx));
    clearColor = Black;

    GXSetCoPlanar(0);
    GXSetCullMode(0);
    GXSetClipMode(0);
    GXSetScissor(0, 0, gc_native_display.rmode->fbWidth, gc_native_display.rmode->efbHeight);
    GXSetScissorBoxOffset(0, 0);
    GXSetNumIndStages(0);
    fogColor = clearColor;
    GXSetFog(0, 0.0f, 1.0f, 0.1f, 1.0f, fogColor);
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
    clearColor = Black;
    GXSetCopyClear(clearColor, 0x00FFFFFFu);
    GProfile_GCN_GxDrawDone();
    VIWaitForRetrace();
    VIFlush();
    GXCopyDisp(_RwGCXFBDisp, 1);
}

#pragma opt_unroll_loops off
#pragma ppc_unroll_instructions_limit 1
/* Soft ceiling: ~92.8% -- retail mtctr lwzu/stwu @682 copy vs dword-pair do/while; stop. */
static void gc_native_display_render(NativeRenderCb cb, void* arg) {
    Mtx posMtx;
    GXColor clearColor;
    GXColor fogColor;
    void* xfbA;
    void* xfbB;
    void* curXfb;
    int first;
    int i;
    GXRenderModeObj* mode;
    const unsigned int* src;
    unsigned int* dst;
    int n;

    /* Retail: spill Black (@681) then mtctr dword-pair copy of @682 into posMtx. */
    clearColor = Black;
    src = (const unsigned int*)s_identityPosMtx;
    dst = (unsigned int*)posMtx;
    n = 6;
    do {
        dst[0] = src[0];
        dst[1] = src[1];
        src += 2;
        dst += 2;
        n -= 1;
    } while (n != 0);

    first = 1;

    if (gc_native_display.xfbDisp != 0) {
        xfbA = gc_native_display.xfbDisp;
        xfbB = gc_native_display.xfbCopy;
    } else {
        RwGameCubeGetXFBs(&xfbA, &xfbB);
    }

    GXSetCoPlanar(0);
    GXSetCullMode(2);
    GXSetClipMode(0);
    mode = gc_native_display.rmode;
    GXSetScissor(0, 0, mode->fbWidth, mode->efbHeight);
    GXSetScissorBoxOffset(0, 0);
    GXSetNumIndStages(0);

    fogColor = clearColor;
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

    clearColor = Black;
    GXSetCopyClear(clearColor, 0x00FFFFFFu);

    curXfb = xfbA;
    for (i = 0; i < uFrameBlastCount; i++) {
        mode = gc_native_display.rmode;
        if (mode->field_rendering != 0) {
            GXSetViewportJitter(0.0f, 0.0f, (float)mode->fbWidth, (float)mode->efbHeight, 0.0f,
                                1.0f, (unsigned long)VIGetNextField());
        } else {
            GXSetViewport(0.0f, 0.0f, (float)mode->fbWidth, (float)mode->efbHeight, 0.0f, 1.0f);
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

        if (curXfb == xfbA) {
            curXfb = xfbB;
        } else {
            curXfb = xfbA;
        }
    }
}
#pragma ppc_unroll_instructions_limit 40
#pragma opt_unroll_loops reset

static void render_text(void* text) {
    char* s;
    char* walk;
    GXColor black;
    GXColor mat;
    GXColor amb;
    int w;
    int maxW;
    int lineW;
    int charW;
    int lines;
    int msgW;
    int msgH;
    int x;
    int y;
    OSFontHeader* font;

    s = text;
    save_projection_matrix();
    set_2d_projection();

    black = Black;
    w = screen_width;

    GXSetNumChans(1);
    GXSetChanCtrl(0, 0, 0, 0, 0, 0, 2);
    mat = black;
    GXSetChanMatColor(0, mat);
    amb = black;
    GXSetChanAmbColor(0, amb);
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
    wgPipe[0] = (unsigned short)w;
    wgPipe[0] = 0;
    wgPipe[0] = (unsigned short)w;
    wgPipe[0] = 0x1E0;
    wgPipe[0] = 0;
    wgPipe[0] = 0x1E0;

    GXSetNumChans(0);
    GXSetNumTexGens(0);
    GXSetNumTevStages(1);
    GXSetTevOp(0, 3);
    GXSetTevOrder(0, 0, 0, 0xFF);

    font = FontData;
    if (font == 0) {
        msgW = 0;
    } else {
        maxW = 0;
        lineW = 0;
        walk = s;
        while (*walk != '\0') {
            if (*walk == '\n') {
                if (maxW < lineW) {
                    maxW = lineW;
                }
                lineW = 0;
            }
            walk = OSGetFontWidth(walk, &charW);
            lineW += FontSpace + (FontSize * charW) / (int)font->width;
        }
        if (maxW < lineW) {
            maxW = lineW;
        }
        msgW = (maxW + 0xF) >> 4;
    }
    x = (screen_width - msgW) >> 1;

    font = FontData;
    if (font == 0) {
        msgH = 0;
    } else {
        lines = 1;
        for (walk = s; *walk != '\0'; walk++) {
            if (*walk == '\n') {
                lines++;
            }
        }
        msgH = (lines * ((int)font->leading * (int)FontSize) / (int)font->width + 0xF) >> 4;
    }
    y = (screen_height - msgH) >> 1;
    romfont_puts((short)x, y, s);
    restore_projection_matrix();
}

#pragma dont_inline on
static void render_text_without_clear(char* text, int x, int y) {
    char* walk;
    GXColor color;

    walk = text;
    if (FontData != 0) {
        for (; *walk != '\0'; walk++) {
        }
    }

    color.r = 0xFF;
    color.g = 0xFF;
    color.b = 0xFF;
    color.a = 0xFF;

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
    romfont_puts((short)x, y, text);
}
#pragma dont_inline reset

static void render_image(void* unused) {
    GXColor black;
    GXColor amb;
    GXColor mat;
    int w;
    int h;

    save_projection_matrix();
    set_2d_projection();

    /* A1: amb before mat -> mat@0x8 amb@0xc; Black then h@r30 w@r31. */
    black = Black;
    h = screen_height;
    w = screen_width;

    GXSetNumChans(1);
    GXSetChanCtrl(0, 0, 0, 0, 0, 0, 2);
    mat = black;
    GXSetChanMatColor(0, mat);
    amb = black;
    GXSetChanAmbColor(0, amb);
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
    wgPipe[0] = (unsigned short)w;
    wgPipe[0] = 0;
    wgPipe[0] = (unsigned short)w;
    wgPipe[0] = (unsigned short)h;
    wgPipe[0] = 0;
    wgPipe[0] = (unsigned short)h;

    GXSetNumChans(0);
    GXSetNumTexGens(0);
    GXSetNumTevStages(1);
    GXSetTevOp(0, 3);
    GXSetTevOrder(0, 0, 0, 0xFF);
    display_image();
    restore_projection_matrix();
}

void gc_native_display_pass_to_RW(void) {
    GcNativeDisplay* d;
    void* xfbDisp;
    void* fifo;
    void* xfbCopy;

    GXDrawDone();
    if (pal_565 != 0) {
        _mwMemFree(pal_565, 0, 0);
        pal_565 = 0;
    }
    /*
     * Soft ceiling: 98.25% -- operations, field offsets, and store order match;
     * only the base and loaded-value register allocation differs.
     */
    d = &gc_native_display;
    xfbDisp = d->xfbDisp;
    fifo = d->fifo;
    xfbCopy = d->xfbCopy;
    _RwGCXFBDisp = xfbDisp;
    _RwGCXFBCopy = xfbCopy;
    _RwDlDefaultFifo = fifo;
}

/*
 * Soft ceiling: 98.61% -- the retail TV-format selection has one additional
 * equivalent branch; the remaining differences are truncation/load scheduling.
 */
void gc_native_display_init(void) {
    int tvFormat;
    GXRenderModeObj* mode;
    int xfbBytes;
    int xfbHalf;
    void* raw;
    void* fifo;
    void* xfb1;
    void* xfb2;
    float yscale;
    unsigned short copyHeight;
    int pixFmt;

    gc_grab_renderpipe();
    VIInit();
    tvFormat = VIGetTvFormat();
    if (tvFormat == 1 || tvFormat < 1 || tvFormat >= 3) {
        gc_native_display.rmode = &GXNtsc480IntDf;
    } else {
        gc_native_display.rmode = &GXMpal480IntDf;
    }

    mode = gc_native_display.rmode;
    xfbBytes = (((int)mode->fbWidth + 0xF) & 0xFFF0) * (int)mode->xfbHeight;
    xfbHalf = xfbBytes * 2;
    raw = _mwMemMalloc(permanent_heap, _RwDlFifoSize + (unsigned long)(xfbBytes * 4) + 0x1F, 5, 0,
                       0, 0);
    fifo = (void*)(((unsigned long)raw + 0x1F) & ~0x1Fu);
    _RwDl_FIFO_XFB = raw;
    _RwDlDefaultFifo = fifo;
    gc_native_display.fifo = fifo;
    DCInvalidateRange(fifo, _RwDlFifoSize);

    xfb1 = (void*)((unsigned char*)_RwDlDefaultFifo + _RwDlFifoSize);
    xfb2 = (void*)((unsigned char*)xfb1 + xfbHalf);
    _RwGCXFBDisp = xfb1;
    _RwGCXFB1 = xfb1;
    gc_native_display.xfbDisp = xfb1;
    _RwGCXFB2 = xfb2;
    gc_native_display.xfbCopy = xfb2;
    _RwGCXFBCopy = xfb2;
    DCFlushRange(xfb1, (unsigned long)xfbHalf);
    DCFlushRange(gc_native_display.xfbCopy, (unsigned long)xfbHalf);

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
    mode = gc_native_display.rmode;
    GXSetDispCopyDst(mode->fbWidth, copyHeight);
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
                   (unsigned char)(((unsigned int)mode->xfbHeight - (unsigned int)mode->viHeight) >>
                                   31));

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
    int done;
    int want480p;
    int i;
    int ready;

    want480p = 0;
    done = 0;
    uFrameBlastCount = 3;
    init_controller();
    PADReset(0xC0000000);

    while (done == 0) {
        done = 1;
        PADRead(pads);
        i = 0;
        ready = 2;
        do {
            if (pads[i].err == -2 || pads[i].err == -3) {
                done = 0;
            }
            i++;
            ready--;
        } while (ready != 0);
    }

    i = 0;
    ready = 2;
    do {
        if (pads[i].err == 0 && (pads[i].button & 0x200) == 0x200) {
            want480p = 1;
        }
        i++;
        ready--;
    } while (ready != 0);

    if ((OSGetResetCode() & 1) != 0) {
        want480p = 1;
    }
    if (OSGetProgressiveMode() == 1) {
        want480p = 1;
    }

    if (want480p != 0) {
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
    }
    uFrameBlastCount = 0xF;
}

static void displayContinueMessage(PADStatus* pads, char* msg) {
    DragonTextPrompt prompt;
    char yesBuf[20];
    char noBuf[20];
    PADStatus prev[3];
    unsigned long long start;
    int done;
    int ready;
    int i;
    unsigned short btn;

    done = 0;
    strcpy(yesBuf, "");
    strcpy(noBuf, "");
    prompt.message = msg;
    prompt.yes_str = yesBuf;
    prompt.no_str = noBuf;
    start = OSGetTime();

    while (done == 0) {
        ready = 0;
        PADReset(0xC0000000);
        i = 0;
        do {
            prev[i] = pads[i];
            i++;
        } while (i < 3);

        while (ready == 0) {
            ready = 1;
            PADRead(pads);
            i = 0;
            do {
                if (pads[i].err == -2 || pads[i].err == -3) {
                    ready = 0;
                }
                i++;
            } while (i < 2);
        }

        handle_reset_switch();
        i = 0;
        do {
            if (pads[i].err == 0) {
                btn = pads[i].button;
                if ((btn & 1) == 1 || (btn & 2) == 2) {
                    break;
                }
                if ((btn & 0x100 & (prev[i].button ^ btn)) != 0) {
                    done = 1;
                    break;
                }
            }
            i++;
        } while (i < 2);

        gc_grab_renderpipe();
        gc_native_display_render((NativeRenderCb)display_dragon_with_text, &prompt);
        gc_release_renderpipe();

        if (timed_out_10s(start) != 0) {
            done = 1;
        }
    }
}

static void gcSetup480P(void) {
    GXRenderModeObj* mode;
    int xfbBytes;
    int xfbHalf;
    void* raw;
    void* fifo;
    void* xfb1;
    void* xfb2;
    float yscale;
    unsigned short copyHeight;
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
    mode = gc_native_display.rmode;
    xfbBytes = (((int)mode->fbWidth + 0xF) & 0xFFF0) * (int)mode->xfbHeight;
    xfbHalf = xfbBytes * 2;
    raw = _mwMemMalloc(permanent_heap, _RwDlFifoSize + (unsigned long)(xfbBytes * 4) + 0x1F, 5, 0,
                       0, 0);
    fifo = (void*)(((unsigned long)raw + 0x1F) & ~0x1Fu);
    _RwDl_FIFO_XFB = raw;
    _RwDlDefaultFifo = fifo;
    gc_native_display.fifo = fifo;
    DCInvalidateRange(fifo, _RwDlFifoSize);

    xfb1 = (void*)((unsigned char*)fifo + _RwDlFifoSize);
    xfb2 = (void*)((unsigned char*)xfb1 + xfbHalf);
    _RwGCXFBDisp = xfb1;
    _RwGCXFB1 = xfb1;
    gc_native_display.xfbDisp = xfb1;
    _RwGCXFB2 = xfb2;
    gc_native_display.xfbCopy = xfb2;
    _RwGCXFBCopy = xfb2;
    DCFlushRange(xfb1, (unsigned long)xfbHalf);
    DCFlushRange(xfb2, (unsigned long)xfbHalf);

    VISetBlack(1);
    VIFlush();
    VIWaitForRetrace();
    VIWaitForRetrace();

    GXInit(gc_native_display.fifo, 0x40000);
    VIConfigure(gc_native_display.rmode);

    mode = gc_native_display.rmode;
    GXSetScissor(0, 0, mode->fbWidth, mode->efbHeight);
    GXSetDispCopySrc(0, 0, mode->fbWidth, mode->efbHeight);
    yscale = GXGetYScaleFactor(mode->efbHeight, mode->xfbHeight);
    copyHeight = GXSetDispCopyYScale(yscale);
    mode = gc_native_display.rmode;
    GXSetDispCopyDst(mode->fbWidth, copyHeight);
    mode = gc_native_display.rmode;
    GXSetCopyFilter(mode->aa, mode->sample_pattern, 1, mode->vfilter);

    if (gc_native_display.rmode->aa == 0) {
        GXSetPixelFmt(0, 0);
    } else {
        GXSetPixelFmt(2, 0);
    }

    mode = gc_native_display.rmode;
    GXSetFieldMode(mode->field_rendering,
                   (unsigned char)(((unsigned int)mode->xfbHeight - (unsigned int)mode->viHeight) >>
                                   31));

    gc_release_renderpipe();

    mode = gc_native_display.rmode;
    screen_width = mode->fbWidth;
    screen_height = mode->efbHeight;

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

static int gc_prompt_for_480P(PADStatus* pads) {
    DragonTextPrompt prompt;
    char msgBuf[84];
    char yesBuf[20];
    char noBuf[20];
    PADStatus prev[3];
    unsigned long long start;
    int yes;
    int done;
    int ready;
    int i;
    unsigned short btn;

    yes = 1;
    done = 0;
    strcpy(msgBuf, "Do you want to display the game\n    in Progressive Scan mode?");
    strcpy(yesBuf, "YES");
    strcpy(noBuf, "NO");
    prompt.message = msgBuf;
    prompt.yes_str = yesBuf;
    prompt.no_str = noBuf;
    start = OSGetTime();

    while (done == 0) {
        ready = 0;
        PADReset(0xC0000000);
        i = 0;
        do {
            prev[i] = pads[i];
            i++;
        } while (i < 3);

        while (ready == 0) {
            ready = 1;
            PADRead(pads);
            i = 0;
            do {
                if (pads[i].err == -2 || pads[i].err == -3) {
                    ready = 0;
                }
                i++;
            } while (i < 2);
        }

        handle_reset_switch();
        i = 0;
        do {
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
                if ((btn & 0x100 & (prev[i].button ^ btn)) != 0) {
                    done = 1;
                    break;
                }
            }
            i++;
        } while (i < 2);

        prompt.yes_hi = (unsigned char)yes;
        gc_grab_renderpipe();
        gc_native_display_render((NativeRenderCb)display_dragon_with_text, &prompt);
        gc_release_renderpipe();

        if (timed_out_10s(start) != 0) {
            done = 1;
        }
    }
    return yes;
}

void pokeFilter(void* vfilter) {
    GXRenderModeObj* mode;

    mode = gc_native_display.rmode;
    GXSetCopyFilter(mode->aa, mode->sample_pattern, 1, (unsigned char*)vfilter);
}

void display_dragon_with_text(DragonTextPrompt* prompt) {
    GXColor black;
    int w;
    int h;
    unsigned long fontSize;
    int msgW;
    int msgH;
    int yesNoH;
    int yesW;
    int noW;
    int y;
    unsigned long i;
    unsigned long len;

    save_projection_matrix();
    set_2d_projection();

    black = Black;
    h = screen_height;
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
    wgPipe[0] = (unsigned short)w;
    wgPipe[0] = 0;
    wgPipe[0] = (unsigned short)w;
    wgPipe[0] = (unsigned short)h;
    wgPipe[0] = 0;
    wgPipe[0] = (unsigned short)h;

    GXSetNumChans(0);
    GXSetNumTexGens(0);
    GXSetNumTevStages(1);
    GXSetTevOp(0, 3);
    GXSetTevOrder(0, 0, 0, 0xFF);
    display_image();

    if (OSGetFontEncode() == 1) {
        fontSize = 0x120F00;
    } else {
        fontSize = 0x20120;
    }
    FontData = _mwMemMalloc(wave_heap, fontSize, 5, 0, 0, 0);
    if (FontData == 0) {
        OSPanic("gcdisplay.c", 0x50D, "Ins. memory to load ROM font.");
    }
    if (OSInitFont(FontData) == 0) {
        OSPanic("gcdisplay.c", 0x511, "ROM font is available in boot ROM ver 0.8 or later.");
    }

    FontSize = (short)(FontData->width << 4);
    FontSpace = -0x10;

    msgW = font_string_width(prompt->message);
    render_text_without_clear(prompt->message, (screen_width - msgW) >> 1, 0x15E);

    if (prompt->yes_hi == 0) {
        len = strlen(prompt->yes_str);
        for (i = 0; i < len; i++) {
            prompt->yes_str[i] = (char)(prompt->yes_str[i] | 0x20);
        }
        len = strlen(prompt->no_str);
        for (i = 0; i < len; i++) {
            prompt->no_str[i] = (char)(prompt->no_str[i] & 0xDF);
        }
    } else {
        len = strlen(prompt->no_str);
        for (i = 0; i < len; i++) {
            prompt->no_str[i] = (char)(prompt->no_str[i] | 0x20);
        }
        len = strlen(prompt->yes_str);
        for (i = 0; i < len; i++) {
            prompt->yes_str[i] = (char)(prompt->yes_str[i] & 0xDF);
        }
    }

    msgH = font_string_height(prompt->message);
    yesNoH = font_string_height("YESNO");
    yesW = font_string_width(prompt->yes_str);
    y = msgH + (yesNoH >> 1) + 0x15E;
    render_text_without_clear(prompt->yes_str, ((screen_width >> 1) - 100) - (yesW >> 1), y);

    noW = font_string_width(prompt->no_str);
    render_text_without_clear(prompt->no_str, ((screen_width >> 1) + 100) - (noW >> 1), y);

    if (FontData != 0) {
        _mwMemFree(FontData, 0, 0);
        FontData = 0;
    }
    restore_projection_matrix();
}

void display_image(void) {
    int i;
    unsigned short src;
    unsigned short dst;
    short left;
    GXTlutObj tlut;
    GXTexObj tex;
    Mtx texMtx;

    if (pal_565 == 0) {
        pal_565 = _mwMemMalloc(permanent_heap, 0x200, 5, 0, 0, 0);
        if (pal_565 == 0) {
            return;
        }
        for (i = 0; i < 0x100; i++) {
            src = loading_palette[i];
            /* RGB555 -> RGB565 (retail bit pack). */
            dst = (unsigned short)(((src >> 10) & 0x1F) | (src << 11) | ((src & 0x3E0) << 1));
            pal_565[i] = dst;
        }
    }

    DCFlushRange(loading_image, 0x10000);
    DCFlushRange(pal_565, 0x200);
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

    /* Soft ceiling: ~96.3% -- rlwimi pack + WGPIPE sth schedule; stop. */
    left = (short)((screen_width >> 1) - 0x80);

    GXClearVtxDesc();
    GXSetVtxDesc(9, 1);
    GXSetVtxDesc(0xD, 1);
    GXSetVtxAttrFmt(0, 9, 0, 3, 0);
    GXSetVtxAttrFmt(0, 0xD, 1, 3, 0);
    GXBegin(0x80, 0, 4);
    /* Quad: 256x256 CI8 centered horizontally, y 0x70..0x170, UV 0..256. */
    wgPipe[0] = (unsigned short)left;
    wgPipe[0] = 0x70;
    wgPipe[0] = 0;
    wgPipe[0] = 0;
    wgPipe[0] = (unsigned short)(left + 0x100);
    wgPipe[0] = 0x70;
    wgPipe[0] = 0x100;
    wgPipe[0] = 0;
    wgPipe[0] = (unsigned short)(left + 0x100);
    wgPipe[0] = 0x170;
    wgPipe[0] = 0x100;
    wgPipe[0] = 0x100;
    wgPipe[0] = (unsigned short)left;
    wgPipe[0] = 0x170;
    wgPipe[0] = 0;
    wgPipe[0] = 0x100;
}

void tile_image(unsigned char* dest) {
    unsigned char* tmp;
    int tile;
    int pixel;
    int tx;
    int ty;
    int x;
    int y;

    /* Retail's signed /% lowering matches; only register coloring/scheduling remains. */
    tmp = _mwMemMalloc(wave_heap, 0x10000, 5, 0, 0, 0);
    for (tile = 0; tile < 0x800; tile++) {
        tx = tile % 32;
        ty = tile / 32;
        for (pixel = 0; pixel < 0x20; pixel++) {
            x = tx * 8 + (pixel % 8);
            y = ty * 4 + (pixel / 8);
            tmp[tile * 0x20 + pixel] = loading_image[y * 0x100 + x];
        }
    }
    memcpy(dest, tmp, 0x10000);
    _mwMemFree(tmp, 0, 0);
}

int is_progressive_scan_mode(void) {
    return progscan_mode;
}
