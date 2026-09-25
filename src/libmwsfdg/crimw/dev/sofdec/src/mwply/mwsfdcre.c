#include "cri/sj.h"
#include "cri/adxt_internal.h"
#include "runtime/cstring.h"
#include "sofdec/sfd_mpvf.h"
#include "sofdec/sfd_player.h"

typedef struct LSC LSC;
typedef struct SFXHandle SFXHandle;
typedef struct MwsPlayer MwsPlayer;
typedef void* (*MwsMallocFn)(void*, int);
typedef void (*MwsFreeFn)(void*, void*);

typedef struct MwsStHandle {
    int active;
    unsigned char reserved_004[8];
    SJ* stream;
    int element_id;
    void* backend;
} MwsStHandle;

typedef struct MwsPictureUserConfig {
    void* buffer;
    int buffer_size;
    int element_size;
} MwsPictureUserConfig;

typedef struct MwsPictureUserWork {
    unsigned char header[0x40];
    unsigned char element_buffer[0x7C0];
} MwsPictureUserWork;

typedef char MwsPictureUserWorkSizeCheck[
    sizeof(MwsPictureUserWork) == 0x800 ? 1 : -1];

typedef struct MwsCreateParams {
    int file_type;
    int maximum_bps;
    int width;
    int height;
    int frame_count;
    int decoder_count;
    void* work;
    int work_size;
    int composition_mode;
    int buffer_format;
    int field_28;
    int field_2C;
} MwsCreateParams;

struct MwsPlayer {
    const void* interface;
    int active;
    int status;
    MwsCreateParams create; /* +0x0C - copy of the creation parameters */
    int playback_mode;
    SfdHandle* sfd;
    ADXStream* stream;
    void* transport;
    LSC* loader;
    int field_050;
    int field_054;
    int field_058;
    int flow_limit;
    int field_060;
    unsigned char reserved_064[8];
    int field_06C;
    int field_070;
    signed char concat_play;
    signed char concat_stopped;
    signed char paused;
    unsigned char field_077;
    int entry_count;
    unsigned char reserved_07C[12];
    int field_088;
    unsigned char reserved_08C[0x20];
    SFXHandle* sfx;
    void* composition_work;
    int composition_work_size;
    unsigned char reserved_0B8[0xAC];
    MwsPictureUserConfig internal_picture_user;
    unsigned char reserved_170[12];
    MwsPictureUserConfig* picture_user;
    MwsPictureUserWork* picture_user_work;
    int picture_user_header_size;
    int picture_user_write;
    int picture_user_read;
    SJ* additional_info_sj;
    void* additional_info_buffer;
    int additional_info_buffer_size;
    unsigned char reserved_19C[0x1C];
    char* filename;
    int filename_capacity;
    int start_requested;
    int file_offset;
    int file_length;
    int file_end_position;
    SJ* supply_sj;
    SJ* input_sj;
    void* input_buffer;
    int input_buffer_size;
    int input_buffer_extra_size;
    int supply_mode;
    void* supply_buffer;
    int supply_buffer_size;
    int supply_buffer_extra_size;
    SJ* memory_sj;
    void* memory_buffer;
    int memory_buffer_size;
    void* arena;
    unsigned int arena_size;
    unsigned char* arena_cursor;
    unsigned int arena_used;
    int allocation_count;
    void* allocations[32];
    MwsStHandle sound;
    unsigned char reserved_2AC[8];
    int sound_state;
};

typedef struct MwsLibraryWork {
    int field_00;
    float frame_rate;
    int video_frequency; /* +0x08 - reference refresh rate */
    int frame_pool;      /* +0x0C - frames held by the decoder pool */
    int field_10;
    unsigned char reserved_014[0x14];
    MwsMallocFn malloc_fn;
    MwsFreeFn free_fn;
    void* allocator_object;
    unsigned char reserved_034[4];
    int use_picture_user_data;
    int pause_border;
    unsigned char reserved_040[0x1C];
    int field_5C;
    unsigned char reserved_060[8];
    int error_code;
    MwsPlayer players[8];
} MwsLibraryWork;

typedef struct MwsReferenceBuffers { void* buffers[2]; } MwsReferenceBuffers;


typedef struct MwsPlayerInterface {
    void* reserved[3];
    void (*vsync)(void);
    int (*exec_server)(MwsPlayer*);
    void (*destroy)(MwsPlayer*);
    void (*start_filename)(MwsPlayer*, const char*);
    void (*stop)(MwsPlayer*);
    int (*get_status)(MwsPlayer*);
    void (*get_time)(MwsPlayer*, int*, int*);
    void (*pause)(MwsPlayer*, int);
    void (*set_volume)(MwsPlayer*, int);
    int (*get_volume)(MwsPlayer*);
    void (*set_pan)(MwsPlayer*, int);
    int (*get_pan)(MwsPlayer*);
    void (*start_sj)(MwsPlayer*, SJ*);
    void (*start_memory)(MwsPlayer*, void*, int);
} MwsPlayerInterface;

