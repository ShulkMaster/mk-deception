#include "libmkparticle/rw_engine.h"
#include "rw/rpworld_types.h"
#include "rw/rwerror.h"
#include "rw/rwfreelist.h"
#include "rw/rwstream.h"

typedef struct RpMeshGlobals {
    RwUInt16 nextSerialNum;
    RwUInt8 reserved02[6];
    RwUInt8 meshFlags[0x20];
    RwUInt8 primitiveType[6];
} RpMeshGlobals;

typedef struct RpMeshObjectHeader {
    RwUInt8 type;
    RwUInt8 reserved01[7];
    RwUInt32 flags;
} RpMeshObjectHeader;

static RwFreeList* MeshStatic = NULL;
RwModuleInfo meshModule;

#define MESHGLOBALS \
    ((RpMeshGlobals*)((RwUInt8*)RwEngineInstance + meshModule.globalsOffset))
#define MESHOBJECTHASINDICES(object)                                      \
    (((((const RpMeshObjectHeader*)(object))->type == 8) &&                \
      !(((const RpMeshObjectHeader*)(object))->flags & 0x01000000)) ||    \
     ((((const RpMeshObjectHeader*)(object))->type == 7) &&                \
      !(((const RpMeshObjectHeader*)(object))->flags & 0x01000000)))

static void MeshFreeListsDestroy(void)
{
    if (MeshStatic != NULL) {
        RwFreeListDestroy(MeshStatic);
        MeshStatic = NULL;
    }
}

static RwBool MeshFreeListsCreate(void)
{
    /* Retail lowers this same pointer test branchlessly. */
    RwBool result;
    MeshStatic = RwFreeListCreate(sizeof(RpBuildMesh), 50, 4, 0x40502);
    result = FALSE;
    if (MeshStatic != NULL)
        result = TRUE;
    return result;
}

void _rpMeshHeaderDestroy(RpMeshHeader* meshHeader)
{
    /* Retail clears this local after freeing it; the clear is unobservable. */
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
        instance = NULL;
        return instance;
    }
    MESHGLOBALS->nextSerialNum = 1;
    meshModule.numInstances++;
    MESHGLOBALS->meshFlags[0] = 3;
    MESHGLOBALS->meshFlags[1] = 4;
    MESHGLOBALS->meshFlags[2] = 5;
    MESHGLOBALS->meshFlags[4] = 1;
    MESHGLOBALS->meshFlags[8] = 2;
    MESHGLOBALS->meshFlags[0x10] = 6;
    MESHGLOBALS->primitiveType[0] = 4;
    MESHGLOBALS->primitiveType[1] = 8;
    MESHGLOBALS->primitiveType[2] = 0;
    MESHGLOBALS->primitiveType[3] = 1;
    MESHGLOBALS->primitiveType[4] = 2;
    MESHGLOBALS->primitiveType[5] = 0x10;
    return instance;
}

