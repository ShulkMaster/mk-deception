#include "cri/mpv.h"

const unsigned char* MPV_SearchDelim(const unsigned char* data, int length,
                                     int mask) {
    const unsigned char* current = data;
    const unsigned char* end = data + length;
    int state = 0;

    while (current < end) {
        signed char byte = (signed char)*current++;

        switch (state) {
        case 0:
            if (byte == 0) {
                state = 1;
            }
            break;
        case 1:
            state = byte == 0 ? 2 : 0;
            break;
        case 2:
            if (byte == 1) {
                state = 3;
            } else if (byte != 0) {
                state = 0;
            }
            break;
        case 3:
            if (mask & MPV_CheckDelim(current - 4)) {
                return (void*)(current - 4);
            }
            state = 0;
            break;
        }
    }
    return 0;
}

/* TODO: [breakthrough needed] 75.6875%; signed-pointer reconstruction regresses; reverse-loop/state lowering remains structurally different. */
void* MPV_BsearchDelim(const unsigned char* data, int length, int mask) {
    const unsigned char* current = data;
    const unsigned char* end = data - length;
    int state = 0;

    while (end < current) {
        signed char byte = (signed char)*--current;

        switch (state) {
        case 0:
            state = 1;
            break;
        case 1:
            if (byte == 1) {
                state = 2;
            }
            break;
        case 2:
            if (byte == 0) {
                state = 3;
            } else if (byte != 1) {
                state = 1;
            }
            break;
        case 3:
            if (byte == 0) {
                if (mask & MPV_CheckDelim(current)) {
                    return (void*)current;
                }
                state = 0;
            } else {
                state = byte == 1 ? 2 : 1;
            }
            break;
        }
    }
    return 0;
}

int MPV_CheckDelim(const unsigned char* data) {
    int delimiter;
    int result;

    delimiter = (data[0] << 8) | data[1];
    delimiter <<= 8;
    delimiter |= data[2];
    delimiter <<= 8;
    delimiter |= data[3];
    if (delimiter == 0x100) {
        result = 4;
    } else if (delimiter == 0x101) {
        result = 3;
    } else if (delimiter > 0x101 && delimiter <= 0x1AF) {
        result = 1;
    } else if (delimiter == 0x1B2) {
        result = 0x20;
    } else if (delimiter == 0x1B3) {
        result = 0x40;
    } else if (delimiter == 0x1B5) {
        result = 0x10;
    } else if (delimiter == 0x1B7) {
        result = 0x80;
    } else if (delimiter == 0x1B8) {
        result = 8;
    } else {
        result = 0;
    }
    return result;
}
