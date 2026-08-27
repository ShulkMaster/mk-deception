#include "dolphin/circle_buffer.h"
#include "dolphin/debugger_driver.h"
#include "dolphin/trk.h"

#define GDEV_BUFFER_SIZE 0x500
#define GDEV_ERR_NOT_INITIALIZED -0x2711
#define GDEV_ERR_ALREADY_INITIALIZED -0x2715
#define GDEV_ERR_READ_ERROR -0x2719

static CircleBuffer gRecvCB;
static u8 gRecvBuf[GDEV_BUFFER_SIZE];
static BOOL gIsInitialized;

int gdev_cc_initialize(volatile u8** input_pending, EXICallback monitor_callback)
{
    MWTRACE(1, "CALLING EXI2_Init\n");
    DBInitComm(input_pending, monitor_callback);
    MWTRACE(1, "DONE CALLING EXI2_Init\n");
    CircleBufferInitialize(&gRecvCB, gRecvBuf, GDEV_BUFFER_SIZE);
    return 0;
}

int gdev_cc_shutdown(void)
{
    return 0;
}

int gdev_cc_open(void)
{
    if (gIsInitialized) {
        return GDEV_ERR_ALREADY_INITIALIZED;
    }

    gIsInitialized = 1;
    return 0;
}

int gdev_cc_close(void)
{
    return 0;
}

int gdev_cc_read(u8* data, int size)
{
    u8 buffer[GDEV_BUFFER_SIZE];
    int original_size;
    u32 result;
    int expected_size;
    int polled_size;

    result = 0;
    if (!gIsInitialized) {
        return GDEV_ERR_NOT_INITIALIZED;
    }

    MWTRACE(1, "Expected packet size : 0x%08x (%ld)\n", size, size);

    original_size = size;
    expected_size = size;
    while (CBGetBytesAvailableForRead(&gRecvCB) < (u32)expected_size) {
        result = 0;
        polled_size = DBQueryData();
        if (polled_size != 0) {
            result = DBRead(buffer, expected_size);
            if (result == 0) {
                CircleBufferWriteBytes(&gRecvCB, buffer, polled_size);
            }
        }
    }

    if (result == 0) {
        CircleBufferReadBytes(&gRecvCB, data, original_size);
    } else {
        MWTRACE(8, "cc_read : error reading bytes from EXI2 %ld\n", result);
    }

    return result;
}

int gdev_cc_write(const u8* bytes, int length)
{
    int written;
    int remaining;
    const u8* cursor;

    cursor = bytes;
    remaining = length;

    if (!gIsInitialized) {
        MWTRACE(8, "cc not initialized\n");
        return GDEV_ERR_NOT_INITIALIZED;
    }

    MWTRACE(8, "cc_write : Output data 0x%08x %ld bytes\n", bytes, length);

    while (remaining > 0) {
        MWTRACE(1, "cc_write sending %ld bytes\n", remaining);
        written = DBWrite(cursor, remaining);
        if (written == 0) {
            break;
        }
        cursor += written;
        remaining -= written;
    }

    return 0;
}

int gdev_cc_pre_continue(void)
{
    DBClose();
    return 0;
}

int gdev_cc_post_stop(void)
{
    DBOpen();
    return 0;
}

int gdev_cc_peek(void)
{
    int polled_size;
    u8 buffer[GDEV_BUFFER_SIZE];

    polled_size = DBQueryData();
    if (polled_size <= 0) {
        return 0;
    }

    if (DBRead(buffer, polled_size) == 0) {
        CircleBufferWriteBytes(&gRecvCB, buffer, polled_size);
    } else {
        return GDEV_ERR_READ_ERROR;
    }

    return polled_size;
}

int gdev_cc_initinterrupts(void)
{
    DBInitInterrupts();
    return 0;
}
