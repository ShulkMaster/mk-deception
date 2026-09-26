#include "runtime/cstdio.h"
#include "runtime/cstdlib.h"
#include "ctype.h"

#undef va_start
#define va_start(arguments, format) \
    ((void)(format), __builtin_va_info(&(arguments)))

enum ScanArgumentOption {
    SCAN_NORMAL,
    SCAN_CHAR,
    SCAN_SHORT,
    SCAN_LONG,
    SCAN_LONG_LONG,
    SCAN_DOUBLE,
    SCAN_LONG_DOUBLE,
    SCAN_WCHAR
};

typedef unsigned char ScanCharacterMap[32];

typedef struct ScanFormat {
    unsigned char suppress_assignment;
    unsigned char field_width_specified;
    unsigned char argument_option;
    unsigned char conversion;
    int field_width;
    ScanCharacterMap character_set;
} ScanFormat;

typedef int (*ScanReadProc)(void*, int, int);

#define SET_SCAN_CHARACTER(map, character) \
    ((map)[(unsigned char)(character) >> 3] |= \
     (1 << ((unsigned char)(character) & 7)))
#define TEST_SCAN_CHARACTER(map, character) \
    ((map)[(unsigned char)(character) >> 3] & (1 << ((character) & 7)))

int mbtowc(unsigned short* output, const char* input, unsigned long length);
long double __strtold(int max_width, ScanReadProc read_proc,
                      void* read_context, int* chars_scanned, int* overflow);

static int __sformatter(ScanReadProc read_proc, void* read_context,
                        const char* format_string, __va_list arguments);
static const char* parse_format(const char* format_string, ScanFormat* format);

int sscanf(const char* input, const char* format_string, ...)
{
    __va_list arguments;
    __InStrCtrl control;

    va_start(arguments, format_string);
    control.NextChar = (char*)input;
    if (input == 0 || *control.NextChar == '\0') {
        return -1;
    }
    control.NullCharDetected = 0;
    return __sformatter(__StringRead, &control, format_string, arguments);
}

int __StringRead(void* context, int character, int action)
{
    __InStrCtrl* control = (__InStrCtrl*)context;
    char result;

    switch (action) {
    case __GetAChar:
        result = *control->NextChar;
        if (result == '\0') {
            control->NullCharDetected = 1;
            return -1;
        }
        control->NextChar++;
        return (unsigned char)result;
    case __UngetAChar:
        if (control->NullCharDetected == 0) {
            control->NextChar--;
        } else {
            control->NullCharDetected = 0;
        }
        return character;
    case __TestForError:
        return control->NullCharDetected;
    }
    return 0;
}

