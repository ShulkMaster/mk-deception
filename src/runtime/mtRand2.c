#include "runtime/mtRand2.h"

#define N 624
#define M 397
#define MATRIX_A 0x9908b0dfU
#define UPPER_MASK 0x80000000U
#define LOWER_MASK 0x7fffffffU

unsigned int mt[N];
static unsigned int mag01[2] = {0x0U, MATRIX_A};

int mti;
int reseed_rnd_tbl;

unsigned int genlrand(void) {
    unsigned int y;
    int idx;

    idx = mti;
    y = mt[idx];

    y ^= y >> 11;
    y ^= (y << 7) & 0x9d2c5680U;
    y ^= (y << 15) & 0xefc60000U;
    mti++;

    y ^= y >> 18;
    mt[idx] = y;

    if (mti < N) {
        return y;
    }
    reseed_rnd_tbl = 1;
    mti = 0;
    return y;
}

/* TODO: [near miss] 98.39%; final twist accumulation improved; zero, state base and remaining tail GPRs differ. */
void reload_rnd_tbl(void) {
    int idx;
    unsigned int y;
    unsigned int final_word;

    for (idx = 0; idx < N - M; idx++) {
        y = (mt[idx] & UPPER_MASK) | (mt[idx + 1] & LOWER_MASK);
        mt[idx] = mt[idx + M] ^ (y >> 1) ^ mag01[y & 0x1U];
    }

    for (; idx < N - 1; idx++) {
        y = (mt[idx] & UPPER_MASK) | (mt[idx + 1] & LOWER_MASK);
        mt[idx] = mt[idx + M - N] ^ (y >> 1) ^ mag01[y & 0x1U];
    }

    mti = 0;
    final_word = mt[N - 1];
    final_word = (final_word & UPPER_MASK) | (mt[0] & LOWER_MASK);
    reseed_rnd_tbl = 0;
    mt[N - 1] = (final_word >> 1) ^ mt[M - 1] ^ mag01[final_word & 0x1U];
}

/* TODO: [near miss] 99.09%; constant setup order restored; first-loop r5/r6 allocation differs. */
void sgenrand(unsigned int seed) {
    unsigned int x;
    int idx;

    x = 0x5b3e9U;
    if (seed == 0) {
        seed = 0x12345678U;
    }
    mt[0] = seed;
    mti = idx = 1;
    for (; idx < N; idx++) {
        mt[idx] = mt[idx - 1] * 69069U;
    }

    mti = 1;
    for (idx = 1; idx < N; idx++) {
        mt[idx] ^= x;
        x = x * 0x159bU + 0x13e8bU;
        x ^= (x >> 9) ^ (x << 19);
    }
    mti = idx;
}
