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

#define STORAGE_MAX_DEVICES 2
#define STORAGE_MAX_SLOTS 7
#define STORAGE_DEVICE_STRIDE 0x28F0
#define STORAGE_PROFILE_SIZE 0x5C0
#define STORAGE_NAME_LEN 0xB
#define STORAGE_SETTINGS_SIZE 0x70
#define STORAGE_LOAD_SIZE 0x28B8 /* device stride - settings off */
#define MEMCARD_SCAN_PID 0x300B

/* Retail MKD save footprint in blocks (space-needed = this - freeBlocks). */
#define STORAGE_BLOCKS_REQUIRED 0x3A

/*
 * Device status codes (StorageDevice.status). Display mapping is
 * find_device_display_status; present/error/full tables live in memcard.c.
 */
#define STORAGE_STATUS_OK 0
#define STORAGE_STATUS_ABSENT 1
#define STORAGE_STATUS_NO_FILE 2
#define STORAGE_STATUS_ERROR_A 3
#define STORAGE_STATUS_ERROR_B 4
#define STORAGE_STATUS_FULL 5
#define STORAGE_STATUS_BROKEN_FILE 6
#define STORAGE_STATUS_UNFORMATTED 7
#define STORAGE_STATUS_8 8
#define STORAGE_STATUS_FORMAT_NEEDED 9
#define STORAGE_STATUS_10 10
#define STORAGE_STATUS_FORMAT_ALT 0xb

/*
 * One profile slot inside a device (0x5C0). Aligns with PlayerProfile for
 * name @ +0x08, PIN @ +0x13, icon @ +0x19; present @ +0x5B4; idChecksum @ +0x5B8.
 */
typedef struct StorageProfileSlot {
    char pad00[8]; /* +0x00 */
    char name[STORAGE_NAME_LEN]; /* +0x08 */
    unsigned char pin[6]; /* +0x13 */
    unsigned char icon; /* +0x19 -- PPWLS icon index into ppwls_icon[] */
    unsigned char pad1A[2];
    int view_stats_early[3][2]; /* +0x1C .. +0x33 */
    int arcade_finishes; /* +0x34 -- view profile arcade finishes */
    unsigned char pad38[0x40 - 0x38]; /* +0x38 .. +0x3F */
    int koins[6]; /* +0x40 -- view profile koin counts */
    unsigned char pad58[0x148 - 0x58];
    ProfileUnlockBits64 unlock_cat1; /* +0x148 */
    ProfileUnlockBits64 unlock_cat2; /* +0x150 */
    unsigned int unlock_cat3; /* +0x158 */
    unsigned int pad15C;
    ProfileUnlockBits64 unlock_cat4; /* +0x160 */
    unsigned int unlock_cat5; /* +0x168 */
    unsigned int unlock_cat6; /* +0x16C, indexed from bit 10 */
    ProfileUnlockBits64 unlock_cat7; /* +0x170 */
    ProfileUnlockBits64 unlock_cat8; /* +0x178 */
    ProfileUnlockBits64 unlock_cat9; /* +0x180 */
    unsigned int unlock_cat10; /* +0x188 */
    unsigned int pad18C;
    unsigned char pad190[0x4FC - 0x190];
    int view_stats_mid[3][2]; /* +0x4FC .. +0x513 */
    int bg_team_valid;        /* +0x514 */
    int bg_team[5];           /* +0x518 .. +0x52B */
    int view_stats_late[3][2]; /* +0x52C .. +0x543 */
    unsigned char pad544[0x5B4 - 0x544];
    int present; /* +0x5B4 */
    int idChecksum; /* +0x5B8 */
    unsigned char pad5BC[0x5C0 - 0x5BC]; /* +0x5BC .. +0x5BF */
} StorageProfileSlot; /* 0x5C0 */

/*
 * Per-device memcard status blob (retail stride 0x28F0).
 * profileCount is a byte @ +0xC (stb/lbz); name[0xB] follows @ +0xD.
 */
typedef struct StorageDevice {
    int status; /* +0x00 */
    unsigned int freeBlocks; /* +0x04 */
    int freeBytes; /* +0x08 */
    unsigned char profileCount; /* +0x0C */
    char name[STORAGE_NAME_LEN]; /* +0x0D .. +0x17 */
    int inUse[STORAGE_MAX_SLOTS]; /* +0x18 .. +0x33 */
    int pad34; /* +0x34 */
    GameSettings settings; /* +0x38 .. +0xA3 */
    unsigned char settings_padA4[4]; /* +0xA4 .. +0xA7 */
    StorageProfileSlot profiles[STORAGE_MAX_SLOTS]; /* +0xA8 */
    unsigned char pad28E8[8]; /* +0x28E8 .. +0x28EF */
} StorageDevice; /* 0x28F0 */

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
