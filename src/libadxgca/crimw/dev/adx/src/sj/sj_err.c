#include "cri/svm.h"

void SJERR_CallErr(const char* message)
{
    SVM_CallErr1(message);
}
