#include "runtime/asm_sequences.inc"

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

asm long ADX_DecodeSte4AsSte(signed char* input, long numBlocks,
                             short* outputLeft, short delayLeft[2],
                             short* outputRight, short delayRight[2],
                             short coefficient0, short coefficient1,
                             short* randomState, short randomMultiplier,
                             short randomIncrement)
{
    SEQ_ADX_DecodeSte4AsSte();
}

asm int ADX_DecodeSte4AsMono(signed char* input, int numBlocks,
                             short* outputLeft, short delayLeft[2],
                             short* outputRight, short delayRight[2],
                             short coefficient0, short coefficient1,
                             short* randomState, short randomMultiplier,
                             short randomIncrement)
{
    SEQ_ADX_DecodeSte4AsMono();
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
