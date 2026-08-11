#include "dolphin/gx.h"
#include "dolphin/vi.h"
#include "libmkparticle/rw_engine.h"
#include "rw/rwerror.h"
#include "rw/rwim3d.h"
#include "rw/rwcamera_internal.h"

typedef struct RwIm2DVertex {
    RwV3d position;
    union {
        RwUInt32 color;
        struct {
            RwUInt8 red;
            RwUInt8 green;
            RwUInt8 blue;
            RwUInt8 alpha;
        } channels;
    } emissiveColor;
    RwV2d texCoords;
} RwIm2DVertex;

extern RwTexture* _RwDlTexture;
extern GXRenderModeObj* _RwDlRenderMode;
extern RwInt32 _RwDlFSAA;
extern RwInt32 _RwDlFSAATop;
extern RwInt32 _RwDlHalfHeight;

extern void _rwDlTextureRasterFlush(void);
extern void GXSetCurrentMtx(RwUInt32 matrix);

static void GXSetTexCoordGen(RwInt32 destination, RwInt32 function,
                             RwInt32 source, RwInt32 matrix);
static void GXEnd(void);
static void GXTexCoord2f32(RwReal s, RwReal t);
static void GXColor4u8(RwUInt8 red, RwUInt8 green, RwUInt8 blue,
                       RwUInt8 alpha);
static void GXPosition3f32(RwReal x, RwReal y, RwReal z);

static RwInt32 _rwDlPrimConvTbl[7] = {
    0, 0xA8, 0xB0, 0x90, 0x98, 0xA0, 0xB8
};

static RwReal projVector[7] = {
    1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f
};

static Mtx posMatrix = {
    {1.0f, 0.0f, 0.0f, 0.0f},
    {0.0f, 1.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, -1.0f, 0.0f},
};

static RwReal _rwDlProjectionMatrix[7];

static void _rw2DRenderPrimitiveInit(void)
{
    RwCamera* camera;
    RwRaster* raster;

    GXClearVtxDesc();
    GXSetVtxDesc(9, 1);
    GXSetVtxAttrFmt(0, 9, 1, 4, 0);
    GXSetVtxDesc(0xB, 1);
    GXSetVtxAttrFmt(0, 0xB, 1, 5, 0);
    GXSetNumTevStages(1);
    GXSetNumChans(1);
    GXSetChanCtrl(4, 0, 0, 1, 0, 0, 2);
    GXSetChanCtrl(5, 0, 0, 0, 0, 0, 2);

    if (_RwDlTexture->raster != 0) {
        GXSetVtxDesc(0xD, 1);
        GXSetVtxAttrFmt(0, 0xD, 1, 4, 0);
        GXSetTevOp(0, 0);
        GXSetNumTexGens(1);
        GXSetTexCoordGen(0, 1, 4, 0x3C);
        GXSetTevOrder(0, 0, 0, 4);
        _rwDlTextureRasterFlush();
    } else {
        GXSetNumTexGens(0);
        GXSetTevOrder(0, 0xFF, 0xFF, 4);
        GXSetTevOp(0, 4);
    }

    if (_RwDlRenderMode->field_rendering != 0) {
        GXSetViewportJitter(0.0f, 0.0f, _RwDlRenderMode->fbWidth,
                            _RwDlRenderMode->xfbHeight, 0.0f, 1.0f,
                            VIGetNextField() ^ 1);
    } else {
        GXSetViewport(0.0f, 0.0f, _RwDlRenderMode->fbWidth,
                      _RwDlRenderMode->xfbHeight, 0.0f, 1.0f);
    }

    camera = (RwCamera*)RwEngineInstance->curCamera;
    raster = camera->frameBuffer;
    projVector[1] = 2.0f / _RwDlRenderMode->fbWidth;
    projVector[3] = -2.0f / _RwDlRenderMode->xfbHeight;
    GXGetProjectionv(_rwDlProjectionMatrix);
    GXSetProjectionv(projVector);
    posMatrix[0][3] = raster->offsetX + 0.5f;
    posMatrix[1][3] = raster->offsetY + 0.5f;
    GXLoadPosMtxImm(posMatrix, 0);
    GXSetCurrentMtx(0);
}

