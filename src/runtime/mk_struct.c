#include "runtime/mk_struct.h"

#include "mw/mwMem.h"
#include "mw/mwMemHeap.h"
#include "runtime/mk_mem.h"

typedef struct PlyrInfoInitView {
    unsigned char data[0x6C];
} PlyrInfoInitView;

typedef struct GameInfoInitView {
    unsigned int field_00;
    unsigned int field_04;
    unsigned char pad_008[0x9C];
    PlyrInfoInitView plyr0;
    PlyrInfoInitView plyr1;
    unsigned char pad_17C[0x8C];
    unsigned int field_208;
    unsigned int field_20C;
} GameInfoInitView;

typedef struct GameSettingsInitView {
    unsigned char pad_00[0x4C];
    unsigned int field_4C;
    unsigned int field_50;
} GameSettingsInitView;

typedef char PlyrInfoInitViewSize[(sizeof(PlyrInfoInitView) == 0x6C) ? 1 : -1];
typedef char GameInfoInitViewSize[(sizeof(GameInfoInitView) == 0x210) ? 1 : -1];
typedef char GameSettingsInitViewSize[(sizeof(GameSettingsInitView) == 0x54) ? 1 : -1];

/* Retail .rodata adds two alignment bytes and a four-byte linker gap. */
static const char stringBase0[0xA] = "get_mkhdr";

void setup_fixed_block_heaps(void);
void init_global_vars(void);
void reset_ani_data_space(void);
void init_mkproc(void);
void init_weapon_trails(void);
void start_obj_proc(void);
void init_2d_obj_lists(void);
void start_bone_hierarchy_proc(void);
void start_cloth_proc(void);
void start_morph_proc(void);
void set_background_color(int r, int g, int b, int a);
void init_font_system(void);
void set_mode_of_play(int mode);
void init_port_info_struct(void);
void init_cmdscript_system(void);
void atm_reset_current_page(int unused);
void reset_game_state(void);
void push_game_state(int state);
void init_player_switch_maps(void);
void init_plyr_info_struct(PlyrInfoInitView* info);
void init_bet_info_struct(void);

extern int force_bgnd_num;
extern GameInfoInitView g_game_info;
extern GameSettingsInitView game_settings;

MkPtr* mkptr_list = 0;
MkPtr* free_mkptrs = 0;
MkPtr* master_clean_up_list = 0;
static int net_override_instance = 0;
static int net_instance_value = 0;
int global_instance_ctr = 0;

static void discard_mkptr(MkPtr* ptr);

/* Pop a node from free_mkptrs. Clears flags as a word at +0x14 (retail). */
#define POP_FREE_MKPTR(ptr_)                                                       \
    do {                                                                           \
        (ptr_) = free_mkptrs;                                                      \
        if ((ptr_) != 0) {                                                         \
            free_mkptrs = (ptr_)->next;                                            \
            (ptr_)->flags_word = 0;                                                \
            (ptr_)->hdr = 0;                                                       \
            (ptr_)->next = 0;                                                      \
            (ptr_)->prev = 0;                                                      \
            (ptr_)->list = 0;                                                      \
            (ptr_)->instance = 0;                                                  \
        }                                                                          \
    } while (0)

#define BIND_MKPTR(ptr_, hdr_)                                                     \
    do {                                                                           \
        (ptr_)->instance = (hdr_)->instance;                                       \
        (ptr_)->hdr = (hdr_);                                                      \
    } while (0)

#define SET_NO_OWN(ptr_)                                                           \
    do {                                                                           \
        (ptr_)->f.no_own = 1;                                                      \
    } while (0)

/* Unlink: load list+next first; clear prev, then next, then list (retail order). */
#define UNLINK_MKPTR(ptr_)                                                         \
    do {                                                                           \
        MkPtr** list_ = (ptr_)->list;                                              \
        MkPtr* next_ = (ptr_)->next;                                               \
        if (list_ != 0) {                                                          \
            if (list_ != 0) {                                                      \
                MkPtr* prev_ = (ptr_)->prev;                                       \
                if (prev_ != 0) {                                                  \
                    prev_->next = next_;                                           \
                } else {                                                           \
                    *list_ = next_;                                                \
                }                                                                  \
                if (next_ != 0) {                                                  \
                    next_->prev = prev_;                                           \
                }                                                                  \
                (ptr_)->prev = 0;                                                  \
                (ptr_)->next = 0;                                                  \
                (ptr_)->list = 0;                                                  \
            }                                                                      \
        }                                                                          \
    } while (0)

