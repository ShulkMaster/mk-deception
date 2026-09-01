#include "runtime/mk_proc.h"

#include "mw/mwMem.h"
#include "mw/mwMemHeap.h"
#include "game/game_info.h"
#include "platform/main.h"
#include "runtime/plyr_pdata.h"
#include "runtime/utils.h"

#include "runtime/asm_sequences.inc"

static void _destroy_proc_pid_mask(MkHdr* hdr);
static void dispatch_proc_list(MkPtr** list);

static inline int proc_list_available(MkPtr** list) {
    return list != 0;
}

void activate_cmdscript(void);
void deactivate_cmdscript(void);

static float zero_float = 0.0f;
int _paused = 0;
static MkProcStackWord* _local_sp_save = 0;
static MkProcStackWord* _slpx_sp = 0;
static MkProcStackWord* _slpx_pc = 0;
static int pid_to_kill_mask = 0;
static int pid_to_kill = 0;
static MkPtr* aproc_mkptr = 0;
MkHdr* apdata = 0;
MkProc* aproc_nodestroy = 0;
MkProc* aproc = 0;
float _mkproc_sleep_ticks = 0.0f;
MkPtr* active_proc_list = 0;
int network_pause_procs = 0;

void mkproc_die(void) {
    if (aproc->destroy_cb != 0) {
        aproc->destroy_cb();
        if (aproc->instance == 0) {
            return;
        }
    }
    aproc_nodestroy = 0;
    aproc->vtbl->destroy(aproc);
}

void dispatch_nostack(void) {
    MkProc* proc = aproc;
    float sleep_ticks;

    do {
        sleep_ticks = proc->entry();
        proc = aproc;
    } while (sleep_ticks == zero_float);
    proc->sleep_ticks = sleep_ticks;
    if (proc->destroy_cb != 0) {
        proc->destroy_cb();
        proc = aproc;
        if (proc->instance == 0) {
            return;
        }
    }
    if (proc->sleep_ticks < zero_float) {
        aproc_nodestroy = 0;
        proc->vtbl->destroy(proc);
    }
}

void sleep_nostack(void) {}

void system_stack_nostack(void) {}

void local_stack_nostack(void) {}

asm void jump_sleep_nostack(MkProcEntryFn entry, float ticks) {
    SEQ_jump_sleep_nostack();
}

/* Exceptional retail-derived platform context-switch boundaries. */
asm void dispatch_tinystack(void) {
    SEQ_dispatch_tinystack();
}

asm void sleep_tinystack(void) {
    SEQ_sleep_tinystack();
}

void system_stack_tinystack(void) {}

void local_stack_tinystack(void) {}

asm void jump_sleep_tinystack(MkProcEntryFn entry, float ticks) {
    SEQ_jump_sleep_tinystack();
}

asm void dispatch_bigstack(void) {
    SEQ_dispatch_bigstack();
}

asm void sleep_bigstack(void) {
    SEQ_sleep_bigstack();
}

asm void system_stack_bigstack(void) {
    SEQ_system_stack_bigstack();
}

asm void local_stack_bigstack(void) {
    SEQ_local_stack_bigstack();
}

asm void jump_sleep_bigstack(MkProcEntryFn entry, float ticks) {
    SEQ_jump_sleep_bigstack();
}

asm void mkproc_dispatch(void) {
    SEQ_mkproc_dispatch();
}

