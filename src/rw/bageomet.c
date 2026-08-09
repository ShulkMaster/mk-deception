#include "libmkparticle/rw_engine.h"
#include "runtime/cstring.h"
#include "rw/rpworld_types.h"
#include "rw/rwerror.h"
#include "rw/rwplcore.h"
#include "rw/rwresources.h"
#include "rw/rwstream.h"
#include "rw/rwstream_internal.h"

typedef struct RpGeometryChunkInfo {
    RwUInt32 format;
    RwInt32 numTriangles;
    RwInt32 numVertices;
    RwInt32 numMorphTargets;
} RpGeometryChunkInfo;

typedef struct RpMorphTargetChunkInfo {
    RwSphere sphere;
    RwInt32 hasVertices;
    RwInt32 hasNormals;
} RpMorphTargetChunkInfo;

RwPluginRegistry geometryTKList = {0x60, 0x60, 0, 0, NULL, NULL};
static RwModuleInfo geometryModule;

extern RpMeshHeader* _rpMeshOptimise(RpBuildMesh* buildMesh,
                                     RwBool useTriStrip);
extern RwStream* RwStreamReadReal(RwStream* stream, RwReal* reals,
                                  RwUInt32 numBytes);
extern void _rpMaterialSetDefaultSurfaceProperties(
    const RpSurfaceProperties* surface);

static RwBool GeometryAnnihilate(RpGeometry* geometry)
{
    geometry->refCount++;
    RpGeometryLock(geometry, 0xFFF);
    _rwPluginRegistryDeInitObject(&geometryTKList, geometry);
    if (geometry->morphTarget != NULL) {
        RwEngineInstance->fpFree(geometry->morphTarget);
        geometry->morphTarget = NULL;
    }
    _rpMaterialListDeinitialize(&geometry->matList);
    geometry->refCount--;
    RwEngineInstance->fpFree(geometry);
    return TRUE;
}

void* _rpGeometryOpen(void* instance, RwInt32 offset, RwInt32 size)
{
    geometryModule.globalsOffset = offset;
    geometryModule.numInstances++;
    return instance;
}

void* _rpGeometryClose(void* instance, RwInt32 offset, RwInt32 size)
{
    geometryModule.numInstances--;
    return instance;
}

RwInt32 RpGeometryAddMorphTargets(RpGeometry* geometry, RwInt32 count)
{
    RwInt32 morphTargetSize;
    RwInt32 allocationSize;
    RpMorphTarget* morphTargets;
    RwUInt8* data;
    RwInt32 index;

    if (geometry->flags & 0x01000000) {
        morphTargetSize = sizeof(RpMorphTarget);
    } else {
        morphTargetSize = sizeof(RpMorphTarget) +
                          geometry->numVertices * sizeof(RwV3d);
        if (geometry->flags & 0x10) {
            morphTargetSize += geometry->numVertices * sizeof(RwV3d);
        }
    }

    allocationSize = morphTargetSize * (geometry->numMorphTargets + count);
    if (geometry->morphTarget != NULL) {
        RwUInt8* source;
        RwUInt8* destination;
        RwInt32 bytes;

        morphTargets = RwEngineInstance->fpRealloc(
            geometry->morphTarget, allocationSize, 0x3000F);
        if (morphTargets == NULL) {
            RwError error;
            error.pluginID = 2;
            error.errorCode = _rwerror(0x80000013, allocationSize);
            RwErrorSet(&error);
            return -1;
        }

        source = (RwUInt8*)morphTargets +
                 morphTargetSize * geometry->numMorphTargets - 1;
        destination = source + count * sizeof(RpMorphTarget);
        bytes = morphTargetSize * geometry->numMorphTargets -
                sizeof(RpMorphTarget) * geometry->numMorphTargets;
        while (bytes-- != 0) {
            *destination-- = *source--;
        }
    } else {
        morphTargets = RwEngineInstance->fpMalloc(allocationSize, 0x3000F);
        if (morphTargets == NULL) {
            RwError error;
            error.pluginID = 2;
            error.errorCode = _rwerror(0x80000013, allocationSize);
            RwErrorSet(&error);
            return -1;
        }
    }

    geometry->numMorphTargets += count;
    geometry->morphTarget = morphTargets;
    data = (RwUInt8*)morphTargets +
           geometry->numMorphTargets * sizeof(RpMorphTarget);

    for (index = 0; index < geometry->numMorphTargets; index++) {
        RpMorphTarget* morphTarget = &geometry->morphTarget[index];
        morphTarget->verts = NULL;
        morphTarget->normals = NULL;
        if (!(geometry->flags & 0x01000000) && geometry->numVertices != 0) {
            morphTarget->verts = data;
            data += geometry->numVertices * sizeof(RwV3d);
            if (geometry->flags & 0x10) {
                morphTarget->normals = data;
                data += geometry->numVertices * sizeof(RwV3d);
            }
        }
    }

    for (index = geometry->numMorphTargets - count;
         index < geometry->numMorphTargets; index++) {
        RpMorphTarget* morphTarget = &geometry->morphTarget[index];
        morphTarget->sphere.x = 0.0f;
        morphTarget->sphere.y = 0.0f;
        morphTarget->sphere.z = 0.0f;
        morphTarget->sphere.radius = 0.0f;
        morphTarget->parentGeom = geometry;
    }

    return geometry->numMorphTargets - count;
}

