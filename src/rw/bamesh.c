#include "libmkparticle/rw_engine.h"
#include "rw/rpworld_types.h"
#include "rw/rwerror.h"
#include "rw/rwfreelist.h"
#include "rw/rwstream.h"

typedef struct RpMeshGlobals {
    RwInt16 nextSerialNum;
    RwUInt16 reserved02;
    RwFreeList* triStripListEntryFreeList;
    RwUInt8 meshFlags[0x20];
    RwUInt8 primitiveType[6];
} RpMeshGlobals;

typedef struct RpMeshObjectHeader {
    RwUInt8 type;
    RwUInt8 reserved01[7];
    RwUInt32 flags;
} RpMeshObjectHeader;

typedef struct RpBinMeshHeader {
    RwInt32 flags;
    RwInt32 numMeshes;
    RwInt32 totalIndices;
} RpBinMeshHeader;

typedef struct RpBinMesh {
    RwInt32 numIndices;
    RwInt32 materialIndex;
} RpBinMesh;

typedef struct RpMeshStatic {
    RwFreeList* buildMeshFreeList;
} RpMeshStatic;

static RpMeshStatic MeshStatic = {0};
RwModuleInfo meshModule;

static RpMeshGlobals* MeshGlobals(void)
{
    return (RpMeshGlobals*)((RwUInt8*)RwEngineInstance +
                            meshModule.globalsOffset);
}

static RwBool MeshObjectHasIndices(const void* object)
{
    const RpMeshObjectHeader* header = object;
    return (header->type == 8 || header->type == 7) &&
           !(header->flags & 0x01000000);
}

static void MeshFreeListsDestroy(void)
{
    if (MeshStatic.buildMeshFreeList != 0) {
        RwFreeListDestroy(MeshStatic.buildMeshFreeList);
        MeshStatic.buildMeshFreeList = 0;
    }
}

static RwBool MeshFreeListsCreate(void)
{


    RwBool result;
    MeshStatic.buildMeshFreeList =
        RwFreeListCreate(sizeof(RpBuildMesh), 50, 4, 0x40502);
    result = MeshStatic.buildMeshFreeList != 0;
    return result;
}

void _rpMeshHeaderDestroy(RpMeshHeader* meshHeader)
{

    RwEngineInstance->fpFree(meshHeader);
}

RpMeshHeader* _rpMeshHeaderCreate(RwUInt32 size)
{
    RpMeshHeader* meshHeader = RwEngineInstance->fpMalloc(size, 0x30502);
    return meshHeader;
}

void* _rpMeshClose(void* instance, RwInt32 offset, RwInt32 size)
{
    meshModule.numInstances--;
    if (meshModule.numInstances == 0) {
        MeshFreeListsDestroy();
    }
    return instance;
}

void* _rpMeshOpen(void* instance, RwInt32 offset, RwInt32 size)
{
    meshModule.globalsOffset = offset;
    if (meshModule.numInstances == 0 && !MeshFreeListsCreate()) {
        MeshFreeListsDestroy();
        instance = 0;
        return instance;
    }
    MeshGlobals()->nextSerialNum = 1;
    meshModule.numInstances++;
    MeshGlobals()->meshFlags[0] = 3;
    MeshGlobals()->meshFlags[1] = 4;
    MeshGlobals()->meshFlags[2] = 5;
    MeshGlobals()->meshFlags[4] = 1;
    MeshGlobals()->meshFlags[8] = 2;
    MeshGlobals()->meshFlags[0x10] = 6;
    MeshGlobals()->primitiveType[0] = 4;
    MeshGlobals()->primitiveType[1] = 8;
    MeshGlobals()->primitiveType[2] = 0;
    MeshGlobals()->primitiveType[3] = 1;
    MeshGlobals()->primitiveType[4] = 2;
    MeshGlobals()->primitiveType[5] = 0x10;
    return instance;
}

