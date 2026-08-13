#ifndef RW_NATIVE_INTERNAL_H
#define RW_NATIVE_INTERNAL_H

typedef union GameCubeNativeDataReference {
    unsigned int offset;
    void* pointer;
} GameCubeNativeDataReference;

typedef struct GameCubeNativeMesh {
    GameCubeNativeDataReference displayList;
    unsigned int displayListSize;
} GameCubeNativeMesh;

typedef struct GameCubeNativeMeshHeader {
    unsigned short token;
    unsigned short field_0x02;
    unsigned int field_0x04;
    unsigned int numMeshes;
    GameCubeNativeMesh meshes[1];
} GameCubeNativeMeshHeader;

typedef struct GameCubeNativeTextureHeader {
    int platform;
    unsigned int filterAddressing;
    int maxAnisotropy;
    int biasClamp;
    int edgeLod;
    float lodBias;
    char name[32];
    char mask[32];
} GameCubeNativeTextureHeader;

typedef struct GameCubeNativeRasterHeader {
    int format;
    unsigned short width;
    unsigned short height;
    unsigned char depth;
    unsigned char numLevels;
    unsigned char tileMode;
    unsigned char paletteFormat;
    int hasAlpha;
} GameCubeNativeRasterHeader;

#endif
