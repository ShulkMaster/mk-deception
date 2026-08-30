#include "dolphin/types.h"
#include "runtime/cstring.h"

typedef struct SFXAObject SFXAObject;
typedef struct SFXZObject SFXZObject;
typedef void (*SFXErrorCallback)(void* object, const char* message);

typedef struct SFXHandle {
    s32 active;
    s32 frame_status;
    s32 format;
    s32 field_count;
    s32 field_10;
    u8 reserved_14[0x14];
    SFXZObject* depth;
    s32 depth_enabled;
    s32 field_30;
    SFXAObject* alpha;
    u8 reserved_38[4];
    s32 field_3c;
    void* work_0;
    void* work_1;
    void* work_2;
    void* depth_work;
    u8 reserved_50[8];
    void* buffer;
    s32 buffer_size;
    u8 reserved_60[8];
    s32 frame_number;
    u8 reserved_6c[8];
    s32 field_74;
} SFXHandle;

typedef struct SFXLibraryWork {
    s32 active_count;
    s32 handle_count;
    SFXErrorCallback error_callback;
    void* error_object;
    s32 error_count;
    s32 ccir_fx;
    SFXHandle handles[8];
    u8 reserved_3d8[0x10];
} SFXLibraryWork;

extern SFXZObject* SFXZ_Create(void* work, s32 enabled, s32 format, s32 flags);
extern void SFXZ_Destroy(SFXZObject* object);
extern void SFXZ_Finish(void);
extern void SFXZ_Init(void);
extern SFXAObject* SFXA_Create(void);
extern void SFXA_Destroy(SFXAObject* object);
extern void SFXA_Finish(void);
extern void SFXA_Init(void);
extern void SFXSUD_Finish(void);
extern void SFXSUD_Init(void);
extern void CFT_Ycc420plnToArgb8888Init(SFXLibraryWork* work, s32 count);

const char sfx_ver_str[] =
    "\nCRI SFX/GC Ver.2.08 Build:Sep  3 2004 11:38:56\n";

static const char* sfx_dummy;
SFXLibraryWork sfx_libwork;
s32 sfxcnv_forcesplit;
s32 sfx_init_cnt;

s32 SFX_GetCcirFx(void) {
    return sfx_libwork.ccir_fx;
}

void SFXLIB_Error(void* unused0, void* unused1, const char* message) {
    SFXErrorCallback callback = sfx_libwork.error_callback;

    sfx_libwork.error_count++;
    if (callback != 0) {
        callback(sfx_libwork.error_object, message);
    }
}

void SFX_Destroy(SFXHandle* handle) {
    if (handle != 0) {
        SFXZObject* depth = handle->depth;
        SFXAObject* alpha = handle->alpha;

        handle->active = 0;
        SFXZ_Destroy(depth);
        SFXA_Destroy(alpha);
        sfx_libwork.active_count--;
    }
}

static inline SFXHandle* SFX_FindFreeHandle(void) {
    SFXHandle* handle = sfx_libwork.handles;
    s32 remaining = sfx_libwork.handle_count;

    while (remaining > 0) {
        if (handle->active == 0) {
            return handle;
        }
        handle++;
        remaining--;
    }
    return 0;
}

SFXHandle* SFX_Create(void* buffer, s32 buffer_size) {
    /* Soft ceiling: retail keeps the initializer constants in call registers;
     * this clean form reloads them before SFXZ_Create. */
    SFXHandle* handle = SFX_FindFreeHandle();
    SFXZObject* depth;
    SFXAObject* alpha;
    s32 zero;
    s32 format;
    s32 enabled;

    if (handle == 0) {
        return 0;
    }

    if ((s64)buffer_size >= 0x301f) {
    } else {
        SFXLIB_Error(0, 0, "E201194: sfx_InitHn: work size is short.");
        return 0;
    }

    memset(handle, 0, sizeof(*handle));
    zero = 0;
    format = 0x11;
    enabled = 1;
    handle->frame_status = zero;
    handle->format = format;
    handle->field_count = zero;
    handle->field_10 = zero;
    handle->depth_enabled = enabled;
    handle->field_30 = zero;
    handle->field_3c = zero;
    handle->work_0 = (void*)(((u32)buffer + 31) & ~31);
    handle->work_1 = (u8*)handle->work_0 + 0x400;
    handle->work_2 = (u8*)handle->work_1 + 0x400;
    handle->depth_work = (u8*)handle->work_2 + 0x400;
    handle->buffer = buffer;
    handle->buffer_size = buffer_size;
    handle->frame_number = -1;
    handle->field_74 = zero;
    handle->active = enabled;

    depth = SFXZ_Create(handle->depth_work, enabled, format, zero);
    if (depth == 0) {
        SFXLIB_Error(0, 0, "E201281: SfxZHn: can't create.");
        SFX_Destroy(handle);
        return 0;
    }
    handle->depth = depth;

    alpha = SFXA_Create();
    if (alpha == 0) {
        SFXLIB_Error(0, 0, "E202011: SfxAHn: can't create.");
        SFX_Destroy(handle);
        return 0;
    }
    handle->alpha = alpha;

    sfx_libwork.active_count++;
    return handle;
}

void SFX_SetErrFn(SFXErrorCallback callback, void* object) {
    sfx_libwork.error_callback = callback;
    sfx_libwork.error_object = object;
}

void SFX_Finish(void) {
    if (sfx_init_cnt > 0) {
        SFXZ_Finish();
        SFXA_Finish();
        SFXSUD_Finish();
        sfx_init_cnt--;
    }
}

void SFX_Init(void) {
    if (sfx_init_cnt < 1) {
        s32 handle_count = 8;

        sfx_dummy = sfx_ver_str;
        memset(&sfx_libwork, 0, sizeof(sfx_libwork));
        sfx_libwork.handle_count = handle_count;
        sfx_libwork.ccir_fx = 1;
        CFT_Ycc420plnToArgb8888Init(&sfx_libwork, handle_count);
        SFXSUD_Init();
        SFXZ_Init();
        SFXA_Init();
        sfxcnv_forcesplit = 0;
        sfx_init_cnt++;
    }
}
