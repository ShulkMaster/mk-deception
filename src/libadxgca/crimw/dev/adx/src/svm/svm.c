#include "cri/svm.h"
#include "runtime/cstdio.h"

#undef va_start
#define va_start(arguments, format) ((void)(format), __builtin_va_info(&(arguments)))

typedef struct SVMServerCallback {
    SVMServerFunction function;
    void* object;
} SVMServerCallback;

typedef struct SVMCallback {
    SVMCallbackFunction function;
    void* object;
} SVMCallback;

typedef struct SVMErrorCallback {
    SVMErrorFunction function;
    void* object;
} SVMErrorCallback;

const char* const svm_build =
    "\nSVM/GC Ver.1.54 Build:Sep  3 2004 17:48:15\n";

int svm_init_level = 0;
int svm_lock_level = 0;
int svm_locking_type = 0;
static char buf_75[32];
static SVMCallback svm_post_waitv_func;
static SVMCallback svm_pre_waitv_func;
static int (*svm_tas_fptr)(int* value);
char svmerr_msg[128];
static SVMErrorCallback svm_err_func;
static SVMCallback svm_unlock_func;
static SVMCallback svm_lock_func;
static SVMCallback svm_goto_border_func[8];
static SVMServerCallback svm_svr_ftbl[8][6];
int svm_svr_exec_flag[8];
int svm_exec_cnt[8];

static const char svm_unlock_error[] =
    "2103102:SVM:svm_unlock:lock type miss match.(type org=%d, type now=%d)";
static const char svm_exec_id_error[] =
    "1071301:SVM_ExecSvrFuncId:illegal id";
static const char svm_exec_type_error[] =
    "1071302:SVM_ExecSvrFuncId:illegal svtype";

static inline int SVM_ExecServerGroup(int group, int flag_index)
{
    int result = 0;
    int index;
    SVMServerCallback* callback = &svm_svr_ftbl[group][0];

    for (index = 0; index < 6; index++, callback++) {
        if (callback->function != 0) {
            svm_svr_exec_flag[flag_index] = 1;
            result |= callback->function(callback->object);
            svm_svr_exec_flag[flag_index] = 0;
        }
    }
    svm_exec_cnt[flag_index]++;
    return result;
}

static inline void svm_lock_internal(void)
{
    if (svm_lock_func.function != 0) {
        svm_lock_func.function(svm_lock_func.object);
        if (svm_lock_level == 0) svm_locking_type = 1;
        svm_lock_level++;
    }
}

static inline void svm_unlock_internal(void)
{
    if (svm_unlock_func.function != 0) {
        svm_lock_level = svm_lock_level - 1;
        if (svm_lock_level == 0) {
            if (svm_locking_type != 1) {
                SVM_CallErr(svm_unlock_error, svm_locking_type, 1);
            }
            svm_locking_type = 0;
        }
        svm_unlock_func.function(svm_unlock_func.object);
    }
}

static inline void svm_report_error(const char* message)
{
    strncpy(svmerr_msg, message, 0x7F);
    if (svm_err_func.function != 0) {
        svm_err_func.function(svm_err_func.object, svmerr_msg);
    }
}

unsigned int SVM_TestAndSet(int* value)
{
    unsigned int result;
    int previous;

    if (svm_tas_fptr != 0) {
        result = svm_tas_fptr(value);
    } else {
        svm_lock_internal();
        previous = *value;
        *value = 1;
        result = (unsigned int)((1 - previous) | (previous - 1)) >> 31;
        svm_unlock_internal();
    }
    return result;
}

void SVM_Finish(void)
{
    svm_init_level--;
    if (svm_init_level == 0) {
        memset(svm_svr_exec_flag, 0, sizeof(svm_svr_exec_flag));
        memset(&svm_lock_func, 0, sizeof(svm_lock_func));
        memset(&svm_unlock_func, 0, sizeof(svm_unlock_func));
        memset(&svm_pre_waitv_func, 0, sizeof(svm_pre_waitv_func));
        memset(&svm_post_waitv_func, 0, sizeof(svm_post_waitv_func));
        svm_exec_cnt[0] = 0;
        svm_exec_cnt[1] = 0;
        svm_exec_cnt[2] = 0;
        svm_exec_cnt[3] = 0;
        svm_exec_cnt[4] = 0;
        svm_exec_cnt[5] = 0;
        svm_tas_fptr = 0;
        memset(&svm_err_func, 0, sizeof(svm_err_func));
    }
}

