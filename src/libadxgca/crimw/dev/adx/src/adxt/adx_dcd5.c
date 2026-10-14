extern int adx_decode_output_mono_flag;

static const long AdxQtbl[16] = {
    0, 1, 2, 3, 4, 5, 6, 7,
    -8, -7, -6, -5, -4, -3, -2, -1,
};

long ADX_DecodeSte4AsSte(signed char*, long, short*, short*, short*,
                        short*, short, short, short*, short, short);
int ADX_DecodeSte4AsMono(signed char*, int, short*, short*, short*,
                         short*, short, short, short*, short, short);

int ADX_DecodeSte4(const signed char* input, int numBlocks,
                   short* outputLeft, short delayLeft[2],
                   short* outputRight, short delayRight[2],
                   short coefficient0, short coefficient1,
                   short* randomState, short randomMultiplier,
                   short randomIncrement) {
    if (adx_decode_output_mono_flag == 0) {
        return ADX_DecodeSte4AsSte((signed char*)input, numBlocks, outputLeft, delayLeft,
            outputRight, delayRight, coefficient0, coefficient1, randomState,
            randomMultiplier, randomIncrement);
    }
    return ADX_DecodeSte4AsMono((signed char*)input, numBlocks, outputLeft, delayLeft,
        outputRight, delayRight, coefficient0, coefficient1, randomState,
        randomMultiplier, randomIncrement);
}

/* TODO: [near miss] 85.1842%; CTR/operations agree; saved predictor homes and
 * sample scheduling remain; canonical types/order do not close donor pin residue. */
long ADX_DecodeSte4AsSte(signed char* input, long numBlocks,
                        short* outputLeft, short delayLeft[2],
                        short* outputRight, short delayRight[2],
                        short coefficient0, short coefficient1,
                        short* randomState, short randomMultiplier,
                        short randomIncrement) {
    long olderLeft;
    long olderRight;
    long previousRight;
    long previousLeft;
    long block;
    long leftPacked;
    long rightPacked;
    short leftGain;
    short rightGain;
    long header;
    long decodedRight;
    long blockCount;
    long key;
    long sample;

    blockCount = numBlocks / 2;

    previousLeft = delayLeft[0];
    olderLeft = delayLeft[1];
    previousRight = delayRight[0];
    olderRight = delayRight[1];
    for (block = 0; block < blockCount; block++) {
        header = *(short *)input;
        if (header & 0x8000) {
            return block * 2;
        }
        key = *randomState;
        leftGain = ((header ^ key) & 0x1FFF) + 1;
        key = randomIncrement + key * randomMultiplier;
        *randomState = key;
        *randomState = *randomState & 0x7FFF;
        header = *(short *)(input + 0x12);
        if (header & 0x8000) {
            return block * 2;
        }
        key = *randomState;
        rightGain = ((header ^ key) & 0x1FFF) + 1;
        key = randomIncrement + key * randomMultiplier;
        *randomState = key;
        *randomState = *randomState & 0x7FFF;
        input += 2;
        for (sample = 0; sample < 16; sample++) {
            leftPacked = input[0];
            rightPacked = input[0x12];
            input++;
            olderLeft = (leftPacked >> 4) * leftGain +
                ((coefficient0 * previousLeft + coefficient1 * olderLeft) >> 12);
            if (olderLeft > 32767 || olderLeft < -32768) {
                if (olderLeft < -32768) {
                    olderLeft = -32768;
                } else if (olderLeft > 32767) {
                    olderLeft = 32767;
                }
            }
            decodedRight = (rightPacked >> 4) * rightGain;
            decodedRight += (coefficient0 * previousRight + coefficient1 * olderRight) >> 12;
            if (decodedRight > 32767 || decodedRight < -32768) {
                if (decodedRight < -32768) {
                    decodedRight = -32768;
                } else if (decodedRight > 32767) {
                    decodedRight = 32767;
                }
            }
            outputLeft[0] = olderLeft;
            leftPacked = AdxQtbl[leftPacked & 0xF];
            outputRight[0] = decodedRight;
            rightPacked = AdxQtbl[rightPacked & 0xF];
            previousLeft = leftPacked * leftGain +
                ((coefficient0 * olderLeft + coefficient1 * previousLeft) >> 12);
            if (previousLeft > 32767 || previousLeft < -32768) {
                if (previousLeft < -32768) {
                    previousLeft = -32768;
                } else if (previousLeft > 32767) {
                    previousLeft = 32767;
                }
            }
            previousRight = rightPacked * rightGain +
                ((coefficient0 * decodedRight + coefficient1 * previousRight) >> 12);
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
        }
        input += 0x12;
    }
    delayLeft[0] = previousLeft;
    delayLeft[1] = olderLeft;
    delayRight[0] = previousRight;
    delayRight[1] = olderRight;
    return numBlocks;
}

/* TODO: [breakthrough] 90.74468%; later older-left ownership restores block/packed homes;
 * header, LCG and mix owners plus entry scheduling remain. */
