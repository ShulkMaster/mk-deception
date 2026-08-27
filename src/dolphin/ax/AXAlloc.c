#include "dolphin/ax.h"
#include "dolphin/ax_internal.h"
#include "dolphin/os.h"

static AXVPB* __AXStackHead[AX_PRIORITY_STACKS];
static AXVPB* __AXStackTail[AX_PRIORITY_STACKS];
static AXVPB* __AXCallbackStack;

AXVPB* __AXGetStackHead(unsigned long priority)
{
    return __AXStackHead[priority];
}

void __AXServiceCallbackStack(void)
{
    AXVPB* voice;

    for (voice = __AXPopCallbackStack(); voice;
         voice = __AXPopCallbackStack()) {
        if (voice->priority != 0) {
            if (voice->callback) {
                voice->callback(voice);
            }
            __AXRemoveFromStack(voice);
            __AXPushFreeStack(voice);
        }
    }
}

void __AXAllocInit(void)
{
    unsigned long i;

    __AXCallbackStack = 0;
    for (i = 0; i < AX_PRIORITY_STACKS; i++) {
        __AXStackHead[i] = __AXStackTail[i] = 0;
    }
}

void __AXPushFreeStack(AXVPB* voice)
{
    voice->next = __AXStackHead[0];
    __AXStackHead[0] = voice;
    voice->priority = 0;
}

static inline AXVPB* __AXPopFreeStack(void)
{
    AXVPB* voice;

    voice = __AXStackHead[0];
    if (voice) {
        __AXStackHead[0] = voice->next;
    }
    return voice;
}

void __AXPushCallbackStack(AXVPB* voice)
{
    voice->next1 = __AXCallbackStack;
    __AXCallbackStack = voice;
}

AXVPB* __AXPopCallbackStack(void)
{
    AXVPB* voice = __AXCallbackStack;

    if (voice != 0) {
        __AXCallbackStack = voice->next1;
    }
    return voice;
}

void __AXRemoveFromStack(AXVPB* voice)
{
    unsigned long priority = voice->priority;
    AXVPB* head = __AXStackHead[priority];
    AXVPB* tail = __AXStackTail[priority];

    if (head == tail) {
        __AXStackHead[priority] = __AXStackTail[priority] = 0;
        return;
    }
    if (voice == head) {
        __AXStackHead[priority] = voice->next;
        __AXStackHead[priority]->prev = 0;
        return;
    }
    if (voice == tail) {
        __AXStackTail[priority] = voice->prev;
        __AXStackTail[priority]->next = 0;
        return;
    }
    head = voice->prev;
    tail = voice->next;
    head->next = tail;
    tail->prev = head;
}

static inline void __AXPushStackHead(AXVPB* voice, unsigned long priority)
{
    voice->next = __AXStackHead[priority];
    voice->prev = 0;
    if (voice->next) {
        __AXStackHead[priority]->prev = voice;
        __AXStackHead[priority] = voice;
    } else {
        __AXStackTail[priority] = voice;
        __AXStackHead[priority] = voice;
    }
    voice->priority = priority;
}

static inline AXVPB* __AXPopStackFromBottom(unsigned long priority)
{
    AXVPB* voice = 0;

    if (__AXStackHead[priority]) {
        if (__AXStackHead[priority] == __AXStackTail[priority]) {
            voice = __AXStackHead[priority];
            __AXStackHead[priority] = __AXStackTail[priority] = 0;
        } else if (__AXStackTail[priority]) {
            voice = __AXStackTail[priority];
            __AXStackTail[priority] = voice->prev;
            __AXStackTail[priority]->next = 0;
        }
    }
    return voice;
}

void AXFreeVoice(AXVPB* voice)
{
    unsigned long interrupts = OSDisableInterrupts();

    __AXRemoveFromStack(voice);
    if (voice->pb.state == 1) {
        voice->depop = 1;
    }
    __AXSetPBDefault(voice);
    __AXPushFreeStack(voice);
    OSRestoreInterrupts(interrupts);
}

AXVPB* AXAcquireVoice(unsigned long priority, AXVoiceCallback callback,
                      unsigned long user_context)
{
    unsigned long interrupts;
    AXVPB* voice;
    unsigned long i;

    interrupts = OSDisableInterrupts();
    voice = __AXPopFreeStack();
    if (voice == 0) {
        for (i = 1; i < priority; i++) {
            voice = __AXPopStackFromBottom(i);
            if (voice) {
                if (voice->pb.state == 1) {
                    voice->depop = 1;
                }
                if (voice->callback != 0) {
                    voice->callback(voice);
                }
                break;
            }
        }
    }
    if (voice) {
        __AXPushStackHead(voice, priority);
        voice->callback = callback;
        voice->user_context = user_context;
        __AXSetPBDefault(voice);
    }
    OSRestoreInterrupts(interrupts);
    return voice;
}

void AXSetVoicePriority(AXVPB* voice, unsigned long priority)
{
    unsigned long interrupts = OSDisableInterrupts();

    __AXRemoveFromStack(voice);
    __AXPushStackHead(voice, priority);
    OSRestoreInterrupts(interrupts);
}
