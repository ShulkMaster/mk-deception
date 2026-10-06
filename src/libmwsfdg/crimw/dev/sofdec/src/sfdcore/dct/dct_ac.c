double cos(double);
const char* DCT_GetVerStr(void);

static double dctac_i_const[8][8];
static double dctac_f_const[8][8];
static const char* dctac_version_dummy;

static inline double dctac_Cos(double angle, int column) {
    return cos(angle * (0.5 + (double)column));
}

/* TODO: [near miss] 92.36%; unrolled multiply-add FPR/load scheduling remains;
 * accumulator scope, loop shape and declaration forms were already measured. */
void dctac_TransDouble(const double* input, double* output,
                       const double transform[8][8]) {
    double temporary[64];
    double sum;
    int row;
    int column;
    int k;

    for (row = 0; row < 8; row++) {
        for (column = 0; column < 8; column++) {
            sum = 0.0;
            for (k = 0; k < 8; k++) {
                sum += transform[k][column] * input[row * 8 + k];
            }
            temporary[row * 8 + column] = sum;
        }
    }
    for (column = 0; column < 8; column++) {
        for (row = 0; row < 8; row++) {
            sum = 0.0;
            for (k = 0; k < 8; k++) {
                sum += transform[k][row] * temporary[k * 8 + column];
            }
            output[row * 8 + column] = sum;
        }
    }
}

void DCT_AcIdctDouble(const double input[64], double output[64]) {
    dctac_TransDouble(input, output, dctac_i_const);
}

void DCT_AcFdctDouble(const double input[64], double output[64]) {
    dctac_TransDouble(input, output, dctac_f_const);
}

/* TODO: [near miss] 86.78%; retail BSS pool-base relocations remain; verify table layout. */
void DCT_AcInit(void) {
    double* inverse_element;
    double* forward_element;
    double* inverse_row;
    double* forward_column;
    int row;
    int column;
    double scale;
    double angle;

    dctac_version_dummy = DCT_GetVerStr();
    inverse_row = &dctac_i_const[0][0];
    forward_column = &dctac_f_const[0][0];
    for (row = 0; row < 8; row++, inverse_row += 8, forward_column++) {
        if (row == 0) {
            scale = 0.3535533905932738;
        } else {
            scale = 0.5;
        }
        angle = 0.39269908169872414 * row;
        inverse_element = inverse_row;
        forward_element = forward_column;
        for (column = 0; column < 8;
             column++, inverse_element++, forward_element += 8) {
            double value = scale * dctac_Cos(angle, column);
            *inverse_element = value;
            *forward_element = value;
        }
    }
}
