#include "dolphin/os.h"
#include "runtime/cstring.h"
#include "dolphin/gx_fifo.h"
#include "rw/rwengine.h"
#include "rw/dlbrkpt.h"

typedef struct RwGxBreakPtEntry {
    void* address;
    unsigned char active;
    unsigned char drawDone;
    unsigned char reserved_0x06[2];
    RwGxDrawDoneUserCallback callback;
    void* data;
} RwGxBreakPtEntry;

typedef struct RwGxBreakPtQueue {
    unsigned char inCallback;
    unsigned char waiting;
    unsigned char restartRequested;
    unsigned char breakEnabled;
    void* currentAddress;
    int currentIndex;
    int capacity;
    int count;
    int head;
    int pendingDrawDone;
    RwGxBreakPtEntry* entries;
} RwGxBreakPtQueue;

void MWY_GCN_RW_RestartFromGxBreakPtCurrent(void);

static void MWY_GCN_RW_GxBreakPtCallback_ForRW(void* data);
static int i_SkipInactiveGxBreakPts(void);
static void i_AdvanceToNextGxBreakPt(void);
static int i_FindGxBreakPt(void* address);
static void MWY_GCN_RW_GxDrawDoneCallback_General(void);
static void MWY_GCN_RW_GxBreakPtCallback_General(void);
static RwGxBreakPtEntry* i_MWY_GCN_RW_AppendGxBreakPtQueue(
    void* address, unsigned char active, RwGxDrawDoneUserCallback callback,
    void* data);
static void MWY_GCN_RW_AppendGxBreakPtQueue(void* address, int active,
                                           RwGxDrawDoneUserCallback callback,
                                           void* data);

RwGxBreakPtQueue RwGxBreakPt_Q;
int _RwGxBreakPtInitialQueueSize = 0x10;
int RwGxBreakPt_bQInitialized;
RwGxBreakPtCallback RwGxBreakPt_Callback;
GXBreakPtCallback RwGxBreakPt_PreviousCallback;
GXDrawDoneCallback RwGxDrawDone_PreviousCallback;

static void MWY_GCN_RW_GxBreakPtCallback_ForRW(void* data)
{
    RwGxBreakPtCallback callback = RwGxBreakPt_Callback;

    (void)data;
    if (callback != 0) {
        callback();
    }
}

static int i_SkipInactiveGxBreakPts(void)
{


    int count = RwGxBreakPt_Q.count;
    int head = RwGxBreakPt_Q.head;
    int capacity = RwGxBreakPt_Q.capacity;
    RwGxBreakPtEntry* entry = &RwGxBreakPt_Q.entries[head];

    while (count > 0 && entry->active == 0) {
        head++;
        if (head >= capacity) {
            head -= capacity;
        }
        entry = &RwGxBreakPt_Q.entries[head];
        count--;
    }
    RwGxBreakPt_Q.count = count;
    RwGxBreakPt_Q.head = head;
    return count;
}

static void i_AdvanceToNextGxBreakPt(void)
{


    int count = i_SkipInactiveGxBreakPts();

    if (count != 0) {
        int index = RwGxBreakPt_Q.head;
        RwGxBreakPtEntry* entry = &RwGxBreakPt_Q.entries[index];

        RwGxBreakPt_Q.breakEnabled = 1;
        RwGxBreakPt_Q.currentAddress = entry->address;
        RwGxBreakPt_Q.currentIndex = index;
        GXEnableBreakPt(entry->address);
    } else {
        RwGxBreakPt_Q.breakEnabled = 0;
        GXDisableBreakPt();
    }
}

static int i_FindGxBreakPt(void* address)
{
    int found = -1;
    int count = RwGxBreakPt_Q.count;
    int index = RwGxBreakPt_Q.head;
    int capacity = RwGxBreakPt_Q.capacity;
    RwGxBreakPtEntry* entry = &RwGxBreakPt_Q.entries[index];

    while (count != 0) {
        if (entry->address == address) {
            if (found < 0) {
                found = index;
            }
            if (entry->active == 0) {
                return index;
            }
        }
        index++;
        if (index >= capacity) {
            index -= capacity;
        }
        entry = &RwGxBreakPt_Q.entries[index];
        count--;
    }
    return found;
}

