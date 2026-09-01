#include "runtime/mk_pdata.h"

#include "runtime/cstring.h"
#include "runtime/mk_proc.h"
#include "runtime/mk_struct.h"

extern MkVtable5 vtbl_mkpdata_generic;

typedef struct MkPdataProcFlags {
    unsigned char has_pdata : 1;
    unsigned char pad : 7;
} MkPdataProcFlags;

void zero_pdata_payload(int size, MkHdr* dest) {
    MkVtable5* saved_vtbl;
    unsigned int saved_instance;

    saved_vtbl = dest->vtbl;
    saved_instance = dest->instance;
    memset(dest, 0, size);
    dest->vtbl = saved_vtbl;
    dest->instance = saved_instance;
}

MkProc* create_mkproc_fx(int proc_id, MkProcEntryFn proc_fn, MkHdr** pdata_out) {
    MkProc* mkproc;
    MkHdr* pdata;
    int flags_pair[2];
    MkPdataProcFlags* bits;

    flags_pair[0] = 0;
    if (pdata_out != 0) {
        bits = (MkPdataProcFlags*)&flags_pair[0];
        bits->has_pdata = 1;
    }
    flags_pair[1] = flags_pair[0];
    mkproc = get_mkproc_nostack(&flags_pair[1]);
    if (pdata_out != 0) {
        pdata = get_mkhdr(&vtbl_mkpdata_generic, 0xC);
        *pdata_out = pdata;
        mkproc = create_mkproc(0x20, mkproc, proc_id, proc_fn, *pdata_out);
    } else {
        mkproc = create_mkproc(0x20, mkproc, proc_id, proc_fn, 0);
    }
    return mkproc;
}

MkProc* _create_mkproc_generic_bigstack(int proc_id, int priority, MkProcEntryFn proc_fn,
                                        int pdata_size, MkHdr** pdata_out) {
    MkProc* mkproc;
    MkHdr* pdata;
    int flags;
    int flags_arg;
    MkPdataProcFlags* bits;

    flags = 0;
    if (pdata_out != 0) {
        bits = (MkPdataProcFlags*)&flags;
        bits->has_pdata = 1;
    }
    flags_arg = flags;
    mkproc = get_mkproc_bigstack(&flags_arg);
    if (pdata_out != 0) {
        pdata = get_mkhdr(&vtbl_mkpdata_generic, pdata_size);
        *pdata_out = pdata;
        mkproc = create_mkproc(priority, mkproc, proc_id, proc_fn, *pdata_out);
    } else {
        mkproc = create_mkproc(priority, mkproc, proc_id, proc_fn, 0);
    }
    return mkproc;
}

MkProc* _create_mkproc_generic_tinystack(int proc_id, int priority, MkProcEntryFn proc_fn,
                                         int pdata_size, MkHdr** pdata_out) {
    MkProc* mkproc;
    MkHdr* pdata;
    int flags;
    int flags_arg;
    MkPdataProcFlags* bits;

    flags = 0;
    if (pdata_out != 0) {
        bits = (MkPdataProcFlags*)&flags;
        bits->has_pdata = 1;
    }
    flags_arg = flags;
    mkproc = get_mkproc_tinystack(&flags_arg);
    if (pdata_out != 0) {
        pdata = get_mkhdr(&vtbl_mkpdata_generic, pdata_size);
        *pdata_out = pdata;
        mkproc = create_mkproc(priority, mkproc, proc_id, proc_fn, *pdata_out);
    } else {
        mkproc = create_mkproc(priority, mkproc, proc_id, proc_fn, 0);
    }
    return mkproc;
}

MkProc* _create_mkproc_generic_nostack(int proc_id, int priority, MkProcEntryFn proc_fn,
                                       int pdata_size, MkHdr** pdata_out) {
    MkProc* mkproc;
    MkHdr* pdata;
    int flags;
    int flags_arg;
    MkPdataProcFlags* bits;

    flags = 0;
    if (pdata_out != 0) {
        bits = (MkPdataProcFlags*)&flags;
        bits->has_pdata = 1;
    }
    flags_arg = flags;
    mkproc = get_mkproc_nostack(&flags_arg);
    if (pdata_out != 0) {
        pdata = get_mkhdr(&vtbl_mkpdata_generic, pdata_size);
        *pdata_out = pdata;
        mkproc = create_mkproc(priority, mkproc, proc_id, proc_fn, *pdata_out);
    } else {
        mkproc = create_mkproc(priority, mkproc, proc_id, proc_fn, 0);
    }
    return mkproc;
}

void vdestroy_mkpdata_generic(MkHdr* pdata) {
    pdata->instance = 0;
    mkhdr_memfree(pdata);
}

void destroy_mkpdata_generic(MkHdr* pdata) {
    pdata->instance = 0;
    mkhdr_memfree(pdata);
}

MkHdr* get_mkpdata_generic(int size) {
    return get_mkhdr(&vtbl_mkpdata_generic, size);
}
