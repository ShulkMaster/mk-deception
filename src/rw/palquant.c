#include "libmkparticle/rw_engine.h"
#include "rw/palquant.h"
#include "rw/rwfreelist.h"

#define RED 0
#define GREEN 1
#define BLUE 2
#define ALPHA 3
#define RWPALQUANT_MAXDEPTH 8
#define RWPALQUANT_MAXCOLOR (1 << RWPALQUANT_MAXDEPTH)

struct RwRGBA {
    RwUInt8 red;
    RwUInt8 green;
    RwUInt8 blue;
    RwUInt8 alpha;
};

typedef RwUInt32 OctantMap;

static OctantMap splice[RWPALQUANT_MAXCOLOR];
static RwUInt32 QuantDepth = 6;

#define ColorLengthSquared(color)                                      \
    ((color)->red * (color)->red + (color)->green * (color)->green +  \
     (color)->blue * (color)->blue + (color)->alpha * (color)->alpha)

#define ColorScale(result, color, scale)        \
    do {                                        \
        (result)->red = (color)->red * (scale); \
        (result)->green =                       \
            (color)->green * (scale);           \
        (result)->blue =                        \
            (color)->blue * (scale);            \
        (result)->alpha =                       \
            (color)->alpha * (scale);           \
    } while (0)

#define ColorAdd(result, first, second)                         \
    do {                                                        \
        (result)->red = (first)->red + (second)->red;           \
        (result)->green = (first)->green + (second)->green;     \
        (result)->blue = (first)->blue + (second)->blue;        \
        (result)->alpha = (first)->alpha + (second)->alpha;     \
    } while (0)

#define ColorSub(result, first, second)                         \
    do {                                                        \
        (result)->red = (first)->red - (second)->red;           \
        (result)->green = (first)->green - (second)->green;     \
        (result)->blue = (first)->blue - (second)->blue;        \
        (result)->alpha = (first)->alpha - (second)->alpha;     \
    } while (0)

#define ColorFromRGBA(result, color)                                  \
    do {                                                              \
        (result)->red = (RwReal)(color)->red * (1.0f / 255.0f);       \
        (result)->green = (RwReal)(color)->green * (1.0f / 255.0f);   \
        (result)->blue = (RwReal)(color)->blue * (1.0f / 255.0f);     \
        (result)->alpha = (RwReal)(color)->alpha * (1.0f / 255.0f);   \
    } while (0)

static void ToMatchSpace(RwRGBAReal* color)
{
    color->red *= 0.5093697f;
    color->blue *= 0.19420783f;
}

static void FromMatchSpace(RwRGBAReal* color)
{
    color->red /= 0.5093697f;
    color->blue /= 0.19420783f;
}

static RwPalQuantLeafNode* InitLeaf(RwPalQuantLeafNode* leaf)
{
    leaf->palIndex = 0;
    leaf->weight = 0.0f;
    leaf->ac.red = 0.0f;
    leaf->ac.green = 0.0f;
    leaf->ac.blue = 0.0f;
    leaf->ac.alpha = 0.0f;
    leaf->variance = 0.0f;
    return leaf;
}

static void LeafAddPixel(RwPalQuantLeafNode* leaf, RwRGBA* color,
                         RwReal weight)
{
    /* Stock color macros match retail; remaining delta is save-helper form. */
    RwRGBAReal realColor;
    RwRGBA offset;
    RwUInt8 mask = (1 << (RWPALQUANT_MAXDEPTH - QuantDepth)) - 1;

    offset.red = color->red & mask;
    offset.green = color->green & mask;
    offset.blue = color->blue & mask;
    offset.alpha = color->alpha & mask;
    ColorFromRGBA(&realColor, &offset);
    ToMatchSpace(&realColor);
    leaf->weight += weight;
    leaf->variance += weight * ColorLengthSquared(&realColor);
    ColorScale(&realColor, &realColor, weight);
    ColorAdd(&leaf->ac, &leaf->ac, &realColor);
}

