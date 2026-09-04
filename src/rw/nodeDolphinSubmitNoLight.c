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

static int _rwDlPrimConvTbl[7] = {
    0, 0xA8, 0xB0, 0x90, 0x98, 0xA0, 0xB8
};

static void GXEnd(void);
static void GXTexCoord2f32(float s, float t);
static void GXColor4u8(unsigned char red, unsigned char green, unsigned char blue,
                       unsigned char alpha);
static void GXPosition3f32(float x, float y, float z);

#define rwSubmitVertexColor(vertex)                                           \
    do {                                                                       \
        GXPosition3f32((vertex)->position.x, (vertex)->position.y,             \
                       (vertex)->position.z);                                  \
        GXColor4u8((vertex)->color_channels.red,                               \
                   (vertex)->color_channels.green,                             \
                   (vertex)->color_channels.blue,                              \
                   (vertex)->color_channels.alpha);                            \
    } while (0)

#define rwSubmitTexturedVertex(vertex)                                        \
    do {                                                                       \
        rwSubmitVertexColor(vertex);                                          \
        GXTexCoord2f32((vertex)->texCoords.x, (vertex)->texCoords.y);          \
    } while (0)

static int _rwDlImmInstanceNode(
    RxPipelineNode* self, const RxPipelineNodeParam* params)
{
    _rwDlImmPool = params->dataParam;
    return 1;
}

