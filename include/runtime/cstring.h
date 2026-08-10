#ifndef MKD_RUNTIME_CSTRING_H
#define MKD_RUNTIME_CSTRING_H

#ifdef __cplusplus
extern "C" {
#endif

void* memcpy(void* destination, const void* source, unsigned long size);
void* memset(void* destination, int value, unsigned long size);
int memcmp(const void* lhs, const void* rhs, unsigned long size);
char* strcpy(char* destination, const char* source);
char* strncpy(char* destination, const char* source, unsigned long count);
char* strcat(char* destination, const char* source);
char* strncat(char* destination, const char* source, unsigned long count);
char* strchr(const char* string, int character);
char* strrchr(const char* string, int character);
char* strstr(const char* string, const char* substring);
unsigned long strlen(const char* string);
int strcmp(const char* lhs, const char* rhs);
int strncmp(const char* lhs, const char* rhs, unsigned long count);
int stricmp(const char* lhs, const char* rhs);
int strnicmp(const char* lhs, const char* rhs, unsigned long count);
char* strtok(char* string, const char* delimiters);
char* strupr(char* string);

#ifdef __cplusplus
}
#endif

#endif
