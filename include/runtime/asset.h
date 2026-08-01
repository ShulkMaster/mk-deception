#ifndef RUNTIME_ASSET_H
#define RUNTIME_ASSET_H

#include "runtime/section_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct RwTexture RwTexture;

/*
 * SEC art decode (asset.o). Header/member layouts live in section_types.h.
 *
 * Retail: Legal screen (attract):
 *   Disc: /art/attract.ssf member legal_screen.sec @ 0xC1800, size 0x60680
 *   section_id 0x017E; members LEGAL_A (idx 0) + LEGAL_B (idx 1)
 *   load_tga(0x90046, 0x017E0000 / 0x017E0001) -> RwTexture*
 *   Pixels: GX CI8 8x4 tiled, TLUT RGB565 (gx=9, tlut=1); see gcinstance
 *   inplaceNativeTextureRead. Pad = low 5 bits of texBytes when 32-aligned.
 *
 * Retail: MAIN_MENU / mode-select natives (portrait + ScreenEngine set):
 *   Portrait (menu idle):
 *     load_ssf(msel_art_file_table)              -- "msel_art.ssf"
 *     add_art_section_by_name_async(0x90046, "msel_*.sec")
 *     wait_for_slot_load(0x90046)
 *     Color:  load_named_tga_from_slot(0x90046, "MSEL_PORTRAIT")
 *     Alpha:  load_named_alpha_texture_from_slot(0x90046, "MSEL_PORTRAIT")
 *       -- alpha is the *next* SEC member after a name match; same name string.
 *   ScreenEngine set (LoadScreenSet / LoadSetData):
 *     load_ssf(screen_engine_file_table)         -- "screen_engine.ssf"
 *     add_art_section_async(slot, sec_scr_*)     -- returns file_index (1-based)
 *     wait_for_slot_load(slot)
 *     load_named_binary_block_from_file(slot, file_index, "STRINGS", &size)
 *     load_named_binary_block_from_file(slot, file_index, "SCREEN", &size)
 *       -> pointers into SEC buffer (offset+size members; not textures)
 *     ScreenInstancer::LoadSetData(set, screenBlob, size, 0)
 *   Or scan all files in slot: load_named_binary_block(slot, "SCREEN", &size)
 *   Packed oid: load_binary_block(slot, (section_id << 16) | member_index, &size)
 * Soft ceiling: load_named_tga ~84%, load_named_alpha ~84%, load_tga ~78%,
 *   load_binary_block ~74%, get_nav/cdf_data ~69%, process_art ~96%,
 *   named binary ~87-92%, get_artid ~86%, named cdf/bloodpath ~92%
 *   (dead beq after type==ART) -- stop Matching grind.
 */

void annihilate_art_section_data(SecSlotFileEntry* entry);
void process_anim_section_data(SecSlotFileEntry* entry);
void process_art_section_data(SecSlotFileEntry* entry);

/* Packed art oid: (section_id << 16) | member_index. Callers often cast the
 * literal through char* (see load_2d_pfxobj_xy(..., (char*)0x017E0000, ...)). */
RwTexture* load_tga(int handle, unsigned int art_oid);
RwTexture* load_named_tga_from_slot(int handle, const char* name);
/* Color+alpha pair: returns texture on member after first name match. */
RwTexture* load_named_alpha_texture_from_slot(int handle, char* name);
/* Name -> packed oid for load_tga / load_binary_block. */
unsigned int get_artid_of_named_item_in_slot(
    int handle, char* name, int unused);

/*
 * Named binary SEC members (STRINGS / SCREEN / etc.).
 * Returns pointer into the loaded SEC buffer; *out_size = member size.
 * file_index is 1-based (add_art_section_async return value).
 */
void* load_named_binary_block_from_file(int handle, int file_index, char* name,
                                        int* out_size);
void* load_named_binary_block(int handle, char* name, int* out_size);
void* load_binary_block(int handle, unsigned int art_oid, int* out_size);
void* get_cdf_data(int handle, unsigned int art_oid);
void* load_named_cdf_data_from_slot(int handle, char* name);
void* load_named_bloodpath_data_from_slot(int handle, char* name);

#ifdef __cplusplus
}
#endif

#endif
