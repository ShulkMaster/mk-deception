#include "dolphin/os.h"
#include "libmkparticle/rw_engine.h"
#include "rw/dlbrkpt.h"

typedef struct GXFifoObj GXFifoObj;
typedef void (*GXBreakPtCallback)(void);
typedef void (*GXDrawDoneCallback)(void);

typedef struct RwGxBreakPtEntry {
    void* address;
    RwUInt8 active;
    RwUInt8 drawDone;
    RwUInt8 reserved_0x06[2];
    RwGxDrawDoneUserCallback callback;
    void* data;
} RwGxBreakPtEntry;

typedef struct RwGxBreakPtQueue {
    RwUInt8 inCallback;
    RwUInt8 waiting;
    RwUInt8 restartRequested;
    RwUInt8 breakEnabled;
    void* currentAddress;
    RwInt32 currentIndex;
    RwInt32 capacity;
    RwInt32 count;
    RwInt32 head;
    RwInt32 pendingDrawDone;
    RwGxBreakPtEntry* entries;
} RwGxBreakPtQueue;

extern void GXEnableBreakPt(void* address);
extern void GXDisableBreakPt(void);
extern GXBreakPtCallback GXSetBreakPtCallback(GXBreakPtCallback callback);
extern GXDrawDoneCallback GXSetDrawDoneCallback(GXDrawDoneCallback callback);
extern void GXSetDrawDone(void);
extern GXFifoObj* GXGetCPUFifo(void);
extern void GXGetFifoPtrs(GXFifoObj* fifo, void** readPtr, void** writePtr);
extern void* memset(void* destination, int value, unsigned long size);

void MWY_GCN_RW_RestartFromGxBreakPtCurrent(void);

static void MWY_GCN_RW_GxBreakPtCallback_ForRW(void* data);
static RwInt32 i_SkipInactiveGxBreakPts(void);
static void i_AdvanceToNextGxBreakPt(void);
static RwInt32 i_FindGxBreakPt(void* address);
static void MWY_GCN_RW_GxDrawDoneCallback_General(void);
static void MWY_GCN_RW_GxBreakPtCallback_General(void);
static RwGxBreakPtEntry* i_MWY_GCN_RW_AppendGxBreakPtQueue(
    void* address, RwUInt8 active, RwGxDrawDoneUserCallback callback,
    void* data);
static void MWY_GCN_RW_AppendGxBreakPtQueue(void* address, RwBool active,
                                           RwGxDrawDoneUserCallback callback,
                                           void* data);

RwGxBreakPtQueue RwGxBreakPt_Q;
RwInt32 _RwGxBreakPtInitialQueueSize = 0x10;
RwInt32 RwGxBreakPt_bQInitialized;
RwGxBreakPtCallback RwGxBreakPt_Callback;
GXBreakPtCallback RwGxBreakPt_PreviousCallback;
GXDrawDoneCallback RwGxDrawDone_PreviousCallback;

static void MWY_GCN_RW_GxBreakPtCallback_ForRW(void* data)
{
    RwGxBreakPtCallback callback = RwGxBreakPt_Callback;

    (void)data;
    if (callback != NULL) {
        callback();
    }
}

