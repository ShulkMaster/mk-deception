#include "runtime/cstring.h"

#define HAS_ZERO_BYTE(word) (((word) + 0xFEFEFEFFU) & 0x80808080U)

/* MSL retains its strerror literal pool beside strtok's empty sentinel. */
#define ERROR_STRINGS                                                          \
    "Argument list too long\0Permission denied\0"                            \
    "Resource temporarily unavailable\0Bad file descriptor\0Device busy\0"  \
    "No child processes\0Resource deadlock avoided\0"                        \
    "Numerical argument out of domain\0File exists\0Bad address\0"           \
    "File too large\0File Position Error\0Wide character encoding error\0"   \
    "Interrupted system call\0Invalid argument\0Input/output error\0"        \
    "Is a directory\0Too many open files\0Too many links\0"                 \
    "File name too long\0Too many open files in system\0"                    \
    "Operation not supported by device\0No such file or directory\0"         \
    "No error detected\0Exec format error\0No locks available\0"            \
    "Cannot allocate memory\0No space left on device\0"                      \
    "Function not implemented\0Not a directory\0Directory not empty\0"      \
    "Inappropriate ioctl for device\0Device not configured\0"                \
    "Operation not permitted\0Broken pipe\0Result too large\0"              \
    "Read-only file system\0Signal error\0Illegal seek\0No such process\0"  \
    "Unknown error\0Cross-device link\0Unknown Error (%d)"

static char* n = "\0" ERROR_STRINGS;
static char* s = "\0" ERROR_STRINGS;

unsigned long strlen(const char* string)
{
    unsigned long length = (unsigned long)-1;
    const unsigned char* current = (const unsigned char*)string - 1;

    do {
        ++length;
    } while (*++current != 0);
    return length;
}

char* strcpy(char* destination, const char* source)
{
    unsigned char* to = (unsigned char*)destination;
    const unsigned char* from = (const unsigned char*)source;
    unsigned long alignment;
    unsigned long word;

    if ((alignment = (unsigned long)from & 3U) ==
        ((unsigned long)to & 3U)) {
        if (alignment != 0) {
            if ((*to = *from) == 0)
                return destination;
            for (alignment = 3U - alignment; alignment != 0; --alignment) {
                if ((*++to = *++from) == 0)
                    return destination;
            }
            ++to;
            ++from;
        }
        word = *(const unsigned long*)from;
        if (!HAS_ZERO_BYTE(word)) {
            to -= sizeof(unsigned long);
            do {
                to += sizeof(unsigned long);
                *(unsigned long*)to = word;
                from += sizeof(unsigned long);
                word = *(const unsigned long*)from;
            } while (!HAS_ZERO_BYTE(word));
            to += sizeof(unsigned long);
        }
    }
    if ((*to = *from) == 0)
        return destination;
    do {
        ++to;
        ++from;
    } while ((*to = *from) != 0);
    return destination;
}

char* strncpy(char* destination, const char* source, unsigned long count)
{
    const unsigned char* from = (const unsigned char*)source - 1;
    unsigned char* to = (unsigned char*)destination - 1;

    ++count;
    while (--count != 0) {
        if ((*++to = *++from) == 0) {
            while (--count != 0)
                *++to = 0;
            break;
        }
    }
    return destination;
}

char* strcat(char* destination, const char* source)
{
    const unsigned char* from = (const unsigned char*)source - 1;
    unsigned char* to = (unsigned char*)destination - 1;

    while (*++to != 0) {
    }
    --to;
    while ((*++to = *++from) != 0) {
    }
    return destination;
}

char* strncat(char* destination, const char* source, unsigned long count)
{
    const unsigned char* from = (const unsigned char*)source - 1;
    unsigned char* to = (unsigned char*)destination - 1;

    while (*++to != 0) {
    }
    --to;
    ++count;
    while (--count != 0) {
        if ((*++to = *++from) == 0) {
            --to;
            break;
        }
    }
    *++to = 0;
    return destination;
}

