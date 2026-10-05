#ifndef FAST_RW_H
#define FAST_RW_H
int RwRenderStateSet_SRCBLEND_DESTBLEND(int srcBlend, int destBlend);
void RwRenderStateSet_rwRENDERSTATEVERTEXALPHAENABLE(int enable);
void RwRenderStateSet_rwRENDERSTATECULLMODE(int mode);
void RwRenderStateSet_rwRENDERSTATEZTESTENABLE(int enable);
void RwRenderStateSet_rwRENDERSTATEZWRITEENABLE(int enable);
void RwRenderStateSet_rwRENDERSTATETEXTUREFILTER(int filter);
#endif
