#include "dolphin/gx.h"
#include "rw/gamecube.h"
#include "rw/rwerror.h"
#include "rw/rwim3d.h"
#include "rw/rxpipeline.h"

typedef struct RwIm3DPool {
    RwIm3DTransformData transformData;
    RwIm3DStash stash;
} RwIm3DPool;

static RwIm3DPool* _rwDlImmPool;

static RwInt32 _rwDlPrimConvTbl[7] = {
    0, 0xA8, 0xB0, 0x90, 0x98, 0xA0, 0xB8
};

static void GXSetTexCoordGen(RwInt32 destination, RwInt32 function,
                             RwInt32 source, RwInt32 matrix);
static void GXEnd(void);
static void GXTexCoord2f32(RwReal s, RwReal t);
static void GXColor4u8(RwUInt8 red, RwUInt8 green, RwUInt8 blue,
                       RwUInt8 alpha);
static void GXPosition3f32(RwReal x, RwReal y, RwReal z);

static RwBool _rwDlImmInstanceNode(
    RxPipelineNode* self, const RxPipelineNodeParam* params)
{
    _rwDlImmPool = params->dataParam;
    return TRUE;
}

static RxNodeDefinition nodeImmInstanceCSL = {
    "ImmInstance.csl",
    {_rwDlImmInstanceNode, NULL, NULL, NULL, NULL, NULL, NULL},
    {0, NULL, NULL, 0, NULL},
    0,
    FALSE,
    0
};

RxNodeDefinition* RxNodeDefinitionGetGameCubeImmInstance(void)
{
    return &nodeImmInstanceCSL;
}

