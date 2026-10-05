#ifndef PLATFORM_IO_LOG_H
#define PLATFORM_IO_LOG_H

struct SwitchLogEntry {
    int switch_index;
    int tick;
    const char* label;
    int joy_state;
    int mapped_index;
};

typedef char check_SwitchLogEntry_size[(sizeof(struct SwitchLogEntry) == 0x14) ? 1 : -1];

extern struct SwitchLogEntry p1_switch_log[30];
extern struct SwitchLogEntry p2_switch_log[30];
extern struct SwitchLogEntry p1_pad_switch_log[30];
extern struct SwitchLogEntry p2_pad_switch_log[30];
extern int p1_log_index;
extern int p2_log_index;
extern int p1_pad_log_index;
extern int p2_pad_log_index;
extern int p1_current_log_index;
extern int p2_current_log_index;

#endif
