#ifndef MKD_SOFDEC_SFD_TIMER_H
#define MKD_SOFDEC_SFD_TIMER_H

#include "sofdec/sfd_player_types.h"

extern const int SFTIM_prate[9];

typedef struct SfdTimerSummary {
    long long total;
    long long minimum;
    long long maximum;
    int count;
    int field_1C;
} SfdTimerSummary;

typedef struct SfdTimerLibraryWork {
    int field_00;
    int field_04;
    int source;
} SfdTimerLibraryWork;

typedef struct SfdTimeCode {
    int frame_rate_code;
    int drop_frame;
    int hours;
    int minutes;
    int seconds;
    int frames;
    int frame_offset;
    short reserved_1C;
    short subframe;
} SfdTimeCode;

typedef void (*SfdTimeCodeConvertFn)(int rate, const SfdTimeCode* timecode,
                                     int* value, int* scale);

typedef char SfdTimerSummarySizeCheck[
    sizeof(SfdTimerSummary) == 0x20 ? 1 : -1];
typedef char SfdTimerLibraryWorkSizeCheck[
    sizeof(SfdTimerLibraryWork) == 0x0C ? 1 : -1];
typedef char SfdTimeCodeSizeCheck[sizeof(SfdTimeCode) == 0x20 ? 1 : -1];

void SFTMR_AddTsum(SfdTimerSummary* summary, long long elapsed);
void SFTMR_InitTsum(SfdTimerSummary* summary);
void SFTIM_Init(SfdTimerLibraryWork* work, int source);
void SFTIM_Finish(SfdTimerLibraryWork* work);
int SFTIM_GetTime(SfdHandle* handle, int* seconds, int* subsecond);
int SFTIM_GetTimeSub(SfdHandle* handle, int* seconds, int* subsecond);
int SFTIM_IsStagnant(SfdHandle* handle);
int SFD_CmpTime(int lhs_seconds, int lhs_subsecond, int rhs_seconds,
                int rhs_subsecond);
int SFTIM_GetSpeed(SfdHandle* handle);
void SFTIM_SetSpeed(SfdHandle* handle, int speed);
void SFTIM_SetTimeFn(SfdHandle* handle, SfdTimeSourceFn callback, int index);
int SFTIM_ChkRegularTime(SfdHandle* handle, int* value, int* scale);
void SFTIM_SetStartTime(SfdTimerState* state, int value, int scale);
int SFTIM_IsVideoTerm(SfdHandle* handle);
int SFTIM_IsGetFrmTimeTunit(SfdHandle* handle, int value, int scale);
int SFTIM_IsGetFrmTime(SfdHandle* handle, const SfdFrameTime* frame_time);
void SFTIM_GetTimeOneFrmVideo(SfdHandle* handle, int* value, int* scale);
void SFTIM_InitTtu(SfdTimerTimeUnit* unit, int scale);
int SFTIM_GetNextItime(SfdTimerState* state, int time);
void SFTIM_UpdateItime(SfdTimerState* state, int time);
void SFTIM_Tc2Time(const SfdTimeCode* timecode, int* value, int* scale);
int SFTIM_GetVideoStartSample(SfdTimerState* state, int sample_rate,
                              int* using_time_unit);
unsigned int SFTIM_GetAudioStartSample(SfdTimerState* state, int sample_rate);
void SFTIM_Pause(SfdHandle* handle, int state);

#endif
