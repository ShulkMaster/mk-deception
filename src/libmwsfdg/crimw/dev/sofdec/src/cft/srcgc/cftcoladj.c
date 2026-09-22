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

/* TODO: [near miss] 97.755104%; RE4 scalar/lifetime order restores the cofactor schedule; remaining mismatches are register coloring. */
void CFT_MakeInverseMtx3D(CFTMtx3D matrix, CFTMtx3D inverse)
{
    float determinant;
    float scale;
    float a;
    float b;
    float c;
    float d;
    float e;
    float f;
    float g;
    float h;
    float i;

    a = matrix[0][0];
    b = matrix[0][1];
    c = matrix[0][2];
    d = matrix[1][0];
    e = matrix[1][1];
    f = matrix[1][2];
    g = matrix[2][0];
    h = matrix[2][1];
    i = matrix[2][2];
    scale = cft_inverse_scale[0];
    determinant = (a * e * i + b * f * g + c * d * h) -
                  (a * f * h + b * d * i + c * e * g);
    inverse[0][0] = scale * ((e * i - f * h) / determinant);
    inverse[0][1] = scale * (-(b * i - c * h) / determinant);
    inverse[0][2] = scale * ((b * f - c * e) / determinant);
    inverse[1][0] = scale * (-(d * i - f * g) / determinant);
    inverse[1][1] = scale * ((a * i - c * g) / determinant);
    inverse[1][2] = scale * (-(a * f - c * d) / determinant);
    inverse[2][0] = scale * ((d * h - e * g) / determinant);
    inverse[2][1] = scale * (-(a * h - b * g) / determinant);
    inverse[2][2] = scale * ((a * e - b * d) / determinant);
}

void CFT_MakeInvConvTableCustom(CFTConvTable luma, CFTConvTable chroma_u,
                                CFTConvTable chroma_v)
{
    int i;
    int j;
    int v;
    int t;

    j = 0;
    v = 0;
    while (j < 0x10) {
        luma[j++] = v++;
        luma[j++] = v;
        v += 2;
    }
    while (j < 0xB0) {
        luma[j] = v;
        luma[j + 1] = v;
        j += 2;
        v++;
    }
    while (j < 0xC0) {
        luma[j] = v;
        v++;
        j++;
    }
    while (j < 0x100) {
        t = 0xFF;
        if (v < 0xFF) {
            t = v;
        }
        luma[j] = t;
        v += 2;
        j++;
    }

    j = 0x80;
    v = j;
    while (j > 0x68) {
        chroma_u[j] = v;
        chroma_v[j] = v;
        chroma_u[j - 1] = v;
        chroma_v[j - 1] = v;
        chroma_u[j - 2] = v;
        chroma_v[j - 2] = v;
        j -= 3;
        v--;
    }
    for (i = j; i >= 0; i--) {
        chroma_u[i] = i * v / j;
        chroma_v[i] = i * v / j;
    }

    v = 0x80;
    t = v;
    while (v < 0x98) {
        chroma_u[v] = t;
        chroma_v[v] = t;
        chroma_u[v + 1] = t;
        chroma_v[v + 1] = t;
        chroma_u[v + 2] = t;
        chroma_v[v + 2] = t;
        v += 3;
        t++;
    }
    i = v;
    for (; v <= 0xFF; v++) {
        chroma_u[v] = t + (0xFF - t) * (v - i) / (0xFF - i);
        chroma_v[v] = t + (0xFF - t) * (v - i) / (0xFF - i);
    }
}
