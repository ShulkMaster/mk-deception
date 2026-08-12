#include "libmkparticle/rw_engine.h"
#include "rw/gamecube_texture.h"
#include "rw/palquant.h"
#include "rw/rwerror.h"
#include "rw/rxpipeline.h"

enum {
    rwRASTERFORMATDEFAULT = 0x0000,
    rwRASTERFORMAT1555 = 0x0100,
    rwRASTERFORMAT565 = 0x0200,
    rwRASTERFORMAT4444 = 0x0300,
    rwRASTERFORMATLUM8 = 0x0400,
    rwRASTERFORMAT8888 = 0x0500,
    rwRASTERFORMAT888 = 0x0600,
    rwRASTERFORMAT555 = 0x0A00,
    rwRASTERFORMATPIXELFORMATMASK = 0x0F00,
    rwRASTERFORMATAUTOMIPMAP = 0x1000,
    rwRASTERFORMATPAL8 = 0x2000,
    rwRASTERFORMATPAL4 = 0x4000,
    rwRASTERFORMATMIPMAP = 0x8000
};

typedef int (*RwDlPixelConvertFn)(const RwRGBA* color);
typedef void (*RwDlPixelUnconvertFn)(RwRGBA* color, unsigned int pixel);

extern void* RwRasterLockPalette(RwRaster* raster, int lockMode);
extern RwRaster* RwRasterUnlockPalette(RwRaster* raster);
extern RwImage* RwImageCreate(int width, int height, int depth);
extern int RwImageDestroy(RwImage* image);
extern RwImage* RwImageAllocatePixels(RwImage* image);

int _rwDlFindMSB(int value)
{
    int position = -1;

    while (value != 0) {
        position++;
        value >>= 1;
    }
    return position;
}

static int _rwDlConv8888To555(const RwRGBA* color)
{
    int result = ((color->blue >> 3) & 0x1F) |
        (((color->red << 7) & 0x7C00) | 0x8000 |
         ((color->green & 0xF8) << 2));

    return result;
}

static int _rwDlConv8888To565(const RwRGBA* color)
{
    int result = ((color->blue >> 3) & 0x1F) |
        (((color->red << 8) & 0xF800) |
         ((color->green & 0xFC) << 3));

    return result;
}

static int _rwDlConv8888To555or3444(const RwRGBA* color)
{
    int result;

    if (color->alpha != 0xFF) {
        result = ((color->blue >> 4) & 0x0F) |
            ((color->green & 0xF0) |
             (((color->alpha << 7) & 0x7000) |
              ((color->red & 0xF0) << 4)));
    } else {
        result = ((color->blue >> 3) & 0x1F) |
            (((color->red << 7) & 0x7C00) | 0x8000 |
             ((color->green & 0xF8) << 2));
    }
    return result;
}

static int _rwDlConv8888ToDl888(const RwRGBA* color)
{
    int result = color->blue |
        ((color->red << 16) | 0xFF000000 | (color->green << 8));

    return result;
}

static int _rwDlConv8888ToDl8888(const RwRGBA* color)
{
    int result = color->blue |
        ((color->green << 8) |
         ((color->alpha << 24) | (color->red << 16)));

    return result;
}

int _rwDlRGBToPixel(void* pixelOut, void* colorIn, int format)
{
    unsigned int pixel;

    switch (format & rwRASTERFORMATPIXELFORMATMASK) {
    case rwRASTERFORMATDEFAULT:
        _rwDlRGBToPixel(&pixel, colorIn, rwRASTERFORMAT565);
        break;
    case rwRASTERFORMATLUM8: {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(2,
            "rwRASTERFORMATLUM8 not yet supported");
        RwErrorSet(&error);
        break;
    }
    case rwRASTERFORMAT1555:
    case rwRASTERFORMAT4444:
        pixel = _rwDlConv8888To555or3444(colorIn);
        break;
    case rwRASTERFORMAT555:
        pixel = _rwDlConv8888To555(colorIn);
        break;
    case rwRASTERFORMAT565:
        pixel = _rwDlConv8888To565(colorIn);
        break;
    case rwRASTERFORMAT8888:
        pixel = _rwDlConv8888ToDl8888(colorIn);
        break;
    case rwRASTERFORMAT888:
        pixel = _rwDlConv8888ToDl888(colorIn);
        break;
    default: {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x8000000D);
        RwErrorSet(&error);
        break;
    }
    }
    *(unsigned int*)pixelOut = pixel;
    return 1;
}

