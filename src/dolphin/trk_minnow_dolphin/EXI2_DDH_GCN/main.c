#include "dolphin/amc_exi2.h"
#include "dolphin/circle_buffer.h"
#include "dolphin/trk.h"

#define DDH_BUFFER_SIZE 0x800
#define DDH_ERR_NOT_INITIALIZED -0x2711
#define DDH_ERR_ALREADY_INITIALIZED -0x2715
#define DDH_ERR_READ_ERROR -0x2719

static CircleBuffer gRecvCB;
static u8 gRecvBuf[DDH_BUFFER_SIZE];
static BOOL gIsInitialized;

int ddh_cc_initialize(volatile u8** input_pending, EXICallback monitor_callback)
{
    MWTRACE(1, "CALLING EXI2_Init\n");
    EXI2_Init(input_pending, monitor_callback);
    MWTRACE(1, "DONE CALLING EXI2_Init\n");
    CircleBufferInitialize(&gRecvCB, gRecvBuf, DDH_BUFFER_SIZE);
    return 0;
}

int ddh_cc_shutdown(void)
{
    return 0;
}

int ddh_cc_open(void)
{
    if (gIsInitialized) {
        return DDH_ERR_ALREADY_INITIALIZED;
    }

    gIsInitialized = 1;
    return 0;
}

int ddh_cc_close(void)
{
    return 0;
}

int ddh_cc_read(u8* data, int size)
{
    u8 buffer[DDH_BUFFER_SIZE];
    int original_size;
    u32 result;
    int expected_size;
    int polled_size;

    result = 0;
    if (!gIsInitialized) {
        return DDH_ERR_NOT_INITIALIZED;
    }

    MWTRACE(1, "Expected packet size : 0x%08x (%ld)\n", size, size);

    original_size = expected_size = size;
    while (CBGetBytesAvailableForRead(&gRecvCB) < (u32)expected_size) {
        result = 0;
        polled_size = EXI2_Poll();
        if (polled_size != 0) {
            result = EXI2_ReadN(buffer, polled_size);
            if (result == AMC_EXI_NO_ERROR) {
                CircleBufferWriteBytes(&gRecvCB, buffer, polled_size);
            }
        }
    }

    if (result == AMC_EXI_NO_ERROR) {
        CircleBufferReadBytes(&gRecvCB, data, original_size);
    } else {
        MWTRACE(8, "cc_read : error reading bytes from EXI2 %ld\n", result);
    }

    return result;
}

int ddh_cc_write(const u8* bytes, int length)
{
    int written;
    int remaining;
    const u8* cursor;

    cursor = bytes;
    remaining = length;

    if (!gIsInitialized) {
        MWTRACE(8, "cc not initialized\n");
        return DDH_ERR_NOT_INITIALIZED;
    }

    MWTRACE(8, "cc_write : Output data 0x%08x %ld bytes\n", bytes, length);

    while (remaining > 0) {
        MWTRACE(1, "cc_write sending %ld bytes\n", remaining);
        written = EXI2_WriteN(cursor, remaining);
        if (written == AMC_EXI_NO_ERROR) {
            break;
        }
        cursor += written;
        remaining -= written;
    }

    return 0;
}

int ddh_cc_pre_continue(void)
{
    EXI2_Unreserve();
    return 0;
}

int ddh_cc_post_stop(void)
{
    EXI2_Reserve();
    return 0;
}

int ddh_cc_peek(void)
{
    int polled_size;
    u8 buffer[DDH_BUFFER_SIZE];

    polled_size = EXI2_Poll();
    if (polled_size <= 0) {
        return 0;
    }

    if (EXI2_ReadN(buffer, polled_size) == AMC_EXI_NO_ERROR) {
        CircleBufferWriteBytes(&gRecvCB, buffer, polled_size);
    } else {
        return DDH_ERR_READ_ERROR;
    }

    return polled_size;
}

int ddh_cc_initinterrupts(void)
{
    EXI2_EnableInterrupts();
    return 0;
}