static int __sformatter(ScanReadProc read_proc, void* read_context,
                        const char* format_string, __va_list arguments)
{
    int num_chars, chars_read, items_assigned, conversions;
    int base, negative, overflow;
    const char* format_ptr;
    char format_char;
    char c;
    ScanFormat format;
    long long_num;
    unsigned long u_long_num;
    long long long_long_num;
    unsigned long long u_long_long_num;
    long double long_double_num;
    char* arg_ptr;
    int terminate = 0;

    format_ptr = format_string;
    chars_read = 0;
    items_assigned = 0;
    conversions    = 0;

    while (!terminate && (format_char = *format_ptr) != 0) {
        if (isspace(format_char)) {
            do {
                format_char = *++format_ptr;
            } while (isspace(format_char));

            while (isspace(c = read_proc(read_context, 0, __GetAChar)))
                ++chars_read;

            read_proc(read_context, c, __UngetAChar);

            continue;
        }

        if (format_char != '%') {
            if ((c = read_proc(read_context, 0, __GetAChar)) != (unsigned char)format_char) {
                read_proc(read_context, c, __UngetAChar);
                goto exit;
            }

            chars_read++;
            format_ptr++;

            continue;
        }

        format_ptr = parse_format(format_ptr, &format);

        if (!format.suppress_assignment && format.conversion != '%') {
            arg_ptr = *((char**)__va_arg(arguments, 1));
        } else {
            arg_ptr = 0;
        }

        if ((format.conversion != 'n') && read_proc(read_context, 0, __TestForError)) {
            terminate = 1;
            goto exit;
        }

        switch (format.conversion) {
        case 'd':
            base = 10;
            goto signed_int;
        case 'i':
            base = 0;
        signed_int:
            if ((format.argument_option == SCAN_LONG_LONG))
                u_long_long_num = __strtoull(base, format.field_width, read_proc, read_context, &num_chars, &negative, &overflow);
            else
                u_long_num = __strtoul(base, format.field_width, read_proc, read_context, &num_chars, &negative, &overflow);

            if (!num_chars) {
                goto exit;
            }

            chars_read += num_chars;

            if ((format.argument_option == SCAN_LONG_LONG))
                long_long_num = (negative ? -u_long_long_num : u_long_long_num);
            else
                long_num = (negative ? -u_long_num : u_long_num);

            if (arg_ptr) {
                switch (format.argument_option) {
                case SCAN_NORMAL:
                    *(int*)arg_ptr = long_num;
                    break;
                case SCAN_CHAR:
                    *(signed char*)arg_ptr = long_num;
                    break;
                case SCAN_SHORT:
                    *(short*)arg_ptr = long_num;
                    break;
                case SCAN_LONG:
                    *(long*)arg_ptr = long_num;
                    break;
                case SCAN_LONG_LONG:
                    *(long long*)arg_ptr = long_long_num;
                    break;
                }

                items_assigned++;
            }

            conversions++;
            break;
        case 'o':
            base = 8;
            goto unsigned_int;
        case 'u':
            base = 10;
            goto unsigned_int;
        case 'x':
        case 'X':
            base = 16;
        unsigned_int:
            if ((format.argument_option == SCAN_LONG_LONG))
                u_long_long_num = __strtoull(base, format.field_width, read_proc, read_context, &num_chars, &negative, &overflow);
            else
                u_long_num = __strtoul(base, format.field_width, read_proc, read_context, &num_chars, &negative, &overflow);

            if (!num_chars) {
                goto exit;
            }

            chars_read += num_chars;

            if (negative) {
                if (format.argument_option == SCAN_LONG_LONG)
                    u_long_long_num = -u_long_long_num;
                else
                    u_long_num = -u_long_num;
            }

            if (arg_ptr) {
                switch (format.argument_option) {
                case SCAN_NORMAL:
                    *(unsigned int*)arg_ptr = u_long_num;
                    break;
                case SCAN_CHAR:
                    *(unsigned char*)arg_ptr = u_long_num;
                    break;
                case SCAN_SHORT:
                    *(unsigned short*)arg_ptr = u_long_num;
                    break;
                case SCAN_LONG:
                    *(unsigned long*)arg_ptr = u_long_num;
                    break;
                case SCAN_LONG_LONG:
                    *(unsigned long long*)arg_ptr = u_long_long_num;
                    break;
                }

                items_assigned++;
            }

            conversions++;
            break;
        case 'a':
        case 'f':
        case 'e':
        case 'E':
        case 'g':
        case 'G':
            long_double_num = __strtold(format.field_width, read_proc, read_context, &num_chars, &overflow);

            if (!num_chars) {
                goto exit;
            }

            chars_read += num_chars;

            if (arg_ptr) {
                switch (format.argument_option) {
                case SCAN_NORMAL:
                    *(float*)arg_ptr = long_double_num;
                    break;
                case SCAN_DOUBLE:
                    *(double*)arg_ptr = long_double_num;
                    break;
                case SCAN_LONG_DOUBLE:
                    *(long double*)arg_ptr = long_double_num;
                    break;
                }

                items_assigned++;
            }

            conversions++;
            break;

        case 'c':

            if (!format.field_width_specified)
                format.field_width = 1;

            if (arg_ptr) {
                int rval;
                num_chars = 0;

                while (format.field_width-- && ((rval = (read_proc(read_context, 0, __GetAChar))) != -1)) {
                    c = rval;

                    if (format.argument_option == SCAN_WCHAR) {
                        mbtowc(((unsigned short*)arg_ptr), (char*)(&c), 1);
                        arg_ptr++;
                    } else {
                        *arg_ptr++ = c;
                    }
                    num_chars++;
                }

                if (!num_chars) {
                    goto exit;
                }

                chars_read += num_chars;

                items_assigned++;
            } else {
                num_chars = 0;

                while (format.field_width-- && ((c = (read_proc(read_context, 0, __GetAChar))) != -1)) {
                    num_chars++;
                }
                if (!num_chars)
                    goto exit;
            }

            conversions++;
            break;
        case '%':
            while (isspace(c = read_proc(read_context, 0, __GetAChar)))
                chars_read++;

            if (c != '%') {
                read_proc(read_context, c, __UngetAChar);
                goto exit;
            }

            chars_read++;
            break;
        case 's':
            c = read_proc(read_context, 0, __GetAChar);
            while (isspace(c)) {
                chars_read++;
                c = read_proc(read_context, 0, __GetAChar);
            }

            read_proc(read_context, c, __UngetAChar);
        case '[':
            if (arg_ptr) {
                num_chars = 0;

                while (format.field_width-- && ((c = (read_proc(read_context, 0, __GetAChar))) != -1)
                       && TEST_SCAN_CHARACTER(format.character_set, c)) {
                    if (format.argument_option == SCAN_WCHAR) {
                        mbtowc(((unsigned short*)arg_ptr), (char*)&c, 1);
                        arg_ptr = (char*)((unsigned short*)arg_ptr + 1);
                    } else {
                        *arg_ptr++ = c;
                    }
                    num_chars++;
                }

                if (!num_chars) {
                    read_proc(read_context, c, __UngetAChar);
                    goto exit;
                }

                chars_read += num_chars;

                if (format.argument_option == SCAN_WCHAR)
                    *(unsigned short*)arg_ptr = 0;
                else
                    *arg_ptr = 0;

                items_assigned++;
            } else {
                num_chars = 0;

                while (format.field_width-- && ((c = (read_proc(read_context, 0, __GetAChar))) != -1)
                       && TEST_SCAN_CHARACTER(format.character_set, c)) {

                    num_chars++;
                }

                if (!num_chars) {
                    read_proc(read_context, c, __UngetAChar);
                    break;
                }
                chars_read += num_chars;
            }

            if (format.field_width >= 0)
                read_proc(read_context, c, __UngetAChar);

            conversions++;
            break;
        case 'n':
            if (arg_ptr)
                switch (format.argument_option) {
                case SCAN_NORMAL:
                    *(int*)arg_ptr = chars_read;
                    break;
                case SCAN_SHORT:
                    *(short*)arg_ptr = chars_read;
                    break;
                case SCAN_LONG:
                    *(long*)arg_ptr = chars_read;
                    break;
                case SCAN_CHAR:
                    *(char*)arg_ptr = chars_read;
                    break;
                case SCAN_LONG_LONG:
                    *(long long*)arg_ptr = chars_read;
                    break;
                }
            continue;
        case 0xFF:
        default:
            goto exit;
        }
    }

exit:

    if (read_proc(read_context, 0, __TestForError) && conversions == 0)
        return -1;

    return items_assigned;
}