/* Bitfield no_own test -> lbz + extrwi. */
#define MAYBE_DESTROY_OWNED(ptr_)                                                  \
    do {                                                                           \
        MkHdr* hdr_ = (ptr_)->hdr;                                                 \
        if (hdr_ != 0) {                                                           \
            if ((ptr_)->f.no_own == 0) {                                            \
                if ((ptr_)->instance == hdr_->instance) {                          \
                    if (hdr_->instance != 0) {                                     \
                        ((int (*)(MkHdr*))hdr_->vtbl->destroy)(hdr_);              \
                    }                                                              \
                }                                                                  \
            }                                                                      \
        }                                                                          \
    } while (0)

#define RECYCLE_MKPTR(ptr_)                                                        \
    do {                                                                           \
        MkPtr* zero_ = 0;                                                          \
        (ptr_)->instance = 0;                                                      \
        (ptr_)->hdr = 0;                                                           \
        (ptr_)->list = &free_mkptrs;                                               \
        (ptr_)->next = free_mkptrs;                                                \
        (ptr_)->prev = zero_;                                                      \
        if (free_mkptrs != 0) {                                                    \
            free_mkptrs->prev = (ptr_);                                            \
        }                                                                          \
        free_mkptrs = (ptr_);                                                      \
    } while (0)

#define INSERT_AT_HEAD(ptr_, list_)                                                \
    do {                                                                           \
        (ptr_)->list = (list_);                                                    \
        (ptr_)->next = *(list_);                                                   \
        (ptr_)->prev = 0;                                                          \
        if (*(list_) != 0) {                                                       \
            (*(list_))->prev = (ptr_);                                             \
        }                                                                          \
        *(list_) = (ptr_);                                                         \
    } while (0)

#define ASSIGN_INSTANCE(dest_)                                                     \
    do {                                                                           \
        if (net_override_instance != 0) {                                          \
            int ov_ = net_instance_value;                                          \
            net_override_instance = 0;                                             \
            (dest_) = ov_;                                                         \
        } else {                                                                   \
            global_instance_ctr--;                                                 \
            (dest_) = global_instance_ctr;                                         \
        }                                                                          \
    } while (0)

MkHdr* get_mkhdr_generic(unsigned int size) {
    MkHdr* hdr;

    if (size <= 0x40U) {
        hdr = _mwMemMalloc(fixed_block_16_heap, (unsigned long)size, 4, (void *)stringBase0, 0, 0);
    } else if (size <= 0x80U) {
        hdr = _mwMemMalloc(fixed_block_32_heap, (unsigned long)size, 4, (void *)stringBase0, 0, 0);
    } else if (size <= 0x100U) {
        hdr = _mwMemMalloc(fixed_block_64_heap, (unsigned long)size, 4, (void *)stringBase0, 0, 0);
    } else if (size <= 0x200U) {
        hdr = _mwMemMalloc(fixed_block_128_heap, (unsigned long)size, 4, (void *)stringBase0, 0, 0);
    } else if (size <= 0x800U) {
        hdr = _mwMemMalloc(fixed_block_512_heap, (unsigned long)size, 4, (void *)stringBase0, 0, 0);
    } else if (size <= 0x1000U) {
        hdr = _mwMemMalloc(fixed_block_1024_heap, (unsigned long)size, 4, (void *)stringBase0, 0, 0);
    } else {
        hdr = _mwMemMalloc(wave_heap, (unsigned long)size, 4, (void *)stringBase0, 0, 0);
    }
    if (hdr == 0) {
        return 0;
    }
    hdr->vtbl = &vtbl_mkhdr_generic;
    ASSIGN_INSTANCE(hdr->instance);
    return hdr;
}

int vdestroy_mkhdr_generic(MkHdr* hdr) {
    hdr->instance = 0;
    _mwMemFree(hdr, 0, 0);
}

