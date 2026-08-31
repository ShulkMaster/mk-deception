#ifndef MKD_SOFDEC_MPV_ERROR_H
#define MKD_SOFDEC_MPV_ERROR_H

typedef struct MPVContext MPVContext;
typedef void (*MPVErrorCallback)(int object, int error);

typedef struct MPVErrorInfo {
    MPVErrorCallback callback;
    int callback_object;
    int first_error;
    int field_0C;
    int field_10;
} MPVErrorInfo;

int MPVERR_SetCode(MPVContext* handle, int error);
int MPV_SetErrFunc(MPVContext* handle, MPVErrorCallback callback, int object);
void MPVERR_InitErrInf(MPVErrorInfo* info);
void MPVERR_Init(void);

typedef char MPVErrorInfoSizeCheck[
    sizeof(MPVErrorInfo) == 0x14 ? 1 : -1];

#endif
