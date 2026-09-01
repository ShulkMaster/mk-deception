#ifndef DOLPHIN_TRK_H
#define DOLPHIN_TRK_H

#include "dolphin/types.h"

typedef int DSError;
typedef int MessageBufferID;
typedef int MessageCommandID;

typedef struct MessageBuffer {
    u32 mutex;
    BOOL is_in_use;
    u32 length;
    u32 position;
    u8 data[0x880];
} MessageBuffer;

typedef struct TRKEvent {
    int event_type;
    u32 event_id;
    MessageBufferID message_buffer_id;
} TRKEvent;

typedef struct TRKTargetState {
    u8 field_0x00[0x8C];
    u32 saved_msr;
    u8 field_0x90[8];
    u32 stopped;
    u8 input_activated;
    u8 field_0x9D[3];
    volatile u8* input_pending;
} TRKTargetState;

typedef struct TRKCPUState {
    u8 field_0x00[0x80];
    u32 pc;
    u8 field_0x84[0x1B4];
    u32 extended1_state;
    u8 field_0x23C[0xBC];
    u16 exception_id;
} TRKCPUState;

typedef struct TRKExceptionStatus {
    u32 pc;
    u32 field_0x04;
    u16 exception_id;
    u8 field_0x0A[6];
} TRKExceptionStatus;

typedef char MessageBufferSizeCheck[sizeof(MessageBuffer) == 0x890 ? 1 : -1];
typedef char TRKEventSizeCheck[sizeof(TRKEvent) == 0xC ? 1 : -1];

extern BOOL gTRKBigEndian;
extern TRKTargetState gTRKState;
extern TRKCPUState gTRKCPUState;
extern TRKExceptionStatus gTRKExceptionStatus;

void MWTRACE(int level, const char* format, ...);

DSError TRKInitializeMutex(void* mutex);
DSError TRKAcquireMutex(void* mutex);
DSError TRKReleaseMutex(void* mutex);

DSError TRKInitializeEventQueue(void);
BOOL TRKGetNextEvent(TRKEvent* event);
DSError TRKPostEvent(TRKEvent* event);
void TRKConstructEvent(TRKEvent* event, int event_type);
void TRKDestructEvent(TRKEvent* event);

DSError TRKGetFreeBuffer(MessageBufferID* buffer_id, MessageBuffer** buffer);
MessageBuffer* TRKGetBuffer(MessageBufferID buffer_id);
void TRKReleaseBuffer(MessageBufferID buffer_id);
void* TRK_memcpy(void* destination, const void* source, u32 size);
void* TRK_memset(void* destination, int value, u32 size);
DSError TRKInitializeMessageBuffers(void);
void TRKResetBuffer(MessageBuffer* buffer, BOOL keep_data);
DSError TRKSetBufferPosition(MessageBuffer* buffer, u32 position);
DSError TRKAppendBuffer(MessageBuffer* buffer, const void* data, u32 length);
DSError TRKReadBuffer(MessageBuffer* buffer, void* data, u32 length);
DSError TRKAppendBuffer_ui8(MessageBuffer* buffer, const u8* data, int count);
DSError TRKAppendBuffer_ui32(MessageBuffer* buffer, const u32* data, int count);
DSError TRKAppendBuffer1_ui64(MessageBuffer* buffer, u64 value);
DSError TRKReadBuffer_ui8(MessageBuffer* buffer, u8* data, int count);
DSError TRKReadBuffer_ui32(MessageBuffer* buffer, u32* data, int count);
DSError TRKReadBuffer1_ui64(MessageBuffer* buffer, u64* value);
DSError TRKMessageSend(MessageBuffer* message);

DSError TRKInitializeDispatcher(void);
DSError TRKDispatchMessage(MessageBuffer* message);

DSError TRKInitializeSerialHandler(void);
DSError TRKTerminateSerialHandler(void);
void TRKGetInput(void);

DSError TRKInitializeNub(void);
DSError TRKTerminateNub(void);
void TRKNubWelcome(void);
void TRK_board_display(const char* message);
void InitializeProgramEndTrap(void);
int TRKInitializeTarget(void);
DSError TRKInitializeIntDrivenUART(u32 address, u32 channel, u32 unused,
                                   volatile u8** input_pending_ptr);
void TRKTargetSetInputPendingPtr(volatile u8* input_pending_ptr);

DSError TRKWriteUARTN(const void* data, u32 length);
u32 GetTRKConnected(void);
void SetTRKConnected(u32 connected);
void TRKNubMainLoop(void);
DSError TRKTargetContinue(void);

void usr_put_initialize(void);
BOOL usr_puts_serial(const char* message);

void SetUseSerialIO(u8 serial_io);
u8 GetUseSerialIO(void);

extern volatile u8* gTRKInputPendingPtr;

#endif
