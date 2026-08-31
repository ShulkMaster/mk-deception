#include "dolphin/os.h"
#include "dolphin/vi.h"

typedef void (*SVMCallbackFunction)(void* object);
typedef void (*SVMErrorFunction)(void* object, char* message);

typedef struct ADXMThreadParams {
    int lock_priority;
    int safe_priority;
    int vsync_priority;
    int fs_priority;
    int field_10;
    int mwidle_priority;
} ADXMThreadParams;

typedef struct ADXMSleepCallback {
    SVMCallbackFunction function;
    void* object;
} ADXMSleepCallback;

extern void SVM_Init(const char* build);
extern void SVM_Finish(void);
extern void SVM_SetCbLock(SVMCallbackFunction function, void* object);
extern void SVM_SetCbUnlock(SVMCallbackFunction function, void* object);
extern void SVM_SetCbErr(SVMErrorFunction function, void* object);
extern void SVM_SetCbBdr(int server_id, SVMCallbackFunction function,
                         void* object);
extern int SVM_ExecSvrMwIdle(int count);
extern int SVM_ExecSvrFs(int count);
extern int SVM_ExecSvrVsync(int external_count, int count);
extern int SVM_ExecSvrMain(void);
extern void SVM_CallErr1(const char* message);
extern int adxt_vsync_cnt;

void* adxm_mwidle_proc(void* argument);
void* adxm_fs_proc(void* argument);
void* adxm_vsync_proc(void* argument);
void* adxm_safe_proc(void* argument);
void adxm_goto_mwidle_border(void* object);
void adxm_unlock(void* object);
void adxm_lock(void* object);

const char* const adxgc_build =
    "\nADXGC Ver.1.21 Build:Sep  3 2004 17:49:25\n";

static ADXMThreadParams adxm_save_tprm = {0, 0, 0, 0, 0, 0};
ADXMSleepCallback adxm_mwidle_sleep_cb = {0, 0};

int adxgc_exec_svr;
int adxm_init_level;
int adxm_lock_level;
int adxm_goto_border_flag;
int adxm_safe_cnt;
int adxm_vsync_cnt;
int adxm_fs_cnt;
int adxm_mwidle_cnt;
int adxm_mwidle_exec_flag;
OSThread* adxm_main_thread;
OSThread adxm_mwidle_thread;
OSThread adxm_vsync_thread;
OSThread adxm_fs_thread;
OSThread adxm_safe_thread;
int adxm_cur_prio;
int adxm_set_prio;
int adxm_fs_end;
int adxm_fs_act;
int adxm_vsync_end;
int adxm_vsync_act;
int adxm_mwidle_end;
int adxm_mwidle_act;
int adxm_safe_end;
int adxm_safe_act;
static unsigned char adxm_stack_mwidle[0x2000];
static unsigned char adxm_stack_fs[0x2000];
static unsigned char adxm_stack_vsync[0x2000];
static unsigned char adxm_stack_safe[0x1000];

typedef char ADXMThreadParamsSizeCheck[
    sizeof(ADXMThreadParams) == 0x18 ? 1 : -1];
typedef char ADXMSleepCallbackSizeCheck[
    sizeof(ADXMSleepCallback) == 0x8 ? 1 : -1];

void ADXM_ShutdownThrd(void)
{
    adxm_init_level--;
    if (adxm_init_level == 0) {
        adxm_mwidle_act = 0;
        OSSetThreadPriority(&adxm_mwidle_thread, 1);
        while (adxm_mwidle_end == 0) {
            OSResumeThread(&adxm_mwidle_thread);
        }

        OSCancelThread(&adxm_vsync_thread);
        OSCancelThread(&adxm_fs_thread);
        adxm_safe_act = 0;
        OSResumeThread(&adxm_safe_thread);
        while (adxm_safe_end == 0) {
            OSResumeThread(&adxm_safe_thread);
        }
        SVM_Finish();
    }
}

int ADXM_IsSetupThrd(void)
{
    return adxm_init_level != 0;
}

void ADXM_SetupThrd(const ADXMThreadParams* params)
{
    if (adxm_init_level == 0) {
        SVM_Init(adxgc_build);
        SVM_SetCbLock(adxm_lock, 0);
        SVM_SetCbUnlock(adxm_unlock, 0);

        if (params == 0) {
            adxm_save_tprm.field_10 = 16;
            adxm_save_tprm.lock_priority = 1;
            adxm_save_tprm.safe_priority = 8;
            adxm_save_tprm.vsync_priority = 12;
            adxm_save_tprm.fs_priority = 14;
            adxm_save_tprm.mwidle_priority = 24;
        } else {
            adxm_save_tprm = *params;
        }

        OSCreateThread(&adxm_safe_thread, adxm_safe_proc, 0,
                       adxm_stack_safe + sizeof(adxm_stack_safe),
                       sizeof(adxm_stack_safe),
                       adxm_save_tprm.safe_priority, 1);
        OSCreateThread(&adxm_vsync_thread, adxm_vsync_proc, 0,
                       adxm_stack_vsync + sizeof(adxm_stack_vsync),
                       sizeof(adxm_stack_vsync),
                       adxm_save_tprm.vsync_priority, 1);
        OSCreateThread(&adxm_fs_thread, adxm_fs_proc, 0,
                       adxm_stack_fs + sizeof(adxm_stack_fs),
                       sizeof(adxm_stack_fs),
                       adxm_save_tprm.fs_priority, 1);
        OSCreateThread(&adxm_mwidle_thread, adxm_mwidle_proc, 0,
                       adxm_stack_mwidle + sizeof(adxm_stack_mwidle),
                       sizeof(adxm_stack_mwidle),
                       adxm_save_tprm.mwidle_priority, 1);

        adxm_main_thread = OSGetCurrentThread();
        adxm_fs_act = 1;
        adxm_mwidle_act = 1;
        adxm_vsync_act = 1;
        adxm_safe_act = 1;
        adxm_fs_end = 0;
        adxm_mwidle_end = 0;
        adxm_vsync_end = 0;
        adxm_safe_end = 0;
        adxm_set_prio = 0;

        OSResumeThread(&adxm_vsync_thread);
        OSResumeThread(&adxm_fs_thread);
        OSResumeThread(&adxm_mwidle_thread);
        SVM_SetCbBdr(6, adxm_goto_mwidle_border, 0);
    }
    adxm_init_level++;
}