static void _rwDlConv555To8888(RwRGBA* color, unsigned int pixel)
{
    color->red = (pixel >> 7) & 0xF8;
    color->green = (pixel >> 2) & 0xF8;
    color->blue = (pixel << 3) & 0xF8;
    color->alpha = 0xFF;
}

static void _rwDlConv565To8888(RwRGBA* color, unsigned int pixel)
{
    color->red = (pixel >> 8) & 0xF8;
    color->green = (pixel >> 3) & 0xFC;
    color->blue = (pixel << 3) & 0xF8;
    color->alpha = 0xFF;
}

static void _rwDlConv1555To8888(RwRGBA* color, unsigned int pixel)
{
    if ((pixel & 0x8000) != 0) {
        color->red = (pixel >> 7) & 0xF8;
        color->green = (pixel >> 2) & 0xF8;
        color->blue = (pixel << 3) & 0xF8;
        color->alpha = 0xFF;
    } else {
        color->red = (pixel >> 4) & 0xF0;
        color->green = pixel & 0xF0;
        color->blue = (pixel << 4) & 0xF0;
        color->alpha = 0;
    }
}

static void _rwDlConv4444To8888(RwRGBA* color, unsigned int pixel)
{
    if ((pixel & 0x8000) != 0) {
        color->red = (pixel >> 7) & 0xF8;
        color->green = (pixel >> 2) & 0xF8;
        color->blue = (pixel << 3) & 0xF8;
        color->alpha = 0xFF;
    } else {
        color->red = (pixel >> 4) & 0xF0;
        color->green = pixel & 0xF0;
        color->blue = (pixel << 4) & 0xF0;
        color->alpha = (pixel >> 7) & 0xE0;
    }
}

static void _rwDlConvDl888To8888(RwRGBA* color, unsigned int pixel)
{
    color->alpha = 0xFF;
    color->red = (pixel >> 16) & 0xFF;
    color->green = (pixel >> 8) & 0xFF;
    color->blue = pixel;
}

static void _rwDlConvDl8888To8888(RwRGBA* color, unsigned int pixel)
{
    color->alpha = pixel >> 24;
    color->red = (pixel >> 16) & 0xFF;
    color->green = (pixel >> 8) & 0xFF;
    color->blue = pixel;
}

int _rwDlPixelToRGB(void* colorOut, void* pixelIn, int format)
{
    unsigned int pixel = *(unsigned int*)pixelIn;

    switch (format & rwRASTERFORMATPIXELFORMATMASK) {
    case rwRASTERFORMATDEFAULT:
        _rwDlPixelToRGB(colorOut, pixelIn, rwRASTERFORMAT565);
        break;
    case rwRASTERFORMATLUM8: {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(2,
            "rwRASTERFORMATLUM8 not yet supported");
        RwErrorSet(&error);
        break;
    }
    case rwRASTERFORMAT555:
        _rwDlConv555To8888(colorOut, pixel);
        break;
    case rwRASTERFORMAT1555:
        _rwDlConv1555To8888(colorOut, pixel);
        break;
    case rwRASTERFORMAT565:
        _rwDlConv565To8888(colorOut, pixel);
        break;
    case rwRASTERFORMAT4444:
        _rwDlConv4444To8888(colorOut, pixel);
        break;
    case rwRASTERFORMAT888:
        _rwDlConvDl888To8888(colorOut, pixel);
        break;
    case rwRASTERFORMAT8888:
        _rwDlConvDl8888To8888(colorOut, pixel);
        break;
    default: {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x8000000D);
        RwErrorSet(&error);
        break;
    }
    }
    return 1;
}

static RwDlPixelUnconvertFn _rwDlSelectUnconvertFn(int format)
{
    RwDlPixelUnconvertFn result = 0;

    switch (format & rwRASTERFORMATPIXELFORMATMASK) {
    case rwRASTERFORMATLUM8: {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(2,
            "rwRASTERFORMATLUM8 not yet supported");
        RwErrorSet(&error);
        break;
    }
    case rwRASTERFORMAT555:
        result = _rwDlConv555To8888;
        break;
    case rwRASTERFORMAT565:
        result = _rwDlConv565To8888;
        break;
    case rwRASTERFORMAT1555:
        result = _rwDlConv1555To8888;
        break;
    case rwRASTERFORMAT4444:
        result = _rwDlConv4444To8888;
        break;
    case rwRASTERFORMAT8888:
        result = _rwDlConvDl8888To8888;
        break;
    case rwRASTERFORMAT888:
        result = _rwDlConvDl888To8888;
        break;
    default: {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x8000000D);
        RwErrorSet(&error);
        break;
    }
    }
    return result;
}

