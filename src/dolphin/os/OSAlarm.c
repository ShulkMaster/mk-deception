#include "dolphin/base/PPCArch.h"
#include "dolphin/os.h"

typedef struct OSAlarmQueue {
    OSAlarm* head;
    OSAlarm* tail;
} OSAlarmQueue;


static void DecrementerExceptionHandler(__OSException exception,
                                        OSContext* context);
static int OnReset(int final);

static OSResetFunctionInfo ResetFunctionInfo = {OnReset, 0xFFFFFFFF, 0, 0};
static OSAlarmQueue AlarmQueue;

static void SetTimer(OSAlarm* alarm)
{
    OSTime delta;

    delta = alarm->fire - __OSGetSystemTime();
    if (delta < 0) {
        PPCMtdec(0);
    } else if (delta < 0x80000000) {
        PPCMtdec(delta);
    } else {
        PPCMtdec(0x7FFFFFFF);
    }
}

void OSInitAlarm(void)
{
    if (__OSGetExceptionHandler(8) != DecrementerExceptionHandler) {
        AlarmQueue.head = AlarmQueue.tail = 0;
        __OSSetExceptionHandler(8, DecrementerExceptionHandler);
        OSRegisterResetFunction(&ResetFunctionInfo);
    }
}

void OSCreateAlarm(OSAlarm* alarm)
{
    alarm->handler = 0;
    alarm->tag = 0;
}

static void InsertAlarm(OSAlarm* alarm, OSTime fire, OSAlarmHandler handler)
{
    OSAlarm* next;
    OSAlarm* previous;

    if (alarm->period > 0) {
        OSTime now = __OSGetSystemTime();
        fire = alarm->start;
        if (alarm->start < now) {
            fire += alarm->period * ((now - alarm->start) / alarm->period + 1);
        }
    }

    alarm->handler = handler;
    alarm->fire = fire;
    for (next = AlarmQueue.head; next; next = next->next) {
        if (next->fire <= fire) {
            continue;
        }
        alarm->prev = next->prev;
        next->prev = alarm;
        alarm->next = next;
        previous = alarm->prev;
        if (previous) {
            previous->next = alarm;
        } else {
            AlarmQueue.head = alarm;
            SetTimer(alarm);
        }
        return;
    }

    alarm->next = 0;
    previous = AlarmQueue.tail;
    AlarmQueue.tail = alarm;
    alarm->prev = previous;
    if (previous) {
        previous->next = alarm;
    } else {
        AlarmQueue.head = AlarmQueue.tail = alarm;
        SetTimer(alarm);
    }
}

void OSSetAlarm(OSAlarm* alarm, OSTime tick, OSAlarmHandler handler)
{
    int enabled = OSDisableInterrupts();
    alarm->period = 0;
    InsertAlarm(alarm, __OSGetSystemTime() + tick, handler);
    OSRestoreInterrupts(enabled);
}

void OSSetPeriodicAlarm(OSAlarm* alarm, OSTime start, OSTime period,
                        OSAlarmHandler handler)
{
    int enabled = OSDisableInterrupts();
    alarm->period = period;
    alarm->start = __OSTimeToSystemTime(start);
    InsertAlarm(alarm, 0, handler);
    OSRestoreInterrupts(enabled);
}

void OSCancelAlarm(OSAlarm* alarm)
{
    OSAlarm* next;
    int enabled = OSDisableInterrupts();

    if (!alarm->handler) {
        OSRestoreInterrupts(enabled);
        return;
    }

    next = alarm->next;
    if (!next) {
        AlarmQueue.tail = alarm->prev;
    } else {
        next->prev = alarm->prev;
    }
    if (alarm->prev) {
        alarm->prev->next = next;
    } else {
        AlarmQueue.head = next;
        if (next) {
            SetTimer(next);
        }
    }
    alarm->handler = 0;
    OSRestoreInterrupts(enabled);
}

static void DecrementerExceptionCallback(__OSException exception,
                                         OSContext* context)
{
    OSAlarm* alarm;
    OSAlarm* next;
    OSAlarmHandler handler;
    OSTime now;
    OSContext callback_context;

    now = __OSGetSystemTime();
    alarm = AlarmQueue.head;
    if (!alarm) {
        OSLoadContext(context);
        return;
    }
    if (now < alarm->fire) {
        SetTimer(alarm);
        OSLoadContext(context);
        return;
    }

    next = alarm->next;
    AlarmQueue.head = next;
    if (!next) {
        AlarmQueue.tail = 0;
    } else {
        next->prev = 0;
    }

    handler = alarm->handler;
    alarm->handler = 0;
    if (alarm->period > 0) {
        InsertAlarm(alarm, 0, handler);
    }
    if (AlarmQueue.head) {
        SetTimer(AlarmQueue.head);
    }

    OSDisableScheduler();
    OSClearContext(&callback_context);
    OSSetCurrentContext(&callback_context);
    handler(alarm, context);
    OSClearContext(&callback_context);
    OSSetCurrentContext(context);
    OSEnableScheduler();
    __OSReschedule();
    OSLoadContext(context);
}

/* Retail's leaf saves volatile exception registers before entering this C path. */
static void DecrementerExceptionHandler(__OSException exception,
                                        OSContext* context)
{
    DecrementerExceptionCallback(exception, context);
}

static int OnReset(int final)
{
    OSAlarm* alarm;
    OSAlarm* next;

    if (final) {
        alarm = AlarmQueue.head;
        next = alarm ? alarm->next : 0;
        while (alarm) {
            if (!__DVDTestAlarm(alarm)) {
                OSCancelAlarm(alarm);
            }
            alarm = next;
            next = alarm ? alarm->next : 0;
        }
    }
    return 1;
}
