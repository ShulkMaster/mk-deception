#include "rw/rwengine.h"
#include "runtime/cstring.h"
#include "rw/bamateri.h"
#include "rw/rpworld_types.h"
#include "rw/rwerror.h"
#include "rw/rwplcore.h"
#include "rw/rwresources.h"
#include "rw/rwstream.h"
#include "rw/rwstream_internal.h"

typedef struct RpGeometryChunkInfo {
    unsigned int format;
    int numTriangles;
    int numVertices;
    int numMorphTargets;
} RpGeometryChunkInfo;

typedef struct RpMorphTargetChunkInfo {
    RwSphere sphere;
    int hasVertices;
    int hasNormals;
} RpMorphTargetChunkInfo;

typedef struct RpPackedTriangle {
    unsigned int vertex01;
    unsigned int vertex2Mat;
} RpPackedTriangle;

RwPluginRegistry geometryTKList = {0x60, 0x60, 0, 0, 0, 0};
static RwModuleInfo geometryModule;

static int GeometryAnnihilate(RpGeometry* geometry)
{
    geometry->refCount++;
    RpGeometryLock(geometry, 0xFFF);
    _rwPluginRegistryDeInitObject(&geometryTKList, geometry);
    if (geometry->morphTarget != 0) {
        RwEngineInstance->fpFree(geometry->morphTarget);
        geometry->morphTarget = 0;
    }
    _rpMaterialListDeinitialize(&geometry->matList);
    geometry->refCount--;
    RwEngineInstance->fpFree(geometry);
    return 1;
}

void* _rpGeometryOpen(void* instance, int offset, int size)
{
    geometryModule.globalsOffset = offset;
    geometryModule.numInstances++;
    return instance;
}

void* _rpGeometryClose(void* instance, int offset, int size)
{
    geometryModule.numInstances--;
    return instance;
}

int RpGeometryAddMorphTargets(RpGeometry* geometry, int count)
{
    int morphTargetSize;
    int allocationSize;
    RpMorphTarget* morphTargets;
    unsigned char* data;
    int index;

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
    if (geometry->morphTarget != 0) {
        unsigned char* source;
        unsigned char* destination;
        int bytes;

        morphTargets = RwEngineInstance->fpRealloc(
            geometry->morphTarget, allocationSize, 0x3000F);
        if (morphTargets == 0) {
            RwError error;
            error.pluginID = 2;
            error.errorCode = _rwerror(0x80000013, allocationSize);
            RwErrorSet(&error);
            return -1;
        }

        source = (unsigned char*)morphTargets +
                 morphTargetSize * geometry->numMorphTargets - 1;
        destination = source + count * sizeof(RpMorphTarget);
        bytes = morphTargetSize * geometry->numMorphTargets -
                sizeof(RpMorphTarget) * geometry->numMorphTargets;
        while (bytes-- != 0) {
            *destination-- = *source--;
        }
    } else {
        morphTargets = RwEngineInstance->fpMalloc(allocationSize, 0x3000F);
        if (morphTargets == 0) {
            RwError error;
            error.pluginID = 2;
            error.errorCode = _rwerror(0x80000013, allocationSize);
            RwErrorSet(&error);
            return -1;
        }
    }

    geometry->numMorphTargets += count;
    geometry->morphTarget = morphTargets;
    data = (unsigned char*)morphTargets +
           geometry->numMorphTargets * sizeof(RpMorphTarget);

    for (index = 0; index < geometry->numMorphTargets; index++) {
        RpMorphTarget* morphTarget = &geometry->morphTarget[index];
        morphTarget->verts = 0;
        morphTarget->normals = 0;
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
        morphTarget->sphere.center.x = 0.0f;
        morphTarget->sphere.center.y = 0.0f;
        morphTarget->sphere.center.z = 0.0f;
        morphTarget->sphere.radius = 0.0f;
        morphTarget->parentGeom = geometry;
    }

    return geometry->numMorphTargets - count;
}

int RpGeometryAddMorphTarget(RpGeometry* geometry)
{
    return RpGeometryAddMorphTargets(geometry, 1);
}

RpGeometry* RpGeometryForAllMaterials(RpGeometry* geometry,
                                      RpMaterialCallBack callback, void* data)
{


    RpMaterialCallBack callBack = callback;
    void* pData = data;
    int numMaterials = geometry->matList.numMaterials;
    int index;

    for (index = 0; index < numMaterials; index++) {
        RpMaterial* material = geometry->matList.materials[index];
        if (callBack(material, pData) == 0) {
            return geometry;
        }
    }
    return geometry;
}

