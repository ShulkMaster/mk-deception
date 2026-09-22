#include "cri/sj.h"
#include "dolphin/types.h"
#include "runtime/cstring.h"
#include "sofdec/sfx.h"

extern int sscanf(const char* input, const char* format, ...);
extern s32 SFX_GetCcirFx(void);

typedef void (*SFXZConvertCallback)(u32* source, void* destination,
                                    f32 near_clip, f32 far_clip);

struct SFXZObject {
    s32 active;
    s32 z_bit;
    s32 tag_info_set;
    u8* tag_data;
    s32 tag_size;
    s32 reserved_14;
    s32 header_info_set;
    u8* header_data;
    s32 header_size;
    s32 reserved_24;
    s32 frame_info_set;
    u8* frame_data;
    s32 frame_size;
    s32 reserved_34;
    s32 reserved_38;
    f32 near_clip;
    f32 far_clip;
    SFXZConvertCallback convert_callback;
    s32 reserved_48;
};

typedef struct SFXZWork {
    s32 active_count;
    s32 use_direct_conversion;
    s32 capacity;
    SFXZObject objects[8];
} SFXZWork;

typedef char SFXZObjectSizeCheck[sizeof(SFXZObject) == 0x4C ? 1 : -1];
typedef char SFXZWorkSizeCheck[sizeof(SFXZWork) == 0x26C ? 1 : -1];

/* These diagnostics belong to retail sfx_zmv.o even though the corresponding
 * validation paths are disabled for this platform build. */
static const char sfxzmv_invalid_zbit[] =
    "E201313: sfxcnv_CnvZbitToCft : zbit is invalid.";
static const char sfxzmv_composition_tag[] = "COMPO";
static const f32 sfxzmv_zero[1] = { 0.0f };
static const char sfxzmv_zclip_unset[] =
    "E201315: sfxcnv_CnvFrmZcmn : zclip is not set.";
static const char sfxzmv_unsupported_format[] =
    "E201191: SFX_CnvFrmZcmn : frmfmt is not support.";
static const f32 sfxzmv_minimum_clip[1] = { -14.0f };
static const char sfxzmv_invalid_zclip[] =
    "E201314: SFXZ_SetZclip : zclip is invalid.";

SFXZWork sfxz_work;

static void sfxzmv_MakeOrgZ32TblByCCIR(SFXZObject* object, u32 near_z,
                                       u32 far_z, u32* table);
static void sfxzmv_MakeOrgZ32TblByDirect(SFXZObject* object, u32 near_z,
                                         u32 far_z, u32* table);

static void sfxzmv_MakeZ32Tbl(SFXZObject* object, u32* original,
                              u32* converted)
{
    f64 near_clip = object->near_clip;
    f64 far_clip = object->far_clip;

    if (sfxz_work.use_direct_conversion == 1) {
        s32 i;

        for (i = 0; i < 0x100; i++) {
            converted[i] = original[i] & 0x7FFFFF80;
            converted[i] <<= 1;
        }
    } else {
        s32 i;
        u32* destination;
        u32* source;
        f64 reciprocal;
        f64 a;
        f64 b;

        source = original;
        destination = converted;
        reciprocal = 1.0 / (far_clip - near_clip);
        a = 16777215.0 * far_clip * reciprocal;
        b = far_clip * (16777215.0 * reciprocal * near_clip);
        for (i = 0; i < 0x100; i++) {
            if (*source == 0) {
                *source = 1;
            }
            *destination = (u32)(a -
                                 b / (far_clip * (f64)*source / 2147483647.0));
            source++;
            destination++;
        }
    }
}

static void sfxzmv_MakeZ16Tbl(SFXZObject* object, u32* original,
                              u16* converted)
{
    f64 near_clip = object->near_clip;
    f64 far_clip = object->far_clip;

    if (sfxz_work.use_direct_conversion == 1) {
        s32 i;

        for (i = 0; i < 0x100; i++) {
            converted[i] = (u16)(original[i] >> 15);
        }
    } else {
        s32 i;
        u16* destination;
        u32* source;
        f64 reciprocal;
        f64 a;
        f64 b;

        source = original;
        destination = converted;
        reciprocal = 1.0 / (far_clip - near_clip);
        a = 65535.0 * far_clip * reciprocal;
        b = far_clip * (65535.0 * reciprocal * near_clip);
        for (i = 0; i < 0x100; i++) {
            if (*source == 0) {
                *source = 1;
            }
            *destination++ = (s32)(a -
                                   b / (far_clip * (f64)*source / 2147483647.0));
            source++;
        }
    }
}

