typedef signed char s8;
typedef signed long s32;
typedef unsigned long u32;

typedef struct VaList {
    s8 g_register;
    s8 f_register;
    unsigned short reserved;
    char* input_arg_area;
    char* reg_save_area;
} VaList;

void* __va_arg(VaList* list, s32 type)
{
    char* address;
    s8* register_index = &list->g_register;
    s32 index = list->g_register;
    s32 maximum = 8;
    s32 size = 4;
    s32 increment = 1;
    s32 even = 0;
    s32 fpr_offset = 0;
    s32 register_size = 4;

    if (type == 3) {
        register_index = &list->f_register;
        index = list->f_register;
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
        address = (char*)(((u32)address + (size - 1)) & ~(size - 1));
        list->input_arg_area = address + size;
    }
    if (type == 0)
        address = *(char**)address;
    return address;
}
