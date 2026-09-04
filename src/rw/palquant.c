#include "rw/rwengine.h"
#include "rw/palquant.h"
#include "rw/rwfreelist.h"

typedef unsigned int OctantMap;

static OctantMap splice[256];
static unsigned int QuantDepth = 6;

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

static PalQuantLeaf* InitLeaf(PalQuantLeaf* leaf)
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

static void LeafAddPixel(PalQuantLeaf* leaf, RwRGBA* color,
                         float weight)
{
    RwRGBAReal realColor;
    RwRGBA offset;
    unsigned char mask = (1 << (8 - QuantDepth)) - 1;

    offset.red = color->red & mask;
    offset.green = color->green & mask;
    offset.blue = color->blue & mask;
    offset.alpha = color->alpha & mask;
    realColor.red = (float)offset.red * (1.0f / 255.0f);
    realColor.green = (float)offset.green * (1.0f / 255.0f);
    realColor.blue = (float)offset.blue * (1.0f / 255.0f);
    realColor.alpha = (float)offset.alpha * (1.0f / 255.0f);
    ToMatchSpace(&realColor);
    leaf->weight += weight;
    leaf->variance += weight *
        (realColor.red * realColor.red + realColor.green * realColor.green +
         realColor.blue * realColor.blue + realColor.alpha * realColor.alpha);
    realColor.red *= weight;
    realColor.green *= weight;
    realColor.blue *= weight;
    realColor.alpha *= weight;
    leaf->ac.red += realColor.red;
    leaf->ac.green += realColor.green;
    leaf->ac.blue += realColor.blue;
    leaf->ac.alpha += realColor.alpha;
}

/* Finalizes a leaf's variance and folds in its octree-cell origin. */
static void LeafCalcStats(PalQuantLeaf* leaf, RwRGBA* origin)
{
    RwRGBAReal realColor;

    leaf->variance -=
        (leaf->ac.alpha * leaf->ac.alpha +
         (leaf->ac.blue * leaf->ac.blue +
          (leaf->ac.red * leaf->ac.red +
           leaf->ac.green * leaf->ac.green))) /
        leaf->weight;
    if (leaf->variance < 0.0f) leaf->variance = 0.0f;
    realColor.red = (float)origin->red * (1.0f / 255.0f);
    realColor.green = (float)origin->green * (1.0f / 255.0f);
    realColor.blue = (float)origin->blue * (1.0f / 255.0f);
    realColor.alpha = (float)origin->alpha * (1.0f / 255.0f);
    ToMatchSpace(&realColor);
    realColor.red *= leaf->weight;
    realColor.green *= leaf->weight;
    realColor.blue *= leaf->weight;
    realColor.alpha *= leaf->weight;
    leaf->ac.red += realColor.red;
    leaf->ac.green += realColor.green;
    leaf->ac.blue += realColor.blue;
    leaf->ac.alpha += realColor.alpha;
}

static void StatsAdd(PalQuantLeaf* combined, PalQuantLeaf* first,
                     PalQuantLeaf* second)
{
    combined->variance = first->variance + second->variance;
    if (first->weight > 0.0f && second->weight > 0.0f) {
        RwRGBAReal color1;
        RwRGBAReal color2;
        float reciprocal1 = 1.0f / first->weight;
        float reciprocal2 = 1.0f / second->weight;
        color1.red = first->ac.red * reciprocal1;
        color1.green = first->ac.green * reciprocal1;
        color1.blue = first->ac.blue * reciprocal1;
        color1.alpha = first->ac.alpha * reciprocal1;
        color2.red = second->ac.red * reciprocal2;
        color2.green = second->ac.green * reciprocal2;
        color2.blue = second->ac.blue * reciprocal2;
        color2.alpha = second->ac.alpha * reciprocal2;
        color1.red -= color2.red;
        color1.green -= color2.green;
        color1.blue -= color2.blue;
        color1.alpha -= color2.alpha;
        combined->variance +=
            (color1.red * color1.red + color1.green * color1.green +
             color1.blue * color1.blue + color1.alpha * color1.alpha) /
            (reciprocal1 + reciprocal2);
    }
    combined->ac.red = first->ac.red + second->ac.red;
    combined->ac.green = first->ac.green + second->ac.green;
    combined->ac.blue = first->ac.blue + second->ac.blue;
    combined->ac.alpha = first->ac.alpha + second->ac.alpha;
    combined->weight = first->weight + second->weight;
}

