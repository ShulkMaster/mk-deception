/* Explicit aggregate initialization preserves retail .data placement. */
int gcg_ci_rdmode[1] = {0};
char gcg_ci_root_dir[256];

void gcCiSetRdMode(int unused0, int unused1, int unused2, int mode)
{
    gcg_ci_rdmode[0] = mode;
}
