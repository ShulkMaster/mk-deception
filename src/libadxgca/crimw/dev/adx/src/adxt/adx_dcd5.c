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

/* TODO: [breakthrough needed] 79.82895%; traversal and saturation restored; CTR, frame and predictor lowering remain. */
int ADX_DecodeSte4AsSte(const signed char* input, int numBlocks,
                        short* outputLeft, short delayLeft[2],
                        short* outputRight, short delayRight[2],
                        short coefficient0, short coefficient1,
                        short* randomState, short randomMultiplier,
                        short randomIncrement) {
    int predictor0 = coefficient0;
    int predictor1 = coefficient1;
    int block;
    int blockCount = numBlocks / 2;
    int previousLeft = delayLeft[0];
    int olderLeft = delayLeft[1];
    int previousRight = delayRight[0];
    int olderRight = delayRight[1];
    const int* quantizer = AdxQtbl;

    for (block = 0; block < blockCount; block++) {
        short leftCode = *(const short*)input;
        short rightCode;
        int key;
        short leftGain;
        short rightGain;
        unsigned int sample;

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

        input += 2;
        sample = 16;
        do {
            signed char leftPacked = input[0];
            signed char rightPacked = input[18];
            int decodedRight;
            int leftQuantized;
            int rightQuantized;

            input++;
            olderLeft = (leftPacked >> 4) * leftGain +
                ((predictor0 * previousLeft + predictor1 * olderLeft) >> 12);
            if (olderLeft > 32767 || olderLeft < -32768) {
                if (olderLeft < -32768) {
                    olderLeft = -32768;
                } else if (olderLeft > 32767) {
                    olderLeft = 32767;
                }
            }
            decodedRight = (rightPacked >> 4) * rightGain +
                ((predictor0 * previousRight + predictor1 * olderRight) >> 12);
            if (decodedRight > 32767 || decodedRight < -32768) {
                if (decodedRight < -32768) {
                    decodedRight = -32768;
                } else if (decodedRight > 32767) {
                    decodedRight = 32767;
                }
            }
            outputLeft[0] = olderLeft;
            leftQuantized = quantizer[leftPacked & 15];
            outputRight[0] = decodedRight;
            rightQuantized = quantizer[rightPacked & 15];
            previousLeft = leftQuantized * leftGain +
                ((predictor0 * olderLeft + predictor1 * previousLeft) >> 12);
            if (previousLeft > 32767 || previousLeft < -32768) {
                if (previousLeft < -32768) {
                    previousLeft = -32768;
                } else if (previousLeft > 32767) {
                    previousLeft = 32767;
                }
            }
            previousRight = rightQuantized * rightGain +
                ((predictor0 * decodedRight + predictor1 * previousRight) >> 12);
            if (previousRight > 32767 || previousRight < -32768) {
                if (previousRight < -32768) {
                    previousRight = -32768;
                } else if (previousRight > 32767) {
                    previousRight = 32767;
                }
            }
            outputLeft[1] = previousLeft;
            olderRight = decodedRight;
            outputLeft += 2;
            outputRight[1] = previousRight;
            outputRight += 2;
        } while (--sample != 0);
        input += 18;
    }
    delayLeft[0] = previousLeft;
    delayLeft[1] = olderLeft;
    delayRight[0] = previousRight;
    delayRight[1] = olderRight;
    return numBlocks;
}

/* TODO: [breakthrough needed] 69.11%; retail CTR/invariant profile unresolved; recover whole-unit mode before cursor/gain tuning. */
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
            outputRight[0] = mixed;
            outputLeft[0] = mixed;
            leftQuantized = quantizer[leftPacked & 15];
            rightQuantized = quantizer[rightPacked & 15];
            previousLeft = clamp_sample(leftQuantized * leftGain +
                ((coefficient0 * decodedLeft + coefficient1 * previousLeft) >> 12));
            previousRight = clamp_sample(rightQuantized * rightGain +
                ((coefficient0 * decodedRight + coefficient1 * previousRight) >> 12));
            mixed = clamp_sample(((previousLeft + previousRight) * 7) / 10);
            outputRight[1] = mixed;
            outputLeft[1] = mixed;
            outputRight += 2;
            outputLeft += 2;
        } while (--sample != 0);
    }
    delayLeft[0] = previousLeft;
    delayLeft[1] = olderLeft;
    delayRight[0] = previousRight;
    delayRight[1] = olderRight;
    return numBlocks;
}

/* TODO: [breakthrough needed] 85.53%; quantizer base retained; retail short/CTR/frame shape remains unresolved. */
int ADX_DecodeMono4(const signed char* input, int numBlocks, short* output,
                    short delay[2], short coefficient0, short coefficient1,
                    short* randomState, short randomMultiplier,
                    short randomIncrement) {
    int block;
    int previous = delay[0];
    int older = delay[1];
    int predictor0 = coefficient0;
    int predictor1 = coefficient1;
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
                ((predictor0 * previous + predictor1 * older) >> 12);
            int quantized;
            if (decoded > 32767 || decoded < -32768) {
                if (decoded < -32768) {
                    decoded = -32768;
                } else if (decoded > 32767) {
                    decoded = 32767;
                }
            }
            quantized = quantizer[packed & 15];
            output[0] = decoded;
            previous = clamp_sample(quantized * gain +
                ((predictor0 * decoded + predictor1 * previous) >> 12));
            output[1] = previous;
            older = decoded;
            output += 2;
        } while (--sample != 0);
    }
    delay[0] = previous;
    delay[1] = older;
    return numBlocks;
}