static void LeafCalcStats(RwPalQuantLeafNode* leaf, RwRGBA* origin)
{
    /* Stock color-macro operations are exact; only local/FPR scheduling differs. */
    RwRGBAReal realColor;

    leaf->variance -= ColorLengthSquared(&leaf->ac) / leaf->weight;
    if (leaf->variance < 0.0f) leaf->variance = 0.0f;
    ColorFromRGBA(&realColor, origin);
    ToMatchSpace(&realColor);
    ColorScale(&realColor, &realColor, leaf->weight);
    ColorAdd(&leaf->ac, &leaf->ac, &realColor);
}

static void StatsAdd(RwPalQuantLeafNode* combined, RwPalQuantLeafNode* first,
                     RwPalQuantLeafNode* second)
{
    /* Retail expands the same color macros with different temporary lifetimes. */
    combined->variance = first->variance + second->variance;
    if (first->weight > 0.0f && second->weight > 0.0f) {
        RwRGBAReal color1;
        RwRGBAReal color2;
        RwReal reciprocal1 = 1.0f / first->weight;
        RwReal reciprocal2 = 1.0f / second->weight;
        ColorScale(&color1, &first->ac, reciprocal1);
        ColorScale(&color2, &second->ac, reciprocal2);
        ColorSub(&color1, &color1, &color2);
        combined->variance +=
            ColorLengthSquared(&color1) / (reciprocal1 + reciprocal2);
    }
    ColorAdd(&combined->ac, &first->ac, &second->ac);
    combined->weight = first->weight + second->weight;
}

static void StatsSub(RwPalQuantLeafNode* remainder,
                     RwPalQuantLeafNode* whole,
                     RwPalQuantLeafNode* subset)
{
    /* Retail expands the same color macros with different temporary lifetimes. */
    remainder->weight = whole->weight - subset->weight;
    ColorSub(&remainder->ac, &whole->ac, &subset->ac);
    remainder->variance = whole->variance - subset->variance;
    if (remainder->weight > 0.0f && subset->weight > 0.0f) {
        RwRGBAReal color1;
        RwRGBAReal color2;
        RwReal reciprocal1 = 1.0f / subset->weight;
        RwReal reciprocal2 = 1.0f / remainder->weight;
        ColorScale(&color1, &subset->ac, reciprocal1);
        ColorScale(&color2, &remainder->ac, reciprocal2);
        ColorSub(&color1, &color1, &color2);
        remainder->variance -=
            ColorLengthSquared(&color1) / (reciprocal1 + reciprocal2);
    }
}

static void RepresentativeColor(RwRGBA* color, RwPalQuantLeafNode* node)
{
    RwRGBAReal realColor;
    RwInt32 rgba[4];

    ColorScale(&realColor, &node->ac, 1.0f / node->weight);
    FromMatchSpace(&realColor);
    rgba[0] = (RwInt32)(realColor.red * 255.99f);
    rgba[1] = (RwInt32)(realColor.green * 255.99f);
    rgba[2] = (RwInt32)(realColor.blue * 255.99f);
    rgba[3] = (RwInt32)(realColor.alpha * 255.99f);
    color->red = 255;
    if (rgba[0] < 255) color->red = rgba[0];
    color->green = 255;
    if (rgba[1] < 255) color->green = rgba[1];
    color->blue = 255;
    if (rgba[2] < 255) color->blue = rgba[2];
    color->alpha = 255;
    if (rgba[3] < 255) color->alpha = rgba[3];
}

static RwPalQuantBranchNode* InitBranch(RwPalQuantBranchNode* branch)
{
    RwInt32 i;
    for (i = 0; i < 16; i++) branch->dir[i] = NULL;
    return branch;
}

static OctantMap GetOctAdr(const RwRGBA* color)
{
    RwInt32 shift = RWPALQUANT_MAXDEPTH - QuantDepth;
    return (splice[color->red >> shift] << 3) |
           (splice[color->green >> shift] << 2) |
           (splice[color->blue >> shift] << 1) |
           splice[color->alpha >> shift];
}

static RwPalQuantOctNode* AllocateToLeaf(RwPalQuant* quantizer,
                                         RwPalQuantOctNode* root,
                                         OctantMap octants, RwInt32 depth)
{
    if (depth == 0) return root;
    if (root->branch.dir[octants & 15] == NULL) {
        RwPalQuantOctNode* node = RwEngineInstance->fpFreeListAlloc(
            quantizer->cubeFreeList, 0x30411);
        root->branch.dir[octants & 15] = node;
        InitBranch(&node->branch);
        if (depth == 1) InitLeaf(&node->leaf);
    }
    return AllocateToLeaf(quantizer, root->branch.dir[octants & 15],
                          octants >> 4, depth - 1);
}

