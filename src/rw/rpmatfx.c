#include "libmkparticle/rw_engine.h"
#include "rw/batextur.h"
#include "rw/rpmatfx.h"
#include "rw/rwerror.h"
#include "rw/rwfreelist.h"
#include "rw/rwplcore.h"

typedef struct MatFXModuleInfo {
    RwInt32 unused;
    RwInt32 numInstances;
    RwFreeList* materialDataFreeList;
} MatFXModuleInfo;

extern void* memset(void*, RwInt32, RwUInt32);
extern void* memcpy(void*, const void*, RwUInt32);
extern RwStream* RwStreamWriteReal(RwStream*, const RwReal*, RwUInt32);
extern RwStream* RwStreamReadReal(RwStream*, RwReal*, RwUInt32);
extern RwImage* RwImageCreate(RwInt32, RwInt32, RwInt32);
extern RwImage* RwImageAllocatePixels(RwImage*);
extern RwBool RwImageDestroy(RwImage*);
extern RwImage* RwImageMakeMask(RwImage*);
extern RwImage* RwImageApplyMask(RwImage*, const RwImage*);
extern RwInt32 RpMaterialRegisterPlugin(
    RwInt32, RwUInt32, RwPluginObjectConstructor, RwPluginObjectDestructor,
    RwPluginObjectCopy);
extern RwInt32 RpMaterialRegisterPluginStream(
    RwUInt32, RwPluginDataChunkReadCallBack, RwPluginDataChunkWriteCallBack,
    RwPluginDataChunkGetSizeCallBack);
extern RwInt32 RpAtomicRegisterPlugin(
    RwInt32, RwUInt32, RwPluginObjectConstructor, RwPluginObjectDestructor,
    RwPluginObjectCopy);
extern RwInt32 RpAtomicRegisterPluginStream(
    RwUInt32, RwPluginDataChunkReadCallBack, RwPluginDataChunkWriteCallBack,
    RwPluginDataChunkGetSizeCallBack);
extern RwBool _rpMatFXPipelinesCreate(void);
extern void _rpMatFXPipelinesDestroy(void);
extern RpAtomic* _rpMatFXPipelineAtomicSetup(RpAtomic*);
extern RpWorldSector* _rpMatFXPipelineWorldSectorSetup(RpWorldSector*);
extern RwTexture* _rpMatFXSetupBumpMapTexture(RwTexture*, RwTexture*);
extern void _rpMatFXSetupDualRenderState(RpMatFXDualData*, RwInt32);
extern RwBool _rpMultiTexturePlatformPluginsAttach(void);

MatFXModuleInfo MatFXInfo = {0, 0, NULL};
static RwFreeList _rpMatFXMaterialDataFreeList;
static RwInt32 _rpMatFXMaterialDataFreeListBlockSize = 0x80;
static RwInt32 _rpMatFXMaterialDataFreeListPreallocBlocks = 1;
RwInt32 MatFXMaterialDataOffset;
static RwInt32 MatFXAtomicDataOffset;
static RwInt32 MatFXWorldSectorDataOffset;

void* MatFXGetData(RpMaterial* material, RpMatFXMaterialFlags effect)
{
    RpMatFXMaterialData* data =
        RWPLUGINOFFSET(RpMatFXMaterialData*, material, MatFXMaterialDataOffset);
    RwUInt8 i;
    for (i = 0; i < 2; i++) {
        if (data->slot[i].type == effect)
            return &data->slot[i].data;
    }
    return NULL;
}

static const void* MatFXGetConstData(const RpMaterial* material,
                                     RpMatFXMaterialFlags effect)
{
    const RpMatFXMaterialData* data =
        RWPLUGINOFFSET(RpMatFXMaterialData*, material, MatFXMaterialDataOffset);
    RwUInt8 i;
    for (i = 0; i < 2; i++) {
        if (data->slot[i].type == effect)
            return &data->slot[i].data;
    }
    return NULL;
}

static void* MatFXClose(void* instance, RwInt32 offset, RwInt32 size)
{
    MatFXInfo.numInstances--;
    if (MatFXInfo.numInstances == 0)
        _rpMatFXPipelinesDestroy();
    if (MatFXInfo.materialDataFreeList) {
        RwFreeListDestroy(MatFXInfo.materialDataFreeList);
        MatFXInfo.materialDataFreeList = NULL;
    }
    return instance;
}

static void* MatFXOpen(void* instance, RwInt32 offset, RwInt32 size)
{
    if (MatFXInfo.numInstances == 0) {
        MatFXInfo.materialDataFreeList = RwFreeListCreateAndPreallocateSpace(
            sizeof(RpMatFXMaterialData), _rpMatFXMaterialDataFreeListBlockSize,
            4, _rpMatFXMaterialDataFreeListPreallocBlocks,
            &_rpMatFXMaterialDataFreeList, 0x40120);
        if (!MatFXInfo.materialDataFreeList) {
            instance = NULL;
            return instance;
        }
        if (!_rpMatFXPipelinesCreate()) {
            instance = NULL;
            return instance;
        }
    }
    MatFXInfo.numInstances++;
    return instance;
}

