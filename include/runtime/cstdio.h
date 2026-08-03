#ifndef MKD_RUNTIME_CSTDIO_H
#define MKD_RUNTIME_CSTDIO_H

#include "runtime/cstdarg.h"

#ifdef __cplusplus
extern "C" {
#endif

int sprintf(char* output, const char* format, ...);
int vsprintf(char* output, const char* format, __va_list args);
int snprintf(char* output, unsigned long size, const char* format, ...);

#ifdef __cplusplus
}
#endif

#endif