void RwPalQuantAddImage(RwPalQuant* quantizer, RwImage* image,
                        RwReal weight)
{
    /* Retail retains an unused checked-depth local, widening its save range. */
    RwInt32 width;
    RwInt32 height;
    RwInt32 stride;
    RwUInt8* pixels;
    RwRGBA* palette;

    stride = image->stride;
    pixels = image->pixels;
    palette = (RwRGBA*)image->palette;
    height = image->height;

    switch (image->depth) {
    case 4:
    case 8:
        while (height--) {
            RwUInt8* linePixels = pixels;
            width = image->width;
            while (width--) {
                RwRGBA* color = &palette[*linePixels];
                RwPalQuantOctNode* leaf = AllocateToLeaf(
                    quantizer, quantizer->root, GetOctAdr(color), QuantDepth);
                LeafAddPixel(&leaf->leaf, color, weight);
                linePixels++;
            }
            pixels += stride;
        }
        break;
    case 32:
        while (height--) {
            RwRGBA* color = (RwRGBA*)pixels;
            width = image->width;
            while (width--) {
                RwPalQuantOctNode* leaf = AllocateToLeaf(
                    quantizer, quantizer->root, GetOctAdr(color), QuantDepth);
                LeafAddPixel(&leaf->leaf, color, weight);
                color++;
            }
            pixels += stride;
        }
        break;
    }
}

static void assignindex(RwPalQuantOctNode* root, RwInt32* origin,
                        RwInt32 depth, RwPalQuantRGBABox* region,
                        RwInt32 paletteIndex)
{
    RwPalQuantRGBABox testBox;
    RwInt32 i;
    RwInt32 dR;
    RwInt32 dG;
    RwInt32 dB;
    RwInt32 dA;

    if (root == NULL) return;
    testBox.col0[RED] = origin[RED];
    testBox.col0[GREEN] = origin[GREEN];
    testBox.col0[BLUE] = origin[BLUE];
    testBox.col0[ALPHA] = origin[ALPHA];
    testBox.col1[RED] = origin[RED] + (1 << depth);
    testBox.col1[GREEN] = origin[GREEN] + (1 << depth);
    testBox.col1[BLUE] = origin[BLUE] + (1 << depth);
    testBox.col1[ALPHA] = origin[ALPHA] + (1 << depth);
    dR = testBox.col1[RED] - region->col0[RED];
    dG = testBox.col1[GREEN] - region->col0[GREEN];
    dB = testBox.col1[BLUE] - region->col0[BLUE];
    dA = testBox.col1[ALPHA] - region->col0[ALPHA];
    if (dR <= 0 || dG <= 0 || dB <= 0 || dA <= 0) return;
    dR = testBox.col0[RED] - region->col1[RED];
    dG = testBox.col0[GREEN] - region->col1[GREEN];
    dB = testBox.col0[BLUE] - region->col1[BLUE];
    dA = testBox.col0[ALPHA] - region->col1[ALPHA];
    if (dR >= 0 || dG >= 0 || dB >= 0 || dA >= 0) return;
    if (depth == 0) {
        root->leaf.palIndex = (RwUInt8)paletteIndex;
    } else {
        depth--;
        for (i = 0; i < 16; i++) {
            RwInt32 suborigin[4];
            suborigin[RED] = origin[RED] + (((i >> 3) & 1) << depth);
            suborigin[GREEN] = origin[GREEN] + (((i >> 2) & 1) << depth);
            suborigin[BLUE] = origin[BLUE] + (((i >> 1) & 1) << depth);
            suborigin[ALPHA] = origin[ALPHA] + ((i & 1) << depth);
            assignindex(root->branch.dir[i], suborigin, depth, region,
                        paletteIndex);
        }
    }
}