static void StatsSub(PalQuantLeaf* remainder,
                     PalQuantLeaf* whole,
                     PalQuantLeaf* subset)
{
    remainder->weight = whole->weight - subset->weight;
    remainder->ac.red = whole->ac.red - subset->ac.red;
    remainder->ac.green = whole->ac.green - subset->ac.green;
    remainder->ac.blue = whole->ac.blue - subset->ac.blue;
    remainder->ac.alpha = whole->ac.alpha - subset->ac.alpha;
    remainder->variance = whole->variance - subset->variance;
    if (remainder->weight > 0.0f && subset->weight > 0.0f) {
        RwRGBAReal color1;
        RwRGBAReal color2;
        float reciprocal1 = 1.0f / subset->weight;
        float reciprocal2 = 1.0f / remainder->weight;
        color1.red = subset->ac.red * reciprocal1;
        color1.green = subset->ac.green * reciprocal1;
        color1.blue = subset->ac.blue * reciprocal1;
        color1.alpha = subset->ac.alpha * reciprocal1;
        color2.red = remainder->ac.red * reciprocal2;
        color2.green = remainder->ac.green * reciprocal2;
        color2.blue = remainder->ac.blue * reciprocal2;
        color2.alpha = remainder->ac.alpha * reciprocal2;
        color1.red -= color2.red;
        color1.green -= color2.green;
        color1.blue -= color2.blue;
        color1.alpha -= color2.alpha;
        remainder->variance -=
            (color1.red * color1.red + color1.green * color1.green +
             color1.blue * color1.blue + color1.alpha * color1.alpha) /
            (reciprocal1 + reciprocal2);
    }
}

/* Converts a leaf's weighted mean back into a clamped palette color. */
static void RepresentativeColor(RwRGBA* color, PalQuantLeaf* node)
{
    RwRGBAReal realColor;
    int rgba[4];

    realColor.red = node->ac.red * (1.0f / node->weight);
    realColor.green = node->ac.green * (1.0f / node->weight);
    realColor.blue = node->ac.blue * (1.0f / node->weight);
    realColor.alpha = node->ac.alpha * (1.0f / node->weight);
    FromMatchSpace(&realColor);
    rgba[0] = (int)(realColor.red * 255.99f);
    rgba[1] = (int)(realColor.green * 255.99f);
    rgba[2] = (int)(realColor.blue * 255.99f);
    rgba[3] = (int)(realColor.alpha * 255.99f);
    color->red = 255;
    if (rgba[0] < 255) color->red = rgba[0];
    color->green = 255;
    if (rgba[1] < 255) color->green = rgba[1];
    color->blue = 255;
    if (rgba[2] < 255) color->blue = rgba[2];
    color->alpha = 255;
    if (rgba[3] < 255) color->alpha = rgba[3];
}

static PalQuantBranch* InitBranch(PalQuantBranch* branch)
{
    int i;
    for (i = 0; i < 16; i++) branch->dir[i] = 0;
    return branch;
}

static OctantMap GetOctAdr(const RwRGBA* color)
{
    int shift = 8 - QuantDepth;
    return (splice[color->red >> shift] << 3) |
           (splice[color->green >> shift] << 2) |
           (splice[color->blue >> shift] << 1) |
           splice[color->alpha >> shift];
}

static PalQuantNode* AllocateToLeaf(RwPalQuant* quantizer,
                                         PalQuantNode* root,
                                         OctantMap octants, int depth)
{
    if (depth == 0) return root;
    if (root->branch.dir[octants & 15] == 0) {
        PalQuantNode* node = RwEngineInstance->fpFreeListAlloc(
            quantizer->cubeFreeList, 0x30411);
        root->branch.dir[octants & 15] = node;
        InitBranch(&node->branch);
        if (depth == 1) InitLeaf(&node->leaf);
    }
    return AllocateToLeaf(quantizer, root->branch.dir[octants & 15],
                          octants >> 4, depth - 1);
}

