#include "dolphin/cache.h"

void ADXF_Ocbi(void* address, unsigned long size)
{
    DCInvalidateRange(address, size);
}
