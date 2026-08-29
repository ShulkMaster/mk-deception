#include "rw/rwengine.h"
#include "rw/rpworld_types.h"
#include "rw/rwerror.h"
#include "rw/rwstream.h"
#include "rw/rwstream_internal.h"

static RpMaterial* MaterialAddRef(RpMaterial* material)
{
    material->refCount++;
    return material;
}

static RpMaterial **_rpMaterialListAlloc(int size);

RpMaterialList *_rpMaterialListDeinitialize(RpMaterialList *materialList)
{
    RpMaterial **materials = materialList->materials;

    if (materials != 0) {
        int numMaterials = materialList->numMaterials;
        int index;

        for (index = 0; index < numMaterials; index++) {
            RpMaterialDestroy(materials[index]);
            materials[index] = 0;
        }
        RwEngineInstance->fpFree(materials);
        materials = 0;
        materialList->materials = materials;
    }
    materialList->numMaterials = 0;
    materialList->space = 0;
    return materialList;
}

RpMaterialList *_rpMaterialListInitialize(RpMaterialList *materialList)
{
    materialList->space = 0;
    materialList->materials = 0;
    materialList->numMaterials = 0;
    return materialList;
}

static RpMaterial **_rpMaterialListAlloc(int size)
{


    RpMaterial **materials;
    unsigned int bytes = size * sizeof(RpMaterial *);

    materials = RwEngineInstance->fpMalloc(bytes, 0x1030008);
    return materials;
}

RpMaterial *_rpMaterialListGetMaterial(const RpMaterialList *materialList,
                                       int index)
{
    return materialList->materials[index];
}

RpMaterialList *_rpMaterialListSetSize(RpMaterialList *materialList,
                                       int size)
{
    if (materialList->space < size) {
        RpMaterial **materials;
        unsigned int bytes = size * sizeof(RpMaterial *);

        if (materialList->materials != 0) {
            materials = RwEngineInstance->fpRealloc(materialList->materials,
                                                    bytes, 0x1030008);
        } else {
            materials = RwEngineInstance->fpMalloc(bytes, 0x1030008);
        }
        if (materials == 0) {
            RwError error;
            error.pluginID = 2;
            error.errorCode = _rwerror(0x80000013, bytes);
            RwErrorSet(&error);
            return 0;
        }
        materialList->materials = materials;
        materialList->space = size;
    }
    return materialList;
}

int _rpMaterialListAppendMaterial(RpMaterialList *materialList,
                                      RpMaterial *material)
{
    RpMaterial **materials;

    if (materialList->space > materialList->numMaterials) {
        materials = materialList->materials + materialList->numMaterials;
        *materials = material;
        MaterialAddRef(material);
        materialList->numMaterials++;
        return materialList->numMaterials - 1;
    }

    {
        int space = materialList->space + 20;
        unsigned int bytes = space * sizeof(RpMaterial *);

        if (materialList->materials != 0) {
            materials = RwEngineInstance->fpRealloc(materialList->materials,
                                                    bytes, 0x1030008);
        } else {
            materials = _rpMaterialListAlloc(space);
        }
        if (materials == 0) {
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
    MaterialAddRef(material);
    materialList->numMaterials++;
    return materialList->numMaterials - 1;
}

int _rpMaterialListFindMaterialIndex(const RpMaterialList *materialList,
                                         const RpMaterial *material)
{
    int index = materialList->numMaterials;

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
    int numMaterials;
    unsigned int length;
    unsigned int version;
    int *materialIndices;
    int index;
    int status;

    if (!RwStreamFindChunk(stream, 1, &length, &version)) {
        return 0;
    }
    if (version >= 0x34000 && version <= 0x36003) {
        status = RwStreamReadInt32(stream, &numMaterials,
                                   sizeof(numMaterials)) != 0;
        if (!status) {
            return 0;
        }

        _rpMaterialListInitialize(materialList);
        if (numMaterials == 0) {
            return materialList;
        }
        if (_rpMaterialListSetSize(materialList, numMaterials) == 0) {
            _rpMaterialListDeinitialize(materialList);
            return 0;
        }

        materialIndices =
            RwEngineInstance->fpMalloc(numMaterials * sizeof(int), 0x10501);
        status = RwStreamReadInt32(stream, materialIndices,
                                   numMaterials * sizeof(int)) != 0;
        if (!status) {
            RwEngineInstance->fpFree(materialIndices);
            _rpMaterialListDeinitialize(materialList);
            return 0;
        }

        for (index = 0; index < numMaterials; index++) {
            RpMaterial *material;

            if (materialIndices[index] < 0) {
                if (!RwStreamFindChunk(stream, 7, 0, &version)) {
                    RwEngineInstance->fpFree(materialIndices);
                    _rpMaterialListDeinitialize(materialList);
                    return 0;
                }
                if (version >= 0x34000 && version <= 0x36003) {
                    material = RpMaterialStreamRead(stream);
                    if (material == 0) {
                        RwEngineInstance->fpFree(materialIndices);
                        _rpMaterialListDeinitialize(materialList);
                        return 0;
                    }
                } else {
                    RwError error;
                    error.pluginID = 2;
                    error.errorCode = _rwerror(0x80000004);
                    RwErrorSet(&error);
                    RwEngineInstance->fpFree(materialIndices);
                    _rpMaterialListDeinitialize(materialList);
                    return 0;
                }
            } else {
                material = _rpMaterialListGetMaterial(materialList,
                                                      materialIndices[index]);
                MaterialAddRef(material);
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
        return 0;
    }
}
