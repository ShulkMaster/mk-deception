#ifndef MKD_RUNTIME_CSTDLIB_H
#define MKD_RUNTIME_CSTDLIB_H

#ifdef __cplusplus
extern "C" {
#endif

void exit(int code);
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
