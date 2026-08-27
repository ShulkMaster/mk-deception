double cos(double);
const char* DCT_GetVerStr(void);

static double dctac_i_const[8][8];
static double dctac_f_const[8][8];
static const char* dctac_version_dummy;

void dctac_TransDouble(const double* input, double* output,
                       const double* transform) {
    double temporary[64];
    int row;
    int column;

    for (row = 0; row < 64; row += 8) {
        int row1 = row + 1;
        int row2 = row + 2;
        int row3 = row + 3;
        int row4 = row + 4;
        int row5 = row + 5;
        int row6 = row + 6;
        int row7 = row + 7;

        for (column = 0; column < 8; column += 2) {
            double sample0 = input[row];
            double sample1 = input[row1];
            double sample2 = input[row2];
            double sample3 = input[row3];
            double sample4 = input[row4];
            double sample5 = input[row5];
            double sample6 = input[row6];
            double sample7 = input[row7];
            double value0 = 0.0;
            double value1 = 0.0;

            value0 += transform[column] * sample0;
            value0 += transform[column + 8] * sample1;
            value0 += transform[column + 16] * sample2;
            value0 += transform[column + 24] * sample3;
            value0 += transform[column + 32] * sample4;
            value0 += transform[column + 40] * sample5;
            value0 += transform[column + 48] * sample6;
            value0 += transform[column + 56] * sample7;
            value1 += transform[column + 1] * sample0;
            value1 += transform[column + 9] * sample1;
            value1 += transform[column + 17] * sample2;
            value1 += transform[column + 25] * sample3;
            value1 += transform[column + 33] * sample4;
            value1 += transform[column + 41] * sample5;
            value1 += transform[column + 49] * sample6;
            value1 += transform[column + 57] * sample7;
            temporary[row + column] = value0;
            temporary[row + column + 1] = value1;
        }
    }

    for (column = 0; column < 8; column++) {
        int column1 = column + 8;
        int column2 = column + 16;
        int column3 = column + 24;
        int column4 = column + 32;
        int column5 = column + 40;
        int column6 = column + 48;
        int column7 = column + 56;

        for (row = 0; row < 8; row += 2) {
            double sample0 = temporary[column];
            double sample1 = temporary[column1];
            double sample2 = temporary[column2];
            double sample3 = temporary[column3];
            double sample4 = temporary[column4];
            double sample5 = temporary[column5];
            double sample6 = temporary[column6];
            double sample7 = temporary[column7];
            double value0 = 0.0;
            double value1 = 0.0;

            value0 += transform[row] * sample0;
            value0 += transform[row + 8] * sample1;
            value0 += transform[row + 16] * sample2;
            value0 += transform[row + 24] * sample3;
            value0 += transform[row + 32] * sample4;
            value0 += transform[row + 40] * sample5;
            value0 += transform[row + 48] * sample6;
            value0 += transform[row + 56] * sample7;
            value1 += transform[row + 1] * sample0;
            value1 += transform[row + 9] * sample1;
            value1 += transform[row + 17] * sample2;
            value1 += transform[row + 25] * sample3;
            value1 += transform[row + 33] * sample4;
            value1 += transform[row + 41] * sample5;
            value1 += transform[row + 49] * sample6;
            value1 += transform[row + 57] * sample7;
            output[row * 8 + column] = value0;
            output[(row + 1) * 8 + column] = value1;
        }
    }
}

void DCT_AcIdctDouble(const double input[8][8], double output[8][8]) {
    dctac_TransDouble(&input[0][0], &output[0][0], &dctac_i_const[0][0]);
}

void DCT_AcInit(void) {
    int row;
    int column;

    dctac_version_dummy = DCT_GetVerStr();
    for (row = 0; row < 8; row++) {
        double scale = row == 0 ? 0.3535533905932738 : 0.5;
        for (column = 0; column < 8; column++) {
            double value = scale * cos(
                0.39269908169872414 * row * (0.5 + column));
            dctac_i_const[row][column] = value;
            dctac_f_const[column][row] = value;
        }
    }
}
