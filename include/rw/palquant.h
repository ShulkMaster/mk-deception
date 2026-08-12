#ifndef RW_PALQUANT_H
#define RW_PALQUANT_H

#include "rw/rwcore_types.h"

typedef struct RwFreeList RwFreeList;

typedef struct PalQuantColorBox {
    int col0[4];
    int col1[4];
} PalQuantColorBox;

typedef struct PalQuantLeaf {
    float weight;
    RwRGBAReal ac;
    float variance;
    unsigned char palIndex;
} PalQuantLeaf;

typedef struct PalQuantNode PalQuantNode;

typedef struct PalQuantBranch {
    PalQuantNode* dir[16];
} PalQuantBranch;

struct PalQuantNode {
    PalQuantLeaf leaf;
    PalQuantBranch branch;
};

typedef struct RwPalQuant {
    PalQuantColorBox cubes[256];
    float variances[256];
    PalQuantLeaf volumes[256];
    PalQuantNode* root;
    RwFreeList* cubeFreeList;
} RwPalQuant;

int RwPalQuantInit(RwPalQuant* quantizer);
void RwPalQuantTerm(RwPalQuant* quantizer);
void RwPalQuantAddImage(RwPalQuant* quantizer, RwImage* image,
                        float weight);
int RwPalQuantResolvePalette(RwRGBA* palette, int maxColors,
                                 RwPalQuant* quantizer);
void RwPalQuantMatchImage(unsigned char* destinationPixels,
                          int destinationStride,
                          int destinationDepth, int packed,
                          RwPalQuant* quantizer, RwImage* image);

#endif