static void* MatFXMaterialConstructor(void* object, RwInt32 offset, RwInt32 size)
{
    RWPLUGINOFFSET(RpMatFXMaterialData*, object, MatFXMaterialDataOffset) = NULL;
    return object;
}

static RpMatFXMaterialData* MatFXMaterialDataClean(RpMatFXMaterialData* data)
{
    RwUInt8 i;
    for (i = 0; i < 2; i++) {
        switch (data->slot[i].type) {
        case rpMATFXEFFECTBUMPMAP: {
            RpMatFXBumpMapData* bump = &data->slot[i].data.bump;
            if (bump->texture)
                RwTextureDestroy(bump->texture);
            if (bump->bumped_texture)
                RwTextureDestroy(bump->bumped_texture);
            break;
        }
        case rpMATFXEFFECTENVMAP: {
            RpMatFXEnvMapData* env = &data->slot[i].data.env;
            if (env->texture)
                RwTextureDestroy(env->texture);
            break;
        }
        case rpMATFXEFFECTDUAL: {
            RpMatFXDualData* dual = &data->slot[i].data.dual;
            if (dual->texture)
                RwTextureDestroy(dual->texture);
            break;
        }
        case rpMATFXEFFECTUVTRANSFORM: {
            RpMatFXUVTransformData* uv = &data->slot[i].data.uv;
            uv->baseTransform = NULL;
            uv->dualTransform = NULL;
            break;
        }
        case rpMATFXEFFECTBUMPENVMAP:
        case rpMATFXEFFECTNULL:
        default:
            break;
        }
    }
    memset(data, 0, sizeof(*data));
    return data;
}

static void* MatFXMaterialDestructor(void* object, RwInt32 offset, RwInt32 size)
{
    RpMatFXMaterialData* data;
    data = RWPLUGINOFFSET(RpMatFXMaterialData*, object,
                          MatFXMaterialDataOffset);
    if (data) {
        MatFXMaterialDataClean(data);
        RwEngineInstance->fpFreeListFree(MatFXInfo.materialDataFreeList, data);
        RWPLUGINOFFSET(RpMatFXMaterialData*, object,
                       MatFXMaterialDataOffset) = NULL;
    }
    return object;
}

static RpMatFXMaterialData* MatFXMaterialGetData(RpMaterial* material)
{
    RpMatFXMaterialData* data =
        RWPLUGINOFFSET(RpMatFXMaterialData*, material, MatFXMaterialDataOffset);
    if (!data) {
        data = RwEngineInstance->fpFreeListAlloc(
            MatFXInfo.materialDataFreeList, 0x30120);
        if (!data)
            return NULL;
        memset(data, 0, sizeof(*data));
        RWPLUGINOFFSET(RpMatFXMaterialData*, material,
                       MatFXMaterialDataOffset) = data;
    }
    return data;
}

static void* MatFXMaterialCopy(void* destination, const void* source,
                               RwInt32 offset, RwInt32 size)
{
    const RpMaterial* src = source;
    RpMaterial* dst = destination;
    const RpMatFXMaterialData* srcData =
        RWPLUGINOFFSET(RpMatFXMaterialData*, src, MatFXMaterialDataOffset);
    RpMatFXMaterialData* dstData;
    RwUInt8 i;
    (void)offset;
    (void)size;
    if (!srcData)
        return NULL;
    dstData = MatFXMaterialGetData(dst);
    if (!dstData)
        return NULL;
    RpMatFXMaterialSetEffects(dst, srcData->effects);
    for (i = 0; i < 2; i++) {
        switch (srcData->slot[i].type) {
        case rpMATFXEFFECTBUMPMAP:
            RpMatFXMaterialSetBumpMapFrame(dst,
                RpMatFXMaterialGetBumpMapFrame(src));
            RpMatFXMaterialSetBumpMapCoefficient(dst,
                RpMatFXMaterialGetBumpMapCoefficient(src));
            dstData->slot[i].data.bump.texture =
                srcData->slot[i].data.bump.texture;
            dstData->slot[i].data.bump.bumped_texture =
                srcData->slot[i].data.bump.bumped_texture;
            if (dstData->slot[i].data.bump.texture)
                dstData->slot[i].data.bump.texture->ref_count++;
            if (dstData->slot[i].data.bump.bumped_texture)
                dstData->slot[i].data.bump.bumped_texture->ref_count++;
            break;
        case rpMATFXEFFECTENVMAP: {
            RwTexture* texture = RpMatFXMaterialGetEnvMapTexture(src);
            if (texture)
                RpMatFXMaterialSetEnvMapTexture(dst, texture);
            RpMatFXMaterialSetEnvMapFrame(dst,
                RpMatFXMaterialGetEnvMapFrame(src));
            RpMatFXMaterialSetEnvMapFrameBufferAlpha(dst,
                RpMatFXMaterialGetEnvMapFrameBufferAlpha(src));
            RpMatFXMaterialSetEnvMapCoefficient(dst,
                RpMatFXMaterialGetEnvMapCoefficient(src));
            break;
        }
        case rpMATFXEFFECTDUAL: {
            RwTexture* texture = RpMatFXMaterialGetDualTexture(src);
            RwBlendFunction srcBlend, dstBlend;
            RpMatFXMaterialGetDualBlendModes(src, &srcBlend, &dstBlend);
            if (texture)
                RpMatFXMaterialSetDualTexture(dst, texture);
            RpMatFXMaterialSetDualBlendModes(dst, srcBlend, dstBlend);
            break;
        }
        case rpMATFXEFFECTUVTRANSFORM: {
            RwMatrix *base, *dual;
            src = RpMatFXMaterialGetUVTransformMatrices(src, &base, &dual);
            dst = RpMatFXMaterialSetUVTransformMatrices(dst, base, dual);
            break;
        }
        default:
            break;
        }
    }
    return destination;
}

