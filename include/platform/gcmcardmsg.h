#ifndef PLATFORM_GCMCARDMSG_H
#define PLATFORM_GCMCARDMSG_H

/*
 * gcmcardmsg.o - Midway GC memcard message bodies (B20 + B21 boot popups).
 * Campaign history: docs/campaigns/index.md (B20-B22)
 * Nintendo card.a Matching is out of campaign.
 */

int is_this_a_hault_message(void);
void mcard_msg_end(void);
int gc_no_space_routine(const char* nameOrNull, int device);
void gc_boot_space_check(void);
void mcard_msg_middle_sleep(int mode, int caller);
void mcard_msg_card_changed_at_format(int device);
void mcard_msg_card_inaccessable_in_konq(void);
void mcard_msg_auto_save(int device);
void mcard_msg_save_failed(int device);
void mcard_msg_create_failed(int device);
void mcard_msg_create_successful(int device);
void mcard_msg_save_successful(int device);
void mcard_msg_confirm_erase(void);
void mcard_msg_load_no_card_konq_region_hault(const char* profileName, int unused, int device);
void mcard_msg_profile_damaged_in_konquest(void);
void mcard_msg_profile_reset_confirmation(void);
void mcard_msg_cant_enter_konquest(int device, const char* profileName);
void mcard_msg_save_no_card_konq_region_hault(const char* profileName, int unused);
void mcard_msg_quit_confirmation(void);
void mcard_msg_save_error_konq_region(int device);
void mcard_msg_name_conflict(void);
void mcard_msg_format_failed(int device);
void mcard_msg_format_successful(int device);
void mcard_msg_formating(int device);
void mcard_msg_format_confirmation(int device);
void mcard_msg_no_file(int device);
void mcard_msg_card_gone(const char* profileName, int device);
void mcard_msg_crc_failure(const char* nameOrNull, int device);
void mcard_msg_incompatible_card(const char* nameOrNull, int device);
void mcard_msg_wrong_device(const char* nameOrNull, int device);
void mcard_msg_card_damaged(const char* nameOrNull, int device);
void mcard_msg_another_market(const char* nameOrNull, int device);
void mcard_msg_sys_corrupt(const char* nameOrNull, int device);
void mcard_msg_no_cards_at_settings(void);
void mcard_msg_mu_removed(const char* nameOrNull, int device);
void mcard_msg_delete_failed_generic(int device);
void mcard_msg_delete_successful_generic(int device);
void mcard_msg_delete_failed(int device);
void mcard_msg_delete_successful(int device);
void mcard_msg_deleting_file(int device);
void mcard_msg_no_storage(const char* text);
void mcard_msg_read(int device);
void mcard_msg_deleting_data(int device);
void mcard_msg_create(int device);
void mcard_msg_save(int device);

/* NBC text ids for Slot A / Slot B (also used by gcmcard). */
extern int gc_mc_default_name[2];
extern int msg_format_confirmation_answer;
extern int msg_format_failed_answer;
extern int mcard_msg_card_changed_at_format_answer;
extern int msg_no_file_answer;

extern int msg_card_gone_answer;

extern int mcard_msg_wrong_device_answer;

extern int mcard_msg_mu_removed_answer;

extern int mcard_msg_no_cards_at_settings_answer;

extern int mcard_msg__no_cards_at_boot_answer;

extern int mcard_msg_incompatible_card_answer;
extern int msg_another_market_answer;
extern int msg_sys_corrupt_answer;
extern int msg_crc_failure_answer;
extern int mcard_msg_card_damaged_answer;
extern int mcard_msg_no_space_answer;
extern int mcard_msg_confirm_erase_answer;
extern int mcard_msg_name_conflict_answer;
extern int msg_save_cancelled_answer;
extern int msg_profile_reset_confirmation_answer;
extern int msg_load_no_card_konq_region_hault_answer;
extern int msg_save_no_card_konq_region_hault_answer;
extern int msg_save_error_konq_region_answer;
extern int msg_quit_confirmation_answer;

#endif