static RxNodeDefinition nodeImmInstanceCSL = {
    "ImmInstance.csl",
    {_rwDlImmInstanceNode, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    0,
    0,
    0
};

RxNodeDefinition* RxNodeDefinitionGetGameCubeImmInstance(void)
{
    return &nodeImmInstanceCSL;
}

static void _rw3DRenderPrimitiveInit(const RwIm3DStash* stash)
{
    _rwDlTransformSetup(stash->localToWorld, 0);
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


/* Submit transformed immediate-mode vertices to GX, with optional texcoords. */
/* TODO: Retail keeps the two triangle divisors in GPRs and materializes NULL;
 * this O0 build spills the clean divisor locals and compares NULL immediately. */
static int DlSubmitNode(
    RxPipelineNode* self, const RxPipelineNodeParam* params)
{
    RwIm3DStash* stash = &_rwDlImmPool->stash;
    RwIm3DVertex* vertices = _rwDlImmPool->transformData.vertices;
    if (NULL != stash->renderData.indices) {
        const RwImVertexIndex* indices = stash->renderData.indices;

        _rw3DRenderPrimitiveInit(stash);
        GXBegin(_rwDlPrimConvTbl[stash->renderData.primitiveType], 0,
                (unsigned short)stash->renderData.numIndices);

        switch (stash->renderData.primitiveType) {
        case rwPRIMTYPEPOLYLINE:
        case rwPRIMTYPETRISTRIP:
        case rwPRIMTYPETRIFAN: {
            unsigned short count = (unsigned short)stash->renderData.numIndices;
            if ((stash->flags & 1) != 0) {
                while (count-- != 0) {
                    RwIm3DVertex* vertex = &vertices[*indices];
                    rwSubmitTexturedVertex(vertex);
                    indices++;
                }
            } else {
                while (count-- != 0) {
                    RwIm3DVertex* vertex = &vertices[*indices];
                    rwSubmitVertexColor(vertex);
                    indices++;
                }
            }
            break;
        }
        case rwPRIMTYPETRILIST: {
            unsigned int verticesPerTriangle = 3;
            unsigned short primitiveCount =
                (unsigned short)((unsigned int)stash->renderData.numIndices /
                                 verticesPerTriangle);
            if ((stash->flags & 1) != 0) {
                while (primitiveCount-- != 0) {
                    RwIm3DVertex* vertex = &vertices[*indices];
                    rwSubmitTexturedVertex(vertex);
                    indices++;
                    vertex = &vertices[*indices];
                    rwSubmitTexturedVertex(vertex);
                    indices++;
                    vertex = &vertices[*indices];
                    rwSubmitTexturedVertex(vertex);
                    indices++;
                }
            } else {
                while (primitiveCount-- != 0) {
                    RwIm3DVertex* vertex = &vertices[*indices];
                    rwSubmitVertexColor(vertex);
                    indices++;
                    vertex = &vertices[*indices];
                    rwSubmitVertexColor(vertex);
                    indices++;
                    vertex = &vertices[*indices];
                    rwSubmitVertexColor(vertex);
                    indices++;
                }
            }
            break;
        }
        case rwPRIMTYPELINELIST: {
            unsigned short primitiveCount =
                (unsigned short)((unsigned int)stash->renderData.numIndices >> 1);
            if ((stash->flags & 1) != 0) {
                while (primitiveCount-- != 0) {
                    RwIm3DVertex* vertex = &vertices[*indices];
                    rwSubmitTexturedVertex(vertex);
                    indices++;
                    vertex = &vertices[*indices];
                    rwSubmitTexturedVertex(vertex);
                    indices++;
                }
            } else {
                while (primitiveCount-- != 0) {
                    RwIm3DVertex* vertex = &vertices[*indices];
                    rwSubmitVertexColor(vertex);
                    indices++;
                    vertex = &vertices[*indices];
                    rwSubmitVertexColor(vertex);
                    indices++;
                }
            }
            break;
        }
        default: {
            RwError error;
            error.pluginID = 1;
            error.errorCode = _rwerror(0x25);
            RwErrorSet(&error);
            break;
        }
        }
        GXEnd();
    } else {
        _rw3DRenderPrimitiveInit(stash);
        GXBegin(_rwDlPrimConvTbl[stash->renderData.primitiveType], 0,
                _rwDlImmPool->transformData.numVertices);

        switch (stash->renderData.primitiveType) {
        case rwPRIMTYPEPOLYLINE:
        case rwPRIMTYPETRISTRIP:
        case rwPRIMTYPETRIFAN: {
            unsigned short count = _rwDlImmPool->transformData.numVertices;
            if ((stash->flags & 1) != 0) {
                while (count-- != 0) {
                    rwSubmitTexturedVertex(vertices);
                    vertices++;
                }
            } else {
                while (count-- != 0) {
                    rwSubmitVertexColor(vertices);
                    vertices++;
                }
            }
            break;
        }
        case rwPRIMTYPETRILIST: {
            int verticesPerTriangle = 3;
            unsigned short primitiveCount =
                _rwDlImmPool->transformData.numVertices / verticesPerTriangle;
            if ((stash->flags & 1) != 0) {
                while (primitiveCount-- != 0) {
                    rwSubmitTexturedVertex(vertices);
                    vertices++;
                    rwSubmitTexturedVertex(vertices);
                    vertices++;
                    rwSubmitTexturedVertex(vertices);
                    vertices++;
                }
            } else {
                while (primitiveCount-- != 0) {
                    rwSubmitVertexColor(vertices);
                    vertices++;
                    rwSubmitVertexColor(vertices);
                    vertices++;
                    rwSubmitVertexColor(vertices);
                    vertices++;
                }
            }
            break;
        }
        case rwPRIMTYPELINELIST: {
            unsigned short primitiveCount =
                _rwDlImmPool->transformData.numVertices >> 1;
            if ((stash->flags & 1) != 0) {
                while (primitiveCount-- != 0) {
                    rwSubmitTexturedVertex(vertices);
                    vertices++;
                    rwSubmitTexturedVertex(vertices);
                    vertices++;
                }
            } else {
                while (primitiveCount-- != 0) {
                    rwSubmitVertexColor(vertices);
                    vertices++;
                    rwSubmitVertexColor(vertices);
                    vertices++;
                }
            }
            break;
        }
        default: {
            RwError error;
            error.pluginID = 1;
            error.errorCode = _rwerror(0x25);
            RwErrorSet(&error);
            break;
        }
        }
        GXEnd();
    }
    return 1;
}

#undef rwSubmitTexturedVertex
#undef rwSubmitVertexColor

static RxNodeDefinition nodeDlSubmitNoLightCSL = {
    "SubmitNoLight.csl",
    {DlSubmitNode, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    0,
    0,
    0
};

RxNodeDefinition* RxNodeDefinitionGetGameCubeSubmitNoLight(void)
{
    return &nodeDlSubmitNoLightCSL;
}

static void GXEnd(void)
{
}


static void GXTexCoord2f32(float s, float t)
{
    *(volatile float*)GXFIFO_ADDR = s;
    *(volatile float*)GXFIFO_ADDR = t;
}

static void GXColor4u8(unsigned char red, unsigned char green, unsigned char blue,
                       unsigned char alpha)
{
    *(volatile unsigned char*)GXFIFO_ADDR = red;
    *(volatile unsigned char*)GXFIFO_ADDR = green;
    *(volatile unsigned char*)GXFIFO_ADDR = blue;
    *(volatile unsigned char*)GXFIFO_ADDR = alpha;
}

static void GXPosition3f32(float x, float y, float z)
{
    *(volatile float*)GXFIFO_ADDR = x;
    *(volatile float*)GXFIFO_ADDR = y;
    *(volatile float*)GXFIFO_ADDR = z;
}