RwInt32 RpGeometryAddMorphTarget(RpGeometry* geometry)
{
    return RpGeometryAddMorphTargets(geometry, 1);
}

RpGeometry* RpGeometryForAllMaterials(RpGeometry* geometry,
                                      RpMaterialCallBack callback, void* data)
{
    RpMaterialCallBack callBack = callback;
    void* pData = data;
    RwInt32 numMaterials = geometry->matList.numMaterials;
    RwInt32 index;

    for (index = 0; index < numMaterials; index++) {
        RpMaterial* material = geometry->matList.materials[index];
        if (callBack(material, pData) == NULL) {
            return geometry;
        }
    }
    return geometry;
}

RpGeometry* RpGeometryLock(RpGeometry* geometry, RwInt32 lockMode)
{
    geometry->lockedSinceLastInst |= lockMode;
    if ((lockMode & 1) && geometry->meshHeader != NULL) {
        _rpMeshDestroy(geometry->meshHeader);
        geometry->meshHeader = NULL;
    }
    return geometry;
}

RpGeometry* RpGeometryUnlock(RpGeometry* geometry)
{
    RwUInt16 numTextures = 0;
    RwUInt16 numRasters = 0;
    RwUInt16 numPipelines = 0;
    RpBuildMesh* buildMesh;
    RpMeshHeader* meshHeader;

    if (geometry->meshHeader != NULL) {
        return geometry;
    }

    buildMesh = _rpBuildMeshCreate(geometry->numTriangles);
    if (buildMesh == NULL) {
        return NULL;
    }

    if (geometry->matList.numMaterials > 0) {
        RwInt32 tableSize = geometry->matList.numMaterials * sizeof(void*);
        RwTexture** textures = RwEngineInstance->fpMalloc(tableSize, 0x10006);
        RwRaster** rasters = RwEngineInstance->fpMalloc(tableSize, 0x10503);
        RxPipeline** pipelines = RwEngineInstance->fpMalloc(tableSize, 0x10503);
        RwInt32 triangleIndex;

        for (triangleIndex = 0; triangleIndex < geometry->numTriangles;
             triangleIndex++) {
            RpTriangle* triangle = &geometry->triangles[triangleIndex];
            RpMaterial* material =
                _rpMaterialListGetMaterial(&geometry->matList,
                                           triangle->matIndex);
            RwTexture* texture = material->texture;
            RwRaster* raster = texture != NULL ? texture->raster : NULL;
            RxPipeline* pipeline = material->pipeline;
            RwUInt16 textureIndex;
            RwUInt16 rasterIndex;
            RwUInt16 pipelineIndex;

            for (textureIndex = 0; textureIndex < numTextures; textureIndex++) {
                if (textures[textureIndex] == texture) break;
            }
            if (textureIndex == numTextures) {
                textures[textureIndex] = texture;
                numTextures++;
            }

            for (rasterIndex = 0; rasterIndex < numRasters; rasterIndex++) {
                if (rasters[rasterIndex] == raster) break;
            }
            if (rasterIndex == numRasters) {
                rasters[rasterIndex] = raster;
                numRasters++;
            }

            for (pipelineIndex = 0; pipelineIndex < numPipelines;
                 pipelineIndex++) {
                if (pipelines[pipelineIndex] == pipeline) break;
            }
            if (pipelineIndex == numPipelines) {
                pipelines[pipelineIndex] = pipeline;
                numPipelines++;
            }

            _rpBuildMeshAddTriangle(
                buildMesh, material, triangle->vertIndex[0],
                triangle->vertIndex[1], triangle->vertIndex[2],
                (RwUInt16)triangle->matIndex, textureIndex, rasterIndex,
                pipelineIndex);
        }

        RwEngineInstance->fpFree(textures);
        RwEngineInstance->fpFree(rasters);
        RwEngineInstance->fpFree(pipelines);
    }

    meshHeader = _rpMeshOptimise(buildMesh, (geometry->flags & 1) != 0);
    if (meshHeader != NULL) {
        geometry->meshHeader = meshHeader;
        return geometry;
    }

    _rpBuildMeshDestroy(buildMesh);
    return NULL;
}

