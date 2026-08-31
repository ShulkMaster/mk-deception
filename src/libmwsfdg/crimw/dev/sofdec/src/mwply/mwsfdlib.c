#include "cri/sj.h"
#include "runtime/cstring.h"
#include "runtime/cstdio.h"

typedef struct SfdHandle SfdHandle;

typedef struct MwsInitParam {
    float frame_rate;
    int max_width;
    int decoder_count;
    int field_0C;
    int field_10;
    int field_14;
    int field_18;
    int svm_parameter;
} MwsInitParam;

typedef struct MwsPlayerSlot {
    int reserved_000;
    int active;
    unsigned char reserved_008[0x2B0];
} MwsPlayerSlot;

typedef struct MwsLibraryWork {
    int field_00;
    float frame_rate;
    int max_width;
    int decoder_count;
    int field_10;
    unsigned char reserved_014[0x24];
    int use_picture_user_data;
    int pause_border;
    unsigned char reserved_040[0x1C];
    int field_5C;
    unsigned char reserved_060[0x08];
    int error_code;
    MwsPlayerSlot players[8];
} MwsLibraryWork;

typedef char MwsInitParamSizeCheck[sizeof(MwsInitParam) == 0x20 ? 1 : -1];
typedef char MwsPlayerSlotSizeCheck[sizeof(MwsPlayerSlot) == 0x2B8 ? 1 : -1];
typedef char MwsLibraryWorkSizeCheck[sizeof(MwsLibraryWork) == 0x162C ? 1 : -1];

extern void* SFD_tr_in_mem[];
extern void* SFD_tr_sd_mps[];
extern void* SFD_tr_vd_mpv[];
extern void* SFD_tr_vo_manu[];
extern void* SFD_tr_ad_adxt[];
extern void* SFD_tr_ao_auto_p[];
extern void* SFD_tr_uo[];

const char mwsfd_ver_str[] =
    "\nMWSFD/GC Ver.3.31 Build:Sep  3 2004 11:38:18\n\0"
    "Append: MW2407 GC20Apr2004Patch1\n";
static void* const mwsfd_trentry[15] = {
    SFD_tr_in_mem, SFD_tr_sd_mps, SFD_tr_vd_mpv, SFD_tr_vo_manu,
    SFD_tr_ad_adxt, SFD_tr_ao_auto_p, SFD_tr_uo,
    0, 0, 0, 0, 0, 0, 0, 0
};
static const struct {
    void* const* transports;
    int work_size;
} mwsfd_initsfdpara = {mwsfd_trentry, 0xEA24};
const int mwsfd_siz_mwplyhn = sizeof(MwsPlayerSlot);

static const char data_error[] = "DATA ERROR(%08X)";
static const char creation_small[] =
    "SFD ERROR(%08X): 'max_width, max_height' of creation parameter is small. Increase this value.";
static const char read_buffer_small[] =
    "SFD ERROR(%08X): Read buffer is small. Increase 'max_bps' of creation parameter.";
static const char frame_pool_invalid[] =
    "SFD ERROR(%08X): Frame pool size is incorrect. Set positive integer to 'nfrm_pool_wk' of creation parameter.";
static const char adxt_handles_exceeded[] =
    "SFD ERROR(%08X): Number of ADXT handles exceeds its maximum number. MWPLY handle uses one ADXT handle(stereo) for MWSFD_FTYPE_SFD.";
static const char generic_sfd_error[] = "SFD ERROR(%08X)";
static const char compatible_version[] = "1.940";
static const char incompatible_version[] =
    "E011081 mwPlySfdInit: Not compatible SFD Version.";
typedef struct MwsInitLiterals {
    float default_frame_rate;
    char null_parameter[0x28];
    char init_gsc_failed[0x28];
    float rounding_half;
    float rate_scale;
    char init_failed[0x20];
} MwsInitLiterals;
static const MwsInitLiterals mwsfd_init_literals = {
    59.94f,
    "E1122611 mwPlyInitSfdFx: iprm is NULL.",
    "E2005 mwPlyInitSfdFx: can't init GSC",
    0.5f,
    1000.0f,
    "ERR20010421A : mwPlyInitSfdFx"
};

int mwsfd_init_cnt;
int mwsfd_init_flag;
SfdHandle* mwsfd_err_sfdhn;
void* mwsfd_err_mwsfdhn;
static char mwg_sfd_errstr[0x100];
static int mwg_sfd_errcnt;
static int mwg_sfd_errcode[16];
MwsLibraryWork mwsfd_libwork;
static const char* cri_verstr_ptr;
int gap_06_804AE1E4_bss;