static void _rw3DRenderPrimitiveInit(const RwIm3DStash* stash)
{
    _rwDlTransformSetup(stash->localToWorld, FALSE);
    GXClearVtxDesc();
    GXSetVtxDesc(9, 1);
    GXSetVtxAttrFmt(0, 9, 1, 4, 0);
    GXSetVtxDesc(0xB, 1);
    GXSetVtxAttrFmt(0, 0xB, 1, 5, 0);
    GXSetNumTevStages(1);
    GXSetNumChans(1);
    GXSetChanCtrl(4, 0, 0, 1, 0, 0, 2);
    GXSetChanCtrl(5, 0, 0, 0, 0, 0, 2);
    if ((stash->flags & 1) != 0) {
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
}

#define SUBMIT_VERTEX(vertex, textured)                                   \
    do {                                                                  \
        GXPosition3f32((vertex)->position.x, (vertex)->position.y,         \
                        (vertex)->position.z);                             \
        GXColor4u8((vertex)->color_channels.red,                           \
                   (vertex)->color_channels.green,                        \
                   (vertex)->color_channels.blue,                         \
                   (vertex)->color_channels.alpha);                       \
        if (textured) {                                                    \
            GXTexCoord2f32((vertex)->texCoords.x, (vertex)->texCoords.y);  \
        }                                                                 \
    } while (0)

#define SUBMIT_INDEXED_VERTEX(vertices, indices, textured)                \
    do {                                                                  \
        RwIm3DVertex* vertex = &(vertices)[*(indices)++];                  \
        SUBMIT_VERTEX(vertex, textured);                                  \
    } while (0)

#define SUBMIT_DIRECT_VERTEX(vertices, textured)                          \
    do {                                                                  \
        SUBMIT_VERTEX(vertices, textured);                                \
        (vertices)++;                                                     \
    } while (0)

static RwBool DlSubmitNode(
    RxPipelineNode* self, const RxPipelineNodeParam* params)
{
    RwIm3DStash* stash = &_rwDlImmPool->stash;
    RwIm3DVertex* vertices = _rwDlImmPool->transformData.vertices;
    const RwImVertexIndex* indices = stash->renderData.indices;
    RwUInt16 count;

    if (indices != NULL) {
        _rw3DRenderPrimitiveInit(stash);
        count = (RwUInt16)stash->renderData.numIndices;
        GXBegin(_rwDlPrimConvTbl[stash->renderData.primitiveType], 0, count);
        switch (stash->renderData.primitiveType) {
        case rwPRIMTYPELINELIST:
            count = (RwUInt16)(count >> 1);
            if ((_rwDlImmPool->stash.flags & 1) != 0) {
                while (count-- != 0) {
                    SUBMIT_INDEXED_VERTEX(vertices, indices, TRUE);
                    SUBMIT_INDEXED_VERTEX(vertices, indices, TRUE);
                }
            } else {
                while (count-- != 0) {
                    SUBMIT_INDEXED_VERTEX(vertices, indices, FALSE);
                    SUBMIT_INDEXED_VERTEX(vertices, indices, FALSE);
                }
            }
            break;
        case rwPRIMTYPETRILIST:
            count = (RwUInt16)(count / 3);
            if ((_rwDlImmPool->stash.flags & 1) != 0) {
                while (count-- != 0) {
                    SUBMIT_INDEXED_VERTEX(vertices, indices, TRUE);
                    SUBMIT_INDEXED_VERTEX(vertices, indices, TRUE);
                    SUBMIT_INDEXED_VERTEX(vertices, indices, TRUE);
                }
            } else {
                while (count-- != 0) {
                    SUBMIT_INDEXED_VERTEX(vertices, indices, FALSE);
                    SUBMIT_INDEXED_VERTEX(vertices, indices, FALSE);
                    SUBMIT_INDEXED_VERTEX(vertices, indices, FALSE);
                }
            }
            break;
        case rwPRIMTYPEPOLYLINE:
        case rwPRIMTYPETRISTRIP:
        case rwPRIMTYPETRIFAN:
            if ((_rwDlImmPool->stash.flags & 1) != 0) {
                while (count-- != 0) {
                    SUBMIT_INDEXED_VERTEX(vertices, indices, TRUE);
                }
            } else {
                while (count-- != 0) {
                    SUBMIT_INDEXED_VERTEX(vertices, indices, FALSE);
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
    } else {
        _rw3DRenderPrimitiveInit(stash);
        count = _rwDlImmPool->transformData.numVertices;
        GXBegin(_rwDlPrimConvTbl[stash->renderData.primitiveType], 0, count);
        switch (stash->renderData.primitiveType) {
        case rwPRIMTYPELINELIST:
            count = (RwUInt16)(count >> 1);
            if ((_rwDlImmPool->stash.flags & 1) != 0) {
                while (count-- != 0) {
                    SUBMIT_DIRECT_VERTEX(vertices, TRUE);
                    SUBMIT_DIRECT_VERTEX(vertices, TRUE);
                }
            } else {
                while (count-- != 0) {
                    SUBMIT_DIRECT_VERTEX(vertices, FALSE);
                    SUBMIT_DIRECT_VERTEX(vertices, FALSE);
                }
            }
            break;
        case rwPRIMTYPETRILIST:
            count = (RwUInt16)(count / 3);
            if ((_rwDlImmPool->stash.flags & 1) != 0) {
                while (count-- != 0) {
                    SUBMIT_DIRECT_VERTEX(vertices, TRUE);
                    SUBMIT_DIRECT_VERTEX(vertices, TRUE);
                    SUBMIT_DIRECT_VERTEX(vertices, TRUE);
                }
            } else {
                while (count-- != 0) {
                    SUBMIT_DIRECT_VERTEX(vertices, FALSE);
                    SUBMIT_DIRECT_VERTEX(vertices, FALSE);
                    SUBMIT_DIRECT_VERTEX(vertices, FALSE);
                }
            }
            break;
        case rwPRIMTYPEPOLYLINE:
        case rwPRIMTYPETRISTRIP:
        case rwPRIMTYPETRIFAN:
            if ((_rwDlImmPool->stash.flags & 1) != 0) {
                while (count-- != 0) {
                    SUBMIT_DIRECT_VERTEX(vertices, TRUE);
                }
            } else {
                while (count-- != 0) {
                    SUBMIT_DIRECT_VERTEX(vertices, FALSE);
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
    }
    GXEnd();
    return TRUE;
}

static RxNodeDefinition nodeDlSubmitNoLightCSL = {
    "SubmitNoLight.csl",
    {DlSubmitNode, NULL, NULL, NULL, NULL, NULL, NULL},
    {0, NULL, NULL, 0, NULL},
    0,
    FALSE,
    0
};

RxNodeDefinition* RxNodeDefinitionGetGameCubeSubmitNoLight(void)
{
    return &nodeDlSubmitNoLightCSL;
}

static void GXSetTexCoordGen(RwInt32 destination, RwInt32 function,
                             RwInt32 source, RwInt32 matrix)
{
    GXSetTexCoordGen2(destination, function, source, matrix, 0, 0x7D);
}

static void GXEnd(void)
{
}

/* These stock GX immediate helpers write directly to the hardware FIFO. */
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
