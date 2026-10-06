#ifndef MKD_PLATFORM_GCIO_H
#define MKD_PLATFORM_GCIO_H

#ifdef __cplusplus
extern "C"
#endif
int get_num_controllers(void);
void scan_switches(void);
void turn_rumble_off(int channel);
void turn_rumble_on(int channel, int strength);
int is_rumble_available(int channel);
int init_controller(void);

#endif
