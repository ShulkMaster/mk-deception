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

volatile int svm_init_level = 0;
volatile int svm_lock_level = 0;
volatile int svm_locking_type = 0;
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

/* RE4 preserves these unused helpers and APIs. Retail links their text out, but
 * their first references establish the observed SVM BSS ownership and order. */
static void svm_itoa(int value, char* string, int length)
{
    static char buffer[32];
    int index;
    int digit_count;
    int copy_count;

    for (index = 0; index < 32; index++) {
        buffer[index] = value % 10;
        value /= 10;
        if (value == 0) {
            buffer[index] = '\0';
            break;
        }
    }
    digit_count = strlen(buffer);
    copy_count = length - 1;
    if (digit_count < copy_count) {
        copy_count = digit_count;
    }
    for (index = 0; index < copy_count; index++) {
        string[index] = buffer[copy_count - 1 - index];
    }
    string[index] = '\0';
}

void SVM_SetCbWaitVsync(SVMCallbackFunction pre_function, void* pre_object,
                        SVMCallbackFunction post_function, void* post_object)
{
    svm_post_waitv_func.function = post_function;
    svm_post_waitv_func.object = post_object;
    svm_pre_waitv_func.function = pre_function;
    svm_pre_waitv_func.object = pre_object;
}

void SVM_SetCbTestAndSet(int (*function)(int* value))
{
    svm_tas_fptr = function;
}

static void svm_call_err1(const char* message)
{
    strncpy(svmerr_msg, message, 0x7F);
    if (svm_err_func.function != 0) {
        svm_err_func.function(svm_err_func.object, svmerr_msg);
    }
}

static void svm_unlock(void)
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

static void svm_lock(void)
{
    if (svm_lock_func.function != 0) {
        svm_lock_func.function(svm_lock_func.object);
        if (svm_lock_level == 0) svm_locking_type = 1;
        svm_lock_level++;
    }
}

void SVM_SetCbGotoSvrBorder(int server_type, SVMCallbackFunction function,
                            void* object)
{
    svm_goto_border_func[server_type].function = function;
    svm_goto_border_func[server_type].object = object;
}

int SVM_GetNumCbSvr(int server_type)
{
    int index;
    int count = 0;

    for (index = 0; index < 6; index++) {
        if (svm_svr_ftbl[server_type][index].function != 0) {
            count++;
        }
    }
    return count;
}

static int svm_exec_svr(int server_type)
{
    SVMServerCallback* callback;
    int index;
    int result;

    result = 0;
    callback = svm_svr_ftbl[server_type];
    for (index = 0; index < 6; index++) {
        if (callback->function != 0) {
            svm_svr_exec_flag[server_type] = 1;
            result |= callback->function(callback->object);
            svm_svr_exec_flag[server_type] = 0;
        }
        callback++;
    }
    svm_exec_cnt[server_type]++;
    return result;
}

unsigned int SVM_TestAndSet(int* value)
{
    unsigned int result;
    int previous;

    if (svm_tas_fptr != 0) {
        result = svm_tas_fptr(value);
    } else {
        svm_lock();
        previous = *value;
        *value = 1;
        result = (unsigned int)((1 - previous) | (previous - 1)) >> 31;
        svm_unlock();
    }
    return result;
}

static void svm_clear(void)
{
    int index;
    int* counter;

    memset(svm_svr_exec_flag, 0, sizeof(svm_svr_exec_flag));
    memset(&svm_lock_func, 0, sizeof(svm_lock_func));
    memset(&svm_unlock_func, 0, sizeof(svm_unlock_func));
    memset(&svm_pre_waitv_func, 0, sizeof(svm_pre_waitv_func));
    memset(&svm_post_waitv_func, 0, sizeof(svm_post_waitv_func));
    counter = svm_exec_cnt;
    for (index = 0; index < 6; index++) {
        *counter++ = 0;
    }
    svm_tas_fptr = 0;
}

void SVM_Finish(void)
{
    svm_init_level--;
    if (svm_init_level == 0) {
        svm_clear();
        memset(&svm_err_func, 0, sizeof(svm_err_func));
    }
}

void SVM_Init(void)
{
    if (svm_init_level == 0) {
        svm_clear();
    }
    svm_init_level++;
}

int SVM_ExecSvrMwIdle(void)
{
    return svm_exec_svr(6);
}

int SVM_ExecSvrMain(void)
{
    return svm_exec_svr(5);
}

int SVM_ExecSvrFs(void)
{
    return svm_exec_svr(4);
}

int SVM_ExecSvrVsync(void)
{
    return svm_exec_svr(2);
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
    svm_lock();
    svm_err_func.function = function;
    svm_err_func.object = object;
    svm_unlock();
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
    svm_lock();
    svm_goto_border_func[server_id].function = function;
    svm_goto_border_func[server_id].object = object;
    svm_unlock();
}

void SVM_SetCbSvrId(int server_type, int id, SVMServerFunction function,
                    void* object)
{
    SVMServerCallback* callback;

    if (id < 0 || id >= 6) {
        svm_call_err1("1071201:SVM_SetCbSvrId:illegal id");
    }
    /* Retail compares id against 8 here, despite reporting server_type. */
    if (server_type < 0 || id >= 8) {
        svm_call_err1("1071202:SVM_SetCbSvrId:illegal svtype");
    }
    svm_lock();
    callback = &svm_svr_ftbl[server_type][id];
    if (callback->function != 0) {
        svm_call_err1("2100801:SVM_SetCbSvrId:over write callback function.");
    }
    callback->function = function;
    callback->object = object;
    svm_unlock();
}

void SVM_DelCbSvr(int server_type, int id)
{
    if (id < 0 || id >= 6) {
        svm_call_err1("1051002:SVM_DelCbSvr:illegal id");
    }
    svm_lock();
    svm_svr_ftbl[server_type][id].function = 0;
    svm_svr_ftbl[server_type][id].object = 0;
    svm_unlock();
}

int SVM_SetCbSvr(int server_type, SVMServerFunction function, void* object)
{
    int id;
    SVMServerCallback* callback;

    svm_lock();
    callback = svm_svr_ftbl[server_type];
    for (id = 0; id < 6; callback++, id++) {
        if (callback->function == 0) {
            callback->function = function;
            callback->object = object;
            break;
        }
    }
    if (id == 6) {
        svm_call_err1("1051001:SVM_SetCbSvr:too many server function");
    }
    svm_unlock();
    if (id == 6) {
        return -1;
    }
    return id;
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
    svm_unlock();
}

void SVM_Lock(void)
{
    svm_lock();
}

static const char svm_space[] = " ";
