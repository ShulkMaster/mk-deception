#include "dolphin/base/PPCArch.h"
#include "dolphin/os.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef unsigned long long u64;
typedef signed int s32;
typedef int BOOL;
typedef signed long OSPriority;
typedef void (*OSIdleFunction)(void* parameter);

#define TRUE 1
#define FALSE 0
#define NULL ((void*)0)
#define OS_THREAD_STATE_READY 1
#define OS_THREAD_STATE_RUNNING 2
#define OS_THREAD_STATE_WAITING 4
#define OS_THREAD_STATE_MORIBUND 8
#define OS_THREAD_ATTR_DETACH 1
#define OS_THREAD_STACK_MAGIC 0xDEADBABE
#define OS_THREAD_SPECIFIC_MAX 2
#define OS_PRIORITY_MIN 0
#define OS_PRIORITY_MAX 31
#define MSR_FP 0x2000
#define OS_ERROR_MAX 16
#define __OSCurrentThread (*(OSThread**)0x800000E4)
#define __gUnkThread1 (*(OSThread**)0x800000D8)
#define __OSActiveThreadQueue (*(OSThreadQueue*)0x800000DC)
#define LINE(first, second, third) (second)
#define ASSERTLINE(line, condition) ((void)0)
#define ASSERTMSGLINE(line, condition, message) ((void)0)
#define ASSERTMSG1LINE(line, condition, format, value) ((void)0)

extern unsigned char _stack_end[];
extern unsigned long __OSFpscrEnableBits;
extern OSErrorHandler __OSErrorTable[17];

#define ENQUEUE_THREAD(thread, queue, link)       \
    do {                                          \
        OSThread* __prev = (queue)->tail; \
        if (__prev == NULL) {                     \
            (queue)->head = (thread);             \
        } else {                                  \
            __prev->link.next = (thread);         \
        }                                         \
        (thread)->link.prev = __prev;             \
        (thread)->link.next = 0;                  \
        (queue)->tail = (thread);                 \
    } while(0);

#define DEQUEUE_THREAD(thread, queue, link)             \
    do {                                                \
        OSThread* __next = (thread)->link.next; \
        OSThread* __prev = (thread)->link.prev; \
        if (__next == NULL) {                           \
            (queue)->tail = __prev;                     \
        } else {                                        \
            __next->link.prev = __prev;                 \
        }                                               \
        if (__prev == NULL) {                           \
            (queue)->head = __next;                     \
        } else {                                        \
            __prev->link.next = __next;                 \
        }                                               \
    } while(0);

#define ENQUEUE_THREAD_PRIO(thread, queue, link)       \
    do {                                               \
        OSThread* __prev;                      \
        OSThread* __next;                      \
        for(__next = (queue)->head; __next             \
          && (__next->priority <= (thread)->priority); \
                __next = __next->link.next) ;          \
                                                       \
        if (__next == NULL) {                          \
            ENQUEUE_THREAD(thread, queue, link);       \
        } else {                                       \
            (thread)->link.next = __next;              \
            __prev = __next->link.prev;                \
            __next->link.prev = (thread);              \
            (thread)->link.prev = __prev;              \
            if (__prev == NULL) {                      \
                (queue)->head = (thread);              \
            } else {                                   \
                __prev->link.next = (thread);          \
            }                                          \
        }                                              \
    } while(0);

#define DEQUEUE_HEAD(thread, queue, link)             \
    do {                                              \
        OSThread* __next = thread->link.next; \
        if (__next == NULL) {                         \
            (queue)->tail = 0;                        \
        } else {                                      \
            __next->link.prev = 0;                    \
        }                                             \
        (queue)->head = __next;                       \
    } while(0);

// defined in linkscript
extern u8 _stack_end[];
extern u8 _stack_addr[];

static OSThreadQueue RunQueue[32];
static OSThread IdleThread;
static OSThread DefaultThread;
static OSContext IdleContext;
static volatile u32 RunQueueBits;
static volatile int RunQueueHint;
static s32 Reschedule;

#define ALIGN4(val) (((val) + 0x3) & ~0x3)
#define ALIGN8(val) (((val) + 0x7) & ~0x7)