RpGeometry* RpGeometryLock(RpGeometry* geometry, int lockMode)
{
    geometry->lockedSinceLastInst |= lockMode;
    if ((lockMode & 1) && geometry->meshHeader != 0) {
        _rpMeshDestroy(geometry->meshHeader);
        geometry->meshHeader = 0;
    }
    return geometry;
}

RpGeometry* RpGeometryUnlock(RpGeometry* geometry)
{
    RwTexture** textures;
    RwRaster** rasters;
    RxPipeline** pipelines;
    unsigned short numTextures = 0;
    unsigned short numRasters = 0;
    unsigned short numPipelines = 0;

    if (geometry->meshHeader == 0) {
        RpBuildMesh* buildMesh = _rpBuildMeshCreate(geometry->numTriangles);

        if (buildMesh != 0) {
            RpMeshHeader* meshHeader;
            int triangleIndex;
            int numMaterials = geometry->matList.numMaterials;

            if (numMaterials > 0) {
                textures = RwEngineInstance->fpMalloc(
                    sizeof(RwTexture*) * numMaterials, 0x10006);
                rasters = RwEngineInstance->fpMalloc(
                    sizeof(RwRaster*) * numMaterials, 0x10503);
                pipelines = RwEngineInstance->fpMalloc(
                    sizeof(RxPipeline*) * numMaterials, 0x10503);

                for (triangleIndex = 0;
                     triangleIndex < geometry->numTriangles;
                     triangleIndex++) {
                    RpTriangle* triangle =
                        &geometry->triangles[triangleIndex];
                    RpMaterial* material;
                    unsigned short textureIndex;
                    unsigned short rasterIndex;
                    unsigned short pipelineIndex;
                    RxPipeline* pipeline = 0;
                    RwTexture* texture = 0;
                    RwRaster* raster = 0;

                    material = _rpMaterialListGetMaterial(
                        &geometry->matList, triangle->matIndex);
                    texture = material->texture;
                    for (textureIndex = 0; textureIndex < numTextures;
                         textureIndex++) {
                        if (textures[textureIndex] == texture) {
                            break;
                        }
                    }
                    if (textureIndex == numTextures) {
                        textures[textureIndex] = texture;
                        numTextures++;
                    }
                    if (texture != 0) {
                        raster = texture->raster;
                    }

                    for (rasterIndex = 0; rasterIndex < numRasters;
                         rasterIndex++) {
                        if (rasters[rasterIndex] == raster) {
                            break;
                        }
                    }
                    if (rasterIndex == numRasters) {
                        rasters[rasterIndex] = raster;
                        numRasters++;
                    }

                    pipeline = material->pipeline;
                    for (pipelineIndex = 0; pipelineIndex < numPipelines;
                         pipelineIndex++) {
                        if (pipelines[pipelineIndex] == pipeline) {
                            break;
                        }
                    }
                    if (pipelineIndex == numPipelines) {
                        pipelines[pipelineIndex] = pipeline;
                        numPipelines++;
                    }

                    _rpBuildMeshAddTriangle(
                        buildMesh, material, triangle->vertIndex[0],
                        triangle->vertIndex[1], triangle->vertIndex[2],
                        triangle->matIndex, textureIndex, rasterIndex,
                        pipelineIndex);
                }

                RwEngineInstance->fpFree(textures);
                RwEngineInstance->fpFree(rasters);
                RwEngineInstance->fpFree(pipelines);
            }

            if ((geometry->flags & 1) != 0) {
                meshHeader = _rpMeshOptimise(buildMesh, 1);
            } else {
                meshHeader = _rpMeshOptimise(buildMesh, 0);
            }

            if (meshHeader != 0) {
                geometry->meshHeader = meshHeader;
                return geometry;
            }

            _rpBuildMeshDestroy(buildMesh);
            return 0;
        }

        return 0;
    }

    return geometry;
}

