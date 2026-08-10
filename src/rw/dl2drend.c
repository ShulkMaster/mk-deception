#include "dolphin/gx.h"
#include "dolphin/vi.h"
#include "libmkparticle/rw_engine.h"
#include "rw/rwerror.h"
#include "rw/rwim3d.h"

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

#define SUBMIT_UNTEXTURED_VERTEX(vertex)                                 \
    do {                                                                 \
        GXPosition3f32((vertex)->position.x, (vertex)->position.y,        \
                       (vertex)->position.z);                            \
        GXColor4u8((RwUInt8)(vertex)->emissiveColor.channels.red,         \
                   (RwUInt8)(vertex)->emissiveColor.channels.green,       \
                   (RwUInt8)(vertex)->emissiveColor.channels.blue,        \
                   (RwUInt8)(vertex)->emissiveColor.channels.alpha);      \
    } while (0)

#define SUBMIT_TEXTURED_VERTEX(vertex)                                   \
    do {                                                                 \
        SUBMIT_UNTEXTURED_VERTEX(vertex);                                \
        GXTexCoord2f32((vertex)->texCoords.x, (vertex)->texCoords.y);     \
    } while (0)

#define SUBMIT_TEXTURED_INDEXED(vertices, index)                         \
    do {                                                                 \
        RwIm2DVertex* currentVertex = &(vertices)[*(index)++];            \
        SUBMIT_TEXTURED_VERTEX(currentVertex);                            \
    } while (0)

#define SUBMIT_UNTEXTURED_INDEXED(vertices, index)                       \
    do {                                                                 \
        RwIm2DVertex* currentVertex = &(vertices)[*(index)++];            \
        SUBMIT_UNTEXTURED_VERTEX(currentVertex);                          \
    } while (0)

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

    if (_RwDlTexture->raster != NULL) {
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
    /* Submission instructions are exact. Retail selects save/restore helpers
     * for the four nonvolatile vertex registers; clean O0 C saves them
     * individually. */
    RwIm2DVertex* first = &vertices[vertex1];
    RwIm2DVertex* second = &vertices[vertex2];
    RwIm2DVertex* third = &vertices[vertex3];
    _rw2DRenderPrimitiveInit();
    GXBegin(0x90, 0, 3);
    if (_RwDlTexture->raster != NULL) {
        SUBMIT_TEXTURED_VERTEX(first);
        SUBMIT_TEXTURED_VERTEX(second);
        SUBMIT_TEXTURED_VERTEX(third);
    } else {
        SUBMIT_UNTEXTURED_VERTEX(first);
        SUBMIT_UNTEXTURED_VERTEX(second);
        SUBMIT_UNTEXTURED_VERTEX(third);
    }
    GXEnd();
    _rw2DRenderPrimativeTerm();
    return TRUE;
}

RwBool _rwDlIm2DRenderLine(RwIm2DVertex* vertices, RwInt32 numVertices,
                           RwInt32 vertex1, RwInt32 vertex2)
{
    /* Submission instructions are exact; the residual is the same retail
     * helper-save selection used by the triangle path. */
    RwIm2DVertex* first = &vertices[vertex1];
    RwIm2DVertex* second = &vertices[vertex2];
    _rw2DRenderPrimitiveInit();
    GXBegin(0xA8, 0, 2);
    if (_RwDlTexture->raster != NULL) {
        SUBMIT_TEXTURED_VERTEX(first);
        SUBMIT_TEXTURED_VERTEX(second);
    } else {
        SUBMIT_UNTEXTURED_VERTEX(first);
        SUBMIT_UNTEXTURED_VERTEX(second);
    }
    GXEnd();
    _rw2DRenderPrimativeTerm();
    return TRUE;
}