RwStream* _rpMatFXStreamWriteTexture(RwStream* stream, RwTexture* texture)
{
    RwInt32 present = NULL != texture;
    if (!RwStreamWriteInt32(stream, &present, sizeof(present)))
        return NULL;
    if (present && !RwTextureStreamWrite(texture, stream))
        return NULL;
    return stream;
}

RwStream* _rpMatFXStreamReadTexture(RwStream* stream, RwTexture** texture)
{
    RwInt32 present;
    RwError error;
    if (!RwStreamReadInt32(stream, &present, sizeof(present)))
        return NULL;
    if (present) {
        if (!RwStreamFindChunk(stream, 6, NULL, NULL))
            return NULL;
        RwErrorGet(&error);
        *texture = RwTextureStreamRead(stream);
        if (!*texture) {
            RwErrorGet(&error);
            if (error.errorCode != (RwInt32)0x80000000 &&
                error.errorCode != 0x16) {
                RwErrorSet(&error);
                return NULL;
            }
        }
    } else {
        *texture = NULL;
    }
    return stream;
}

RwInt32 _rpMatFXStreamSizeTexture(RwTexture* texture)
{
    /* Retail differs only in the operand order of the commutative size add. */
    RwInt32 size = 4;
    if (texture != NULL) {
        size += RwTextureStreamGetSize(texture);
        size += 0xC;
    }
    return size;
}

static RwStream* MatFXMaterialStreamWrite(RwStream* stream, RwInt32 length,
                                          const void* object, RwInt32 offset,
                                          RwInt32 size)
{
    const RpMatFXMaterialData* data =
        RWPLUGINOFFSET(RpMatFXMaterialData*, object, offset);
    RwUInt8 i;
    RwInt32 value;
    RwReal real;
    (void)length;
    (void)size;
    value = data->effects;
    if (!RwStreamWriteInt32(stream, &value, 4)) return NULL;
    for (i = 0; i < 2; i++) {
        value = data->slot[i].type;
        if (!RwStreamWriteInt32(stream, &value, 4)) return NULL;
        switch (data->slot[i].type) {
        case rpMATFXEFFECTBUMPMAP:
            real = -data->slot[i].data.bump.storedCoefficient;
            if (!RwStreamWriteReal(stream, &real, 4) ||
                !_rpMatFXStreamWriteTexture(stream, data->slot[i].data.bump.texture) ||
                !_rpMatFXStreamWriteTexture(stream, data->slot[i].data.bump.bumped_texture))
                return NULL;
            break;
        case rpMATFXEFFECTENVMAP:
            if (!RwStreamWriteReal(stream, &data->slot[i].data.env.coefficient, 4)) return NULL;
            value = data->slot[i].data.env.useFrameBufferAlpha;
            if (!RwStreamWriteInt32(stream, &value, 4) ||
                !_rpMatFXStreamWriteTexture(stream, data->slot[i].data.env.texture)) return NULL;
            break;
        case rpMATFXEFFECTDUAL:
            value = data->slot[i].data.dual.srcBlendMode;
            if (!RwStreamWriteInt32(stream, &value, 4)) return NULL;
            value = data->slot[i].data.dual.dstBlendMode;
            if (!RwStreamWriteInt32(stream, &value, 4) ||
                !_rpMatFXStreamWriteTexture(stream, data->slot[i].data.dual.texture)) return NULL;
            break;
        default:
            break;
        }
    }
    return stream;
}