static void _rwDlImage4GetFromRaster(RwImage* image, RwRaster* raster)
{
    RwDlPixelUnconvertFn convert =
        _rwDlSelectUnconvertFn((unsigned int)raster->format << 8);
    int x;
    int y;

    switch (raster->depth) {
    case 4:
        for (x = 0; x < 16; x++)
            convert(&((RwRGBA*)image->palette)[x],
                    ((unsigned short*)raster->palette)[x]);
        for (y = 0; y < raster->height; y++) {
            unsigned char* destination = image->pixels + image->stride * y;
            unsigned char* source = raster->pixels + raster->stride * y;
            for (x = 0; x < raster->width; x += 2) {
                *destination++ = *source >> 4;
                *destination++ = *source++ & 0xF;
            }
        }
        break;
    case 8:
    case 16:
    case 32: {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(2,
            "Conversion from 8/16/32bit rasters to 4bit images is not supported");
        RwErrorSet(&error);
        break;
    }
    default: {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x8000000C);
        RwErrorSet(&error);
        break;
    }
    }
}

static void _rwDlImage8GetFromRaster(RwImage* image, RwRaster* raster)
{
    RwDlPixelUnconvertFn convert =
        _rwDlSelectUnconvertFn((unsigned int)raster->format << 8);
    int x;
    int y;

    switch (raster->depth) {
    case 4:
        for (x = 0; x < 16; x++)
            convert(&((RwRGBA*)image->palette)[x],
                    ((unsigned short*)raster->palette)[x]);
        for (y = 0; y < raster->height; y++) {
            unsigned char* destination = image->pixels + image->stride * y;
            unsigned char* source = raster->pixels + raster->stride * y;
            for (x = 0; x < raster->width; x += 2) {
                *destination++ = *source >> 4;
                *destination++ = *source++ & 0xF;
            }
        }
        break;
    case 8:
        for (x = 0; x < 256; x++)
            convert(&((RwRGBA*)image->palette)[x],
                    ((unsigned short*)raster->palette)[x]);
        for (y = 0; y < raster->height; y++) {
            unsigned char* source = raster->pixels + raster->stride * y;
            unsigned char* destination = image->pixels + image->stride * y;
            for (x = 0; x < raster->width; x++)
                *destination++ = *source++;
        }
        break;
    case 16:
    case 32: {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(2,
            "Conversion from 16/32bit rasters to 8bit images is not supported");
        RwErrorSet(&error);
        break;
    }
    default: {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x8000000C);
        RwErrorSet(&error);
        break;
    }
    }
}

static void _rwDlImage32GetFromRaster(RwImage* image, RwRaster* raster)
{
    RwDlPixelUnconvertFn convert =
        _rwDlSelectUnconvertFn((unsigned int)raster->format << 8);
    RwRGBA palette[256];
    int x;
    int y;

    switch (raster->depth) {
    case 4:
        for (x = 0; x < 16; x++)
            convert(&palette[x], ((unsigned short*)raster->palette)[x]);
        for (y = 0; y < raster->height; y++) {
            unsigned char* source = raster->pixels + raster->stride * y;
            RwRGBA* destination =
                (RwRGBA*)(image->pixels + image->stride * y);
            for (x = 0; x < raster->width; x += 2) {
                *destination++ = palette[*source >> 4];
                *destination++ = palette[*source++ & 0xF];
            }
        }
        break;
    case 8:
        for (x = 0; x < 256; x++)
            convert(&palette[x], ((unsigned short*)raster->palette)[x]);
        for (y = 0; y < raster->height; y++) {
            unsigned char* source = raster->pixels + raster->stride * y;
            RwRGBA* destination =
                (RwRGBA*)(image->pixels + image->stride * y);
            for (x = 0; x < raster->width; x++)
                *destination++ = palette[*source++];
        }
        break;
    case 16:
        for (y = 0; y < raster->height; y++) {
            unsigned short* source =
                (unsigned short*)(raster->pixels + raster->stride * y);
            RwRGBA* destination =
                (RwRGBA*)(image->pixels + image->stride * y);
            for (x = 0; x < raster->width; x++)
                convert(destination++, *source++);
        }
        break;
    case 32:
        for (y = 0; y < raster->height; y++) {
            unsigned int* source =
                (unsigned int*)(raster->pixels + raster->stride * y);
            RwRGBA* destination =
                (RwRGBA*)(image->pixels + image->stride * y);
            for (x = 0; x < raster->width; x++)
                convert(destination++, *source++);
        }
        break;
    default: {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x8000000C);
        RwErrorSet(&error);
        break;
    }
    }
}