// prototypes
static void OSInitMutexQueue(OSMutexQueue* queue);
static inline void __OSSwitchThread(OSThread* nextThread);
static inline int __OSIsThreadActive(OSThread* thread);
static inline void SetRun(OSThread* thread);
static void UnsetRun(OSThread* thread);
static OSThread* SetEffectivePriority(OSThread* thread, OSPriority priority);
static inline void UpdatePriority(OSThread* thread);
static OSThread* SelectThread(int yield);
static void DefaultSwitchThreadCallback(OSThread* from, OSThread* to) {}
static OSSwitchThreadCallback SwitchThreadCallback = DefaultSwitchThreadCallback;

static inline void OSSetCurrentThread(OSThread* thread) {
    SwitchThreadCallback(__OSCurrentThread, thread);
    __OSCurrentThread = thread;
}

void __OSThreadInit() {
    OSThread* thread = &DefaultThread;
    OSPriority prio;

    thread->state = OS_THREAD_STATE_RUNNING;
    thread->attr = 1;
    thread->priority = thread->base = 0x10;
    thread->suspend = 0;
    thread->value = (void*)-1;
    thread->mutex = 0;

    OSInitThreadQueue(&thread->queueJoin);
#ifdef DEBUG
    OSInitMutexQueue(&thread->queueMutex);
#else
    thread->queueMutex.head = thread->queueMutex.tail = 0;
#endif

    ASSERTLINE(LINE(348, 357, 357), PPCMfmsr() & MSR_FP);

    __gUnkThread1 = thread;
    OSClearContext(&thread->context);
    OSSetCurrentContext(&thread->context);
    thread->stackBase = (u8*)&_stack_addr;
    thread->stackEnd = (u32*)&_stack_end;
    *(u32*)thread->stackEnd = OS_THREAD_STACK_MAGIC;
    OSSetCurrentThread(thread);
    OSClearStack(0);
    RunQueueBits = 0;
    RunQueueHint = 0;

    for (prio = 0; prio <= 31; prio++) {
        OSInitThreadQueue(&RunQueue[prio]);
    }
    OSInitThreadQueue(&__OSActiveThreadQueue);

    ENQUEUE_THREAD(thread, &__OSActiveThreadQueue, linkActive);

    OSClearContext(&IdleContext);
    Reschedule = 0;
}

#if DEBUG
static void OSInitMutexQueue(OSMutexQueue* queue) {
    queue->head = queue->tail = 0;
}
#endif

void OSInitThreadQueue(OSThreadQueue* queue) {
    queue->head = queue->tail = 0;
}

OSThread* OSGetCurrentThread() {
    return __OSCurrentThread;
}

static inline void __OSSwitchThread(OSThread* nextThread) {
    OSSetCurrentThread(nextThread);
    OSSetCurrentContext(&nextThread->context);
    OSLoadContext(&nextThread->context);
}

static inline BOOL __OSIsThreadActive(OSThread* thread) {
    OSThread* active;

    if (thread->state == 0) {
        return FALSE;
    }

    for (active = __OSActiveThreadQueue.head; active; active = active->linkActive.next) {
        if (thread == active) {
            return TRUE;
        }
    }
    return FALSE;
}

s32 OSDisableScheduler(void) {
    BOOL enabled;
    s32 count;

    enabled = OSDisableInterrupts();
    count = Reschedule;
    Reschedule = count + 1;
    OSRestoreInterrupts(enabled);
    return count;
}

s32 OSEnableScheduler(void) {
    BOOL enabled;
    s32 count;

    enabled = OSDisableInterrupts();
    count = Reschedule;
    Reschedule = count - 1;
    OSRestoreInterrupts(enabled);
    return count;
}

static inline void SetRun(OSThread* thread) {
    ASSERTLINE(LINE(536, 554, 554), !IsSuspended(thread->suspend));
    ASSERTLINE(LINE(537, 555, 555), thread->state == OS_THREAD_STATE_READY);

    ASSERTLINE(LINE(539, 557, 557), OS_PRIORITY_MIN <= thread->priority && thread->priority <= OS_PRIORITY_MAX);

    thread->queue = &RunQueue[thread->priority];

    ENQUEUE_THREAD(thread, thread->queue, link);

    RunQueueBits |= 1 << (OS_PRIORITY_MAX - thread->priority);
    RunQueueHint = 1;
}

