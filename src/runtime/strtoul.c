#include "ctype.h"
#include "runtime/cstdio.h"
#include "runtime/cstdlib.h"

#define ULONG_MAX_VALUE 0xFFFFFFFFUL
#define ULLONG_MAX_VALUE 0xFFFFFFFFFFFFFFFFULL
#define MSL_ERANGE 34

enum ScanState {
    SCAN_START = 0x01,
    SCAN_CHECK_FOR_ZERO = 0x02,
    SCAN_LEADING_ZERO = 0x04,
    SCAN_NEED_DIGIT = 0x08,
    SCAN_DIGIT_LOOP = 0x10,
    SCAN_FINISHED = 0x20,
    SCAN_FAILURE = 0x40,
};

#define SCAN_IS_FINAL(state) ((state) & (SCAN_FINISHED | SCAN_FAILURE))
#define SCAN_SUCCEEDED(state) \
    ((state) & (SCAN_LEADING_ZERO | SCAN_DIGIT_LOOP | SCAN_FINISHED))
#define FETCH_CHAR() \
    (count++, read_proc(read_context, 0, __GetAChar))
#define UNFETCH_CHAR(character) \
    read_proc(read_context, (character), __UngetAChar)

extern int errno;

int atoi(const char* str) {
    unsigned long value;
    int overflow;
    int negative;
    int count;
    __InStrCtrl input;

    input.NextChar = (char*)str;
    input.NullCharDetected = 0;
    value = __strtoul(
        10, 0x7FFFFFFF, __StringRead, &input,
        &count, &negative, &overflow);
    if (overflow || (!negative && value > 0x7FFFFFFFUL) ||
        (negative && value > 0x80000000UL)) {
        value = negative ? 0x80000000UL : 0x7FFFFFFFUL;
        errno = MSL_ERANGE;
    } else if (negative) {
        value = (unsigned long)-(long)value;
    }
    return (int)value;
}

unsigned long strtoul(const char* str, char** end, int base) {
    unsigned long value;
    int count;
    int negative;
    int overflow;
    __InStrCtrl input;

    input.NextChar = (char*)str;
    input.NullCharDetected = 0;
    value = __strtoul(
        base, 0x7FFFFFFF, &__StringRead, (void*)&input,
        &count, &negative, &overflow);
    if (end != 0) {
        *end = (char*)str + count;
    }
    if (overflow) {
        value = ULONG_MAX_VALUE;
        errno = MSL_ERANGE;
    } else if (negative) {
        value = (unsigned long)-(long)value;
    }
    return value;
}

unsigned long long __strtoull(
    int base, int max_width, int (*read_proc)(void*, int, int),
    void* read_context, int* chars_scanned, int* negative, int* overflow) {
    int scan_state = SCAN_START;
    int count = 0;
    int spaces = 0;
    unsigned long long value = 0;
    unsigned long long value_max = 0;
    unsigned long long ullong_max = ULLONG_MAX_VALUE;
    int c;

    *negative = *overflow = 0;
    if (base < 0 || base == 1 || base > 36 || max_width < 1) {
        scan_state = SCAN_FAILURE;
    } else {
        c = FETCH_CHAR();
    }
    if (base != 0) {
        value_max = ULLONG_MAX_VALUE / base;
    }
    while (count <= max_width && c != -1 && !SCAN_IS_FINAL(scan_state)) {
        switch (scan_state) {
        case SCAN_START:
            if (isspace(c)) {
                c = FETCH_CHAR();
                count--;
                spaces++;
                break;
            }
            if (c == '+') {
                c = FETCH_CHAR();
            } else if (c == '-') {
                c = FETCH_CHAR();
                *negative = 1;
            }
            scan_state = SCAN_CHECK_FOR_ZERO;
            break;

        case SCAN_CHECK_FOR_ZERO:
            if ((base == 0 || base == 16) && c == '0') {
                scan_state = SCAN_LEADING_ZERO;
                c = FETCH_CHAR();
                break;
            }
            scan_state = SCAN_NEED_DIGIT;
            break;

        case SCAN_LEADING_ZERO:
            if (c == 'X' || c == 'x') {
                base = 16;
                scan_state = SCAN_NEED_DIGIT;
                c = FETCH_CHAR();
                break;
            }
            if (base == 0) {
                base = 8;
            }
            scan_state = SCAN_DIGIT_LOOP;
            break;

        case SCAN_NEED_DIGIT:
        case SCAN_DIGIT_LOOP:
            if (base == 0) {
                base = 10;
            }
            if (!value_max) {
                value_max = ullong_max / base;
            }
            if (isdigit(c)) {
                if ((c -= '0') >= base) {
                    if (scan_state == SCAN_DIGIT_LOOP) {
                        scan_state = SCAN_FINISHED;
                    } else {
                        scan_state = SCAN_FAILURE;
                    }
                    c += '0';
                    break;
                }
            } else if (!isalpha(c) || (toupper(c) - 'A' + 10) >= base) {
                if (scan_state == SCAN_DIGIT_LOOP) {
                    scan_state = SCAN_FINISHED;
                } else {
                    scan_state = SCAN_FAILURE;
                }
                break;
            } else {
                c = toupper(c) - 'A' + 10;
            }
            if (value > value_max) {
                *overflow = 1;
            }
            value *= base;
            if (c > ullong_max - value) {
                *overflow = 1;
            }
            value += c;
            scan_state = SCAN_DIGIT_LOOP;
            c = FETCH_CHAR();
            break;
        }
    }
    if (!SCAN_SUCCEEDED(scan_state)) {
        count = 0;
        value = *chars_scanned = 0;
    } else {
        count--;
        *chars_scanned = count + spaces;
    }
    UNFETCH_CHAR(c);
    return value;
}