extern int mwg_vcnt;
extern SfdHandle* mwPlyGetSfdHn(void* player);
extern void MWSFSVM_Error(const char* message, ...);
extern void MWSFSVM_Init(int parameter);
extern void MWSFSVM_Finish(void);
extern void MWSFSVM_DeleteVfunc(void);
extern void MWSFSVM_DeleteMainFunc(void);
extern void MWSFSVM_DeleteIdleFunc(void);
typedef int (*MwsServerFunction)(void* object);

extern void MWSFSVM_EntryIdVfunc(int id, MwsServerFunction function,
                                 void* object);
extern void MWSFSVM_EntryMainFunc(MwsServerFunction function, void* object);
extern void MWSFSVM_EntryIdleFunc(MwsServerFunction function, void* object);
extern void MWSFSVR_SetMwsfdSvrFlg(int enabled);
extern int MWSFSVR_VsyncThrdProc(void* object);
extern int MWSFSVR_MainThrdProc(void* object);
extern int MWSFSVR_IdleThrdProc(void* object);
extern void MWSFSFX_Init(void);
extern void MWSFSFX_Finish(void);
extern void mwSfdDestroy(MwsPlayerSlot* player);
extern void ADXT_Init(void);
extern void ADXT_Finish(void);
extern void LSC_Init(void);
extern void LSC_Finish(void);
extern void LSC_EntryErrFunc(void (*function)(void*, const char*), void* object);
extern int MWSTM_InitStatic(void);
extern void MWSTM_FinishStatic(void);
extern void SJMEM_Init(void);
extern void SJMEM_Finish(void);
extern void SJUNI_Init(void);
extern void SJUNI_Finish(void);
extern void SJRBF_Init(void);
extern void SFD_Finish(void);
extern int SFD_IsVersionCompatible(const char* version, int handle_size,
                                   void* const* transports, int* version_count,
                                   int reserved, int enabled, float frame_rate,
                                   float rate_scale);
extern int SFD_Init(void* parameters);
extern int SFD_SetErrFn(int type, void (*function)(void*, int), void* object);
extern void MWSFD_SetCond(void* player, int condition, int value);

void MWSFLIB_SfdErrFunc(void* player, int error)
{
    if (player != 0) {
        SfdHandle* sfd = mwPlyGetSfdHn(player);
        mwsfd_err_mwsfdhn = player;
        mwsfd_err_sfdhn = sfd;
    } else {
        mwsfd_err_mwsfdhn = 0;
        mwsfd_err_sfdhn = 0;
    }
    if (error != 0) {
        mwg_sfd_errcode[mwg_sfd_errcnt] = error;
        if (mwg_sfd_errcnt < 15) {
            mwg_sfd_errcnt++;
        }
    }

    switch ((unsigned int)error) {
    case 0xFFFFFFFD:
    case 0xFFFFFFFE:
        sprintf(mwg_sfd_errstr, data_error, error);
        break;
    case 0xFF000F17:
    case 0xFF000F18:
        sprintf(mwg_sfd_errstr, creation_small, error);
        break;
    case 0xFF000408:
    case 0xFF00040C:
    case 0xFF000F1C:
        sprintf(mwg_sfd_errstr, read_buffer_small, error);
        break;
    case 0xFF000F15:
        sprintf(mwg_sfd_errstr, frame_pool_invalid, error);
        break;
    case 0xFF000C04:
        sprintf(mwg_sfd_errstr, adxt_handles_exceeded, error);
        break;
    default:
        sprintf(mwg_sfd_errstr, generic_sfd_error, error);
        break;
    }
    MWSFSVM_Error(mwg_sfd_errstr);
}

int MWSFLIB_SetErrCode(int error)
{
    mwsfd_libwork.error_code = error;
    return error & ~-((error == 0) & 1);
}

void mwPlyFinishSfdFx(void)
{
    MwsLibraryWork* cursor = &mwsfd_libwork;
    int index;

    mwsfd_init_cnt--;
    if (mwsfd_init_cnt != 0) {
        return;
    }
    index = 0;
    do {
        if (cursor->players[0].active == 1) {
            mwSfdDestroy(&cursor->players[0]);
        }
        index++;
        cursor = (MwsLibraryWork*)((unsigned char*)cursor +
                                  sizeof(MwsPlayerSlot));
    } while (index < 8);
    MWSFSVM_DeleteVfunc();
    MWSFSVM_DeleteMainFunc();
    MWSFSVM_DeleteIdleFunc();
    MWSFSFX_Finish();
    LSC_Finish();
    SFD_Finish();
    ADXT_Finish();
    MWSTM_FinishStatic();
    SJUNI_Finish();
    SJMEM_Finish();
    SJRBF_Finish();
    MWSFSVM_Finish();
}

