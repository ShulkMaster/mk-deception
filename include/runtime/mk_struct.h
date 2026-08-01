#ifndef MK_STRUCT_H
#define MK_STRUCT_H

#include "runtime/mk_vtbl.h"

extern MkVtable5 vtbl_mkhdr_generic;

typedef struct MkHdr MkHdr;
typedef int (*MkHdrVtblFn)(MkHdr* hdr);

typedef struct MkHdrVtable {
    MkHdrVtblFn fn0;
    MkHdrVtblFn fn1;
    MkHdrVtblFn fn2;
    MkHdrVtblFn fn3;
    MkHdrVtblFn destroy;
} MkHdrVtable;

struct MkHdr {
    union {
        MkVtable5* vtbl;
        MkHdrVtable* typed_vtbl;
    };
    unsigned int instance;
};

/*
 * Tinystack / fog / character-anim pdata latch (size 0x10).
 * obj is live iff obj != 0 && obj->instance == obj_instance.
 */
typedef struct MkObjLatch {
    MkHdr hdr;                 /* +0x00 */
    MkHdr* obj;                /* +0x08 */
    unsigned int obj_instance; /* +0x0C */
} MkObjLatch; /* 0x10 */

typedef struct MkPtr {
    MkHdr* hdr;
    struct MkPtr* next;
    struct MkPtr* prev;
    struct MkPtr** list;
    unsigned int instance;
    /* +0x14: MWCC bitfield -> lbz/rlwimi/stb set, lbz/extrwi. test; pop clears as word. */
    union {
        unsigned int flags_word;
        struct {
            unsigned char no_own : 1;
            unsigned char flags_pad : 7;
            unsigned char flags_rest[3];
        } f;
    };
} MkPtr; /* 0x18 */

#define MKPTR_NO_OWN(p) ((p)->f.no_own != 0)

typedef void (*MkListApplyFn)(MkHdr* hdr);

extern MkPtr* free_mkptrs;
extern MkPtr* mkptr_list;
extern MkPtr* master_clean_up_list;
extern int global_instance_ctr;

MkHdr* get_mkhdr_generic(unsigned int size);
int vdestroy_mkhdr_generic(MkHdr* hdr);
MkHdr* get_mkhdr(MkVtable5* vtbl, unsigned int size);
void apply_to_mklist(MkListApplyFn fn, MkPtr** list);
MkPtr* find_in_mklist(MkHdr* hdr, MkPtr** list);
void insert_mkptr_before(MkPtr* insert, MkPtr* before);
void append_mkptr_after(MkPtr* insert, MkPtr* after);
void insert_mkptr(MkPtr* insert, MkPtr** list);
void discard_list(MkPtr** list);
void discard_mkptrs(MkPtr* head);
void destroy_mkptrs(MkPtr* head);
void destroy_mkptr(MkPtr* ptr);
void mk_pull_destroy(MkHdr* hdr, MkPtr** list);
void mk_pull_discard(MkHdr* hdr, MkPtr** list);
MkPtr* mk_pull(MkHdr* hdr, MkPtr** list);
MkPtr* mk_append_after_mkptr(MkHdr* hdr, MkPtr* after);
MkPtr* mk_append(MkHdr* hdr, MkPtr** list);
MkPtr* mk_insert_no_own(MkHdr* hdr, MkPtr** list);
MkPtr* mk_insert(MkHdr* hdr, MkPtr** list);
MkPtr* get_mkptr_not_owns_mkhdr(MkHdr* hdr);
MkPtr* get_mkptr_owns_mkhdr(MkHdr* hdr);
int get_mkptr_count(void);
void init_free_mkptrs(void);
void mk_system_reset(void);
void mk_system_init(void);
void purge_master_clean_up_list(void);
MkPtr* next_mkptr(MkPtr* ptr);
MkPtr* first_mkptr(MkPtr** list);
MkHdr* first_mkhdr(MkPtr** list);
void mk_set_instance(unsigned int* instance_out);
void destroy_list(MkPtr** list);
MkHdr* as_mkhdr(MkHdr* hdr);
void mkhdr_memfree(MkHdr* hdr);

#endif