void ADXM_SetCbErr(SVMErrorFunction function, void* object)
{
    SVM_SetCbErr(function, object);
}

void* adxm_mwidle_proc(void* argument)
{
    ADXMThreadParams* thread_params = &adxm_save_tprm;
    ADXMSleepCallback* sleep_callback = &adxm_mwidle_sleep_cb;

    (void)argument;
    while (adxm_mwidle_act == 1) {
        int count = adxm_mwidle_cnt++;

        if (SVM_ExecSvrMwIdle(count) == 0 ||
            adxm_goto_border_flag == 1) {
            if (adxm_goto_border_flag == 1) {
                adxm_goto_border_flag = 0;
                OSSetThreadPriority(&adxm_mwidle_thread,
                                    thread_params->mwidle_priority);
            }
            if (sleep_callback->function != 0) {
                sleep_callback->function(sleep_callback->object);
            }
            OSSuspendThread(&adxm_mwidle_thread);
        }
    }
    adxm_mwidle_end = 1;
    return 0;
}

void* adxm_fs_proc(void* argument)
{
    (void)argument;
    while (adxm_fs_act == 1) {
        int count;

        VIWaitForRetrace();
        count = adxm_fs_cnt++;
        SVM_ExecSvrFs(count);
    }
    adxm_fs_end = 1;
    return 0;
}

void* adxm_vsync_proc(void* argument)
{
    int* external_vsync_count = &adxt_vsync_cnt;
    ADXMSleepCallback* sleep_callback = &adxm_mwidle_sleep_cb;

    (void)argument;
    while (adxm_vsync_act == 1) {
        int external_count;
        int count;

        VIWaitForRetrace();
        external_count = (*external_vsync_count)++;
        count = ++adxm_vsync_cnt;
        SVM_ExecSvrVsync(external_count, count);
        if (adxm_mwidle_end == 0) {
            OSResumeThread(&adxm_mwidle_thread);
            if (sleep_callback->function != 0) {
                sleep_callback->function(sleep_callback->object);
            }
        }
    }
    adxm_vsync_end = 1;
    return 0;
}

void* adxm_safe_proc(void* argument)
{
    (void)argument;
    while (adxm_safe_act == 1) {
        adxm_safe_cnt++;
    }
    adxm_safe_end = 1;
    return 0;
}

void adxm_goto_mwidle_border(void* object)
{
    int count;

    (void)object;
    if (adxm_mwidle_end != 1) {
        adxm_goto_border_flag = 1;
        OSSetThreadPriority(&adxm_mwidle_thread,
                            adxm_save_tprm.lock_priority);
        for (count = 0; count < 200000000; count++) {
            OSResumeThread(&adxm_mwidle_thread);
            if (adxm_goto_border_flag == 0) {
                break;
            }
        }
        if (count == 200000000) {
            SVM_CallErr1("1060102: Internal Error: adxm_goto_mwidle_border");
        }
        OSSetThreadPriority(&adxm_mwidle_thread,
                            adxm_save_tprm.mwidle_priority);
    }
}

void adxm_unlock(void* object)
{
    OSThread* thread;

    (void)object;
    adxm_lock_level--;
    if (adxm_lock_level == 0) {
        thread = OSGetCurrentThread();
        OSSuspendThread(&adxm_safe_thread);
        OSSetThreadPriority(thread, adxm_cur_prio);
    }
}

void adxm_lock(void* object)
{
    int interrupts;
    OSThread* thread;
    int priority;

    (void)object;
    if (adxm_lock_level == 0) {
        interrupts = OSDisableInterrupts();
        OSDisableScheduler();
        adxm_set_prio = 1;
        thread = OSGetCurrentThread();
        priority = OSGetThreadPriority(thread);
        OSSetThreadPriority(thread, adxm_save_tprm.lock_priority);
        adxm_cur_prio = priority;
        adxm_set_prio = 0;
        OSEnableScheduler();
        OSRestoreInterrupts(interrupts);
        OSResumeThread(&adxm_safe_thread);
    }
    adxm_lock_level++;
}

void ADXM_ExecMain(void)
{
    SVM_ExecSvrMain();
}

void ADXM_WaitVsync(void)
{
    VIWaitForRetrace();
}
