int UTY_CmpTime(int count1, int scale1, int count2, int scale2)
{
    int ret;
    int size = sizeof(long long);

    if (size < 8) {
        for (;;) {
        }
    }

    if ((long long)count1 * scale2 <= (long long)count2 * scale1) {
        ret = 1;
    } else {
        ret = 0;
    }
    return ret;
}