static void _rw2DRenderPrimativeTerm(void)
{
    RwCamera* camera = (RwCamera*)RwEngineInstance->curCamera;
    RwRaster* raster = camera->frameBuffer;

    if (raster != raster->parent) {
        if (_RwDlFSAA == 0) {
            if (_RwDlRenderMode->field_rendering != 0) {
                GXSetViewportJitter(
                    raster->offsetX, raster->offsetY, raster->width,
                    raster->height, 0.0f, 1.0f, VIGetNextField() ^ 1);
            } else {
                GXSetViewport(raster->offsetX, raster->offsetY,
                              raster->width, raster->height, 0.0f, 1.0f);
            }
            GXSetScissor(raster->offsetX, raster->offsetY, raster->width,
                         raster->height);
        } else {
            if (_RwDlRenderMode->field_rendering != 0) {
                GXSetViewportJitter(
                    raster->offsetX, raster->offsetY, raster->width,
                    raster->height, 0.0f, 1.0f, VIGetNextField() ^ 1);
            } else {
                GXSetViewport(raster->offsetX, raster->offsetY * 2,
                              raster->width, raster->height * 2, 0.0f,
                              1.0f);
            }

            if (_RwDlFSAATop != 0) {
                if ((raster->offsetY + raster->height) * 2 <=
                    _RwDlHalfHeight + 2) {
                    GXSetScissor(raster->offsetX, raster->offsetY * 2,
                                 _RwDlRenderMode->fbWidth,
                                 raster->height * 2);
                } else if (raster->offsetY * 2 > _RwDlHalfHeight + 2) {
                    GXSetScissor(0, 0, _RwDlRenderMode->fbWidth,
                                 _RwDlHalfHeight + 2);
                } else {
                    GXSetScissor(raster->offsetX, raster->offsetY * 2,
                                 _RwDlRenderMode->fbWidth,
                                 _RwDlHalfHeight + 2);
                }
                GXSetScissorBoxOffset(0, 0);
            } else {
                if (raster->offsetY * 2 >= _RwDlHalfHeight - 2) {
                    GXSetScissor(raster->offsetX, raster->offsetY * 2,
                                 _RwDlRenderMode->fbWidth,
                                 raster->height * 2);
                } else if ((raster->offsetY + raster->height) * 2 <
                           _RwDlHalfHeight + 2) {
                    GXSetScissor(0, _RwDlHalfHeight - 2,
                                 _RwDlRenderMode->fbWidth,
                                 _RwDlHalfHeight + 2);
                } else {
                    GXSetScissor(raster->offsetX, _RwDlHalfHeight - 2,
                                 _RwDlRenderMode->fbWidth,
                                 raster->height * 2);
                }
                GXSetScissorBoxOffset(0, _RwDlHalfHeight - 2);
            }
        }
    }
    GXSetProjectionv(_rwDlProjectionMatrix);
}