/* Accumulates every source pixel into the quantizer's matching octree leaf. */
void RwPalQuantAddImage(RwPalQuant* quantizer, RwImage* image,
                        float weight)
{

    int width;
    int height;
    int stride;
    unsigned char* pixels;
    RwRGBA* palette;

    stride = image->stride;
    pixels = image->pixels;
    palette = (RwRGBA*)image->palette;
    height = image->height;

    switch (image->depth) {
    case 4:
    case 8:
        while (height--) {
            unsigned char* linePixels = pixels;
            width = image->width;
            while (width--) {
                RwRGBA* color = &palette[*linePixels];
                OctantMap octants = GetOctAdr(color);
                PalQuantNode* leaf = AllocateToLeaf(
                    quantizer, quantizer->root, octants, QuantDepth);
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
                OctantMap octants = GetOctAdr(color);
                PalQuantNode* leaf = AllocateToLeaf(
                    quantizer, quantizer->root, octants, QuantDepth);
                LeafAddPixel(&leaf->leaf, color, weight);
                color++;
            }
            pixels += stride;
        }
        break;
    }
}

static void assignindex(PalQuantNode* root, int* origin,
                        int depth, PalQuantColorBox* region,
                        int paletteIndex)
{
    PalQuantColorBox testBox;
    int i;
    int dR;
    int dG;
    int dB;
    int dA;

    if (root == 0) return;
    testBox.col0[0] = origin[0];
    testBox.col0[1] = origin[1];
    testBox.col0[2] = origin[2];
    testBox.col0[3] = origin[3];
    testBox.col1[0] = origin[0] + (1 << depth);
    testBox.col1[1] = origin[1] + (1 << depth);
    testBox.col1[2] = origin[2] + (1 << depth);
    testBox.col1[3] = origin[3] + (1 << depth);
    dR = testBox.col1[0] - region->col0[0];
    dG = testBox.col1[1] - region->col0[1];
    dB = testBox.col1[2] - region->col0[2];
    dA = testBox.col1[3] - region->col0[3];
    if (dR <= 0 || dG <= 0 || dB <= 0 || dA <= 0) return;
    dR = testBox.col0[0] - region->col1[0];
    dG = testBox.col0[1] - region->col1[1];
    dB = testBox.col0[2] - region->col1[2];
    dA = testBox.col0[3] - region->col1[3];
    if (dR >= 0 || dG >= 0 || dB >= 0 || dA >= 0) return;
    if (depth == 0) {
        root->leaf.palIndex = (unsigned char)paletteIndex;
    } else {
        depth--;
        for (i = 0; i < 16; i++) {
            int suborigin[4];
            suborigin[0] = origin[0] + (((i >> 3) & 1) << depth);
            suborigin[1] = origin[1] + (((i >> 2) & 1) << depth);
            suborigin[2] = origin[2] + (((i >> 1) & 1) << depth);
            suborigin[3] = origin[3] + ((i & 1) << depth);
            assignindex(root->branch.dir[i], suborigin, depth, region,
                        paletteIndex);
        }
    }
}

static void AssignPalIndex(PalQuantNode* root,
                           PalQuantColorBox* cube,
                           int paletteIndex)
{
    int origin[4];
    origin[0] = 0;
    origin[1] = 0;
    origin[2] = 0;
    origin[3] = 0;
    assignindex(root, origin, QuantDepth, cube, paletteIndex);
}