static void AssignPalIndex(RwPalQuantOctNode* root,
                           RwPalQuantRGBABox* cube,
                           RwInt32 paletteIndex)
{
    RwInt32 origin[4];
    origin[RED] = 0;
    origin[GREEN] = 0;
    origin[BLUE] = 0;
    origin[ALPHA] = 0;
    assignindex(root, origin, QuantDepth, cube, paletteIndex);
}

static void addvolume(RwPalQuantOctNode* root, RwInt32* origin,
                      RwInt32 depth, RwPalQuantRGBABox* region,
                      RwPalQuantLeafNode* volume)
{
    RwPalQuantRGBABox testBox;
    RwInt32 i;
    RwInt32 dR;
    RwInt32 dG;
    RwInt32 dB;
    RwInt32 dA;

    if (root == NULL) return;
    testBox.col0[RED] = origin[RED];
    testBox.col0[GREEN] = origin[GREEN];
    testBox.col0[BLUE] = origin[BLUE];
    testBox.col0[ALPHA] = origin[ALPHA];
    testBox.col1[RED] = origin[RED] + (1 << depth);
    testBox.col1[GREEN] = origin[GREEN] + (1 << depth);
    testBox.col1[BLUE] = origin[BLUE] + (1 << depth);
    testBox.col1[ALPHA] = origin[ALPHA] + (1 << depth);
    dR = testBox.col1[RED] - region->col0[RED];
    dG = testBox.col1[GREEN] - region->col0[GREEN];
    dB = testBox.col1[BLUE] - region->col0[BLUE];
    dA = testBox.col1[ALPHA] - region->col0[ALPHA];
    if (dR <= 0 || dG <= 0 || dB <= 0 || dA <= 0) return;
    dR = testBox.col0[RED] - region->col1[RED];
    dG = testBox.col0[GREEN] - region->col1[GREEN];
    dB = testBox.col0[BLUE] - region->col1[BLUE];
    dA = testBox.col0[ALPHA] - region->col1[ALPHA];
    if (dR >= 0 || dG >= 0 || dB >= 0 || dA >= 0) return;
    dR = testBox.col0[RED] - region->col0[RED];
    dG = testBox.col0[GREEN] - region->col0[GREEN];
    dB = testBox.col0[BLUE] - region->col0[BLUE];
    dA = testBox.col0[ALPHA] - region->col0[ALPHA];
    if (dR >= 0 && dG >= 0 && dB >= 0 && dA >= 0) {
        dR = region->col1[RED] - testBox.col1[RED];
        dG = region->col1[GREEN] - testBox.col1[GREEN];
        dB = region->col1[BLUE] - testBox.col1[BLUE];
        dA = region->col1[ALPHA] - testBox.col1[ALPHA];
        if (dR >= 0 && dG >= 0 && dB >= 0 && dA >= 0) {
            StatsAdd(volume, volume, &root->leaf);
            return;
        }
    }
    if (depth > 0) {
        depth--;
        for (i = 0; i < 16; i++) {
            RwInt32 suborigin[4];
            suborigin[RED] = origin[RED] + (((i >> 3) & 1) << depth);
            suborigin[GREEN] = origin[GREEN] + (((i >> 2) & 1) << depth);
            suborigin[BLUE] = origin[BLUE] + (((i >> 1) & 1) << depth);
            suborigin[ALPHA] = origin[ALPHA] + ((i & 1) << depth);
            addvolume(root->branch.dir[i], suborigin, depth, region, volume);
        }
    }
}

static RwPalQuantLeafNode* BoxStats(RwPalQuantLeafNode* volume,
                                    RwPalQuantOctNode* root,
                                    RwPalQuantRGBABox* cube)
{
    RwInt32 origin[4];
    origin[RED] = 0;
    origin[GREEN] = 0;
    origin[BLUE] = 0;
    origin[ALPHA] = 0;
    InitLeaf(volume);
    addvolume(root, origin, QuantDepth, cube, volume);
    return volume;
}

