#include "dolphin/dvd.h"
#include "dolphin/os.h"

static unsigned long ErrorTable[18] = {
    0x00000000, 0x00023A00, 0x00062800, 0x00030200, 0x00031100,
    0x00052000, 0x00052001, 0x00052100, 0x00052400, 0x00052401,
    0x00052402, 0x000B5A01, 0x00056300, 0x00020401, 0x00020400,
    0x00040800, 0x00100007, 0x00000000,
};

static unsigned char ErrorCode2Num(unsigned long error_code)
{
    unsigned long index;

    for (index = 0; index < 18; index++) {
        if (error_code == ErrorTable[index]) {
            return index;
        }
    }

    if (error_code >= 0x100000 && error_code <= 0x100008) {
        return 17;
    }

    return 29;
}

static unsigned char ConvertError(unsigned long error)
{
    unsigned long status_code;
    unsigned long error_code;
    unsigned char error_number;

    if (error == 0x01234567) {
        return 0xFF;
    }
    if (error == 0x01234568) {
        return 0xFE;
    }

    status_code = error >> 24;
    error_code = error & 0x00FFFFFF;
    error_number = ErrorCode2Num(error_code);
    if (status_code >= 6) {
        status_code = 6;
    }

    return status_code * 30 + error_number;
}

void __DVDStoreErrorCode(unsigned long error)
{
    OSSramEx* sram;
    unsigned char error_number;

    error_number = ConvertError(error);
    sram = __OSLockSramEx();
    sram->dvdErrorCode = error_number;
    __OSUnlockSramEx(1);
}