typedef char MwsStHandleSizeCheck[sizeof(MwsStHandle) == 0x18 ? 1 : -1];
typedef char MwsCreateParamsSizeCheck[sizeof(MwsCreateParams) == 0x30 ? 1 : -1];
typedef char MwsPlayerSizeCheck[sizeof(MwsPlayer) == 0x2B8 ? 1 : -1];
typedef char MwsLibraryWorkSizeCheck[sizeof(MwsLibraryWork) == 0x162C ? 1 : -1];
typedef char MwsPlayerInterfaceSizeCheck[sizeof(MwsPlayerInterface) == 0x44 ? 1 : -1];

extern const SfdTransportInterface SFD_tr_in_mem, SFD_tr_sd_mps;
extern const SfdTransportInterface SFD_tr_vd_mpv, SFD_tr_ad_adxt;
extern const SfdTransportInterface SFD_tr_vo_manu, SFD_tr_ao_auto_p;
extern const SfdTransportInterface SFD_tr_uo;
extern MwsLibraryWork* MWSFLIB_GetLibWorkPtr(void);
extern int MWSFLIB_SetErrCode(int);
extern void MWSFLIB_SfdErrFunc(SfdCallbackObject, int);
extern void MWSFSVM_Error(const char*, ...);
extern int MWSFD_GetUsePicUsr(void);
extern void MWSFPLY_SetFlowLimit(MwsPlayer*);
extern void MWSFSVR_SetHnMwplySvrFlg(MwsPlayer*, int);
extern void MWSFSVR_SetHnSfdSvrFlg(MwsPlayer*, int);
extern void mwSfdStopDec(MwsPlayer*);
extern void MWSFTAG_DestroyAinfSj(MwsPlayer*);
extern SJ* MWSFTAG_CreateAinfSj(MwsPlayer*);
extern int MWSFTAG_SetAinfSj(MwsPlayer*);
extern void MWSFTAG_InitTagInf(MwsPlayer*);
extern int MWSFTAG_IsUseAinfSj(const MwsCreateParams*);
extern void MWSFFRM_InitSfhInfTable(MwsPlayer*);
extern void MWSFFRM_SetShfCbFn(MwsPlayer*);
extern void MWSST_Destroy(MwsStHandle*);
extern SFXHandle* MWSFSFX_Create(void*, int, int, int);
extern void MWSFSFX_Destroy(SFXHandle*);
extern int MWSFSFX_CalcHnWorkSiz(int, int);
extern void MWSFSFX_SetCompoMode(MwsPlayer*, int);
extern ADXStream* MWSTM_Create(SJ*);
extern void MWSTM_Destroy(ADXStream*);
extern LSC* LSC_Create(SJ*);
extern void LSC_Destroy(LSC*);
extern void LSC_SetStmHndl(LSC*, ADXStream*);
extern int SFD_SetPicUsrBuf(SfdHandle*, void*, int, int);
extern int SFD_SetMpvCond(SfdHandle*, int, int);

extern void mwSfdVsync(void);
extern int mwSfdExecSvrHndl(MwsPlayer*);
extern void mwSfdDestroy(MwsPlayer*);
extern void mwSfdStartFname(MwsPlayer*, const char*);
extern void mwSfdStop(MwsPlayer*);
extern int mwSfdGetStat(MwsPlayer*);
extern void mwSfdGetTime(MwsPlayer*, int*, int*);
extern void mwSfdPause(MwsPlayer*, int);
extern void mwSfdSetOutVol(MwsPlayer*, int);
extern int mwSfdGetOutVol(MwsPlayer*);
extern void mwSfdSetOutPan(MwsPlayer*, int);
extern int mwSfdGetOutPan(MwsPlayer*);
extern void mwSfdStartSj(MwsPlayer*, SJ*);
extern void mwSfdStartMem(MwsPlayer*, void*, int);

#pragma force_active on
static const SfdTransportSetup mwsfd_mps_trsetup = {{
    &SFD_tr_in_mem, &SFD_tr_sd_mps, &SFD_tr_vd_mpv, &SFD_tr_ad_adxt,
    0, 0, &SFD_tr_vo_manu, &SFD_tr_ao_auto_p, &SFD_tr_uo
}};
static const SfdTransportSetup mwsfd_mpv_trsetup = {{
    &SFD_tr_in_mem, 0, &SFD_tr_vd_mpv, 0, 0, 0, &SFD_tr_vo_manu, 0, 0
}};
static const SfdTransportSetup mwsfd_vonlysfd_trsetup = {{
    &SFD_tr_in_mem, &SFD_tr_sd_mps, &SFD_tr_vd_mpv, 0, 0, 0,
    &SFD_tr_vo_manu, 0, &SFD_tr_uo
}};
static const char allocation_count_exceeded[0x24] = "E2053001 MWSFD_Malloc: cnt over.";
static const char header_null[0x28] = "E204161: mwPlyGetHdrInf(): NULL pointer";
static const char header_size_invalid[0x2C] = "E204162: mwPlyGetHdrInf(): bufsize error";
static const char destroy_failed[0x20] = "E20010703E MWSFCRE_DestroySfd: ";
static const char allocation_leak[] = "E2053005: forgot free.";
static const char composition_work_failed[0x24] = "E2053003: not enough work: sfx_wk";
static const char additional_info_work_failed[0x28] = "E2053004: not enough work: ainfsj_buf";
static const char invalid_buffer_format[0x24] = "E3012102: Buffer format is invalid.";
static const char create_parameter_null[0x2C] = "E1122612 mwPlyCreateSofdec : cprm is NULL.";
static const char player_limit_exceeded[0x50] = "E4061801 mwPlyCreateSofdec: Number of MWPLY handles exceeds its maximum number.";
static const char allocator_missing[0x3C] = "E2053006 mwPlyCreateSofdec: Didn't set malloc/free func.";
static const char create_sfd_failed[0x24] = "E2012 mwPlyCreate:can't create SFD";
static const char picture_user_internal_error[] = "E02120501: Internal Error: mwsfcre_AttachPicUsrBuf().";
static const char create_picture_short_message[] =
    "E02120502: mwsfcre_AttachPicUsrBuf(): usrdatbuf is short.";