static RwReal FindBestCut(RwPalQuantOctNode* root, RwPalQuantRGBABox* cube,
                          RwInt32 channel, RwInt32* cuts,
                          RwPalQuantLeafNode* whole)
{
    /* The canonical SDK body is recovered; retail homes extra debug locals. */
    RwPalQuantRGBABox leftCube;
    RwPalQuantLeafNode left;
    RwPalQuantLeafNode right;
    RwReal minimum;
    RwReal sum;
    RwInt32 i;

    minimum = whole->variance;
    leftCube = *cube;
    {
        RwInt32 min;
        RwInt32 max;
        min = cube->col0[channel];
        max = cube->col1[channel];
        while (max - min > 1) {
            i = min + ((max - min) >> 1);
            leftCube.col1[channel] = i;
            BoxStats(&left, root, &leftCube);
            StatsSub(&right, whole, &left);
            if (left.weight == 0.0f) {
                min = i;
            } else if (right.weight == 0.0f) {
                max = i;
            } else {
                sum = left.variance + right.variance;
                if (sum < minimum) {
                    minimum = sum;
                    cuts[channel] = leftCube.col1[channel];
                }
                if (left.variance < right.variance)
                    min = i;
                else
                    max = i;
            }
        }
    }
    return minimum;
}

static RwBool nCut(RwPalQuantOctNode* root, RwPalQuantLeafNode* whole,
                   RwPalQuantRGBABox* first, RwPalQuantRGBABox* second)
{
    /* Cut selection is exact; aggregate-copy and stack scheduling account for the residue. */
    RwReal minimum[4];
    RwInt32 cuts[4];
    RwInt32 i;
    RwInt32 best;

    minimum[RED] = FindBestCut(root, first, RED, cuts, whole);
    minimum[GREEN] = FindBestCut(root, first, GREEN, cuts, whole);
    minimum[BLUE] = FindBestCut(root, first, BLUE, cuts, whole);
    minimum[ALPHA] = FindBestCut(root, first, ALPHA, cuts, whole);
    best = 0;
    for (i = 0; i < 4; i++) {
        if (minimum[i] < minimum[best]) {
            best = i;
        }
    }
    if (minimum[best] < whole->variance) {
        *second = *first;
        first->col1[best] = second->col0[best] = (RwUInt8)cuts[best];
        return TRUE;
    }
    return FALSE;
}

static RwPalQuantLeafNode* CalcNodeWeights(RwPalQuantOctNode* root,
                                           RwRGBA* origin,
                                           RwInt32 depth)
{
    RwPalQuantLeafNode* leaf = NULL;
    if (root != NULL) {
        leaf = &root->leaf;
        if (depth > 0) {
            RwInt32 i;
            InitLeaf(leaf);
            for (i = 0; i < 16; i++) {
                RwRGBA suborigin;
                RwPalQuantLeafNode* subnode;
                RwUInt32 shift = depth - 1 +
                                 (RWPALQUANT_MAXDEPTH - QuantDepth);
                suborigin.red = origin->red + (((i >> 3) & 1) << shift);
                suborigin.green = origin->green + (((i >> 2) & 1) << shift);
                suborigin.blue = origin->blue + (((i >> 1) & 1) << shift);
                suborigin.alpha = origin->alpha + ((i & 1) << shift);
                subnode = CalcNodeWeights(root->branch.dir[i], &suborigin,
                                          depth - 1);
                if (subnode != NULL) StatsAdd(leaf, leaf, subnode);
            }
        } else {
            LeafCalcStats(leaf, origin);
        }
    }
    return leaf;
}

static RwInt32 CountLeafs(RwPalQuantOctNode* root, RwInt32 depth)
{
    /* Retail uses helper save/restore for this otherwise identical recursion. */
    RwInt32 i;
    RwInt32 count;

    count = 0;
    if (root != NULL) {
        if (depth > 0) {
            for (i = 0; i < 16; i++) {
                count += CountLeafs(root->branch.dir[i], depth - 1);
            }
        } else {
            count = 1;
        }
    }
    return count;
}

static RwInt32 ExtractNodes(RwPalQuantOctNode* root, RwRGBA* palette,
                            RwInt32 nodeIndex, RwInt32 depth)
{
    if (root != NULL) {
        if (depth > 0) {
            RwInt32 i;
            for (i = 0; i < 16; i++)
                nodeIndex = ExtractNodes(root->branch.dir[i], palette,
                                         nodeIndex, depth - 1);
        } else {
            RepresentativeColor(&palette[nodeIndex], &root->leaf);
            root->leaf.palIndex = (RwUInt8)nodeIndex;
            nodeIndex++;
        }
    }
    return nodeIndex;
}

