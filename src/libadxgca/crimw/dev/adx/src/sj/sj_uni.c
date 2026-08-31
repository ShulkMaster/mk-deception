#include "runtime/cstring.h"

extern void SJCRS_Lock(void);
extern void SJCRS_Unlock(void);

int sjuni_init_cnt;
unsigned char sjuni_obj[0xC00];

void SJUNI_Finish(void)
{
    SJCRS_Lock();
    sjuni_init_cnt--;
    if (sjuni_init_cnt == 0) {
        memset(sjuni_obj, 0, sizeof(sjuni_obj));
    }
    SJCRS_Unlock();
}

void SJUNI_Init(void)
{
    SJCRS_Lock();
    if (sjuni_init_cnt == 0) {
        memset(sjuni_obj, 0, sizeof(sjuni_obj));
    }
    sjuni_init_cnt++;
    SJCRS_Unlock();
}
