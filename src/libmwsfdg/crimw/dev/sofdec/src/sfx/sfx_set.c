#include "movie/sfx_set.h"

int SJ_SearchTag(
    const SfxDataRange* source, const char* tag,
    const char* terminator, SfxDataRange* result, const char* tag_base);

int SFX_GetColAdj(SfxTagInfo* info) {
    return info->color_adjustment;
}

void SFX_SetColAdj(SfxTagInfo* info, int adjustment) {
    info->color_adjustment = adjustment;
}

void SFX_GetTagInf(
    SfxTagInfo* info, int* tag_start, int* tag_size) {
    if (info->tag_info_set != 1) {
        *tag_start = 0;
        *tag_size = 0;
        return;
    }
    *tag_start = info->tag_start;
    *tag_size = info->tag_size;
}

void SFX_SetTagInf(SfxTagInfo* info, void* data, int size) {
    SfxDataRange source;
    SfxDataRange result;
    const char* tag;
    SfxEffect* effect;

    tag = "SFXZ";
    effect = info->effect;
    source.data = data;
    source.size = size;
    info->tag_data = data;
    info->tag_size = size;
    if (SJ_SearchTag(&source, tag, "SFXINFE", &result, tag) == 0) {
        SFXZ_SetTagInf(effect, 0, 0);
    } else {
        SFXZ_SetTagInf(effect, result.data, result.size);
    }
    info->tag_info_set = 1;
}

void SFX_SetUnitWidth(SfxTagInfo* info, int width) {
    info->unit_width = width;
}

void SFX_SetOutBufSize(
    SfxTagInfo* info, int buffer_size, int buffer_count) {
    info->output_buffer_size = buffer_size;
    info->output_buffer_count = buffer_count;
}

int SFX_GetFxType(SfxTagInfo* info) {
    return info->effect_type;
}

void SFX_SetFxType(SfxTagInfo* info, int effect_type) {
    info->effect_type = effect_type;
}

void SFX_SetCompoMode(SfxTagInfo* info, int mode) {
    info->composition_mode = mode;
}
