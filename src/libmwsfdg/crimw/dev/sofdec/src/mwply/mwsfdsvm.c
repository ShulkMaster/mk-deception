#include "cri/svm.h"
#include "runtime/cstdio.h"

#undef va_start
#define va_start(arguments, format) ((void)(format), __builtin_va_info(&(arguments)))

static char errstr[256];
static int mwg_main_fid;
static int mwg_idle_fid;
static int mwg_vsync_fid;
static int mwg_vbin_fid;

void MWSFSVM_GotoIdleBorder(void)
{
    SVM_GotoSvrBorder(6);
}

void MWSFSVM_Error(const char* format, ...)
{
    __va_list arguments;

    memset(errstr, 0, sizeof(errstr));
    va_start(arguments, format);
    vsprintf(errstr, format, arguments);
    SVM_CallErr1(errstr);
    va_end(arguments);
}

unsigned int MWSFSVM_TestAndSet(int* value)
{
    return SVM_TestAndSet(value);
}

void MWSFSVM_DeleteMainFunc(void)
{
    SVM_DelCbSvr(5, mwg_main_fid);
}

void MWSFSVM_EntryMainFunc(SVMServerFunction function, void* object)
{
    mwg_main_fid = SVM_SetCbSvr(5, function, object);
}

void MWSFSVM_DeleteIdleFunc(void)
{
    SVM_DelCbSvr(6, mwg_idle_fid);
}

void MWSFSVM_EntryIdleFunc(SVMServerFunction function, void* object)
{
    mwg_idle_fid = SVM_SetCbSvr(6, function, object);
}

void MWSFSVM_DeleteVfunc(void)
{
    SVM_DelCbSvr(2, mwg_vsync_fid);
}

void MWSFSVM_EntryIdVfunc(int id, SVMServerFunction function, void* object)
{
    SVM_SetCbSvrId(2, id, function, object);
    mwg_vsync_fid = id;
}

void MWSFSVM_Finish(void)
{
    SVM_Finish();
}

void MWSFSVM_Init(void)
{
    SVM_Init();
    mwg_vbin_fid = 0;
    mwg_vsync_fid = 0;
    mwg_idle_fid = 0;
    mwg_main_fid = 0;
}