static void sfxzmv_MakeCnvZTbl(SFXZObject* object, u32 near_z, u32 far_z,
                               void* table)
{
    u32* original = (u32*)table + 0x100;

    memset(table, 0, 0x400);
    if (SFX_GetCcirFx() == 1) {
        sfxzmv_MakeOrgZ32TblByCCIR(object, near_z, far_z, original);
    } else {
        sfxzmv_MakeOrgZ32TblByDirect(object, near_z, far_z, original);
    }

    if (object->convert_callback == 0) {
        if (object->z_bit == 16) {
            sfxzmv_MakeZ16Tbl(object, original, table);
        } else {
            sfxzmv_MakeZ32Tbl(object, original, table);
        }
    } else {
        object->convert_callback(original, table, object->near_clip,
                                 object->far_clip);
    }
}

#pragma dont_inline on
void SFXZ_MakeCnvZTbl(SFXZObject* object, void* source, void* table)
{
    SJCK result1;
    SJCK source1;
    SJCK result2;
    SJCK source2;
    unsigned long frame_far_z;
    unsigned long frame_near_z;
    unsigned long frame_value;
    unsigned long record_size;
    u8* data;
    u32 near_z;
    u32 far_z;

    if (object->frame_info_set != 1 || object->frame_data == 0) {
        data = 0;
    } else {
        source1.data = object->frame_data;
        source1.len = object->frame_size;
        SJ_SearchTag(&source1, "ZMFSIZE", "SFXINFE", &result1);
        data = result1.data;
    }
    if (data == 0) {
        near_z = 0;
        far_z = 0x7FFFFFFF;
    } else {
        sscanf((const char*)data, "%lx", &record_size);
        if (object->frame_info_set != 1 || object->frame_data == 0) {
            data = 0;
        } else {
            source2.data = object->frame_data;
            source2.len = object->frame_size;
            SJ_SearchTag(&source2, "ZMFDATA", "SFXINFE", &result2);
            data = result2.data;
        }
        if (data == 0) {
            near_z = 0x7FFFFFFF;
        } else {
            sscanf((const char*)data + record_size * (u32)source,
                   "%lx %lx %lx", &frame_value, &frame_near_z,
                   &frame_far_z);
            near_z = frame_near_z;
            far_z = frame_far_z;
        }
    }

    sfxzmv_MakeCnvZTbl(object, near_z, far_z, table);
}
#pragma dont_inline off

#define SFXZ_MAKE_ORG_Z32_TBL(near_value, far_value, destination)             \
    {                                                                          \
        s32 i;                                                                 \
        u32* p;                                                                \
        if ((far_value) == 0x80000000U) {                                      \
            (far_value) = 0x7FFFFFFFU;                                        \
        }                                                                      \
        if ((near_value) == 0x80000000U) {                                     \
            (near_value) = 0x7FFFFFFFU;                                        \
        }                                                                      \
        p = (destination);                                                     \
        for (i = 0; i < 9; i++) {                                              \
            *p++ = 0;                                                          \
        }                                                                      \
        for (i = 9; i < 17; i++) {                                             \
            (destination)[i] = (near_value);                                   \
        }                                                                      \
        if ((near_value) == (far_value)) {                                     \
            for (i = 17; i < 0xE0; i++) {                                     \
                (destination)[i] = (near_value);                               \
            }                                                                  \
        } else {                                                               \
            for (i = 17; i < 0xE0; i++) {                                     \
                (destination)[i] =                                               \
                    (near_value) + (i - 17) * (((far_value) - (near_value)) / 207); \
            }                                                                  \
        }                                                                      \
        for (i = 0xE0; i < 0xF0; i++) {                                       \
            (destination)[i] = (far_value);                                    \
        }                                                                      \
        for (i = 0xF0; i < 0x100; i++) {                                      \
            (destination)[i] = 0x7FFFFFFFU;                                    \
        }                                                                      \
    }

