#include "movie/sfx_set.h"
#include "sofdec/sfx.h"

extern s32 SFXA_IsNeedUpdateLumiTbl(SFXAObject* object);
extern void SFXA_MakeAlp3110Tbl(SFXAObject* object, s32 adjustment,
                                void* table);
extern void SFXA_MakeAlp3211Tbl(SFXAObject* object, s32 adjustment,
                                void* table);
extern void SFXA_MakeAlpLumiTbl(SFXAObject* object, s32 adjustment,
                                void* table);
extern void SFXZ_MakeCnvZTbl(SFXZObject* object, s32 adjustment,
                             void* table);
extern void CFT_MakeArgb8888ColAdjTbl(void* table);
extern void CFT_MakeYcc422ColAdjTbl(void* table);

void SFX_MakeTable(SFXHandle* handle, SFXFrameInfo* frame,
                   s32 composition_mode)
{
    s32 make_table = 1;
    s32 i;
    s8* table;

    if (handle->field_3c == 100) {
        make_table = 0;
    } else if (handle->field_3c == composition_mode) {
        switch (composition_mode) {
        case 0:
        case 100:
            break;
        case 2:
            if (SFXA_IsNeedUpdateLumiTbl(handle->alpha) != 1) {
                make_table = 0;
            }
            break;
        case 1:
        case 4:
        case 5:
        case 21:
        case 22:
            make_table = 0;
            break;
        default:
            break;
        }
    }

    if (make_table != 1) {
        return;
    }

    handle->field_3c = composition_mode;
    switch (composition_mode) {
    case 11:
    case 13:
        SFXZ_MakeCnvZTbl(handle->depth, frame->color_adjustment,
                         handle->work_0);
        break;
    case 2:
        SFXA_MakeAlpLumiTbl(handle->alpha, frame->color_adjustment,
                            handle->work_0);
        break;
    case 4:
        SFXA_MakeAlp3110Tbl(handle->alpha, frame->color_adjustment,
                            handle->work_0);
        break;
    case 5:
        SFXA_MakeAlp3211Tbl(handle->alpha, frame->color_adjustment,
                            handle->work_0);
        break;
    case 21:
        CFT_MakeArgb8888ColAdjTbl(handle->work_0);
        break;
    case 22:
        CFT_MakeYcc422ColAdjTbl(handle->work_0);
        break;
    case 1:
        table = handle->work_0;
        for (i = 0; i < 16; i++) {
            table[i] = 0;
        }
        for (i = 16; i < 236; i++) {
            table[i] = 1.164f * (i - 16);
        }
        for (i = 236; i < 256; i++) {
            table[i] = 255;
        }
        break;
    case 0:
    case 3:
    case 100:
    default:
        SFXLIB_Error(handle, (void*)frame,
                     "E201311: sfxcnv_MakeTable : compo is not support.");
        break;
    }
}

s32 SFX_DecideTableAlph3(SfxTagInfo* info, s32 composition_mode)
{
    s32 effect_type;
    s32 table;

    if (composition_mode == 0x51) {
        table = 4;
    } else if (composition_mode == 0x61) {
        table = 5;
    } else {
        effect_type = SFX_GetFxType(info);
        if (effect_type == 0x51) {
            table = 4;
        } else if (effect_type == 0x61) {
            table = 5;
        } else {
            table = 5;
        }
    }
    return table;
}

s32 sfxcnv_IsCnvUpHalf(const SFXHandle* handle)
{
    switch (handle->stream_info) {
    case 0x11:
    case 0x31:
    case 0x41:
    case 0x51:
    case 0x61:
    case 0x71:
    case 0xf1:
    case 0x111:
    case 0x1001:
        return 0;
    case 0x21:
    case 0x101:
        return 1;
    default:
        SFXLIB_Error(0, 0,
                     "E201312: sfxcnv_IsCnvUpHalf : compo is invalid.");
        return 0;
    }
}

void SFX_SetBottomUpPlnBuf(SFXPlaneBuffer* plane)
{
    plane->pixels = (u8*)plane->pixels + plane->pitch * (plane->height - 1);
    plane->pitch = -plane->pitch;
}

const int gap_04_8031785C_rodata = 0;
