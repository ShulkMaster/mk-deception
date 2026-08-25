#include "dolphin/dvd.h"
#include "dolphin/os.h"

extern volatile unsigned long __DIRegs[];
extern volatile unsigned long __PIRegs[];

static int FirstRead = 1;
static volatile int StopAtNextInt;
static unsigned long LastLength;
static DVDLowCallback Callback;
static DVDLowCallback ResetCoverCallback;
static volatile OSTime LastResetEnd;
static volatile unsigned long ResetOccurred;
static volatile int WaitingCoverClose;
static volatile int Breaking;
static volatile unsigned long WorkAroundType;
static unsigned long WorkAroundSeekLocation;
static volatile OSTime LastReadFinished;
static OSTime LastReadIssued;
static volatile int LastCommandWasRead;
static volatile unsigned long NextCommandNumber;

typedef struct DVDBuffer {
    void* address;
    unsigned long length;
    unsigned long offset;
} DVDBuffer;

typedef struct DVDLowCommand {
    signed long command;
    void* address;
    unsigned long length;
    unsigned long offset;
    DVDLowCallback callback;
} DVDLowCommand;

static DVDLowCommand CommandList[3];
static OSAlarm AlarmForWA;
static OSAlarm AlarmForTimeout;
static OSAlarm AlarmForBreak;
static DVDBuffer Prev;
static DVDBuffer Curr;

static void Read(void*, unsigned long, unsigned long, DVDLowCallback);

void __DVDInitWA(void)
{
    NextCommandNumber = 0;
    CommandList[0].command = -1;
    __DVDLowSetWAType(0, 0);
    OSInitAlarm();
}

static inline int ProcessNextCommand(void)
{
    signed long number = NextCommandNumber;
    if (CommandList[number].command == 1) {
        NextCommandNumber++;
        Read(CommandList[number].address, CommandList[number].length,
             CommandList[number].offset, CommandList[number].callback);
        return 1;
    }
    if (CommandList[number].command == 2) {
        NextCommandNumber++;
        DVDLowSeek(CommandList[number].offset, CommandList[number].callback);
        return 1;
    }
    return 0;
}

void __DVDInterruptHandler(__OSInterrupt interrupt, OSContext* context)
{
    DVDLowCallback callback;
    OSContext exception_context;
    unsigned long cause = 0;
    unsigned long reg;
    unsigned long pending;
    unsigned long mask;

    if (LastCommandWasRead) {
        LastReadFinished = __OSGetSystemTime();
        FirstRead = 0;
        Prev = Curr;
        if (StopAtNextInt) cause |= 8;
    }
    LastCommandWasRead = 0;
    StopAtNextInt = 0;
    reg = __DIRegs[0];
    mask = reg & 0x2A;
    pending = (reg & 0x54) & (mask << 1);
    if (pending & 0x40) cause |= 8;
    if (pending & 0x10) cause |= 1;
    if (pending & 4) cause |= 2;
    if (cause) {
        ResetOccurred = 0;
        OSCancelAlarm(&AlarmForTimeout);
    }
    __DIRegs[0] = pending | mask;

    if (ResetOccurred &&
        __OSGetSystemTime() - LastResetEnd < OSMillisecondsToTicks(200)) {
        reg = __DIRegs[1];
        mask = reg & 2;
        pending = (reg & 4) & (mask << 1);
        if ((pending & 4) && ResetCoverCallback) ResetCoverCallback(4);
        if (pending & 4) ResetCoverCallback = 0;
        __DIRegs[1] = __DIRegs[1];
    } else if (WaitingCoverClose) {
        reg = __DIRegs[1];
        mask = reg & 2;
        pending = (reg & 4) & (mask << 1);
        if (pending & 4) cause |= 4;
        __DIRegs[1] = pending | mask;
        WaitingCoverClose = 0;
    } else {
        __DIRegs[1] = 0;
    }
    if ((cause & 8) && !Breaking) cause &= ~8;
    if (cause & 1) {
        if (ProcessNextCommand()) return;
    } else {
        CommandList[0].command = -1;
        NextCommandNumber = 0;
    }

    OSClearContext(&exception_context);
    OSSetCurrentContext(&exception_context);
    if (cause) {
        callback = Callback;
        Callback = 0;
        if (callback) callback(cause);
        Breaking = 0;
    }
    OSClearContext(&exception_context);
    OSSetCurrentContext(context);
}

static void AlarmHandler(OSAlarm* alarm, OSContext* context)
{
    ProcessNextCommand();
}