void apply_to_mklist(MkListApplyFn fn, MkPtr** list) {
    MkListApplyFn apply;
    MkPtr* next;
    MkPtr* cur;
    MkHdr* hdr;

    apply = fn;
    if (list == 0) {
        return;
    }
    cur = *list;
    while (cur != 0) {
        hdr = cur->hdr;
        if (cur->instance != hdr->instance) {
            next = cur->next;
            discard_mkptr(cur);
            cur = next;
        } else {
            apply(hdr);
            cur = cur->next;
        }
    }
}

MkPtr* find_in_mklist(MkHdr* hdr, MkPtr** list) {
    MkHdr* target;
    MkHdr* cur_hdr;
    MkPtr* cur;
    MkPtr* next;

    target = hdr;
    cur = *list;
    while (cur != 0) {
        cur_hdr = cur->hdr;
        if (cur->instance != cur_hdr->instance) {
            next = cur->next;
            discard_mkptr(cur);
            cur = next;
        } else if (target == cur_hdr) {
            return cur;
        } else {
            cur = cur->next;
        }
    }
    return 0;
}

void insert_mkptr_before(MkPtr* insert, MkPtr* before) {
    MkPtr* prev;

    insert->list = before->list;
    insert->next = before;
    prev = before->prev;
    before->prev = insert;
    insert->prev = prev;
    if (prev != 0) {
        prev->next = insert;
        return;
    }
    if (insert->list != 0) {
        *insert->list = insert;
    }
}

void append_mkptr_after(MkPtr* insert, MkPtr* after) {
    MkPtr* next;

    insert->list = after->list;
    next = after->next;
    after->next = insert;
    insert->next = next;
    if (next != 0) {
        next->prev = insert;
    }
    insert->prev = after;
}

void insert_mkptr(MkPtr* insert, MkPtr** list) {
    insert->list = list;
    insert->next = *list;
    insert->prev = 0;
    if (*list != 0) {
        (*list)->prev = insert;
    }
    *list = insert;
}

void discard_list(MkPtr** list) {
    discard_mkptrs(*list);
    *list = 0;
}

void discard_mkptrs(MkPtr* head) {
    MkPtr* next;

    while (head != 0) {
        next = head->next;
        discard_mkptr(head);
        head = next;
    }
}

void destroy_mkptrs(MkPtr* head) {
    /* Soft ceiling: ~97.98% -- clean typed next/cursor assignments coalesce;
     * retail retains an extra load-result move between GPRs. */
    /* tmp cast + cursor copy => lwz r5 / mr r31,r5; prev before list => r3/r4. */
    MkPtr* next;
    MkPtr* cursor;
    MkPtr* prev;
    MkPtr** list;
    MkHdr* hdr;

    while (head != 0) {
        list = head->list;
        next = head->next;
        cursor = next;
        if (list != 0) {
            if (list != 0) {
                prev = head->prev;
                if (prev != 0) {
                    prev->next = next;
                } else {
                    *list = next;
                }
                if (next != 0) {
                    next->prev = prev;
                }
                head->prev = 0;
                head->next = 0;
                head->list = 0;
            }
        }
        hdr = head->hdr;
        if (hdr != 0) {
            if (head->f.no_own == 0) {
                if (head->instance == hdr->instance) {
                    if (hdr->instance != 0) {
                        hdr->typed_vtbl->destroy(hdr);
                    }
                }
            }
        }
        {
            MkPtr* zero = 0;
            head->instance = 0;
            head->hdr = 0;
            head->list = &free_mkptrs;
            head->next = free_mkptrs;
            head->prev = zero;
            if (free_mkptrs != 0) {
                free_mkptrs->prev = head;
            }
            free_mkptrs = head;
        }
        head = cursor;
    }
}

void destroy_mkptr(MkPtr* ptr) {
    MkPtr* next;
    MkPtr* prev;
    MkPtr** list;
    MkHdr* hdr;

    list = ptr->list;
    if (list != 0) {
        if (list != 0) {
            prev = ptr->prev;
            next = ptr->next;
            if (prev != 0) {
                prev->next = next;
            } else {
                *list = next;
            }
            if (next != 0) {
                next->prev = prev;
            }
            ptr->prev = 0;
            ptr->next = 0;
            ptr->list = 0;
        }
    }
    hdr = ptr->hdr;
    if (hdr != 0) {
        if (ptr->f.no_own == 0) {
            if (ptr->instance == hdr->instance) {
                if (hdr->instance != 0) {
                    hdr->typed_vtbl->destroy(hdr);
                }
            }
        }
    }
    {
        MkPtr* zero = 0;
        ptr->instance = 0;
        ptr->hdr = 0;
        ptr->list = &free_mkptrs;
        ptr->next = free_mkptrs;
        ptr->prev = zero;
        if (free_mkptrs != 0) {
            free_mkptrs->prev = ptr;
        }
        free_mkptrs = ptr;
    }
}

