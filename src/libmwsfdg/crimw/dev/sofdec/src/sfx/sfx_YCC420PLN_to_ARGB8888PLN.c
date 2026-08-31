#include "movie/sfx_set.h"
#include "sofdec/sfx.h"

typedef struct CFTYcc420Planar {
    const u8* y;
    const u8* cb;
    const u8* cr;
    s32 y_stride;
    s32 cb_stride;
    s32 cr_stride;
    u8 reserved_18[4];
} CFTYcc420Planar;

typedef struct CFTArgb8888Output {
    SFXPlaneBuffer plane;
    u8 reserved_10[8];
} CFTArgb8888Output;

typedef char CFTYcc420PlanarSizeCheck[
    sizeof(CFTYcc420Planar) == 0x1C ? 1 : -1];
typedef char CFTArgb8888OutputSizeCheck[
    sizeof(CFTArgb8888Output) == 0x18 ? 1 : -1];

extern void CFT_Ycc420plnToArgb8888(const CFTYcc420Planar* source,
                                    const CFTArgb8888Output* destination,
                                    void* table);
extern void CFT_Ycc420plnToA256V(const CFTYcc420Planar* source,
                                 const CFTArgb8888Output* destination,
                                 void* table);
extern void CFT_Argb420ToArgb8(const void* source, void* destination,
                               s32 width, s32 height);
extern s32 SFX_DecideTableAlph3(SfxTagInfo* info, s32 composition_mode);
extern s32 SFX_GetCcirFx(void);

static inline SfxTagInfo* sfxcnv_GetTagInfo(SFXHandle* handle)
{
    /* SFXHandle exposes its tag-control prefix through the SfxTagInfo API. */
    return (SfxTagInfo*)handle;
}

static inline void sfxcnv_CnvNormalYcc420plnToArgb8888(
    SFXHandle* handle, SFXFrameInfo* frame, void* output, s32 use_table)
{
    CFTYcc420Planar source;
    CFTArgb8888Output destination;
    void* table;

    source.y = frame->y.pixels;
    source.cb = frame->cb.pixels;
    source.cr = frame->cr.pixels;
    source.y_stride = frame->y.width;
    source.cb_stride = frame->cb.width;
    source.cr_stride = frame->cr.width;

    destination.plane.pixels = output;
    destination.plane.width = frame->field_44;
    if (sfxcnv_IsCnvUpHalf(handle) == 1) {
        destination.plane.height = frame->field_48 / 2;
    } else {
        destination.plane.height = frame->field_48;
    }

    if (handle->field_count == 0) {
        destination.plane.pitch = frame->y.width * 4;
    } else if (handle->field_14 == 0) {
        destination.plane.pitch = handle->field_count / 4;
    } else {
        destination.plane.pitch = handle->field_count;
    }

    if (handle->field_74 == 1) {
        SFX_SetBottomUpPlnBuf(&destination.plane);
    }

    if (use_table == 1) {
        table = handle->work_0;
    } else {
        table = 0;
    }

    /* Both GC source orientations use the same CFT conversion entry point. */
    if (frame->field_74 == 1) {
        if (table != 0) {
            CFT_Ycc420plnToArgb8888(&source, &destination, table);
        } else {
            CFT_Ycc420plnToArgb8888(&source, &destination, 0);
        }
    } else {
        if (table != 0) {
            CFT_Ycc420plnToArgb8888(&source, &destination, table);
        } else {
            CFT_Ycc420plnToArgb8888(&source, &destination, 0);
        }
    }
}