static void dispatch_proc_list(MkPtr** list) {
    /* The indirect dispatcher may swap process context; keep traversal state in memory. */
    MkPtr* volatile link;
    int* volatile paused = &_paused;

    if (list == 0) {
        return;
    }
    *paused = 0;
    if (g_game_info.pause_flag_bits.controller_disable_guard || network_pause_procs != 0) {
        *paused = 1;
    }
    link = *list;
    while (link != 0) {
        MkProc* current = MKPROC_FROM_HDR(link->hdr);
        if (link->instance != (unsigned int)current->instance) {
            MkPtr* next = link->next;
            link->hdr = 0;
            destroy_mkptr(link);
            link = next;
            continue;
        }
        aproc_nodestroy = 0;
        aproc = current;
        if (aproc != 0 && (*paused == 0 || aproc->flags_bits.skip_if_paused)) {
            if (aproc->flags_bits.use_game_speed) {
                aproc->sleep_ticks -= game_speed;
            } else {
                aproc->sleep_ticks -= 1.0f;
            }
            if (aproc->sleep_ticks <= 0.0f) {
                do {
                    aproc_mkptr = first_mkptr(&aproc->pdata_list);
                    if (aproc_mkptr == 0) {
                        if (aproc->flags_bits.one_shot) {
                            if (aproc->instance != 0) {
                                aproc->vtbl->destroy(aproc);
                            }
                            break;
                        }
                        apdata = 0;
                    } else {
                        apdata = aproc_mkptr->hdr;
                        if (apdata == 0) {
                            if (aproc->instance != 0) {
                                aproc->vtbl->destroy(aproc);
                            }
                            break;
                        }
                    }
                    aproc_nodestroy = 0;
                    if (aproc->pre_destroy == 0 ||
                        (aproc->pre_destroy(), aproc->instance != 0)) {
                        aproc_nodestroy = aproc;
                        if (aproc->flags_bits.game_info) {
                            activate_cmdscript();
                        }
                        aproc->vtbl->dispatch();
                        deactivate_cmdscript();
                    }
                } while (0);
            }
        }
        link = link->next;
    }
}

MkHdr* pdata_of_proc(MkProc* proc) {
    MkHdr* pdata = first_mkhdr(&proc->pdata_list);

    if (pdata != 0) {
        return pdata->typed_vtbl->fn1(pdata);
    }
    return 0;
}

MkHdr* next_apdata(void) {
    aproc_mkptr = next_mkptr(aproc_mkptr);
    if (aproc_mkptr != 0) {
        apdata = aproc_mkptr->hdr;
    } else {
        apdata = 0;
    }
    return apdata;
}

/* Soft ceiling: 99.67742% -- four operand-register coloring records only. */
MkProc* get_mkproc_bigstack(int* flags) {
    MkProc* proc = _mwMemMalloc(mkproc_heap, sizeof(MkProc), 0x80, 0, 0, 0);
    unsigned char* stack;

    if (proc != 0) {
        proc->vtbl = 0;
        mk_set_instance(&proc->instance);
        proc->pdata_list = 0;
        proc->pdata_list_b = 0;
        proc->sleep_ticks = 0.0f;
        proc->pre_destroy = 0;
        proc->destroy_cb = 0;
        proc->flags = 0;
    }
    if (proc != 0) {
        int proc_flags = *flags;
        MkVtableMkproc* vtbl = &vtbl_mkproc_bigstack;
        proc->vtbl = vtbl;
        proc->flags = proc_flags;
        stack = _mwMemMalloc(bigstack_heap, 0x4000, 0x80, 0, 0, 0);
        if (stack != 0) {
            stack += 0x3FE8;
        }
        proc->stack_top = stack;
        proc->stack_ptr = proc->stack_top;
        if (proc->stack_top == 0) {
            proc->instance = 0;
            _mwMemFree(proc, 0, 0);
            proc = 0;
        }
    }
    return proc;
}

/* Soft ceiling: 99.67742% -- four operand-register coloring records only. */
MkProc* get_mkproc_tinystack(int* flags) {
    MkProc* proc = _mwMemMalloc(mkproc_heap, sizeof(MkProc), 0x80, 0, 0, 0);
    unsigned char* stack;

    if (proc != 0) {
        proc->vtbl = 0;
        mk_set_instance(&proc->instance);
        proc->pdata_list = 0;
        proc->pdata_list_b = 0;
        proc->sleep_ticks = 0.0f;
        proc->pre_destroy = 0;
        proc->destroy_cb = 0;
        proc->flags = 0;
    }
    if (proc != 0) {
        int proc_flags = *flags;
        MkVtableMkproc* vtbl = &vtbl_mkproc_tinystack;
        proc->vtbl = vtbl;
        proc->flags = proc_flags;
        stack = _mwMemMalloc(tinystack_heap, 0x200, 0x80, 0, 0, 0);
        if (stack != 0) {
            stack += 0x200;
        }
        proc->stack_top = stack;
        proc->stack_ptr = proc->stack_top;
        if (proc->stack_top == 0) {
            proc->instance = 0;
            _mwMemFree(proc, 0, 0);
            proc = 0;
        }
    }
    return proc;
}