static const float create_picture_rounding_half = 0.5f;
static const char input_sj_failed[0x24] = "E2013 mwPlyCreate:can't create SJ";
static const char memory_sj_failed[0x24] = "E2020 mwPlyCreate:can't create SJ";
static const char create_sfx_failed[0x1C] = "E201185: can't create SfxHn";
static const char additional_info_failed[0x2C] = "E201212 mwPlyCreate: can't set AddInfSJ";
#pragma force_active reset

MwsPlayerInterface mwsfd_if = {
    {0, 0, 0}, mwSfdVsync, mwSfdExecSvrHndl, mwSfdDestroy,
    mwSfdStartFname, mwSfdStop, mwSfdGetStat, mwSfdGetTime, mwSfdPause,
    mwSfdSetOutVol, mwSfdGetOutVol, mwSfdSetOutPan, mwSfdGetOutPan,
    mwSfdStartSj, mwSfdStartMem
};

static SfdCreateConfig mwsfd_sfdmps_crepara = {
    {&mwsfd_mps_trsetup, 0, {0x10000, 0x50800, 0x12000, 0, 0, 0, 0}, 0, 0x800},
    3, {0}, 3, 0, 0
};
static SfdCreateConfig mwsfd_sfdmpv_crepara = {
    {&mwsfd_mpv_trsetup, 0, {0x10000, 0x50800, 0x12000, 0, 0, 0, 0}, 0, 0x800},
    3, {0}, 3, 0, 0
};
static SfdCreateConfig mwsfd_vonlysfd_crepara = {
    {&mwsfd_vonlysfd_trsetup, 0, {0x10000, 0x50800, 0x12000, 0, 0, 0, 0}, 0, 0x800},
    3, {0}, 3, 0, 0
};
static SfdMpvParameters mwsfd_mpvpara = {
    0xC0, 0xF0, 0x160, 0x1E0, 0, 0x160, 0x1E0, 4, 0};
static SfdAdxtParameters mwsfd_adxtpara = {0x5DCC, 0x120, 0, 2, 0xBB80, 0xC1C0, 0};
static int mwsfd_packsize = 0x800;

/* User-supplied frame buffers. */
static int mwsfdcre_bufnum;
static int mwsfdcre_bufsize;
static void* mwsfdcre_bufptr[16];
/* Component buffer sizes decided by mwsfcre_CreateSfd. */
static int adxibuf;  /* ADXT input ring buffer */
static int adxwk;    /* ADXT work */
static int tab;      /* decoded frame table */
static int rfb;      /* reference frame buffers */
static int aib;      /* audio input buffer */
static int vib;      /* video input buffer */
static int sib;      /* system input buffer */
static int sjb;      /* file stream joint ring buffer */
static unsigned char* mwsfd_sisjadr; /* 64-byte aligned stream joint buffer */

static inline void* mwsfcre_Alloc(MwsPlayer* player, int size)
{
    MwsLibraryWork* work;
    void* memory;
    if (player->allocation_count >= 32) {
        MWSFSVM_Error(allocation_count_exceeded);
        return 0;
    }
    if (size < 0) return 0;
    if (player->arena != 0) {
        if (player->arena_used + size > player->arena_size) {
            memory = 0;
        } else {
            memory = player->arena_cursor;
            player->arena_cursor += size;
            player->arena_used += size;
        }
    } else {
        work = MWSFLIB_GetLibWorkPtr();
        memory = work->malloc_fn(work->allocator_object, size);
    }
    if (memory != 0) {
        player->allocations[player->allocation_count] = memory;
        player->allocation_count++;
    }
    return memory;
}

static inline void mwsfcre_FreeAll(MwsPlayer* player)
{
    MwsLibraryWork* work;
    int index;
    void* memory;
    for (index = 0; index < 32; index++) {
        int reverse_index = 31 - index;
        memory = player->allocations[reverse_index];
        if (memory != 0) {
            if (player->arena == 0) {
                work = MWSFLIB_GetLibWorkPtr();
                work->free_fn(work->allocator_object, memory);
            }
            player->allocation_count--;
            player->allocations[reverse_index] = 0;
        }
    }
}

static inline int mwsfcre_CalcFrameSize(int source_width, int source_height)
{
    int height;
    int width;
    int luma_size;
    int chroma_size;

    width = (source_width + 15) / 16 * 16;
    height = (source_height + 15) / 16 * 16;
    luma_size = height * ((width + 31) / 32 * 32);
    chroma_size = (height / 2) * ((width / 2 + 31) / 32 * 32);
    return luma_size + chroma_size * 2 + 0x20;
}