static RwStream* MatFXMaterialStreamRead(RwStream* stream, RwInt32 length,
                                         void* object, RwInt32 offset,
                                         RwInt32 size)
{
    RpMaterial* material = object;
    RpMatFXMaterialData* data = MatFXMaterialGetData(material);
    RwUInt8 i;
    RwInt32 effects, type, value[2];
    RwReal coefficient;
    RwTexture *texture, *bumpTexture;
    (void)length; (void)offset; (void)size;
    if (!data || !RwStreamReadInt32(stream, &effects, 4)) return NULL;
    RpMatFXMaterialSetEffects(material, effects);
    for (i = 0; i < 2; i++) {
        if (!RwStreamReadInt32(stream, &type, 4)) return NULL;
        switch (type) {
        case rpMATFXEFFECTBUMPMAP:
            texture = bumpTexture = NULL;
            if (!RwStreamReadReal(stream, &coefficient, 4) ||
                !_rpMatFXStreamReadTexture(stream, &texture)) return NULL;
            if (!_rpMatFXStreamReadTexture(stream, &bumpTexture)) {
                if (texture) RwTextureDestroy(texture);
                return NULL;
            }
            if (texture) {
                data->slot[i].data.bump.texture = texture;
                data->slot[i].data.bump.bumped_texture = bumpTexture;
                data->slot[i].data.bump.coefficient =
                    1.0f / texture->raster->width;
            } else if (bumpTexture) {
                RpMatFXMaterialSetBumpMapTexture(material, bumpTexture);
                RwTextureDestroy(bumpTexture);
            } else {
                data->slot[i].data.bump.texture = NULL;
                data->slot[i].data.bump.bumped_texture = NULL;
            }
            RpMatFXMaterialSetBumpMapCoefficient(material, coefficient);
            break;
        case rpMATFXEFFECTENVMAP:
            texture = NULL;
            if (!RwStreamReadReal(stream, &coefficient, 4) ||
                !RwStreamReadInt32(stream, value, 4) ||
                !_rpMatFXStreamReadTexture(stream, &texture)) return NULL;
            if (texture) {
                RpMatFXMaterialSetEnvMapTexture(material, texture);
                RwTextureDestroy(texture);
            }
            RpMatFXMaterialSetEnvMapCoefficient(material, coefficient);
            RpMatFXMaterialSetEnvMapFrameBufferAlpha(material, value[0]);
            break;
        case rpMATFXEFFECTDUAL:
            texture = NULL;
            if (!RwStreamReadInt32(stream, value, 8) ||
                !_rpMatFXStreamReadTexture(stream, &texture)) return NULL;
            if (texture) {
                RpMatFXMaterialSetDualTexture(material, texture);
                RwTextureDestroy(texture);
            }
            RpMatFXMaterialSetDualBlendModes(material, value[0], value[1]);
            break;
        default:
            break;
        }
    }
    return stream;
}

static RwInt32 MatFXMaterialStreamGetSize(const void* object, RwInt32 offset,
                                          RwInt32 size)
{
    /* Retail ignores the callback offset and uses the module-owned slot. */
    const RpMatFXMaterialData* data =
        RWPLUGINOFFSET(RpMatFXMaterialData*, object, MatFXMaterialDataOffset);
    RwInt32 streamSize = 4;
    RwUInt8 i;
    (void)offset;
    (void)size;
    if (!data || data->effects == rpMATFXEFFECTNULL) return 0;
    for (i = 0; i < 2; i++) {
        streamSize += 4;
        switch (data->slot[i].type) {
        case rpMATFXEFFECTBUMPMAP:
            streamSize = streamSize + 4 +
                _rpMatFXStreamSizeTexture(data->slot[i].data.bump.texture);
            streamSize = streamSize + _rpMatFXStreamSizeTexture(
                data->slot[i].data.bump.bumped_texture);
            break;
        case rpMATFXEFFECTENVMAP:
            streamSize = streamSize + 4 + 4 +
                _rpMatFXStreamSizeTexture(data->slot[i].data.env.texture);
            break;
        case rpMATFXEFFECTDUAL:
            streamSize = streamSize + 8 +
                _rpMatFXStreamSizeTexture(data->slot[i].data.dual.texture);
            break;
        case rpMATFXEFFECTBUMPENVMAP:
        case rpMATFXEFFECTUVTRANSFORM:
        default:
            break;
        }
    }
    return streamSize;
}

static void MatFXAtomicConstructor(RpAtomic* atomic)
{
    RwBool* enabled =
        (RwBool*)((RwUInt8*)atomic + MatFXAtomicDataOffset);
    *enabled = FALSE;
}

static void MatFXAtomicDestructor(RpAtomic* atomic)
{
    RwBool* enabled =
        (RwBool*)((RwUInt8*)atomic + MatFXAtomicDataOffset);
    *enabled = FALSE;
}

static void MatFXAtomicCopy(RpAtomic* destination, const RpAtomic* source)
{
    /*
     * Retail and this body perform the same extension loads, test, and store.
     * Its plugin callback parameter homes select a wider nonvolatile save set;
     * clean typed extension pointers leave only that compiler-emission residue.
     */
    const RwBool* sourceEnabled =
        (const RwBool*)((const RwUInt8*)source + MatFXAtomicDataOffset);
    RwBool* destinationEnabled =
        (RwBool*)((RwUInt8*)destination + MatFXAtomicDataOffset);

    if (*sourceEnabled != FALSE) {
        *destinationEnabled = TRUE;
    }
}