int strcmp(const char* lhs, const char* rhs)
{
    const unsigned char* left = (const unsigned char*)lhs;
    const unsigned char* right = (const unsigned char*)rhs;
    unsigned long alignment;
    unsigned long left_byte = *left;
    unsigned long right_byte = *right;
    unsigned long left_word;
    unsigned long right_word;
    int result = (int)left_byte - (int)right_byte;

    if (result != 0)
        return result;
    if ((alignment = (unsigned long)left & 3U) ==
        ((unsigned long)right & 3U)) {
        if (alignment != 0) {
            if (left_byte == 0)
                return 0;
            for (alignment = 3U - alignment; alignment != 0; --alignment) {
                left_byte = *++left;
                right_byte = *++right;
                result = (int)left_byte - (int)right_byte;
                if (result != 0)
                    return result;
                if (left_byte == 0)
                    return 0;
            }
            ++left;
            ++right;
        }
        left_word = *(const unsigned long*)left;
        right_word = *(const unsigned long*)right;
        while (!HAS_ZERO_BYTE(left_word) && left_word == right_word) {
            left += sizeof(unsigned long);
            right += sizeof(unsigned long);
            left_word = *(const unsigned long*)left;
            right_word = *(const unsigned long*)right;
        }
        if (!HAS_ZERO_BYTE(left_word))
            return left_word > right_word ? 1 : -1;
        left_byte = *left;
        right_byte = *right;
        result = (int)left_byte - (int)right_byte;
        if (result != 0)
            return result;
    }
    if (left_byte == 0)
        return 0;
    do {
        left_byte = *++left;
        right_byte = *++right;
        result = (int)left_byte - (int)right_byte;
        if (result != 0)
            return result;
    } while (left_byte != 0);
    return 0;
}

int strncmp(const char* lhs, const char* rhs, unsigned long count)
{
    const unsigned char* left = (const unsigned char*)lhs - 1;
    const unsigned char* right = (const unsigned char*)rhs - 1;
    unsigned long left_byte;
    unsigned long right_byte;

    ++count;
    while (--count != 0) {
        left_byte = *++left;
        right_byte = *++right;
        if (left_byte != right_byte)
            return (int)left_byte - (int)right_byte;
        if (left_byte == 0)
            break;
    }
    return 0;
}

char* strchr(const char* string, int character)
{
    const unsigned char* current = (const unsigned char*)string - 1;
    unsigned long wanted = (unsigned long)character & 0xFFU;
    unsigned long value;

    while ((value = *++current) != 0) {
        if (value == wanted)
            return (char*)current;
    }
    return wanted != 0 ? 0 : (char*)current;
}

char* strrchr(const char* string, int character)
{
    const unsigned char* current = (const unsigned char*)string - 1;
    const unsigned char* found = 0;
    unsigned long wanted = (unsigned long)character & 0xFFU;
    unsigned long value;

    while ((value = *++current) != 0) {
        if (value == wanted)
            found = current;
    }
    if (found != 0)
        return (char*)found;
    return wanted != 0 ? 0 : (char*)current;
}

char* strtok(char* string, const char* delimiters)
{
    unsigned char delimiter_map[32] = {0};
    const unsigned char* delimiter;
    unsigned char* current;
    unsigned char* token;
    unsigned long value;

    if (string != 0)
        s = string;
    delimiter = (const unsigned char*)delimiters - 1;
    while ((value = *++delimiter) != 0) {
        delimiter_map[(value >> 3) & 0x1FU] |= 1U << (value & 7);
    }
    current = (unsigned char*)s - 1;
    while ((value = *++current) != 0) {
        if ((delimiter_map[(value >> 3) & 0x1FU] &
             (1U << (value & 7))) == 0)
            break;
    }
    if (value == 0) {
        s = n;
        return 0;
    }
    token = current;
    while ((value = *++current) != 0) {
        if ((delimiter_map[(value >> 3) & 0x1FU] &
             (1U << (value & 7))) != 0)
            break;
    }
    if (value == 0) {
        s = n;
        return (char*)token;
    }
    s = (char*)current + 1;
    *current = 0;
    return (char*)token;
}

char* strstr(const char* string, const char* substring)
{
    const unsigned char* current = (const unsigned char*)string - 1;
    const unsigned char* pattern = (const unsigned char*)substring - 1;
    unsigned long first;
    unsigned long string_byte;
    unsigned long pattern_byte;

    if (substring == 0 || (first = *++pattern) == 0)
        return (char*)string;
    while ((string_byte = *++current) != 0) {
        if (string_byte == first) {
            const unsigned char* scan = current - 1;
            const unsigned char* match = pattern - 1;

            while ((string_byte = *++scan) ==
                       (pattern_byte = *++match) &&
                   string_byte != 0) {
            }
            if (pattern_byte == 0)
                return (char*)current;
        }
    }
    return 0;
}