static RwInt32 i_SkipInactiveGxBreakPts(void)
{
    RwInt32 count = RwGxBreakPt_Q.count;
    RwInt32 head = RwGxBreakPt_Q.head;
    RwInt32 capacity = RwGxBreakPt_Q.capacity;
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
    RwInt32 count = i_SkipInactiveGxBreakPts();

    if (count != 0) {
        RwInt32 index = RwGxBreakPt_Q.head;
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

static RwInt32 i_FindGxBreakPt(void* address)
{
    RwInt32 found = -1;
    RwInt32 count = RwGxBreakPt_Q.count;
    RwInt32 index = RwGxBreakPt_Q.head;
    RwInt32 capacity = RwGxBreakPt_Q.capacity;
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
        RwInt32 index = RwGxBreakPt_Q.head;
        RwGxBreakPtEntry* entry = &RwGxBreakPt_Q.entries[index];

        entry->callback(entry->data);
        MWY_GCN_RW_RestartFromGxBreakPtCurrent();
    }
    RwGxBreakPt_Q.pendingDrawDone--;
}

static void MWY_GCN_RW_GxBreakPtCallback_General(void)
{
    RwBool interrupts = OSDisableInterrupts();
    void* previousAddress;
    RwBool advanced;

    RwGxBreakPt_Q.inCallback = 1;
    if (RwGxBreakPt_Q.breakEnabled != 0) {
        previousAddress = RwGxBreakPt_Q.currentAddress;
    } else {
        previousAddress = NULL;
    }

    do {
        advanced = FALSE;
        RwGxBreakPt_Q.waiting = 0;
        RwGxBreakPt_Q.restartRequested = 0;
        if (i_SkipInactiveGxBreakPts() != 0) {
            RwGxBreakPtEntry* entry =
                &RwGxBreakPt_Q.entries[RwGxBreakPt_Q.head];

            if (entry->drawDone != 0) {
                if (RwGxBreakPt_Q.pendingDrawDone < 0) {
                    entry->callback(entry->data);
                    RwGxBreakPt_Q.restartRequested = 1;
                }
                RwGxBreakPt_Q.pendingDrawDone++;
            } else if (entry->callback != NULL) {
                entry->callback(entry->data);
            } else {
                RwGxBreakPt_Q.restartRequested = 1;
            }

            if (RwGxBreakPt_Q.restartRequested != 0) {
                entry->active = 0;
                RwGxBreakPt_Q.restartRequested = 0;
                i_AdvanceToNextGxBreakPt();
                advanced = TRUE;
            } else {
                RwGxBreakPt_Q.waiting = 1;
            }
        }
    } while (advanced != FALSE &&
             previousAddress == RwGxBreakPt_Q.currentAddress &&
             RwGxBreakPt_Q.breakEnabled != 0);

    RwGxBreakPt_Q.inCallback = 0;
    OSRestoreInterrupts(interrupts);
}

static RwGxBreakPtEntry* i_MWY_GCN_RW_AppendGxBreakPtQueue(
    void* address, RwUInt8 active, RwGxDrawDoneUserCallback callback,
    void* data)
{
    RwGxBreakPtEntry* entry = NULL;

    if (RwGxBreakPt_Q.capacity > RwGxBreakPt_Q.count) {
        RwInt32 index = RwGxBreakPt_Q.head + RwGxBreakPt_Q.count;

        RwGxBreakPt_Q.count++;
        if (index >= RwGxBreakPt_Q.capacity) {
            index -= RwGxBreakPt_Q.capacity;
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

static void MWY_GCN_RW_AppendGxBreakPtQueue(void* address, RwBool active,
                                           RwGxDrawDoneUserCallback callback,
                                           void* data)
{
    RwBool interrupts = OSDisableInterrupts();

    i_MWY_GCN_RW_AppendGxBreakPtQueue(address, active, callback, data);
    OSRestoreInterrupts(interrupts);
}

void MWY_GCN_RW_ActivateGxBreakPtQueue(void)
{
    RwBool interrupts = OSDisableInterrupts();

    if (RwGxBreakPt_bQInitialized == FALSE) {
        memset(&RwGxBreakPt_Q, 0, sizeof(RwGxBreakPt_Q));
        RwGxBreakPt_bQInitialized = TRUE;
        RwGxBreakPt_Q.capacity = _RwGxBreakPtInitialQueueSize;
        if (RwGxBreakPt_Q.capacity < 0x10) {
            RwGxBreakPt_Q.capacity = 0x10;
        }
        RwGxBreakPt_Q.entries = RwEngineInstance->fpMalloc(
            RwGxBreakPt_Q.capacity * sizeof(RwGxBreakPtEntry), 0x40411);
        memset(RwGxBreakPt_Q.entries, 0,
               RwGxBreakPt_Q.capacity * sizeof(RwGxBreakPtEntry));
        RwGxBreakPt_PreviousCallback =
            GXSetBreakPtCallback(MWY_GCN_RW_GxBreakPtCallback_General);
        RwGxDrawDone_PreviousCallback =
            GXSetDrawDoneCallback(MWY_GCN_RW_GxDrawDoneCallback_General);
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
    RwBool interrupts = OSDisableInterrupts();
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
    RwBool interrupts = OSDisableInterrupts();
    RwInt32 index = i_FindGxBreakPt(address);

    if (index >= 0) {
        RwGxBreakPt_Q.entries[index].active = 1;
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
    RwBool interrupts = OSDisableInterrupts();

    if (RwGxBreakPt_Q.inCallback != 0) {
        RwGxBreakPt_Q.restartRequested = 1;
    } else if (RwGxBreakPt_Q.waiting != 0 &&
               RwGxBreakPt_Q.breakEnabled != 0) {
        void* previousAddress = RwGxBreakPt_Q.currentAddress;

        RwGxBreakPt_Q.entries[RwGxBreakPt_Q.head].active = 0;
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
