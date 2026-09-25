#include "cri/mpv.h"

/* TODO: [near miss] 92.118645%; donor state-one branches and direct input
 * cursor agree; stop this round at signed-byte/state register lowering. */
const unsigned char* MPV_SearchDelim(const unsigned char* data, int length,
                                     int mask) {
    const unsigned char* end = data + length;
    int state = 0;

    while (data < end) {
        signed char byte = (signed char)*data++;

        switch (state) {
        case 0:
            if (byte == 0) {
                state = 1;
            }
            break;
        case 1:
            if (byte == 0) {
                state = 2;
            } else {
                state = 0;
            }
            break;
        case 2:
            if (byte == 1) {
                state = 3;
            } else if (byte != 0) {
                state = 0;
            }
            break;
        case 3:
            if (mask & MPV_CheckDelim(data - 4)) {
                return (void*)(data - 4);
            }
            state = 0;
            break;
        }
    }
    return 0;
}

/* TODO: [near miss] 90.234375%; donor branches, direct cursor and byte
 * lifetime agree; stop this round at signed-byte/state register lowering. */
void* MPV_BsearchDelim(const unsigned char* data, int length, int mask) {
    const unsigned char* end = data - length;
    signed char byte;
    int state = 0;

    while (end < data) {
        byte = (signed char)*--data;

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
                if (mask & MPV_CheckDelim(data)) {
                    return (void*)data;
                }
                state = 0;
            } else if (byte != 1) {
                state = 1;
            } else {
                state = 2;
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
