#ifndef MKD_RUNTIME_CSTDLIB_H
#define MKD_RUNTIME_CSTDLIB_H

#include "dolphin/types.h"

#ifdef __cplusplus
extern "C" {
#endif

void exit(int code);
void* malloc(size_t size);
void* calloc(size_t count, size_t size);
void* realloc(void* allocation, size_t size);
void free(void* allocation);
void qsort(void* base, size_t count, size_t size,
           int (*compare)(const void*, const void*));

void* __sys_alloc(size_t size);
void __sys_free(void* allocation);

int __abs(int value);
int atoi(const char* str);
unsigned long strtoul(const char* str, char** end, int base);
unsigned long __strtoul(
    int base, int max_width, int (*read_proc)(void*, int, int),
    void* read_context, int* chars_scanned, int* negative, int* overflow);
unsigned long long __strtoull(
    int base, int max_width, int (*read_proc)(void*, int, int),
    void* read_context, int* chars_scanned, int* negative, int* overflow);

#ifdef __cplusplus
}
#endif

#endif
