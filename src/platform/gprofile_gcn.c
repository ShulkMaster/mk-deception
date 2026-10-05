#include "platform/gprofile_gcn.h"
#include "dolphin/gx.h"
#include "rw/dlbrkpt.h"

static void s_GProfile_GCN_GxDrawDone_Handler(void* userData);

void GProfile_GCN_GxDrawDone(void) {
    volatile int done;

    done = 1;
    MWY_GCN_RW_InsertGxDrawDoneCallback(s_GProfile_GCN_GxDrawDone_Handler,
                                       (void*)&done);
    GXFlush();
    while (done != 0) {
    }
}

static void s_GProfile_GCN_GxDrawDone_Handler(void* userData) {
    volatile int* done;

    done = userData;
    *done = 0;
}
