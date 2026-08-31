#include "cri/svm.h"
#include "runtime/cstring.h"

typedef void (*AdxErrorCallback)(void* object, const char* message);

extern void ADXT_DestroyAll(void);
extern void ADXRNA_Finish(void);
extern void ADXF_Finish(void);
extern void ADXSTM_Finish(void);
extern void LSC_Finish(void);
extern void ADXCRS_Lock(void);
extern void ADXCRS_Unlock(void);
extern void ADXSJD_Finish(void);
extern void ADXERR_Finish(void);
extern void SJMEM_Finish(void);
extern void SJRBF_Finish(void);
extern void SJUNI_Finish(void);
extern void ADXCRS_Init(void);
extern void SJUNI_Init(void);
extern void SJRBF_Init(void);
extern void SJMEM_Init(void);
extern void ADXERR_Init(void);
extern void ADXSTM_Init(void);
extern void ADXSJD_Init(void);
extern void ADXF_Init(void);
extern void ADXRNA_Init(void);
extern void LSC_Init(void);
extern void ADXRNA_EntryErrFunc(AdxErrorCallback callback, void* object);
extern void LSC_EntryErrFunc(AdxErrorCallback callback, void* object);
extern int ADXM_IsSetupThrd(void);
extern void ADXT_SetDefSvrFreq(int frequency);
extern void ADXT_ExecFsSvr(void);
extern void ADXT_ExecServer(void);
extern void LSC_ExecServer(void);
extern void ADXERR_CallErrFunc1(const char* message);

int adxt_init_cnt = 0;
int adxt_svr_id = 0;
int adxt_svr_main_id = 0;
int adxt_svr_fs_id = 0;
int adxt_vsync_cnt = 0;
unsigned char adxt_obj[0xC00];
static const char* cri_verstr_ptr;

int adxt_vsync_svr_flag = 1;

const char adxt_build[0x50] =
    "\nADXT/GC Ver.9.28 Build:Sep  3 2004 17:49:12\n\0"
    "Append: MW2407 GC20Apr2004Patch1\n";

int adxt_exec_fssvr(void* object);
int adxt_exec_tsvr(void* object);
static int adxt_exec_main_nothrd(void* object);
static int adxt_exec_main_thrd(void* object);
void adxini_lscerr_cbfn(void* object, const char* message);
void adxini_rnaerr_cbfn(void* object, const char* message);

void ADXT_Finish(void)
{
    adxt_init_cnt--;
    if (adxt_init_cnt == 0) {
        ADXT_DestroyAll();
        ADXRNA_Finish();
        ADXF_Finish();
        ADXSTM_Finish();
        LSC_Finish();
        ADXCRS_Lock();
        SVM_DelCbSvr(2, 1);
        SVM_DelCbSvr(4, adxt_svr_fs_id);
        SVM_DelCbSvr(5, adxt_svr_main_id);
        SVM_Finish();
        ADXSJD_Finish();
        ADXERR_Finish();
        SJMEM_Finish();
        SJRBF_Finish();
        SJUNI_Finish();
        ADXCRS_Unlock();
    }
}

void ADXT_Init(void)
{
    cri_verstr_ptr = adxt_build;
    if (adxt_init_cnt == 0) {
        ADXCRS_Init();
        ADXCRS_Lock();
        SJUNI_Init();
        SJRBF_Init();
        SJMEM_Init();
        ADXERR_Init();
        ADXSTM_Init();
        ADXSJD_Init();
        ADXF_Init();
        ADXRNA_Init();
        LSC_Init();
        SVM_Init();
        ADXRNA_EntryErrFunc(adxini_rnaerr_cbfn, 0);
        LSC_EntryErrFunc(adxini_lscerr_cbfn, 0);
        memset(adxt_obj, 0, sizeof(adxt_obj));
        if (ADXM_IsSetupThrd() == 1 && adxt_vsync_svr_flag == 1) {
            SVM_SetCbSvrId(2, 1, adxt_exec_tsvr, 0);
            adxt_svr_fs_id = SVM_SetCbSvr(4, adxt_exec_fssvr, 0);
            adxt_svr_main_id = SVM_SetCbSvr(5, adxt_exec_main_thrd, 0);
        } else {
            adxt_svr_main_id = SVM_SetCbSvr(5, adxt_exec_main_nothrd, 0);
        }
        adxt_vsync_cnt = 0;
        ADXT_SetDefSvrFreq(60);
        ADXCRS_Unlock();
    }
    adxt_init_cnt++;
}

int adxt_exec_fssvr(void* object)
{
    (void)object;
    ADXT_ExecFsSvr();
    return 0;
}

int adxt_exec_tsvr(void* object)
{
    (void)object;
    ADXT_ExecServer();
    return 0;
}

static int adxt_exec_main_nothrd(void* object)
{
    (void)object;
    ADXT_ExecServer();
    ADXT_ExecFsSvr();
    LSC_ExecServer();
    return 0;
}

static int adxt_exec_main_thrd(void* object)
{
    (void)object;
    LSC_ExecServer();
    return 0;
}

void adxini_lscerr_cbfn(void* object, const char* message)
{
    (void)object;
    ADXERR_CallErrFunc1(message);
}

void adxini_rnaerr_cbfn(void* object, const char* message)
{
    (void)object;
    ADXERR_CallErrFunc1(message);
}