RwBool _rwDlIm2DRenderTriangle(RwIm2DVertex* vertices,
                               RwInt32 numVertices, RwInt32 vertex1,
                               RwInt32 vertex2, RwInt32 vertex3)
{

    RwIm2DVertex* first = &vertices[vertex1];
    RwIm2DVertex* second = &vertices[vertex2];
    RwIm2DVertex* third = &vertices[vertex3];
    _rw2DRenderPrimitiveInit();
    GXBegin(0x90, 0, 3);
    if (_RwDlTexture->raster != 0) {
        GXPosition3f32(first->position.x, first->position.y, first->position.z);
        GXColor4u8(first->emissiveColor.channels.red,
                   first->emissiveColor.channels.green,
                   first->emissiveColor.channels.blue,
                   first->emissiveColor.channels.alpha);
        GXTexCoord2f32(first->texCoords.x, first->texCoords.y);
        GXPosition3f32(second->position.x, second->position.y,
                       second->position.z);
        GXColor4u8(second->emissiveColor.channels.red,
                   second->emissiveColor.channels.green,
                   second->emissiveColor.channels.blue,
                   second->emissiveColor.channels.alpha);
        GXTexCoord2f32(second->texCoords.x, second->texCoords.y);
        GXPosition3f32(third->position.x, third->position.y, third->position.z);
        GXColor4u8(third->emissiveColor.channels.red,
                   third->emissiveColor.channels.green,
                   third->emissiveColor.channels.blue,
                   third->emissiveColor.channels.alpha);
        GXTexCoord2f32(third->texCoords.x, third->texCoords.y);
    } else {
        GXPosition3f32(first->position.x, first->position.y, first->position.z);
        GXColor4u8(first->emissiveColor.channels.red,
                   first->emissiveColor.channels.green,
                   first->emissiveColor.channels.blue,
                   first->emissiveColor.channels.alpha);
        GXPosition3f32(second->position.x, second->position.y,
                       second->position.z);
        GXColor4u8(second->emissiveColor.channels.red,
                   second->emissiveColor.channels.green,
                   second->emissiveColor.channels.blue,
                   second->emissiveColor.channels.alpha);
        GXPosition3f32(third->position.x, third->position.y, third->position.z);
        GXColor4u8(third->emissiveColor.channels.red,
                   third->emissiveColor.channels.green,
                   third->emissiveColor.channels.blue,
                   third->emissiveColor.channels.alpha);
    }
    GXEnd();
    _rw2DRenderPrimativeTerm();
    return 1;
}

RwBool _rwDlIm2DRenderLine(RwIm2DVertex* vertices, RwInt32 numVertices,
                           RwInt32 vertex1, RwInt32 vertex2)
{

    RwIm2DVertex* first = &vertices[vertex1];
    RwIm2DVertex* second = &vertices[vertex2];
    _rw2DRenderPrimitiveInit();
    GXBegin(0xA8, 0, 2);
    if (_RwDlTexture->raster != 0) {
        GXPosition3f32(first->position.x, first->position.y, first->position.z);
        GXColor4u8(first->emissiveColor.channels.red,
                   first->emissiveColor.channels.green,
                   first->emissiveColor.channels.blue,
                   first->emissiveColor.channels.alpha);
        GXTexCoord2f32(first->texCoords.x, first->texCoords.y);
        GXPosition3f32(second->position.x, second->position.y,
                       second->position.z);
        GXColor4u8(second->emissiveColor.channels.red,
                   second->emissiveColor.channels.green,
                   second->emissiveColor.channels.blue,
                   second->emissiveColor.channels.alpha);
        GXTexCoord2f32(second->texCoords.x, second->texCoords.y);
    } else {
        GXPosition3f32(first->position.x, first->position.y, first->position.z);
        GXColor4u8(first->emissiveColor.channels.red,
                   first->emissiveColor.channels.green,
                   first->emissiveColor.channels.blue,
                   first->emissiveColor.channels.alpha);
        GXPosition3f32(second->position.x, second->position.y,
                       second->position.z);
        GXColor4u8(second->emissiveColor.channels.red,
                   second->emissiveColor.channels.green,
                   second->emissiveColor.channels.blue,
                   second->emissiveColor.channels.alpha);
    }
    GXEnd();
    _rw2DRenderPrimativeTerm();
    return 1;
}