int _rwDlImageGetFromRaster(void* imageIn, void* rasterIn,
                               int unused)
{
    RwImage* image = imageIn;
    RwRaster* raster = rasterIn;
    int rasterLocked = 0;
    int paletteLocked = 0;

    if ((raster->privateFlags & 2) == 0) {
        RwRasterLock(raster, 0, 2);
        rasterLocked = 1;
    }
    if ((((unsigned int)raster->format << 8) &
         (rwRASTERFORMATPAL4 | rwRASTERFORMATPAL8)) != 0 &&
        (raster->privateFlags & 8) == 0) {
        RwRasterLockPalette(raster, 2);
        paletteLocked = 1;
    }
    switch (image->depth) {
    case 4:
        _rwDlImage4GetFromRaster(image, raster);
        break;
    case 8:
        _rwDlImage8GetFromRaster(image, raster);
        break;
    case 32:
        _rwDlImage32GetFromRaster(image, raster);
        break;
    default: {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x80000008);
        RwErrorSet(&error);
        break;
    }
    }
    if (paletteLocked == 1)
        RwRasterUnlockPalette(raster);
    if (rasterLocked == 1)
        RwRasterUnlock(raster);
    return 1;
}

static RwDlPixelConvertFn _rwDlSelectConvertFn(const RwRaster* raster)
{
    RwDlPixelConvertFn result = 0;

    switch (((unsigned int)raster->format << 8) &
            rwRASTERFORMATPIXELFORMATMASK) {
    case rwRASTERFORMATLUM8: {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(2,
            "rwRASTERFORMATLUM8 not yet supported");
        RwErrorSet(&error);
        break;
    }
    case rwRASTERFORMAT555:
        result = _rwDlConv8888To555;
        break;
    case rwRASTERFORMAT565:
        result = _rwDlConv8888To565;
        break;
    case rwRASTERFORMAT1555:
    case rwRASTERFORMAT4444:
        result = _rwDlConv8888To555or3444;
        break;
    case rwRASTERFORMAT888:
        result = _rwDlConv8888ToDl888;
        break;
    case rwRASTERFORMAT8888:
        result = _rwDlConv8888ToDl8888;
        break;
    default: {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x8000000D);
        RwErrorSet(&error);
        break;
    }
    }
    return result;
}

static RwImage* _rwDolphinPalettizeImage(RwImage* image, int depth)
{
    RwPalQuant quantizer;
    RwImage* palettized = RwImageCreate(image->width, image->height, depth);

    if (palettized == 0)
        return 0;
    RwImageAllocatePixels(palettized);
    if (RwPalQuantInit(&quantizer) == 0)
        return 0;
    RwPalQuantAddImage(&quantizer, image, 1.0f);
    RwPalQuantResolvePalette((RwRGBA*)palettized->palette, 1 << depth,
                             &quantizer);
    RwPalQuantMatchImage(palettized->pixels, palettized->stride,
                         palettized->depth, 0, &quantizer, image);
    RwPalQuantTerm(&quantizer);
    return palettized;
}

