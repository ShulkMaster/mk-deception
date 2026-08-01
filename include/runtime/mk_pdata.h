#ifndef MK_PDATA_H
#define MK_PDATA_H

#include "runtime/mk_proc.h"
#include "runtime/mk_struct.h"

void zero_pdata_payload(int size, MkHdr* dest);
MkProc* create_mkproc_fx(int proc_id, MkProcEntryFn proc_fn, MkHdr** pdata_out);
MkProc* _create_mkproc_generic_bigstack(int proc_id, int priority, MkProcEntryFn proc_fn,
                                        int pdata_size, MkHdr** pdata_out);
MkProc* _create_mkproc_generic_tinystack(int proc_id, int priority, MkProcEntryFn proc_fn,
                                         int pdata_size, MkHdr** pdata_out);
MkProc* _create_mkproc_generic_nostack(int proc_id, int priority, MkProcEntryFn proc_fn,
                                       int pdata_size, MkHdr** pdata_out);
int vdestroy_mkpdata_generic(MkHdr* pdata);
void destroy_mkpdata_generic(MkHdr* pdata);
MkHdr* get_mkpdata_generic(int size);

#endif
