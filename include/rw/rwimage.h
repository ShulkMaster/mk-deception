#ifndef RW_RWIMAGE_H
#define RW_RWIMAGE_H

#include "rw/rwcore_types.h"

RwImage* RwImageCreate(int width, int height, int depth);
int RwImageDestroy(RwImage* image);
RwImage* RwImageAllocatePixels(RwImage* image);
RwImage* RwImageFreePixels(RwImage* image);
RwImage* RwImageCopy(RwImage* destination, const RwImage* source);
const char* RwImageFindFileType(const char* name);
RwImage* RwImageReadMaskedImage(const char* name, const char* maskName);
RwImage* RwImageResample(RwImage* destination, const RwImage* source);
RwImage* RwImageCreateResample(const RwImage* source, int width, int height);
RwImage* RwImageGammaCorrect(RwImage* image);
RwImage* RwImageMakeMask(RwImage* image);
RwImage* RwImageApplyMask(RwImage* image, const RwImage* mask);
int RwImageSetGamma(float gamma);

void* _rwImageOpen(void* instance, int offset, int size);
void* _rwImageClose(void* instance, int offset, int size);

#endif