RpGeometry* RpGeometryCreate(RwInt32 numVertices, RwInt32 numTriangles,
                             RwUInt32 format)
{
    RwUInt32 allocationSize;
    RwUInt32 formatFlags;
    RwUInt32 numTexCoordSets;
    RwUInt32 texCoordFlags;
    RpGeometry* geometry;

    if (numVertices < 0 || numVertices >= 0x10000 || numTriangles < 0) {
        if (numVertices >= 0x10000) {
            RwError error;
            error.pluginID = 2;
            error.errorCode = _rwerror(6);
            RwErrorSet(&error);
        }
        return NULL;
    }

    formatFlags = format & 0xFF;
    if (format & 0x00FF0000) {
        numTexCoordSets = (format & 0x00FF0000) >> 16;
    } else if (format & 0x80) {
        numTexCoordSets = 2;
    } else {
        numTexCoordSets = (format >> 2) & 1;
    }

    texCoordFlags = numTexCoordSets == 1 ? 4 :
                    (numTexCoordSets > 1 ? 0x80 : 0);
    formatFlags = (formatFlags & ~0x84) | texCoordFlags;

    allocationSize = geometryTKList.sizeOfStruct;
    if (!(format & 0x01000000)) {
        if (formatFlags & 8) {
            allocationSize += numVertices * 4;
        }
        if (numTexCoordSets != 0) {
            allocationSize += numVertices * numTexCoordSets *
                              sizeof(RwTexCoords);
        }
        allocationSize += numTriangles * sizeof(RpTriangle);
    }

    geometry = RwEngineInstance->fpMalloc(allocationSize, 0x3000F);
    if (geometry == NULL) {
        return NULL;
    }
    if (_rpMaterialListInitialize(&geometry->matList) == NULL) {
        return NULL;
    }

    geometry->morphTarget = NULL;
    geometry->numMorphTargets = 0;
    geometry->object.type = 8;
    geometry->object.subType = 0;
    geometry->object.flags = 0;
    geometry->object.privateFlags = 0;
    geometry->object.parent = NULL;
    geometry->repEntry = NULL;
    geometry->lockedSinceLastInst = 0;
    geometry->refCount = 1;
    geometry->meshHeader = NULL;
    geometry->numTexCoordSets = numTexCoordSets;
    memset(geometry->texCoords, 0, sizeof(geometry->texCoords));
    geometry->preLitLum = NULL;
    geometry->triangles = NULL;
    geometry->numTriangles = numTriangles;
    geometry->flags = formatFlags | (format & 0x0F000000);
    geometry->numVertices = numVertices;

    if (!(format & 0x01000000)) {
        RwUInt8* data = (RwUInt8*)geometry + geometryTKList.sizeOfStruct;
        RwUInt32 index;

        if ((formatFlags & 8) && numVertices != 0) {
            geometry->preLitLum = data;
            data += numVertices * 4;
        }
        if (numTexCoordSets != 0 && numVertices != 0) {
            for (index = 0; index < numTexCoordSets; index++) {
                geometry->texCoords[index] = data;
                data += numVertices * sizeof(RwTexCoords);
            }
        }
        if (numTriangles != 0) {
            RwInt32 index;
            geometry->triangles = (RpTriangle*)data;
            data += numTriangles * sizeof(RpTriangle);
            for (index = 0; index < numTriangles; index++) {
                geometry->triangles[index].matIndex = -1;
            }
        }
    }

    if (RpGeometryAddMorphTarget(geometry) < 0) {
        _rpMaterialListDeinitialize(&geometry->matList);
        RwEngineInstance->fpFree(geometry);
        return NULL;
    }

    _rwPluginRegistryInitObject(&geometryTKList, geometry);
    return geometry;
}

