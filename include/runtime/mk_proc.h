#ifndef MK_PROC_H
#define MK_PROC_H

#include "runtime/mk_struct.h"

typedef struct MkVtableMkproc MkVtableMkproc;
typedef struct MkProc MkProc;

typedef unsigned int MkProcStackWord;

typedef struct MkProcStackFrame {
    MkProcStackWord back_chain;
    MkProcStackWord saved_lr;
} MkProcStackFrame;

typedef float (*MkProcEntryFn)(void);
typedef void (*MkProcCallbackFn)(void);

struct MkProc {
    MkVtableMkproc* vtbl;
    int instance;
    int pid;
    float sleep_ticks;
    float saved_f14;
    float saved_f15;
    float saved_f16;
    float saved_f17;
    float saved_f18;
    float saved_f19;
    float saved_f20;
    float saved_f21;
    float saved_f22;
    float saved_f23;
    float saved_f24;
    float saved_f25;
    float saved_f26;
    float saved_f27;
    float saved_f28;
    float saved_f29;
    float saved_f30;
    float saved_f31;
    int saved_r14;
    int saved_r15;
    int saved_r16;
    int saved_r17;
    int saved_r18;
    int saved_r19;
    int saved_r20;
    int saved_r21;
    int saved_r22;
    int saved_r23;
    int saved_r24;
    int saved_r25;
    int saved_r26;
    int saved_r27;
    int saved_r28;
    int saved_r29;
    int saved_r30;
    int saved_r31;
    int saved_cr;
    int return_sp;
    int flags;
    int priority;
    MkProcCallbackFn pre_destroy;
    MkProcCallbackFn destroy_cb;
    MkProcEntryFn entry;
    int saved_lr;
    MkPtr* pdata_list;
    MkPtr* pdata_list_b;
    unsigned char* stack_top;
    unsigned char* stack_ptr;
};

#define MKPROC_FLAG_GAME_INFO_BIT  0x00000004
#define MKPROC_FLAG_SKIP_IF_PAUSED 0x00000008
#define MKPROC_FLAG_USE_GAME_SPEED 0x00000010
#define MKPROC_FLAG_NO_DESTROY     0x00000020
#define MKPROC_FLAG_DEFER_RUN      0x00000040
#define MKPROC_FLAG_ONE_SHOT       0x00000080

extern float zero_float;
extern int _paused;
extern MkProcStackWord* _local_sp_save;
extern MkProcStackWord* _slpx_sp;
extern MkProcStackWord* _slpx_pc;
extern int pid_to_kill_mask;
extern int pid_to_kill;
extern MkPtr* aproc_mkptr;
extern MkHdr* apdata;
extern MkProc* aproc_nodestroy;
extern MkProc* aproc;
extern float _mkproc_sleep_ticks;
extern MkPtr* active_proc_list;
extern int network_pause_procs;

void mkproc_die(void);
void mkproc_dispatch(void);
MkHdr* pdata_of_proc(MkProc* proc);
MkHdr* next_apdata(void);
MkProc* get_mkproc_bigstack(int* flags);
MkProc* get_mkproc_tinystack(int* flags);
MkProc* get_mkproc_nostack(int* flags);
void xfer_proc(MkProc* proc, MkProcEntryFn entry);
MkProc* find_mkproc_pid(int pid);
void destroy_mkprocs_pid_from_list(int pid, MkPtr** list);
void destroy_mkprocs_pid(int pid);
void destroy_all_mkprocs(void);
void init_mkproc(void);
MkProc* create_mkproc(int priority, MkProc* proc, int pid, MkProcEntryFn entry, MkHdr* pdata);
void mkproc_change_priority(MkProc* proc, int priority);
void insert_new_mkproc(MkProc* proc);
float p_idle(void);

#endif
