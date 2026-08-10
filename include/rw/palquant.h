#ifndef RW_PALQUANT_H
#define RW_PALQUANT_H

#include "rw/rwcore_types.h"

typedef struct RwFreeList RwFreeList;

typedef struct RwPalQuantRGBABox {
    RwInt32 col0[4];
    RwInt32 col1[4];
} RwPalQuantRGBABox;

typedef struct RwPalQuantLeafNode {
    RwReal weight;
    RwRGBAReal ac;
    RwReal variance;
    RwUInt8 palIndex;
} RwPalQuantLeafNode;

typedef struct RwPalQuantOctNode RwPalQuantOctNode;

typedef struct RwPalQuantBranchNode {
    RwPalQuantOctNode* dir[16];
} RwPalQuantBranchNode;

struct RwPalQuantOctNode {
    RwPalQuantLeafNode leaf;
    RwPalQuantBranchNode branch;
};

typedef struct RwPalQuant {
    RwPalQuantRGBABox cubes[256];
    RwReal variances[256];
    RwPalQuantLeafNode volumes[256];
    RwPalQuantOctNode* root;
    RwFreeList* cubeFreeList;
} RwPalQuant;

RwBool RwPalQuantInit(RwPalQuant* quantizer);
void RwPalQuantTerm(RwPalQuant* quantizer);
void RwPalQuantAddImage(RwPalQuant* quantizer, RwImage* image,
                        RwReal weight);
RwInt32 RwPalQuantResolvePalette(RwRGBA* palette, RwInt32 maxColors,
                                 RwPalQuant* quantizer);
void RwPalQuantMatchImage(RwUInt8* destinationPixels,
                          RwInt32 destinationStride,
                          RwInt32 destinationDepth, RwBool packed,
                          RwPalQuant* quantizer, RwImage* image);

#endif
