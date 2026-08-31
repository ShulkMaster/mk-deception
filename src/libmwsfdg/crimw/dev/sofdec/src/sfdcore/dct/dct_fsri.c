#include "runtime/cstring.h"
#include "sofdec/dct_fsri.h"

const char* DCT_GetVerStr(void);
void DCT_AcInit(void);
void DCT_AcIdctDouble(const double input[8][8], double output[8][8]);

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
/* Retail's object ends with one named word of BSS alignment padding. */
unsigned int gap_06_80497E8C_bss;

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
        /* The original library treats an invalid scan value as fatal. */
        while (1) {}
    }
    return result;
}

static inline void dctFsriStoreSparseCoefficient(int coefficient, int index,
                                                  const double* value)
{
    PreIDCT[dctFsriScanIndex(coefficient)][index] = (float)*value;
}

/*
 * Soft ceiling: retail performs both inverse-transform passes with paired-
 * single operations and quantized paired stores. The compiler exposes no
 * portable C intrinsic for that kernel, so this preserves its scalar lanes,
 * arithmetic order, output permutation, and rounding behavior.
 */
static void DCT_FsriTransCore(DctFsriParams* params, int coded_block_pattern)
{
    float* coefficients = params->coefficients;
    DctFsriBlock** output_blocks = params->output_blocks;
    float* workspace = params->workspace;
    int block;

    for (block = 0; block < 6; block++) {
        if (coded_block_pattern < 0) {
            DctFsriBlock* output = output_blocks[block];

            if (params->block_nonzero[block] == 0) {
                float dc = coefficients[0];
                signed short value;
                unsigned int packed;
                int pair;

                if (dc < 0.0) {
                    value = (signed short)(dc - 0.5f);
                } else {
                    value = (signed short)(dc + 0.5f);
                }
                packed = (unsigned short)value;
                packed |= packed << 16;
                for (pair = 0; pair < 32; pair++) {
                    output->packed[pair] = packed;
                }
            } else {
                int row_pair;
                int column;

                /* The input interleaves each pair of rows. */
                for (row_pair = 0; row_pair < 4; row_pair++) {
                    int lane;
                    for (lane = 0; lane < 2; lane++) {
                        const float* source = coefficients + row_pair * 16 + lane;
                        float x0 = source[0];
                        float x1 = source[2];
                        float x2 = source[4];
                        float x3 = source[6];
                        float x4 = source[8];
                        float x5 = source[10];
                        float x6 = source[12];
                        float x7 = source[14];
                        float even_sum_26 = x2 + x6;
                        float even_delta_26 = (x2 - x6) * B0TableOrg[0];
                        float even_delta_04 = x0 - x4;
                        float even_sum_04 = x0 + x4;
                        float odd_delta_53 = x5 - x3;
                        float odd_sum_53 = x5 + x3;
                        float odd_delta_17 = x1 - x7;
                        float odd_sum_17 = x1 + x7;
                        float even_low;
                        float even_high;
                        float even_mid_low;
                        float even_mid_high;
                        float odd_outer;
                        float odd_inner;
                        float odd_low;
                        float odd_high;
                        float* destination = workspace + (row_pair * 2 + lane) * 8;

                        even_delta_26 -= even_sum_26;
                        even_low = even_sum_04 - even_sum_26;
                        even_high = even_sum_04 + even_sum_26;
                        even_mid_low = even_delta_04 - even_delta_26;
                        even_mid_high = even_delta_04 + even_delta_26;

                        odd_outer = (odd_sum_17 - odd_sum_53) * B0TableOrg[0];
                        odd_high = odd_sum_17 + odd_sum_53;
                        odd_inner = (odd_delta_53 - odd_delta_17) * B0TableOrg[6];
                        odd_delta_17 *= B0TableOrg[4];
                        odd_delta_53 *= B0TableOrg[2];
                        odd_delta_17 -= odd_inner;
                        odd_delta_53 -= odd_inner;
                        odd_inner = odd_delta_17 - odd_high;
                        odd_outer -= odd_inner;
                        odd_low = odd_delta_53 - odd_outer;

                        /* Paired-single merges leave each row in the order
                         * consumed by the column pass. */
                        destination[0] = even_high + odd_high;
                        destination[2] = even_mid_low + odd_outer;
                        destination[4] = even_low - odd_low;
                        destination[6] = even_mid_high - odd_inner;
                        destination[1] = even_mid_high + odd_inner;
                        destination[3] = even_low + odd_low;
                        destination[5] = even_mid_low - odd_outer;
                        destination[7] = even_high - odd_high;
                    }
                }

                for (column = 0; column < 8; column++) {
                    float x0 = workspace[column];
                    float x1 = workspace[8 + column];
                    float x2 = workspace[16 + column];
                    float x3 = workspace[24 + column];
                    float x4 = workspace[32 + column];
                    float x5 = workspace[40 + column];
                    float x6 = workspace[48 + column];
                    float x7 = workspace[56 + column];
                    float even_sum_26 = x2 + x6;
                    float even_delta_26 = (x2 - x6) * B0TableOrg[0];
                    float even_delta_04 = x0 - x4;
                    float even_sum_04 = x0 + x4;
                    float odd_delta_53 = x5 - x3;
                    float odd_sum_53 = x5 + x3;
                    float odd_delta_17 = x1 - x7;
                    float odd_sum_17 = x1 + x7;
                    float even_low;
                    float even_high;
                    float even_mid_low;
                    float even_mid_high;
                    float odd_outer;
                    float odd_inner;
                    float odd_low;
                    float odd_high;
                    float value0;
                    float value1;
                    float value2;
                    float value3;
                    float value4;
                    float value5;
                    float value6;
                    float value7;

                    even_delta_26 -= even_sum_26;
                    even_low = even_sum_04 - even_sum_26;
                    even_high = even_sum_04 + even_sum_26;
                    even_mid_low = even_delta_04 - even_delta_26;
                    even_mid_high = even_delta_04 + even_delta_26;

                    odd_outer = (odd_sum_17 - odd_sum_53) * B0TableOrg[0];
                    odd_high = odd_sum_17 + odd_sum_53;
                    odd_inner = (odd_delta_53 - odd_delta_17) * B0TableOrg[6];
                    odd_delta_17 *= B0TableOrg[4];
                    odd_delta_53 *= B0TableOrg[2];
                    odd_delta_17 -= odd_inner;
                    odd_delta_53 -= odd_inner;
                    odd_inner = odd_delta_17 - odd_high;
                    odd_outer -= odd_inner;
                    odd_low = odd_delta_53 - odd_outer;

                    value0 = even_high + odd_high;
                    value1 = even_mid_low + odd_outer;
                    value2 = even_low - odd_low;
                    value3 = even_mid_high - odd_inner;
                    value4 = even_mid_high + odd_inner;
                    value5 = even_low + odd_low;
                    value6 = even_mid_low - odd_outer;
                    value7 = even_high - odd_high;

                    output->samples[column] = (signed short)(value0 +
                        (value0 >= 0.0 ? B0TableOrg[8] : B0TableOrg[10]));
                    output->samples[8 + column] = (signed short)(value4 +
                        (value4 >= 0.0 ? B0TableOrg[8] : B0TableOrg[10]));
                    output->samples[16 + column] = (signed short)(value1 +
                        (value1 >= 0.0 ? B0TableOrg[8] : B0TableOrg[10]));
                    output->samples[24 + column] = (signed short)(value5 +
                        (value5 >= 0.0 ? B0TableOrg[8] : B0TableOrg[10]));
                    output->samples[32 + column] = (signed short)(value2 +
                        (value2 >= 0.0 ? B0TableOrg[8] : B0TableOrg[10]));
                    output->samples[40 + column] = (signed short)(value6 +
                        (value6 >= 0.0 ? B0TableOrg[8] : B0TableOrg[10]));
                    output->samples[48 + column] = (signed short)(value3 +
                        (value3 >= 0.0 ? B0TableOrg[8] : B0TableOrg[10]));
                    output->samples[56 + column] = (signed short)(value7 +
                        (value7 >= 0.0 ? B0TableOrg[8] : B0TableOrg[10]));
                }
            }
        }
        coded_block_pattern = (int)((unsigned int)coded_block_pattern << 1);
        coefficients += 64;
    }
}

void DCT_FsriInitScanTbl(const signed char source[64], signed char destination[64])
{
    int index;
    for (index = 0; index < 64; index++) {
        destination[index] = (signed char)dctFsriScanIndex(source[index]);
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
    /* Retail programs GQR7 for signed 16-bit paired-single stores. */
}

/* Retail rematerializes the scan row for every transformed coefficient. */
#pragma opt_loop_invariants off
static void initSparseTbl(void)
{
    union DctCoefficientMatrix {
        double matrix[8][8];
        double coefficients[64];
    } source, destination;
    int coefficient;

    memset(PreIDCT, 0, sizeof(PreIDCT));
    DCT_AcInit();

    for (coefficient = 0; coefficient < 64; coefficient++) {
        int index;
        for (index = 0; index < 64; index++) {
            if (index == coefficient) {
                source.coefficients[index] = 1.0 / sfsd_scale_tbl[index];
            } else {
                source.coefficients[index] = 0.0;
            }
        }

        DCT_AcIdctDouble(source.matrix, destination.matrix);

        for (index = 0; index < 64; index++) {
            dctFsriStoreSparseCoefficient(coefficient, index,
                                          &destination.coefficients[index]);
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
            (float)sfsd_scale_tbl[index];
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