void mk_pull_destroy(MkHdr* hdr, MkPtr** list) {
    MkPtr* ptr;
    MkPtr* next;
    MkPtr* prev;
    MkPtr** ptr_list;
    MkHdr* owned;
    MkPtr* zero;

    ptr = mk_pull(hdr, list);
    if (ptr != 0) {
        ptr_list = ptr->list;
        if (ptr_list != 0) {
            if (ptr_list != 0) {
                prev = ptr->prev;
                next = ptr->next;
                if (prev != 0) {
                    prev->next = next;
                } else {
                    *ptr_list = next;
                }
                if (next != 0) {
                    next->prev = prev;
                }
                ptr->prev = 0;
                ptr->next = 0;
                ptr->list = 0;
            }
        }
        owned = ptr->hdr;
        if (owned != 0) {
            if (ptr->f.no_own == 0) {
                if (ptr->instance == owned->instance) {
                    if (owned->instance != 0) {
                        owned->typed_vtbl->destroy(owned);
                    }
                }
            }
        }
        zero = 0;
        ptr->instance = 0;
        ptr->hdr = 0;
        ptr->list = &free_mkptrs;
        ptr->next = free_mkptrs;
        ptr->prev = zero;
        if (free_mkptrs != 0) {
            free_mkptrs->prev = ptr;
        }
        free_mkptrs = ptr;
    }
}

void mk_pull_discard(MkHdr* hdr, MkPtr** list) {
    MkPtr* ptr;

    ptr = mk_pull(hdr, list);
    if (ptr != 0) {
        discard_mkptr(ptr);
    }
}

MkPtr* mk_pull(MkHdr* hdr, MkPtr** list) {
    MkPtr* ptr;
    MkPtr* next;
    MkPtr* prev;
    MkPtr** ptr_list;

    ptr = find_in_mklist(hdr, list);
    if (ptr != 0) {
        ptr_list = ptr->list;
        if (ptr_list != 0) {
            prev = ptr->prev;
            next = ptr->next;
            if (prev != 0) {
                prev->next = next;
            } else {
                *ptr_list = next;
            }
            if (next != 0) {
                next->prev = prev;
            }
            ptr->prev = 0;
            ptr->next = 0;
            ptr->list = 0;
        }
    }
    return ptr;
}

MkPtr* mk_append_after_mkptr(MkHdr* hdr, MkPtr* after) {
    MkPtr* ptr;
    MkPtr* next;

    POP_FREE_MKPTR(ptr);
    if (ptr != 0) {
        BIND_MKPTR(ptr, hdr);
    }
    if (ptr != 0) {
        ptr->list = after->list;
        next = after->next;
        after->next = ptr;
        ptr->next = next;
        if (next != 0) {
            next->prev = ptr;
        }
        ptr->prev = after;
    }
    return ptr;
}

MkPtr* mk_append(MkHdr* hdr, MkPtr** list) {
    MkPtr* ptr;
    MkPtr* cur;
    MkPtr* next;

    POP_FREE_MKPTR(ptr);
    if (ptr != 0) {
        BIND_MKPTR(ptr, hdr);
    }
    if (ptr != 0) {
        cur = *list;
        if (cur == 0) {
            INSERT_AT_HEAD(ptr, list);
        } else {
            while (cur != 0) {
                if (cur->next == 0) {
                    ptr->list = cur->list;
                    next = cur->next;
                    cur->next = ptr;
                    ptr->next = next;
                    if (next != 0) {
                        next->prev = ptr;
                    }
                    ptr->prev = cur;
                    break;
                }
                cur = cur->next;
            }
        }
    }
    return ptr;
}

MkPtr* mk_insert_no_own(MkHdr* hdr, MkPtr** list) {
    MkPtr* ptr;

    POP_FREE_MKPTR(ptr);
    if (ptr != 0) {
        BIND_MKPTR(ptr, hdr);
        SET_NO_OWN(ptr);
    }
    if (ptr != 0) {
        INSERT_AT_HEAD(ptr, list);
    }
    return ptr;
}