void SVM_Init(void)
{
    if (svm_init_level == 0) {
        memset(svm_svr_exec_flag, 0, sizeof(svm_svr_exec_flag));
        memset(&svm_lock_func, 0, sizeof(svm_lock_func));
        memset(&svm_unlock_func, 0, sizeof(svm_unlock_func));
        memset(&svm_pre_waitv_func, 0, sizeof(svm_pre_waitv_func));
        memset(&svm_post_waitv_func, 0, sizeof(svm_post_waitv_func));
        svm_exec_cnt[0] = 0;
        svm_exec_cnt[1] = 0;
        svm_exec_cnt[2] = 0;
        svm_exec_cnt[3] = 0;
        svm_exec_cnt[4] = 0;
        svm_exec_cnt[5] = 0;
        svm_tas_fptr = 0;
    }
    svm_init_level++;
}

int SVM_ExecSvrMwIdle(void)
{
    return SVM_ExecServerGroup(6, 6);
}

int SVM_ExecSvrMain(void)
{
    return SVM_ExecServerGroup(5, 5);
}

int SVM_ExecSvrFs(void)
{
    return SVM_ExecServerGroup(4, 4);
}

int SVM_ExecSvrVsync(void)
{
    return SVM_ExecServerGroup(2, 2);
}

void SVM_SetCbUnlock(SVMCallbackFunction function, void* object)
{
    svm_unlock_func.function = function;
    svm_unlock_func.object = object;
}

void SVM_SetCbLock(SVMCallbackFunction function, void* object)
{
    svm_lock_func.function = function;
    svm_lock_func.object = object;
}

void SVM_SetCbErr(SVMErrorFunction function, void* object)
{
    svm_lock_internal();
    svm_err_func.function = function;
    svm_err_func.object = object;
    svm_unlock_internal();
}

void SVM_GotoSvrBorder(int server_id)
{
    if (svm_goto_border_func[server_id].function != 0) {
        svm_goto_border_func[server_id].function(
            svm_goto_border_func[server_id].object);
    }
}

void SVM_SetCbBdr(int server_id, SVMCallbackFunction function, void* object)
{
    svm_lock_internal();
    svm_goto_border_func[server_id].function = function;
    svm_goto_border_func[server_id].object = object;
    svm_unlock_internal();
}

void SVM_SetCbSvrId(int server_type, int id, SVMServerFunction function,
                    void* object)
{
    SVMServerCallback* callback;

    if (id < 0 || id >= 6) {
        svm_report_error("1071201:SVM_SetCbSvrId:illegal id");
    }
    /* Retail compares id against 8 here, despite reporting server_type. */
    if (server_type < 0 || id >= 8) {
        svm_report_error("1071202:SVM_SetCbSvrId:illegal svtype");
    }
    svm_lock_internal();
    callback = &svm_svr_ftbl[server_type][id];
    if (callback->function != 0) {
        svm_report_error("2100801:SVM_SetCbSvrId:over write callback function.");
    }
    callback->function = function;
    callback->object = object;
    svm_unlock_internal();
}

void SVM_DelCbSvr(int server_type, int id)
{
    if (id < 0 || id >= 6) {
        svm_report_error("1051002:SVM_DelCbSvr:illegal id");
    }
    svm_lock_internal();
    svm_svr_ftbl[server_type][id].function = 0;
    svm_svr_ftbl[server_type][id].object = 0;
    svm_unlock_internal();
}

int SVM_SetCbSvr(int server_type, SVMServerFunction function, void* object)
{
    int id;
    SVMServerCallback* callback;

    svm_lock_internal();
    callback = &svm_svr_ftbl[server_type][0];
    for (id = 0; id < 6; id++, callback++) {
        if (callback->function == 0) {
            callback->function = function;
            callback->object = object;
            break;
        }
    }
    if (id == 6) {
        svm_report_error("1051001:SVM_SetCbSvr:too many server function");
    }
    svm_unlock_internal();
    return id == 6 ? -1 : id;
}

void SVM_CallErr1(const char* message)
{
    strncpy(svmerr_msg, message, 0x7F);
    if (svm_err_func.function != 0) {
        svm_err_func.function(svm_err_func.object, svmerr_msg);
    }
}

void SVM_CallErr(const char* message, ...)
{
    __va_list arguments;

    memset(svmerr_msg, 0, sizeof(svmerr_msg));
    va_start(arguments, message);
    vsprintf(svmerr_msg, message, arguments);
    if (svm_err_func.function != 0) {
        svm_err_func.function(svm_err_func.object, svmerr_msg);
    }
    va_end(arguments);
}

void SVM_Unlock(void)
{
    svm_unlock_internal();
}

void SVM_Lock(void)
{
    svm_lock_internal();
}

static const char svm_space[] = " ";
