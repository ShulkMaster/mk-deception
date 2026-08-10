#include "rw/rwcore_types.h"

extern RwImage* RwImageCreate(RwInt32 width, RwInt32 height, RwInt32 depth);
extern RwImage* RwImageAllocatePixels(RwImage* image);
extern RwBool RwImageDestroy(RwImage* image);
extern RwImage* RwImageFreePixels(RwImage* image);
extern RwImage* RwImageCopy(RwImage* destination, const RwImage* source);


static void ImageResampleGetSpan(const RwImage* source, RwInt32 xStart,
                                 RwInt32 xEnd, RwInt32 y,
                                 RwRGBAReal* color)
{
    RwUInt8* pixel = source->pixels + (y >> 16) * source->stride +
                     (xStart >> 16) * 4;
    RwInt32 current;
    RwReal weight;

    if ((xStart >> 16) == (xEnd >> 16)) {
        color->red = (1.0f / 255.0f) * pixel[0];
        color->green = (1.0f / 255.0f) * pixel[1];
        color->blue = (1.0f / 255.0f) * pixel[2];
        color->alpha = (1.0f / 255.0f) * pixel[3];
        weight = (1.0f / 65536.0f) * (xEnd - xStart);
        color->red *= weight;
        color->green *= weight;
        color->blue *= weight;
        color->alpha *= weight;
    } else {
        RwRGBAReal sample;
        current = ((xStart >> 16) + 1) << 16;
        color->red = (1.0f / 255.0f) * pixel[0];
        color->green = (1.0f / 255.0f) * pixel[1];
        color->blue = (1.0f / 255.0f) * pixel[2];
        color->alpha = (1.0f / 255.0f) * pixel[3];
        weight = (1.0f / 65536.0f) * (current - xStart);
        color->red *= weight;
        color->green *= weight;
        color->blue *= weight;
        color->alpha *= weight;
        pixel += 4;
        while ((current >> 16) != (xEnd >> 16)) {
            sample.red = (1.0f / 255.0f) * pixel[0];
            sample.green = (1.0f / 255.0f) * pixel[1];
            sample.blue = (1.0f / 255.0f) * pixel[2];
            sample.alpha = (1.0f / 255.0f) * pixel[3];
            color->red += sample.red;
            color->green += sample.green;
            color->blue += sample.blue;
            color->alpha += sample.alpha;
            pixel += 4;
            current += 0x10000;
        }
        sample.red = (1.0f / 255.0f) * pixel[0];
        sample.green = (1.0f / 255.0f) * pixel[1];
        sample.blue = (1.0f / 255.0f) * pixel[2];
        sample.alpha = (1.0f / 255.0f) * pixel[3];
        weight = (1.0f / 65536.0f) * (xEnd - current);
        sample.red *= weight;
        sample.green *= weight;
        sample.blue *= weight;
        sample.alpha *= weight;
        color->red += sample.red;
        color->green += sample.green;
        color->blue += sample.blue;
        color->alpha += sample.alpha;
    }
    weight = 1.0f / ((1.0f / 65536.0f) * (xEnd - xStart));
    color->red *= weight;
    color->green *= weight;
    color->blue *= weight;
    color->alpha *= weight;
}


