#include "ctype.h"

extern int __msl_strnicmp(const char* left, const char* right, unsigned long count);


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
        left_char = _tolower(*left++);
        right_char = _tolower(*right++);
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
        *cursor = _tolower(*cursor);
        ++cursor;
    }
    return string;
}
