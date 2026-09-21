#include "dolphin/arq.h"
#include "dolphin/os.h"

const char* __ARQVersion =
    "<< Dolphin SDK - ARQ\trelease build: Apr  5 2004 04:15:04 (0x2301) >>";
/* Retail SDK object has a four-byte terminal .sdata layout slot here. */
unsigned char gap_07_8050FECC_sdata[4] = {0, 0, 0, 0};

static ARQRequest* __ARQRequestQueueHi;
static ARQRequest* __ARQRequestTailHi;
static ARQRequest* __ARQRequestQueueLo;
static ARQRequest* __ARQRequestTailLo;
static ARQRequest* __ARQRequestPendingHi;
static ARQRequest* __ARQRequestPendingLo;
static ARQCallback __ARQCallbackHi;
static ARQCallback __ARQCallbackLo;
static unsigned long __ARQChunkSize;
static int __ARQ_init_flag;

void __ARQPopTaskQueueHi(void)
{
    if (__ARQRequestQueueHi != 0) {
        if (__ARQRequestQueueHi->type == 0) {
            ARStartDMA(__ARQRequestQueueHi->type,
                       __ARQRequestQueueHi->source,
                       __ARQRequestQueueHi->destination,
                       __ARQRequestQueueHi->length);
        } else {
            ARStartDMA(__ARQRequestQueueHi->type,
                       __ARQRequestQueueHi->destination,
                       __ARQRequestQueueHi->source,
                       __ARQRequestQueueHi->length);
        }
        __ARQCallbackHi = __ARQRequestQueueHi->callback;
        __ARQRequestPendingHi = __ARQRequestQueueHi;
        __ARQRequestQueueHi = __ARQRequestQueueHi->next;
    }
}


void __ARQServiceQueueLo(void)
{
    if (__ARQRequestPendingLo == 0 && __ARQRequestQueueLo != 0) {
        __ARQRequestPendingLo = __ARQRequestQueueLo;
        __ARQRequestQueueLo = __ARQRequestQueueLo->next;
    }

    if (__ARQRequestPendingLo != 0) {
        if (__ARQRequestPendingLo->length <= __ARQChunkSize) {
            if (__ARQRequestPendingLo->type == 0) {
                ARStartDMA(__ARQRequestPendingLo->type,
                           __ARQRequestPendingLo->source,
                           __ARQRequestPendingLo->destination,
                           __ARQRequestPendingLo->length);
            } else {
                ARStartDMA(__ARQRequestPendingLo->type,
                           __ARQRequestPendingLo->destination,
                           __ARQRequestPendingLo->source,
                           __ARQRequestPendingLo->length);
            }
            __ARQCallbackLo = __ARQRequestPendingLo->callback;
        } else if (__ARQRequestPendingLo->type == 0) {
            ARStartDMA(__ARQRequestPendingLo->type,
                       __ARQRequestPendingLo->source,
                       __ARQRequestPendingLo->destination, __ARQChunkSize);
        } else {
            ARStartDMA(__ARQRequestPendingLo->type,
                       __ARQRequestPendingLo->destination,
                       __ARQRequestPendingLo->source, __ARQChunkSize);
        }
        __ARQRequestPendingLo->length -= __ARQChunkSize;
        __ARQRequestPendingLo->source += __ARQChunkSize;
        __ARQRequestPendingLo->destination += __ARQChunkSize;
    }
}

void __ARQCallbackHack(unsigned long requestAddress)
{
    (void)requestAddress;
}

void __ARQInterruptServiceRoutine(void)
{
    if (__ARQCallbackHi != 0) {
        __ARQCallbackHi((unsigned long)__ARQRequestPendingHi);
        __ARQRequestPendingHi = 0;
        __ARQCallbackHi = 0;
    } else if (__ARQCallbackLo != 0) {
        __ARQCallbackLo((unsigned long)__ARQRequestPendingLo);
        __ARQRequestPendingLo = 0;
        __ARQCallbackLo = 0;
    }

    __ARQPopTaskQueueHi();
    if (__ARQRequestPendingHi == 0) {
        __ARQServiceQueueLo();
    }
}

void ARQInit(void)
{
    if (__ARQ_init_flag != 1) {
        OSRegisterVersion(__ARQVersion);
        __ARQRequestQueueHi = __ARQRequestQueueLo = 0;
        __ARQChunkSize = 0x1000;
        ARRegisterDMACallback(__ARQInterruptServiceRoutine);
        __ARQRequestPendingHi = 0;
        __ARQRequestPendingLo = 0;
        __ARQCallbackHi = 0;
        __ARQCallbackLo = 0;
        __ARQ_init_flag = 1;
    }
}

void ARQPostRequest(ARQRequest* request, unsigned long owner, unsigned long type,
                    unsigned long priority, unsigned long source,
                    unsigned long destination, unsigned long length,
                    ARQCallback callback)
{
    int enabled;

    request->next = 0;
    request->owner = owner;
    request->type = type;
    request->source = source;
    request->destination = destination;
    request->length = length;
    if (callback != 0) {
        request->callback = callback;
    } else {
        request->callback = __ARQCallbackHack;
    }

    enabled = OSDisableInterrupts();
    switch (priority) {
    case 0:
        if (__ARQRequestQueueLo != 0) {
            __ARQRequestTailLo->next = request;
        } else {
            __ARQRequestQueueLo = request;
        }
        __ARQRequestTailLo = request;
        break;
    case 1:
        if (__ARQRequestQueueHi != 0) {
            __ARQRequestTailHi->next = request;
        } else {
            __ARQRequestQueueHi = request;
        }
        __ARQRequestTailHi = request;
        break;
    }

    if (__ARQRequestPendingHi == 0 && __ARQRequestPendingLo == 0) {
        __ARQPopTaskQueueHi();
        if (__ARQRequestPendingHi == 0) {
            __ARQServiceQueueLo();
        }
    }
    OSRestoreInterrupts(enabled);
}
