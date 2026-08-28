long long UTY_MulDivRound64(long long value, long long scale, long long divisor) {
    int sign;
    long long result;

    if (divisor == 0) {
        if ((value ^ scale) >= 0) {
            return 0x7FFFFFFFFFFFFFFFLL;
        }
        return -0x7FFFFFFFFFFFFFFFLL - 1;
    }

    sign = 1;
    if (value < 0) {
        value = -value;
        sign = -1;
    }
    if (scale < 0) {
        scale = -scale;
        sign = -sign;
    }
    if (divisor < 0) {
        divisor = -divisor;
        sign = -sign;
    }

    result = (value * scale + divisor / 2) / divisor;
    if (sign < 0) {
        result = -result;
    }
    return result;
}
