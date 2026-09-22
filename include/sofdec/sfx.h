#ifndef MKD_SOFDEC_SFX_H
#define MKD_SOFDEC_SFX_H

#include "dolphin/types.h"

typedef struct SFXAObject SFXAObject;
typedef struct SFXZObject SFXZObject;
typedef void (*SFXErrorCallback)(void* object, const char* message);

typedef struct SFXHandle {
    s32 active;
    s32 composition_mode;
    s32 effect_type;
    s32 output_width;
    s32 output_height;
    s32 unit_width;
    s32 tag_info_set;
    void* tag_data;
    s32 tag_size;
    s32 reserved_24;
    SFXZObject* depth;
    s32 depth_enabled;
    s32 field_30;
    SFXAObject* alpha;
    s32 color_adjustment;
    s32 table_type;
    u8* work_buffers[4];
    s32 reserved_50[2];
    void* work;
    s32 work_size;
    const char* picture_user_data;
    s32 picture_user_data_size;
    s32 frame_number;
    s32 reserved_6C[2];
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
    s32 frame_width;
    s32 frame_height;
    void* table_source;
    void* tag_data;
    s32 tag_size;
    s32 field_58;
    s32 field_5C;
    s32 picture_structure;
    s32 chroma_format;
    s32 field_68;
    s32 field_6C;
    s32 field_70;
    s32 chroma_position_h;
    s32 chroma_position_v;
    u8 reserved_7C[0x0C];
} SFXFrameInfo;

typedef char SFXHandleSizeCheck[sizeof(SFXHandle) == 0x78 ? 1 : -1];
typedef char SFXFrameInfoSizeCheck[
    sizeof(SFXFrameInfo) == 0x88 ? 1 : -1];

int SFXINF_GetStmInf(const SFXFrameInfo* frame, const char* field);
int SFX_GetTypeCcs(SFXHandle* handle);
int SFX_GetTypeDivField(SFXHandle* handle);
void SFX_SetPicUsrDat(SFXHandle* handle, const char* data, int size);
void SFXLIB_Error(SFXHandle* handle, SFXFrameInfo* frame,
                  const char* message);
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
