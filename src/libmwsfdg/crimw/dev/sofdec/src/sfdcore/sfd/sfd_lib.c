#include "cri/sj.h"
#include "cri/svm.h"
#include "sofdec/sfd_library.h"
#include "sofdec/sfd_player.h"
#include "sofdec/uty_mem.h"

const char SFLIB_version_str[0x54] =
    "\nCRI SFD/GC Ver.1.940 Build:Sep  3 2004 11:38:48\n\0"
    "Append: MW2407 GC20Apr2004Patch1\n";
int sflib_sizeof_sfdhn;
void* SFD_pts_error_msg;
SfdHandle* sfd_hn_last;
static const char* cri_verstr_ptr;
SfdLibraryWork SFLIB_libwork;

void SFLIB_UnlockCs(int* token)
{
    (void)token;
    SVM_Unlock();
}

void SFLIB_LockCs(int* token)
{
    (void)token;
    SVM_Lock();
}

int SFLIB_CheckHn(SfdHandle* handle)
{
    sfd_hn_last = handle;
    if (handle == 0) {
        return -1;
    }
    return handle->playback_state == 0 ? -1 : 0;
}

static inline void sflib_CallErrFn(SfdErrorInfo* info, int error)
{
    if (error != 0 && info->callback != 0) {
        info->callback(info->callback_object, error);
    }
}

int SFD_SetErrFn(SfdHandle* handle, SfdErrorCallback callback, int object)
{
    if (handle == 0) {
        SFLIB_libwork.error_info.callback = callback;
        SFLIB_libwork.error_info.callback_object = object;
    } else {
        if (SFLIB_CheckHn(handle) != 0) {
            if (SFLIB_libwork.error_info.first_error == 0) {
                SFLIB_libwork.error_info.first_error = 0xFF000101;
            }
            sflib_CallErrFn(&SFLIB_libwork.error_info, 0xFF000101);
            return 0xFF000101;
        }

        handle->error_info.callback = callback;
        handle->error_info.callback_object = object;
    }
    return 0;
}

int SFLIB_SetErr(SfdHandle* handle, int error)
{
    if (error == 0) {
        return 0;
    }
    if (handle == 0) {
        if (SFLIB_libwork.error_info.first_error == 0) {
            SFLIB_libwork.error_info.first_error = error;
        }
        sflib_CallErrFn(&SFLIB_libwork.error_info, error);
    } else {
        if (handle->error_info.first_error == 0) {
            handle->error_info.first_error = error;
        }
        sflib_CallErrFn(&handle->error_info, error);
        if (handle->playback_state > 0) {
            handle->playback_state = -handle->playback_state;
        }
    }
    return error;
}

void SFLIB_InitErrInf(SfdErrorInfo* info)
{
    info->callback = 0;
    info->callback_object = 0;
    info->first_error = 0;
    info->field_0C = 0;
    info->field_10 = 0;
}

int SFD_Finish(void)
{
    int destroy_error;
    int error;
    int i;
    int transport_error;
    SfdHandle** slot;

    slot = SFLIB_libwork.handles;
    destroy_error = 0;
    for (i = 0; i < 8; i++, slot++) {
        if (*slot != 0) {
            destroy_error = SFD_Destroy(*slot);
        }
    }
    SFTIM_Finish(&SFLIB_libwork.timer_work);
    SFBUF_Finish(&SFLIB_libwork.buffer_work);
    transport_error = SFTRN_Finish(&SFLIB_libwork.transport_registry);
    SFHDS_Finish();
    SJRBF_Finish();
    if (transport_error == 0) {
        error = 0;
        if (destroy_error != 0) {
            error = destroy_error;
        }
    } else {
        error = transport_error;
    }
    return error;
}

int SFD_Init(const SfdLibraryConfig* config)
{
    int error;
    int i;

    sflib_sizeof_sfdhn = sizeof(SfdHandle);
    cri_verstr_ptr = SFLIB_version_str;
    SJRBF_Init();
    UTY_MemsetDword((unsigned int*)&SFLIB_libwork, 0, 0x89);
    MEM_Copy(SFLIB_libwork.default_conditions, SFPLY_cond_dfl, 0x190);
    SFLIB_libwork.transport_registry_source = config->transport_registry;
    SFLIB_libwork.timer_source = config->timer_source;
    SFLIB_libwork.field_0198 = 0;
    SFLIB_InitErrInf(&SFLIB_libwork.error_info);
    SFTIM_Init(&SFLIB_libwork.timer_work, config->timer_source);
    SFBUF_Init(&SFLIB_libwork.buffer_work);
    SFLIB_libwork.reset_in_progress = 0;
    SFLIB_libwork.retained_adxt = 0;
    for (i = 0; i < 8; i++) {
        SFLIB_libwork.handles[i] = 0;
    }
    error = 0;
    i = SFTRN_Init(&SFLIB_libwork.transport_registry,
                   config->transport_registry);
    if (i != 0) {
        error = i;
    }
    if (error != 0) {
        return error;
    }
    SFPLY_Init();
    SFHDS_Init();
    return 0;
}

int SFD_IsVersionCompatible(const char* version, int handle_size)
{
    (void)version;
    return handle_size == sizeof(SfdHandle);
}
