#include "runtime/critical_regions.h"

typedef void (*ExitFunction)(void);

extern void __destroy_global_chain(void);
extern void _ExitProcess(void);
extern int raise(int signal);
extern ExitFunction _dtors[];

ExitFunction __atexit_funcs[64];
ExitFunction __console_exit;
ExitFunction __stdio_exit;
int __atexit_curr_func;
int __aborting;

void exit(int status)
{
    ExitFunction* destructor;

    if (!__aborting) {
        __begin_critical_region(0);
        __end_critical_region(0);
        __destroy_global_chain();

        destructor = _dtors;
        while (*destructor != 0) {
            (*destructor)();
            ++destructor;
        }
        if (__stdio_exit != 0) {
            __stdio_exit();
            __stdio_exit = 0;
        }
    }

    __begin_critical_region(0);
    while (__atexit_curr_func > 0)
        __atexit_funcs[--__atexit_curr_func]();
    __end_critical_region(0);
    __kill_critical_regions();

    if (__console_exit != 0) {
        __console_exit();
        __console_exit = 0;
    }
    _ExitProcess();
}

void abort(void)
{
    raise(1);
    __aborting = 1;
    __begin_critical_region(0);
    while (__atexit_curr_func > 0)
        __atexit_funcs[--__atexit_curr_func]();
    __end_critical_region(0);
    __kill_critical_regions();

    if (__console_exit != 0) {
        __console_exit();
        __console_exit = 0;
    }
    _ExitProcess();
}