RwBool _rwDlIm2DRenderPrimitive(RwPrimitiveType primitiveType,
                                RwIm2DVertex* vertices,
                                RwInt32 numVertices)
{

    RwIm2DVertex* vertex = vertices;
    RwUInt16 count = (RwUInt16)numVertices;
    RwUInt16 verticesPerPrimitive;
    RwBool textured;

    _rw2DRenderPrimitiveInit();
    GXBegin(_rwDlPrimConvTbl[primitiveType], 0, count);

    switch (primitiveType) {
    case rwPRIMTYPELINELIST:
        verticesPerPrimitive = 2;
        break;
    case rwPRIMTYPETRILIST:
        verticesPerPrimitive = 3;
        break;
    case rwPRIMTYPEPOLYLINE:
    case rwPRIMTYPETRISTRIP:
    case rwPRIMTYPETRIFAN:
        verticesPerPrimitive = 1;
        break;
    default: {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x25);
        RwErrorSet(&error);
        verticesPerPrimitive = 0;
        break;
    }
    }

    textured = _RwDlTexture->raster != 0;
    if (verticesPerPrimitive != 0) {
        RwUInt16 primitiveCount = count / verticesPerPrimitive;

        while (primitiveCount-- != 0) {
            RwUInt16 vertexCount = verticesPerPrimitive;

            while (vertexCount-- != 0) {
                GXPosition3f32(vertex->position.x, vertex->position.y,
                               vertex->position.z);
                GXColor4u8(vertex->emissiveColor.channels.red,
                           vertex->emissiveColor.channels.green,
                           vertex->emissiveColor.channels.blue,
                           vertex->emissiveColor.channels.alpha);
                if (textured) {
                    GXTexCoord2f32(vertex->texCoords.x, vertex->texCoords.y);
                }
                vertex++;
            }
        }
    }

    GXEnd();
    _rw2DRenderPrimativeTerm();
    return 1;
}

RwBool _rwDlIm2DRenderIndexedPrimitive(
    RwPrimitiveType primitiveType, RwIm2DVertex* vertices,
    RwInt32 numVertices, RwImVertexIndex* indices, RwInt32 numIndices)
{

    RwImVertexIndex* index = indices;
    RwUInt16 count = (RwUInt16)numIndices;
    RwUInt16 verticesPerPrimitive;
    RwBool textured;

    _rw2DRenderPrimitiveInit();
    GXBegin(_rwDlPrimConvTbl[primitiveType], 0, count);
    switch (primitiveType) {
    case rwPRIMTYPELINELIST:
        verticesPerPrimitive = 2;
        break;
    case rwPRIMTYPETRILIST:
        verticesPerPrimitive = 3;
        break;
    case rwPRIMTYPEPOLYLINE:
    case rwPRIMTYPETRISTRIP:
    case rwPRIMTYPETRIFAN:
        verticesPerPrimitive = 1;
        break;
    default: {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x25);
        RwErrorSet(&error);
        verticesPerPrimitive = 0;
        break;
    }
    }

    textured = _RwDlTexture->raster != 0;
    if (verticesPerPrimitive != 0) {
        RwUInt16 primitiveCount = count / verticesPerPrimitive;

        while (primitiveCount-- != 0) {
            RwUInt16 vertexCount = verticesPerPrimitive;

            while (vertexCount-- != 0) {
                RwIm2DVertex* vertex = &vertices[*index++];

                GXPosition3f32(vertex->position.x, vertex->position.y,
                               vertex->position.z);
                GXColor4u8(vertex->emissiveColor.channels.red,
                           vertex->emissiveColor.channels.green,
                           vertex->emissiveColor.channels.blue,
                           vertex->emissiveColor.channels.alpha);
                if (textured) {
                    GXTexCoord2f32(vertex->texCoords.x, vertex->texCoords.y);
                }
            }
        }
    }

    GXEnd();
    _rw2DRenderPrimativeTerm();
    return 1;
}

static void GXSetTexCoordGen(RwInt32 destination, RwInt32 function,
                             RwInt32 source, RwInt32 matrix)
{
    GXSetTexCoordGen2(destination, function, source, matrix, 0, 0x7D);
}

static void GXEnd(void)
{
}


static void GXTexCoord2f32(RwReal s, RwReal t)
{
    *(volatile RwReal*)0xCC008000 = s;
    *(volatile RwReal*)0xCC008000 = t;
}

static void GXColor4u8(RwUInt8 red, RwUInt8 green, RwUInt8 blue,
                       RwUInt8 alpha)
{
    *(volatile RwUInt8*)0xCC008000 = red;
    *(volatile RwUInt8*)0xCC008000 = green;
    *(volatile RwUInt8*)0xCC008000 = blue;
    *(volatile RwUInt8*)0xCC008000 = alpha;
}

static void GXPosition3f32(RwReal x, RwReal y, RwReal z)
{
    *(volatile RwReal*)0xCC008000 = x;
    *(volatile RwReal*)0xCC008000 = y;
    *(volatile RwReal*)0xCC008000 = z;
}
