#ifndef GAME_PLYRPROFILE_H
#define GAME_PLYRPROFILE_H

#include "mwScreenEngine/TextureCollection.h"

#include "game/profile_unlock.h"
#include "runtime/mk_proc.h"

typedef struct StorageProfileSlot StorageProfileSlot;

/*
 * plyrprofile.o - player profiles + memcard boot PPWLS ("what's loaded").
 * Deeper CARD I/O is memcard/gcmcard. Campaign history: docs/campaigns/index.md.
 *
 * Retail: Boot chain (after legal in p_attract_mode):
 *   gamelogic_jump(6, p_player_profile_boot_screen_entry_point)
 *     -> set_section_memory_scheme(SECTION_MEMORY_SCHEME_PROFILE)
 *     -> jump_sleep(p_player_profile_whats_loaded_screen)
 *          -> load_screen("common/memory_card/mc_main") + fade + wait
 *          -> gamelogic_jump(0, p_atm_loop)
 * Soft ceiling: g_bMemCardScreensDisabled is only consulted from insert_mu;
 *   not a boot fast-path in this tree.
 *
 * Menu create/view/delete (B22): cases 18-20 in p_main_menu jump to
 *   p_create_profile / p_view_profile / p_delete_profile; exit via
 *   gamelogic_jump(6, p_main_menu).
 */

#define SECTION_MEMORY_SCHEME_PROFILE 4

#define PROFILE_SIZE 0x5C0
#define PROFILE_SWITCHMAP_COUNT 16
#define PROFILE_KONQUEST_SIZE 0x36C

/*
 * Runtime / on-card profile blob (sizeof 0x5C0). Layout shared with
 * StorageProfileSlot for name/pin/icon; unlock bitfields @ +0x148..+0x188
 * (is_mark_as_unlocked categories 1..10).
 *
 * name @ +0x08, PIN @ +0x13, icon @ +0x19, switch_map @ +0x108,
 * unlock words @ +0x148.., konquest @ +0x190 (0x36C), idChecksum @ +0x5B8.
 */
typedef struct PlayerProfile {
    unsigned char active; /* +0x00 */
    unsigned char pad01[7]; /* +0x01 */
    char name[0xB]; /* +0x08 */
    unsigned char pin[6]; /* +0x13 */
    unsigned char icon; /* +0x19 -- PPWLS index into ppwls_icon[] */
    unsigned char pad1A[0x40 - 0x1A];
    int koins[6]; /* +0x40 */
    int lifetime_koins[6]; /* +0x58 */
    unsigned char pad70[0x104 - 0x70];
    int rumble; /* +0x104 */
    int switch_map[PROFILE_SWITCHMAP_COUNT]; /* +0x108 -- word0 of each default entry */
    ProfileUnlockBits64 unlock_cat1; /* +0x148 */
    ProfileUnlockBits64 unlock_cat2; /* +0x150 */
    unsigned int unlock_cat3; /* +0x158 */
    unsigned int pad15C; /* +0x15C */
    ProfileUnlockBits64 unlock_cat4; /* +0x160 */
    unsigned int unlock_cat5; /* +0x168 */
    unsigned int unlock_cat6; /* +0x16C, indexed from bit 10 */
    ProfileUnlockBits64 unlock_cat7; /* +0x170 */
    ProfileUnlockBits64 unlock_cat8; /* +0x178 */
    ProfileUnlockBits64 unlock_cat9; /* +0x180 */
    unsigned int unlock_cat10; /* +0x188 */
    unsigned int pad18C; /* +0x18C */
    unsigned char konquest[PROFILE_KONQUEST_SIZE]; /* +0x190 */
    unsigned char pad4FC[0x5B8 - 0x4FC]; /* +0x4FC */
    int idChecksum; /* +0x5B8 */
    unsigned char pad5BC[0x5C0 - 0x5BC];
} PlayerProfile; /* 0x5C0 */

/*
 * Multi-profile list mkproc pdata: UI code/PIN at +0x14 (not PlayerProfile.pin @ +0x13).
 * Used by ppl_get_multi_profile_* walks.
 */
typedef struct PplListPdata {
    unsigned char pad00[0x14]; /* +0x00 */
    unsigned char code[6]; /* +0x14 */
} PplListPdata;

#define PPWLS_PROC_PID 0x3008
#define PPWLS_PROC_PRIO 0x23
#define PPWLS_TIMEOUT_PROC_PDATA 8

#define PPWLS_SCREEN_SLOT 0x90046
#define PPWLS_SCREEN_PATH "common/memory_card/mc_main"

#define PPWLS_EVENT_REFRESH 0x1FB7
#define PPWLS_EVENT_DONE 0x1FBE

#define PPWLS_FADE_FRAMES 10

typedef struct ProfileIconNames {
    const char* color;
    const char* alpha;
} ProfileIconNames;

extern ProfileIconNames ppwls_icon[];

float p_player_profile_boot_screen_entry_point(void);
float p_reset_ppwls_timeout(void);
void reset_ppwls_timeout(void);
void set_ppwls_input_done(void);
void init_player_profiles(void);
void unload_player_profiles(void);
void unload_p1_player_profile(void);
void unload_p2_player_profile(void);

void memory_save_profile(int player, PlayerProfile* dest);
void memory_load_profile(int player, PlayerProfile* src);
StorageProfileSlot* scan_storage_for_code(int* state, int player, int port,
                                          unsigned char* code, int* device, int* slot);
int move_profile_p1_to_p2(void);
int move_profile_p2_to_p1(void);
int count_all_profiles(void);
char* ppv_get_current_profile_name(void);
void mark_profile_as_in_use(int device, int slot);

float p_create_profile(void);
float p_view_profile(void);
float p_delete_profile(void);
void erase_player_profile(int device, int slot);
void check_new_mu_for_in_use_profiles(int device);
void ppc_set_button_answer(int answer);
void ppc_set_current_icon_selection(unsigned char icon);
int ppc_get_code_state(void);
void ppc_transition_pause(int paused);
int pne_is_name_already_used(void);
void pp_name_entry_proces_char_entry(const char* key_name);
char* get_current_create_a_profile_name(void);

void set_profile_to_default(PlayerProfile* profile);
void summarize_unlocked_items(void);
int get_coffin_bit(const unsigned char* bits, unsigned int index);
void set_coffin_bit(unsigned char* bits, unsigned int index, int value);

int is_mark_as_unlocked(PlayerProfile* profile, int category, int character);
void mark_as_unlocked(PlayerProfile* profile, int category, int character);
void mark_as_locked(PlayerProfile* profile, int category, int character);

int validate_save_location(int player);
int validate_konq_save_location(int player);
int validate_konq_load_location(int player);
void quit_from_konquest(void);

int find_device_display_status(int device);
void format_or_recreate_a_device(int device);
int does_name_already_exist(const char* name);
int ppl_get_multi_profile_count(int player);
int ppl_get_multi_profile_names_p1(char** out);
int ppl_get_multi_profile_names_p2(char** out);
void ppl_get_multi_profile_icon_p1(GVTexturePair out, int count);
void ppl_get_multi_profile_icon_p2(GVTexturePair out, int count);

void ppv_get_current_profile_koins(char* dest, int index);
void ppv_get_current_profile_arcade_finishes(char* dest);
struct McIconListArg;
void ppv_view_profile_icon_list(GVTexturePair out);
void ppv_update_profile_cursor(int delta);
void get_profile_stats(char** outs);
void format_value_to_display(char* dest, unsigned int value);
char* get_heros_name(int which);

#endif
