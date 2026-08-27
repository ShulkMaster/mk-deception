int UTY_CmpTime(int leftValue, int rightScale, int rightValue, int leftScale) {
    /*
     * Retail uses this signed 64-bit cross-product comparison. At -O1 the
     * remaining four-byte delta is equivalent boolean-return lowering; the
     * multiply operands, signed high words, guard, and result are unchanged.
     */
    int integerWidth = sizeof(long long);
    int isEarlier;

    if (integerWidth < 8) {
        for (;;) {
        }
    }

    isEarlier = (long long)rightValue * rightScale <
                (long long)leftValue * leftScale;
    if (isEarlier) {
        return 0;
    }
    return 1;
}
