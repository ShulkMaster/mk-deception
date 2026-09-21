#include "sofdec/sfx.h"

typedef struct CFTYcc420Planar {
    const u8* y;
    const u8* cb;
    const u8* cr;
    s32 y_stride;
    s32 cb_stride;
    s32 cr_stride;
    s32 reserved_18[4];
} CFTYcc420Planar;

typedef char CFTYcc420PlanarSizeCheck[
    sizeof(CFTYcc420Planar) == 0x28 ? 1 : -1];

extern void CFT_Ycc420plnToY84C44(const CFTYcc420Planar* source,
                                   void* y, void* c, s32 y_pitch,
                                   s32 height);

static inline s32 sfxcnv_GetOutHeight(SFXHandle* handle, SFXFrameInfo* frame)
{
    s32 height = handle->output_height;

    if (height == 0) {
        height = frame->y.height;
    }
    if (sfxcnv_IsCnvUpHalf(handle) == 1) {
        height /= 2;
    }
    return height;
}

void SFX_CnvFrmYcc420plnToY84C44(SFXHandle* handle,
                                  SFXFrameInfo* frame,
                                  void* y, void* c)
{
    CFTYcc420Planar source;
    s32 pitch;
    s32 height;

    switch (handle->composition_mode) {
    case 0x11:
    case 0x101:
        source.y = frame->y.pixels;
        source.cb = frame->cb.pixels;
        source.cr = frame->cr.pixels;
        source.y_stride = frame->y.width;
        source.cb_stride = frame->cb.width;
        source.cr_stride = frame->cr.width;

        pitch = handle->output_width == 0 ? frame->y.width : handle->output_width;
        height = sfxcnv_GetOutHeight(handle, frame);
        CFT_Ycc420plnToY84C44(&source, y, c, pitch, height);
        break;
    case 0x21:
    case 0x31:
    case 0x41:
    case 0x51:
    case 0x61:
    case 0x71:
    case 0xF1:
    case 0x111:
    case 0x1001:
    default:
        SFXLIB_Error(handle, frame,
                     "E201192: CnvToY84C44 : compo is not support.");
        break;
    }
}
