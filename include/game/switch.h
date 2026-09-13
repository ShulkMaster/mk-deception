#ifndef GAME_SWITCH_H
#define GAME_SWITCH_H

#include "runtime/mk_struct.h"

typedef struct PlyrInfo PlyrInfo;

/* Generic switch process payload; player follows its owned MkHdr. */
typedef struct SwitchPdata {
    MkHdr hdr;
    PlyrInfo* player;
} SwitchPdata;

float pad_l2_proc(void);
float pad_r2_proc(void);
float pad_l1_proc(void);
float pad_r1_proc(void);
float pad_rup_proc(void);
float pad_rrt_proc(void);
float pad_rdn_proc(void);
float pad_rlt_proc(void);
float pad_select_proc(void);
float pad_lt_stick_btn_proc(void);
float pad_rt_stick_btn_proc(void);
float pad_start_proc(void);
float pad_lup_proc(void);
float pad_lrt_proc(void);
float pad_ldn_proc(void);
float pad_llt_proc(void);

int ck_eat_online_switches(void);
float angle_jump_scan_after_move(void);

#endif