static void AlarmHandlerForTimeout(OSAlarm* alarm, OSContext* context)
{
    DVDLowCallback callback;
    OSContext exception_context;
    __OSMaskInterrupts(0x400);
    OSClearContext(&exception_context);
    OSSetCurrentContext(&exception_context);
    callback = Callback;
    Callback = 0;
    if (callback) callback(0x10);
    OSClearContext(&exception_context);
    OSSetCurrentContext(context);
}

static inline void SetTimeoutAlarm(OSTime timeout)
{
    OSCreateAlarm(&AlarmForTimeout);
    OSSetAlarm(&AlarmForTimeout, timeout, AlarmHandlerForTimeout);
}

static void Read(void* address, unsigned long length, unsigned long offset,
                 DVDLowCallback callback)
{
    Callback = callback;
    StopAtNextInt = 0;
    LastCommandWasRead = 1;
    LastReadIssued = __OSGetSystemTime();
    __DIRegs[2] = 0xA8000000;
    __DIRegs[3] = offset / 4;
    __DIRegs[4] = length;
    __DIRegs[5] = (unsigned long)address;
    __DIRegs[6] = length;
    LastLength = length;
    __DIRegs[7] = 3;
    SetTimeoutAlarm(OSSecondsToTicks(length > 0xA00000 ? 20 : 10));
}

static inline int AudioBufferOn(void)
{
    return DVDGetCurrentDiskID()->streaming != 0;
}

static inline int HitCache(const DVDBuffer* current, const DVDBuffer* previous)
{
    unsigned long previous_end =
        (previous->offset + previous->length - 1) >> 15;
    unsigned long current_start = current->offset >> 15;
    unsigned long cache_blocks = AudioBufferOn() ? 5 : 15;
    return current_start > previous_end - 2 ||
           current_start < previous_end + cache_blocks + 3;
}

static inline void DoJustRead(void* address, unsigned long length,
                       unsigned long offset, DVDLowCallback callback)
{
    CommandList[0].command = -1;
    NextCommandNumber = 0;
    Read(address, length, offset, callback);
}

static void SeekTwiceBeforeRead(void* address, unsigned long length,
                                unsigned long offset, DVDLowCallback callback)
{
    unsigned long new_offset =
        (offset & ~0x7FFF) == 0 ? 0 :
        (offset & ~0x7FFF) + WorkAroundSeekLocation;
    CommandList[0].command = 2;
    CommandList[0].offset = new_offset;
    CommandList[0].callback = callback;
    CommandList[1].command = 1;
    CommandList[1].address = address;
    CommandList[1].length = length;
    CommandList[1].offset = offset;
    CommandList[1].callback = callback;
    CommandList[2].command = -1;
    NextCommandNumber = 0;
    DVDLowSeek(new_offset, callback);
}

static inline void WaitBeforeRead(void* address, unsigned long length,
                           unsigned long offset, DVDLowCallback callback,
                           OSTime wait)
{
    CommandList[0].command = 1;
    CommandList[0].address = address;
    CommandList[0].length = length;
    CommandList[0].offset = offset;
    CommandList[0].callback = callback;
    CommandList[1].command = -1;
    NextCommandNumber = 0;
    OSCreateAlarm(&AlarmForWA);
    OSSetAlarm(&AlarmForWA, wait, AlarmHandler);
}

int DVDLowRead(void* address, unsigned long length, unsigned long offset,
               DVDLowCallback callback)
{
    unsigned long previous_end;
    unsigned long current_start;
    OSTime elapsed;
    __DIRegs[6] = length;
    Curr.address = address;
    Curr.length = length;
    Curr.offset = offset;
    if (WorkAroundType == 0) {
        DoJustRead(address, length, offset, callback);
    } else if (WorkAroundType == 1) {
        if (FirstRead) {
            SeekTwiceBeforeRead(address, length, offset, callback);
        } else if (!HitCache(&Curr, &Prev)) {
            DoJustRead(address, length, offset, callback);
        } else {
            previous_end = ((Prev.offset + Prev.length - 1) >> 15) & 0x1FFFF;
            current_start = (Curr.offset >> 15) & 0x1FFFF;
            if (previous_end == current_start ||
                previous_end + 1 == current_start) {
                elapsed = __OSGetSystemTime() - LastReadFinished;
                if (elapsed > OSMillisecondsToTicks(5)) {
                    DoJustRead(address, length, offset, callback);
                } else {
                    WaitBeforeRead(address, length, offset, callback,
                        OSMillisecondsToTicks(5) - elapsed +
                        OSMicrosecondsToTicks(500));
                }
            } else {
                SeekTwiceBeforeRead(address, length, offset, callback);
            }
        }
    }
    return 1;
}

