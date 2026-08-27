extern int adx_decode_output_mono_flag;

static const int AdxQtbl[16] = {
    0, 1, 2, 3, 4, 5, 6, 7,
    -8, -7, -6, -5, -4, -3, -2, -1,
};

static inline int clamp_sample(int sample) {
    if (sample > 32767 || sample < -32768) {
        if (sample < -32768) {
            sample = -32768;
        } else if (sample > 32767) {
            sample = 32767;
        }
    }
    return sample;
}

int ADX_DecodeSte4AsSte(const signed char*, int, short*, short*, short*,
                        short*, short, short, short*, short, short);
int ADX_DecodeSte4AsMono(const signed char*, int, short*, short*, short*,
                         short*, short, short, short*, short, short);

int ADX_DecodeSte4(const signed char* input, int numBlocks,
                   short* outputLeft, short delayLeft[2],
                   short* outputRight, short delayRight[2],
                   short coefficient0, short coefficient1,
                   short* randomState, short randomMultiplier,
                   short randomIncrement) {
    if (adx_decode_output_mono_flag == 0) {
        return ADX_DecodeSte4AsSte(input, numBlocks, outputLeft, delayLeft,
            outputRight, delayRight, coefficient0, coefficient1, randomState,
            randomMultiplier, randomIncrement);
    }
    return ADX_DecodeSte4AsMono(input, numBlocks, outputLeft, delayLeft,
        outputRight, delayRight, coefficient0, coefficient1, randomState,
        randomMultiplier, randomIncrement);
}

int ADX_DecodeSte4AsSte(const signed char* input, int numBlocks,
                        short* outputLeft, short delayLeft[2],
                        short* outputRight, short delayRight[2],
                        short coefficient0, short coefficient1,
                        short* randomState, short randomMultiplier,
                        short randomIncrement) {
    int block;
    int previousLeft = delayLeft[0];
    int olderLeft = delayLeft[1];
    int previousRight = delayRight[0];
    int olderRight = delayRight[1];
    const int* quantizer = AdxQtbl;

    for (block = 0; block < numBlocks / 2; block++, input += 36) {
        const signed char* leftData = input;
        const signed char* rightData = input + 18;
        short leftCode = *(const short*)leftData;
        short rightCode;
        int leftGain;
        int rightGain;
        int sample;

        if (leftCode & 0x8000) return block * 2;
        leftGain = ((leftCode ^ *randomState) & 0x1fff) + 1;
        *randomState = randomIncrement + *randomState * randomMultiplier;
        *randomState &= 0x7fff;
        rightCode = *(const short*)rightData;
        if (rightCode & 0x8000) return block * 2;
        rightGain = ((rightCode ^ *randomState) & 0x1fff) + 1;
        *randomState = randomIncrement + *randomState * randomMultiplier;
        *randomState &= 0x7fff;

        leftData += 2;
        rightData += 2;
        sample = 16;
        do {
            signed char leftPacked = *leftData++;
            signed char rightPacked = *rightData++;
            int decodedLeft = (leftPacked >> 4) * leftGain +
                ((coefficient0 * previousLeft + coefficient1 * olderLeft) >> 12);
            int decodedRight = (rightPacked >> 4) * rightGain +
                ((coefficient0 * previousRight + coefficient1 * olderRight) >> 12);

            decodedLeft = clamp_sample(decodedLeft);
            decodedRight = clamp_sample(decodedRight);
            *outputLeft++ = (short)decodedLeft;
            *outputRight++ = (short)decodedRight;
            olderLeft = decodedLeft;
            olderRight = decodedRight;
            previousLeft = clamp_sample(quantizer[leftPacked & 15] * leftGain +
                ((coefficient0 * decodedLeft + coefficient1 * previousLeft) >> 12));
            previousRight = clamp_sample(quantizer[rightPacked & 15] * rightGain +
                ((coefficient0 * decodedRight + coefficient1 * previousRight) >> 12));
            *outputLeft++ = (short)previousLeft;
            *outputRight++ = (short)previousRight;
        } while (--sample != 0);
    }
    delayLeft[0] = (short)previousLeft;
    delayLeft[1] = (short)olderLeft;
    delayRight[0] = (short)previousRight;
    delayRight[1] = (short)olderRight;
    return numBlocks;
}

