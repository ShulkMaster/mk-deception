unsigned int UTY_MulDiv(int multiplicand, int multiplier, int divisor) {
    if (divisor == 0) {
        /* Soft ceiling: 99.35% - two saturation-path GPR assignments differ. */
        unsigned int limit = 0x80000000u;
        unsigned int sign = (unsigned int)(multiplicand ^ multiplier) >> 31;
        return sign != 0 ? limit : limit - 1;
    }

    return (unsigned int)(((long long)multiplicand * multiplier) / divisor);
}
