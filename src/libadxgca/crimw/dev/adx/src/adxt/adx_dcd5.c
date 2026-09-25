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

/* TODO: [breakthrough] 67.243420%; typed key snapshots and output pairs retained; CTR/predictor allocation remains. */
int ADX_DecodeSte4AsSte(const signed char* input, int numBlocks,
                        short* outputLeft, short delayLeft[2],
                        short* outputRight, short delayRight[2],
                        short coefficient0, short coefficient1,
                        short* randomState, short randomMultiplier,
                        short randomIncrement) {
    int block;
    int blockCount = numBlocks / 2;
    int previousLeft = delayLeft[0];
    int olderLeft = delayLeft[1];
    int previousRight = delayRight[0];
    int olderRight = delayRight[1];
    const int* quantizer = AdxQtbl;

    for (block = 0; block < blockCount; block++) {
        const signed char* data;
        short leftCode = *(const short*)input;
        short rightCode;
        int key;
        int leftGain;
        int rightGain;
        int sample;

        if (leftCode & 0x8000) return block * 2;
        key = *randomState;
        leftGain = ((leftCode ^ key) & 0x1fff) + 1;
        *randomState = randomIncrement + key * randomMultiplier;
        *randomState &= 0x7fff;
        rightCode = *(const short*)(input + 18);
        if (rightCode & 0x8000) return block * 2;
        key = *randomState;
        rightGain = ((rightCode ^ key) & 0x1fff) + 1;
        *randomState = randomIncrement + key * randomMultiplier;
        *randomState &= 0x7fff;

        data = input + 2;
        sample = 16;
        do {
            signed char leftPacked = data[0];
            signed char rightPacked = data[18];
            int decodedLeft = (leftPacked >> 4) * leftGain +
                ((coefficient0 * previousLeft + coefficient1 * olderLeft) >> 12);
            int decodedRight = (rightPacked >> 4) * rightGain +
                ((coefficient0 * previousRight + coefficient1 * olderRight) >> 12);
            int leftQuantized;
            int rightQuantized;

            data++;

            decodedLeft = clamp_sample(decodedLeft);
            decodedRight = clamp_sample(decodedRight);
            leftQuantized = quantizer[leftPacked & 15];
            outputLeft[0] = (short)decodedLeft;
            rightQuantized = quantizer[rightPacked & 15];
            outputRight[0] = (short)decodedRight;
            previousLeft = clamp_sample(leftQuantized * leftGain +
                ((coefficient0 * decodedLeft + coefficient1 * previousLeft) >> 12));
            previousRight = clamp_sample(rightQuantized * rightGain +
                ((coefficient0 * decodedRight + coefficient1 * previousRight) >> 12));
            outputLeft[1] = (short)previousLeft;
            outputLeft += 2;
            outputRight[1] = (short)previousRight;
            outputRight += 2;
            olderLeft = decodedLeft;
            olderRight = decodedRight;
        } while (--sample != 0);
        input = data + 18;
    }
    delayLeft[0] = (short)previousLeft;
    delayLeft[1] = (short)olderLeft;
    delayRight[0] = (short)previousRight;
    delayRight[1] = (short)olderRight;
    return numBlocks;
}

/* TODO: [breakthrough] 69.101070%; block count, paired stores, and right-key
 * ownership are explicit; input traversal and inner-loop lowering remain. */
int ADX_DecodeSte4AsMono(const signed char* input, int numBlocks,
                         short* outputLeft, short delayLeft[2],
                         short* outputRight, short delayRight[2],
                         short coefficient0, short coefficient1,
                         short* randomState, short randomMultiplier,
                         short randomIncrement) {
    int block;
    int blockCount = numBlocks / 2;
    int previousLeft = delayLeft[0];
    int olderLeft = delayLeft[1];
    int previousRight = delayRight[0];
    int olderRight = delayRight[1];
    const int* quantizer = AdxQtbl;

    for (block = 0; block < blockCount; block++, input += 36) {
        const signed char* leftData = input;
        const signed char* rightData = input + 18;
        short leftCode = *(const short*)leftData;
        short rightCode;
        int leftGain;
        int rightGain;
        short rightKey;
        int sample;

        if (leftCode & 0x8000) return block * 2;
        leftGain = ((leftCode ^ *randomState) & 0x1fff) + 1;
        *randomState = randomIncrement + *randomState * randomMultiplier;
        *randomState &= 0x7fff;
        rightCode = *(const short*)rightData;
        if (rightCode & 0x8000) return block * 2;
        rightKey = *randomState;
        rightGain = ((rightCode ^ rightKey) & 0x1fff) + 1;
        *randomState = randomIncrement + rightKey * randomMultiplier;
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
            short mixed;
            int leftQuantized;
            int rightQuantized;

            decodedLeft = clamp_sample(decodedLeft);
            decodedRight = clamp_sample(decodedRight);
            olderLeft = decodedLeft;
            olderRight = decodedRight;
            mixed = clamp_sample(((decodedLeft + decodedRight) * 7) / 10);
            outputRight[0] = (short)mixed;
            outputLeft[0] = (short)mixed;
            leftQuantized = quantizer[leftPacked & 15];
            rightQuantized = quantizer[rightPacked & 15];
            previousLeft = clamp_sample(leftQuantized * leftGain +
                ((coefficient0 * decodedLeft + coefficient1 * previousLeft) >> 12));
            previousRight = clamp_sample(rightQuantized * rightGain +
                ((coefficient0 * decodedRight + coefficient1 * previousRight) >> 12));
            mixed = clamp_sample(((previousLeft + previousRight) * 7) / 10);
            outputRight[1] = (short)mixed;
            outputLeft[1] = (short)mixed;
            outputRight += 2;
            outputLeft += 2;
        } while (--sample != 0);
    }
    delayLeft[0] = (short)previousLeft;
    delayLeft[1] = (short)olderLeft;
    delayRight[0] = (short)previousRight;
    delayRight[1] = (short)olderRight;
    return numBlocks;
}

/* TODO: [breakthrough needed] 84.470590%; direct first-sample clamp and
 * delayed history match; retail CTR/frame lowering still needs source evidence. */
int ADX_DecodeMono4(const signed char* input, int numBlocks, short* output,
                    short delay[2], short coefficient0, short coefficient1,
                    short* randomState, short randomMultiplier,
                    short randomIncrement) {
    int block;
    int previous = delay[0];
    int older = delay[1];
    int predictor0 = coefficient0;
    int predictor1 = coefficient1;

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
                ((predictor0 * previous + predictor1 * older) >> 12);
            int quantized;
            if (decoded > 32767 || decoded < -32768) {
                if (decoded < -32768) {
                    decoded = -32768;
                } else if (decoded > 32767) {
                    decoded = 32767;
                }
            }
            quantized = AdxQtbl[packed & 15];
            output[0] = (short)decoded;
            previous = clamp_sample(quantized * gain +
                ((predictor0 * decoded + predictor1 * previous) >> 12));
            output[1] = (short)previous;
            older = decoded;
            output += 2;
        } while (--sample != 0);
    }
    delay[0] = (short)previous;
    delay[1] = (short)older;
    return numBlocks;
}