static RwUInt8 GetIndex(RwPalQuantOctNode* root, OctantMap octants,
                        RwInt32 depth)
{
    /* Retail selects helper saves for this otherwise identical recursion. */
    RwUInt8 result;
    if (depth == 0)
        result = root->leaf.palIndex;
    else
        result = GetIndex(root->branch.dir[octants & 15], octants >> 4,
                          depth - 1);
    return result;
}

RwInt32 RwPalQuantResolvePalette(RwRGBA* palette, RwInt32 maxColors,
                                 RwPalQuant* quantizer)
{
    RwInt32 numColors;
    RwInt32 uniqueColors;
    RwInt32 i;
    RwRGBA origin = { 0, 0, 0, 0 };

    CalcNodeWeights(quantizer->root, &origin, QuantDepth);
    for (i = 0; i < maxColors; i++) {
        palette[i].red = 0;
        palette[i].green = 0;
        palette[i].blue = 0;
        palette[i].alpha = 0;
    }
    numColors = maxColors;
    uniqueColors = CountLeafs(quantizer->root, QuantDepth);
    if (uniqueColors <= numColors) {
        numColors = uniqueColors;
        ExtractNodes(quantizer->root, palette, 0, QuantDepth);
    } else {
        quantizer->cubes[0].col0[RED] = 0;
        quantizer->cubes[0].col0[GREEN] = 0;
        quantizer->cubes[0].col0[BLUE] = 0;
        quantizer->cubes[0].col0[ALPHA] = 0;
        quantizer->cubes[0].col1[RED] = 1 << QuantDepth;
        quantizer->cubes[0].col1[GREEN] = 1 << QuantDepth;
        quantizer->cubes[0].col1[BLUE] = 1 << QuantDepth;
        quantizer->cubes[0].col1[ALPHA] = 1 << QuantDepth;
        BoxStats(&quantizer->volumes[0], quantizer->root,
                 &quantizer->cubes[0]);
        quantizer->variances[0] = quantizer->volumes[0].variance;
        for (i = 1; i < numColors; i++) {
            RwInt32 nextSplit = -1;
            RwInt32 k;
            RwReal maximum = 0.0f;
            for (k = 0; k < i; k++) {
                if (quantizer->variances[k] > maximum) {
                    maximum = quantizer->variances[k];
                    nextSplit = k;
                }
            }
            if (nextSplit == -1) break;
            if (nCut(quantizer->root, &quantizer->volumes[nextSplit],
                     &quantizer->cubes[nextSplit], &quantizer->cubes[i])) {
                BoxStats(&quantizer->volumes[nextSplit], quantizer->root,
                         &quantizer->cubes[nextSplit]);
                BoxStats(&quantizer->volumes[i], quantizer->root,
                         &quantizer->cubes[i]);
                quantizer->variances[nextSplit] =
                    quantizer->volumes[nextSplit].variance;
                quantizer->variances[i] = quantizer->volumes[i].variance;
            } else {
                quantizer->variances[nextSplit] = 0.0f;
                i--;
            }
        }
        for (i = 0; i < numColors; i++) {
            AssignPalIndex(quantizer->root, &quantizer->cubes[i], i);
            RepresentativeColor(&palette[i], &quantizer->volumes[i]);
        }
    }
    return numColors;
}