static inline int mwsfcre_UsesAudio(int file_type)
{
    int uses_audio;
    switch (file_type) {
    case 2:
    case 3:
        uses_audio = 0;
        break;
    default:
        uses_audio = 1;
        break;
    }
    return uses_audio;
}

static inline void mwsfcre_AttachPictureUser(MwsPlayer* player,
                                              const char* short_error)
{
    MwsPictureUserConfig* config = player->picture_user;
    void* buffer;
    int element_size;
    int skip_count;

    if (config == 0) {
        MWSFSVM_Error(picture_user_internal_error);
    } else {
        skip_count = player->create.frame_count;
        element_size = config->element_size;
        buffer = config->buffer;
        if (config->buffer_size < (skip_count + 3) * element_size) {
            MWSFSVM_Error(short_error);
        } else if (MWSFD_GetUsePicUsr() == 1) {
            SFD_SetPicUsrBuf(player->sfd, buffer, skip_count + 3, element_size);
        }
    }
}

void mwPlySetMallocFn(MwsMallocFn malloc_fn, MwsFreeFn free_fn, void* object)
{
    MwsLibraryWork* work = MWSFLIB_GetLibWorkPtr();
    work->malloc_fn = malloc_fn;
    work->free_fn = free_fn;
    work->allocator_object = object;
}

/* Decoder-side teardown; retail inlines it into mwSfdDestroy. */
static inline void MWSFCRE_DestroySfd(MwsPlayer* player)
{
    if (player->loader != 0) LSC_Destroy(player->loader);
    if (player->stream != 0) MWSTM_Destroy(player->stream);
    if (player->input_sj != 0)
        player->input_sj->interface->destroy(player->input_sj);
    if (player->memory_sj != 0)
        player->memory_sj->interface->destroy(player->memory_sj);
    if (player->sfd != 0) {
        if (SFD_Destroy(player->sfd) != 0) {
            MWSFLIB_SetErrCode(-0x132);
            MWSFSVM_Error(destroy_failed);
        }
    }
    MWSST_Destroy(&player->sound);
    mwsfcre_FreeAll(player);
    if (player->allocation_count != 0) MWSFSVM_Error(allocation_leak);
    memset(player, 0, sizeof(*player));
    player->active = 0;
    player->interface = &mwsfd_if;
}

void mwSfdDestroy(MwsPlayer* player)
{
    if (player == 0) {
        return;
    }
    mwSfdStopDec(player);
    player->active = 0;
    MWSFTAG_DestroyAinfSj(player);
    if (player->sfx != 0) MWSFSFX_Destroy(player->sfx);
    MWSFCRE_DestroySfd(player);
}

/* TODO: [near miss] 97.768364%; shared free-loop declaration order changes register coloring; allocation behavior retained. */
static int mwsfcre_MallocCompoWork(MwsPlayer* player)
{
    const MwsCreateParams* create = &player->create;
    int size = MWSFSFX_CalcHnWorkSiz(player->create.width, player->create.height);
    void* memory = mwsfcre_Alloc(player, size);
    if (memory == 0) {
        MWSFSVM_Error(composition_work_failed);
        mwsfcre_FreeAll(player);
        return -1;
    }
    player->composition_work = memory;
    player->composition_work_size = size;
    if (MWSFTAG_IsUseAinfSj(create) == 1) {
        memory = mwsfcre_Alloc(player, 0x20000);
        if (memory == 0) {
            MWSFSVM_Error(additional_info_work_failed);
            mwsfcre_FreeAll(player);
            return -1;
        }
        player->additional_info_buffer = memory;
        player->additional_info_buffer_size = 0x20000;
    } else {
        player->additional_info_buffer = 0;
        player->additional_info_buffer_size = 0;
    }
    return 0;
}

/* Linker-discarded in retail: installs user frame buffers instead of component work. Its
 * references give the .bss objects their retail first-reference order. */
/* TODO: [blocked] emitted here but absent from retail; `inline` loses the .bss order, so this
 * relies on link-time dead stripping once mwsfdcre links (unverified while NonMatching). */
void mwPlySetFrmBuf(int count, int size, void** buffers)
{
    int i;

    mwsfdcre_bufnum = count;
    mwsfdcre_bufsize = size;
    for (i = 0; i < 16; i++) {
        mwsfdcre_bufptr[i] = buffers[i];
    }
    adxibuf = 0;
    adxwk = 0;
    tab = 0;
    rfb = 0;
    aib = 0;
    vib = 0;
    sib = 0;
    sjb = 0;
    mwsfd_sisjadr = 0;
}

static int mwsfcre_MallocRfb(MwsPlayer*, const MwsCreateParams*,
                             MwsReferenceBuffers*);
static SfdHandle* mwsfcre_CreateSfd(MwsPlayer*, const MwsCreateParams*);

static inline int mwsfcre_IsValidBufFmt(const MwsCreateParams* params)
{
    int valid = 1;

    if (params->buffer_format != 0 && params->buffer_format != 3) {
        MWSFSVM_Error(invalid_buffer_format);
        valid = 0;
    }
    return valid;
}

static inline int mwsfcre_ChkMallocFn(const MwsCreateParams* params)
{
    MwsLibraryWork* work = MWSFLIB_GetLibWorkPtr();
    int result = 0;

    if (params->work == 0) {
        if (work->malloc_fn == 0) result = -1;
        if (work->free_fn == 0) result = -1;
    }
    return result;
}