RpGeometry* _rpGeometryAddRef(RpGeometry* geometry)
{
    geometry->refCount++;
    return geometry;
}

RwBool RpGeometryDestroy(RpGeometry* geometry)
{
    RwBool result = TRUE;

    if (geometry->refCount - 1 <= 0) {
        if (geometry->repEntry != NULL) {
            RwResourcesFreeResEntry(geometry->repEntry);
        }
        geometry->refCount--;
        result = GeometryAnnihilate(geometry);
    } else {
        geometry->refCount--;
    }
    return result;
}

RwInt32 RpGeometryRegisterPlugin(RwInt32 size, RwUInt32 pluginID,
                                 RwPluginObjectConstructor constructCB,
                                 RwPluginObjectDestructor destructCB,
                                 RwPluginObjectCopy copyCB)
{
    RwInt32 offset = _rwPluginRegistryAddPlugin(
        &geometryTKList, size, pluginID, constructCB, destructCB, copyCB);
    return offset;
}

RwInt32 RpGeometryRegisterPluginStream(
    RwUInt32 pluginID, RwPluginDataChunkReadCallBack readCB,
    RwPluginDataChunkWriteCallBack writeCB,
    RwPluginDataChunkGetSizeCallBack getSizeCB)
{
    RwInt32 offset = _rwPluginRegistryAddPluginStream(
        &geometryTKList, pluginID, readCB, writeCB, getSizeCB);
    return offset;
}

