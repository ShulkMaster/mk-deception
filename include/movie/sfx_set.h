#ifndef MOVIE_SFX_SET_H
#define MOVIE_SFX_SET_H

#include "cri/sj.h"

typedef struct SfxEffect SfxEffect;

typedef struct SfxTagInfo {
    int field_0x00;
    int composition_mode;
    int effect_type;
    int output_buffer_size;
    int output_buffer_count;
    int unit_width;
    int tag_info_set;
    union {
        int tag_start;
        void* tag_data;
    };
    int tag_size;
    char pad24[4];
    SfxEffect* effect;
    char pad2C[0x0C];
    int color_adjustment;
} SfxTagInfo;

typedef SJCK SfxDataRange;

void SFXZ_SetTagInf(SfxEffect* effect, void* data, int size);

int SFX_GetColAdj(SfxTagInfo* info);
void SFX_SetColAdj(SfxTagInfo* info, int adjustment);
void SFX_GetTagInf(SfxTagInfo* info, int* tag_start, int* tag_size);
void SFX_SetTagInf(SfxTagInfo* info, void* data, int size);
void SFX_SetUnitWidth(SfxTagInfo* info, int width);
void SFX_SetOutBufSize(SfxTagInfo* info, int buffer_size, int buffer_count);
int SFX_GetFxType(SfxTagInfo* info);
void SFX_SetFxType(SfxTagInfo* info, int effect_type);
void SFX_SetCompoMode(SfxTagInfo* info, int mode);

#endif
