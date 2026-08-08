#include "libmkparticle/rw_engine.h"
#include "rw/rpworld_types.h"
#include "rw/rwerror.h"
#include "rw/rwstream.h"
#include "rw/rwstream_internal.h"

#define RpMaterialAddRef(material) ((material)->refCount++, (material))

static RpMaterial **_rpMaterialListAlloc(RwInt32 size);

RpMaterialList *_rpMaterialListDeinitialize(RpMaterialList *materialList)
{
    RpMaterial **materials = materialList->materials;

    if (materials != NULL) {
        RwInt32 numMaterials = materialList->numMaterials;
        RwInt32 index;

        for (index = 0; index < numMaterials; index++) {
            RpMaterialDestroy(materials[index]);
            materials[index] = NULL;
        }
        RwEngineInstance->fpFree(materials);
        materialList->materials = NULL;
    }
    materialList->numMaterials = 0;
    materialList->space = 0;
    return materialList;
}

RpMaterialList *_rpMaterialListInitialize(RpMaterialList *materialList)
{
    materialList->space = 0;
    materialList->materials = NULL;
    materialList->numMaterials = 0;
    return materialList;
}

static RpMaterial **_rpMaterialListAlloc(RwInt32 size)
{
    /* Near miss: retail initializes the result to NULL before immediately
     * overwriting it; clean C omits that dead store. */
    RpMaterial **materials;
    RwUInt32 bytes = size * sizeof(RpMaterial *);

    materials = RwEngineInstance->fpMalloc(bytes, 0x1030008);
    return materials;
}

RpMaterial *_rpMaterialListGetMaterial(const RpMaterialList *materialList,
                                       RwInt32 index)
{
    return materialList->materials[index];
}

RpMaterialList *_rpMaterialListSetSize(RpMaterialList *materialList,
                                       RwInt32 size)
{
    if (materialList->space < size) {
        RwUInt32 bytes = size * sizeof(RpMaterial *);
        RpMaterial **materials;

        if (materialList->materials != NULL) {
            materials = RwEngineInstance->fpRealloc(materialList->materials,
                                                    bytes, 0x1030008);
        } else {
            materials = RwEngineInstance->fpMalloc(bytes, 0x1030008);
        }
        if (materials == NULL) {
            RwError error;
            error.pluginID = 2;
            error.errorCode = _rwerror(0x80000013, bytes);
            RwErrorSet(&error);
            return NULL;
        }
        materialList->materials = materials;
        materialList->space = size;
    }
    return materialList;
}

RwInt32 _rpMaterialListAppendMaterial(RpMaterialList *materialList,
                                      RpMaterial *material)
{
    RpMaterial **materials;

    if (materialList->space > materialList->numMaterials) {
        materials = materialList->materials + materialList->numMaterials;
        *materials = material;
        RpMaterialAddRef(material);
        materialList->numMaterials++;
        return materialList->numMaterials - 1;
    }

    {
        RwInt32 space = materialList->space + 20;
        RwUInt32 bytes = space * sizeof(RpMaterial *);

        if (materialList->materials != NULL) {
            materials = RwEngineInstance->fpRealloc(materialList->materials,
                                                    bytes, 0x1030008);
        } else {
            materials = _rpMaterialListAlloc(space);
        }
        if (materials == NULL) {
            RwError error;
            error.pluginID = 2;
            error.errorCode = _rwerror(0x80000013, bytes);
            RwErrorSet(&error);
            return -1;
        }
        materialList->materials = materials;
        materialList->space += 20;
    }

    materials[materialList->numMaterials] = material;
    RpMaterialAddRef(material);
    materialList->numMaterials++;
    return materialList->numMaterials - 1;
}

RwInt32 _rpMaterialListFindMaterialIndex(const RpMaterialList *materialList,
                                         const RpMaterial *material)
{
    RwInt32 index = materialList->numMaterials;

    while (index-- > 0) {
        if (materialList->materials[index] == material) {
            break;
        }
    }
    return index;
}

RpMaterialList *_rpMaterialListStreamRead(RwStream *stream,
                                          RpMaterialList *materialList)
{
    RwInt32 numMaterials;
    RwUInt32 length;
    RwUInt32 version;
    RwInt32 *materialIndices;
    RwInt32 index;
    RwBool status;

    if (!RwStreamFindChunk(stream, 1, &length, &version)) {
        return NULL;
    }
    if (version >= 0x34000 && version <= 0x36003) {
        status = RwStreamReadInt32(stream, &numMaterials,
                                   sizeof(numMaterials)) != FALSE;
        if (!status) {
            return NULL;
        }

        _rpMaterialListInitialize(materialList);
        if (numMaterials == 0) {
            return materialList;
        }
        if (_rpMaterialListSetSize(materialList, numMaterials) == NULL) {
            _rpMaterialListDeinitialize(materialList);
            return NULL;
        }

        materialIndices =
            RwEngineInstance->fpMalloc(numMaterials * sizeof(RwInt32), 0x10501);
        status = RwStreamReadInt32(stream, materialIndices,
                                   numMaterials * sizeof(RwInt32)) != FALSE;
        if (!status) {
            RwEngineInstance->fpFree(materialIndices);
            _rpMaterialListDeinitialize(materialList);
            return NULL;
        }

        for (index = 0; index < numMaterials; index++) {
            RpMaterial *material;

            if (materialIndices[index] < 0) {
                if (!RwStreamFindChunk(stream, 7, NULL, &version)) {
                    RwEngineInstance->fpFree(materialIndices);
                    _rpMaterialListDeinitialize(materialList);
                    return NULL;
                }
                if (version >= 0x34000 && version <= 0x36003) {
                    material = RpMaterialStreamRead(stream);
                    if (material == NULL) {
                        RwEngineInstance->fpFree(materialIndices);
                        _rpMaterialListDeinitialize(materialList);
                        return NULL;
                    }
                } else {
                    RwError error;
                    error.pluginID = 2;
                    error.errorCode = _rwerror(0x80000004);
                    RwErrorSet(&error);
                    RwEngineInstance->fpFree(materialIndices);
                    _rpMaterialListDeinitialize(materialList);
                    return NULL;
                }
            } else {
                material = _rpMaterialListGetMaterial(materialList,
                                                      materialIndices[index]);
                RpMaterialAddRef(material);
            }

            _rpMaterialListAppendMaterial(materialList, material);
            RpMaterialDestroy(material);
        }

        RwEngineInstance->fpFree(materialIndices);
        return materialList;
    } else {
        RwError error;
        error.pluginID = 2;
        error.errorCode = _rwerror(0x80000004);
        RwErrorSet(&error);
        return NULL;
    }
}