static void sfxzmv_MakeOrgZ32TblByCCIR(SFXZObject* object, u32 near_z,
                                       u32 far_z, u32* table)
{
    s32 i;
    s32 j;
    s32 k;
    s32 l;
    u32* d;
    u8* ccir_table;
    u32* z_table;

    z_table = table + 0x100;
    ccir_table = (u8*)(z_table + 0x100);
    for (j = 0; j <= 0x0F; j++) {
        ccir_table[j] = 0;
    }
    for (i = 0x10; i <= 0xEB; i++) {
        ccir_table[i] = (u8)(1.164f * (f32)(i - 16));
    }
    for (k = 0xEC; k <= 0xFF; k++) {
        ccir_table[k] = 0xFF;
    }
    SFXZ_MAKE_ORG_Z32_TBL(near_z, far_z, z_table);
    d = table;
    for (l = 0; l <= 0xFF; l++) {
        *d++ = z_table[ccir_table[l]];
    }
}

#undef SFXZ_MAKE_ORG_Z32_TBL

static void sfxzmv_MakeOrgZ32TblByDirect(SFXZObject* object, u32 near_z,
                                         u32 far_z, u32* table)
{
    s32 i;

    if (far_z == 0x80000000U) {
        far_z = 0x7FFFFFFF;
    }
    if (near_z == 0x80000000U) {
        near_z = 0x7FFFFFFF;
    }

    for (i = 0; i < 9; i++) {
        table[i] = 0;
    }
    for (i = 9; i < 17; i++) {
        table[i] = near_z;
    }
    if (near_z == far_z) {
        for (i = 17; i < 224; i++) {
            table[i] = near_z;
        }
    } else {
        for (i = 17; i < 224; i++) {
            table[i] = near_z +
                       (i - 17) * ((far_z - near_z) / 207);
        }
    }
    for (i = 224; i < 240; i++) {
        table[i] = far_z;
    }
    for (i = 240; i < 256; i++) {
        table[i] = 0x7FFFFFFF;
    }
}

void SFXZ_SetTagInf(SFXZObject* object, void* data, s32 size)
{
    SJCK result;
    SJCK source;

    object->tag_info_set = 1;
    object->tag_data = data;
    object->tag_size = size;

    if (object->tag_data == 0) {
        object->header_info_set = 1;
        object->header_data = 0;
        object->header_size = 0;
        object->frame_info_set = 1;
        object->frame_data = 0;
        object->frame_size = 0;
        return;
    }

    source.data = object->tag_data;
    source.len = object->tag_size;
    SJ_SearchTag(&source, "ZMHDR", "SFXINFE", &result);
    object->header_info_set = 1;
    object->header_data = result.data;
    object->header_size = result.len;

    SJ_SearchTag(&source, "ZMVFRM", "SFXINFE", &result);
    object->frame_info_set = 1;
    object->frame_data = result.data;
    object->frame_size = result.len;
}

void SFXZ_Destroy(SFXZObject* object)
{
    if (object != 0) {
        object->active = 0;
        sfxz_work.active_count--;
    }
}

static inline SFXZObject* SFXZ_FindFreeObject(void)
{
    SFXZObject* object = sfxz_work.objects;
    s32 remaining = sfxz_work.capacity;

    while (remaining > 0) {
        if (object->active == 0) {
            return object;
        }
        object++;
        remaining--;
    }
    return 0;
}

SFXZObject* SFXZ_Create(void)
{
    SFXZObject* object = SFXZ_FindFreeObject();

    if (object != 0) {
        object->near_clip = sfxzmv_zero[0];
        object->far_clip = sfxzmv_zero[0];
        object->convert_callback = 0;
        object->reserved_48 = 0;
        object->z_bit = 0;
        sfxz_work.active_count++;
        object->active = 1;
    }
    return object;
}

void SFXZ_Finish(void)
{
}

void SFXZ_Init(void)
{
    memset(&sfxz_work, 0, sizeof(sfxz_work));
    sfxz_work.capacity = 8;
    sfxz_work.use_direct_conversion = 0;
}