static void _rwDlRasterPalletized4SetFromImage(RwRaster* raster,
                                                RwImage* image)
{
    RwGameCubeRasterExt* extension = RwGameCubeRasterExtension(raster);
    RwDlPixelConvertFn convert = _rwDlSelectConvertFn(raster);
    int x;
    int y;

    switch (image->depth) {
    case 4:
        for (y = 0; y < raster->height; y++) {
            unsigned char* source = image->pixels + image->stride * y;
            unsigned char* destination = raster->pixels + raster->stride * y;
            for (x = 0; x < raster->width; x += 2) {
                *destination++ = (source[0] << 4) | (source[1] & 0xF);
                source += 2;
            }
        }
        if (extension->maxLod == 0) {
            for (x = 0; x < 16; x++)
                ((unsigned short*)raster->palette)[x] =
                    convert(&((RwRGBA*)image->palette)[x]);
        }
        break;
    case 8:
    case 32: {
        RwImage* palettized =
            _rwDolphinPalettizeImage(image, raster->depth);
        if (palettized != 0) {
            _rwDlRasterPalletized4SetFromImage(raster, palettized);
            RwImageDestroy(palettized);
        }
        break;
    }
    default: {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x80000008);
        RwErrorSet(&error);
        break;
    }
    }
}

static void _rwDlRasterPalletized8SetFromImage(RwRaster* raster,
                                                RwImage* image)
{
    RwGameCubeRasterExt* extension = RwGameCubeRasterExtension(raster);
    RwDlPixelConvertFn convert = _rwDlSelectConvertFn(raster);
    int x;
    int y;

    switch (image->depth) {
    case 4:
    case 8:
        for (y = 0; y < raster->height; y++) {
            unsigned char* source = image->pixels + image->stride * y;
            unsigned char* destination = raster->pixels + raster->stride * y;
            for (x = 0; x < raster->width; x++)
                *destination++ = *source++;
        }
        if (extension->maxLod == 0) {
            for (x = 0; x < (1 << image->depth); x++)
                ((unsigned short*)raster->palette)[x] =
                    convert(&((RwRGBA*)image->palette)[x]);
        }
        break;
    case 32: {
        RwImage* palettized =
            _rwDolphinPalettizeImage(image, raster->depth);
        if (palettized != 0) {
            _rwDlRasterPalletized8SetFromImage(raster, palettized);
            RwImageDestroy(palettized);
        }
        break;
    }
    default: {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x80000008);
        RwErrorSet(&error);
        break;
    }
    }
}

static void _rwDlRaster16SetFromImage(RwRaster* raster, RwImage* image)
{
    RwDlPixelConvertFn convert = _rwDlSelectConvertFn(raster);
    unsigned short palette[256];
    int x;
    int y;

    switch (image->depth) {
    case 4:
    case 8:
        for (x = 0; x < (1 << image->depth); x++)
            palette[x] = convert(&((RwRGBA*)image->palette)[x]);
        for (y = 0; y < raster->height; y++) {
            unsigned char* source = image->pixels + image->stride * y;
            unsigned short* destination =
                (unsigned short*)(raster->pixels + raster->stride * y);
            for (x = 0; x < raster->width; x++)
                *destination++ = palette[*source++];
        }
        break;
    case 32:
        for (y = 0; y < raster->height; y++) {
            RwRGBA* source =
                (RwRGBA*)(image->pixels + image->stride * y);
            unsigned short* destination =
                (unsigned short*)(raster->pixels + raster->stride * y);
            for (x = 0; x < raster->width; x++)
                *destination++ = convert(source++);
        }
        break;
    default: {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x80000008);
        RwErrorSet(&error);
        break;
    }
    }
}

static void _rwDlRaster32SetFromImage(RwRaster* raster, RwImage* image)
{
    RwDlPixelConvertFn convert = _rwDlSelectConvertFn(raster);
    unsigned int palette[256];
    int x;
    int y;

    switch (image->depth) {
    case 4:
    case 8:
        for (x = 0; x < (1 << image->depth); x++)
            palette[x] = convert(&((RwRGBA*)image->palette)[x]);
        for (y = 0; y < raster->height; y++) {
            unsigned char* source = image->pixels + image->stride * y;
            unsigned int* destination =
                (unsigned int*)(raster->pixels + raster->stride * y);
            for (x = 0; x < raster->width; x++)
                *destination++ = palette[*source++];
        }
        break;
    case 32:
        for (y = 0; y < raster->height; y++) {
            RwRGBA* source =
                (RwRGBA*)(image->pixels + image->stride * y);
            unsigned int* destination =
                (unsigned int*)(raster->pixels + raster->stride * y);
            for (x = 0; x < raster->width; x++)
                *destination++ = convert(source++);
        }
        break;
    default: {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x80000008);
        RwErrorSet(&error);
        break;
    }
    }
}

