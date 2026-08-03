#ifndef MOVIE_MWMOVIE_PLATFORM_H
#define MOVIE_MWMOVIE_PLATFORM_H

#ifdef __cplusplus
extern "C" {
#endif

void __mwMovie_initVideo(void);
void __mwMovie_shutdownVideo(void);
void __mwMovie_syncFrame(void);
void ADXM_ExecMain(void);
void initADXwithDVD(const char* path, int audio_enable);
void initADXwithPC(const char* path, int audio_enable);
void initADXwithMEM(int audio_enable);
void shutdownADX(void);

#ifdef __cplusplus
}
#endif

#endif
