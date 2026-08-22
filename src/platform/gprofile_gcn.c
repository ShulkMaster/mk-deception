#include "platform/gprofile_gcn.h"
#include "dolphin/gx.h"

typedef void (*MwyGxDrawDoneCallback)(void* userData);

void MWY_GCN_RW_InsertGxDrawDoneCallback(MwyGxDrawDoneCallback callback,
                                        void* userData);
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
    *(volatile int*)userData = 0;
}