MkPtr* mk_insert(MkHdr* hdr, MkPtr** list) {
    MkPtr* ptr;

    POP_FREE_MKPTR(ptr);
    if (ptr != 0) {
        BIND_MKPTR(ptr, hdr);
    }
    if (ptr != 0) {
        INSERT_AT_HEAD(ptr, list);
    }
    return ptr;
}

MkPtr* get_mkptr_not_owns_mkhdr(MkHdr* hdr) {
    MkPtr* ptr;

    POP_FREE_MKPTR(ptr);
    if (ptr != 0) {
        BIND_MKPTR(ptr, hdr);
        SET_NO_OWN(ptr);
    }
    return ptr;
}

MkPtr* get_mkptr_owns_mkhdr(MkHdr* hdr) {
    MkPtr* ptr;

    POP_FREE_MKPTR(ptr);
    if (ptr != 0) {
        BIND_MKPTR(ptr, hdr);
    }
    return ptr;
}



void init_free_mkptrs(void) {
    int count;
    int i;
    MkPtr* ptr;
    MkPtr* previous_head;

    /*
     * Soft ceiling: init_free_mkptrs ~99.18% -- the remaining second-loop
     * difference is only the computed node and previous-head GPRs swapped.
     * Direct typed indexed clears reproduce retail's repeated global-base
     * loads and indexed stores; do not replace them with a cached element.
     */
    count = get_mkptr_count();
    for (i = 0; i < count; i++) {
        mkptr_list[i].hdr = 0;
        mkptr_list[i].next = 0;
        mkptr_list[i].prev = 0;
        mkptr_list[i].list = 0;
        mkptr_list[i].instance = 0;
    }
    free_mkptrs = 0;
    for (i = 0; i < count; i++) {
        ptr = &mkptr_list[i];
        ptr->list = &free_mkptrs;
        ptr->next = free_mkptrs;
        ptr->prev = 0;
        previous_head = free_mkptrs;
        if (previous_head != 0) {
            previous_head->prev = ptr;
        }
        free_mkptrs = ptr;
    }
}

void mk_system_reset(void) {
    mwMemHeapWipe(wave_heap);
    setup_fixed_block_heaps();
    init_global_vars();
    reset_ani_data_space();
    reset_wave_mem();
    init_mkproc();
    init_weapon_trails();
    start_obj_proc();
    init_2d_obj_lists();
    start_bone_hierarchy_proc();
    start_cloth_proc();
    start_morph_proc();
    set_background_color(0, 0, 0, 0xFF);
    init_font_system();
}

void mk_system_init(void) {
    force_bgnd_num = -1;
    set_mode_of_play(0xD);
    g_game_info.field_208 = 0;
    g_game_info.field_20C = 0;
    init_port_info_struct();
    game_settings.field_4C = 0;
    game_settings.field_50 = 0;
    init_cmdscript_system();
    g_game_info.field_04 = 0;
    atm_reset_current_page(0);
    reset_game_state();
    push_game_state(0);
    mwMemHeapWipe(wave_heap);
    setup_fixed_block_heaps();
    init_global_vars();
    reset_ani_data_space();
    reset_wave_mem();
    init_mkproc();
    init_weapon_trails();
    start_obj_proc();
    init_2d_obj_lists();
    start_bone_hierarchy_proc();
    start_cloth_proc();
    start_morph_proc();
    set_background_color(0, 0, 0, 0xFF);
    init_font_system();
    init_player_switch_maps();
    init_plyr_info_struct(&g_game_info.plyr0);
    init_plyr_info_struct(&g_game_info.plyr1);
    init_bet_info_struct();
}

void purge_master_clean_up_list(void) {
    MkPtr* ptr;

    ptr = first_mkptr(&master_clean_up_list);
    while (ptr != 0) {
        ptr = next_mkptr(ptr);
    }
}

MkPtr* next_mkptr(MkPtr* ptr) {
    MkPtr* cur;
    MkHdr* hdr;

    while (1) {
        cur = ptr->next;
        if (cur == 0) {
            break;
        }
        hdr = cur->hdr;
        if (hdr != 0) {
            if (cur->instance == hdr->instance) {
                return cur;
            }
        }
        ptr = cur->next;
        discard_mkptr(cur);
        if (ptr == 0) {
            break;
        }
    }
    return 0;
}