int DVDLowSeek(unsigned long offset, DVDLowCallback callback)
{
    Callback = callback; StopAtNextInt = 0;
    __DIRegs[2] = 0xAB000000; __DIRegs[3] = offset / 4; __DIRegs[7] = 1;
    SetTimeoutAlarm(OSSecondsToTicks(10)); return 1;
}

int DVDLowWaitCoverClose(DVDLowCallback callback)
{
    Callback = callback; WaitingCoverClose = 1; StopAtNextInt = 0;
    __DIRegs[1] = 2; return 1;
}

int DVDLowReadDiskID(DVDDiskID* id, DVDLowCallback callback)
{
    Callback = callback; StopAtNextInt = 0;
    __DIRegs[2] = 0xA8000040; __DIRegs[3] = 0;
    __DIRegs[4] = sizeof(DVDDiskID); __DIRegs[5] = (unsigned long)id;
    __DIRegs[6] = sizeof(DVDDiskID); __DIRegs[7] = 3;
    SetTimeoutAlarm(OSSecondsToTicks(10)); return 1;
}

static inline int IssueImmediate(unsigned long command, DVDLowCallback callback)
{
    Callback = callback; StopAtNextInt = 0;
    __DIRegs[2] = command; __DIRegs[7] = 1;
    SetTimeoutAlarm(OSSecondsToTicks(10)); return 1;
}

int DVDLowStopMotor(DVDLowCallback callback)
{ return IssueImmediate(0xE3000000, callback); }
int DVDLowRequestError(DVDLowCallback callback)
{ return IssueImmediate(0xE0000000, callback); }

int DVDLowInquiry(DVDDriveInfo* info, DVDLowCallback callback)
{
    Callback = callback; StopAtNextInt = 0;
    __DIRegs[2] = 0x12000000; __DIRegs[4] = sizeof(DVDDriveInfo);
    __DIRegs[5] = (unsigned long)info; __DIRegs[6] = sizeof(DVDDriveInfo);
    __DIRegs[7] = 3; SetTimeoutAlarm(OSSecondsToTicks(10)); return 1;
}

int DVDLowAudioStream(unsigned long subcommand, unsigned long length,
                      unsigned long offset, DVDLowCallback callback)
{
    Callback = callback; StopAtNextInt = 0;
    __DIRegs[2] = 0xE1000000 | subcommand; __DIRegs[3] = offset >> 2;
    __DIRegs[4] = length; __DIRegs[7] = 1;
    SetTimeoutAlarm(OSSecondsToTicks(10)); return 1;
}

int DVDLowRequestAudioStatus(unsigned long subcommand, DVDLowCallback callback)
{ return IssueImmediate(0xE2000000 | subcommand, callback); }

int DVDLowAudioBufferConfig(int enable, unsigned long size,
                            DVDLowCallback callback)
{ return IssueImmediate(0xE4000000 | (enable ? 0x10000 : 0) | size, callback); }

void DVDLowReset(void)
{
    unsigned long reg;
    OSTime start;
    __DIRegs[1] = 2;
    reg = __PIRegs[9];
    __PIRegs[9] = (reg & ~4) | 1;
    start = __OSGetSystemTime();
    while (__OSGetSystemTime() - start < OSMicrosecondsToTicks(12)) {}
    __PIRegs[9] = reg | 5;
    ResetOccurred = 1;
    LastResetEnd = __OSGetSystemTime();
}

int DVDLowBreak(void) { StopAtNextInt = 1; Breaking = 1; return 1; }

DVDLowCallback DVDLowClearCallback(void)
{
    DVDLowCallback old;
    __DIRegs[1] = 0;
    WaitingCoverClose = 0;
    old = Callback;
    Callback = 0;
    return old;
}

void __DVDLowSetWAType(unsigned long type, signed long seek_location)
{
    int enabled = OSDisableInterrupts();
    WorkAroundType = type;
    WorkAroundSeekLocation = seek_location;
    OSRestoreInterrupts(enabled);
}

int __DVDLowTestAlarm(const OSAlarm* alarm)
{ return alarm == &AlarmForBreak || alarm == &AlarmForTimeout; }