static void UnsetRun(OSThread* thread) {
    OSThreadQueue* queue;

    ASSERTLINE(LINE(560, 578, 578), thread->state == OS_THREAD_STATE_READY);

    ASSERTLINE(LINE(562, 580, 580), OS_PRIORITY_MIN <= thread->priority && thread->priority <= OS_PRIORITY_MAX);
    ASSERTLINE(LINE(563, 581, 581), thread->queue == &RunQueue[thread->priority]);

    queue = thread->queue;

    DEQUEUE_THREAD(thread, queue, link);

    if (!queue->head) {
        RunQueueBits &= ~(1 << (OS_PRIORITY_MAX - thread->priority));
    }
    thread->queue = NULL;
}

OSPriority __OSGetEffectivePriority(OSThread* thread) {
    s32 priority = thread->base;
    OSMutex* mutex;

    for (mutex = thread->queueMutex.head; mutex; mutex = mutex->link.next) {
        OSThread* blocked = mutex->queue.head;
        if (blocked && blocked->priority < priority) {
            priority = blocked->priority;
        }
    }
    return priority;
}

static OSThread* SetEffectivePriority(OSThread* thread, OSPriority priority) {
    ASSERTLINE(LINE(614, 632, 632), !IsSuspended(thread->suspend));

    switch(thread->state) {
    case OS_THREAD_STATE_READY:
        UnsetRun(thread);
        thread->priority = priority;
        SetRun(thread);
        break;
    case OS_THREAD_STATE_WAITING:
        DEQUEUE_THREAD(thread, thread->queue, link);
        thread->priority = priority;

        ENQUEUE_THREAD_PRIO(thread, thread->queue, link);

        if (thread->mutex) {
            ASSERTLINE(LINE(629, 647, 647), thread->mutex->thread);
            return thread->mutex->thread;
        }
        break;
    case OS_THREAD_STATE_RUNNING:
        RunQueueHint = 1;
        thread->priority = priority;
        break;
    }
    return 0;
}

static inline void UpdatePriority(OSThread* thread) {
    s32 priority;

    while (1) {
        if(thread->suspend > 0) {
            break;
        }
        priority = __OSGetEffectivePriority(thread);
        if (thread->priority == priority) {
            break;
        }
        thread = SetEffectivePriority(thread, priority);
        if (thread == 0) {
            break;
        }
    }
}

void __OSPromoteThread(OSThread* thread, OSPriority priority) {
    while (1) {
        if (thread->suspend > 0 || thread->priority <= priority) {
            break;
        }
        thread = SetEffectivePriority(thread, priority);
        if (thread == 0) {
            break;
        }
    }
}

static OSThread* SelectThread(int yield) {
    OSContext* currentContext;
    OSThread* currentThread;
    OSThread* nextThread;
    OSPriority priority;
    OSThreadQueue* queue;

    if (Reschedule > 0) {
        return NULL;
    }

    currentContext = OSGetCurrentContext();
    currentThread = OSGetCurrentThread();

    if (currentContext != &currentThread->context) {
        return NULL;
    }

    if (currentThread) {
        if (currentThread->state == 2) {
            if (yield == 0) {
                priority = __cntlzw(RunQueueBits);
                if (currentThread->priority <= priority)
                    return NULL;
            }
            currentThread->state = OS_THREAD_STATE_READY;
            SetRun(currentThread);
        }
        if (!(currentThread->context.state & 2) && (OSSaveContext(&currentThread->context) != 0)) {
            return NULL;
        }
    }

    if (RunQueueBits == 0) {
        OSSetCurrentThread(NULL);
        OSSetCurrentContext(&IdleContext);
        do {
            OSEnableInterrupts();
            while (RunQueueBits == 0) ;
            OSDisableInterrupts();
        } while (RunQueueBits == 0);
        OSClearContext(&IdleContext);
    }

    RunQueueHint = 0;
    priority = __cntlzw(RunQueueBits);

    ASSERTLINE(LINE(777, 808, 808), OS_PRIORITY_MIN <= priority && priority <= OS_PRIORITY_MAX);

    queue = &RunQueue[priority];
    nextThread = queue->head;

    DEQUEUE_HEAD(nextThread, queue, link);

    ASSERTLINE(LINE(780, 811, 811), nextThread->priority == priority);

    if (!queue->head) {
        RunQueueBits &= ~(1 << (OS_PRIORITY_MAX - priority));
    }
    nextThread->queue = 0;
    nextThread->state = OS_THREAD_STATE_RUNNING;
    __OSSwitchThread(nextThread);
    return nextThread;
}