int ADX_DecodeSte4AsMono(const signed char* input, int numBlocks,
                         short* outputLeft, short delayLeft[2],
                         short* outputRight, short delayRight[2],
                         short coefficient0, short coefficient1,
                         short* randomState, short randomMultiplier,
                         short randomIncrement) {
    int block;
    int previousLeft = delayLeft[0];
    int olderLeft = delayLeft[1];
    int previousRight = delayRight[0];
    int olderRight = delayRight[1];
    const int* quantizer = AdxQtbl;

    for (block = 0; block < numBlocks / 2; block++, input += 36) {
        const signed char* leftData = input;
        const signed char* rightData = input + 18;
        short leftCode = *(const short*)leftData;
        short rightCode;
        int leftGain;
        int rightGain;
        int sample;

        if (leftCode & 0x8000) return block * 2;
        leftGain = ((leftCode ^ *randomState) & 0x1fff) + 1;
        *randomState = randomIncrement + *randomState * randomMultiplier;
        *randomState &= 0x7fff;
        rightCode = *(const short*)rightData;
        if (rightCode & 0x8000) return block * 2;
        rightGain = ((rightCode ^ *randomState) & 0x1fff) + 1;
        *randomState = randomIncrement + *randomState * randomMultiplier;
        *randomState &= 0x7fff;

        leftData += 2;
        rightData += 2;
        sample = 16;
        do {
            signed char leftPacked = *leftData++;
            signed char rightPacked = *rightData++;
            int decodedLeft = (leftPacked >> 4) * leftGain +
                ((coefficient0 * previousLeft + coefficient1 * olderLeft) >> 12);
            int decodedRight = (rightPacked >> 4) * rightGain +
                ((coefficient0 * previousRight + coefficient1 * olderRight) >> 12);
            int mixed;

            decodedLeft = clamp_sample(decodedLeft);
            decodedRight = clamp_sample(decodedRight);
            olderLeft = decodedLeft;
            olderRight = decodedRight;
            mixed = clamp_sample(((decodedLeft + decodedRight) * 7) / 10);
            *outputLeft++ = (short)mixed;
            *outputRight++ = (short)mixed;
            previousLeft = clamp_sample(quantizer[leftPacked & 15] * leftGain +
                ((coefficient0 * decodedLeft + coefficient1 * previousLeft) >> 12));
            previousRight = clamp_sample(quantizer[rightPacked & 15] * rightGain +
                ((coefficient0 * decodedRight + coefficient1 * previousRight) >> 12));
            mixed = clamp_sample(((previousLeft + previousRight) * 7) / 10);
            *outputLeft++ = (short)mixed;
            *outputRight++ = (short)mixed;
        } while (--sample != 0);
    }
    delayLeft[0] = (short)previousLeft;
    delayLeft[1] = (short)olderLeft;
    delayRight[0] = (short)previousRight;
    delayRight[1] = (short)olderRight;
    return numBlocks;
}

int ADX_DecodeMono4(const signed char* input, int numBlocks, short* output,
                    short delay[2], short coefficient0, short coefficient1,
                    short* randomState, short randomMultiplier,
                    short randomIncrement) {
    int block;
    int previous = delay[0];
    int older = delay[1];
    const int* quantizer = AdxQtbl;

    for (block = 0; block < numBlocks; block++) {
        short code = *(const short*)input;
        int gain;
        int sample;

        if (code & 0x8000) return block;
        gain = ((code ^ *randomState) & 0x1fff) + 1;
        *randomState = randomIncrement + *randomState * randomMultiplier;
        *randomState &= 0x7fff;
        input += 2;
        sample = 16;
        do {
            signed char packed = *input++;
            int decoded = (packed >> 4) * gain +
                ((coefficient0 * previous + coefficient1 * older) >> 12);
            decoded = clamp_sample(decoded);
            *output++ = (short)decoded;
            older = decoded;
            previous = clamp_sample(quantizer[packed & 15] * gain +
                ((coefficient0 * decoded + coefficient1 * previous) >> 12));
            *output++ = (short)previous;
        } while (--sample != 0);
    }
    delay[0] = (short)previous;
    delay[1] = (short)older;
    return numBlocks;
}