static void MWY_GCN_RW_GxDrawDoneCallback_General(void)
{
    if (RwGxBreakPt_Q.pendingDrawDone > 0 && RwGxBreakPt_Q.waiting != 0 &&
        RwGxBreakPt_Q.breakEnabled != 0) {
        int index = RwGxBreakPt_Q.head;
        RwGxBreakPtEntry* entry = &RwGxBreakPt_Q.entries[index];

        entry->callback(entry->data);
        MWY_GCN_RW_RestartFromGxBreakPtCurrent();
    }
    RwGxBreakPt_Q.pendingDrawDone--;
}

static void MWY_GCN_RW_GxBreakPtCallback_General(void)
{
    int interrupts = OSDisableInterrupts();
    void* previousAddress;
    int advanced;
    int count;
    int index;
    RwGxBreakPtEntry* entry;

    RwGxBreakPt_Q.inCallback = 1;
    if (RwGxBreakPt_Q.breakEnabled != 0) {
        previousAddress = RwGxBreakPt_Q.currentAddress;
    } else {
        previousAddress = 0;
    }

    do {
        advanced = 0;
        RwGxBreakPt_Q.waiting = 0;
        RwGxBreakPt_Q.restartRequested = 0;
        count = i_SkipInactiveGxBreakPts();
        if (count != 0) {
            index = RwGxBreakPt_Q.head;
            entry = &RwGxBreakPt_Q.entries[index];

            if (entry->drawDone != 0) {
                if (RwGxBreakPt_Q.pendingDrawDone < 0) {
                    entry->callback(entry->data);
                    RwGxBreakPt_Q.restartRequested = 1;
                }
                RwGxBreakPt_Q.pendingDrawDone++;
            } else if (entry->callback != 0) {
                entry->callback(entry->data);
            } else {
                RwGxBreakPt_Q.restartRequested = 1;
            }

            if (RwGxBreakPt_Q.restartRequested != 0) {
                entry->active = 0;
                RwGxBreakPt_Q.restartRequested = 0;
                i_AdvanceToNextGxBreakPt();
                advanced = 1;
            } else {
                RwGxBreakPt_Q.waiting = 1;
            }
        }
    } while (advanced != 0 &&
             previousAddress == RwGxBreakPt_Q.currentAddress &&
             RwGxBreakPt_Q.breakEnabled != 0);

    RwGxBreakPt_Q.inCallback = 0;
    OSRestoreInterrupts(interrupts);
}

static RwGxBreakPtEntry* i_MWY_GCN_RW_AppendGxBreakPtQueue(
    void* address, unsigned char active, RwGxDrawDoneUserCallback callback,
    void* data)
{
    RwGxBreakPtEntry* entry = 0;

    if (RwGxBreakPt_Q.capacity > RwGxBreakPt_Q.count) {
        int count = RwGxBreakPt_Q.count;
        int head = RwGxBreakPt_Q.head;
        int capacity = RwGxBreakPt_Q.capacity;
        int index = head + count;

        RwGxBreakPt_Q.count = count + 1;
        if (index >= capacity) {
            index -= capacity;
        }
        entry = &RwGxBreakPt_Q.entries[index];
        entry->address = address;
        entry->active = active;
        entry->callback = callback;
        entry->data = data;
        entry->drawDone = 0;
        if (active != 0 && RwGxBreakPt_Q.breakEnabled == 0) {
            RwGxBreakPt_Q.breakEnabled = 1;
            RwGxBreakPt_Q.currentAddress = address;
            RwGxBreakPt_Q.currentIndex = index;
            GXEnableBreakPt(address);
        }
    }
    return entry;
}

static void MWY_GCN_RW_AppendGxBreakPtQueue(void* address, int active,
                                           RwGxDrawDoneUserCallback callback,
                                           void* data)
{
    int interrupts = OSDisableInterrupts();

    i_MWY_GCN_RW_AppendGxBreakPtQueue(address, active, callback, data);
    OSRestoreInterrupts(interrupts);
}

