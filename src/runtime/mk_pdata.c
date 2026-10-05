#include "runtime/mk_pdata.h"

#include "runtime/cstring.h"
#include "runtime/mk_proc.h"
#include "runtime/mk_struct.h"

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

    mkproc = get_mkproc_nostack(mkproc_init_flags_for_pdata(pdata_out));
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
    MkProcInitFlags flags;

    flags.value = 0;
    if (pdata_out != 0) {
        flags.bits.has_pdata = 1;
    }

    mkproc = get_mkproc_bigstack(flags);
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
    MkProcInitFlags flags;

    flags.value = 0;
    if (pdata_out != 0) {
        flags.bits.has_pdata = 1;
    }

    mkproc = get_mkproc_tinystack(flags);
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
    MkProcInitFlags flags;

    flags.value = 0;
    if (pdata_out != 0) {
        flags.bits.has_pdata = 1;
    }

    mkproc = get_mkproc_nostack(flags);
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