static void addvolume(PalQuantNode* root, int* origin,
                      int depth, PalQuantColorBox* region,
                      PalQuantLeaf* volume)
{
    PalQuantColorBox testBox;
    int i;
    int dR;
    int dG;
    int dB;
    int dA;

    if (root == 0) return;
    testBox.col0[0] = origin[0];
    testBox.col0[1] = origin[1];
    testBox.col0[2] = origin[2];
    testBox.col0[3] = origin[3];
    testBox.col1[0] = origin[0] + (1 << depth);
    testBox.col1[1] = origin[1] + (1 << depth);
    testBox.col1[2] = origin[2] + (1 << depth);
    testBox.col1[3] = origin[3] + (1 << depth);
    dR = testBox.col1[0] - region->col0[0];
    dG = testBox.col1[1] - region->col0[1];
    dB = testBox.col1[2] - region->col0[2];
    dA = testBox.col1[3] - region->col0[3];
    if (dR <= 0 || dG <= 0 || dB <= 0 || dA <= 0) return;
    dR = testBox.col0[0] - region->col1[0];
    dG = testBox.col0[1] - region->col1[1];
    dB = testBox.col0[2] - region->col1[2];
    dA = testBox.col0[3] - region->col1[3];
    if (dR >= 0 || dG >= 0 || dB >= 0 || dA >= 0) return;
    dR = testBox.col0[0] - region->col0[0];
    dG = testBox.col0[1] - region->col0[1];
    dB = testBox.col0[2] - region->col0[2];
    dA = testBox.col0[3] - region->col0[3];
    if (dR >= 0 && dG >= 0 && dB >= 0 && dA >= 0) {
        dR = region->col1[0] - testBox.col1[0];
        dG = region->col1[1] - testBox.col1[1];
        dB = region->col1[2] - testBox.col1[2];
        dA = region->col1[3] - testBox.col1[3];
        if (dR >= 0 && dG >= 0 && dB >= 0 && dA >= 0) {
            StatsAdd(volume, volume, &root->leaf);
            return;
        }
    }
    if (depth > 0) {
        depth--;
        for (i = 0; i < 16; i++) {
            int suborigin[4];
            suborigin[0] = origin[0] + (((i >> 3) & 1) << depth);
            suborigin[1] = origin[1] + (((i >> 2) & 1) << depth);
            suborigin[2] = origin[2] + (((i >> 1) & 1) << depth);
            suborigin[3] = origin[3] + ((i & 1) << depth);
            addvolume(root->branch.dir[i], suborigin, depth, region, volume);
        }
    }
}

static PalQuantLeaf* BoxStats(PalQuantLeaf* volume,
                                    PalQuantNode* root,
                                    PalQuantColorBox* cube)
{
    int origin[4];
    origin[0] = 0;
    origin[1] = 0;
    origin[2] = 0;
    origin[3] = 0;
    InitLeaf(volume);
    addvolume(root, origin, QuantDepth, cube, volume);
    return volume;
}