int MWSFD_GetPauseBdr(void)
{
    return mwsfd_libwork.pause_border;
}

int MWSFD_GetUsePicUsr(void)
{
    return mwsfd_libwork.use_picture_user_data;
}

static void mwsflib_LscErrFunc(void* object, const char* message);

void mwPlyInitSfdFx(MwsInitParam* parameter)
{
    MwsInitParam local;
    MwsInitParam* sfd_parameter;
    MwsLibraryWork* work;
    int sfd_parameters[2];
    int result;

    if (parameter == 0) {
        MWSFSVM_Error(mwsfd_init_literals.null_parameter);
        return;
    }
    memset(&local, 0, sizeof(local));
    local.frame_rate = parameter->frame_rate;
    local.max_width = parameter->max_width;
    local.decoder_count = parameter->decoder_count;
    local.field_0C = parameter->field_0C;
    local.field_10 = parameter->field_10;
    local.field_14 = parameter->field_14;
    local.field_18 = parameter->field_18;
    local.svm_parameter = parameter->svm_parameter;
    sfd_parameter = &local;
    cri_verstr_ptr = mwsfd_ver_str;
    MWSFSVM_Init(local.svm_parameter);
    local.decoder_count -= 2;
    if (local.decoder_count < 0) {
        local.decoder_count = 0;
    }

    if (mwsfd_init_cnt == 0) {
        ADXT_Init();
        SJRBF_Init();
        SJMEM_Init();
        SJUNI_Init();
        if (MWSTM_InitStatic() != 0) {
            mwsfd_libwork.error_code = -0x65;
            MWSFSVM_Error(mwsfd_init_literals.init_gsc_failed);
        }
        work = &mwsfd_libwork;
        memset(work, 0, sizeof(*work));
        MWSFSVR_SetMwsfdSvrFlg(0);
        work->field_5C = 0;
        if (sfd_parameter != 0) {
            work->frame_rate = sfd_parameter->frame_rate;
            work->max_width = sfd_parameter->max_width;
            work->decoder_count = sfd_parameter->decoder_count;
            work->field_10 = sfd_parameter->field_0C;
        } else {
            work->frame_rate = mwsfd_init_literals.default_frame_rate;
            work->max_width = 1;
            work->decoder_count = 1;
            work->field_10 = 0;
        }
        work->use_picture_user_data = 1;
        work->pause_border = 1;
        mwg_vcnt = 0;
        sfd_parameters[0] = (int)mwsfd_initsfdpara.transports;
        sfd_parameters[1] = mwsfd_initsfdpara.work_size;
        sfd_parameters[1] =
            (int)((mwsfd_init_literals.rate_scale * local.frame_rate) +
                  mwsfd_init_literals.rounding_half);

        if (SFD_IsVersionCompatible(compatible_version, 0x3598,
                                    mwsfd_initsfdpara.transports, &mwg_vcnt,
                                    0, 1, local.frame_rate,
                                    mwsfd_init_literals.rate_scale) != 1) {
            MWSFSVM_Error(incompatible_version);
            result = -1;
        } else if (SFD_Init(sfd_parameters) != 0) {
            result = -0x12D;
            work->error_code = result;
        } else if (SFD_SetErrFn(0, MWSFLIB_SfdErrFunc, 0) != 0) {
            result = -0x12F;
            work->error_code = result;
        } else {
            result = 0;
        }
        if (result != 0) {
            MWSFSVM_Error(mwsfd_init_literals.init_failed);
        }
        mwsfd_init_flag = 1;
        MWSFD_SetCond(0, 0x1B, (int)local.frame_rate);
        MWSFD_SetCond(0, 7, 1);
        LSC_Init();
        LSC_EntryErrFunc(mwsflib_LscErrFunc, 0);
        MWSFSFX_Init();
        MWSFSVM_EntryIdVfunc(2, MWSFSVR_VsyncThrdProc, 0);
        MWSFSVM_EntryMainFunc(MWSFSVR_MainThrdProc, 0);
        MWSFSVM_EntryIdleFunc(MWSFSVR_IdleThrdProc, 0);
    }
    mwsfd_init_cnt++;
}

static void mwsflib_LscErrFunc(void* object, const char* message)
{
    MWSFSVM_Error(message);
}

MwsLibraryWork* MWSFLIB_GetLibWorkPtr(void)
{
    return &mwsfd_libwork;
}
