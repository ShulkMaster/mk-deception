#include "dolphin/gx.h"
#include "rw/gamecube.h"
#include "rw/rwcore_types.h"

static const GXColor OpaqueWhite = {255, 255, 255, 255};
static const GXColor OpaqueBlack = {0, 0, 0, 255};

static void MatFunc1(const RwRGBAReal* surface, const GXColor* color,
                     RwReal scale)
{
    GXColor tevColor;
    GXColor ambient;

    ambient.r = (RwUInt8)(RwInt32)(color->r * (surface->red * scale));
    ambient.g = (RwUInt8)(RwInt32)(color->g * (surface->green * scale));
    ambient.b = (RwUInt8)(RwInt32)(color->b * (surface->blue * scale));
    ambient.a = 0;
    tevColor = *color;
    GXSetTevColor(1, tevColor);
    GXSetTevColor(2, ambient);
}

static void MatFunc2(const RwRGBAReal* surface, RwReal scale)
{
    GXColor color;
    RwReal intensity = 255.0f * scale;

    color.r = (RwUInt8)(RwInt32)(surface->red * intensity);
    color.g = (RwUInt8)(RwInt32)(surface->green * intensity);
    color.b = (RwUInt8)(RwInt32)(surface->blue * intensity);
    color.a = 0;
    GXSetTevColor(2, color);
}

static void MatFunc3(const RwRGBAReal* surface, RwReal scale)
{
    GXColor color;
    RwReal intensity = 255.0f * scale;

    color.r = (RwUInt8)(RwInt32)(surface->red * intensity);
    color.g = (RwUInt8)(RwInt32)(surface->green * intensity);
    color.b = (RwUInt8)(RwInt32)(surface->blue * intensity);
    color.a = 0;
    GXSetChanAmbColor(0, color);
}

static void MatFunc4(const RwRGBAReal* surface, const GXColor* material,
                     RwReal scale)
{
    GXColor color;
    RwReal intensity = 255.0f * scale;

    color.r = (RwUInt8)(RwInt32)(surface->red * intensity);
    color.g = (RwUInt8)(RwInt32)(surface->green * intensity);
    color.b = (RwUInt8)(RwInt32)(surface->blue * intensity);
    color.a = 0;
    GXSetChanMatColor(4, *material);
    GXSetChanAmbColor(0, color);
}

static void MatFunc5(const GXColor* material)
{
    GXSetChanMatColor(4, *material);
}

static void MatFunc6(const RwRGBAReal* surface, RwReal scale)
{
    GXColor color;
    RwReal intensity = 255.0f * scale;

    color.r = (RwUInt8)(RwInt32)(surface->red * intensity);
    color.g = (RwUInt8)(RwInt32)(surface->green * intensity);
    color.b = (RwUInt8)(RwInt32)(surface->blue * intensity);
    color.a = 0;
    GXSetChanMatColor(0, color);
}

