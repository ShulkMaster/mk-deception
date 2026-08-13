#ifndef PLATFORM_GCINIT_H
#define PLATFORM_GCINIT_H

#include "rw/rwdevice.h"

extern RwMemoryFunctions mem_funcs;

RwMemoryFunctions* setup_memory_functions(void);
int select_display_device(void);
void init_debug_message_handler(void);
int hardware_init(void);

#endif
