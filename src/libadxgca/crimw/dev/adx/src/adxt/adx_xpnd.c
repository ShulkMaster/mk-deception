#include "cri/adx_dcd.h"
#include "cri/adx_xpnd.h"

extern void* memset(void* destination, int value, unsigned long size);
extern int ADX_DecodeMono4(const signed char* input, int num_blocks,
                           short* output, short delay[2], short coefficient0,
                           short coefficient1, short* random_state,
                           short random_multiplier, short random_increment);
extern int ADX_DecodeSte4(const signed char* input, int num_blocks,
                          short* output_left, short delay_left[2],
                          short* output_right, short delay_right[2],
                          short coefficient0, short coefficient1,
                          short* random_state, short random_multiplier,
                          short random_increment);

int adxpd_internal_error;
AdxXpnd adxpd_obj[16];
int gap_06_804B5A24_bss;

void ADXPD_ExecHndl(AdxXpnd* decoder)
{
    if (decoder->status == 1) {
        decoder->status = 2;
    }

    if (decoder->status == 2) {
        if (decoder->params.channel_count == 1) {
            decoder->num_decoded_blocks = ADX_DecodeMono4(
                decoder->params.input, decoder->params.num_blocks,
                decoder->params.output_left, decoder->delay[0],
                decoder->coefficients[0], decoder->coefficients[1],
                &decoder->random_state, decoder->random_multiplier,
                decoder->random_increment);
        } else {
            decoder->num_decoded_blocks = ADX_DecodeSte4(
                decoder->params.input, decoder->params.num_blocks,
                decoder->params.output_left, decoder->delay[0],
                decoder->params.output_right, decoder->delay[1],
                decoder->coefficients[0], decoder->coefficients[1],
                &decoder->random_state, decoder->random_multiplier,
                decoder->random_increment);

            if ((decoder->num_decoded_blocks % 2) == 1) {
                adxpd_internal_error = 1;
            }
        }

        decoder->status = 3;
    }
}

int ADXPD_GetNumBlk(AdxXpnd* decoder)
{
    return decoder->num_decoded_blocks;
}

void ADXPD_Reset(AdxXpnd* decoder)
{
    if (decoder->status == 3) {
        decoder->status = 0;
    }
}

void ADXPD_Stop(AdxXpnd* decoder)
{
    decoder->status = 0;
    memset(decoder->delay, 0, sizeof(decoder->delay));
}

void ADXPD_Start(AdxXpnd* decoder)
{
    if (decoder->status == 0) {
        decoder->num_decoded_blocks = 0;
        decoder->status = 1;
    }
}

int ADXPD_EntryPl2(AdxXpnd* decoder, const signed char* input, int num_blocks,
                   short* output_left, short* output_right)
{
    if (decoder->status == 0) {
        decoder->params.channel_count = 1;
        decoder->params.input = input;
        decoder->params.num_blocks = num_blocks;
        decoder->params.output_left = output_left;
        decoder->params.output_right = output_right;
        return 1;
    }

    return 0;
}

int ADXPD_EntrySte(AdxXpnd* decoder, const signed char* input, int num_blocks,
                   short* output_left, short* output_right)
{
    if (decoder->status == 0) {
        decoder->params.channel_count = 2;
        decoder->params.input = input;
        decoder->params.num_blocks = num_blocks;
        decoder->params.output_left = output_left;
        decoder->params.output_right = output_right;
        return 1;
    }

    return 0;
}

int ADXPD_EntryMono(AdxXpnd* decoder, const signed char* input, int num_blocks,
                    short* output_left, short* output_right)
{
    if (decoder->status == 0) {
        decoder->params.channel_count = 1;
        decoder->params.input = input;
        decoder->params.num_blocks = num_blocks;
        decoder->params.output_left = output_left;
        decoder->params.output_right = output_right;
        return 1;
    }

    return 0;
}

int ADXPD_GetStat(AdxXpnd* decoder)
{
    return decoder->status;
}

void ADXPD_Destroy(AdxXpnd* decoder)
{
    if (decoder != 0) {
        decoder->used = 0;
        memset(decoder, 0, sizeof(*decoder));
    }
}

void ADXPD_GetExtPrm(AdxXpnd* decoder, short* random_state,
                     short* random_multiplier, short* random_increment)
{
    *random_state = decoder->random_state;
    *random_multiplier = decoder->random_multiplier;
    *random_increment = decoder->random_increment;
}

void ADXPD_SetExtPrm(AdxXpnd* decoder, short random_state,
                     short random_multiplier, short random_increment)
{
    decoder->random_state = random_state;
    decoder->random_multiplier = random_multiplier;
    decoder->random_increment = random_increment;
}

void ADXPD_GetDly(AdxXpnd* decoder, short delay_left[2],
                  short delay_right[2])
{
    delay_left[0] = decoder->delay[0][0];
    delay_right[0] = decoder->delay[0][1];
    delay_left[1] = decoder->delay[1][0];
    delay_right[1] = decoder->delay[1][1];
}

void ADXPD_SetDly(AdxXpnd* decoder, short delay_left[2],
                  short delay_right[2])
{
    decoder->delay[0][0] = delay_left[0];
    decoder->delay[0][1] = delay_right[0];
    decoder->delay[1][0] = delay_left[1];
    decoder->delay[1][1] = delay_right[1];
}

void ADXPD_SetCoef(AdxXpnd* decoder, int sample_rate, int coefficient)
{
    ADX_GetCoefficient(coefficient, sample_rate, &decoder->coefficients[0],
                       &decoder->coefficients[1]);
}

AdxXpnd* ADXPD_Create(void)
{
    int index;
    AdxXpnd* candidate;
    AdxXpnd* decoder;

    candidate = adxpd_obj;
    for (index = 0; index < 16; candidate++, index++) {
        if (candidate->used == 0) {
            break;
        }
    }

    if (index == 16) {
        return 0;
    }

    decoder = &adxpd_obj[index];
    memset(decoder, 0, sizeof(*decoder));
    decoder->used = 1;
    decoder->index = index;
    decoder->mode = 0;
    decoder->status = 0;
    ADX_GetCoefficient(500, 44100, &decoder->coefficients[0],
                       &decoder->coefficients[1]);
    memset(decoder->delay, 0, sizeof(decoder->delay));

    return decoder;
}

void ADXPD_Init(void)
{
    memset(adxpd_obj, 0, sizeof(adxpd_obj));
}