void MWY_GCN_RW_ActivateGxBreakPtQueue(void)
{


    int interrupts = OSDisableInterrupts();
    GXBreakPtCallback previousBreak;
    GXDrawDoneCallback previousDrawDone;

    if (RwGxBreakPt_bQInitialized == 0) {
        memset(&RwGxBreakPt_Q, 0, sizeof(RwGxBreakPt_Q));
        RwGxBreakPt_bQInitialized = 1;
        RwGxBreakPt_Q.capacity = _RwGxBreakPtInitialQueueSize;
        if (RwGxBreakPt_Q.capacity < 0x10) {
            RwGxBreakPt_Q.capacity = 0x10;
        }
        RwGxBreakPt_Q.entries = RwEngineInstance->fpMalloc(
            RwGxBreakPt_Q.capacity * sizeof(RwGxBreakPtEntry), 0x40411);
        memset(RwGxBreakPt_Q.entries, 0,
               RwGxBreakPt_Q.capacity * sizeof(RwGxBreakPtEntry));
        previousBreak =
            GXSetBreakPtCallback(MWY_GCN_RW_GxBreakPtCallback_General);
        RwGxBreakPt_PreviousCallback = previousBreak;
        previousDrawDone =
            GXSetDrawDoneCallback(MWY_GCN_RW_GxDrawDoneCallback_General);
        RwGxDrawDone_PreviousCallback = previousDrawDone;
    }
    OSRestoreInterrupts(interrupts);
}

void MWY_GCN_RW_SetGxBreakPtCallback(RwGxBreakPtCallback callback)
{
    MWY_GCN_RW_ActivateGxBreakPtQueue();
    RwGxBreakPt_Callback = callback;
}

void MWY_GCN_RW_InsertGxDrawDoneCallback(RwGxDrawDoneUserCallback callback,
                                         void* data)
{
    int interrupts = OSDisableInterrupts();
    void* readPtr;
    void* writePtr;
    RwGxBreakPtEntry* entry;

    GXSetDrawDone();
    GXGetFifoPtrs(GXGetCPUFifo(), &readPtr, &writePtr);
    entry = i_MWY_GCN_RW_AppendGxBreakPtQueue(writePtr, 1, callback, data);
    entry->drawDone = 1;
    OSRestoreInterrupts(interrupts);
}

void MWY_GCN_RW_ActivateGxBreakPt(void* address)
{


    int interrupts = OSDisableInterrupts();
    RwGxBreakPtEntry* entry;
    int index = i_FindGxBreakPt(address);

    if (index >= 0) {
        entry = &RwGxBreakPt_Q.entries[index];
        entry->active = 1;
        if (RwGxBreakPt_Q.inCallback == 0 &&
            RwGxBreakPt_Q.breakEnabled != 0 &&
            RwGxBreakPt_Q.waiting == 0) {
            i_AdvanceToNextGxBreakPt();
        }
    }
    OSRestoreInterrupts(interrupts);
}

void MWY_GCN_RW_RestartFromGxBreakPtCurrent(void)
{


    void* previousAddress;
    int interrupts = OSDisableInterrupts();

    if (RwGxBreakPt_Q.inCallback != 0) {
        RwGxBreakPt_Q.restartRequested = 1;
    } else if (RwGxBreakPt_Q.waiting != 0 &&
               RwGxBreakPt_Q.breakEnabled != 0) {
        RwGxBreakPtEntry* entry;

        previousAddress = RwGxBreakPt_Q.currentAddress;
        entry = &RwGxBreakPt_Q.entries[RwGxBreakPt_Q.head];
        entry->active = 0;
        RwGxBreakPt_Q.waiting = 0;
        i_AdvanceToNextGxBreakPt();
        if (previousAddress == RwGxBreakPt_Q.currentAddress &&
            RwGxBreakPt_Q.breakEnabled != 0) {
            MWY_GCN_RW_GxBreakPtCallback_General();
        }
    }
    OSRestoreInterrupts(interrupts);
}

void MWY_GCN_RW_InsertRwGxBreakPt(void* address)
{
    MWY_GCN_RW_AppendGxBreakPtQueue(address, 1,
                                   MWY_GCN_RW_GxBreakPtCallback_ForRW,
                                   (void*)RwGxBreakPt_Callback);
}

void MWY_GCN_RW_NoteRwGxBreakPt(void* address)
{
    MWY_GCN_RW_AppendGxBreakPtQueue(address, 0,
                                   MWY_GCN_RW_GxBreakPtCallback_ForRW,
                                   (void*)RwGxBreakPt_Callback);
}
