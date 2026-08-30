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
    ((map)[(unsigned char)(character) >> 3] & \
     (1 << ((unsigned char)(character) & 7)))

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
    int num_chars;
    int chars_read = 0;
    int items_assigned = 0;
    int conversions = 0;
    int base;
    int negative;
    int overflow;
    const char* format_cursor = format_string;
    char format_character;
    char character;
    ScanFormat format;
    long signed_value;
    unsigned long unsigned_value;
    long long signed_long_long;
    unsigned long long unsigned_long_long;
    long double floating_value;
    char* argument;

    while ((format_character = *format_cursor) != 0) {
        if (isspace(format_character)) {
            do {
                format_character = *++format_cursor;
            } while (isspace(format_character));
            while (isspace(character =
                               read_proc(read_context, 0, __GetAChar))) {
                chars_read++;
            }
            read_proc(read_context, character, __UngetAChar);
            continue;
        }

        if (format_character != '%') {
            character = read_proc(read_context, 0, __GetAChar);
            if (character != (unsigned char)format_character) {
                read_proc(read_context, character, __UngetAChar);
                break;
            } else {
                chars_read++;
                format_cursor++;
            }
            continue;
        }

        format_cursor = parse_format(format_cursor, &format);
        if (!format.suppress_assignment && format.conversion != '%') {
            argument = *((char**)__va_arg(arguments, 1));
        } else {
            argument = 0;
        }
        if (format.conversion != 'n' &&
            read_proc(read_context, 0, __TestForError)) {
            break;
        }

        switch (format.conversion) {
        case 'd':
        case 'i':
            base = format.conversion == 'd' ? 10 : 0;
            if (format.argument_option == SCAN_LONG_LONG) {
                unsigned_long_long = __strtoull(
                    base, format.field_width, read_proc, read_context,
                    &num_chars, &negative, &overflow);
            } else {
                unsigned_value = __strtoul(
                    base, format.field_width, read_proc, read_context,
                    &num_chars, &negative, &overflow);
            }
            if (num_chars == 0) {
                break;
            }
            chars_read += num_chars;
            if (format.argument_option == SCAN_LONG_LONG)
                signed_long_long = negative ? -unsigned_long_long
                                            : unsigned_long_long;
            else
                signed_value = negative ? -unsigned_value : unsigned_value;
            if (argument != 0) {
                switch (format.argument_option) {
                case SCAN_NORMAL: *(int*)argument = signed_value; break;
                case SCAN_CHAR: *(signed char*)argument = signed_value; break;
                case SCAN_SHORT: *(short*)argument = signed_value; break;
                case SCAN_LONG: *(long*)argument = signed_value; break;
                case SCAN_LONG_LONG:
                    *(long long*)argument = signed_long_long;
                    break;
                }
                items_assigned++;
            }
            conversions++;
            continue;

        case 'o':
        case 'u':
        case 'x':
        case 'X':
            base = format.conversion == 'o' ? 8
                 : format.conversion == 'u' ? 10 : 16;
            if (format.argument_option == SCAN_LONG_LONG) {
                unsigned_long_long = __strtoull(
                    base, format.field_width, read_proc, read_context,
                    &num_chars, &negative, &overflow);
            } else {
                unsigned_value = __strtoul(
                    base, format.field_width, read_proc, read_context,
                    &num_chars, &negative, &overflow);
            }
            if (num_chars == 0) {
                break;
            }
            chars_read += num_chars;
            if (negative) {
                if (format.argument_option == SCAN_LONG_LONG)
                    unsigned_long_long = -unsigned_long_long;
                else
                    unsigned_value = -unsigned_value;
            }
            if (argument != 0) {
                switch (format.argument_option) {
                case SCAN_NORMAL:
                    *(unsigned int*)argument = unsigned_value;
                    break;
                case SCAN_CHAR:
                    *(unsigned char*)argument = unsigned_value;
                    break;
                case SCAN_SHORT:
                    *(unsigned short*)argument = unsigned_value;
                    break;
                case SCAN_LONG:
                    *(unsigned long*)argument = unsigned_value;
                    break;
                case SCAN_LONG_LONG:
                    *(unsigned long long*)argument = unsigned_long_long;
                    break;
                }
                items_assigned++;
            }
            conversions++;
            continue;

        case 'a': case 'f': case 'e': case 'E': case 'g': case 'G':
            floating_value = __strtold(format.field_width, read_proc,
                                       read_context, &num_chars, &overflow);
            if (num_chars == 0) {
                break;
            }
            chars_read += num_chars;
            if (argument != 0) {
                switch (format.argument_option) {
                case SCAN_NORMAL: *(float*)argument = floating_value; break;
                case SCAN_DOUBLE: *(double*)argument = floating_value; break;
                case SCAN_LONG_DOUBLE:
                    *(long double*)argument = floating_value;
                    break;
                }
                items_assigned++;
            }
            conversions++;
            continue;

        case 'c': {
            int read_value;
            if (!format.field_width_specified) format.field_width = 1;
            if (argument != 0) {
                num_chars = 0;
                while (format.field_width-- &&
                       (read_value = read_proc(
                            read_context, 0, __GetAChar)) != -1) {
                    character = read_value;
                    if (format.argument_option == SCAN_WCHAR) {
                        mbtowc((unsigned short*)argument, &character, 1);
                        argument++;
                    } else {
                        *argument++ = character;
                    }
                    num_chars++;
                }
                if (num_chars == 0) {
                    break;
                }
                chars_read += num_chars;
                items_assigned++;
            } else {
                num_chars = 0;
                while (format.field_width-- &&
                       (character = read_proc(
                            read_context, 0, __GetAChar)) != -1) {
                    num_chars++;
                }
                if (num_chars == 0) {
                    break;
                }
            }
            conversions++;
            continue;
        }

        case '%':
            while (isspace(character =
                               read_proc(read_context, 0, __GetAChar)))
                chars_read++;
            if (character != '%') {
                read_proc(read_context, character, __UngetAChar);
                break;
            } else {
                chars_read++;
            }
            continue;

        case 's':
            character = read_proc(read_context, 0, __GetAChar);
            while (isspace(character)) {
                chars_read++;
                character = read_proc(read_context, 0, __GetAChar);
            }
            read_proc(read_context, character, __UngetAChar);
            /* fall through */
        case '[':
            if (argument != 0) {
                num_chars = 0;
                while (format.field_width-- &&
                       (character = read_proc(
                            read_context, 0, __GetAChar)) != -1 &&
                       TEST_SCAN_CHARACTER(
                            format.character_set, character)) {
                    if (format.argument_option == SCAN_WCHAR) {
                        mbtowc((unsigned short*)argument, &character, 1);
                        argument = (char*)((unsigned short*)argument + 1);
                    } else {
                        *argument++ = character;
                    }
                    num_chars++;
                }
                if (num_chars == 0) {
                    read_proc(read_context, character, __UngetAChar);
                    break;
                }
                chars_read += num_chars;
                if (format.argument_option == SCAN_WCHAR)
                    *(unsigned short*)argument = 0;
                else
                    *argument = 0;
                items_assigned++;
            } else {
                num_chars = 0;
                while (format.field_width-- &&
                       (character = read_proc(
                            read_context, 0, __GetAChar)) != -1 &&
                       TEST_SCAN_CHARACTER(
                            format.character_set, character)) {
                    num_chars++;
                }
                if (num_chars == 0) {
                    read_proc(read_context, character, __UngetAChar);
                    continue;
                }
                chars_read += num_chars;
            }
            if (format.field_width >= 0)
                read_proc(read_context, character, __UngetAChar);
            conversions++;
            continue;

        case 'n':
            if (argument != 0) {
                switch (format.argument_option) {
                case SCAN_NORMAL: *(int*)argument = chars_read; break;
                case SCAN_SHORT: *(short*)argument = chars_read; break;
                case SCAN_LONG: *(long*)argument = chars_read; break;
                case SCAN_CHAR: *(char*)argument = chars_read; break;
                case SCAN_LONG_LONG:
                    *(long long*)argument = chars_read;
                    break;
                }
            }
            continue;

        case 0xFF:
        default:
            break;
        }
        break;
    }

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
