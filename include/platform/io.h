#ifndef PLATFORM_IO_H
#define PLATFORM_IO_H

#include "platform/io_log.h"

void scan_remote_switches(void);
void show_sw_log(void);
void log_switch(int player, int switch_index, int tick,
                const char* label, int mapped_index);
void post_switchp(void);
void pre_switchp(void);
void turn_switch_log_off(void);
void turn_switch_log_on(void);
void init_switch_log(void);
void scan_switch_sequences(unsigned int* sequence);
int get_stick_pos(int port, int stick, float* horizontal, float* vertical);
void eat_switch_edge(int port, int switch_index);
int check_switch_edge_any_pad(int switch_index);
int check_switch_edge(int port, int switch_index);
int check_switch(int port, int switch_index);
void unstack_switches(void);
void reapply_controller_disabled_state(unsigned int state);
unsigned int get_controller_disabled_state(void);
void turn_controllers_on(void);
void turn_all_ports_on(void);
void disable_all_ports_but_me(unsigned int port);
void turn_port_off(int port);
void turn_controllers_off(void);
void flush_controller_switch_buffers(void);
void debug_error_message();
void debug_print_message();
void vdebug_print_message(const char* format, ...);

extern unsigned char stick_dead_zone;
extern int debug_message_handler_set;

#endif
