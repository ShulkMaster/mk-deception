#include "platform/gcio.h"

#include "dolphin/dvd.h"
#include "dolphin/pad.h"
#include "dolphin/si.h"
#include "game/game_info.h"

extern void handle_reset_switch(void);
extern void dispatch_pad_sticks(int channel);
extern void dispatch_right_sticks(int channel);
extern void assign_player(int channel);
extern void unassign_player(PlyrInfo* player);
extern void controller_removed(int channel);
extern int are_controllers_locked(void);
extern int check_for_non_game_locked_controller_state(void);

extern SwitchMapEntry default_switch_map[16];

int gc_controller_offset_tbl[16] = {
    -1, 0x20, 0x40, 0x10, 0x800, 0x400, 0x100, 0x200,
    -1, -1, -1, 0x1000, 8, 2, 4, 1,
};

unsigned int pad_channels[5] = {
    0x80000000, 0x40000000, 0x20000000, 0x10000000, 0xffffffff,
};

int pad1_rumble_available;
int pad2_rumble_available;

int get_num_controllers(void) {
    int channel;
    int count = 0;

    for (channel = 0; channel < 3; channel++) {
        if (g_game_info.pads[channel].flag_bits.connected) {
            count++;
        }
    }
    return count;
}

/* Soft ceiling: retail keeps pad and flag-subobject bases in separate GPRs. */
void scan_switches(void) {
    PADStatus statuses[4];
    int channel;

    handle_reset_switch();
    PADRead(statuses);
    PADClampCircle(statuses);

    for (channel = 0; channel < 2; channel++) {
        PADStatus* status = &statuses[channel];

        if (status->err == 0) {
            GcPadSlot* pad = &g_game_info.pads[channel];
            GcPadFlags* flags = &pad->flag_bits;
            int switch_index;

            if (!flags->connected) {
                int probe_channel;

                flags->connected = 1;
                PADInit();
                for (probe_channel = 0; probe_channel < 4; probe_channel++) {
                    int retry_count = 5000;
                    unsigned int probe = SIProbe(probe_channel);

                    while ((probe == 0x80 || probe == 8) && retry_count != 0) {
                        probe = SIProbe(probe_channel);
                        retry_count--;
                    }
                    if (probe == 0x09000000) {
                        if (probe_channel == 0) {
                            pad1_rumble_available = 1;
                        }
                        if (probe_channel == 1) {
                            pad2_rumble_available = 1;
                        }
                    } else if (probe == 0x8b100000) {
                        if (probe_channel == 0) {
                            pad1_rumble_available = 0;
                        }
                        if (probe_channel == 1) {
                            pad2_rumble_available = 0;
                        }
                    }
                }
                SISetSamplingRate(1);
            }

            pad->prev_buttons = pad->buttons;
            pad->buttons = 0;
            for (switch_index = 0; switch_index < 16; switch_index++) {
                int mask = gc_controller_offset_tbl[switch_index];

                if (mask > 0) {
                    if (status->button & mask) {
                        pad->buttons |= default_switch_map[switch_index].mask;
                    } else {
                        pad->buttons &= ~default_switch_map[switch_index].mask;
                    }
                }
            }
            if (flags->stick_dispatch) {
                dispatch_pad_sticks(channel);
            }
            dispatch_right_sticks(channel);
            pad->edge = pad->buttons & (pad->buttons ^ pad->prev_buttons);
            if (pad->edge != 0 && pad->player == 0) {
                assign_player(channel);
            }
            pad->stick_pack = status->stickX + 0x7f;
            pad->stick_pack |= (unsigned int)(~status->stickY + 0x7f) << 8;
            pad->stick_pack |= (unsigned int)(status->substickX + 0x7f) << 16;
            pad->stick_pack |= (unsigned int)(~status->substickY + 0x7f) << 24;
        } else if (status->err == -1) {
            GcPadSlot* pad = &g_game_info.pads[channel];
            GcPadFlags* flags = &pad->flag_bits;

            if (flags->connected) {
                PADReset(pad_channels[channel]);
            }
            if (are_controllers_locked()) {
                if (pad->player != 0) {
                    int is_active = 0;

                    if (pad->player->player_state == 2) {
                        is_active = 1;
                    }
                    if (pad->player->player_state == 1) {
                        is_active = 1;
                    }
                    if (check_for_non_game_locked_controller_state() && is_active) {
                        controller_removed(channel);
                    } else if (is_active) {
                        controller_removed(channel);
                    }
                } else {
                    flags->connected = 0;
                    PADReset(pad_channels[channel]);
                    unassign_player(pad->player);
                }
            } else if (!flags->connected) {
                if (pad->player != 0) {
                    unassign_player(pad->player);
                }
                PADReset(pad_channels[channel]);
            } else {
                flags->connected = 0;
                PADReset(pad_channels[channel]);
            }
        } else if (status->err == -3) {
            GcPadSlot* pad = &g_game_info.pads[channel];

            pad->buttons = 0;
            pad->prev_buttons = 0;
            pad->edge = 0;
        }
    }
}

void turn_rumble_off(int channel) {
    PADControlMotor(channel, 0);
}

void turn_rumble_on(int channel, int strength) {
    PADControlMotor(channel, 1);
}

int is_rumble_available(int channel) {
    if (channel < 0) {
        return 0;
    }
    if (channel >= 3) {
        return 0;
    }
    if (g_game_info.pads[channel].player == 0) {
        return 0;
    }
    if (!DVDCheckDisk()) {
        return 0;
    }
    if (channel == 0) {
        return pad1_rumble_available;
    }
    /* Soft ceiling: retail booleanizes this final channel test with subic/subfe. */
    return pad2_rumble_available & (channel == 1);
}

int init_controller(void) {
    int channel;

    PADInit();
    for (channel = 0; channel < 4; channel++) {
        int retry_count = 5000;
        unsigned int probe = SIProbe(channel);

        while ((probe == 0x80 || probe == 8) && retry_count != 0) {
            probe = SIProbe(channel);
            retry_count--;
        }
        if (probe == 0x09000000) {
            if (channel == 0) {
                pad1_rumble_available = 1;
            }
            if (channel == 1) {
                pad2_rumble_available = 1;
            }
        } else if (probe == 0x8b100000) {
            if (channel == 0) {
                pad1_rumble_available = 0;
            }
            if (channel == 1) {
                pad2_rumble_available = 0;
            }
        }
    }
    SISetSamplingRate(1);
    return 1;
}
