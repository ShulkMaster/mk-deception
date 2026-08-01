#ifndef LIMB_H
#define LIMB_H

#include "runtime/mk_struct.h"

typedef struct LimbProcLatch {
    MkHdr* hdr;
    unsigned int instance;
} LimbProcLatch;

typedef struct LimbRuntime {
    char pad00[0x138];
    LimbProcLatch controller_proc; /* +0x138 */
    MkPtr* proc_list;              /* +0x140 */
    LimbProcLatch bone_procs[15];  /* +0x144 */
} LimbRuntime;

#endif