RpBuildMesh* _rpBuildMeshCreate(RwUInt32 bufferSize)
{
    /* Retail body is exact; only its r29-r31 save/restore helpers differ. */
    RpBuildMesh* mesh = RwEngineInstance->fpFreeListAlloc(MeshStatic, 0x30502);

    if (mesh != NULL) {
        mesh->numTriangles = 0;
        if (bufferSize != 0) {
            RwUInt32 size = bufferSize * sizeof(RpBuildMeshTriangle);
            mesh->meshTriangles = RwEngineInstance->fpMalloc(size, 0x01030502);
            if (mesh->meshTriangles == NULL) {
                RwError error;
                RwEngineInstance->fpFreeListFree(MeshStatic, mesh);
                error.pluginID = 2;
                error.errorCode = _rwerror(0x80000013, size);
                RwErrorSet(&error);
                return NULL;
            }
            mesh->triangleBufferSize = bufferSize;
        } else {
            mesh->meshTriangles = NULL;
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
    return NULL;
}

RwBool _rpBuildMeshDestroy(RpBuildMesh* mesh)
{
    /* Retail clears the local mesh argument after the final free. */
    if (mesh->meshTriangles != NULL) {
        RwEngineInstance->fpFree(mesh->meshTriangles);
        mesh->meshTriangles = NULL;
    }
    RwEngineInstance->fpFreeListFree(MeshStatic, mesh);
    return TRUE;
}

RwBool _rpMeshDestroy(RpMeshHeader* meshHeader)
{
    if (meshHeader->flags != 0 || meshHeader->numMeshes != 0 ||
        meshHeader->serialNum != 0 || meshHeader->totalIndices != 0 ||
        meshHeader->firstMeshOffset != 0) {
        _rpMeshHeaderDestroy(meshHeader);
    }
    return TRUE;
}

RpBuildMesh* _rpBuildMeshAddTriangle(
    RpBuildMesh* mesh, RpMaterial* material, RwInt32 vert1, RwInt32 vert2,
    RwInt32 vert3, RwUInt16 matIndex, RwUInt16 textureIndex,
    RwUInt16 rasterIndex, RwUInt16 pipelineIndex)
{
    RpBuildMeshTriangle* triangle;

    if (mesh->numTriangles >= mesh->triangleBufferSize) {
        RwUInt32 size = (mesh->numTriangles + 1) * sizeof(RpBuildMeshTriangle);
        RpBuildMeshTriangle* triangles;
        if (mesh->numTriangles != 0) {
            triangles = RwEngineInstance->fpRealloc(mesh->meshTriangles, size,
                                                    0x01030502);
        } else {
            triangles = RwEngineInstance->fpMalloc(size, 0x01030502);
        }
        if (triangles == NULL) {
            RwError error;
            error.pluginID = 2;
            error.errorCode = _rwerror(0x80000013, size);
            RwErrorSet(&error);
            return NULL;
        }
        mesh->meshTriangles = triangles;
        mesh->triangleBufferSize = mesh->numTriangles + 1;
    }
    triangle = &mesh->meshTriangles[mesh->numTriangles];
    triangle->material = material;
    triangle->vertIndex[0] = (RwUInt16)vert1;
    triangle->vertIndex[1] = (RwUInt16)vert2;
    triangle->vertIndex[2] = (RwUInt16)vert3;
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
    /* Retail body is exact; only its r29-r31 save/restore helpers differ. */
    RwInt32 numMeshes = meshHeader->numMeshes;
    RpMesh* mesh = (RpMesh*)((RwUInt8*)(meshHeader + 1) +
                            meshHeader->firstMeshOffset);
    while (numMeshes--) {
        if (callback(mesh, meshHeader, data) == NULL) {
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
    if (RwStreamWriteInt32(stream, header, sizeof(header)) == NULL) return NULL;
    mesh = (const RpMesh*)(meshHeader + 1);
    numMeshes = meshHeader->numMeshes;
    while (numMeshes--) {
        RwInt32 info[2];
        info[0] = mesh->numIndices;
        info[1] = _rpMaterialListFindMaterialIndex(materialList, mesh->material);
        if (info[1] < 0) info[1] = 0;
        if (RwStreamWriteInt32(stream, info, sizeof(info)) == NULL) return NULL;
        if (MESHOBJECTHASINDICES(object)) {
            RwUInt32 remaining = mesh->numIndices;
            const RxVertexIndex* index = mesh->indices;
            while (remaining != 0) {
                RwInt32 buffer[256];
                RwUInt32 count = remaining < 256 ? remaining : 256;
                RwUInt32 i;
                for (i = 0; i < count; i++) buffer[i] = *index++;
                if (RwStreamWriteInt32(stream, buffer, count * 4) == NULL)
                    return NULL;
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
    RwInt32 header[3];
    RwInt32 size;
    RpMeshHeader* meshHeader;
    RpMesh* mesh;
    RxVertexIndex* indices;
    RwUInt32 numMeshes;

    if (RwStreamReadInt32(stream, header, sizeof(header)) == NULL) return NULL;
    size = header[1] * 16 + sizeof(RpMeshHeader);
    if (MESHOBJECTHASINDICES(object))
        size += header[2] * 2;
    meshHeader = _rpMeshHeaderCreate(size);
    if (meshHeader == NULL) return NULL;
    mesh = (RpMesh*)(meshHeader + 1);
    indices = (RxVertexIndex*)((RwUInt8*)mesh + header[1] * sizeof(RpMesh));
    meshHeader->flags = header[0];
    meshHeader->numMeshes = header[1];
    meshHeader->serialNum = MESHGLOBALS->nextSerialNum;
    meshHeader->totalIndices = header[2];
    meshHeader->firstMeshOffset = 0;
    MESHGLOBALS->nextSerialNum++;
    numMeshes = meshHeader->numMeshes;
    while (numMeshes--) {
        RwInt32 info[2];
        if (RwStreamReadInt32(stream, info, sizeof(info)) == NULL) return NULL;
        mesh->numIndices = info[0];
        mesh->material = _rpMaterialListGetMaterial(materialList, info[1]);
        mesh->indices = indices;
        if (MESHOBJECTHASINDICES(object)) {
            RwUInt32 remaining = mesh->numIndices;
            while (remaining != 0) {
                RwInt32 buffer[256];
                RwUInt32 count = remaining < 256 ? remaining : 256;
                RwUInt32 i;
                if (RwStreamReadInt32(stream, buffer, count * 4) == NULL)
                    return NULL;
                remaining -= count;
                for (i = 0; i < count; i++) *indices++ = buffer[i];
            }
        }
        mesh++;
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
