#include "runtime/cstdarg.h"

void* __va_arg(__va_list list, int type)
{
    char* address;
    signed char* register_index = &list->gpr;
    int index = list->gpr;
    int maximum = 8;
    int size = 4;
    int increment = 1;
    int even = 0;
    int fpr_offset = 0;
    int register_size = 4;

    if (type == 3) {
        register_index = &list->fpr;
        index = list->fpr;
        size = 8;
        fpr_offset = 32;
        register_size = 8;
    }
    if (type == 2) {
        size = 8;
        maximum--;
        if (index & 1)
            even = 1;
        increment = 2;
    }
    if (index < maximum) {
        index += even;
        address = list->reg_save_area + fpr_offset + index * register_size;
        *register_index = index + increment;
    } else {
        *register_index = 8;
        address = list->input_arg_area;
        address = (char*)(((unsigned int)address + (size - 1)) & ~(size - 1));
        list->input_arg_area = address + size;
    }
    if (type == 0)
        address = *(char**)address;
    return address;
}