static void ImageResampleGetAvgPixel(const RwImage* source, RwInt32 xStart,
                                     RwInt32 xEnd, RwInt32 yStart,
                                     RwInt32 yEnd, RwRGBAReal* color)
{
    RwInt32 current;
    RwReal weight;

    if ((yStart >> 16) == (yEnd >> 16)) {
        ImageResampleGetSpan(source, xStart, xEnd, yStart, color);
        weight = (1.0f / 65536.0f) * (yEnd - yStart);
        color->red *= weight;
        color->green *= weight;
        color->blue *= weight;
        color->alpha *= weight;
    } else {
        RwRGBAReal sample;
        current = ((yStart >> 16) + 1) << 16;
        ImageResampleGetSpan(source, xStart, xEnd, yStart, color);
        weight = (1.0f / 65536.0f) * (current - yStart);
        color->red *= weight;
        color->green *= weight;
        color->blue *= weight;
        color->alpha *= weight;
        while ((current >> 16) != (yEnd >> 16)) {
            ImageResampleGetSpan(source, xStart, xEnd, current, &sample);
            color->red += sample.red;
            color->green += sample.green;
            color->blue += sample.blue;
            color->alpha += sample.alpha;
            current += 0x10000;
        }
        ImageResampleGetSpan(source, xStart, xEnd, current, &sample);
        weight = (1.0f / 65536.0f) * (yEnd - current);
        sample.red *= weight;
        sample.green *= weight;
        sample.blue *= weight;
        sample.alpha *= weight;
        color->red += sample.red;
        color->green += sample.green;
        color->blue += sample.blue;
        color->alpha += sample.alpha;
    }
    weight = 1.0f / ((1.0f / 65536.0f) * (yEnd - yStart));
    color->red *= weight;
    color->green *= weight;
    color->blue *= weight;
    color->alpha *= weight;
}


RwImage* RwImageResample(RwImage* destination, const RwImage* source)
{
    RwInt32 destWidth = destination->width;
    RwInt32 destHeight = destination->height;
    RwInt32 sourceWidth = source->width;
    RwInt32 sourceHeight = source->height;
    RwInt32 xStep;
    RwInt32 yStep;
    RwInt32 sourceY = 0;
    RwInt32 y;

    destination->flags |= source->flags & 2;
    xStep = (RwInt32)(65536.0f * ((RwReal)sourceWidth / (RwReal)destWidth));
    yStep = (RwInt32)(65536.0f * ((RwReal)sourceHeight / (RwReal)destHeight));
    for (y = 0; y < destHeight; ++y) {
        RwUInt8* destinationPixel = destination->pixels + destination->stride * y;
        RwInt32 sourceX = 0;
        RwInt32 x;
        for (x = 0; x < destWidth; ++x) {
            RwRGBAReal color;
            ImageResampleGetAvgPixel(source, sourceX, sourceX + xStep - 1,
                                     sourceY, sourceY + yStep - 1, &color);
            destinationPixel[x * 4] = (RwUInt8)(RwInt32)(255.0f * color.red + 0.5f);
            destinationPixel[x * 4 + 1] = (RwUInt8)(RwInt32)(255.0f * color.green + 0.5f);
            destinationPixel[x * 4 + 2] = (RwUInt8)(RwInt32)(255.0f * color.blue + 0.5f);
            destinationPixel[x * 4 + 3] = (RwUInt8)(RwInt32)(255.0f * color.alpha + 0.5f);
            sourceX += xStep;
        }
        sourceY += yStep;
    }
    return destination;
}

RwImage* RwImageCreateResample(const RwImage* source, RwInt32 width,
                               RwInt32 height)
{
    RwImage* destination = RwImageCreate(width, height, 32);
    if (destination == 0)
        return 0;
    if (RwImageAllocatePixels(destination) == 0) {
        RwImageDestroy(destination);
        return 0;
    }
    if (source->depth != 32) {
        RwImage* converted = RwImageCreate(source->width, source->height, 32);
        if (converted == 0) {
            RwImageFreePixels(destination);
            RwImageDestroy(destination);
            return 0;
        }
        if (RwImageAllocatePixels(converted) == 0) {
            RwImageDestroy(converted);
            RwImageFreePixels(destination);
            RwImageDestroy(destination);
            return 0;
        }
        RwImageCopy(converted, source);
        if (RwImageResample(destination, converted) == 0) {
            RwImageFreePixels(converted);
            RwImageDestroy(converted);
            RwImageFreePixels(destination);
            RwImageDestroy(destination);
            return 0;
        }
        RwImageFreePixels(converted);
        RwImageDestroy(converted);
    } else if (RwImageResample(destination, source) == 0) {
        RwImageFreePixels(destination);
        RwImageDestroy(destination);
        return 0;
    }
    return destination;
}
