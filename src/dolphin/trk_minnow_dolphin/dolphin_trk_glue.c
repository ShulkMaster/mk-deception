#include "dolphin/amc_exi2.h"
#include "dolphin/base/PPCArch.h"
#include "dolphin/cache.h"
#include "dolphin/exi.h"
#include "dolphin/os.h"
#include "dolphin/trk.h"

typedef int (*DBInitializeFn)(volatile u8** input_pending,
                              EXICallback callback);
typedef int (*DBControlFn)(void);
typedef int (*DBReadFn)(u8* destination, int size);
typedef int (*DBWriteFn)(const u8* source, int size);

typedef struct DBCommTable {
    DBInitializeFn initialize;
    DBControlFn initialize_interrupts;
    DBControlFn shutdown;
    DBControlFn peek;
    DBReadFn read;
    DBWriteFn write;
    DBControlFn open;
    DBControlFn close;
    DBControlFn pre_continue;
    DBControlFn post_stop;
} DBCommTable;

int Hu_IsStub(void);
int ddh_cc_initialize(volatile u8** input_pending, EXICallback callback);
int ddh_cc_shutdown(void);
int ddh_cc_open(void);
int ddh_cc_close(void);
int ddh_cc_read(u8* destination, int size);
int ddh_cc_write(const u8* source, int size);
int ddh_cc_peek(void);
int ddh_cc_pre_continue(void);
int ddh_cc_post_stop(void);
int ddh_cc_initinterrupts(void);
int gdev_cc_initialize(volatile u8** input_pending, EXICallback callback);
int gdev_cc_shutdown(void);
int gdev_cc_open(void);
int gdev_cc_close(void);
int gdev_cc_read(u8* destination, int size);
int gdev_cc_write(const u8* source, int size);
int gdev_cc_peek(void);
int gdev_cc_pre_continue(void);
int gdev_cc_post_stop(void);
int gdev_cc_initinterrupts(void);
int udp_cc_initialize(volatile u8** input_pending, EXICallback callback);
int udp_cc_shutdown(void);
int udp_cc_open(void);
int udp_cc_close(void);
int udp_cc_read(u8* destination, int size);
int udp_cc_write(const u8* source, int size);
int udp_cc_peek(void);
int udp_cc_pre_continue(void);
int udp_cc_post_stop(void);

/* Handwritten privileged context restore; tracked in asm.md. */
void TRKLoadContext(OSContext* context, u32 exception_id);

DBCommTable gDBCommTable;
u8 TRK_Use_BBA;

static const u32 EndofProgramInstruction = 0x00454E44;

void TRKUARTInterruptHandler(void)
{
}

void InitializeProgramEndTrap(void)
{
    void* trap_address = (u8*)PPCHalt + 4;

    TRK_memcpy(trap_address, &EndofProgramInstruction, 4);
    ICInvalidateRange(trap_address, 4);
    DCFlushRange(trap_address, 4);
}

void TRK_board_display(const char* message)
{
    OSReport("%s\n", message);
}

void UnreserveEXI2Port(void)
{
    gDBCommTable.pre_continue();
}

void ReserveEXI2Port(void)
{
    gDBCommTable.post_stop();
}

DSError TRKWriteUARTN(const void* source, u32 size)
{
    return gDBCommTable.write((const u8*)source, size) == 0 ? 0 : -1;
}

int TRKReadUARTN(u8* destination, int size)
{
    return gDBCommTable.read(destination, size) == 0 ? 0 : -1;
}

int TRKPollUART(void)
{
    return gDBCommTable.peek();
}

void EnableEXI2Interrupts(void)
{
    if (!TRK_Use_BBA && gDBCommTable.initialize_interrupts != 0) {
        gDBCommTable.initialize_interrupts();
    }
}

void TRKEXICallBack(signed long interrupt, OSContext* context);

DSError TRKInitializeIntDrivenUART(u32 address, u32 channel, u32 unused,
                                   volatile u8** input_pending)
{
    (void)address;
    (void)channel;
    (void)unused;
    gDBCommTable.initialize(input_pending, TRKEXICallBack);
    gDBCommTable.open();
    return 0;
}

int InitMetroTRKCommTable(int hardware_id)
{
    int result = 1;

    OSReport("Devkit set to : %ld\n", hardware_id);
    TRK_Use_BBA = 0;

    switch (hardware_id) {
    case 2:
        OSReport("MetroTRK : Set to BBA\n");
        TRK_Use_BBA = 1;
        gDBCommTable.initialize = udp_cc_initialize;
        gDBCommTable.open = udp_cc_open;
        gDBCommTable.close = udp_cc_close;
        gDBCommTable.read = udp_cc_read;
        gDBCommTable.write = udp_cc_write;
        gDBCommTable.shutdown = udp_cc_shutdown;
        gDBCommTable.peek = udp_cc_peek;
        gDBCommTable.pre_continue = udp_cc_pre_continue;
        gDBCommTable.post_stop = udp_cc_post_stop;
        gDBCommTable.initialize_interrupts = 0;
        return 0;
    case 1:
        OSReport("MetroTRK : Set to GDEV hardware\n");
        result = Hu_IsStub();
        gDBCommTable.initialize = gdev_cc_initialize;
        gDBCommTable.open = gdev_cc_open;
        gDBCommTable.close = gdev_cc_close;
        gDBCommTable.read = gdev_cc_read;
        gDBCommTable.write = gdev_cc_write;
        gDBCommTable.shutdown = gdev_cc_shutdown;
        gDBCommTable.peek = gdev_cc_peek;
        gDBCommTable.pre_continue = gdev_cc_pre_continue;
        gDBCommTable.post_stop = gdev_cc_post_stop;
        gDBCommTable.initialize_interrupts = gdev_cc_initinterrupts;
        break;
    case 0:
        OSReport("MetroTRK : Set to AMC DDH hardware\n");
        result = AMC_IsStub();
        gDBCommTable.initialize = ddh_cc_initialize;
        gDBCommTable.open = ddh_cc_open;
        gDBCommTable.close = ddh_cc_close;
        gDBCommTable.read = ddh_cc_read;
        gDBCommTable.write = ddh_cc_write;
        gDBCommTable.shutdown = ddh_cc_shutdown;
        gDBCommTable.peek = ddh_cc_peek;
        gDBCommTable.pre_continue = ddh_cc_pre_continue;
        gDBCommTable.post_stop = ddh_cc_post_stop;
        gDBCommTable.initialize_interrupts = ddh_cc_initinterrupts;
        break;
    default:
        OSReport("MetroTRK : Set to UNKNOWN hardware. (%ld)\n", hardware_id);
        OSReport("MetroTRK : Invalid hardware ID passed from OS\n");
        OSReport("MetroTRK : Defaulting to GDEV Hardware\n");
        break;
    }
    return result;
}

void TRKEXICallBack(signed long interrupt, OSContext* context)
{
    (void)interrupt;
    OSEnableScheduler();
    TRKLoadContext(context, 0x500);
}