unsigned long __strtoul(
    int base, int max_width, int (*read_proc)(void*, int, int),
    void* read_context, int* chars_scanned, int* negative, int* overflow) {
    int scan_state = SCAN_START;
    int count = 0;
    int spaces = 0;
    unsigned long value = 0;
    unsigned long value_max = 0;
    int c;

    *negative = *overflow = 0;
    if (base < 0 || base == 1 || base > 36 || max_width < 1) {
        scan_state = SCAN_FAILURE;
    } else {
        c = FETCH_CHAR();
    }
    if (base != 0) {
        value_max = ULONG_MAX_VALUE / base;
    }
    while (count <= max_width && c != -1 && !SCAN_IS_FINAL(scan_state)) {
        switch (scan_state) {
        case SCAN_START:
            if (isspace(c)) {
                c = FETCH_CHAR();
                count--;
                spaces++;
                break;
            }
            if (c == '+') {
                c = FETCH_CHAR();
            } else if (c == '-') {
                c = FETCH_CHAR();
                *negative = 1;
            }
            scan_state = SCAN_CHECK_FOR_ZERO;
            break;

        case SCAN_CHECK_FOR_ZERO:
            if ((base == 0 || base == 16) && c == '0') {
                scan_state = SCAN_LEADING_ZERO;
                c = FETCH_CHAR();
                break;
            }
            scan_state = SCAN_NEED_DIGIT;
            break;

        case SCAN_LEADING_ZERO:
            if (c == 'X' || c == 'x') {
                base = 16;
                scan_state = SCAN_NEED_DIGIT;
                c = FETCH_CHAR();
                break;
            }
            if (base == 0) {
                base = 8;
            }
            scan_state = SCAN_DIGIT_LOOP;
            break;

        case SCAN_NEED_DIGIT:
        case SCAN_DIGIT_LOOP:
            if (base == 0) {
                base = 10;
            }
            if (!value_max) {
                value_max = ULONG_MAX_VALUE / base;
            }
            if (isdigit(c)) {
                if ((c -= '0') >= base) {
                    if (scan_state == SCAN_DIGIT_LOOP) {
                        scan_state = SCAN_FINISHED;
                    } else {
                        scan_state = SCAN_FAILURE;
                    }
                    c += '0';
                    break;
                }
            } else if (!isalpha(c) || (toupper(c) - 'A' + 10) >= base) {
                if (scan_state == SCAN_DIGIT_LOOP) {
                    scan_state = SCAN_FINISHED;
                } else {
                    scan_state = SCAN_FAILURE;
                }
                break;
            } else {
                c = toupper(c) - 'A' + 10;
            }
            if (value > value_max) {
                *overflow = 1;
            }
            value *= base;
            if (c > ULONG_MAX_VALUE - value) {
                *overflow = 1;
            }
            value += c;
            scan_state = SCAN_DIGIT_LOOP;
            c = FETCH_CHAR();
            break;
        }
    }
    if (!SCAN_SUCCEEDED(scan_state)) {
        count = 0;
        value = 0;
        *chars_scanned = 0;
    } else {
        count--;
        *chars_scanned = count + spaces;
    }
    UNFETCH_CHAR(c);
    return value;
}
