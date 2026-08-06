#ifndef MCARDMSG_H
#define MCARDMSG_H

int get_p2_pad(void);
int get_p1_pad(void);
void set_memcard_popup_message_type(int type);
void set_memcard_popup_message_options_text(const char* text);
void set_memcard_popup_message_body_text(const char* text);
void set_memcard_popup_message_title_text(const char* text);
void fire_up_memcard_mesage_screen(void);
void mcmsg_nothing(void);
void mcard_msg_remove_screen(void);
void init_memcard_msg_screen(void);
void mcard_msg_handler(void);
int is_mcardmsg_active(void);
int ck_mcard_msg(void);
void recover_from_message(void);
void prepare_for_haulting_message(void);
void prepare_for_sleeping_message(void);

#endif
