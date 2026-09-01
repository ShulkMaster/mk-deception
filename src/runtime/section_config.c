#include "runtime/section_types.h"

extern unsigned char reaction_xfer_him[];
extern unsigned char inplaceGeometryCreate_80056E98[];
extern unsigned char x_attack_1[];
extern unsigned char drone_ai_push_watcher[];
extern unsigned char load_background[];
extern unsigned char pz_fighter_classify_move_8012260C[];
extern unsigned char rotate_xz[];
extern unsigned char render_post_3D_effect[];
extern unsigned char is_rumble_available[];

static SectionPerSlotDef gameart_slots[] = {{0x00, 0x59000}, {-1, 0}};
static SectionPerSlotDef fightingart_slots[] = {{0x05, 0x87000}, {-1, 0}};
static SectionPerSlotDef global_anims_slots[] = {
    {0x06, (unsigned int)(reaction_xfer_him + 0xC3C)}, {-1, 0},
};
static SectionPerSlotDef chess_global_anims_slots[] = {
    {0x5E, (unsigned int)(inplaceGeometryCreate_80056E98 + 0x168)}, {-1, 0},
};
static SectionPerSlotDef character_slots[] = {
    {0x0A, 0x8C800}, {0x0B, 0x90800},
    {0x0C, (unsigned int)(reaction_xfer_him + 0xC3C)},
    {0x0D, 0x2800}, {0x0E, 0x2800}, {0x0F, 0x2800}, {-1, 0},
};
static SectionPerSlotDef pz_character_slots[] = {
    {0x0A, 0x8C800}, {0x0B, 0x90800},
    {0x0C, (unsigned int)(x_attack_1 + 0xB4)}, {-1, 0},
};
static SectionPerSlotDef aux_slots[] = {{0x14, 0xF0000}, {0x15, 0x3E800}, {-1, 0}};
static SectionPerSlotDef aux_reduced_slots[] = {{0x5C, 0x45800}, {-1, 0}};
static SectionPerSlotDef background_slots[] = {{0x1E, 0x244800}, {-1, 0}};
static SectionPerSlotDef ladder_background_slots[] = {{0x6D, 0x2BB800}, {-1, 0}};
static SectionPerSlotDef menu_slots[] = {{0x46, 0x531800}, {-1, 0}};
static SectionPerSlotDef pselect_slots[] = {
    {0x6A, 0x510800}, {0x6B, 0x10800}, {0x6C, 0x10800}, {-1, 0},
};
static SectionPerSlotDef puzzle_slots[] = {
    {0x33, 0x208800}, {0x34, 0x5A800}, {0x35, 0x5A800},
    {0x36, 0x19000}, {0x37, 0x60800}, {0x38, 0x73000},
    {0x51, (unsigned int)(drone_ai_push_watcher + 0x110)},
    {0x39, 0x45800}, {0x3A, (unsigned int)(load_background + 0xD0)}, {-1, 0},
};
static SectionPerSlotDef konquest_slots[] = {
    {0x29, 0x2BB800}, {0x2B, 0x21D800}, {0x2C, 0x17000},
    {0x2D, 0x17000},
    {0x2E, (unsigned int)(pz_fighter_classify_move_8012260C + 0x1F4)},
    {0x30, 0x88000}, {0x28, (unsigned int)(rotate_xz + 0x2C)},
    {0x27, 0x69000}, {0x26, 0x6A000},
    {0x25, 0x62000}, {-1, 0},
};
static SectionPerSlotDef konquest_monk_slots[] = {{0x2A, 0x16000}, {-1, 0}};
static SectionPerSlotDef konquest_interior_slots[] = {{0x2F, 0x6C000}, {-1, 0}};
static SectionPerSlotDef mk_chess_saved_piece_slots[] = {
    {0x52, 0x1A800}, {0x53, 0x1A800}, {0x54, 0x1A800}, {0x55, 0x1A800}, {-1, 0},
};
static SectionPerSlotDef mk_chess_reloaded_piece_slots[] = {
    {0x56, 0x1A800}, {0x57, 0x1A800}, {0x58, 0x1A800},
    {0x59, 0x1A800}, {0x5A, 0x1A800}, {0x5B, 0x1A800}, {-1, 0},
};
static SectionPerSlotDef mk_chess_art_slots[] = {{0x3C, 0xB3000}, {-1, 0}};
static SectionPerSlotDef mk_chess_in_fight_art_slots[] = {
    {0x3E, 0x1A800}, {0x3F, (unsigned int)(render_post_3D_effect + 4)},
    {-1, 0},
};
static SectionPerSlotDef mk_chess_over_slots[] = {{0x69, 0x53000}, {-1, 0}};
static SectionPerSlotDef mk_chess_bgnd_slots[] = {
    {0x3D, 0x11E800}, {0x5D, (unsigned int)(is_rumble_available + 0x88)},
    {-1, 0},
};
static SectionPerSlotDef krypt_slots[] = {
    {0x64, 0x248000}, {0x66, 0x4A000}, {0x65, 0x17000}, {-1, 0},
};
static SectionPerSlotDef coffin_art_slots[] = {
    {0x67, 0x69000}, {0x68, 0x1E5000}, {-1, 0},
};

