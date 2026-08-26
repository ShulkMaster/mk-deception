#include "dolphin/trk.h"

typedef union EndianTest {
    u8 bytes[4];
    u32 word;
} EndianTest;

enum {
    FALSE = 0,
    TRUE = 1,
    DS_NoError = 0,
};

extern void TRK_board_display(const char* message);
extern DSError TRKInitializeMessageBuffers(void);
extern DSError InitializeProgramEndTrap(void);
extern DSError TRKInitializeTarget(void);
extern DSError TRKInitializeIntDrivenUART(u32 address, u32 channel, u32 unused,
                                         volatile u8** input_pending_ptr);
extern void TRKTargetSetInputPendingPtr(volatile u8* input_pending_ptr);

BOOL gTRKBigEndian;

static const char welcome_message[] = "MetroTRK for GAMECUBE v2.6";
static const char initialize_message[] = "Initialize NUB\n";

static inline BOOL TRKInitializeEndian(void);

DSError TRKInitializeNub(void)
{
    DSError error;
    DSError uart_error;

    error = TRKInitializeEndian();

    MWTRACE(1, initialize_message);
    if (error == DS_NoError) {
        usr_put_initialize();
    }
    if (error == DS_NoError) {
        error = TRKInitializeEventQueue();
    }
    if (error == DS_NoError) {
        error = TRKInitializeMessageBuffers();
    }
    if (error == DS_NoError) {
        error = TRKInitializeDispatcher();
    }
    InitializeProgramEndTrap();
    if (error == DS_NoError) {
        error = TRKInitializeSerialHandler();
    }
    if (error == DS_NoError) {
        error = TRKInitializeTarget();
    }
    if (error == DS_NoError) {
        uart_error = TRKInitializeIntDrivenUART(0xE100, 1, 0, &gTRKInputPendingPtr);
        TRKTargetSetInputPendingPtr(gTRKInputPendingPtr);
        if (uart_error != DS_NoError) {
            error = uart_error;
        }
    }
    return error;
}

DSError TRKTerminateNub(void)
{
    TRKTerminateSerialHandler();
    return DS_NoError;
}

void TRKNubWelcome(void)
{
    TRK_board_display(welcome_message);
}

static inline BOOL TRKInitializeEndian(void)
{
    EndianTest endian_test;
    BOOL error = FALSE;

    gTRKBigEndian = TRUE;

    endian_test.bytes[0] = 0x12;
    endian_test.bytes[1] = 0x34;
    endian_test.bytes[2] = 0x56;
    endian_test.bytes[3] = 0x78;

    if (endian_test.word == 0x12345678) {
        gTRKBigEndian = TRUE;
    } else if (endian_test.word == 0x78563412) {
        gTRKBigEndian = FALSE;
    } else {
        error = TRUE;
    }
    return error;
}
