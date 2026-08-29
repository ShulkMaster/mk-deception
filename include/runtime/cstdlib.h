#ifndef MKD_RUNTIME_CSTDLIB_H
#define MKD_RUNTIME_CSTDLIB_H

#ifdef __cplusplus
extern "C" {
#endif

void exit(int code);
void* malloc(unsigned long size);
void* calloc(unsigned long count, unsigned long size);
void* realloc(void* allocation, unsigned long size);
void free(void* allocation);
void qsort(void* base, unsigned long count, unsigned long size,
           int (*compare)(const void*, const void*));

/* Platform hooks used by the MSL allocator implementation. */
void* __sys_alloc(unsigned long size);
void __sys_free(void* allocation);

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
