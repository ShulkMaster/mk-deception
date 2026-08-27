typedef float CFTMtx3D[3][3];
typedef unsigned char CFTConvTable[256];

static const float cft_inverse_scale[] = {1.0f};

void CFT_MakeMtx3D(CFTMtx3D left, CFTMtx3D right, CFTMtx3D product)
{
    product[0][0] = left[0][2] * right[2][0] +
                    (left[0][0] * right[0][0] + left[0][1] * right[1][0]);
    product[0][1] = left[0][2] * right[2][1] +
                    (left[0][0] * right[0][1] + left[0][1] * right[1][1]);
    product[0][2] = left[0][2] * right[2][2] +
                    (left[0][0] * right[0][2] + left[0][1] * right[1][2]);
    product[1][0] = left[1][2] * right[2][0] +
                    (left[1][0] * right[0][0] + left[1][1] * right[1][0]);
    product[1][1] = left[1][2] * right[2][1] +
                    (left[1][0] * right[0][1] + left[1][1] * right[1][1]);
    product[1][2] = left[1][2] * right[2][2] +
                    (left[1][0] * right[0][2] + left[1][1] * right[1][2]);
    product[2][0] = left[2][2] * right[2][0] +
                    (left[2][0] * right[0][0] + left[2][1] * right[1][0]);
    product[2][1] = left[2][2] * right[2][1] +
                    (left[2][0] * right[0][1] + left[2][1] * right[1][1]);
    product[2][2] = left[2][2] * right[2][2] +
                    (left[2][0] * right[0][2] + left[2][1] * right[1][2]);
}

void CFT_MakeInverseMtx3D(CFTMtx3D matrix, CFTMtx3D inverse)
{
    float b = matrix[0][1];
    float d = matrix[1][0];
    float f = matrix[1][2];
    float g = matrix[2][0];
    float a = matrix[0][0];
    float e = matrix[1][1];
    float c = matrix[0][2];
    float i = matrix[2][2];
    float h = matrix[2][1];
    float determinant = h * (c * d) + i * (a * e) + g * (b * f) -
                        (g * (c * e) + h * (a * f) + i * (b * d));

    inverse[0][0] = cft_inverse_scale[0] * ((e * i - f * h) / determinant);
    inverse[0][1] = cft_inverse_scale[0] * (-(b * i - c * h) / determinant);
    inverse[0][2] = cft_inverse_scale[0] * ((b * f - c * e) / determinant);
    inverse[1][0] = cft_inverse_scale[0] * (-(d * i - f * g) / determinant);
    inverse[1][1] = cft_inverse_scale[0] * ((a * i - c * g) / determinant);
    inverse[1][2] = cft_inverse_scale[0] * (-(a * f - c * d) / determinant);
    inverse[2][0] = cft_inverse_scale[0] * ((d * h - e * g) / determinant);
    inverse[2][1] = cft_inverse_scale[0] * (-(a * h - b * g) / determinant);
    inverse[2][2] = cft_inverse_scale[0] * ((a * e - b * d) / determinant);
}

void CFT_MakeInvConvTableCustom(CFTConvTable luma, CFTConvTable chroma_u,
                                CFTConvTable chroma_v)
{
    int index;
    int value = 0;
    int scaled;
    int divisor;
    int start;
    int value_range;

    index = 0;
    while (index < 16) {
        luma[index++] = value++;
        luma[index++] = value++;
        value++;
    }
    for (; index < 176; index += 2) {
        luma[index] = value;
        luma[index + 1] = value;
        value++;
    }
    for (; index < 192; index++) {
        luma[index] = value++;
    }
    for (; index < 256; index++) {
        luma[index] = value < 255 ? value : 255;
        value += 2;
    }

    index = 128;
    value = 128;
    while (index > 104) {
        chroma_u[index] = value;
        chroma_v[index] = value;
        chroma_u[index - 1] = value;
        chroma_v[index - 1] = value;
        chroma_u[index - 2] = value;
        chroma_v[index - 2] = value;
        index -= 3;
        value--;
    }
    scaled = index * value;
    divisor = index;
    for (; index >= 0; index--) {
        unsigned char converted = scaled / divisor;
        chroma_u[index] = converted;
        chroma_v[index] = converted;
        scaled -= value;
    }

    index = 128;
    value = 128;
    while (index < 152) {
        chroma_u[index] = value;
        chroma_v[index] = value;
        chroma_u[index + 1] = value;
        chroma_v[index + 1] = value;
        chroma_u[index + 2] = value;
        chroma_v[index + 2] = value;
        index += 3;
        value++;
    }
    value_range = 255 - value;
    divisor = 255 - index;
    start = index;
    for (; index < 256; index++) {
        unsigned char converted = value + value_range * (index - start) / divisor;
        chroma_u[index] = converted;
        chroma_v[index] = converted;
    }
}
