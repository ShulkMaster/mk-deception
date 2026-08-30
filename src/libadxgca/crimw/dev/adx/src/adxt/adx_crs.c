#include "cri/svm.h"

int adxcrs_lvl = 0;
int adxcrs_msk = 0;

void ADXCRS_Unlock(void)
{
    SVM_Unlock();
}

void ADXCRS_Lock(void)
{
    SVM_Lock();
}

void ADXCRS_Init(void)
{
    adxcrs_lvl = 0;
}
