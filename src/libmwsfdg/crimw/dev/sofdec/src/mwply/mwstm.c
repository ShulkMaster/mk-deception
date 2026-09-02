extern int ADXSTM_GetStat(void* stream);
extern void ADXSTM_StopNw(void* stream);
extern void ADXSTM_ReleaseFileNw(void* stream);
extern int ADXSTM_Start(void* stream);
extern void ADXSTM_BindFileNw(void* stream, const char* path, int offset,
                              int length, int end_position);
extern void ADXSTM_SetEos(void* stream, int end_position);
extern void ADXSTM_Destroy(void* stream);
extern void* ADXSTM_Create(void* source, int priority);
extern int ADXSTM_SetBufSize(void* stream, int minimum_size,
                             int maximum_size);

int MWSTM_GetStat(void* stream)
{
    return ADXSTM_GetStat(stream);
}

void MWSTM_ReqStop(void* stream)
{
    ADXSTM_StopNw(stream);
    ADXSTM_ReleaseFileNw(stream);
}

int MWSTM_ReqStart(void* stream)
{
    return ADXSTM_Start(stream);
}

void MWSTM_SetFileRange(void* stream, const char* path, int offset,
                        int length, int end_position)
{
    ADXSTM_ReleaseFileNw(stream);
    ADXSTM_BindFileNw(stream, path, offset, length, end_position);
    ADXSTM_SetEos(stream, end_position);
}

void MWSTM_Destroy(void* stream)
{
    ADXSTM_Destroy(stream);
}

void* MWSTM_Create(void* source)
{
    return ADXSTM_Create(source, 0);
}

void MWSTM_SetFlowLimit(void* stream, int minimum_size, int maximum_size)
{
    if (stream != 0) {
        ADXSTM_SetBufSize(stream, minimum_size, maximum_size);
    }
}

int MWSTM_IsFsStatErr(void* stream)
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
