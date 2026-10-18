#ifndef RUNTIME_MK_FILEINFO_H
#define RUNTIME_MK_FILEINFO_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MkFileInfo MkFileInfo;
typedef struct MkFileEntry MkFileEntry;
typedef struct MkHwFileRequest MkHwFileRequest;
typedef void (*MkFileOpenCallback)(void* user, const MkFileEntry* entry, int success);

struct MkFileInfo {
    const char* name;       /* +0x00 */
    const MkFileEntry* table;     /* +0x04: owning *_file_table */
    int type;               /* +0x08: MK_FILE_TYPE_* */
};                          /* 0x0C */

#define MK_FILE_TYPE_SECTION   1
#define MK_FILE_TYPE_ANIMATION 2
#define MK_FILE_TYPE_OBJECT    3
#define MK_FILE_TYPE_SSF       4

struct MkFileEntry {
    MkFileInfo* info;       /* +0x00 */
    unsigned int offset;    /* +0x04: byte offset in SSF */
    unsigned int size;      /* +0x08: byte length */
};                          /* 0x0C */

typedef struct SsfContext {
    const MkFileEntry* ssf_file;      /* +0x00: root entry followed by directory */
    MkHwFileRequest* hwfile;    /* +0x04 */
} SsfContext;                   /* 0x08 */

void disable_default_filesystem(void);
MkFileInfo* find_section_by_name(const char* name);
void* mk_file_read_async(void* buffer, int size, int count, const MkFileEntry* entry);
int mk_file_length(const MkFileEntry* entry);
unsigned int mk_file_read(void* buffer, unsigned int size, unsigned int count,
                          const MkFileEntry* entry);
int mk_file_close(const MkFileEntry* entry);
const MkFileEntry* mk_file_open_language(MkFileInfo* info, const char* mode, void* userdata);
const MkFileEntry* mk_file_open(MkFileInfo* info, const char* mode, void* userdata);
const MkFileEntry* mk_file_open_async_withcallback(MkFileInfo* info, const char* mode, void* userdata,
                                             MkFileOpenCallback callback, void* user);
void init_file_loading_table(void);
MkFileInfo* offset_mk_file_info(MkFileInfo* info, int language);
int get_ssf_dir_index(MkFileInfo* info);
MkFileInfo* get_mk_file_info_from_current_ssf(int index);
int num_files_in_ssf(const MkFileEntry* table);
const MkFileEntry* get_current_ssf_file(void);
void load_ssf(const MkFileEntry* ssf_entry);
void restore_previous_ssf(void);
void save_current_ssf(void);
void init_ssf_system(void);

