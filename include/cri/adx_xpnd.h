#ifndef CRI_ADX_XPND_H
#define CRI_ADX_XPND_H

typedef struct AdxXpndParams {
    int channel_count;
    const signed char* input;
    int num_blocks;
    short* output_left;
    short* output_right;
} AdxXpndParams;

typedef struct AdxXpnd {
    int used;
    int index;
    int mode;
    int status;
    int num_decoded_blocks;
    AdxXpndParams params;
    short delay[2][2];
    short coefficients[2];
    short random_state;
    short random_multiplier;
    short random_increment;
    short reserved;
} AdxXpnd;

void ADXPD_ExecHndl(AdxXpnd* decoder);
int ADXPD_GetNumBlk(AdxXpnd* decoder);
void ADXPD_Reset(AdxXpnd* decoder);
void ADXPD_Stop(AdxXpnd* decoder);
void ADXPD_Start(AdxXpnd* decoder);
int ADXPD_EntryPl2(AdxXpnd* decoder, const signed char* input, int num_blocks,
                   short* output_left, short* output_right);
int ADXPD_EntrySte(AdxXpnd* decoder, const signed char* input, int num_blocks,
                   short* output_left, short* output_right);
int ADXPD_EntryMono(AdxXpnd* decoder, const signed char* input, int num_blocks,
                    short* output_left, short* output_right);
int ADXPD_GetStat(AdxXpnd* decoder);
void ADXPD_Destroy(AdxXpnd* decoder);
void ADXPD_GetExtPrm(AdxXpnd* decoder, short* random_state,
                     short* random_multiplier, short* random_increment);
void ADXPD_SetExtPrm(AdxXpnd* decoder, short random_state,
                     short random_multiplier, short random_increment);
void ADXPD_GetDly(AdxXpnd* decoder, short delay_left[2],
                  short delay_right[2]);
void ADXPD_SetDly(AdxXpnd* decoder, short delay_left[2],
                  short delay_right[2]);
void ADXPD_SetCoef(AdxXpnd* decoder, int sample_rate, int coefficient);
AdxXpnd* ADXPD_Create(void);
void ADXPD_Init(void);

#endif
