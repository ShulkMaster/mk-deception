#ifndef MKD_PLATFORM_GCIO_H
#define MKD_PLATFORM_GCIO_H

int get_num_controllers(void);
void scan_switches(void);
void turn_rumble_off(int channel);
void turn_rumble_on(int channel);
int is_rumble_available(int channel);
int init_controller(void);

#endif