RpGeometry* RpGeometryStreamRead(RwStream* stream)
{
    RwUInt32 version;
    RpGeometryChunkInfo chunk;
    RpSurfaceProperties oldSurface;
    RpGeometry* geometry;
    RpMorphTarget* morphTarget;
    RwInt32 index;

    if (!RwStreamFindChunk(stream, 1, NULL, &version)) {
        return NULL;
    }
    if (version < 0x34000 || version > 0x36003) {
        RwError error;
        error.pluginID = 2;
        error.errorCode = _rwerror(0x80000004);
        RwErrorSet(&error);
        return NULL;
    }

    if (version < 0x34001) {
        if (RwStreamRead(stream, &chunk, sizeof(chunk)) != sizeof(chunk)) {
            return NULL;
        }
        if (RwStreamRead(stream, &oldSurface, sizeof(oldSurface)) !=
            sizeof(oldSurface)) {
            return NULL;
        }
        RwMemNative32(&oldSurface, sizeof(oldSurface));
    } else if (RwStreamRead(stream, &chunk, sizeof(chunk)) != sizeof(chunk)) {
        return NULL;
    }
    RwMemNative32(&chunk, sizeof(chunk));

    geometry = RpGeometryCreate(chunk.numVertices, chunk.numTriangles,
                                chunk.format);
    if (geometry == NULL) {
        return NULL;
    }
    if (chunk.numMorphTargets > 1 &&
        RpGeometryAddMorphTargets(geometry,
                                  chunk.numMorphTargets - 1) < 0) {
        RpGeometryDestroy(geometry);
        return NULL;
    }

    if (!(geometry->flags & 0x01000000) && geometry->numVertices != 0) {
        if (chunk.format & 8) {
            RwUInt32 preLitSize = geometry->numVertices * 4;
            if (RwStreamRead(stream, geometry->preLitLum, preLitSize) !=
                preLitSize) {
                RpGeometryDestroy(geometry);
                return NULL;
            }
        }

        if (geometry->numTexCoordSets > 0) {
            RwUInt32 texCoordSize = geometry->numVertices *
                                    sizeof(RwTexCoords);
            for (index = 0; index < geometry->numTexCoordSets; index++) {
                if (RwStreamReadReal(stream, geometry->texCoords[index],
                                     texCoordSize) == NULL) {
                    RpGeometryDestroy(geometry);
                    return NULL;
                }
            }
        }

        if (geometry->numTriangles != 0) {
            RpTriangle* triangle = geometry->triangles;
            RwInt32 count = geometry->numTriangles;
            RwUInt32 size = count * sizeof(RpTriangle);

            if (RwStreamRead(stream, triangle, size) != size) {
                RpGeometryDestroy(geometry);
                return NULL;
            }
            RwMemNative32(triangle, size);
            while (count-- != 0) {
                RwUInt32 first = *(RwUInt32*)&triangle->vertIndex[0];
                RwUInt32 second = *(RwUInt32*)&triangle->vertIndex[2];
                triangle->vertIndex[0] = (RwUInt16)(first >> 16);
                triangle->vertIndex[1] = (RwUInt16)first;
                triangle->vertIndex[2] = (RwUInt16)(second >> 16);
                triangle->matIndex = (RwInt16)second;
                triangle++;
            }
        }
    }

    for (index = 0; index < geometry->numMorphTargets; index++) {
        RpMorphTargetChunkInfo morphInfo;
        morphTarget = &geometry->morphTarget[index];

        if (RwStreamRead(stream, &morphInfo, sizeof(morphInfo)) !=
            sizeof(morphInfo)) {
            RpGeometryDestroy(geometry);
            return NULL;
        }
        RwMemNative32(&morphInfo, sizeof(morphInfo));
        morphTarget->sphere = morphInfo.sphere;

        if (morphInfo.hasVertices && morphInfo.hasNormals) {
            if (RwStreamReadReal(stream, morphTarget->verts,
                                 geometry->numVertices * 24) == NULL) {
                RpGeometryDestroy(geometry);
                return NULL;
            }
        } else {
            if (morphInfo.hasVertices &&
                RwStreamReadReal(stream, morphTarget->verts,
                                 geometry->numVertices * sizeof(RwV3d)) ==
                    NULL) {
                RpGeometryDestroy(geometry);
                return NULL;
            }
            if (morphInfo.hasNormals &&
                RwStreamReadReal(stream, morphTarget->normals,
                                 geometry->numVertices * sizeof(RwV3d)) ==
                    NULL) {
                RpGeometryDestroy(geometry);
                return NULL;
            }
        }
    }

    if (!RwStreamFindChunk(stream, 8, NULL, &version)) {
        return NULL;
    }
    if (version < 0x34000 || version > 0x36003) {
        RwError error;
        RpGeometryDestroy(geometry);
        error.pluginID = 2;
        error.errorCode = _rwerror(0x80000004);
        RwErrorSet(&error);
        return NULL;
    }

    if (version < 0x34001) {
        _rpMaterialSetDefaultSurfaceProperties(&oldSurface);
    }
    if (_rpMaterialListStreamRead(stream, &geometry->matList) == NULL) {
        RpGeometryDestroy(geometry);
        return NULL;
    }
    if (version < 0x34001) {
        _rpMaterialSetDefaultSurfaceProperties(NULL);
    }
    if (_rwPluginRegistryReadDataChunks(&geometryTKList, stream, geometry) ==
        NULL) {
        RpGeometryDestroy(geometry);
        return NULL;
    }
    if (RpGeometryUnlock(geometry) == NULL) {
        RpGeometryDestroy(geometry);
        return NULL;
    }
    return geometry;
}
