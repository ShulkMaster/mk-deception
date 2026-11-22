#ifndef MKD_RUNTIME_CSTRING_H
#define MKD_RUNTIME_CSTRING_H

#include "dolphin/types.h"

#ifdef __cplusplus
extern "C" {
#endif

void* memcpy(void* destination, const void* source, size_t size);
void* memset(void* destination, int value, unsigned int size);
int memcmp(const void* lhs, const void* rhs, size_t size);
char* strcpy(char* destination, const char* source);
char* strncpy(char* destination, const char* source, size_t count);
char* strcat(char* destination, const char* source);
char* strncat(char* destination, const char* source, size_t count);
char* strchr(const char* string, int character);
char* strrchr(const char* string, int character);
char* strstr(const char* string, const char* substring);
size_t strlen(const char* string);
int strcmp(const char* lhs, const char* rhs);
int strncmp(const char* lhs, const char* rhs, size_t count);
int stricmp(const char* lhs, const char* rhs);
int strnicmp(const char* lhs, const char* rhs, size_t count);
char* strtok(char* string, const char* delimiters);
char* strupr(char* string);

#ifdef __cplusplus
}
#endif

#endif