static inline void mwsfcre_InitCompoWork(MwsPlayer* player,
                                        const MwsCreateParams* params)
{
    int index;

    player->arena = params->work;
    player->arena_size = params->work_size;
    player->arena_cursor = params->work;
    player->arena_used = 0;
    player->allocation_count = 0;
    for (index = 0; index < 32; index++) {
        player->allocations[index] = 0;
    }
}

static inline void mwsfcre_SetSfdCond(MwsPlayer* player, MwsLibraryWork* work)
{
    float frame_time;
    int frames;
    int pool;
    int frequency;
    SfdHandle* sfd;

    sfd = player->sfd;
    pool = work->frame_pool;
    frequency = work->video_frequency;
    SFD_SetCond(sfd, 8, 0);
    SFD_SetCond(sfd, 1, 1);
    SFD_SetCond(sfd, 0, 0);
    SFD_SetCond(sfd, 0x17, 4);
    frame_time = create_picture_rounding_half + (float)(frequency * pool * 1000);
    frames = (int)frame_time;
    if ((float)frames > frame_time) frames--;
    SFD_SetCond(sfd, 0x2D, frames);
    SFD_SetCond(sfd, 0x2C, frames);
    SFD_SetCond(sfd, 0x2A, frames);
    SFD_SetCond(sfd, 0xF, 2);
    SFD_SetCond(sfd, 0x33, 0);
    SFD_SetCond(sfd, 0xE, 0);
    SFD_SetCond(sfd, 0x1C, 0);
    SFD_SetMpvCond(sfd, 5, 0);
}

/* TODO: [near miss] 97.41461%; RE4 prm copy, SfdCond/PicUsr/DestroySfd shapes restored;
 * retail keeps the compo-table unroll guard and a player copy at each inlined destroy. */
MwsPlayer* mwPlyCreateSofdec(const MwsCreateParams* params)
{
    MwsLibraryWork* work;
    MwsPlayer* player;
    SfdHandle* sfd;
    SFXHandle* sfx;
    int flow_limit;
    int index;

    if (params == 0) {
        MWSFSVM_Error(create_parameter_null);
        return 0;
    }
    if (mwsfcre_IsValidBufFmt(params) != 1) {
        return 0;
    }
    work = MWSFLIB_GetLibWorkPtr();
    for (index = 0; index < 8; index++) {
        player = &work->players[index];
        if (player->active == 0) break;
    }
    if (index == 8) {
        MWSFLIB_SetErrCode(-0xB);
        MWSFSVM_Error(player_limit_exceeded);
        return 0;
    }
    if (mwsfcre_ChkMallocFn(params) == -1) {
        MWSFSVM_Error(allocator_missing);
        return 0;
    }

    if (player != 0) memset(player, 0, sizeof(*player));
    mwsfcre_InitCompoWork(player, params);
    player->create = *params;

    sfd = mwsfcre_CreateSfd(player, params);
    player->sfd = sfd;
    if (player->sfd == 0) {
        MWSFSVM_Error(create_sfd_failed);
        mwSfdDestroy(player);
        return 0;
    }
    mwsfcre_AttachPictureUser(player, create_picture_short_message);

    {
        int sector_count = params->decoder_count;
        int maximum_bps;
        int file_type;

        file_type = params->file_type;
        maximum_bps = params->maximum_bps;
        if (sector_count <= 0) {
            sector_count = 1;
        }
        if (file_type == 2) {
            flow_limit = sector_count * (maximum_bps / 8 / 0x800 * 0x800);
        } else if (file_type == 3) {
            flow_limit = sector_count * (maximum_bps / 8 / 0x800 * 0x800);
        } else {
            flow_limit = sector_count * (maximum_bps / 8 / 0x800 * 0x800);
        }
    }
    mwsfcre_SetSfdCond(player, work);

    player->input_sj = SJRBF_Create(player->input_buffer,
                                    player->input_buffer_size,
                                    player->input_buffer_extra_size);
    if (player->input_sj == 0) {
        MWSFSVM_Error(input_sj_failed);
        mwSfdDestroy(player);
        return 0;
    }
    player->memory_sj = SJMEM_Create(0, 0);
    if (player->memory_sj == 0) {
        MWSFSVM_Error(memory_sj_failed);
        mwSfdDestroy(player);
        return 0;
    }
    player->interface = &mwsfd_if;
    player->flow_limit = flow_limit;
    player->playback_mode = 1;
    player->field_06C = 0;
    player->status = 0;
    player->field_050 = params->composition_mode;
    player->field_054 = params->composition_mode;
    if (SFD_GetTrHn(sfd, 3, &player->transport) != 0) player->transport = 0;
    player->field_070 = 1;
    player->concat_play = 0;
    player->concat_stopped = 0;
    player->paused = 0;
    player->field_077 = 0;
    player->field_060 = 0;
    MWSFSVR_SetHnMwplySvrFlg(player, 0);
    MWSFSVR_SetHnSfdSvrFlg(player, 0);
    player->field_058 = 1;
    player->field_088 = 0;

    player->stream = MWSTM_Create(player->input_sj);
    if (player->stream == 0) {
        mwSfdDestroy(player);
        return 0;
    }
    MWSFPLY_SetFlowLimit(player);
    player->loader = LSC_Create(player->input_sj);
    player->entry_count = 0;
    LSC_SetStmHndl(player->loader, player->stream);
    if (mwsfcre_MallocCompoWork(player) == -1) {
        mwSfdDestroy(player);
        return 0;
    }
    sfx = MWSFSFX_Create(player->composition_work,
                         player->composition_work_size,
                         params->width, params->height);
    if (sfx == 0) {
        MWSFSVM_Error(create_sfx_failed);
        mwSfdDestroy(player);
        return 0;
    }
    player->sfx = sfx;
    MWSFSFX_SetCompoMode(player, player->create.composition_mode);
    player->additional_info_sj = MWSFTAG_CreateAinfSj(player);
    if (MWSFTAG_SetAinfSj(player) != 0) {
        MWSFSVM_Error(additional_info_failed);
        mwSfdDestroy(player);
        return 0;
    }
    MWSFTAG_InitTagInf(player);
    MWSFFRM_InitSfhInfTable(player);
    MWSFFRM_SetShfCbFn(player);
    player->active = 1;
    return player;
}