static float FindBestCut(PalQuantNode* root, PalQuantColorBox* cube,
                          int channel, int* cuts,
                          PalQuantLeaf* whole)
{

    PalQuantColorBox leftCube;
    PalQuantLeaf left;
    PalQuantLeaf right;
    float minimum;
    float sum;
    int i;

    minimum = whole->variance;
    leftCube = *cube;
    {
        int min;
        int max;
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

static int nCut(PalQuantNode* root, PalQuantLeaf* whole,
                   PalQuantColorBox* first, PalQuantColorBox* second)
{

    float minimum[4];
    int cuts[4];
    int i;
    int best;

    minimum[0] = FindBestCut(root, first, 0, cuts, whole);
    minimum[1] = FindBestCut(root, first, 1, cuts, whole);
    minimum[2] = FindBestCut(root, first, 2, cuts, whole);
    minimum[3] = FindBestCut(root, first, 3, cuts, whole);
    best = 0;
    for (i = 0; i < 4; i++) {
        if (minimum[i] < minimum[best]) {
            best = i;
        }
    }
    if (minimum[best] < whole->variance) {
        *second = *first;
        first->col1[best] = second->col0[best] = (unsigned char)cuts[best];
        return 1;
    }
    return 0;
}

/* Recursively aggregates leaf statistics through the quantizer octree. */
static PalQuantLeaf* CalcNodeWeights(PalQuantNode* root,
                                           RwRGBA* origin,
                                           int depth)
{
    PalQuantLeaf* leaf = 0;
    if (root != 0) {
        leaf = &root->leaf;
        if (depth > 0) {
            int i;
            InitLeaf(leaf);
            for (i = 0; i < 16; i++) {
                RwRGBA suborigin;
                PalQuantLeaf* subnode;
                unsigned int shift = depth - 1 +
                                 (8 - QuantDepth);
                suborigin.red =
                    origin->red + ((((unsigned int)i >> 3) & 1) << shift);
                suborigin.green =
                    origin->green + ((((unsigned int)i >> 2) & 1) << shift);
                suborigin.blue =
                    origin->blue + ((((unsigned int)i >> 1) & 1) << shift);
                suborigin.alpha =
                    origin->alpha + (((unsigned int)i & 1) << shift);
                subnode = CalcNodeWeights(root->branch.dir[i], &suborigin,
                                          depth - 1);
                if (subnode != 0) StatsAdd(leaf, leaf, subnode);
            }
        } else {
            LeafCalcStats(leaf, origin);
        }
    }
    return leaf;
}

static int CountLeafs(PalQuantNode* root, int depth)
{

    int i;
    int count;

    count = 0;
    if (root != 0) {
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

static int ExtractNodes(PalQuantNode* root, RwRGBA* palette,
                            int nodeIndex, int depth)
{
    if (root != 0) {
        if (depth > 0) {
            int i;
            for (i = 0; i < 16; i++)
                nodeIndex = ExtractNodes(root->branch.dir[i], palette,
                                         nodeIndex, depth - 1);
        } else {
            RepresentativeColor(&palette[nodeIndex], &root->leaf);
            root->leaf.palIndex = (unsigned char)nodeIndex;
            nodeIndex++;
        }
    }
    return nodeIndex;
}

static unsigned char GetIndex(PalQuantNode* root, OctantMap octants,
                        int depth)
{

    unsigned char result;
    if (depth == 0)
        result = root->leaf.palIndex;
    else
        result = GetIndex(root->branch.dir[octants & 15], octants >> 4,
                          depth - 1);
    return result;
}

int RwPalQuantResolvePalette(RwRGBA* palette, int maxColors,
                                 RwPalQuant* quantizer)
{
    int numColors;
    int uniqueColors;
    int i;
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
        quantizer->cubes[0].col0[0] = 0;
        quantizer->cubes[0].col0[1] = 0;
        quantizer->cubes[0].col0[2] = 0;
        quantizer->cubes[0].col0[3] = 0;
        quantizer->cubes[0].col1[0] = 1 << QuantDepth;
        quantizer->cubes[0].col1[1] = 1 << QuantDepth;
        quantizer->cubes[0].col1[2] = 1 << QuantDepth;
        quantizer->cubes[0].col1[3] = 1 << QuantDepth;
        BoxStats(&quantizer->volumes[0], quantizer->root,
                 &quantizer->cubes[0]);
        quantizer->variances[0] = quantizer->volumes[0].variance;
        for (i = 1; i < numColors; i++) {
            int nextSplit = -1;
            int k;
            float maximum = 0.0f;
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

void RwPalQuantMatchImage(unsigned char* destinationPixels,
                          int destinationStride,
                          int destinationDepth, int packed,
                          RwPalQuant* quantizer, RwImage* image)
{

    unsigned int width;
    unsigned int x;
    unsigned int height;
    unsigned int stride = image->stride;
    unsigned char* pixels = image->pixels;
    unsigned char* destination;
    unsigned char nodeIndex;
    OctantMap octants;

    if (destinationDepth == 4 && packed == 1) {
        switch (image->depth) {
        case 4:
        case 8: {
            RwRGBA* palette = (RwRGBA*)image->palette;
            height = image->height;
            while (height--) {
                unsigned char* source = pixels;
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
                unsigned char* source = pixels;
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

int RwPalQuantInit(RwPalQuant* quantizer)
{
    unsigned int i;
    unsigned int j;
    unsigned int maxValue = 1 << QuantDepth;

    for (i = 0; i < maxValue; i++) {
        OctantMap mask = 0;
        for (j = 0; j < QuantDepth; j++)
            mask |= (i & (1 << j))
                        ? 1 << ((QuantDepth - 1 - j) * 4)
                        : 0;
        splice[i] = mask;
    }
    quantizer->cubeFreeList =
        RwFreeListCreate(sizeof(PalQuantNode), 0x400, 4, 0x30411);
    quantizer->root = RwEngineInstance->fpFreeListAlloc(
        quantizer->cubeFreeList, 0x30411);
    InitBranch(&quantizer->root->branch);
    return 1;
}

static void DeleteOctTree(RwPalQuant* quantizer, PalQuantNode* root,
                          int depth)
{

    int i;
    if (root != 0) {
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
    quantizer->root = 0;
    RwFreeListDestroy(quantizer->cubeFreeList);
}
