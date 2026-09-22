#include "runtime/cstring.h"

typedef void (*ADXErrorCallback)(void* object, const char* message);

extern void SVM_CallErr(const char* message, ...);

ADXErrorCallback adxerr_func = 0;
void* adxerr_obj = 0;
char adxerr_msg[256];

static inline void adxerr_itoa(int value, signed char* string, int length)
{
    static signed char buf[32];
    int i;
    int n;
    int l;

    for (i = 0; i < 32; i++) {
        string[i] = value % 10;
        value /= 10;
        if (value == 0) {
            string[i] = '\0';
            break;
        }
    }

    l = strlen((const char*)buf);
    n = length - 1;
    if (l < n) {
        n = l;
    }
    for (i = 0; i < n; i++) {
        string[i] = buf[n - 1 - i];
    }
    string[i] = '\0';
}

/* TODO: [near miss] 99.96784%; instructions and BSS layout agree; only anonymous pooled-space/local relocation labels remain. */
void ADXERR_ItoA2(int value1, int value2, signed char* string, int length)
{
    adxerr_itoa(value1, string, length);
    strncat((char*)string, " ", length - strlen((const char*)string) - 1);
    adxerr_itoa(value2, string + strlen((const char*)string),
                4 - strlen((const char*)string));
}

void ADXERR_CallErrFunc2(const char* message1, const char* message2)
{
    strncpy(adxerr_msg, message1, sizeof(adxerr_msg) - 1);
    strncat(adxerr_msg, message2, sizeof(adxerr_msg) - 1);
    if (adxerr_func != 0) {
        adxerr_func(adxerr_obj, adxerr_msg);
    }
    SVM_CallErr(adxerr_msg);
}

void ADXERR_CallErrFunc1(const char* message)
{
    strncpy(adxerr_msg, message, sizeof(adxerr_msg) - 1);
    if (adxerr_func != 0) {
        adxerr_func(adxerr_obj, adxerr_msg);
    }
    SVM_CallErr(adxerr_msg);
}

void ADXERR_Finish(void)
{
    memset(adxerr_msg, 0, sizeof(adxerr_msg));
    adxerr_func = 0;
    adxerr_obj = 0;
}

void ADXERR_Init(void)
{
    memset(adxerr_msg, 0, sizeof(adxerr_msg));
    adxerr_func = 0;
    adxerr_obj = 0;
}