/* Soft ceiling: 99.30232% -- five operand-register coloring records only. */
MkProc* get_mkproc_nostack(int* flags) {
    MkProc* proc = _mwMemMalloc(mkproc_heap, sizeof(MkProc), 0x80, 0, 0, 0);

    if (proc != 0) {
        proc->vtbl = 0;
        mk_set_instance(&proc->instance);
        proc->pdata_list = 0;
        proc->pdata_list_b = 0;
        proc->sleep_ticks = 0.0f;
        proc->pre_destroy = 0;
        proc->destroy_cb = 0;
        proc->flags = 0;
    }
    if (proc != 0) {
        MkVtableMkproc* vtbl = &vtbl_mkproc_nostack;
        int proc_flags = *flags;
        proc->vtbl = vtbl;
        proc->flags = proc_flags;
        proc->stack_top = 0;
        proc->stack_ptr = proc->stack_top;
    }
    return proc;
}

void xfer_proc(MkProc* proc, MkProcEntryFn entry) {
    proc->stack_ptr = proc->stack_top;
    proc->entry = entry;
    proc->sleep_ticks = 0.0f;
}

MkProc* find_mkproc_pid(int pid) {
    MkPtr* link;

    if (proc_list_available(&active_proc_list)) {
        link = active_proc_list;
        while (link != 0) {
            MkProc* proc = MKPROC_FROM_HDR(link->hdr);
            if (link->instance != (unsigned int)proc->instance) {
                MkPtr* next = link->next;
                link->hdr = 0;
                destroy_mkptr(link);
                link = next;
            } else {
                if (proc->pid == pid) {
                    return proc;
                }
                link = link->next;
            }
        }
    }
    return 0;
}

void destroy_mkprocs_pid_from_list(int pid, MkPtr** list) {
    pid_to_kill = pid;
    pid_to_kill_mask = -1;
    apply_to_mklist(_destroy_proc_pid_mask, list);
}

void destroy_mkprocs_pid(int pid) {
    pid_to_kill = pid;
    pid_to_kill_mask = -1;
    apply_to_mklist(_destroy_proc_pid_mask, &active_proc_list);
}

static void _destroy_proc_pid_mask(MkHdr* hdr) {
    MkProc* proc;

    if (hdr != 0) {
        proc = MKPROC_FROM_HDR(hdr->typed_vtbl->fn0(hdr));
    } else {
        proc = 0;
    }
    if (proc != 0 && proc != aproc_nodestroy &&
        (unsigned int)pid_to_kill == ((unsigned int)proc->pid & pid_to_kill_mask) &&
        proc->instance != 0) {
        proc->vtbl->destroy(proc);
    }
}

void vdestroy_mkproc_bigstack(MkProc* proc) {
    if (proc != aproc_nodestroy && !proc->flags_bits.no_destroy) {
        proc->instance = 0;
        _mwMemFree(proc->stack_top - 0x3FE8, 0, 0);
        destroy_list(&proc->pdata_list);
        destroy_list(&proc->pdata_list_b);
        proc->instance = 0;
        _mwMemFree(proc, 0, 0);
    }
}

void vdestroy_mkproc_tinystack(MkProc* proc) {
    if (proc != aproc_nodestroy && !proc->flags_bits.no_destroy) {
        proc->instance = 0;
        _mwMemFree(proc->stack_top - 0x200, 0, 0);
        destroy_list(&proc->pdata_list);
        destroy_list(&proc->pdata_list_b);
        proc->instance = 0;
        _mwMemFree(proc, 0, 0);
    }
}

void vdestroy_mkproc_nostack(MkProc* proc) {
    if (proc != aproc_nodestroy && !proc->flags_bits.no_destroy) {
        proc->instance = 0;
        destroy_list(&proc->pdata_list);
        destroy_list(&proc->pdata_list_b);
        proc->instance = 0;
        _mwMemFree(proc, 0, 0);
    }
}

void destroy_mkproc_nostack(MkProc* proc) {
    if (proc != aproc_nodestroy && !proc->flags_bits.no_destroy) {
        proc->instance = 0;
        destroy_list(&proc->pdata_list);
        destroy_list(&proc->pdata_list_b);
        proc->instance = 0;
        _mwMemFree(proc, 0, 0);
    }
}