static SectionSlotDef fighting_mode_smm[] = {
    {0x00, gameart_slots, 0x59000}, {0x03, character_slots, 0x124800},
    {0x04, character_slots, 0x124800}, {0x01, fightingart_slots, 0x87000},
    {0x0F, global_anims_slots, 0}, {0x02, background_slots, 0x244800},
    {0x05, aux_slots, 0x12E800}, {-1, 0, 0},
};
static SectionSlotDef ladder_mode_smm[] = {
    {0x00, gameart_slots, 0x59000}, {0x03, character_slots, 0x124800},
    {0x04, character_slots, 0x124800}, {0x01, fightingart_slots, 0x87000},
    {0x0F, global_anims_slots, 0}, {0x18, ladder_background_slots, 0x2BB800},
    {0x11, aux_reduced_slots, 0x45800}, {-1, 0, 0},
};
static SectionSlotDef konquest_mode_smm[] = {
    {0x00, gameart_slots, 0x59000}, {0x0B, konquest_monk_slots, 0x16000},
    {0x0A, konquest_interior_slots, 0x6C000}, {0x06, konquest_slots, 0x6C4000},
    {-1, 0, 0},
};
static SectionSlotDef puzzle_mode_smm[] = {
    {0x00, gameart_slots, 0x59000}, {0x03, pz_character_slots, 0x11D000},
    {0x04, pz_character_slots, 0x11D000}, {0x01, fightingart_slots, 0x87000},
    {0x07, puzzle_slots, 0x3EF800}, {-1, 0, 0},
};
static SectionSlotDef mk_chess_mode_smm[] = {
    {0x00, gameart_slots, 0x59000}, {0x03, character_slots, 0x124800},
    {0x04, character_slots, 0x124800}, {0x01, fightingart_slots, 0x87000},
    {0x08, mk_chess_bgnd_slots, 0x11E800}, {0x0D, mk_chess_art_slots, 0xB3000},
    {0x0E, mk_chess_in_fight_art_slots, 0x1A800},
    {0x10, mk_chess_saved_piece_slots, 0x6A000},
    {0x12, chess_global_anims_slots, 0}, {0x11, aux_reduced_slots, 0x45800},
    {0x13, mk_chess_reloaded_piece_slots, 0x9F000}, {-1, 0, 0},
};
static SectionSlotDef mk_chess_fight_mode_smm[] = {
    {0x00, gameart_slots, 0x59000}, {0x03, character_slots, 0x124800},
    {0x04, character_slots, 0x124800}, {0x01, fightingart_slots, 0x87000},
    {0x08, mk_chess_bgnd_slots, 0x11E800}, {0x0D, mk_chess_art_slots, 0xB3000},
    {0x0E, mk_chess_in_fight_art_slots, 0x1A800},
    {0x10, mk_chess_saved_piece_slots, 0x6A000}, {0x0F, global_anims_slots, 0},
    {0x05, aux_slots, 0x12E800}, {-1, 0, 0},
};
static SectionSlotDef mk_chess_game_over_smm[] = {
    {0x00, gameart_slots, 0x59000}, {0x01, fightingart_slots, 0x87000},
    {0x05, aux_slots, 0x12E800}, {0x16, mk_chess_over_slots, 0x53000},
    {-1, 0, 0},
};
static SectionSlotDef mk_chess_mode_arttool_smm[] = {
    {0x00, gameart_slots, 0x59000}, {0x05, aux_slots, 0x12E800},
    {0x0F, global_anims_slots, 0}, {0x01, fightingart_slots, 0x87000},
    {0x08, mk_chess_bgnd_slots, 0x11E800}, {0x0D, mk_chess_art_slots, 0xB3000},
    {-1, 0, 0},
};
static SectionSlotDef menu_mode_smm[] = {
    {0x00, gameart_slots, 0x59000}, {0x03, character_slots, 0x124800},
    {0x04, character_slots, 0x124800}, {0x09, menu_slots, 0x531800},
    {-1, 0, 0},
};
static SectionSlotDef pselect_mode_smm[] = {
    {0x00, gameart_slots, 0x59000}, {0x03, character_slots, 0x124800},
    {0x04, character_slots, 0x124800}, {0x17, pselect_slots, 0x531800},
    {-1, 0, 0},
};
static SectionSlotDef krypt_mode_smm[] = {
    {0x00, gameart_slots, 0x59000}, {0x02, background_slots, 0x244800},
    {0x14, krypt_slots, 0x2A9000}, {0x15, coffin_art_slots, 0x24E000},
    {0x11, aux_reduced_slots, 0x45800}, {-1, 0, 0},
};
static SectionSlotDef attract_mode_smm[] = {
    {0x00, gameart_slots, 0x59000}, {0x09, menu_slots, 0x531800}, {-1, 0, 0},
};

SectionSlotDef* section_memory_maps[12] = {
    fighting_mode_smm, konquest_mode_smm, puzzle_mode_smm,
    mk_chess_mode_smm, menu_mode_smm, mk_chess_fight_mode_smm,
    mk_chess_mode_arttool_smm, krypt_mode_smm, mk_chess_game_over_smm,
    attract_mode_smm, pselect_mode_smm, ladder_mode_smm,
};
