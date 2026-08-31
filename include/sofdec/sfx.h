#ifndef MKD_SOFDEC_SFX_H
#define MKD_SOFDEC_SFX_H

#include "dolphin/types.h"

typedef struct SFXAObject SFXAObject;
typedef struct SFXZObject SFXZObject;
typedef void (*SFXErrorCallback)(void* object, const char* message);

typedef struct SFXHandle {
    s32 active;
    s32 stream_info;
    s32 format;
    s32 field_count;
    s32 field_10;
    s32 field_14;
    u8 reserved_18[0x10];
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

typedef struct SFXPlaneBuffer {
    void* pixels;
    s32 width;
    s32 height;
    s32 pitch;
} SFXPlaneBuffer;

typedef struct SFXFrameInfo {
    s32 format;
    SFXPlaneBuffer y;
    SFXPlaneBuffer cb;
    SFXPlaneBuffer cr;
    u8 reserved_34[0x10];
    s32 field_44;
    s32 field_48;
    s32 color_adjustment;
    u8 reserved_50[0x24];
    s32 field_74;
    u8 reserved_78[4];
} SFXFrameInfo;

typedef char SFXHandleSizeCheck[sizeof(SFXHandle) == 0x78 ? 1 : -1];
typedef char SFXFrameInfoSizeCheck[
    sizeof(SFXFrameInfo) == 0x7C ? 1 : -1];

int SFXINF_GetStmInf(const SFXFrameInfo* frame, const char* field);
void SFXLIB_Error(void* unused0, void* unused1, const char* message);
void SFX_CnvFrmYcc420plnToArgb8888(SFXHandle* handle,
                                    SFXFrameInfo* frame,
                                    void* output);
void SFX_CnvFrmYcc420plnToY84C44(SFXHandle* handle,
                                  SFXFrameInfo* frame,
                                  void* y, void* c);
void SFX_MakeTable(SFXHandle* handle, SFXFrameInfo* frame,
                   s32 composition_mode);
void SFX_SetBottomUpPlnBuf(SFXPlaneBuffer* plane);
s32 sfxcnv_IsCnvUpHalf(const SFXHandle* handle);

#endif
