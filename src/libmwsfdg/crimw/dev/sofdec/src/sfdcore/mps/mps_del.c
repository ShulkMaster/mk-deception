#include "cri/mps.h"

unsigned int MPS_CheckDelim(const unsigned char* data) {
    unsigned int code;

    if (data[0] == 0 && data[1] == 0 && data[2] == 1) {
        code = data[3];
        switch (code) {
        case 0xB9:
            return MPS_DELIM_END;
        case 0xBA:
            return MPS_DELIM_PACK;
        case 0xBB:
            return MPS_DELIM_SYSHD;
        default:
            if (code >= 0xBC) {
                return MPS_DELIM_PKET;
            }
            break;
        }
    }
    return 0;
}
