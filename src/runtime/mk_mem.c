#include "mw/mwMem.h"
#include "mw/mwMemHeap.h"
#include "runtime/mk_mem.h"
#include "runtime/mk_struct.h"

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
            delay = (int)entry->instance - 1;
            if (delay <= 0) {
                _mwMemFree(entry->hdr, 0, 0);
                entry->hdr = 0;
                destroy_mkptr(entry);
            } else {
                entry->instance = (unsigned int)delay;
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
        delay = (int)entry->instance - 1;
        if (delay <= 0) {
            _mwMemFree(entry->hdr, 0, 0);
            entry->hdr = 0;
            destroy_mkptr(entry);
        } else {
            entry->instance = (unsigned int)delay;
        }
        entry = next;
    }
    pfxfont_release_delayed_vertex_buffers();
}

void free_mem_delayed(void *memory, int delay) {
    MkPtr *entry;

    entry = get_mkptr_not_owns_mkhdr((MkHdr *)memory);
    if (entry != 0) {
        entry->instance = (unsigned int)delay;
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
