#ifndef SCRIPT_FUNCTIONS_H
#define SCRIPT_FUNCTIONS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*ScriptEntryFn)(void);

extern ScriptEntryFn script_callable_function_table[];
extern int number_of_script_functions;

#ifdef __cplusplus
}
#endif

#endif
