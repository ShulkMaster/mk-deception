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


static RwBool DlSubmitNode(
    RxPipelineNode* self, const RxPipelineNodeParam* params)
{
    RwIm3DStash* stash = &_rwDlImmPool->stash;
    RwIm3DVertex* vertices = _rwDlImmPool->transformData.vertices;
    const RwImVertexIndex* indices = stash->renderData.indices;
    RwUInt16 count;
    RwUInt16 verticesPerPrimitive;
    RwBool textured;

    _rw3DRenderPrimitiveInit(stash);
    if (indices != 0) {
        count = (RwUInt16)stash->renderData.numIndices;
    } else {
        count = _rwDlImmPool->transformData.numVertices;
    }
    GXBegin(_rwDlPrimConvTbl[stash->renderData.primitiveType], 0, count);

    switch (stash->renderData.primitiveType) {
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

    textured = (stash->flags & 1) != 0;
    if (verticesPerPrimitive != 0) {
        RwUInt16 primitiveCount = count / verticesPerPrimitive;

        while (primitiveCount-- != 0) {
            RwUInt16 vertexCount = verticesPerPrimitive;

            while (vertexCount-- != 0) {
                RwIm3DVertex* vertex;

                if (indices != 0) {
                    vertex = &vertices[*indices++];
                } else {
                    vertex = vertices++;
                }
                GXPosition3f32(vertex->position.x, vertex->position.y,
                               vertex->position.z);
                GXColor4u8(vertex->color_channels.red,
                           vertex->color_channels.green,
                           vertex->color_channels.blue,
                           vertex->color_channels.alpha);
                if (textured) {
                    GXTexCoord2f32(vertex->texCoords.x, vertex->texCoords.y);
                }
            }
        }
    }
    GXEnd();
    return 1;
}

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
