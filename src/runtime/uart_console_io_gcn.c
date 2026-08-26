#include "dolphin/os.h"

typedef void (*IdleProc)(void);

int InitializeUART(unsigned long baudRate);
int WriteUARTN(const void* buffer, unsigned long length);
int __TRK_write_console(unsigned long file, unsigned char* buffer,
                        unsigned long* count, IdleProc idleProc);

static inline int __init_uart_console(void)
{
    static int initialized;
    int result = 0;

    if (initialized == 0) {
        result = InitializeUART(0xE100);
        if (result == 0) {
            initialized = 1;
        }
    }

    return result;
}

__declspec(weak) int __write_console(unsigned long file, unsigned char* buffer,
                                     unsigned long* count, IdleProc idleProc)
{
    if ((OSGetConsoleType() & 0x20000000) == 0) {
        if (__init_uart_console() != 0) {
            return 1;
        }
        if (WriteUARTN(buffer, *count) != 0) {
            *count = 0;
            return 1;
        }
    }

    __TRK_write_console(file, buffer, count, idleProc);
    return 0;
}
