#ifndef MKD_SOFDEC_SFD_ERROR_H
#define MKD_SOFDEC_SFD_ERROR_H

#include "sofdec/sfd_player_types.h"
typedef void (*SfdErrorCallback)(int object, int error);

typedef struct SfdErrorInfo {
    SfdErrorCallback callback;
    int callback_object;
    int first_error;
    int field_0C;
    int field_10;
} SfdErrorInfo;

typedef char SfdErrorInfoSizeCheck[
    sizeof(SfdErrorInfo) == 0x14 ? 1 : -1];

void SFLIB_InitErrInf(SfdErrorInfo* info);
int SFLIB_CheckHn(SfdHandle* handle);
int SFLIB_SetErr(SfdHandle* handle, int error);
int SFD_SetErrFn(SfdHandle* handle, SfdErrorCallback callback, int object);
void SFLIB_LockCs(int* token);
void SFLIB_UnlockCs(int* token);

#endif