extern const MkFileEntry ashrah_file_table[22];
extern const MkFileEntry ashrah_alt_file_table[22];
extern const MkFileEntry baraka_file_table[22];
extern const MkFileEntry baraka_alt_file_table[22];
extern const MkFileEntry boraicho_file_table[22];
extern const MkFileEntry boraicho_alt_file_table[22];
extern const MkFileEntry cassius_file_table[22];
extern const MkFileEntry cassius_alt_file_table[22];
extern const MkFileEntry dairou_file_table[22];
extern const MkFileEntry dairou_alt_file_table[22];
extern const MkFileEntry dragonking_file_table[14];
extern const MkFileEntry ermac_file_table[23];
extern const MkFileEntry ermac_alt_file_table[23];
extern const MkFileEntry freak_file_table[20];
extern const MkFileEntry goro_file_table[21];
extern const MkFileEntry goro_alt_file_table[21];
extern const MkFileEntry hotaru_file_table[22];
extern const MkFileEntry hotaru_alt_file_table[22];
extern const MkFileEntry jade_file_table[22];
extern const MkFileEntry jade_alt_file_table[22];
extern const MkFileEntry kabal_file_table[22];
extern const MkFileEntry kabal_alt_file_table[22];
extern const MkFileEntry kenshi_file_table[22];
extern const MkFileEntry kenshi_alt_file_table[22];
extern const MkFileEntry kira_file_table[22];
extern const MkFileEntry kira_alt_file_table[22];
extern const MkFileEntry kobra_file_table[22];
extern const MkFileEntry kobra_alt_file_table[22];
extern const MkFileEntry limei_file_table[22];
extern const MkFileEntry limei_alt_file_table[22];
extern const MkFileEntry liukang_file_table[22];
extern const MkFileEntry liukang_alt_file_table[22];
extern const MkFileEntry liukang_ghost_file_table[22];
extern const MkFileEntry mileena_file_table[22];
extern const MkFileEntry mileena_alt_file_table[22];
extern const MkFileEntry mkda_blaze_file_table[19];
extern const MkFileEntry mkda_cage_file_table[19];
extern const MkFileEntry mkda_drahmin_file_table[13];
extern const MkFileEntry mkda_frost_file_table[19];
extern const MkFileEntry mkda_jax_file_table[19];
extern const MkFileEntry mkda_jax_alt_file_table[19];
extern const MkFileEntry mkda_kitana_file_table[19];
extern const MkFileEntry mkda_kunglao_file_table[19];
extern const MkFileEntry mkda_nitara_file_table[19];
extern const MkFileEntry mkda_quanchi_file_table[19];
extern const MkFileEntry mkda_rayden_file_table[22];
extern const MkFileEntry mkda_shang_file_table[16];
extern const MkFileEntry mkda_sonya_file_table[19];
extern const MkFileEntry nightwolf_file_table[22];
extern const MkFileEntry nightwolf_alt_file_table[22];
extern const MkFileEntry noob_file_table[13];
extern const MkFileEntry noob_alt_file_table[13];
extern const MkFileEntry noobsmoke_file_table[20];
extern const MkFileEntry noobsmoke_alt_file_table[20];
extern const MkFileEntry raiden_file_table[22];
extern const MkFileEntry raiden_alt_file_table[22];
extern const MkFileEntry scorpion_file_table[22];
extern const MkFileEntry scorpion_alt_file_table[22];
extern const MkFileEntry shaokahn_file_table[21];
extern const MkFileEntry shaokahn_alt_file_table[21];
extern const MkFileEntry shujinko_file_table[22];
extern const MkFileEntry shujinko_alt_file_table[23];
extern const MkFileEntry shujinko_13_file_table[20];
extern const MkFileEntry sindel_file_table[22];
extern const MkFileEntry sindel_alt_file_table[22];
extern const MkFileEntry skab_file_table[22];
extern const MkFileEntry skab_alt_file_table[22];
extern const MkFileEntry smoke_file_table[17];
extern const MkFileEntry smoke_alt_file_table[17];
extern const MkFileEntry subzero_file_table[22];
extern const MkFileEntry subzero_alt_file_table[22];
extern const MkFileEntry tanya_file_table[22];
extern const MkFileEntry tanya_alt_file_table[22];
extern const MkFileEntry beetlelair_file_table[8];
extern const MkFileEntry kq_cr1_fight_file_table[43];
extern const MkFileEntry courtyard_file_table[6];
extern const MkFileEntry darkprison_file_table[38];
extern const MkFileEntry deadpool_file_table[6];
extern const MkFileEntry dka_file_table[6];
extern const MkFileEntry dragonmountain_file_table[6];
extern const MkFileEntry kq_er1_fight_file_table[29];
extern const MkFileEntry kq_er2_fight_file_table[42];
extern const MkFileEntry kq_ed1_fight_file_table[36];
extern const MkFileEntry fallingcliff_file_table[6];
extern const MkFileEntry fortress_file_table[6];
extern const MkFileEntry hellsfoundry_file_table[6];
extern const MkFileEntry kangtomb_file_table[7];
extern const MkFileEntry krypt_file_table[24];
extern const MkFileEntry kuatanpalace_file_table[6];
extern const MkFileEntry ladder_file_table[5];
extern const MkFileEntry livingforest_file_table[4];
extern const MkFileEntry lostwarriors_file_table[6];
extern const MkFileEntry lower_mines_file_table[6];
extern const MkFileEntry netherbelly_file_table[6];
extern const MkFileEntry kq_nr1_fight_file_table[42];
extern const MkFileEntry kq_nx1_fight_file_table[24];
extern const MkFileEntry nexus_arena_file_table[5];
extern const MkFileEntry kq_nexus_nis_file_table[16];
extern const MkFileEntry kq_or1_fight_file_table[37];
extern const MkFileEntry kq_ow1_fight_file_table[39];
extern const MkFileEntry portal_file_table[5];
extern const MkFileEntry pz_ladder_file_table[5];
extern const MkFileEntry reddragon_file_table[10];
extern const MkFileEntry skytemple_file_table[9];
extern const MkFileEntry slaughterhouse_file_table[10];
extern const MkFileEntry thepit_file_table[6];
extern const MkFileEntry yinyang_file_table[6];
extern const MkFileEntry gameart_file_table[5];
extern const MkFileEntry misc_anims_list_file_table[5];
extern const MkFileEntry mkchess_ingame_art_file_table[6];
extern const MkFileEntry msel_art_file_table[54];
extern const MkFileEntry pselect_file_table[31];
extern const MkFileEntry fatalityanims_file_table[55];
extern const MkFileEntry nameentryart_file_table[3];
extern const MkFileEntry puzzlefighter_file_table[49];
extern const MkFileEntry screen_engine_file_table[17];
extern const MkFileEntry mk_chess_file_table[43];
extern const MkFileEntry konquest_common_file_table[39];
extern const MkFileEntry kamidogu_earth_file_table[3];
extern const MkFileEntry kon_earthrealm_0_file_table[22];
extern const MkFileEntry kon_earthrealm_1_file_table[32];
extern const MkFileEntry kon_earthrealm_2_file_table[37];
extern const MkFileEntry kon_netherrealm_1_file_table[38];
extern const MkFileEntry kon_chaosrealm_1_file_table[27];
extern const MkFileEntry kon_outworld_1_file_table[23];
extern const MkFileEntry kon_orderrealm_1_file_table[28];
extern const MkFileEntry kon_edenia_1_file_table[25];
extern const MkFileEntry kon_nexus_1_file_table[31];
extern const MkFileEntry kon_unique_npcs_file_table[65];
extern const MkFileEntry konquest_pui_award_tgas_file_table[81];
extern const MkFileEntry loading_images_file_table[98];
extern const MkFileEntry attract_file_table[10];
extern const MkFileEntry bios_file_table[56];
extern const MkFileEntry bio_text_file_table[8];
extern const MkFileEntry krypt_art_file_table[406];
extern const MkFileEntry galleryart_file_table[16];
extern const MkFileEntry endings_file_table[32];

extern MkFileInfo sec_pz_shared_anims;
extern MkFileInfo sec_reduced_shared_anims;
extern MkFileInfo sec_hand_anims;
extern MkFileInfo sec_shared_anims;

extern MkFileInfo ssf_attract;
extern MkFileInfo ssf_msel_art;
extern MkFileInfo sec_scr_main_menu;
extern MkFileInfo sec_attract;
extern MkFileInfo sec_mkd_end_game;
extern MkFileInfo sec_legal_screen;
extern MkFileInfo sec_legal_screen_spa;
extern MkFileInfo sec_legal_screen_ger;
extern MkFileInfo sec_legal_screen_fre;
extern MkFileInfo sec_legal_screen_ita;
extern MkFileInfo sec_demo_logo;
extern MkFileInfo sec_sysart;

#ifdef __cplusplus
}
#endif

#endif
