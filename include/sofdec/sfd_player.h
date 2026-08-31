#ifndef MKD_SOFDEC_SFD_PLAYER_H
#define MKD_SOFDEC_SFD_PLAYER_H

#include "sofdec/sfd_library.h"

typedef void (*SfdPlayerRecordFrameFn)(SfdHandle* handle, void* frame);
typedef void (*SfdPlayerConditionFn)(SfdHandle* handle, int parameter,
                                     int* picture_counts);
typedef struct SfdPlayerPtsInfo {
    long long pts;
    int size;
} SfdPlayerPtsInfo;
typedef int (*SfdPlayerSetPtsInfoFn)(unsigned int state[3],
                                     const SfdPlayerPtsInfo* info);

typedef struct SfdAdxtParameters {
    int stream_buffer_size;
    int stream_buffer_extra_size;
    void* stream_buffer;
    int maximum_channels;
    int decoder_work_size;
    int decoder_buffer_size;
    void* decoder_buffer;
} SfdAdxtParameters;

typedef char SfdAdxtParametersSizeCheck[
    sizeof(SfdAdxtParameters) == 0x1C ? 1 : -1];

int SFSET_GetCond(SfdHandle* handle, int condition);
int SFD_GetTrHn(SfdHandle* handle, int transport_index, void** output);
int SFD_GetCond(SfdHandle* handle, int condition, int* value);
int SFD_SetCond(SfdHandle* handle, int condition, int value);
int SFD_GetHnStat(SfdHandle* handle);
int SFPL2_Standby(SfdHandle* handle);
int SFPL2_Pause(SfdHandle* handle, int paused);
void SFTIM_InitHn(SfdHandle* handle, SfdTimerState* state);
void SFTIM_VbIn(void);
void SFSEE_InitHn(SfdSeekState* state);
void SFSEE_ExecServer(SfdHandle* handle);
void SFSEE_FixAvPlay(SfdHandle* handle, int video_enabled,
                     int audio_enabled);
int SFD_EntrySeek(SfdHandle* handle, SfdHandle* source);
int SFD_SetByteRate(SfdHandle* handle, int byte_rate);
int SFD_SetFileSize(SfdHandle* handle, int file_size);
int SFD_SetTotTime(SfdHandle* handle, int value, int scale);
int SFD_SetSeekPos(SfdHandle* handle, int position);
int SFD_SetUsrTimeFn(SfdHandle* handle, SfdTimeSourceFn callback);
int SFD_SetUsrIsSkipFn(SfdHandle* handle, SfdTimerCallback callback);
int SFD_SetExtClockFn(SfdHandle* handle, SfdExternalClockFn callback, int arg0,
                      int arg1);
int SFD_GetFps(SfdHandle* handle, int* frame_rate);
int SFD_GetTime(SfdHandle* handle, int* value, int* scale);
void SFD_SetSpeed(SfdHandle* handle, int speed);
void SFD_SetAdxtPara(SfdAdxtParameters* parameters);
int SFD_GetOutVol(SfdHandle* handle);
void SFD_SetOutVol(SfdHandle* handle, int volume);
int SFD_GetOutPan(SfdHandle* handle, int channel);
void SFD_SetOutPan(SfdHandle* handle, int channel, int pan);
void SFD_SetVideoPts(SfdHandle* handle, void* entries, int buffer_size);
int SFMPV_SaveCond(SfdHandle* handle, void* conditions, int count);
void SFMPV_RestoreCond(SfdHandle* handle, const void* conditions, int count);

int SFD_SetSupplySj(SfdHandle* handle, const SfdBufferSupply* supply);
void SFD_RelFrm(SfdHandle* handle, void* frame);
int SFD_GetFrm(SfdHandle* handle, void** frame);
int SFD_TermSupply(SfdHandle* handle);
int SFPLY_GetResetFlg(void);
int SFD_Stop(SfdHandle* handle);
int SFD_Start(SfdHandle* handle);
int SFD_Destroy(SfdHandle* handle);
SfdHandle* SFD_Create(SfdCreateConfig* create,
                     const void* transport_buffer_setup);
int SFD_ExecOne(SfdHandle* handle);
int SFD_IsSvrWait(void);
void SFPLY_AddSkipPic(SfdHandle* handle, int count, int parameter);
void SFPLY_AddDecPic(SfdHandle* handle, int count, int parameter);
int SFD_IsHnSvrWait(SfdHandle* handle);
void SFD_VbIn(void);
void SFPLY_Init(void);

extern SfdPlayerRecordFrameFn SFPLY_recordgetfrm;
extern int sfply_last_hnctrl_wksiz;
extern SfdPlayerSetPtsInfoFn SFPLY_SetPtsInfo;
extern void (*SFPLY_ResetPtsm)(unsigned int* pts);
extern const unsigned int SFPLY_cond_dfl[101];

#endif