void __OSReschedule(void) {
    if (RunQueueHint != 0) {
        SelectThread(0);
    }
}

void OSYieldThread(void) {
    BOOL enabled = OSDisableInterrupts();

    SelectThread(1);
    OSRestoreInterrupts(enabled);
}

int OSCreateThread(OSThread* thread, void* (*func)(void*), void* param, void* stack, u32 stackSize, OSPriority priority, u16 attr) {
    BOOL enabled;
    u32 sp;
    int i;

    ASSERTMSGLINE(LINE(864, 895, 895), ((priority >= OS_PRIORITY_MIN) && (priority <= OS_PRIORITY_MAX)), "OSCreateThread(): priority out of range (0 <= priority <= 31).");

    if ((priority < OS_PRIORITY_MIN) || (priority > OS_PRIORITY_MAX)) {
        return 0;
    }

    thread->state = OS_THREAD_STATE_READY;
    thread->attr = attr & 1U;
    thread->base = priority;
    thread->priority = priority;
    thread->suspend = 1;
    thread->value = (void*)-1;
    thread->mutex = 0;
    OSInitThreadQueue(&thread->queueJoin);
#ifdef DEBUG
    OSInitMutexQueue(&thread->queueMutex);
#else
    thread->queueMutex.head = 0;
    thread->queueMutex.tail = 0;
#endif
    sp = (u32)stack;
    sp &= ~7;
    sp -= 8;
    ((u32*)sp)[0] = 0;
    ((u32*)sp)[1] = 0;
    OSInitContext(&thread->context, (u32)func, sp);
    thread->context.lr = (u32)&OSExitThread;
    thread->context.gpr[3] = (u32)param;
    thread->stackBase = stack;
    thread->stackEnd = (void*)((unsigned int)stack - stackSize);
    *thread->stackEnd = OS_THREAD_STACK_MAGIC;
    thread->error = 0;
    for (i = 0; i < 2; i++) {
        thread->specific[i] = NULL;
    }
    enabled = OSDisableInterrupts();

    if (__OSErrorTable[16] != NULL) {
        thread->context.srr1 |= 0x900;
        thread->context.state |= 1;
        thread->context.fpscr = (__OSFpscrEnableBits & 0xf8) | 4;
        for (i = 0; i < 32; ++i) {
            *(u64*)&thread->context.fpr[i] = (u64)0xffffffffffffffffLL;
            *(u64*)&thread->context.psf[i] = (u64)0xffffffffffffffffLL;
        }
    }

    ASSERTMSG1LINE(LINE(918, 949, 949), __OSIsThreadActive(thread) == 0L, "OSCreateThread(): thread %p is still active.", thread);

    ENQUEUE_THREAD(thread, &__OSActiveThreadQueue, linkActive);

    OSRestoreInterrupts(enabled);
    return 1;
}

void OSExitThread(void* val) {
    BOOL enabled = OSDisableInterrupts();
    OSThread* currentThread = OSGetCurrentThread();

    ASSERTMSGLINE(LINE(943, 974, 974), currentThread,
        "OSExitThread(): current thread does not exist.");
    ASSERTMSGLINE(LINE(945, 976, 976), currentThread->state == OS_THREAD_STATE_RUNNING,
        "OSExitThread(): current thread is not running.");
    ASSERTMSGLINE(LINE(947, 978, 978), __OSIsThreadActive(currentThread) != 0,
        "OSExitThread(): current thread is not active.");

    OSClearContext(&currentThread->context);
    if (currentThread->attr & 1) {
        DEQUEUE_THREAD(currentThread, &__OSActiveThreadQueue, linkActive);
        currentThread->state = 0;
    } else {
        currentThread->state = 8;
        currentThread->value = val;
    }
    __OSUnlockAllMutex(currentThread);
    OSWakeupThread(&currentThread->queueJoin);
    RunQueueHint = 1;
#ifdef DEBUG
    __OSReschedule();
#else
    if (RunQueueHint != 0) {
        SelectThread(0);
    }
#endif
    OSRestoreInterrupts(enabled);
}

