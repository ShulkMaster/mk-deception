#include "cri/sj.h"
#include "runtime/cstring.h"

int sj_hexstr_to_val_tbl[0x70] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0,
    0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

static inline int sj_HexStrToVal(const char* text, int count)
{
    int value;
    int i;

    value = 0;
    for (i = 0; i < count; i++) {
        value = (value << 4) + sj_hexstr_to_val_tbl[text[i]];
    }
    return value;
}

unsigned char* SJ_SearchTag(
    SJCK* source, const char* tag, const char* terminator, SJCK* result) {
    unsigned char* current;
    unsigned char* end;

    result->data = 0;
    result->len = 0;
    current = source->data;
    end = source->data + source->len;
    while (current < end) {
        if (strncmp((const char*)current, tag, 7) == 0) {
            result->data = current + 0x10;
            result->len = sj_HexStrToVal((const char*)current + 8, 7);
            break;
        }
        if (terminator != 0 &&
            strncmp((const char*)current, terminator, 7) == 0) {
            return 0;
        }
        current += sj_HexStrToVal((const char*)current + 8, 7) + 0x10;
    }
    if (current < end) {
        return current;
    }
    return 0;
}

void SJ_SplitChunk(
    const SJCK* source, int nbyte, SJCK* first, SJCK* remainder) {
    *first = *source;
    remainder->len = first->len;
    if (first->len > nbyte) {
        first->len = nbyte;
    }
    remainder->len -= first->len;
    if (remainder->len == 0) {
        remainder->data = 0;
    } else {
        remainder->data = first->data + first->len;
    }
}