RpBuildMesh* _rpBuildMeshCreate(RwUInt32 bufferSize)
{

    RpBuildMesh* mesh =
        RwEngineInstance->fpFreeListAlloc(MeshStatic.buildMeshFreeList, 0x30502);

    if (mesh != 0) {
        mesh->numTriangles = 0;
        if (bufferSize != 0) {
            RwUInt32 size = bufferSize * sizeof(RpBuildMeshTriangle);
            mesh->meshTriangles = RwEngineInstance->fpMalloc(size, 0x01030502);
            if (mesh->meshTriangles == 0) {
                RwError error;
                RwEngineInstance->fpFreeListFree(MeshStatic.buildMeshFreeList, mesh);
                error.pluginID = 2;
                error.errorCode = _rwerror(0x80000013, size);
                RwErrorSet(&error);
                return 0;
            }
            mesh->triangleBufferSize = bufferSize;
        } else {
            mesh->meshTriangles = 0;
            mesh->triangleBufferSize = 0;
        }
        return mesh;
    }
    {
        RwError error;
        error.pluginID = 2;
        error.errorCode = _rwerror(0x80000013, sizeof(RpBuildMesh));
        RwErrorSet(&error);
    }
    return 0;
}

RwBool _rpBuildMeshDestroy(RpBuildMesh* mesh)
{

    if (mesh->meshTriangles != 0) {
        RwEngineInstance->fpFree(mesh->meshTriangles);
        mesh->meshTriangles = 0;
    }
    RwEngineInstance->fpFreeListFree(MeshStatic.buildMeshFreeList, mesh);
    return 1;
}

RwBool _rpMeshDestroy(RpMeshHeader* meshHeader)
{
    if (meshHeader->flags != 0 || meshHeader->numMeshes != 0 ||
        meshHeader->serialNum != 0 || meshHeader->totalIndices != 0 ||
        meshHeader->firstMeshOffset != 0) {
        _rpMeshHeaderDestroy(meshHeader);
    }
    return 1;
}






RpBuildMesh* _rpBuildMeshAddTriangle(
    RpBuildMesh* mesh, RpMaterial* material, RwInt32 vert1, RwInt32 vert2,
    RwInt32 vert3, RwUInt16 matIndex, RwUInt16 textureIndex,
RwUInt16 rasterIndex, RwUInt16 pipelineIndex)
{
    RpBuildMeshTriangle* triangle;

    if (mesh->numTriangles >= mesh->triangleBufferSize) {
        RpBuildMeshTriangle* triangles;
        RwUInt32 size = (mesh->numTriangles + 1) * sizeof(RpBuildMeshTriangle);
        if (mesh->numTriangles != 0) {
            triangles = RwEngineInstance->fpRealloc(mesh->meshTriangles, size,
                                                    0x01030502);
        } else {
            triangles = RwEngineInstance->fpMalloc(size, 0x01030502);
        }
        if (triangles == 0) {
            RwError error;
            error.pluginID = 2;
            error.errorCode = _rwerror(0x80000013, size);
            RwErrorSet(&error);
            return 0;
        }
        mesh->meshTriangles = triangles;
        mesh->triangleBufferSize = mesh->numTriangles + 1;
    }
    triangle = &mesh->meshTriangles[mesh->numTriangles];
    triangle->material = material;
    triangle->vertIndex[0] = (RwInt16)vert1;
    triangle->vertIndex[1] = (RwInt16)vert2;
    triangle->vertIndex[2] = (RwInt16)vert3;
    triangle->matIndex = matIndex;
    triangle->textureIndex = textureIndex;
    triangle->rasterIndex = rasterIndex;
    triangle->pipelineIndex = pipelineIndex;
    mesh->numTriangles++;
    return mesh;
}

RpMeshHeader* _rpMeshHeaderForAllMeshes(RpMeshHeader* meshHeader,
                                        RpMeshCallBack callback, void* data)
{

    RwInt32 numMeshes = meshHeader->numMeshes;
    RpMesh* mesh = (RpMesh*)((RwUInt8*)(meshHeader + 1) +
                            meshHeader->firstMeshOffset);
    while (numMeshes--) {
        if (callback(mesh, meshHeader, data) == 0) {
            return meshHeader;
        }
        mesh++;
    }
    return meshHeader;
}

