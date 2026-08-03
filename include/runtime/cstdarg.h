#ifndef MKD_RUNTIME_CSTDARG_H
#define MKD_RUNTIME_CSTDARG_H

typedef struct MkVaListState {
    unsigned char gpr;
    unsigned char fpr;
    unsigned char reserved[2];
    char* input_arg_area;
    char* reg_save_area;
} __va_list[1];

#define va_start(list, last_arg) __va_start(list, last_arg)
#define va_end(list) ((void)0)

void* __va_arg(__va_list args, int type);

#endif
