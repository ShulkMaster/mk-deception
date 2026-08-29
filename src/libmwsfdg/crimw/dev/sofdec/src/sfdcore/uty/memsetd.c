#include "sofdec/uty_mem.h"

void UTY_MemsetDword(unsigned int* destination, unsigned int value, unsigned int count) {
    unsigned int* end = destination + count;
    unsigned int remainder = (count & 15) + 1;

    while (--remainder != 0) {
        *--end = value;
    }

    /* This unrolled block may have been generated automatically somehow. */
    count = (count >> 4) + 1;
    while (--count != 0) {
        end[-1] = value;
        end[-2] = value;
        end[-3] = value;
        end[-4] = value;
        end[-5] = value;
        end[-6] = value;
        end[-7] = value;
        end[-8] = value;
        end[-9] = value;
        end[-10] = value;
        end[-11] = value;
        end[-12] = value;
        end[-13] = value;
        end[-14] = value;
        end[-15] = value;
        end -= 16;
        *end = value;
    }
}
