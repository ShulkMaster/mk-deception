#ifndef PLATFORM_OS_TYPES_H
#define PLATFORM_OS_TYPES_H

#include "dolphin/types.h"

typedef int OSHeapHandle;
typedef struct OSContext OSContext;
typedef struct OSThread OSThread;
typedef struct OSMutex OSMutex;

struct OSContext {
    u32 gpr[32];
    u32 cr, lr, ctr, xer;
    double fpr[32];
    u32 fpscr_pad, fpscr, srr0, srr1;
    unsigned short mode, state;
    u32 gqr[8];
    u32 psf_pad;
    double psf[32];
};

typedef struct OSThreadQueue {
    OSThread* head;
    OSThread* tail;
} OSThreadQueue;

typedef struct OSThreadLink {
    OSThread* next;
    OSThread* prev;
} OSThreadLink;

typedef struct OSMutexQueue {
    OSMutex* head;
    OSMutex* tail;
} OSMutexQueue;

typedef struct OSMutexLink {
    OSMutex* next;
    OSMutex* prev;
} OSMutexLink;

struct OSMutex {
    OSThreadQueue queue;
    OSThread* thread;
    s32 count;
    OSMutexLink link;
};

struct OSThread {
    OSContext context;
    unsigned short state;
    unsigned short attr;
    s32 suspend;
    s32 priority;
    s32 base;
    void* value;
    OSThreadQueue* queue;
    OSThreadLink link;
    OSThreadQueue queueJoin;
    OSMutex* mutex;
    OSMutexQueue queueMutex;
    OSThreadLink linkActive;
    unsigned char* stackBase;
    u32* stackEnd;
    s32 error;
    void* specific[2];
};

typedef struct OSCond {
    OSThreadQueue queue;
} OSCond;

typedef char OSContextSizeCheck[sizeof(OSContext) == 0x2C8 ? 1 : -1];
typedef char OSMutexSizeCheck[sizeof(OSMutex) == 0x18 ? 1 : -1];
typedef char OSThreadSizeCheck[sizeof(OSThread) == 0x318 ? 1 : -1];
typedef char OSCondSizeCheck[sizeof(OSCond) == 0x08 ? 1 : -1];
struct OSAlarm {
    void (*handler)(struct OSAlarm* alarm, OSContext* context);
    u32 tag;
    signed long long fire;
    struct OSAlarm* prev;
    struct OSAlarm* next;
    signed long long period;
    signed long long start;
};
typedef struct OSAlarm OSAlarm;
typedef char OSAlarmSizeCheck[sizeof(OSAlarm) == 0x28 ? 1 : -1];
#endif
