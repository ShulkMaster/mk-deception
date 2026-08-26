#include "ctype.h"

extern int __msl_strnicmp(const char* left, const char* right, unsigned long count);

static inline int lower_char(int character)
{
    return character == -1 ? -1 : (int)__lower_map[(unsigned char)character];
}

char* strupr(char* string)
{
    char* cursor = string;
    while (*cursor != '\0') {
        *cursor = toupper(*cursor);
        ++cursor;
    }
    return string;
}

int strnicmp(const char* left, const char* right, unsigned long count)
{
    return __msl_strnicmp(left, right, count);
}

int stricmp(const char* left, const char* right)
{
    signed char left_char;
    signed char right_char;

    do {
        left_char = lower_char(*left++);
        right_char = lower_char(*right++);
        if (left_char < right_char)
            return -1;
        if (left_char > right_char)
            return 1;
    } while (left_char != 0);
    return 0;
}

char* strlwr(char* string)
{
    char* cursor = string;
    while (*cursor != '\0') {
        *cursor = lower_char(*cursor);
        ++cursor;
    }
    return string;
}
