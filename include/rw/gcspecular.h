#ifndef RW_GCSPECULAR_H
#define RW_GCSPECULAR_H

#include "rw/rpworld_types.h"

typedef union SpecularMaterialFlags {
    unsigned int word;
    struct {
        signed char hidden : 1;
        signed char reflectionPass : 1;
        signed char cullFront : 1;
        signed char swapMode : 1;
        unsigned char reserved : 4;
        unsigned char field_0x01[3];
    } bits;
    struct {
        unsigned char value;
        unsigned char field_0x01[3];
    } raw;
} SpecularMaterialFlags;

typedef struct SpecularMaterialPluginData {
    void* light;
    RwFrame* frame;
    RwTexture* texture;
    RwTexture* saved_texture;
    RpSurfaceProperties savedSurface;
    int clipValue;
    float shininess;
    RwRGBA tint;
    float gloss;
    SpecularMaterialFlags flags;
} SpecularMaterialPluginData;

extern int SpecularMaterialOffset;
extern int SpecularGeometryOffset;

#endif