static void sfxcnv_CnvAlphFulYcc420plnToArgb8888(
    SFXHandle* handle, SFXFrameInfo* frame, void* output)
{
    CFTYcc420Planar alpha_source;
    CFTArgb8888Output alpha_destination;
    CFTArgb8888Output destination;
    CFTYcc420Planar source;
    s32 y_offset;
    void* table;

    source.y = frame->y.pixels;
    source.cb = frame->cb.pixels;
    source.cr = frame->cr.pixels;
    source.y_stride = frame->y.width;
    source.cb_stride = frame->cb.width;
    source.cr_stride = frame->cr.width;

    destination.plane.pixels = output;
    destination.plane.width = frame->field_44;
    if (sfxcnv_IsCnvUpHalf(handle) == 1) {
        destination.plane.height = frame->field_48 / 2;
    } else {
        destination.plane.height = frame->field_48;
    }
    if (handle->field_count == 0) {
        destination.plane.pitch = frame->y.width * 4;
    } else if (handle->field_14 == 0) {
        destination.plane.pitch = handle->field_count / 4;
    } else {
        destination.plane.pitch = handle->field_count;
    }
    if (handle->field_74 == 1) {
        SFX_SetBottomUpPlnBuf(&destination.plane);
    }
    /* Both GC source orientations use the same CFT conversion entry point. */
    if (frame->field_74 == 1) {
        CFT_Ycc420plnToArgb8888(&source, &destination, 0);
    } else {
        CFT_Ycc420plnToArgb8888(&source, &destination, 0);
    }

    y_offset = frame->y.width * frame->y.height / 2;
    alpha_source.y = (const u8*)frame->y.pixels + y_offset;
    alpha_source.cb = (const u8*)frame->cb.pixels + y_offset / 2;
    alpha_source.cr = (const u8*)frame->cr.pixels + y_offset / 2;
    alpha_source.y_stride = frame->y.width;
    alpha_source.cb_stride = frame->cb.width;
    alpha_source.cr_stride = frame->cr.width;

    alpha_destination.plane.pixels = output;
    alpha_destination.plane.width = frame->field_44;
    alpha_destination.plane.height = frame->field_48 / 2;
    if (handle->field_count == 0) {
        alpha_destination.plane.pitch = frame->y.width * 4;
    } else if (handle->field_14 == 0) {
        alpha_destination.plane.pitch = handle->field_count / 4;
    } else {
        alpha_destination.plane.pitch = handle->field_count;
    }

    if (SFX_GetCcirFx() == 1) {
        table = handle->work_0;
    } else {
        table = 0;
    }
    CFT_Ycc420plnToA256V(&alpha_source, &alpha_destination, table);
}

void SFX_CnvFrmYcc420plnToArgb8888(SFXHandle* handle,
                                    SFXFrameInfo* frame,
                                    void* output)
{
    switch (handle->stream_info) {
    case 0x11:
        if (SFX_GetColAdj(sfxcnv_GetTagInfo(handle)) != 1) {
            sfxcnv_CnvNormalYcc420plnToArgb8888(handle, frame, output, 0);
            break;
        }
        SFX_MakeTable(handle, frame, 0x15);
        sfxcnv_CnvNormalYcc420plnToArgb8888(
            handle, frame, output, 1);
        break;
    case 0x101:
        sfxcnv_CnvNormalYcc420plnToArgb8888(handle, frame, output, 0);
        break;
    case 0xF1:
        CFT_Argb420ToArgb8(frame->y.pixels, output,
                           frame->y.width, frame->y.height);
        break;
    case 0x21:
        SFX_MakeTable(handle, frame, 1);
        sfxcnv_CnvAlphFulYcc420plnToArgb8888(handle, frame, output);
        break;
    case 0x31:
        SFX_MakeTable(handle, frame, 2);
        sfxcnv_CnvNormalYcc420plnToArgb8888(
            handle, frame, output, 1);
        break;
    case 0x41:
    case 0x51:
    case 0x61:
        SFX_MakeTable(
            handle, frame,
            SFX_DecideTableAlph3(sfxcnv_GetTagInfo(handle),
                                 handle->stream_info));
        sfxcnv_CnvNormalYcc420plnToArgb8888(
            handle, frame, output, 1);
        break;
    case 0x1001:
        SFX_MakeTable(handle, frame, 0x15);
        sfxcnv_CnvNormalYcc420plnToArgb8888(
            handle, frame, output, 1);
        break;
    case 0x71:
    case 0x111:
    default:
        SFXLIB_Error(
            handle, (void*)frame,
            "E201182: CnvToArgb8888 : compo is not support.");
        break;
    }
}
