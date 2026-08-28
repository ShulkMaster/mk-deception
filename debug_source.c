
int gc_format_procedure(int device);
int gc_delete_file(int device, const char *fileName);
int check_load_profile_result(int *result, int device);
extern const int mcmasks[2];
typedef union ProfileUnlockBits64
{
  unsigned long long value;
  unsigned int words[2];
} ProfileUnlockBits64;
typedef struct GameSettings
{
  float volume[6];
  int kombat_difficulty;
  int arcade_difficulty;
  union 
  {
    int rounds_to_win;
    int puzzle_difficulty;
  };
  int round_time;
  int blood_level;
  union 
  {
    int brightness;
    int round_timer_value;
  };
  int damage_level;
  int combo_breaker;
  int fatalities;
  int pad_3C;
  int konquest_latch;
  int pad_44[4];
  int color_red;
  int color_blue;
  int color_green;
  int display_brightness;
  int gamma;
  int contrast;
} GameSettings;
typedef struct StorageProfileSlot
{
  char pad00[8];
  char name[0xB];
  unsigned char pin[6];
  unsigned char icon;
  unsigned char pad1A[2];
  int view_stats_early[3][2];
  int arcade_finishes;
  unsigned char pad38[0x40 - 0x38];
  int koins[6];
  unsigned char pad58[0x148 - 0x58];
  ProfileUnlockBits64 unlock_cat1;
  ProfileUnlockBits64 unlock_cat2;
  unsigned int unlock_cat3;
  unsigned int pad15C;
  ProfileUnlockBits64 unlock_cat4;
  unsigned int unlock_cat5;
  unsigned int unlock_cat6;
  ProfileUnlockBits64 unlock_cat7;
  ProfileUnlockBits64 unlock_cat8;
  ProfileUnlockBits64 unlock_cat9;
  unsigned int unlock_cat10;
  unsigned int pad18C;
  unsigned char pad190[0x4FC - 0x190];
  int view_stats_mid[3][2];
  int bg_team_valid;
  int bg_team[5];
  int view_stats_late[3][2];
  unsigned char pad544[0x5B4 - 0x544];
  int present;
  int idChecksum;
  unsigned char pad5BC[0x5C0 - 0x5BC];
} StorageProfileSlot;
typedef struct StorageDevice
{
  int status;
  unsigned int freeBlocks;
  int freeBytes;
  unsigned char profileCount;
  char name[0xB];
  int inUse[7];
  int pad34;
  GameSettings settings;
  unsigned char settings_padA4[4];
  StorageProfileSlot profiles[7];
  unsigned char pad28E8[8];
} StorageDevice;
int is_storage_device_full(int device);
extern StorageDevice storage_status[2];
typedef struct StorageProfileSlot StorageProfileSlot;
void check_new_mu_for_in_use_profiles(int device);
void summarize_unlocked_items(void);
int gc_no_space_routine(const char *nameOrNull, int device);
void mcard_msg_crc_failure(const char *nameOrNull, int device);
void mcard_msg_incompatible_card(const char *nameOrNull, int device);
void mcard_msg_wrong_device(const char *nameOrNull, int device);
void mcard_msg_card_damaged(const char *nameOrNull, int device);
void mcard_msg_another_market(const char *nameOrNull, int device);
void mcard_msg_sys_corrupt(const char *nameOrNull, int device);
void mcard_msg_mu_removed(const char *nameOrNull, int device);
void mcard_msg_delete_failed(int device);
void mcard_msg_delete_successful(int device);
void mcard_msg_deleting_file(int device);
extern int mcard_msg_wrong_device_answer;
extern int mcard_msg_mu_removed_answer;
extern int mcard_msg_incompatible_card_answer;
extern int msg_another_market_answer;
extern int msg_sys_corrupt_answer;
extern int msg_crc_failure_answer;
extern int mcard_msg_card_damaged_answer;
#pragma use_lmw_stmw on
const char *nbc_find_text(int index, int table);
char *strcpy(char *dest, const char *src);
void reset_storage_device_status_structure(int device);
void storage_status_change_calculations(int device);
void summarize_unlocked_items(void);
extern int f_writing_to_memcard;
extern int mcard_msg_incompatible_card_answer;
extern int msg_another_market_answer;
extern int mcard_msg_wrong_device_answer;
extern int msg_sys_corrupt_answer;
extern int mcard_msg_mu_removed_answer;
extern int msg_crc_failure_answer;
extern int mcard_msg_card_damaged_answer;
const int mcmasks[2] = {1, 2};
int check_load_profile_result(int *result, int device)
{
  StorageDevice *dev;
  int cont;
  int answer;
  const char *name;
  int status;
  dev = &storage_status[device];
  status = *result;
  dev->status = status;
  switch (status)
  {
    case 8:
      reset_storage_device_status_structure(device);
      strcpy(dev->name, "");
      (&storage_status[device])->freeBlocks = 0;
      name = nbc_find_text(0x70, 0);
      mcard_msg_incompatible_card(name, device);
      if (mcard_msg_incompatible_card_answer == 2)
    {
      cont = 0;
    }
    else
    {
      cont = 1;
    }
      break;

    case 9:
      reset_storage_device_status_structure(device);
      strcpy(dev->name, "");
      (&storage_status[device])->freeBlocks = 0;
      name = nbc_find_text(0x70, 0);
      mcard_msg_another_market(name, device);
      answer = msg_another_market_answer;
      switch (answer)
    {
      case 2:
        cont = 0;
        break;

      case 3:
        gc_format_procedure(device);
        cont = 0;
        break;

      default:
        cont = 1;
        break;

    }

      break;

    case 10:
      reset_storage_device_status_structure(device);
      strcpy(dev->name, "");
      (&storage_status[device])->freeBlocks = 0;
      name = nbc_find_text(0x70, 0);
      mcard_msg_wrong_device(name, device);
      if (mcard_msg_wrong_device_answer == 2)
    {
      cont = 0;
    }
    else
    {
      cont = 1;
    }
      break;

    case 11:
      reset_storage_device_status_structure(device);
      strcpy(dev->name, "");
      (&storage_status[device])->freeBlocks = 0;
      name = nbc_find_text(0x70, 0);
      mcard_msg_sys_corrupt(name, device);
      answer = msg_sys_corrupt_answer;
      switch (answer)
    {
      case 2:
        cont = 0;
        break;

      case 3:
        gc_format_procedure(device);
        cont = 0;
        break;

      default:
        cont = 1;
        break;

    }

      break;

    case 0:
      storage_status_change_calculations(device);
      cont = 1;
      break;

    case 1:
      reset_storage_device_status_structure(device);
      strcpy(dev->name, "");
      (&storage_status[device])->freeBlocks = 0;
      name = nbc_find_text(0x70, 0);
      mcard_msg_mu_removed(name, device);
      if (mcard_msg_mu_removed_answer == 1)
    {
      cont = 1;
    }
    else
    {
      cont = 0;
    }
      break;

    case 2:
      if (is_storage_device_full(device) != 0)
    {
      reset_storage_device_status_structure(device);
      strcpy(dev->name, "");
      *result = 5;
      dev->status = 5;
      name = nbc_find_text(0x70, 0);
      cont = gc_no_space_routine(name, device) == 0;
    }
    else
    {
      reset_storage_device_status_structure(device);
      cont = 1;
    }
      break;

    case 5:
      reset_storage_device_status_structure(device);
      strcpy(dev->name, "");
      name = nbc_find_text(0x70, 0);
      cont = gc_no_space_routine(name, device) == 0;
      break;

    case 6:
      reset_storage_device_status_structure(device);
      strcpy(dev->name, "");
      (&storage_status[device])->freeBlocks = 0;
      name = nbc_find_text(0x70, 0);
      mcard_msg_crc_failure(name, device);
      answer = msg_crc_failure_answer;
      switch (answer)
    {
      case 2:
        cont = 0;
        break;

      case 3:
        f_writing_to_memcard = 1;
        mcard_msg_deleting_file(device);
        if (gc_delete_file(device, "MKD") != 0)
      {
        mcard_msg_delete_successful(device);
        cont = 0;
        f_writing_to_memcard = 0;
      }
      else
      {
        mcard_msg_delete_failed(device);
        cont = 1;
        f_writing_to_memcard = 0;
      }
        break;

      default:
        cont = 1;
        break;

    }

      break;

    case 4:
      name = nbc_find_text(0x70, 0);
      mcard_msg_card_damaged(name, device);
      if (mcard_msg_card_damaged_answer == 1)
    {
      cont = 1;
    }
    else
    {
      cont = 0;
    }
      break;

    case 3:

    case 7:

    default:
      reset_storage_device_status_structure(device);
      strcpy(dev->name, "");
      cont = 1;
      (&storage_status[device])->freeBlocks = 0;
      break;

  }

  summarize_unlocked_items();
  (&storage_status[device])->status = *result;
  check_new_mu_for_in_use_profiles(device);
  return cont;
}

int gc_format_procedure(int device);
int gc_delete_file(int device, const char *fileName);