/* Soft ceiling: 98.57143% -- nine cursor/owner GPR-coloring records only. */
void destroy_all_mkprocs(void) {
    MkPtr* link;

    aproc_nodestroy = 0;
    if (proc_list_available(&active_proc_list)) {
        link = active_proc_list;
        while (link != 0) {
            MkProc* proc = MKPROC_FROM_HDR(link->hdr);
            if (link->instance != (unsigned int)proc->instance) {
                MkPtr* next = link->next;
                link->hdr = 0;
                destroy_mkptr(link);
                link = next;
            } else {
                proc->flags_bits.no_destroy = 0;
                if (proc->instance != 0) {
                    proc->vtbl->destroy(proc);
                }
                link = link->next;
            }
        }
    }
}

void init_mkproc(void) {
    init_mkpdata_plyrs();
    _local_sp_save = 0;
    active_proc_list = 0;
    pause_procs(0);
    network_pause_procs = 0;
}

MkProc* create_mkproc(int priority, MkProc* proc, int pid, MkProcEntryFn entry, MkHdr* pdata) {
    if (proc != 0) {
        if (proc->flags_bits.defer_run) {
            proc->flags_bits.one_shot = 1;
        }
        proc->priority = priority;
        proc->entry = entry;
        proc->pid = pid;
        if (pdata != 0) {
            mk_insert(pdata, &proc->pdata_list);
        } else if (proc->flags_bits.one_shot) {
            if (proc->instance != 0) {
                proc->vtbl->destroy(proc);
            }
            proc = 0;
        }
        if (proc != 0) {
            insert_new_mkproc(proc);
        }
    } else if (pdata != 0 && pdata->instance != 0) {
        pdata->typed_vtbl->destroy(pdata);
    }
    return proc;
}

/*
 * Soft ceiling: 98.181816% -- stable ordering and stale-link CFG agree;
 * residue is eight GPR operands plus one equivalent branch encoding.
 */
void mkproc_change_priority(MkProc* proc, int priority) {
    int new_priority;
    MkPtr* insert;
    MkPtr* previous;
    MkPtr* link;

    mk_pull_discard(&proc->hdr, &active_proc_list);
    proc->priority = priority;
    new_priority = proc->priority;
    insert = get_mkptr_owns_mkhdr(&proc->hdr);
    previous = 0;
    if (proc_list_available(&active_proc_list)) {
        link = active_proc_list;
        while (link != 0) {
            MkProc* current = MKPROC_FROM_HDR(link->hdr);
            if (link->instance != (unsigned int)current->instance) {
                MkPtr* next = link->next;
                link->hdr = 0;
                destroy_mkptr(link);
                link = next;
            } else {
                if (new_priority < current->priority) {
                    insert_mkptr_before(insert, link);
                    return;
                }
                previous = link;
                link = link->next;
            }
        }
    }
    if (previous != 0) {
        append_mkptr_after(insert, previous);
    } else {
        insert_mkptr(insert, &active_proc_list);
    }
}

/*
 * Soft ceiling: 97.95918% -- stable ordering and stale-link CFG agree;
 * residue is eight GPR operands plus one equivalent branch encoding.
 */
void insert_new_mkproc(MkProc* proc) {
    int priority = proc->priority;
    MkPtr* insert = get_mkptr_owns_mkhdr(&proc->hdr);
    MkPtr* previous = 0;
    MkPtr* link;

    if (proc_list_available(&active_proc_list)) {
        link = active_proc_list;
        while (link != 0) {
            MkProc* current = MKPROC_FROM_HDR(link->hdr);
            if (link->instance != (unsigned int)current->instance) {
                MkPtr* next = link->next;
                link->hdr = 0;
                destroy_mkptr(link);
                link = next;
            } else {
                if (priority < current->priority) {
                    insert_mkptr_before(insert, link);
                    return;
                }
                previous = link;
                link = link->next;
            }
        }
    }
    if (previous != 0) {
        append_mkptr_after(insert, previous);
    } else {
        insert_mkptr(insert, &active_proc_list);
    }
}

float p_idle(void) {
    return 1000.0f;
}