static RwStream* MatFXAtomicStreamWrite(RwStream* stream, RwInt32 length,
                                        const RpAtomic* atomic)
{
    const RwBool* enabled =
        (const RwBool*)((const RwUInt8*)atomic + MatFXAtomicDataOffset);
    RwInt32 value = *enabled;
    RwStream* result = RwStreamWriteInt32(stream, &value, 4);
    return result;
}

static RwStream* MatFXAtomicStreamRead(RwStream* stream, RwInt32 length,
                                       RpAtomic* atomic)
{
    RpAtomic* target = atomic;
    RwInt32 value;
    if (!RwStreamReadInt32(stream, &value, 4)) return NULL;
    if (value) RpMatFXAtomicEnableEffects(target);
    return stream;
}

static RwInt32 MatFXAtomicStreamGetSize(const RpAtomic* atomic)
{
    const RwBool* enabled =
        (const RwBool*)((const RwUInt8*)atomic + MatFXAtomicDataOffset);
    if (*enabled == FALSE) {
        return 0;
    }
    return 4;
}

static void MatFXWorldSectorConstructor(RpWorldSector* sector)
{
    RwBool* enabled =
        (RwBool*)((RwUInt8*)sector + MatFXWorldSectorDataOffset);
    *enabled = FALSE;
}

static void MatFXWorldSectorDestructor(RpWorldSector* sector)
{
    RwBool* enabled =
        (RwBool*)((RwUInt8*)sector + MatFXWorldSectorDataOffset);
    *enabled = FALSE;
}

static void MatFXWorldSectorCopy(RpWorldSector* destination,
                                 const RpWorldSector* source)
{
    /* The atomic-copy callback above has the same retail save-set residue. */
    const RwBool* sourceEnabled =
        (const RwBool*)((const RwUInt8*)source +
                       MatFXWorldSectorDataOffset);
    RwBool* destinationEnabled =
        (RwBool*)((RwUInt8*)destination + MatFXWorldSectorDataOffset);

    if (*sourceEnabled != FALSE) {
        *destinationEnabled = TRUE;
    }
}

static RwStream* MatFXWorldSectorStreamWrite(RwStream* stream, RwInt32 length,
                                             const RpWorldSector* sector)
{
    const RwBool* enabled =
        (const RwBool*)((const RwUInt8*)sector +
                       MatFXWorldSectorDataOffset);
    RwInt32 value = *enabled;
    RwStream* result = RwStreamWriteInt32(stream, &value, 4);
    return result;
}

static RwStream* MatFXWorldSectorStreamRead(RwStream* stream, RwInt32 length,
                                            RpWorldSector* sector)
{
    RpWorldSector* target = sector;
    RwInt32 value;
    if (!RwStreamReadInt32(stream, &value, 4)) return NULL;
    if (value) RpMatFXWorldSectorEnableEffects(target);
    return stream;
}

static RwInt32 MatFXWorldSectorStreamGetSize(const RpWorldSector* sector)
{
    const RwBool* enabled =
        (const RwBool*)((const RwUInt8*)sector +
                       MatFXWorldSectorDataOffset);
    if (*enabled == FALSE) {
        return 0;
    }
    return 4;
}

static void GenBumpedTextureName(RwChar* name, const RwTexture* base,
                                 const RwTexture* bump)
{
    const RwChar* strings[2];
    RwInt32 count = 0;
    RwInt32 i;
    if (base) {
        strings[0] = base->name;
        strings[1] = bump->name;
    } else {
        strings[0] = bump->name;
        strings[1] = bump->name;
    }
    while (count < 30 && (*strings[0] || *strings[1])) {
        for (i = 0; i < 2; i++) {
            if (*strings[i]) {
                *name++ = *strings[i]++;
                count++;
            }
        }
    }
    *name = '\0';
}

