#ifndef DOLPHIN_TRK_H
#define DOLPHIN_TRK_H

#include "dolphin/types.h"

typedef int DSError;
typedef int MessageBufferID;
typedef int MessageCommandID;
typedef int DSIOResult;

typedef enum TRKFileCommand {
    TRK_MSG_WRITE_FILE = 0xD0,
    TRK_MSG_READ_FILE = 0xD1,
    TRK_MSG_OPEN_FILE = 0xD2,
    TRK_MSG_CLOSE_FILE = 0xD3,
    TRK_MSG_POSITION_FILE = 0xD4
} TRKFileCommand;

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
    u32 gpr[32];
    u32 lr;
    u32 ctr;
    u32 xer;
    u32 msr;
    u32 dar;
    u32 dsisr;
    u32 stopped;
    BOOL input_activated;
    volatile u8* input_pending;
} TRKTargetState;

typedef struct TRKDefaultRegisters {
    u32 gpr[32];
    u32 pc;
    u32 lr;
    u32 cr;
    u32 ctr;
    u32 xer;
} TRKDefaultRegisters;

typedef struct TRKFloatRegisters {
    u64 fpr[32];
    u64 fpscr;
    u64 fpecr;
} TRKFloatRegisters;

typedef struct TRKExtended1Registers {
    u32 sr[16];
    u32 tbl;
    u32 tbu;
    u32 hid0;
    u32 hid1;
    u32 msr;
    u32 pvr;
    u32 ibat0u;
    u32 ibat0l;
    u32 ibat1u;
    u32 ibat1l;
    u32 ibat2u;
    u32 ibat2l;
    u32 ibat3u;
    u32 ibat3l;
    u32 dbat0u;
    u32 dbat0l;
    u32 dbat1u;
    u32 dbat1l;
    u32 dbat2u;
    u32 dbat2l;
    u32 dbat3u;
    u32 dbat3l;
    u32 dmiss;
    u32 dcmp;
    u32 hash1;
    u32 hash2;
    u32 imiss;
    u32 icmp;
    u32 rpa;
    u32 sdr1;
    u32 dar;
    u32 dsisr;
    u32 sprg0;
    u32 sprg1;
    u32 sprg2;
    u32 sprg3;
    u32 dec;
    u32 iabr;
    u32 ear;
    u32 dabr;
    u32 pmc1;
    u32 pmc2;
    u32 pmc3;
    u32 pmc4;
    u32 sia;
    u32 mmcr0;
    u32 mmcr1;
    u32 thrm1;
    u32 thrm2;
    u32 thrm3;
    u32 ictc;
    u32 l2cr;
    u32 ummcr2;
    u32 ubamr;
    u32 ummcr0;
    u32 upmc1;
    u32 upmc2;
    u32 usia;
    u32 ummcr1;
    u32 upmc3;
    u32 upmc4;
    u32 usda;
    u32 mmcr2;
    u32 bamr;
    u32 sda;
    u32 msscr0;
    u32 msscr1;
    u32 pir;
    u32 exception_id;
    u32 gqr[8];
    u32 hid_g;
    u32 wpar;
    u32 dma_u;
    u32 dma_l;
} TRKExtended1Registers;

typedef struct TRKExtended2Registers {
    u32 psr[32][2];
} TRKExtended2Registers;

typedef struct TRKCPUState {
    TRKDefaultRegisters default_registers;
    TRKFloatRegisters float_registers;
    TRKExtended1Registers extended1;
    TRKExtended2Registers extended2;
    u32 transport_handler_saved_ra;
} TRKCPUState;

typedef struct TRKStopInfo {
    u32 pc;
    u32 instruction;
    u16 exception_id;
} TRKStopInfo;

typedef struct TRKExceptionStatus {
    TRKStopInfo exception_info;
    u8 in_trk;
    u8 exception_detected;
} TRKExceptionStatus;

typedef char MessageBufferSizeCheck[sizeof(MessageBuffer) == 0x890 ? 1 : -1];
typedef char TRKEventSizeCheck[sizeof(TRKEvent) == 0xC ? 1 : -1];
typedef char TRKCPUStateSizeCheck[sizeof(TRKCPUState) == 0x430 ? 1 : -1];
typedef char TRKExceptionStatusSizeCheck[sizeof(TRKExceptionStatus) == 0x10 ? 1 : -1];
typedef char TRKTargetStateSizeCheck[
    sizeof(TRKTargetState) == 0xA4 ? 1 : -1];

extern BOOL gTRKBigEndian;
extern TRKTargetState gTRKState;
extern TRKCPUState gTRKCPUState;
extern u8 gTRKInterruptVectorTable[];

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
void TRK_flush_cache(u32 address, u32 size);
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
DSError TRKRequestSend(MessageBuffer* request, MessageBufferID* reply_id,
                       int retries, int timeout, BOOL blocking);
MessageBufferID TRKTestForPacket(void);
DSError TRKSuppAccessFile(u32 file_handle, u8* data, u32* count,
                          DSIOResult* io_result, BOOL need_reply, BOOL read);
DSError HandleOpenFileSupportRequest(const char* path, u8 mode, u32* handle,
                                     DSIOResult* io_result);
DSError HandleCloseFileSupportRequest(u32 handle, DSIOResult* io_result);
DSError HandlePositionFileSupportRequest(u32 handle, u32* position, u8 mode,
                                         DSIOResult* io_result);
DSError TRKDoNotifyStopped(MessageCommandID command);
u32 TRKTargetTranslate(u32 address);
void TRK__read_aram(void* buffer, u32 aram_address, u32* length);
void TRK__write_aram(void* buffer, u32 aram_address, u32* length);
void TRKProcessInput(int buffer_id);
void OutputData(const u8* data, s32 length);

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
DSError TRKTargetStop(void);
BOOL TRKTargetStopped(void);
u32 TRKTargetGetPC(void);
DSError TRKTargetSingleStep(u32 count, BOOL step_over);
DSError TRKTargetStepOutOfRange(u32 range_start, u32 range_end,
                                BOOL step_over);

void usr_put_initialize(void);
BOOL usr_puts_serial(const char* message);

void SetUseSerialIO(u8 serial_io);
u8 GetUseSerialIO(void);

extern volatile u8* gTRKInputPendingPtr;

#endif