RwDlObjectRenderCallBack _rwDlObjectRenderSetup(RwUInt32 flags,
                                                 RwUInt32 lightMask,
                                                 RwUInt32 textureMode,
                                                 RwBool useAmbient)
{
    RwDlObjectRenderCallBack callback = NULL;
    RwUInt32 materialSource;
    RwUInt32 ambientSource;
    RwUInt32 enableColor;
    RwUInt32 enableAlpha;
    RwUInt32 colorMaterialSource;
    RwUInt32 alphaAmbientSource;
    RwUInt8 numStages;

    if (flags & 0x84) {
        if ((flags & 8) && textureMode == 1) {
            if (lightMask != 0) {
                if (flags & 0x40) {
                    callback = (RwDlObjectRenderCallBack)MatFunc1;
                } else {
                    GXSetTevColor(1, OpaqueWhite);
                    callback = (RwDlObjectRenderCallBack)MatFunc2;
                }
                materialSource = 0;
                colorMaterialSource = 0;
                enableColor = 1;
                ambientSource = 1;
                if (useAmbient == 1) {
                    enableAlpha = 1;
                    alphaAmbientSource = 1;
                } else {
                    enableAlpha = 0;
                    GXSetChanAmbColor(2, OpaqueBlack);
                    alphaAmbientSource = 0;
                }
            } else {
                if (flags & 0x40) {
                    callback = (RwDlObjectRenderCallBack)MatFunc1;
                } else {
                    GXSetTevColor(1, OpaqueWhite);
                    callback = (RwDlObjectRenderCallBack)MatFunc2;
                }
                ambientSource = 0;
                enableAlpha = 0;
                materialSource = 1;
                if (useAmbient == 1) {
                    colorMaterialSource = 1;
                } else {
                    colorMaterialSource = 0;
                    GXSetChanMatColor(2, OpaqueBlack);
                }
                enableColor = 0;
                alphaAmbientSource = 0;
            }
            numStages = 2;
            GXSetTevColorIn(0, 0xF, 0xA, 2, 4);
            GXSetTevAlphaIn(0, 7, 5, 1, 2);
        } else {
            if (lightMask != 0) {
                if (flags & 0x40) {
                    materialSource = 0;
                    colorMaterialSource = 0;
                    enableColor = 1;
                    if (flags & 8) {
                        ambientSource = 1;
                        if (useAmbient == 1) {
                            enableAlpha = 1;
                            alphaAmbientSource = 1;
                        } else {
                            enableAlpha = 0;
                            GXSetChanAmbColor(2, OpaqueBlack);
                            alphaAmbientSource = 0;
                        }
                        callback = (RwDlObjectRenderCallBack)MatFunc5;
                    } else {
                        ambientSource = 0;
                        enableAlpha = 0;
                        alphaAmbientSource = 0;
                        GXSetChanAmbColor(2, OpaqueBlack);
                        callback = (RwDlObjectRenderCallBack)MatFunc4;
                    }
                } else {
                    materialSource = 0;
                    colorMaterialSource = 0;
                    GXSetChanMatColor(4, OpaqueWhite);
                    enableColor = 1;
                    if (flags & 8) {
                        ambientSource = 1;
                        if (useAmbient == 1) {
                            enableAlpha = 1;
                            alphaAmbientSource = 1;
                        } else {
                            enableAlpha = 0;
                            alphaAmbientSource = 0;
                        }
                    } else {
                        ambientSource = 0;
                        enableAlpha = 0;
                        alphaAmbientSource = 0;
                        callback = (RwDlObjectRenderCallBack)MatFunc3;
                    }
                }
            } else if (flags & 0x40) {
                materialSource = 0;
                colorMaterialSource = 0;
                enableColor = 1;
                if (flags & 8) {
                    ambientSource = 1;
                    if (useAmbient == 1) {
                        enableAlpha = 1;
                        alphaAmbientSource = 1;
                    } else {
                        enableAlpha = 0;
                        GXSetChanAmbColor(2, OpaqueBlack);
                        alphaAmbientSource = 0;
                    }
                    callback = (RwDlObjectRenderCallBack)MatFunc5;
                } else {
                    ambientSource = 0;
                    enableAlpha = 0;
                    alphaAmbientSource = 0;
                    GXSetChanAmbColor(2, OpaqueBlack);
                    callback = (RwDlObjectRenderCallBack)MatFunc4;
                }
            } else {
                if (flags & 8) {
                    materialSource = 1;
                    if (useAmbient == 1) {
                        colorMaterialSource = 1;
                    } else {
                        colorMaterialSource = 0;
                        GXSetChanMatColor(2, OpaqueBlack);
                    }
                } else {
                    materialSource = 0;
                    colorMaterialSource = 0;
                    GXSetChanMatColor(2, OpaqueBlack);
                    callback = (RwDlObjectRenderCallBack)MatFunc6;
                }
                enableColor = 0;
                alphaAmbientSource = 0;
                ambientSource = 0;
                enableAlpha = 0;
            }
            numStages = 1;
            GXSetTevColorIn(0, 0xF, 0xA, 8, 0xF);
            GXSetTevAlphaIn(0, 7, 5, 4, 7);
        }
    } else {
        if (lightMask != 0) {
            materialSource = 0;
            colorMaterialSource = 0;
            GXSetChanMatColor(4, OpaqueWhite);
            enableColor = 1;
            if (flags & 8) {
                ambientSource = 1;
                if (useAmbient == 1) {
                    enableAlpha = 1;
                    alphaAmbientSource = 1;
                } else {
                    enableAlpha = 0;
                    alphaAmbientSource = 0;
                }
            } else {
                ambientSource = 0;
                enableAlpha = 0;
                alphaAmbientSource = 0;
                GXSetChanAmbColor(4, OpaqueBlack);
            }
        } else {
            if (flags & 8) {
                materialSource = 1;
                if (useAmbient == 1) {
                    colorMaterialSource = 1;
                } else {
                    colorMaterialSource = 0;
                    GXSetChanMatColor(2, OpaqueBlack);
                }
            } else {
                materialSource = 0;
                colorMaterialSource = 0;
                GXSetChanMatColor(4, OpaqueBlack);
            }
            ambientSource = 0;
            enableAlpha = 0;
            enableColor = 0;
            alphaAmbientSource = 0;
        }
        numStages = 1;
        if (flags & 0x40) {
            callback = (RwDlObjectRenderCallBack)MatFunc1;
        } else {
            GXSetTevColor(1, OpaqueWhite);
            callback = (RwDlObjectRenderCallBack)MatFunc2;
        }
        GXSetTevColorIn(0, 0xF, 0xA, 2, 4);
        GXSetTevAlphaIn(0, 7, 5, 1, 2);
    }

    GXSetNumTevStages(numStages);
    GXSetTevColorOp(0, 0, 0, 0, 1, 0);
    GXSetTevAlphaOp(0, 0, 0, 0, 1, 0);
    if (numStages > 1) {
        GXSetTevColorIn(1, 0xF, 0, 8, 0xF);
        GXSetTevColorOp(1, 0, 0, 0, 1, 0);
        GXSetTevAlphaIn(1, 7, 0, 4, 7);
        GXSetTevAlphaOp(1, 0, 0, 0, 1, 0);
        GXSetTevOrder(0, 0xFF, 0xFF, 4);
        GXSetTevOrder(1, 0, 0, 0xFF);
        GXSetNumTexGens(1);
        GXSetTexCoordGen2(0, 1, 4, 0x3C, 0, 0x7D);
    } else if (flags & 0x84) {
        GXSetTevOrder(0, 0, 0, 4);
        GXSetNumTexGens(1);
        GXSetTexCoordGen2(0, 1, 4, 0x3C, 0, 0x7D);
    } else {
        GXSetNumTexGens(0);
        GXSetTevOrder(0, 0xFF, 0xFF, 4);
    }
    GXSetNumChans(1);
    GXSetChanCtrl(0, enableColor, ambientSource, materialSource, lightMask, 2,
                  1);
    GXSetChanCtrl(2, alphaAmbientSource, enableAlpha, colorMaterialSource, 0, 0,
                  2);
    GXSetChanCtrl(1, 0, 0, 0, 0, 0, 2);
    GXSetChanCtrl(3, 0, 0, 0, 0, 0, 2);
    return callback;
}
