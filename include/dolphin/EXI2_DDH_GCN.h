#ifndef DOLPHIN_EXI2_DDH_GCN_H
#define DOLPHIN_EXI2_DDH_GCN_H

#include "dolphin/types.h"
#include "dolphin/exi.h"

int ddh_cc_initialize(volatile u8** input_pending, EXICallback monitor_callback);
int ddh_cc_shutdown(void);
int ddh_cc_open(void);
int ddh_cc_close(void);
int ddh_cc_read(u8* data, int size);
int ddh_cc_write(const u8* bytes, int length);
int ddh_cc_pre_continue(void);
int ddh_cc_post_stop(void);
int ddh_cc_peek(void);
int ddh_cc_initinterrupts(void);

#endif
