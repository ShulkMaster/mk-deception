#include "runtime/cstring.h"
#include "sofdec/dct_fsri.h"
#include "runtime/asm_sequences.inc"

const char* DCT_GetVerStr(void);
void DCT_AcInit(void);
void DCT_AcIdctDouble(const double input[64], double output[64]);

static const double scale8[8] = {
    0.3535533905932738,
    0.4903926402016152,
    0.46193976625564337,
    0.4157348061512726,
    0.3535533905932738,
    0.2777851165098011,
    0.1913417161825449,
    0.09754516100806414,
};

static float B0TableOrg[12] = {
    1.4142135381698608f, 1.4142135381698608f,
    2.613126039505005f, 2.613126039505005f,
    1.0823922157287598f, 1.0823922157287598f,
    0.7653668522834778f, 0.7653668522834778f,
    0.5f, 0.5f,
    -0.5f, -0.5f,
};

static float PreIDCT[64][64];
static double sfsd_scale_tbl[64];
static const char* dctfsri_version_dummy;

static inline int dctFsriScanIndex(int index)
{
    int row = index / 8;
    int offset = index % 8;
    int result;

    if ((row % 2) == 0) {
        offset *= 2;
    } else {
        row--;
        offset = offset * 2 + 1;
    }
    result = offset + row * 8;

    if ((result < 0) || (result >= 256)) {
        while (1) {}
    }
    return result;
}

static inline void dctFsriStoreSparseCoefficient(int coefficient, int index,
                                                  const double* value)
{
    PreIDCT[dctFsriScanIndex(coefficient)][index] = *value;
}

static void DCT_FsriTransCore(DctFsriParams* params, int coded_block_pattern)
{
    s8* flags = params->block_nonzero;
    register float* blk;
    DctFsriBlock** tbl;
    register float* work;
    register float* src;
    register float* dst;
    register s32 cnt;
    register DctFsriBlock* o;
    register float* p;
    register __vec2x32float__ x0, c1, c2, c3, c4, c5, c6, x7, x8, x9, x10, x11, x12, x13, x31;
    s32 i;
    int k;
    float dc;
    s32 n;
    u32* out;

    work = params->workspace;
    p = B0TableOrg;
    asm { SEQ_DCT_FsriTransCore_LoadConsts(p, c1, c2, c3, c4, c5, c6) }
    blk = params->coefficients;
    tbl = params->output_blocks;
    for (i = 0; i < 6; i++) {
        if (coded_block_pattern < 0) {
            if (flags[0] == 0) {
                dc = blk[0];
                out = (u32*)tbl[0];
                if (dc < 0.0) {
                    n = (s16)(dc - 0.5f);
                } else {
                    n = (s16)(0.5f + dc);
                }
                n = (n << 16) | (u16)n;
                out += 32;
                for (k = 0; k < 32; k++) {
                    *--out = n;
                }
            } else {
                o = tbl[0];
                asm {
                    SEQ_DCT_FsriTransCore_Idct(src, blk, dst, work, cnt, o, x0, x7, x8, x9, x10, x11, x12, x13,
                                               x31, c1, c2, c3, c4, c5, c6)
                }
            }
        }
        coded_block_pattern <<= 1;
        blk += 64;
        tbl++;
        flags++;
    }
}

void DCT_FsriInitScanTbl(const signed char source[64], signed char destination[64])
{
    int index;
    for (index = 0; index < 64; index++) {
        destination[index] = dctFsriScanIndex(source[index]);
    }
}

void DCT_FsriTransCbp(DctFsriParams* params)
{
    DCT_FsriTransCore(params, params->coded_block_pattern);
}

void DCT_FsriTrans6Blk(DctFsriParams* params)
{
    DCT_FsriTransCore(params, -1);
}

void DCT_FsriSetGqr(void)
{
    asm { SEQ_DCT_FsriSetGqr_Block() }
}

#pragma opt_loop_invariants off
static void initSparseTbl(void)
{
    double source[64];
    double destination[64];
    int coefficient;

    memset(PreIDCT, 0, sizeof(PreIDCT));
    DCT_AcInit();

    for (coefficient = 0; coefficient < 64; coefficient++) {
        int index;
        for (index = 0; index < 64; index++) {
            if (index == coefficient) {
                source[index] = 1.0 / sfsd_scale_tbl[index];
            } else {
                source[index] = 0.0;
            }
        }

        DCT_AcIdctDouble(source, destination);

        for (index = 0; index < 64; index++) {
            dctFsriStoreSparseCoefficient(coefficient, index,
                                          &destination[index]);
        }
    }
}
#pragma opt_loop_invariants reset

void DCT_FsriInitPa(DctFsriParams* params)
{
    memset(params, 0, sizeof(*params));
}

void DCT_FsriInitScaleTbl(float scale_table[64])
{
    int index;
    for (index = 0; index < 64; index++) {
        scale_table[dctFsriScanIndex(index)] =
            sfsd_scale_tbl[index];
    }
}

void DCT_FsriInit(void)
{
    int row;
    int column;

    dctfsri_version_dummy = DCT_GetVerStr();
    for (row = 0; row < 8; row++) {
        for (column = 0; column < 8; column++) {
            sfsd_scale_tbl[row * 8 + column] = scale8[row] * scale8[column];
        }
    }
    initSparseTbl();
}