#pragma force_active on
static const char reset_stop_failed[] =
    "E0203261: MWSFCRE_ResetSfdHn: SFD_Stop() failed.";
static const char reset_error_callback_failed[] =
    "E0203262: MWSFCRE_ResetSfdHn: SFD_SetErrFn() failed.";
static const char reset_picture_user_short[0x38] =
    "E02120503: mwPlyAttachPicUsrBuf(): bufsize is short.";
static const char create_buffer_format_invalid[0x28] =
    "E206011: MwsfdCrePrm: illigal buffmt.";
static const char create_work_failed[0x1C] = "E2053002: not enough work";
static const char create_audio_work_failed[0x1C] = "E4041301: not enough work";
static const char sfd_create_error[0x2C] =
    "E20010703C mwPlyCreateSofdec: create error";
static const char sfd_error_callback_error[0x28] =
    "E20010703D mwPlyCreateSofdec: set errcb";
static const char supply_sj_failed[0x24] =
    "E20010703B MWSFCRE_SetSupplySj: ";
static const char calculate_work_parameter_null[0x34] =
    "E1122613 mwPlyCalcWorkCprmSfd: cprm is NULL.";
#pragma force_active reset

int MWSFCRE_ResetSfdHn(MwsPlayer* player)
{
    MwsPictureUserConfig* config;
    int frame_count;
    int element_size;
    void* buffer;
    SfdHandle* sfd;

    sfd = player->sfd;
    if (SFD_Stop(sfd) != 0) {
        MWSFSVM_Error(reset_stop_failed);
        return -1;
    }
    if (SFD_SetErrFn(sfd, MWSFLIB_SfdErrFunc, player) != 0) {
        MWSFLIB_SetErrCode(-0x12F);
        MWSFSVM_Error(reset_error_callback_failed);
        return -1;
    }
    config = player->picture_user;
    if (config == 0) {
        MWSFSVM_Error(picture_user_internal_error);
    } else {
        frame_count = player->create.frame_count;
        element_size = config->element_size;
        buffer = config->buffer;
        if (config->buffer_size < (frame_count + 3) * element_size) {
            MWSFSVM_Error(create_picture_short_message);
        } else if (MWSFD_GetUsePicUsr() == 1) {
            SFD_SetPicUsrBuf(player->sfd, buffer, frame_count + 3,
                             element_size);
        }
    }
    return 0;
}

static inline int mwsfcre_CnvBufFmt(int format)
{
    int result;

    switch (format) {
    case 0: result = 3; break;
    case 1: result = 1; break;
    case 2: result = 2; break;
    case 3: result = 3; break;
    default:
        MWSFSVM_Error(create_buffer_format_invalid);
        result = 3;
        break;
    }
    return result;
}

static inline int mwsfcre_MallocFrmTbl(MwsPlayer* player,
                                      const MwsCreateParams* params,
                                      void** frame_buffers)
{
    int result = 0;
    int frame_count = params->frame_count;
    int frame_size;
    int index;

    if (params->buffer_format < 0 || params->buffer_format >= 4)
        MWSFSVM_Error(create_buffer_format_invalid);
    frame_size = mwsfcre_CalcFrameSize(params->width, params->height);
    if (mwsfdcre_bufnum != 0) {
        if (mwsfdcre_bufnum < frame_count + 2 ||
            mwsfdcre_bufsize < frame_size) {
            result = -1;
        } else {
            for (index = 0; index < frame_count; index++) {
                frame_buffers[index] = mwsfdcre_bufptr[index + 2];
                if (frame_buffers[index] == 0) result = -1;
            }
        }
    } else {
        for (index = 0; index < frame_count; index++) {
            frame_buffers[index] = mwsfcre_Alloc(player, frame_size);
            if (frame_buffers[index] == 0) result = -1;
        }
    }
    return result;
}

/* Component allocation through a sized local (retail keeps the size test). */
static inline void* mwsfcre_MallocWk(MwsPlayer* player, int work_size)
{
    int size = work_size;

    return mwsfcre_Alloc(player, size);
}