RwBool _rwDlIm2DRenderPrimitive(RwPrimitiveType primitiveType,
                                RwIm2DVertex* vertices,
                                RwInt32 numVertices)
{
    /* Primitive grouping, truncation, error behavior, and FIFO submission are
     * recovered. The stock immediate-mode macros retain broader O0 temporary
     * lifetimes than this typed expansion. */
    RwIm2DVertex* vertex = vertices;
    RwUInt16 count = (RwUInt16)numVertices;

    _rw2DRenderPrimitiveInit();
    GXBegin(_rwDlPrimConvTbl[primitiveType], 0, count);

    switch (primitiveType) {
    case rwPRIMTYPELINELIST: {
        RwInt32 lines = count >> 1;
        if (_RwDlTexture->raster != NULL) {
            while (lines-- != 0) {
                SUBMIT_TEXTURED_VERTEX(vertex);
                vertex++;
                SUBMIT_TEXTURED_VERTEX(vertex);
                vertex++;
            }
        } else {
            while (lines-- != 0) {
                SUBMIT_UNTEXTURED_VERTEX(vertex);
                vertex++;
                SUBMIT_UNTEXTURED_VERTEX(vertex);
                vertex++;
            }
        }
        break;
    }
    case rwPRIMTYPETRILIST: {
        RwInt32 triangles = count / 3;
        if (_RwDlTexture->raster != NULL) {
            while (triangles-- != 0) {
                SUBMIT_TEXTURED_VERTEX(vertex);
                vertex++;
                SUBMIT_TEXTURED_VERTEX(vertex);
                vertex++;
                SUBMIT_TEXTURED_VERTEX(vertex);
                vertex++;
            }
        } else {
            while (triangles-- != 0) {
                SUBMIT_UNTEXTURED_VERTEX(vertex);
                vertex++;
                SUBMIT_UNTEXTURED_VERTEX(vertex);
                vertex++;
                SUBMIT_UNTEXTURED_VERTEX(vertex);
                vertex++;
            }
        }
        break;
    }
    case rwPRIMTYPEPOLYLINE:
    case rwPRIMTYPETRISTRIP:
    case rwPRIMTYPETRIFAN:
        if (_RwDlTexture->raster != NULL) {
            while (count-- != 0) {
                SUBMIT_TEXTURED_VERTEX(vertex);
                vertex++;
            }
        } else {
            while (count-- != 0) {
                SUBMIT_UNTEXTURED_VERTEX(vertex);
                vertex++;
            }
        }
        break;
    default: {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x25);
        RwErrorSet(&error);
        break;
    }
    }

    GXEnd();
    _rw2DRenderPrimativeTerm();
    return TRUE;
}

RwBool _rwDlIm2DRenderIndexedPrimitive(
    RwPrimitiveType primitiveType, RwIm2DVertex* vertices,
    RwInt32 numVertices, RwImVertexIndex* indices, RwInt32 numIndices)
{
    /* Indexed traversal and per-primitive grouping match retail. Remaining
     * differences are stock macro scope/register allocation, not GX behavior. */
    RwImVertexIndex* index = indices;
    RwUInt16 count = (RwUInt16)numIndices;

    _rw2DRenderPrimitiveInit();
    GXBegin(_rwDlPrimConvTbl[primitiveType], 0, count);
    switch (primitiveType) {
    case rwPRIMTYPELINELIST: {
        RwInt32 lines = count >> 1;
        if (_RwDlTexture->raster != NULL) {
            while (lines-- != 0) {
                SUBMIT_TEXTURED_INDEXED(vertices, index);
                SUBMIT_TEXTURED_INDEXED(vertices, index);
            }
        } else {
            while (lines-- != 0) {
                SUBMIT_UNTEXTURED_INDEXED(vertices, index);
                SUBMIT_UNTEXTURED_INDEXED(vertices, index);
            }
        }
        break;
    }
    case rwPRIMTYPETRILIST: {
        RwInt32 triangles = count / 3;
        if (_RwDlTexture->raster != NULL) {
            while (triangles-- != 0) {
                SUBMIT_TEXTURED_INDEXED(vertices, index);
                SUBMIT_TEXTURED_INDEXED(vertices, index);
                SUBMIT_TEXTURED_INDEXED(vertices, index);
            }
        } else {
            while (triangles-- != 0) {
                SUBMIT_UNTEXTURED_INDEXED(vertices, index);
                SUBMIT_UNTEXTURED_INDEXED(vertices, index);
                SUBMIT_UNTEXTURED_INDEXED(vertices, index);
            }
        }
        break;
    }
    case rwPRIMTYPEPOLYLINE:
    case rwPRIMTYPETRISTRIP:
    case rwPRIMTYPETRIFAN:
        if (_RwDlTexture->raster != NULL) {
            while (count-- != 0) {
                SUBMIT_TEXTURED_INDEXED(vertices, index);
            }
        } else {
            while (count-- != 0) {
                SUBMIT_UNTEXTURED_INDEXED(vertices, index);
            }
        }
        break;
    default: {
        RwError error;
        error.pluginID = 1;
        error.errorCode = _rwerror(0x25);
        RwErrorSet(&error);
        break;
    }
    }

    GXEnd();
    _rw2DRenderPrimativeTerm();
    return TRUE;
}

static void GXSetTexCoordGen(RwInt32 destination, RwInt32 function,
                             RwInt32 source, RwInt32 matrix)
{
    GXSetTexCoordGen2(destination, function, source, matrix, 0, 0x7D);
}

static void GXEnd(void)
{
}

/* Stock GX immediate-mode helpers write directly to the hardware FIFO. */
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
