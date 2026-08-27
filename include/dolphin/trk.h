#ifndef DOLPHIN_TRK_H
#define DOLPHIN_TRK_H

typedef unsigned char u8;
typedef unsigned long u32;
typedef int BOOL;
typedef int DSError;
typedef int MessageBufferID;
typedef int MessageCommandID;

typedef struct MessageBuffer {
    u32 field_0x0;
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

typedef char MessageBufferSizeCheck[sizeof(MessageBuffer) == 0x890 ? 1 : -1];
typedef char TRKEventSizeCheck[sizeof(TRKEvent) == 0xC ? 1 : -1];

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
void* TRKGetBuffer(MessageBufferID buffer_id);
void TRKReleaseBuffer(MessageBufferID buffer_id);
void TRKResetBuffer(MessageBuffer* buffer, BOOL keep_data);
DSError TRKSetBufferPosition(MessageBuffer* buffer, u32 position);
DSError TRKAppendBuffer(MessageBuffer* buffer, const void* data, u32 length);
DSError TRKReadBuffer(MessageBuffer* buffer, void* data, u32 length);
DSError TRKAppendBuffer_ui8(MessageBuffer* buffer, const u8* data, int count);
DSError TRKAppendBuffer_ui32(MessageBuffer* buffer, const u32* data, int count);
DSError TRKReadBuffer_ui8(MessageBuffer* buffer, u8* data, int count);
DSError TRKReadBuffer_ui32(MessageBuffer* buffer, u32* data, int count);
DSError TRKMessageSend(MessageBuffer* message);

DSError TRKInitializeDispatcher(void);
DSError TRKDispatchMessage(MessageBuffer* message);

DSError TRKInitializeSerialHandler(void);
DSError TRKTerminateSerialHandler(void);
void TRKGetInput(void);

DSError TRKInitializeNub(void);
DSError TRKTerminateNub(void);
void TRKNubWelcome(void);
void TRKNubMainLoop(void);
DSError TRKTargetContinue(void);

void usr_put_initialize(void);
BOOL usr_puts_serial(const char* message);

void SetUseSerialIO(u8 serial_io);
u8 GetUseSerialIO(void);

extern volatile u8* gTRKInputPendingPtr;

#endif
