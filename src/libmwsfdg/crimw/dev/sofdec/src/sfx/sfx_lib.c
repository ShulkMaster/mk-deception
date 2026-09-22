#include "dolphin/types.h"
#include "runtime/cstring.h"
#include "sofdec/sfx.h"


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

extern SFXZObject* SFXZ_Create(void);
extern void SFXZ_Destroy(SFXZObject* object);
extern void SFXZ_Finish(void);
extern void SFXZ_Init(void);
extern SFXAObject* SFXA_Create(void);
extern void SFXA_Destroy(SFXAObject* object);
extern void SFXA_Finish(void);
extern void SFXA_Init(void);
extern void SFXSUD_Finish(void);
extern void SFXSUD_Init(void);
extern void CFT_Ycc420plnToArgb8888Init(void);

const char sfx_ver_str[] =
    "\nCRI SFX/GC Ver.2.08 Build:Sep  3 2004 11:38:56\n";

s32 sfx_init_cnt = 0;
s32 sfxcnv_forcesplit = 0;
SFXLibraryWork sfx_libwork;
static const char* sfx_dummy;

s32 SFX_GetCcirFx(void) {
    return sfx_libwork.ccir_fx;
}

void SFXLIB_Error(SFXHandle* handle, SFXFrameInfo* frame,
                  const char* message) {
    SFXErrorCallback callback = sfx_libwork.error_callback;
    void* object = sfx_libwork.error_object;

    sfx_libwork.error_count++;
    if (callback != 0) {
        callback(object, message);
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

static inline s32 sfx_IsEnoughWork(s32 buffer_size) {
    return buffer_size >= 0x301f;
}

SFXHandle* SFX_Create(void* buffer, s32 buffer_size) {
    SFXHandle* handle = SFX_FindFreeHandle();
    SFXZObject* depth;
    SFXAObject* alpha;

    if (handle == 0) {
        return handle;
    }

    if (sfx_IsEnoughWork(buffer_size) != 1) {
        SFXLIB_Error(0, 0, "E201194: sfx_InitHn: work size is short.");
        return 0;
    }

    memset(handle, 0, sizeof(*handle));
    handle->composition_mode = 0;
    handle->effect_type = 0x11;
    handle->output_width = 0;
    handle->output_height = 0;
    handle->depth_enabled = 1;
    handle->field_30 = 0;
    handle->table_type = 0;
    handle->work_buffers[0] = (u8*)(((u32)buffer + 31) & ~31);
    handle->work_buffers[1] = handle->work_buffers[0] + 0x400;
    handle->work_buffers[2] = handle->work_buffers[1] + 0x400;
    handle->work_buffers[3] = handle->work_buffers[2] + 0x400;
    handle->work = buffer;
    handle->work_size = buffer_size;
    handle->frame_number = -1;
    handle->field_74 = 0;
    handle->active = 1;

    depth = SFXZ_Create();
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
        sfx_dummy = sfx_ver_str;
        memset(&sfx_libwork, 0, sizeof(sfx_libwork));
        sfx_libwork.handle_count = 8;
        sfx_libwork.ccir_fx = 1;
        CFT_Ycc420plnToArgb8888Init();
        SFXSUD_Init();
        SFXZ_Init();
        SFXA_Init();
        sfxcnv_forcesplit = 0;
        sfx_init_cnt++;
    }
}
