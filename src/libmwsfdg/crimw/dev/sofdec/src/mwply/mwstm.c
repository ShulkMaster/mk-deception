#include "cri/adxt_internal.h"

extern int ADXSTM_GetStat(ADXStream* stream);
extern void ADXSTM_StopNw(ADXStream* stream);
extern void ADXSTM_ReleaseFileNw(ADXStream* stream);
extern int ADXSTM_Start(ADXStream* stream);
extern void ADXSTM_BindFileNw(ADXStream* stream, const char* path, int offset,
                              int length, int end_position);
extern void ADXSTM_SetEos(ADXStream* stream, int end_position);
extern void ADXSTM_Destroy(ADXStream* stream);
extern ADXStream* ADXSTM_Create(void* source, int priority);
extern int ADXSTM_SetBufSize(ADXStream* stream, int minimum_size,
                             int maximum_size);

int MWSTM_GetStat(ADXStream* stream)
{
    return ADXSTM_GetStat(stream);
}

void MWSTM_ReqStop(ADXStream* stream)
{
    ADXSTM_StopNw(stream);
    ADXSTM_ReleaseFileNw(stream);
}

int MWSTM_ReqStart(ADXStream* stream)
{
    return ADXSTM_Start(stream);
}

void MWSTM_SetFileRange(ADXStream* stream, const char* path, int offset,
                        int length, int end_position)
{
    ADXSTM_ReleaseFileNw(stream);
    ADXSTM_BindFileNw(stream, path, offset, length, end_position);
    ADXSTM_SetEos(stream, end_position);
}

void MWSTM_Destroy(ADXStream* stream)
{
    ADXSTM_Destroy(stream);
}

ADXStream* MWSTM_Create(void* source)
{
    return ADXSTM_Create(source, 0);
}

void MWSTM_SetFlowLimit(ADXStream* stream, int minimum_size, int maximum_size)
{
    if (stream != 0) {
        ADXSTM_SetBufSize(stream, minimum_size, maximum_size);
    }
}

int MWSTM_IsFsStatErr(ADXStream* stream)
{
    return ADXSTM_GetStat(stream) == 4;
}

int MWSTM_FinishStatic(void)
{
    return 0;
}

int MWSTM_InitStatic(void)
{
    return 0;
}
