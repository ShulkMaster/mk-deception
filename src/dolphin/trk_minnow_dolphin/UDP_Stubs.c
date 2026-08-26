#include "dolphin/trk.h"
typedef void (*InterruptHandler)(int interrupt, void* context);

__declspec(weak) int udp_cc_initialize(void* flag_out, InterruptHandler handler)
{
    (void)flag_out;
    (void)handler;
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
    (void)destination;
    (void)size;
    return 0;
}

__declspec(weak) int udp_cc_write(const u8* source, int size)
{
    (void)source;
    (void)size;
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
