#include "cri/mps.h"

int MPS_CheckDelim(const unsigned char* data) {
    unsigned int code;

    if (data[0] == 0 && data[1] == 0 && data[2] == 1) {
        code = data[3];
        switch (code) {
        case 0xB9:
            return 0x80000;
        case 0xBA:
            return 0x10000;
        case 0xBB:
            return 0x20000;
        default:
            if (code >= 0xBC) {
                return 0x40000;
            }
            break;
        }
    }
    return 0;
}