RwTexture* _rpMatFXTextureMaskCreate(const RwTexture* base,
                                     const RwTexture* bump)
{
    static const RwChar emptyName[32] = {0};
    RwRaster *baseRaster = NULL, *bumpRaster = bump->raster, *raster;
    RwImage *baseImage, *bumpImage, *resampled;
    RwTexture* texture;
    RwInt32 baseWidth, baseHeight, width, height, depth, format;
    RwInt32 x, y;
    RwUInt32 addressMode;
    RwChar name[32];
    bumpImage = RwImageCreate(bumpRaster->width, bumpRaster->height, 32);
    RwImageAllocatePixels(bumpImage);
    RwImageSetFromRaster(bumpImage, bumpRaster);
    if (base) {
        baseRaster = base->raster;
        baseWidth = baseRaster->width;
        baseHeight = baseRaster->height;
        baseImage = RwImageCreate(baseWidth, baseHeight, 32);
        RwImageAllocatePixels(baseImage);
        RwImageSetFromRaster(baseImage, baseRaster);
    } else {
        baseWidth = bumpRaster->width;
        baseHeight = bumpRaster->height;
        baseImage = RwImageCreate(baseWidth, baseHeight, 32);
        RwImageAllocatePixels(baseImage);
        for (y = 0; y < baseHeight; y++)
            for (x = 0; x < baseWidth; x++)
                *(RwUInt32*)(baseImage->pixels + baseImage->stride * y + x * 4) = 0xffffffff;
    }
    if (baseWidth != bumpRaster->width || baseHeight != bumpRaster->height) {
        resampled = RwImageCreate(baseWidth, baseHeight, 32);
        RwImageAllocatePixels(resampled);
        RwImageResample(resampled, bumpImage);
        RwImageDestroy(bumpImage);
        bumpImage = resampled;
    }
    RwImageMakeMask(bumpImage);
    RwImageApplyMask(baseImage, bumpImage);
    RwImageFindRasterFormat(baseImage, 4, &width, &height, &depth, &format);
    if ((((base ? baseRaster : bumpRaster)->format << 8) & 0x8000) != 0)
        format |= 0x9000;
    raster = RwRasterCreate(width, height, depth, format);
    RwRasterSetFromImage(raster, baseImage);
    texture = RwTextureCreate(raster);
    if (base) {
        texture->filter_flags = (texture->filter_flags & ~0xff) |
                                (base->filter_flags & 0xff);
        addressMode = ((base->filter_flags & 0xf00) >> 8) ==
                      ((base->filter_flags & 0xf000) >> 12)
                          ? (base->filter_flags & 0xf000) >> 12 : 0;
    } else {
        texture->filter_flags = (texture->filter_flags & ~0xff) |
                                (bump->filter_flags & 0xff);
        addressMode = ((bump->filter_flags & 0xf00) >> 8) ==
                      ((bump->filter_flags & 0xf000) >> 12)
                          ? (bump->filter_flags & 0xf000) >> 12 : 0;
    }
    texture->filter_flags = (texture->filter_flags & 0xffff00ff) |
                            (addressMode << 8) | (addressMode << 12);
    RwImageDestroy(baseImage);
    RwImageDestroy(bumpImage);
    memcpy(name, emptyName, sizeof(name));
    GenBumpedTextureName(name, base, bump);
    RwTextureSetName(texture, name);
    return texture;
}

RwBool RpMatFXPluginAttach(void)
{
    RwInt32 result;

    if (RwEngineRegisterPlugin(0, 0x120, MatFXOpen, MatFXClose) < 0) return FALSE;
    MatFXMaterialDataOffset = RpMaterialRegisterPlugin(4, 0x120,
        MatFXMaterialConstructor, MatFXMaterialDestructor, MatFXMaterialCopy);
    if (MatFXMaterialDataOffset < 0) return FALSE;
    result = RpMaterialRegisterPluginStream(0x120, MatFXMaterialStreamRead,
        MatFXMaterialStreamWrite, MatFXMaterialStreamGetSize);
    if (result < 0) return FALSE;
    MatFXAtomicDataOffset = RpAtomicRegisterPlugin(4, 0x120,
        (RwPluginObjectConstructor)MatFXAtomicConstructor,
        (RwPluginObjectDestructor)MatFXAtomicDestructor,
        (RwPluginObjectCopy)MatFXAtomicCopy);
    if (MatFXAtomicDataOffset < 0) return FALSE;
    result = RpAtomicRegisterPluginStream(0x120,
        (RwPluginDataChunkReadCallBack)MatFXAtomicStreamRead,
        (RwPluginDataChunkWriteCallBack)MatFXAtomicStreamWrite,
        (RwPluginDataChunkGetSizeCallBack)MatFXAtomicStreamGetSize);
    if (result < 0) return FALSE;
    MatFXWorldSectorDataOffset = RpWorldSectorRegisterPlugin(4, 0x120,
        (RwPluginObjectConstructor)MatFXWorldSectorConstructor,
        (RwPluginObjectDestructor)MatFXWorldSectorDestructor,
        (RwPluginObjectCopy)MatFXWorldSectorCopy);
    if (MatFXWorldSectorDataOffset < 0) return FALSE;
    result = RpWorldSectorRegisterPluginStream(0x120,
        (RwPluginDataChunkReadCallBack)MatFXWorldSectorStreamRead,
        (RwPluginDataChunkWriteCallBack)MatFXWorldSectorStreamWrite,
        (RwPluginDataChunkGetSizeCallBack)MatFXWorldSectorStreamGetSize);
    if (result < 0) return FALSE;
    if (!_rpMultiTexturePlatformPluginsAttach()) return FALSE;
    return TRUE;
}

RpAtomic* RpMatFXAtomicEnableEffects(RpAtomic* atomic)
{
    RwInt32* enabled = (RwInt32*)((RwUInt8*)atomic + MatFXAtomicDataOffset);
    if (!*enabled) {
        if (!_rpMatFXPipelineAtomicSetup(atomic)) return NULL;
        *enabled = 1;
    }
    return atomic;
}

RwBool RpMatFXAtomicQueryEffects(const RpAtomic* atomic)
{
    const RwBool* enabled =
        (const RwBool*)((const RwUInt8*)atomic + MatFXAtomicDataOffset);
    return *enabled;
}

