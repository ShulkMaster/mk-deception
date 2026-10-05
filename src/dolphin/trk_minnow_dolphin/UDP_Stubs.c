#include "dolphin/UDP_Stubs.h"

__declspec(weak) int udp_cc_initialize(volatile u8** flag_out,
                                       EXICallback handler)
{
    return -1;
}

__declspec(weak) int udp_cc_shutdown(void)
{
    return -1;
}

__declspec(weak) int udp_cc_open(void)
{
    return -1;
}

__declspec(weak) int udp_cc_close(void)
{
    return -1;
}

__declspec(weak) int udp_cc_read(u8* destination, int size)
{
    return 0;
}

__declspec(weak) int udp_cc_write(const u8* source, int size)
{
    return 0;
}

__declspec(weak) int udp_cc_peek(void)
{
    return 0;
}

__declspec(weak) int udp_cc_pre_continue(void)
{
    return -1;
}

__declspec(weak) int udp_cc_post_stop(void)
{
    return -1;
}