RwStream* _rpMeshWrite(const RpMeshHeader* meshHeader, const void* object,
                       RwStream* stream, const RpMaterialList* materialList)
{
    RwInt32 header[3];
    const RpMesh* mesh;
    RwUInt32 numMeshes;

    header[0] = meshHeader->flags;
    header[1] = meshHeader->numMeshes;
    header[2] = meshHeader->totalIndices;
    if (RwStreamWriteInt32(stream, header, sizeof(header)) == 0) return 0;
    mesh = (const RpMesh*)(meshHeader + 1);
    numMeshes = meshHeader->numMeshes;
    while (numMeshes--) {
        RwInt32 info[2];
        info[0] = mesh->numIndices;
        info[1] = _rpMaterialListFindMaterialIndex(materialList, mesh->material);
        if (info[1] < 0) info[1] = 0;
        if (RwStreamWriteInt32(stream, info, sizeof(info)) == 0) return 0;
        if (MeshObjectHasIndices(object)) {
            RwUInt32 remaining = mesh->numIndices;
            const RxVertexIndex* index = mesh->indices;
            while (remaining != 0) {
                RwInt32 buffer[256];
                RwUInt32 count = remaining < 256 ? remaining : 256;
                RwUInt32 i;
                for (i = 0; i < count; i++) buffer[i] = *index++;
                if (RwStreamWriteInt32(stream, buffer, count * 4) == 0)
                    return 0;
                remaining -= count;
            }
        }
        mesh++;
    }
    return stream;
}

RpMeshHeader* _rpMeshRead(RwStream* stream, const void* object,
                          const RpMaterialList* materialList)
{
    RpBinMeshHeader header;
    RwInt32 size;
    RpMeshHeader* meshHeader;
    RpMesh* mesh;
    RxVertexIndex* indices;
    RwUInt32 numMeshes;

    if (RwStreamReadInt32(stream, (RwInt32*)&header, sizeof(header)) == 0)
        return 0;
    size = header.numMeshes * 16 + sizeof(RpMeshHeader);
    if (MeshObjectHasIndices(object))
        size += header.totalIndices * 2;
    meshHeader = _rpMeshHeaderCreate(size);
    if (meshHeader != 0) {
        mesh = (RpMesh*)(meshHeader + 1);
        indices =
            (RxVertexIndex*)((RwUInt8*)mesh + header.numMeshes * sizeof(RpMesh));
        meshHeader->flags = header.flags;
        meshHeader->numMeshes = header.numMeshes;
        meshHeader->serialNum = MeshGlobals()->nextSerialNum;
        meshHeader->totalIndices = header.totalIndices;
        meshHeader->firstMeshOffset = 0;
        MeshGlobals()->nextSerialNum++;
        numMeshes = meshHeader->numMeshes;
        while (numMeshes--) {
            RpBinMesh info;
            if (RwStreamReadInt32(stream, (RwInt32*)&info, sizeof(info)) == 0)
                return 0;
            mesh->numIndices = info.numIndices;
            mesh->material =
                _rpMaterialListGetMaterial(materialList, info.materialIndex);
            mesh->indices = indices;
            if (MeshObjectHasIndices(object)) {
                RwUInt32 remaining = mesh->numIndices;
                while (remaining != 0) {
                    RwInt32 buffer[256];
                    RwInt32* source = buffer;
                    RwUInt32 readIndices =
                        remaining < 256 ? remaining : 256;
                    if (RwStreamReadInt32(stream, source, readIndices * 4) == 0)
                        return 0;
                    remaining -= readIndices;
                    while (readIndices-- != 0)
                        *indices++ = (RxVertexIndex)*source++;
                }
            }
            mesh++;
        }
    }
    return meshHeader;
}

RwInt32 _rpMeshSize(const RpMeshHeader* meshHeader, const void* object)
{
    RwInt32 size;

    if ((((const RpMeshObjectHeader*)object)->type == 8 &&
         (((const RpMeshObjectHeader*)object)->flags & 0x01000000)) ||
        (((const RpMeshObjectHeader*)object)->type == 7 &&
         (((const RpMeshObjectHeader*)object)->flags & 0x01000000))) {
        size = 12 + meshHeader->numMeshes * 8;
    } else {
        size = 12 + meshHeader->numMeshes * 8 +
               meshHeader->totalIndices * 4;
    }
    return size;
}