RpWorldSector* RpMatFXWorldSectorEnableEffects(RpWorldSector* sector)
{
    RwInt32* enabled = (RwInt32*)((RwUInt8*)sector + MatFXWorldSectorDataOffset);
    if (!*enabled) {
        if (!_rpMatFXPipelineWorldSectorSetup(sector)) return NULL;
        *enabled = 1;
    }
    return sector;
}

RpMaterial* RpMatFXMaterialSetEffects(RpMaterial* material,
                                      RpMatFXMaterialFlags effects)
{
    RpMatFXMaterialData* data = MatFXMaterialGetData(material);
    if (!data) return NULL;
    if (effects == rpMATFXEFFECTNULL ||
        (data->effects != rpMATFXEFFECTNULL && data->effects != effects))
        MatFXMaterialDataClean(data);
    data->effects = effects;
    /* The stock explicit NULL case keeps this a dense 0..6 jump table. */
    switch ((RwUInt32)data->effects) {
    case rpMATFXEFFECTNULL: break;
    case rpMATFXEFFECTBUMPMAP: data->slot[0].type = rpMATFXEFFECTBUMPMAP; break;
    case rpMATFXEFFECTENVMAP: data->slot[0].type = rpMATFXEFFECTENVMAP; break;
    case rpMATFXEFFECTBUMPENVMAP:
        data->slot[0].type = rpMATFXEFFECTBUMPMAP;
        data->slot[1].type = rpMATFXEFFECTENVMAP; break;
    case rpMATFXEFFECTDUAL:
        data->slot[0].type = rpMATFXEFFECTDUAL;
        RpMatFXMaterialSetDualBlendModes(material, 5, 6); break;
    case rpMATFXEFFECTUVTRANSFORM:
        data->slot[0].type = rpMATFXEFFECTUVTRANSFORM; break;
    case rpMATFXEFFECTDUALUVTRANSFORM:
        data->slot[0].type = rpMATFXEFFECTUVTRANSFORM;
        data->slot[1].type = rpMATFXEFFECTDUAL;
        RpMatFXMaterialSetDualBlendModes(material, 5, 6); break;
    default: break;
    }
    return material;
}

RpMatFXMaterialFlags RpMatFXMaterialGetEffects(const RpMaterial* material)
{
    const RpMatFXMaterialData* data =
        RWPLUGINOFFSET(RpMatFXMaterialData*, material, MatFXMaterialDataOffset);
    if (data == NULL)
        return rpMATFXEFFECTNULL;
    return data->effects;
}

RpMaterial* RpMatFXMaterialSetBumpMapTexture(RpMaterial* material,
                                             RwTexture* bumpTexture)
{
    static const RwChar emptyName[32] = {0};
    RpMatFXBumpMapData* data = MatFXGetData(material, rpMATFXEFFECTBUMPMAP);
    RwTexture* baseTexture;
    RwTexDictionary* dictionary;
    RwBool invalid;
    RwChar name[32];
    if (data->bumped_texture) { RwTextureDestroy(data->bumped_texture); data->bumped_texture = NULL; }
    if (data->texture) { RwTextureDestroy(data->texture); data->texture = NULL; data->coefficient = 0.0f; }
    if (bumpTexture) {
        data->bumped_texture = bumpTexture;
        bumpTexture->ref_count++;
        invalid = bumpTexture->raster->width == 0;
        baseTexture = material->texture;
        if (!invalid && baseTexture) invalid = baseTexture->raster->width == 0;
        if (!invalid) {
            memcpy(name, emptyName, sizeof(name));
            GenBumpedTextureName(name, baseTexture, bumpTexture);
            dictionary = RwTexDictionaryGetCurrent();
            data->texture = NULL;
            if (dictionary) data->texture = RwTexDictionaryFindNamedTexture(dictionary, name);
            if (!data->texture) {
                data->texture = _rpMatFXSetupBumpMapTexture(baseTexture, bumpTexture);
                if (!data->texture) return NULL;
                if (dictionary) RwTexDictionaryAddTexture(dictionary, data->texture);
            } else {
                data->texture->ref_count++;
            }
            data->coefficient = 1.0f / data->texture->raster->width;
        }
    } else {
        data->coefficient = 1.0f / material->texture->raster->width;
    }
    return material;
}

RpMaterial* RpMatFXMaterialSetBumpMapFrame(RpMaterial* material, RwFrame* frame)
{
    RpMatFXBumpMapData* data =
        MatFXGetData(material, rpMATFXEFFECTBUMPMAP);
    data->frame = frame;
    return material;
}
RpMaterial* RpMatFXMaterialSetBumpMapCoefficient(RpMaterial* material, RwReal coefficient)
{
    RpMatFXBumpMapData* data =
        MatFXGetData(material, rpMATFXEFFECTBUMPMAP);
    data->storedCoefficient = -coefficient;
    return material;
}
RwFrame* RpMatFXMaterialGetBumpMapFrame(const RpMaterial* material)
{
    const RpMatFXBumpMapData* data =
        MatFXGetConstData(material, rpMATFXEFFECTBUMPMAP);
    return data->frame;
}
RwReal RpMatFXMaterialGetBumpMapCoefficient(const RpMaterial* material)
{
    const RpMatFXBumpMapData* data =
        MatFXGetConstData(material, rpMATFXEFFECTBUMPMAP);
    return -data->storedCoefficient;
}