MkPtr* first_mkptr(MkPtr** list) {
    MkPtr* cur;
    MkHdr* hdr;

    while (1) {
        cur = *list;
        if (cur == 0) {
            break;
        }
        hdr = cur->hdr;
        if (hdr != 0) {
            if (cur->instance == hdr->instance) {
                return cur;
            }
        }
        discard_mkptr(cur);
    }
    return 0;
}

MkHdr* first_mkhdr(MkPtr** list) {
    MkPtr* cur;
    MkHdr* hdr;

    while (1) {
        cur = *list;
        if (cur == 0) {
            break;
        }
        hdr = cur->hdr;
        if (hdr != 0) {
            if (cur->instance == hdr->instance) {
                return hdr;
            }
        }
        discard_mkptr(cur);
    }
    return 0;
}

void mk_set_instance(unsigned int* instance_out) {
    if (net_override_instance != 0) {
        unsigned int value = (unsigned int)net_instance_value;
        net_override_instance = 0;
        *instance_out = value;
        return;
    }
    global_instance_ctr--;
    *instance_out = (unsigned int)global_instance_ctr;
}

void destroy_list(MkPtr** list) {
    destroy_mkptrs(*list);
    *list = 0;
}

MkHdr* as_mkhdr(MkHdr* hdr) {
    return hdr;
}

void mkhdr_memfree(MkHdr* hdr) {
    _mwMemFree(hdr, 0, 0);
}

MkHdr* get_mkhdr(MkVtable5* vtbl, unsigned int size) {
    MkHdr* hdr;

    if (size <= 0x40U) {
        hdr = _mwMemMalloc(fixed_block_16_heap, (unsigned long)size, 4, (void *)stringBase0, 0, 0);
    } else if (size <= 0x80U) {
        hdr = _mwMemMalloc(fixed_block_32_heap, (unsigned long)size, 4, (void *)stringBase0, 0, 0);
    } else if (size <= 0x100U) {
        hdr = _mwMemMalloc(fixed_block_64_heap, (unsigned long)size, 4, (void *)stringBase0, 0, 0);
    } else if (size <= 0x200U) {
        hdr = _mwMemMalloc(fixed_block_128_heap, (unsigned long)size, 4, (void *)stringBase0, 0, 0);
    } else if (size <= 0x800U) {
        hdr = _mwMemMalloc(fixed_block_512_heap, (unsigned long)size, 4, (void *)stringBase0, 0, 0);
    } else if (size <= 0x1000U) {
        hdr = _mwMemMalloc(fixed_block_1024_heap, (unsigned long)size, 4, (void *)stringBase0, 0, 0);
    } else {
        hdr = _mwMemMalloc(wave_heap, (unsigned long)size, 4, (void *)stringBase0, 0, 0);
    }
    if (hdr == 0) {
        return 0;
    }
    hdr->vtbl = vtbl;
    ASSIGN_INSTANCE(hdr->instance);
    return hdr;
}

static void discard_mkptr(MkPtr* ptr) {
    MkPtr* next;
    MkPtr* prev;
    MkPtr** list;
    MkHdr* hdr;
    MkPtr* zero;

    ptr->hdr = 0;
    list = ptr->list;
    if (list != 0) {
        if (list != 0) {
            prev = ptr->prev;
            next = ptr->next;
            if (prev != 0) {
                prev->next = next;
            } else {
                *list = next;
            }
            if (next != 0) {
                next->prev = prev;
            }
            ptr->prev = 0;
            ptr->next = 0;
            ptr->list = 0;
        }
    }
    hdr = ptr->hdr;
    if (hdr != 0) {
        if (ptr->f.no_own == 0) {
            if (ptr->instance == hdr->instance) {
                if (hdr->instance != 0) {
                    hdr->typed_vtbl->destroy(hdr);
                }
            }
        }
    }
    zero = 0;
    ptr->instance = 0;
    ptr->hdr = 0;
    ptr->list = &free_mkptrs;
    ptr->next = free_mkptrs;
    ptr->prev = zero;
    if (free_mkptrs != 0) {
        free_mkptrs->prev = ptr;
    }
    free_mkptrs = ptr;
}
