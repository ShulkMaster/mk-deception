#include "sofdec/uty_mem.h"

void UTY_MemcpyDword(unsigned int* destination, const unsigned int* source,
                     unsigned int count) {
    unsigned int remainder = (count & 15) + 1;
    unsigned int blocks;
    unsigned int word0;
    unsigned int word1;
    unsigned int word2;
    unsigned int word3;

    while (--remainder != 0) {
        *destination++ = *source++;
    }

    blocks = (count >> 4) + 1;
    while (--blocks != 0) {
        word0 = source[0];
        word1 = source[1];
        word2 = source[2];
        word3 = source[3];
        destination[0] = word0;
        destination[1] = word1;
        destination[2] = word2;
        destination[3] = word3;
        word0 = source[4];
        word1 = source[5];
        word2 = source[6];
        word3 = source[7];
        destination[4] = word0;
        destination[5] = word1;
        destination[6] = word2;
        destination[7] = word3;
        word0 = source[8];
        word1 = source[9];
        word2 = source[10];
        word3 = source[11];
        destination[8] = word0;
        destination[9] = word1;
        destination[10] = word2;
        destination[11] = word3;
        word0 = source[12];
        word1 = source[13];
        word2 = source[14];
        word3 = source[15];
        destination[12] = word0;
        destination[13] = word1;
        destination[14] = word2;
        destination[15] = word3;
        source += 16;
        destination += 16;
    }
}