RpGeometry* RpGeometryCreate(int numVertices, int numTriangles,
                             unsigned int format)
{


    unsigned int allocationSize;
    int formatFlags;
    unsigned int numTexCoordSets;
    RpGeometry* geometry;

    if (numVertices < 0 || numVertices >= 0x10000 || numTriangles < 0) {
        if (numVertices >= 0x10000) {
            RwError error;
            error.pluginID = 2;
            error.errorCode = _rwerror(6);
            RwErrorSet(&error);
        }
        return 0;
    }

    formatFlags = format & 0xFF;
    allocationSize = geometryTKList.sizeOfStruct;

    numTexCoordSets =
        (format & 0x00FF0000)
            ? ((format & 0x00FF0000) >> 16)
            : ((format & 0x80) ? 2 : ((format & 4) ? 1 : 0));

    formatFlags =
        (formatFlags & ~0x84) |
        (numTexCoordSets == 1 ? 4 : (numTexCoordSets > 1 ? 0x80 : 0));

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
    if (geometry == 0) {
        return 0;
    }
    if (_rpMaterialListInitialize(&geometry->matList) == 0) {
        return 0;
    }

    geometry->morphTarget = 0;
    geometry->numMorphTargets = 0;
    rwInitializeObjectHeader(geometry, 8, 0);
    geometry->repEntry = 0;
    geometry->lockedSinceLastInst = 0;
    geometry->refCount = 1;
    geometry->meshHeader = 0;
    geometry->numTexCoordSets = numTexCoordSets;
    memset(geometry->texCoords, 0, sizeof(geometry->texCoords));
    geometry->preLitLum = 0;
    geometry->triangles = 0;
    geometry->numTriangles = numTriangles;
    geometry->flags = formatFlags | (format & 0x0F000000);
    geometry->numVertices = numVertices;

    if (!(format & 0x01000000)) {
        unsigned char* data = (unsigned char*)geometry + geometryTKList.sizeOfStruct;
        unsigned int index;

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
            int index;
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
        return 0;
    }

    _rwPluginRegistryInitObject(&geometryTKList, geometry);
    return geometry;
}

RpGeometry* _rpGeometryAddRef(RpGeometry* geometry)
{
    geometry->refCount++;
    return geometry;
}

int RpGeometryDestroy(RpGeometry* geometry)
{
    int result = 1;

    if (geometry->refCount - 1 <= 0) {
        if (geometry->repEntry != 0) {
            RwResourcesFreeResEntry(geometry->repEntry);
        }
        geometry->refCount--;
        result = GeometryAnnihilate(geometry);
    } else {
        geometry->refCount--;
    }
    return result;
}

int RpGeometryRegisterPlugin(int size, unsigned int pluginID,
                                 RwPluginObjectConstructor constructCB,
                                 RwPluginObjectDestructor destructCB,
                                 RwPluginObjectCopy copyCB)
{
    int offset = _rwPluginRegistryAddPlugin(
        &geometryTKList, size, pluginID, constructCB, destructCB, copyCB);
    return offset;
}

int RpGeometryRegisterPluginStream(
    unsigned int pluginID, RwPluginDataChunkReadCallBack readCB,
    RwPluginDataChunkWriteCallBack writeCB,
    RwPluginDataChunkGetSizeCallBack getSizeCB)
{
    int offset = _rwPluginRegistryAddPluginStream(
        &geometryTKList, pluginID, readCB, writeCB, getSizeCB);
    return offset;
}

