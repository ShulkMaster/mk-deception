#include "mw/mwMem.h"
#include "mw/mwMemHeap.h"
#include "runtime/mk_mem.h"

typedef struct MkPtr MkPtr;

struct MkPtr {
    void *memory;       /* +0x00 */
    MkPtr *next;        /* +0x04 */
    MkPtr *previous;    /* +0x08 */
    int index;          /* +0x0C */
    int delay;          /* +0x10 */
    int field_14;       /* +0x14: owned by mk_struct; purpose unknown */
};

extern MkPtr *mkptr_list;

extern MkPtr *get_mkptr_not_owns_mkhdr(void *memory);
extern void destroy_mkptr(MkPtr *entry);
extern void insert_mkptr(MkPtr *entry, MkPtr **list);
extern int get_mkptr_count(void);
extern void init_free_mkptrs(void);
extern void pfxfont_release_delayed_vertex_buffers(void);

MkPtr *delayed_free_list;

/* Soft ceiling: purge_delayed_mem_frees ~99.69% -- decrement-result GPR coloring. */
void purge_delayed_mem_frees(void) {
    MkPtr *next;
    MkPtr *entry;
    int delay;

    while ((entry = delayed_free_list) != 0) {
        while (entry != 0) {
            next = entry->next;
            delay = entry->delay - 1;
            if (delay <= 0) {
                _mwMemFree(entry->memory, 0, 0);
                entry->memory = 0;
                destroy_mkptr(entry);
            } else {
                entry->delay = delay;
            }
            entry = next;
        }
        pfxfont_release_delayed_vertex_buffers();
    }
}

/* Soft ceiling: do_delayed_mem_frees ~99.66% -- decrement-result GPR coloring. */
void do_delayed_mem_frees(void) {
    MkPtr *entry;
    MkPtr *next;
    int delay;

    entry = delayed_free_list;
    while (entry != 0) {
        next = entry->next;
        delay = entry->delay - 1;
        if (delay <= 0) {
            _mwMemFree(entry->memory, 0, 0);
            entry->memory = 0;
            destroy_mkptr(entry);
        } else {
            entry->delay = delay;
        }
        entry = next;
    }
    pfxfont_release_delayed_vertex_buffers();
}

void free_mem_delayed(void *memory, int delay) {
    MkPtr *entry;

    entry = get_mkptr_not_owns_mkhdr(memory);
    if (entry != 0) {
        entry->delay = delay;
        insert_mkptr(entry, &delayed_free_list);
    } else {
        _mwMemFree(memory, 0, 0);
    }
}

void *get_mem(unsigned int size) {
    return _mwMemMalloc(wave_heap, size, 4, 0, 0, 0);
}

void free_mem(void *memory) {
    _mwMemFree(memory, 0, 0);
}

void reset_wave_mem(void) {
    mwMemHeapWipe(wave_heap);
    delayed_free_list = 0;
    mkptr_list = _mwMemMalloc(wave_heap, get_mkptr_count() * sizeof(MkPtr), 4,
                              0, 0, 0);
    init_free_mkptrs();
}
