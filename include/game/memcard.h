#ifndef GAME_MEMCARD_H
#define GAME_MEMCARD_H

/*
 * memcard.o - Midway game memcard layer (B20 APIs + B21 PPWLS chrome helpers).
 * Campaign history: docs/campaigns/index.md (B20-B22)
 * Midway wrappers: gcmcard* . Nintendo card.a out of campaign.
 */

#include "runtime/asset.h"
#include "game/profile_unlock.h"
#include "game/settings.h"

#include "game/storage_types.h"

/*
 * Stack pair passed by ScreenEngine GetImageCollection path to
 * create_*_mc_icon_list (Glue builds {textures, alphas} on stack).
 * Retail loads alphas (+0x04) but only fills textures (+0x00).
 */
typedef struct McIconListArg {
    RwTexture** textures; /* +0x00 -- 7 color TGA slots */
    RwTexture** alphas; /* +0x04 -- loaded unused by create_* */
} McIconListArg;

#define DEVICE_AT(device) (&storage_status[(device)])

int init_memcard(void);
int get_mu_access_progress(void);
void reset_storage_device_status_structure(int device);
void storage_status_change_calculations(int device);
int create_new_mk5_profile_file(int device);
void region_data_corruption_message_handler(void);
int compare_checksums(const char* left, const char* right);

/*
 * Load/save with Midway UI + retry. Call sites:
 *   insert_mu: mode 2, settings, name, nameLen=0xB, &freeBlocks, &freeBytes
 *   create_new_mk5: mode 3, title, settings, flag=1, &freeBlocks, &freeBytes
 */
int load_from_memcard_w_error(int device, int mode, void* settings, char* cardName, int nameLen,
                              unsigned int* freeBlocks, int* freeBytes);
int save_to_memcard_w_error(int device, int mode, const char* title, void* settings, int flag,
                            unsigned int* freeBlocks, int* freeBytes);
int save_settings_to_memcard_w_error(int device, int mode, const char* title,
                                     GameSettings* settings, int flag,
                                     unsigned int* freeBlocks, int* freeBytes);
int load_konquest_region_from_memcard_w_error(
    int device, int slot, int arg, int region, void* buffer, char* cardName,
    int nameLen, unsigned int* freeBlocks, int* freeBytes);

void reset_format_or_recreate_flags(void);
void check_format_or_recreate(void);
void set_wls_left_cursor(int device);
int get_wls_left_cursor(void);

void create_right_mc_icon_list(McIconListArg* arg);
void create_left_mc_icon_list(McIconListArg* arg);
void get_right_mcard_text_matrix(char** out);
void get_left_mcard_text_matrix(char** out);
char* get_right_storage_device_space_needed(void);
char* get_left_storage_device_space_needed(void);

int is_device_unformatted(int device);
int is_device_error(int device);
int is_device_full(int device);
int is_device_present(int device);
int is_storage_device_full(int device);

/*
 * Three parameters per retail caller evidence: gcmcard.o's update_storage_status
 * paths load r4=0 and r5=device before the bl, which MWCC only emits when the
 * call-site prototype has >= 3 args. Retail bodies ignore arg1/arg2.
 */
void insert_mu(int device, int arg1, int arg2);
void remove_mu(int device, int arg1, int arg2);

extern StorageDevice storage_status[STORAGE_MAX_DEVICES];
extern void* p1_profile_common;
extern void* p2_profile_common;
extern void* p1_profile_konquest;
extern void* p2_profile_konquest;
extern int g_bMemCardScreensDisabled;
extern int mu_access_progress;

#endif