RpMaterial* RpMatFXMaterialSetEnvMapTexture(RpMaterial* material, RwTexture* texture)
{
    RpMatFXEnvMapData* data = MatFXGetData(material, rpMATFXEFFECTENVMAP);
    texture->ref_count++;
    if (data->texture) { RwTextureDestroy(data->texture); data->texture = NULL; }
    data->texture = texture;
    return material;
}
RpMaterial* RpMatFXMaterialSetEnvMapFrame(RpMaterial* material, RwFrame* frame)
{
    RpMatFXEnvMapData* data =
        MatFXGetData(material, rpMATFXEFFECTENVMAP);
    data->frame = frame;
    return material;
}
RpMaterial* RpMatFXMaterialSetEnvMapFrameBufferAlpha(RpMaterial* material, RwBool alpha)
{
    RpMatFXEnvMapData* data =
        MatFXGetData(material, rpMATFXEFFECTENVMAP);
    data->useFrameBufferAlpha = alpha;
    return material;
}
RpMaterial* RpMatFXMaterialSetEnvMapCoefficient(RpMaterial* material, RwReal coefficient)
{
    RpMatFXEnvMapData* data =
        MatFXGetData(material, rpMATFXEFFECTENVMAP);
    data->coefficient = coefficient;
    return material;
}
RwTexture* RpMatFXMaterialGetEnvMapTexture(const RpMaterial* material)
{
    const RpMatFXEnvMapData* data =
        MatFXGetConstData(material, rpMATFXEFFECTENVMAP);
    return data->texture;
}
RwFrame* RpMatFXMaterialGetEnvMapFrame(const RpMaterial* material)
{
    const RpMatFXEnvMapData* data =
        MatFXGetConstData(material, rpMATFXEFFECTENVMAP);
    return data->frame;
}
RwBool RpMatFXMaterialGetEnvMapFrameBufferAlpha(const RpMaterial* material)
{
    const RpMatFXEnvMapData* data =
        MatFXGetConstData(material, rpMATFXEFFECTENVMAP);
    return data->useFrameBufferAlpha;
}
RwReal RpMatFXMaterialGetEnvMapCoefficient(const RpMaterial* material)
{
    const RpMatFXEnvMapData* data =
        MatFXGetConstData(material, rpMATFXEFFECTENVMAP);
    return data->coefficient;
}

RpMaterial* RpMatFXMaterialSetDualTexture(RpMaterial* material, RwTexture* texture)
{
    RpMatFXDualData* data = MatFXGetData(material, rpMATFXEFFECTDUAL);
    texture->ref_count++;
    if (data->texture) { RwTextureDestroy(data->texture); data->texture = NULL; }
    data->texture = texture;
    _rpMatFXSetupDualRenderState(data, 2);
    _rpMatFXSetupDualRenderState(data, 1);
    return material;
}
RpMaterial* RpMatFXMaterialSetDualBlendModes(RpMaterial* material,
    RwBlendFunction srcBlend, RwBlendFunction dstBlend)
{
    RpMatFXDualData* data = MatFXGetData(material, rpMATFXEFFECTDUAL);
    data->srcBlendMode = srcBlend; data->dstBlendMode = dstBlend;
    _rpMatFXSetupDualRenderState(data, 10);
    _rpMatFXSetupDualRenderState(data, 11);
    return material;
}
RwTexture* RpMatFXMaterialGetDualTexture(const RpMaterial* material)
{
    const RpMatFXDualData* data =
        MatFXGetConstData(material, rpMATFXEFFECTDUAL);
    return data->texture;
}
const RpMaterial* RpMatFXMaterialGetDualBlendModes(const RpMaterial* material,
    RwBlendFunction* srcBlend, RwBlendFunction* dstBlend)
{
    const RpMatFXDualData* data = MatFXGetConstData(material, rpMATFXEFFECTDUAL);
    *srcBlend = data->srcBlendMode;
    *dstBlend = data->dstBlendMode;
    return material;
}
RpMaterial* RpMatFXMaterialSetUVTransformMatrices(RpMaterial* material,
    RwMatrix* base, RwMatrix* dual)
{
    RpMatFXUVTransformData* data = MatFXGetData(material, rpMATFXEFFECTUVTRANSFORM);
    data->baseTransform = base; data->dualTransform = dual; return material;
}
const RpMaterial* RpMatFXMaterialGetUVTransformMatrices(const RpMaterial* material,
    RwMatrix** base, RwMatrix** dual)
{
    const RpMatFXUVTransformData* data = MatFXGetConstData(material, rpMATFXEFFECTUVTRANSFORM);
    if (base) *base = data->baseTransform;
    if (dual) *dual = data->dualTransform;
    return material;
}