int ADX_DecodeSte4AsMono(signed char* input, int numBlocks,
                         short* outputLeft, short delayLeft[2],
                         short* outputRight, short delayRight[2],
                         short coefficient0, short coefficient1,
                         short* randomState, short randomMultiplier,
                         short randomIncrement) {
    long olderRight;
    long previousRight;
    long previousLeft;
    long block;
    long leftPacked;
    long rightPacked;
    short leftGain;
    short rightGain;
    long header;
    long mixed;
    long blockCount;
    long key;
    long sample;
    long olderLeft;

    blockCount = numBlocks / 2;

    previousLeft = delayLeft[0];
    olderLeft = delayLeft[1];
    previousRight = delayRight[0];
    olderRight = delayRight[1];
    for (block = 0; block < blockCount; block++) {
        header = *(short *)input;
        if (header & 0x8000) {
            return block * 2;
        }
        key = *randomState;
        leftGain = ((header ^ key) & 0x1FFF) + 1;
        key = randomIncrement + key * randomMultiplier;
        *randomState = key;
        *randomState = *randomState & 0x7FFF;
        header = *(short *)(input + 0x12);
        if (header & 0x8000) {
            return block * 2;
        }
        key = *randomState;
        rightGain = ((header ^ key) & 0x1FFF) + 1;
        key = randomIncrement + key * randomMultiplier;
        *randomState = key;
        *randomState = *randomState & 0x7FFF;
        input += 2;
        for (sample = 0; sample < 16; sample++) {
            leftPacked = input[0];
            rightPacked = input[0x12];
            input++;
            olderLeft = (leftPacked >> 4) * leftGain +
                ((coefficient0 * previousLeft + coefficient1 * olderLeft) >> 12);
            if (olderLeft > 32767 || olderLeft < -32768) {
                if (olderLeft < -32768) {
                    olderLeft = -32768;
                } else if (olderLeft > 32767) {
                    olderLeft = 32767;
                }
            }
            mixed = (rightPacked >> 4) * rightGain +
                ((coefficient0 * previousRight + coefficient1 * olderRight) >> 12);
            if (mixed > 32767 || mixed < -32768) {
                if (mixed < -32768) {
                    mixed = -32768;
                } else if (mixed > 32767) {
                    mixed = 32767;
                }
            }
            olderRight = mixed;
            mixed = (olderLeft + mixed) * 7 / 10;
            if (mixed > 32767 || mixed < -32768) {
                if (mixed < -32768) {
                    mixed = -32768;
                } else if (mixed > 32767) {
                    mixed = 32767;
                }
            }
            leftPacked = AdxQtbl[leftPacked & 0xF];
            outputRight[0] = mixed;
            rightPacked = AdxQtbl[rightPacked & 0xF];
            outputLeft[0] = mixed;

            previousLeft = leftPacked * leftGain +
                ((coefficient0 * olderLeft + coefficient1 * previousLeft) >> 12);
            if (previousLeft > 0x7FFF || previousLeft < -0x8000) {
                if (previousLeft < -0x8000) {
                    previousLeft = -0x8000;
                } else if (previousLeft > 0x7FFF) {
                    previousLeft = 0x7FFF;
                }
            }
            previousRight = rightPacked * rightGain +
                ((coefficient0 * olderRight + coefficient1 * previousRight) >> 12);
            if (previousRight > 32767 || previousRight < -32768) {
                if (previousRight < -32768) {
                    previousRight = -32768;
                } else if (previousRight > 32767) {
                    previousRight = 32767;
                }
            }
            mixed = (previousLeft + previousRight) * 7 / 10;
            if (mixed > 32767 || mixed < -32768) {
                if (mixed < -32768) {
                    mixed = -32768;
                } else if (mixed > 32767) {
                    mixed = 32767;
                }
            }
            outputRight[1] = mixed;
            outputRight += 2;
            outputLeft[1] = mixed;
            outputLeft += 2;
        }
        input += 0x12;
    }
    delayLeft[0] = previousLeft;
    delayLeft[1] = olderLeft;
    delayRight[0] = previousRight;
    delayRight[1] = olderRight;
    return numBlocks;
}

int ADX_DecodeMono4(const signed char* input, int numBlocks, short* output,
                    short delay[2], short coefficient0, short coefficient1,
                    short* randomState, short randomMultiplier,
                    short randomIncrement) {
    long block;
    long older;
    long previous;
    long sample;
    long code;
    long key;
    short gain;
    long packed;
    long decoded;
    long quantized;

    previous = delay[0];
    older = delay[1];
    for (block = 0; block < numBlocks; block++) {
        code = *(const short*)input;
        if (code & 0x8000) {
            return block;
        }
        key = *randomState;
        gain = ((code ^ key) & 0x1fff) + 1;
        key = randomIncrement + key * randomMultiplier;
        *randomState = key;
        *randomState = *randomState & 0x7fff;
        input += 2;
        for (sample = 0; sample < 16; sample++) {
            packed = input[0];
            input++;
            decoded = (packed >> 4) * gain +
                ((coefficient0 * previous + coefficient1 * older) >> 12);
            if (decoded > 32767 || decoded < -32768) {
                if (decoded < -32768) {
                    decoded = -32768;
                } else if (decoded > 32767) {
                    decoded = 32767;
                }
            }
            quantized = AdxQtbl[packed & 15];
            output[0] = decoded;
            previous = quantized * gain +
                ((coefficient0 * decoded + coefficient1 * previous) >> 12);
            if (previous > 32767 || previous < -32768) {
                if (previous < -32768) {
                    previous = -32768;
                } else if (previous > 32767) {
                    previous = 32767;
                }
            }
            output[1] = previous;
            older = decoded;
            output += 2;
        }
    }
    delay[0] = previous;
    delay[1] = older;
    return numBlocks;
}
