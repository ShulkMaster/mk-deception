#ifndef GAME_BGND_JMT_H
#define GAME_BGND_JMT_H

#include "runtime/mk_proc.h"

typedef struct RopeProcLatch {
    MkProc* proc;
    unsigned int instance;
} RopeProcLatch;

extern RopeProcLatch rope_proc_item;
extern RopeProcLatch sobj_ctrl_proc_item;

#endif