void OSCancelThread(OSThread* thread) {
    BOOL enabled = OSDisableInterrupts();

    ASSERTMSG1LINE(LINE(985, 1016, 1016), __OSIsThreadActive(thread) != 0,
        "OSExitThread(): thread %p is not active.", thread);

    switch(thread->state) {
    case OS_THREAD_STATE_READY:
        if (thread->suspend <= 0) {
            UnsetRun(thread);
        }
        break;
    case OS_THREAD_STATE_RUNNING:
        RunQueueHint = 1;
        break;
    case OS_THREAD_STATE_WAITING:
        DEQUEUE_THREAD(thread, thread->queue, link);
        thread->queue = 0;
        if ((thread->suspend <= 0) && (thread->mutex)) {
            ASSERTLINE(LINE(1004, 1035, 1035), thread->mutex->thread);
            UpdatePriority(thread->mutex->thread);
        }
        break;
    default:
        OSRestoreInterrupts(enabled);
        return;
    }
    OSClearContext(&thread->context);
    if (thread->attr & 1) {
        DEQUEUE_THREAD(thread, &__OSActiveThreadQueue, linkActive);
        thread->state = 0;
    } else {
        thread->state = 8;
    }
    __OSUnlockAllMutex(thread);
    OSWakeupThread(&thread->queueJoin);
    __OSReschedule();
    OSRestoreInterrupts(enabled);
}

int OSJoinThread(OSThread* thread, void** val) {
    BOOL enabled = OSDisableInterrupts();

    ASSERTMSG1LINE(LINE(1061, 1092, 1092), __OSIsThreadActive(thread) != 0, "OSJoinThread(): thread %p is not active.", thread);

    if (!(thread->attr & 1) && (thread->state != OS_THREAD_STATE_MORIBUND) && (thread->queueJoin.head == NULL)) {
        OSSleepThread(&thread->queueJoin);
        if (__OSIsThreadActive(thread) == 0) {
            OSRestoreInterrupts(enabled);
            return 0;
        }
    }
    if (thread->state == OS_THREAD_STATE_MORIBUND) {
        if (val) {
            *val = thread->value;
        }
        DEQUEUE_THREAD(thread, &__OSActiveThreadQueue, linkActive);
        thread->state = 0;
        OSRestoreInterrupts(enabled);
        return 1;
    }
    OSRestoreInterrupts(enabled);
    return 0;
}

s32 OSResumeThread(OSThread* thread) {
    BOOL enabled = OSDisableInterrupts();
    s32 suspendCount;

    ASSERTMSG1LINE(LINE(1140, 1171, 1171), __OSIsThreadActive(thread) != 0, "OSResumeThread(): thread %p is not active.", thread);
    ASSERTMSG1LINE(LINE(1142, 1173, 1173), thread->state != OS_THREAD_STATE_MORIBUND, "OSResumeThread(): thread %p is terminated.", thread);

    suspendCount = thread->suspend--;
    if (thread->suspend < 0) {
        thread->suspend = 0;
    } else if (thread->suspend == 0) {
        switch(thread->state) {
        case OS_THREAD_STATE_READY:
            thread->priority = __OSGetEffectivePriority(thread);
            SetRun(thread);
            break;
        case OS_THREAD_STATE_WAITING:
            ASSERTLINE(LINE(1157, 1188, 1188), thread->queue);
            DEQUEUE_THREAD(thread, thread->queue, link);
            thread->priority = __OSGetEffectivePriority(thread);
            ENQUEUE_THREAD_PRIO(thread, thread->queue, link);
            if (thread->mutex) {
                UpdatePriority(thread->mutex->thread);
            }
        }
        __OSReschedule();
    }
    OSRestoreInterrupts(enabled);
    return suspendCount;
}