static inline void* mwsfcre_MallocX(MwsPlayer* player, int size)
{
    return mwsfcre_Alloc(player, size);
}

/* TODO: [near miss] 94.27381%; retail .bss objects and RE4 size/frame blocks restored; the
 * 0x4000/0x700 allocations keep an unfolded size test and the inlined Rfb schedule differs. */
static SfdHandle* mwsfcre_CreateSfd(MwsPlayer* player,
                                    const MwsCreateParams* params)
{
    MwsReferenceBuffers references;
    void* frame_buffers[16];
    SfdCreateConfig create;
    SfdHandle* sfd;
    void* shared_work;
    void* stream_work;
    void* audio_stream_buffer;
    void* audio_decoder_work;
    MwsPictureUserWork* picture_user_work;
    void* decoder_work;
    void* video_work;
    void* filename_work;
    int frame_size;
    int frame_result;
    int rfb_result;
    int output_format;
    int file_type;
    int width;
    int height;
    int frame_count;
    int allocation_size;

    file_type = params->file_type;
    frame_count = params->frame_count;
    width = params->width;
    height = params->height;
    {
        int sector_count = params->decoder_count;
        int bps;

        bps = params->maximum_bps;
        if (sector_count <= 0) {
            sector_count = 1;
        }
        if (file_type == 2) {
            sjb = sector_count * (bps / 8 / 0x800 * 0x800);
            sib = 0;
            vib = 0;
            aib = 0;
            adxibuf = 0;
            adxwk = 0;
        } else if (file_type == 3) {
            sjb = sector_count * (bps / 8 / 0x800 * 0x800);
            vib = bps / 8 / 0x800 * 0x800 / 2 + 0x800;
            sib = 0;
            aib = 0;
            adxibuf = 0;
            adxwk = 0;
        } else {
            sjb = sector_count * (bps / 8 / 0x800 * 0x800);
            vib = bps / 8 / 0x800 * 0x800 / 2 + 0x800;
            sib = 0;
            aib = 0x5DCC;
            adxibuf = 0x5F0C;
            adxwk = 0xC1C0;
        }
    }

    if (mwsfdcre_bufnum != 0) {
        rfb = 0;
        tab = 0;
    } else {
        int table_frames = params->frame_count;
        int frame_height;
        int frame_width;

        frame_width = params->width;
        frame_height = params->height;
        if (params->buffer_format >= 4 || params->buffer_format < 0) {
            MWSFSVM_Error(create_buffer_format_invalid);
        }
        frame_size = mwsfcre_CalcFrameSize(frame_width, frame_height);
        rfb = frame_size * 2;
        tab = table_frames * frame_size;
    }

    allocation_size = aib +
                      vib +
                      sib + 0x20;
    shared_work = mwsfcre_Alloc(player, allocation_size);
    allocation_size = sjb + 0x40;
    stream_work = mwsfcre_Alloc(player, allocation_size);
    rfb_result = mwsfcre_MallocRfb(player, params, &references);
    frame_result = mwsfcre_MallocFrmTbl(player, params, frame_buffers);

    if (mwsfcre_UsesAudio(file_type) == 1) {
        allocation_size = adxibuf;
        audio_stream_buffer = mwsfcre_Alloc(player, allocation_size);
        allocation_size = adxwk;
        audio_decoder_work = mwsfcre_Alloc(player, allocation_size);
    } else {
        audio_stream_buffer = 0;
        audio_decoder_work = 0;
    }
    picture_user_work = mwsfcre_MallocX(player, 0x800);
    decoder_work = mwsfcre_MallocWk(player, 0x4000);
    video_work = mwsfcre_MallocWk(player, 0x700);
    filename_work = mwsfcre_MallocX(player, 0x100);
    if (shared_work == 0 || stream_work == 0 || rfb_result != 0 ||
        frame_result != 0 || picture_user_work == 0 || decoder_work == 0 ||
        filename_work == 0 || video_work == 0) {
        MWSFSVM_Error(create_work_failed);
        mwsfcre_FreeAll(player);
        return 0;
    }
    if (mwsfcre_UsesAudio(file_type) == 1) {
        if (audio_stream_buffer == 0 || audio_decoder_work == 0) {
            MWSFSVM_Error(create_audio_work_failed);
            mwsfcre_FreeAll(player);
            return 0;
        }
    }

    mwsfd_sisjadr = (unsigned char*)
        (((unsigned int)stream_work + 0x3F) & ~0x3F);
    {
        int current_height;
        int current_width;
        current_width = params->width;
        current_height = params->height;
        mwsfd_mpvpara.chroma_width = (((current_width / 2) + 31) / 32) * 32;
        mwsfd_mpvpara.chroma_height = current_height / 2;
        mwsfd_mpvpara.width = current_width;
        mwsfd_mpvpara.height = current_height;
        mwsfd_mpvpara.reference_buffer = 0;
        mwsfd_mpvpara.maximum_width = current_width;
        mwsfd_mpvpara.maximum_height = current_height;
        mwsfd_mpvpara.frame_count = frame_count;
        mwsfd_mpvpara.frame_buffer = 0;
    }
    mwsfd_adxtpara.stream_buffer = audio_stream_buffer;
    mwsfd_adxtpara.decoder_buffer = audio_decoder_work;

    switch (file_type) {
    case 1:
        create = mwsfd_sfdmps_crepara;
        player->input_buffer = mwsfd_sisjadr;
        player->input_buffer_size = sjb -
                                    mwsfd_packsize;
        player->input_buffer_extra_size = mwsfd_packsize;
        break;
    case 2:
        create = mwsfd_sfdmpv_crepara;
        player->input_buffer = mwsfd_sisjadr;
        player->input_buffer_size =
            sjb - 0x800;
        player->input_buffer_extra_size = 0x800;
        break;
    case 3:
        create = mwsfd_vonlysfd_crepara;
        player->input_buffer = mwsfd_sisjadr;
        player->input_buffer_size = sjb -
                                    mwsfd_packsize;
        player->input_buffer_extra_size = mwsfd_packsize;
        break;
    }
    create.buffer.ring_alignment = mwsfd_packsize;
    if (sib != 0) {
        sib -=
            sib % mwsfd_packsize;
    }

    output_format = mwsfcre_CnvBufFmt(params->buffer_format);
    create.buffer.memory = shared_work;
    create.buffer.buffer_sizes[0] = sib;
    create.buffer.buffer_sizes[1] = vib;
    create.buffer.buffer_sizes[2] = aib;
    create.picture_user_buffer_minimum = frame_count;
    create.maximum_width = width;
    create.maximum_height = height;
    create.video_output_format = output_format;
    create.handle_memory = decoder_work;
    create.handle_memory_size = 0x4000;
    SFD_SetMpvParaTbl(&mwsfd_mpvpara, references.buffers, frame_buffers);
    switch (file_type) {
    case 1:
        SFD_SetAdxtPara(&mwsfd_adxtpara);
        break;
    }
    sfd = SFD_Create(&create, 0);
    if (sfd == 0) {
        MWSFLIB_SetErrCode(-0x131);
        MWSFSVM_Error(sfd_create_error);
        return 0;
    }
    if (SFD_SetErrFn(sfd, MWSFLIB_SfdErrFunc, player) != 0) {
        MWSFLIB_SetErrCode(-0x12F);
        MWSFSVM_Error(sfd_error_callback_error);
        return 0;
    }
    player->filename = filename_work;
    player->filename_capacity = 0x100;
    player->picture_user_work = picture_user_work;
    player->picture_user_header_size = 0x40;
    player->picture_user_write = 0;
    player->picture_user_read = 0;
    {
        MwsPictureUserConfig* picture_user = &player->internal_picture_user;
        picture_user->buffer = picture_user_work->element_buffer;
        picture_user->buffer_size = 0x7C0;
        picture_user->element_size = 0x40;
        player->picture_user = picture_user;
    }
    return sfd;
}