static const char* parse_format(const char* format_string, ScanFormat* format)
{
    const char* cursor = format_string;
    int character;
    int flag_found;
    int invert;
    ScanFormat parsed = {0, 0, SCAN_NORMAL, 0, 0x7FFFFFFF, {0}};

    character = *++cursor;
    if (character == '%') {
        parsed.conversion = character;
        *format = parsed;
        return cursor + 1;
    }
    if (character == '*') {
        parsed.suppress_assignment = 1;
        character = *++cursor;
    }
    if (isdigit(character)) {
        parsed.field_width = 0;
        do {
            parsed.field_width =
                (parsed.field_width * 10) + (character - '0');
            character = *++cursor;
        } while (isdigit(character));
        if (parsed.field_width == 0) {
            parsed.conversion = 0xFF;
            *format = parsed;
            return cursor + 1;
        }
        parsed.field_width_specified = 1;
    }

    flag_found = 1;
    switch (character) {
    case 'h':
        parsed.argument_option = SCAN_SHORT;
        if (cursor[1] == 'h') {
            parsed.argument_option = SCAN_CHAR;
            character = *++cursor;
        }
        break;
    case 'l':
        parsed.argument_option = SCAN_LONG;
        if (cursor[1] == 'l') {
            parsed.argument_option = SCAN_LONG_LONG;
            character = *++cursor;
        }
        break;
    case 'L':
        parsed.argument_option = SCAN_LONG_DOUBLE;
        break;
    default:
        flag_found = 0;
        break;
    }
    if (flag_found) character = *++cursor;

    parsed.conversion = character;
    switch (character) {
    case 'd': case 'i': case 'u': case 'o': case 'x': case 'X':
        if (parsed.argument_option == SCAN_LONG_DOUBLE)
            parsed.conversion = 0xFF;
        break;
    case 'a': case 'f': case 'e': case 'E': case 'g': case 'G':
        if (parsed.argument_option == SCAN_CHAR ||
            parsed.argument_option == SCAN_SHORT ||
            parsed.argument_option == SCAN_LONG_LONG) {
            parsed.conversion = 0xFF;
        } else if (parsed.argument_option == SCAN_LONG) {
            parsed.argument_option = SCAN_DOUBLE;
        }
        break;
    case 'p':
        parsed.argument_option = SCAN_LONG;
        parsed.conversion = 'x';
        break;
    case 'c':
        if (parsed.argument_option == SCAN_LONG)
            parsed.argument_option = SCAN_WCHAR;
        else if (parsed.argument_option != SCAN_NORMAL)
            parsed.conversion = 0xFF;
        break;
    case 's': {
        int index;
        unsigned char* entry;
        if (parsed.argument_option == SCAN_LONG)
            parsed.argument_option = SCAN_WCHAR;
        else if (parsed.argument_option != SCAN_NORMAL)
            parsed.conversion = 0xFF;
        for (index = sizeof(parsed.character_set), entry = parsed.character_set;
             index != 0; --index)
            *entry++ = 0xFF;
        parsed.character_set[1] = 0xC1;
        parsed.character_set[4] = 0xFE;
        break;
    }
    case 'n':
        break;
    case '[':
        if (parsed.argument_option == SCAN_LONG)
            parsed.argument_option = SCAN_WCHAR;
        else if (parsed.argument_option != SCAN_NORMAL)
            parsed.conversion = 0xFF;
        character = *++cursor;
        invert = 0;
        if (character == '^') {
            invert = 1;
            character = *++cursor;
        }
        if (character == ']') {
            SET_SCAN_CHARACTER(parsed.character_set, ']');
            character = *++cursor;
        }
        while (character != 0 && character != ']') {
            int range_end;
            SET_SCAN_CHARACTER(parsed.character_set, character);
            if (cursor[1] == '-' && (range_end = cursor[2]) != 0 &&
                range_end != ']') {
                while (++character <= range_end)
                    SET_SCAN_CHARACTER(parsed.character_set, character);
                cursor += 3;
                character = *cursor;
            } else {
                character = *++cursor;
            }
        }
        if (character == 0) {
            parsed.conversion = 0xFF;
        } else if (invert) {
            int index;
            unsigned char* entry = parsed.character_set;
            for (index = sizeof(parsed.character_set); index != 0;
                 --index, ++entry)
                *entry = ~*entry;
        }
        break;
    default:
        parsed.conversion = 0xFF;
        break;
    }

    *format = parsed;
    return cursor + 1;
}
