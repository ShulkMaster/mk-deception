#include "platform/gcInit.h"

#include "mw/mwMem.h"
#include "mw/mwMemHeap.h"
#include "platform/display.h"

extern void DVDInit(void);
extern void init_debug_timers(void);

extern RwMemoryFunctions mem_funcs;
extern int debug_message_handler_set;
extern int screen_height;
extern int screen_width;

static void* _rwDolphinHeapRealloc(void* memory, unsigned long size,
                                    unsigned int hint) {
    return _mwMemRealloc(memory, permanent_heap, size, 3, 0, 0, 0);
}

static void* _rwDolphinHeapCalloc(unsigned long count, unsigned long size,
                                  unsigned int hint) {
    return _mwMemCalloc(permanent_heap, count, size, 3, 0, 0, 0);
}

static void* _rwDolphinHeapAlloc(unsigned long size, unsigned int hint) {
    return _mwMemMalloc(permanent_heap, size, 3, 0, 0, 0);
}

static void _rwDolphinHeapFree(void* memory) {
    _mwMemFree(memory, 0, 0);
}

void setup_memory_functions(void) {
    mem_funcs.alloc = _rwDolphinHeapAlloc;
    mem_funcs.calloc = _rwDolphinHeapCalloc;
    mem_funcs.realloc = _rwDolphinHeapRealloc;
    mem_funcs.free = _rwDolphinHeapFree;
}

int select_display_device(void) {
    return 1;
}

void init_debug_message_handler(void) {
    debug_message_handler_set = 1;
}

int hardware_init(void) {
    DVDInit();
    screen_width = 640;
    screen_height = 480;
    init_debug_timers();
    return 1;
}

void free(void* memory) {
    _mwMemFree(memory, 0, 0);
}

void* malloc(unsigned long size) {
    return _mwMemMalloc(permanent_heap, size, 3, 0, 0, 0);
}