RpGeometry* RpGeometryStreamRead(RwStream* stream)
{



    unsigned int version;
    RpGeometryChunkInfo chunk;
    RpSurfaceProperties oldSurface;
    RpGeometry* geometry;
    int index;

    if (!RwStreamFindChunk(stream, 1, 0, &version)) {
        return 0;
    }
    if (version < 0x34000 || version > 0x36003) {
        RwError error;
        error.pluginID = 2;
        error.errorCode = _rwerror(0x80000004);
        RwErrorSet(&error);
        return 0;
    }

    if (version < 0x34001) {
        if (RwStreamRead(stream, &chunk, sizeof(chunk)) != sizeof(chunk)) {
            return 0;
        }
        if (RwStreamRead(stream, &oldSurface, sizeof(oldSurface)) !=
            sizeof(oldSurface)) {
            return 0;
        }
        RwMemNative32(&oldSurface, sizeof(oldSurface));
    } else if (RwStreamRead(stream, &chunk, sizeof(chunk)) != sizeof(chunk)) {
        return 0;
    }
    RwMemNative32(&chunk, sizeof(chunk));

    geometry = RpGeometryCreate(chunk.numVertices, chunk.numTriangles,
                                chunk.format);
    if (geometry == 0) {
        return 0;
    }
    if (chunk.numMorphTargets > 1 &&
        RpGeometryAddMorphTargets(geometry,
                                  chunk.numMorphTargets - 1) < 0) {
        RpGeometryDestroy(geometry);
        return 0;
    }

    if (!(geometry->flags & 0x01000000) && geometry->numVertices != 0) {
        if ((int)(chunk.format & 8) != 0) {
            unsigned int preLitSize = geometry->numVertices * 4;
            if (RwStreamRead(stream, geometry->preLitLum, preLitSize) !=
                preLitSize) {
                RpGeometryDestroy(geometry);
                return 0;
            }
        }

        if (geometry->numTexCoordSets > 0) {
            unsigned int texCoordSize = geometry->numVertices *
                                    sizeof(RwTexCoords);
            for (index = 0; index < geometry->numTexCoordSets; index++) {
                if (RwStreamReadReal(stream, geometry->texCoords[index],
                                     texCoordSize) == 0) {
                    RpGeometryDestroy(geometry);
                    return 0;
                }
            }
        }

        if (geometry->numTriangles != 0) {
            RpTriangle* triangle;
            int count;
            unsigned int size;

            count = geometry->numTriangles;
            triangle = geometry->triangles;
            size = count * sizeof(RpPackedTriangle);

            if (RwStreamRead(stream, triangle, size) != size) {
                RpGeometryDestroy(geometry);
                return 0;
            }
            RwMemNative32(triangle, size);
            while (count-- != 0) {
                unsigned short high;
                unsigned short low;
                RpPackedTriangle* source = (RpPackedTriangle*)triangle;

                high = (unsigned short)(source->vertex01 >> 16) & 0xFFFF;
                low = (unsigned short)source->vertex01 & 0xFFFF;
                triangle->vertIndex[0] = high;
                triangle->vertIndex[1] = low;

                high = (unsigned short)(source->vertex2Mat >> 16) & 0xFFFF;
                low = (unsigned short)source->vertex2Mat & 0xFFFF;
                triangle->vertIndex[2] = high;
                triangle->matIndex = low;
                triangle++;
            }
        }
    }

    for (index = 0; index < geometry->numMorphTargets; index++) {
        RpMorphTargetChunkInfo morphInfo;
        RpMorphTarget* morphTarget = &geometry->morphTarget[index];

        if (RwStreamRead(stream, &morphInfo, sizeof(morphInfo)) !=
            sizeof(morphInfo)) {
            RpGeometryDestroy(geometry);
            return 0;
        }
        RwMemNative32(&morphInfo, sizeof(morphInfo));
        morphTarget->sphere = morphInfo.sphere;

        if (morphInfo.hasVertices && morphInfo.hasNormals) {
            if (RwStreamReadReal(stream, morphTarget->verts,
                                 geometry->numVertices * 24) == 0) {
                RpGeometryDestroy(geometry);
                return 0;
            }
        } else {
            if (morphInfo.hasVertices &&
                RwStreamReadReal(stream, morphTarget->verts,
                                 geometry->numVertices * sizeof(RwV3d)) ==
                    0) {
                RpGeometryDestroy(geometry);
                return 0;
            }
            if (morphInfo.hasNormals &&
                RwStreamReadReal(stream, morphTarget->normals,
                                 geometry->numVertices * sizeof(RwV3d)) ==
                    0) {
                RpGeometryDestroy(geometry);
                return 0;
            }
        }
    }

    if (!RwStreamFindChunk(stream, 8, 0, &version)) {
        return 0;
    }
    if (version < 0x34000 || version > 0x36003) {
        RwError error;
        RpGeometryDestroy(geometry);
        error.pluginID = 2;
        error.errorCode = _rwerror(0x80000004);
        RwErrorSet(&error);
        return 0;
    }

    if (version < 0x34001) {
        _rpMaterialSetDefaultSurfaceProperties(&oldSurface);
    }
    if (_rpMaterialListStreamRead(stream, &geometry->matList) == 0) {
        RpGeometryDestroy(geometry);
        return 0;
    }
    if (version < 0x34001) {
        _rpMaterialSetDefaultSurfaceProperties(0);
    }
    if (_rwPluginRegistryReadDataChunks(&geometryTKList, stream, geometry) ==
        0) {
        RpGeometryDestroy(geometry);
        return 0;
    }
    if (RpGeometryUnlock(geometry) == 0) {
        RpGeometryDestroy(geometry);
        return 0;
    }
    return geometry;
}
