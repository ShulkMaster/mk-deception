typedef struct SfxDecoder {
    unsigned char reserved_00[0x60];
    const char* picture_user_data;
    int picture_user_data_size;
} SfxDecoder;

int SUD_AnalyTypeCcs(const char*, int);
int SUD_AnalyTypeDivField(const char*, int);
void SUD_Finish(void);
void SUD_Init(void);

int SFX_GetTypeCcs(SfxDecoder* decoder) {
    return SUD_AnalyTypeCcs(
        decoder->picture_user_data, decoder->picture_user_data_size);
}

int SFX_GetTypeDivField(SfxDecoder* decoder) {
    return SUD_AnalyTypeDivField(
        decoder->picture_user_data, decoder->picture_user_data_size);
}

void SFX_SetPicUsrDat(SfxDecoder* decoder, const char* data, int size) {
    decoder->picture_user_data = data;
    decoder->picture_user_data_size = size;
}

void SFXSUD_Finish(void) {
    SUD_Finish();
}

void SFXSUD_Init(void) {
    SUD_Init();
}
