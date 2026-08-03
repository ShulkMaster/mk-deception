#include "runtime/cstring.h"

const char sud_ver_str[] =
    "\nCRI SUD/GC Ver.0.05 Build:Sep  3 2004 11:38:58\n";
static int sud_init_cnt;
static const char* sud_dummy;

void SUD_SearchSudDat(
    const char* data, int size, const char** header, int* header_size) {
    int index;
    int data_size;
    const char** output_header;
    int* output_size;
    const char* cursor;

    data_size = size;
    output_header = header;
    output_size = header_size;
    cursor = data;
    *output_header = 0;
    *output_size = 0;
    if (cursor == 0 || data_size <= 0) {
        return;
    }

    for (index = 0; index < data_size; cursor++, index++) {
        if (memcmp(cursor, "<", 1) == 0 &&
            memcmp(cursor, "<SUDPS_>", 8) == 0) {
            *output_header = cursor;
            *output_size = *output_header == 0 ? 0 : 0x23;
        }
    }
}

int SUD_AnalyTypeCcs(const char* data, int size) {
    if (data == 0 || size < 0) {
        return 0;
    }
    return strncmp(&data[0x13], "C", 1) == 0;
}

int SUD_AnalyTypeDivField(const char* data, int size) {
    if (data == 0 || size < 0) {
        return 0;
    }
    return strncmp(&data[0x12], "D", 1) == 0;
}

void SUD_Finish(void) {
    if (sud_init_cnt > 0) {
        sud_init_cnt--;
    }
}

void SUD_Init(void) {
    if (sud_init_cnt < 1) {
        sud_init_cnt++;
        sud_dummy = sud_ver_str;
    }
}
