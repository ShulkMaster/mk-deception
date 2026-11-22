#ifndef MKD_RUNTIME_CSTDARG_H
#define MKD_RUNTIME_CSTDARG_H

#ifdef VERSION_GQNE5D
typedef struct MkVaListState {
    signed char gpr;
    signed char fpr;
    unsigned short reserved;
    char* input_arg_area;
    char* reg_save_area;
} __va_list[1];

#define va_start(list, last_arg) __builtin_va_info(&(list))
#define va_arg(ap, type) (*((type*)__va_arg((ap), _var_arg_typeof(type))))

void* __va_arg(__va_list args, int type);
#endif

#ifdef VERSION_SLUS_208_81
typedef char* __va_list;

#define va_start(list, last_arg)                                              \
    ((list) = (char*)__builtin_next_arg(last_arg) -                           \
              (__builtin_args_info(2) >= 8 ? 0 : (8 - __builtin_args_info(2)) * 8))
#define va_arg(ap, type)                                                      \
    (*(type*)(((ap) += (sizeof(type) + 7) & ~7) - ((sizeof(type) + 7) & ~7)))
#endif

#define va_end(list) ((void)0)

#endif
