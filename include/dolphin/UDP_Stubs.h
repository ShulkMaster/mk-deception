#ifndef DOLPHIN_UDP_STUBS_H
#define DOLPHIN_UDP_STUBS_H

#include "dolphin/types.h"
#include "dolphin/exi.h"

int udp_cc_initialize(volatile u8** flag_out, EXICallback handler);
int udp_cc_shutdown(void);
int udp_cc_open(void);
int udp_cc_close(void);
int udp_cc_read(u8* destination, int size);
int udp_cc_write(const u8* source, int size);
int udp_cc_peek(void);
int udp_cc_pre_continue(void);
int udp_cc_post_stop(void);

#endif