/* TODO: [breakthrough needed] 84.51479%; RE4 body over the retail .bss objects; retail issues
 * the saves and buffer-count load before the frame-size math (pure schedule; peephole-off and
 * permuter forms rejected); needs scheduling evidence. */
static int mwsfcre_MallocRfb(MwsPlayer* player,
                             const MwsCreateParams* params,
                             MwsReferenceBuffers* output)
{
    int result = 0;
    int frame_size;

    frame_size = mwsfcre_CalcFrameSize(params->width, params->height);
    if (mwsfdcre_bufnum != 0) {
        /* Retail branches twice on one size comparison, as in RE4. */
        if (mwsfdcre_bufnum < 2 ||
            mwsfdcre_bufsize < frame_size ||
            mwsfdcre_bufsize < frame_size) {
            output->buffers[0] = 0;
            output->buffers[1] = 0;
            result = -1;
        } else {
            output->buffers[0] = mwsfdcre_bufptr[0];
            output->buffers[1] = mwsfdcre_bufptr[1];
        }
    } else {
        output->buffers[0] = mwsfcre_Alloc(player, frame_size);
        output->buffers[1] = mwsfcre_Alloc(player, frame_size);
    }
    if (output->buffers[0] == 0 || output->buffers[1] == 0) result = -1;
    return result;
}

void MWSFCRE_SetSupplySj(MwsPlayer* player)
{
    SfdBufferSupply supply;
    SJ* stream = player->supply_sj;
    SfdHandle* sfd = player->sfd;
    if (stream == 0) return;
    if (stream == player->memory_sj) {
        supply.kind = 1;
        supply.stream_joint = stream;
        supply.buffer = player->memory_buffer;
        supply.buffer_size = player->memory_buffer_size;
        supply.extra_size = 0;
        supply.field_14 = 0;
    } else if (stream == player->input_sj) {
        supply.kind = 0;
        supply.stream_joint = stream;
        supply.buffer = player->input_buffer;
        supply.buffer_size = player->input_buffer_size;
        supply.extra_size = player->input_buffer_extra_size;
        supply.field_14 = 0;
    } else {
        supply.kind = player->supply_mode;
        supply.stream_joint = stream;
        supply.buffer = player->supply_buffer;
        supply.buffer_size = player->supply_buffer_size;
        supply.extra_size = player->supply_buffer_extra_size;
        supply.field_14 = 0;
    }
    if (SFD_SetSupplySj(sfd, &supply) != 0) {
        MWSFLIB_SetErrCode(-0x138);
        MWSFSVM_Error(supply_sj_failed);
    }
}
