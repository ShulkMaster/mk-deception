#ifndef RUNTIME_ASSET_H
#define RUNTIME_ASSET_H

#include "runtime/section_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct RwTexture RwTexture;
typedef struct AniTextureControl AniTextureControl;
typedef struct MkObj MkObj;

void annihilate_art_section_data(SecSlotFileEntry* entry);
void process_anim_section_data(SecSlotFileEntry* entry);
void process_art_section_data(SecSlotFileEntry* entry);
MkObj* load_named_model_for_player(const char* name, int player,
                                   int object_type, int transl);
MkObj* load_named_model_for_bgnd(const char* name, int object_type, int transl);
MkObj* load_named_model_from_slot(int slot, const char* name, int object_type,
                                  int transl);
MkObj* load_model_from_slot(int handle, unsigned int art_oid, int object_type);
MkObj* load_model_from_slot_transl(int handle, unsigned int art_oid,
                                   int object_type);

RwTexture* load_tga(int handle, unsigned int art_oid);
RwTexture* load_named_tga_from_slot(int handle, const char* name);

RwTexture* load_named_alpha_texture_from_slot(int handle, const char* name);

unsigned int get_artid_of_named_item_in_slot(
    int handle, const char* name, int unused);

void* load_named_binary_block_from_file(int handle, int file_index,
                                        const char* name, int* out_size);
void* load_named_binary_block(int handle, const char* name, int* out_size);
void* load_binary_block(int handle, unsigned int art_oid, int* out_size);
void* get_nav_data(int handle, unsigned int art_oid);
void* get_cdf_data(int handle, unsigned int art_oid);
void* load_named_cdf_data_from_slot(int handle, const char* name);
void* load_named_bloodpath_data_from_slot(int handle, const char* name);
AniTextureControl* load_named_wiff_from_slot(int handle, const char* name);
AniTextureControl* get_wiff_atc_block(int handle, unsigned int art_oid);

#ifdef __cplusplus
}
#endif

#endif
