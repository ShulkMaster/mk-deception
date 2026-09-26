#include "cri/lsc_internal.h"
#include "runtime/cstring.h"

extern void LSC_LockCrs(void* state);
extern void LSC_UnlockCrs(void* state);
extern void LSC_Destroy(LSCObject* object);
extern void LSC_EntryErrFunc(void* callback, void* callback_object);

int lsc_init_cnt;
LSCObject lsc_obj[32];

const char* const volatile lsc_build =
    "\nLSC/GC Ver.2.17 Build:Sep  3 2004 17:47:55\n";

void LSC_Finish(void)
{
    int critical_state;
    LSCObject* object;
    int i;

    LSC_LockCrs(&critical_state);
    lsc_init_cnt--;
    if (lsc_init_cnt == 0) {
        for (i = 0; i < 32; i++) {
            object = &lsc_obj[i];
            if (object->used == 1) {
                LSC_Destroy(object);
            }
        }
        memset(lsc_obj, 0, sizeof(lsc_obj));
        LSC_EntryErrFunc(0, 0);
    }
    LSC_UnlockCrs(&critical_state);
}

void LSC_Init(void)
{
    int critical_state;

    lsc_build;
    LSC_LockCrs(&critical_state);
    if (lsc_init_cnt == 0) {
        memset(lsc_obj, 0, sizeof(lsc_obj));
        LSC_EntryErrFunc(0, 0);
    }
    lsc_init_cnt++;
    LSC_UnlockCrs(&critical_state);
}