int _rwDlRasterSetFromImage(void* rasterIn, void* imageIn,
                               int unused)
{
    RwRaster* raster = rasterIn;
    RwImage* image = imageIn;
    int rasterLocked = 0;
    int paletteLocked = 0;
    int format = (unsigned int)raster->format << 8;

    if ((raster->privateFlags & 4) != 0)
        rasterLocked = 1;
    if (rasterLocked == 0 && RwRasterLock(raster, 0, 5) == 0)
        return 0;

    if ((format & (rwRASTERFORMATPAL4 | rwRASTERFORMATPAL8)) != 0) {
        if ((raster->privateFlags & 0x10) != 0)
            paletteLocked = 1;
        if (paletteLocked == 0 &&
            RwRasterLockPalette(raster, 5) == 0) {
            return 0;
        }
    }

    switch (raster->depth) {
    case 4:
        _rwDlRasterPalletized4SetFromImage(raster, image);
        break;
    case 8:
        _rwDlRasterPalletized8SetFromImage(raster, image);
        break;
    case 16:
        _rwDlRaster16SetFromImage(raster, image);
        break;
    case 32:
        _rwDlRaster32SetFromImage(raster, image);
        break;
    default: {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x8000000C);
        RwErrorSet(&error);
        break;
    }
    }

    if ((format & (rwRASTERFORMATPAL4 | rwRASTERFORMATPAL8)) != 0 &&
        paletteLocked == 0) {
        RwRasterUnlockPalette(raster);
    }
    if (rasterLocked == 0)
        RwRasterUnlock(raster);
    return 1;
}

static int _rwDlImageFindFormat(RwImage* image)
{
    int hasAlpha = 0;
    int depth = image->depth;
    int x;
    int y;

    if (depth == 4 || depth == 8) {
        unsigned char* pixels = image->pixels;
        RwRGBA* palette = (RwRGBA*)image->palette;
        for (y = 0; y < image->height; y++) {
            unsigned char* pixel = pixels;
            for (x = 0; x < image->width; x++) {
                unsigned char alpha = palette[*pixel].alpha;
                if (alpha != 0xFF) {
                    hasAlpha = 1;
                    if (alpha > 0xF)
                        return rwRASTERFORMAT4444 |
                            (depth == 4 ? rwRASTERFORMATPAL4 :
                                          rwRASTERFORMATPAL8);
                }
                pixel++;
            }
            pixels += image->stride;
        }
    } else {
        unsigned char* pixels = image->pixels;
        for (y = 0; y < image->height; y++) {
            RwRGBA* pixel = (RwRGBA*)pixels;
            for (x = 0; x < image->width; x++) {
                if (pixel->alpha != 0xFF) {
                    hasAlpha = 1;
                    if (pixel->alpha > 0xF)
                        return rwRASTERFORMAT4444;
                }
                pixel++;
            }
            pixels += image->stride;
        }
    }

    {
        int format =
            hasAlpha ? rwRASTERFORMAT1555 : rwRASTERFORMAT565;
        if (depth == 4)
            format |= rwRASTERFORMATPAL4;
        else if (depth == 8)
            format |= rwRASTERFORMATPAL8;
        return format;
    }
}

int _rwDlImageFindRasterFormat(void* rasterIn, void* imageIn,
                                  int flags)
{
    RwRaster* raster = rasterIn;
    RwImage* image = imageIn;
    int type = flags & 7;

    raster->type = type;
    raster->depth = 0;
    switch (type) {
    case 1:
    case 2:
        raster->format = 0;
        raster->width = image->width;
        raster->height = image->height;
        return 1;
    case 0:
    case 4:
    case 5:
        raster->width = image->width > 0x400 ? 0x400 : image->width;
        raster->height = image->height > 0x400 ? 0x400 : image->height;
        if ((flags & rwRASTERFORMATMIPMAP) != 0) {
            raster->width = 1 << _rwDlFindMSB(raster->width);
            raster->height = 1 << _rwDlFindMSB(raster->height);
        }
        raster->format = (unsigned char)((_rwDlImageFindFormat(image) |
            (flags & (rwRASTERFORMATMIPMAP |
                      rwRASTERFORMATAUTOMIPMAP))) >> 8);
        return 1;
    default: {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x8000000D);
        RwErrorSet(&error);
        return 0;
    }
    }
}
