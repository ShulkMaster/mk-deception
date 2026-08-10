#ifndef MKD_RUNTIME_CFILE_H
#define MKD_RUNTIME_CFILE_H

typedef struct FILE FILE;

FILE* fopen(const char* name, const char* mode);
int fclose(FILE* file);
unsigned int fread(void* address, unsigned int size, unsigned int count,
                   FILE* file);
unsigned int fwrite(const void* address, unsigned int size,
                    unsigned int count, FILE* file);
char* fgets(char* buffer, int maxLength, FILE* file);
int fputs(const char* buffer, FILE* file);
int feof(FILE* file);
int fseek(FILE* file, long offset, int origin);
int fflush(FILE* file);
long ftell(FILE* file);

#endif