s32 OSSuspendThread(OSThread* thread) {
    BOOL enabled = OSDisableInterrupts();
    s32 suspendCount;

    ASSERTMSG1LINE(LINE(1191, 1222, 1222), __OSIsThreadActive(thread) != 0, "OSSuspendThread(): thread %p is not active.", thread);
    ASSERTMSG1LINE(LINE(1193, 1224, 1224), thread->state != OS_THREAD_STATE_MORIBUND, "OSSuspendThread(): thread %p is terminated.", thread);

    suspendCount = thread->suspend++;
    if (suspendCount == 0) {
        switch(thread->state) {
        case OS_THREAD_STATE_RUNNING:
            RunQueueHint = 1;
            thread->state = 1;
            break;
        case OS_THREAD_STATE_READY:
            UnsetRun(thread);
            break;
        case OS_THREAD_STATE_WAITING:
            DEQUEUE_THREAD(thread, thread->queue, link);
            thread->priority = 0x20;
            ENQUEUE_THREAD(thread, thread->queue, link);
            if (thread->mutex) {
                ASSERTLINE(LINE(1214, 1245, 1245), thread->mutex->thread);
                UpdatePriority(thread->mutex->thread);
            }
            break;
        }
        __OSReschedule();
    }
    OSRestoreInterrupts(enabled);
    return suspendCount;
}

void OSSleepThread(OSThreadQueue* queue) {
    BOOL enabled = OSDisableInterrupts();
    OSThread* currentThread = OSGetCurrentThread();

    ASSERTMSGLINE(LINE(1247, 1278, 1278), currentThread, "OSSleepThread(): current thread does not exist.");
    ASSERTMSG1LINE(LINE(1249, 1280, 1280), __OSIsThreadActive(currentThread) != 0, "OSSleepThread(): current thread %p is not active.", currentThread);
    ASSERTMSG1LINE(LINE(1251, 1282, 1282), currentThread->state == OS_THREAD_STATE_RUNNING, "OSSleepThread(): current thread %p is not running.", currentThread);
    ASSERTMSG1LINE(LINE(1253, 1284, 1284), currentThread->suspend <= 0, "OSSleepThread(): current thread %p is suspended.", currentThread);

    currentThread->state = OS_THREAD_STATE_WAITING;
    currentThread->queue = queue;
    ENQUEUE_THREAD_PRIO(currentThread, queue, link);
    RunQueueHint = 1;
    __OSReschedule();
    OSRestoreInterrupts(enabled);
}

void OSWakeupThread(OSThreadQueue* queue) {
    BOOL enabled = OSDisableInterrupts();

    while (queue->head) {
        OSThread* thread = queue->head;

        DEQUEUE_HEAD(thread, queue, link);

        ASSERTLINE(LINE(1282, 1313, 1313), __OSIsThreadActive(thread));
        ASSERTLINE(LINE(1283, 1314, 1314), thread->state != OS_THREAD_STATE_MORIBUND);
        ASSERTLINE(LINE(1284, 1315, 1315), thread->queue == queue);
        thread->state = OS_THREAD_STATE_READY;
        if (thread->suspend <= 0) {
            SetRun(thread);
        }
    }
    __OSReschedule();
    OSRestoreInterrupts(enabled);
}

int OSSetThreadPriority(OSThread* thread, OSPriority priority) {
    BOOL enabled;

    ASSERTMSGLINE(LINE(1310, 1341, 1341), (priority >= OS_PRIORITY_MIN) && (priority <= OS_PRIORITY_MAX), "OSSetThreadPriority(): priority out of range (0 <= priority <= 31).");

    if ((priority < OS_PRIORITY_MIN) || (priority > OS_PRIORITY_MAX)) {
        return 0;
    }
    enabled = OSDisableInterrupts();

    ASSERTMSG1LINE(LINE(1317, 1348, 1348), __OSIsThreadActive(thread) != 0, "OSSetThreadPriority(): thread %p is not active.", thread);
    ASSERTMSG1LINE(LINE(1319, 1350, 1350), thread->state != 8, "OSSetThreadPriority(): thread %p is terminated.", thread);

    if (thread->base != priority) {
        thread->base = priority;
        UpdatePriority(thread);
        __OSReschedule();
    }
    OSRestoreInterrupts(enabled);
    return 1;
}

OSPriority OSGetThreadPriority(OSThread* thread) {
    return thread->base;
}

void OSClearStack(u8 val) {
    u32 sp;
    u32* p;
    u32 pattern;

    pattern = (val << 24) | (val << 16) | (val << 8) | val;
    sp = (u32)OSGetStackPointer();
    for (p = __OSCurrentThread->stackEnd + 1; (u32)p < sp; ++p) {
        *p = pattern;
    }
}