void RwPalQuantMatchImage(RwUInt8* destinationPixels,
                          RwInt32 destinationStride,
                          RwInt32 destinationDepth, RwBool packed,
                          RwPalQuant* quantizer, RwImage* image)
{
    /* Retail retains an assertion-only max-color local; clean source omits it. */
    RwUInt32 width;
    RwUInt32 x;
    RwUInt32 height;
    RwUInt32 stride = image->stride;
    RwUInt8* pixels = image->pixels;
    RwUInt8* destination;
    RwUInt8 nodeIndex;
    OctantMap octants;

    if (destinationDepth == 4 && packed == TRUE) {
        switch (image->depth) {
        case 4:
        case 8: {
            RwRGBA* palette = (RwRGBA*)image->palette;
            height = image->height;
            while (height--) {
                RwUInt8* source = pixels;
                destination = destinationPixels;
                width = image->width;
                for (x = 0; x < width; x++) {
                    RwRGBA* color = &palette[*source++];
                    octants = GetOctAdr(color);
                    nodeIndex =
                        GetIndex(quantizer->root, octants, QuantDepth);
                    if (x & 1) {
                        *destination &= 0x0F;
                        *destination |= (nodeIndex & 0x0F) << 4;
                        destination++;
                    } else {
                        *destination &= 0xF0;
                        *destination |= nodeIndex & 0x0F;
                    }
                }
                pixels += stride;
                destinationPixels += destinationStride;
            }
            break;
        }
        case 32:
            height = image->height;
            while (height--) {
                RwRGBA* source = (RwRGBA*)pixels;
                destination = destinationPixels;
                width = image->width;
                for (x = 0; x < width; x++) {
                    RwRGBA* color = source++;
                    octants = GetOctAdr(color);
                    nodeIndex =
                        GetIndex(quantizer->root, octants, QuantDepth);
                    if (x & 1) {
                        *destination &= 0x0F;
                        *destination |= (nodeIndex & 0x0F) << 4;
                        destination++;
                    } else {
                        *destination &= 0xF0;
                        *destination |= nodeIndex & 0x0F;
                    }
                }
                pixels += stride;
                destinationPixels += destinationStride;
            }
            break;
        }
    } else {
        switch (image->depth) {
        case 4:
        case 8: {
            RwRGBA* palette = (RwRGBA*)image->palette;
            height = image->height;
            while (height--) {
                RwUInt8* source = pixels;
                destination = destinationPixels;
                width = image->width;
                while (width--) {
                    RwRGBA* color = &palette[*source++];
                    octants = GetOctAdr(color);
                    nodeIndex =
                        GetIndex(quantizer->root, octants, QuantDepth);
                    *destination++ = nodeIndex;
                }
                pixels += stride;
                destinationPixels += destinationStride;
            }
            break;
        }
        case 32:
            height = image->height;
            while (height--) {
                RwRGBA* source = (RwRGBA*)pixels;
                destination = destinationPixels;
                width = image->width;
                while (width--) {
                    RwRGBA* color = source++;
                    octants = GetOctAdr(color);
                    nodeIndex =
                        GetIndex(quantizer->root, octants, QuantDepth);
                    *destination++ = nodeIndex;
                }
                pixels += stride;
                destinationPixels += destinationStride;
            }
            break;
        }
    }
}

RwBool RwPalQuantInit(RwPalQuant* quantizer)
{
    RwUInt32 i;
    RwUInt32 j;
    RwUInt32 maxValue = 1 << QuantDepth;

    for (i = 0; i < maxValue; i++) {
        OctantMap mask = 0;
        for (j = 0; j < QuantDepth; j++)
            mask |= (i & (1 << j))
                        ? 1 << ((QuantDepth - 1 - j) * 4)
                        : 0;
        splice[i] = mask;
    }
    quantizer->cubeFreeList =
        RwFreeListCreate(sizeof(RwPalQuantOctNode), 0x400, 4, 0x30411);
    quantizer->root = RwEngineInstance->fpFreeListAlloc(
        quantizer->cubeFreeList, 0x30411);
    InitBranch(&quantizer->root->branch);
    return TRUE;
}

static void DeleteOctTree(RwPalQuant* quantizer, RwPalQuantOctNode* root,
                          RwInt32 depth)
{
    /* Retail uses helper save/restore for this otherwise identical recursion. */
    RwInt32 i;
    if (root != NULL) {
        if (depth > 0) {
            for (i = 0; i < 16; i++) {
                DeleteOctTree(quantizer, root->branch.dir[i], depth - 1);
            }
        }
        RwEngineInstance->fpFreeListFree(quantizer->cubeFreeList, root);
    }
}

void RwPalQuantTerm(RwPalQuant* quantizer)
{
    DeleteOctTree(quantizer, quantizer->root, QuantDepth);
    quantizer->root = NULL;
    RwFreeListDestroy(quantizer->cubeFreeList);
}
