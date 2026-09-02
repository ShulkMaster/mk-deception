#include "cri/sj.h"
#include "runtime/cstring.h"
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
    int file_type;
    int maximum_bps;
    int width;
    int height;
    int frame_count;
    int decoder_count;
    void* create_work;
    int create_work_size;
    int composition_mode;
    int buffer_format;
    int create_field_28;
    int create_field_2C;
    int playback_mode;
    SfdHandle* sfd;
    void* stream;
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
    void* picture_user_work;
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
    int maximum_width;
    int decoder_count;
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

typedef struct MwsCreatePictureLiterals {
    char short_message[0x3C];
    float rounding_half;
} MwsCreatePictureLiterals;

typedef struct MwsCreateBss {
    int buffer_count;
    int buffer_size;
    void* buffers[16];
    int adx_input_buffer_size;
    int adx_decoder_work_size;
    int table_size;
    int reference_buffer_size;
    int audio_input_buffer_size;
    int video_input_buffer_size;
    int system_input_buffer_size;
    int stream_joint_buffer_size;
    unsigned char* stream_joint_buffer;
    int section_tail;
} MwsCreateBss;

typedef struct MwsPackSizeData {
    int value;
    int section_tail;
} MwsPackSizeData;

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
typedef char MwsCreateBssSizeCheck[sizeof(MwsCreateBss) == 0x70 ? 1 : -1];

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
extern void* MWSTM_Create(SJ*);
extern void MWSTM_Destroy(void*);
extern LSC* LSC_Create(SJ*);
extern void LSC_Destroy(LSC*);
extern void LSC_SetStmHndl(LSC*, void*);
extern int SFD_SetPicUsrBuf(SfdHandle*, void*, int, int);
extern int SFD_SetMpvCond(SfdHandle*, int, int);
extern void SFD_SetMpvParaTbl(const int*, void* const*, void* const*, int, int,
                              int);

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
static const char allocation_leak[0x18] = "E2053005: forgot free.";
static const char composition_work_failed[0x24] = "E2053003: not enough work: sfx_wk";
static const char additional_info_work_failed[0x28] = "E2053004: not enough work: ainfsj_buf";
static const char invalid_buffer_format[0x24] = "E3012102: Buffer format is invalid.";
static const char create_parameter_null[0x2C] = "E1122612 mwPlyCreateSofdec : cprm is NULL.";
static const char player_limit_exceeded[0x50] = "E4061801 mwPlyCreateSofdec: Number of MWPLY handles exceeds its maximum number.";
static const char allocator_missing[0x3C] = "E2053006 mwPlyCreateSofdec: Didn't set malloc/free func.";
static const char create_sfd_failed[0x24] = "E2012 mwPlyCreate:can't create SFD";
static const char picture_user_internal_error[0x38] = "E02120501: Internal Error: mwsfcre_AttachPicUsrBuf().";
static const MwsCreatePictureLiterals create_picture_literals = {
    "E02120502: mwsfcre_AttachPicUsrBuf(): usrdatbuf is short.", 0.5f
};
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
static int mwsfd_mpvpara[9] = {0xC0, 0xF0, 0x160, 0x1E0, 0, 0x160, 0x1E0, 4, 0};
static SfdAdxtParameters mwsfd_adxtpara = {0x5DCC, 0x120, 0, 2, 0xBB80, 0xC1C0, 0};
static MwsPackSizeData mwsfd_packsize = {0x800, 0};
static MwsCreateBss mwsfdcre_bufnum;

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
    void* memory;
    int index;
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

static inline int mwsfcre_CalcFrameSize(const MwsCreateParams* params)
{
    int width = ((params->width + 15) / 16) * 16;
    int height = ((params->height + 15) / 16) * 16;
    int luma_stride = ((width + 31) / 32) * 32;
    int chroma_stride = (((width / 2) + 31) / 32) * 32;
    return height * luma_stride + (height / 2) * chroma_stride * 2 + 0x20;
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
    int count;
    if (config == 0) {
        MWSFSVM_Error(picture_user_internal_error);
        return;
    }
    count = player->frame_count + 3;
    if (config->buffer_size < count * config->element_size) {
        MWSFSVM_Error(short_error);
        return;
    }
    if (MWSFD_GetUsePicUsr() == 1) {
        SFD_SetPicUsrBuf(player->sfd, config->buffer, count, config->element_size);
    }
}

void mwPlySetMallocFn(MwsMallocFn malloc_fn, MwsFreeFn free_fn, void* object)
{
    MwsLibraryWork* work = MWSFLIB_GetLibWorkPtr();
    work->malloc_fn = malloc_fn;
    work->free_fn = free_fn;
    work->allocator_object = object;
}

void mwSfdDestroy(MwsPlayer* player)
{
    if (player != 0) {
        mwSfdStopDec(player);
        player->active = 0;
        MWSFTAG_DestroyAinfSj(player);
        if (player->sfx != 0) MWSFSFX_Destroy(player->sfx);
        if (player->loader != 0) LSC_Destroy(player->loader);
        if (player->stream != 0) MWSTM_Destroy(player->stream);
        if (player->input_sj != 0)
            player->input_sj->interface->destroy(player->input_sj);
        if (player->memory_sj != 0)
            player->memory_sj->interface->destroy(player->memory_sj);
        if (player->sfd != 0 && SFD_Destroy(player->sfd) != 0) {
            MWSFLIB_SetErrCode(-0x132);
            MWSFSVM_Error(destroy_failed);
        }
        MWSST_Destroy(&player->sound);
        mwsfcre_FreeAll(player);
        if (player->allocation_count != 0) MWSFSVM_Error(allocation_leak);
        memset(player, 0, sizeof(*player));
        player->active = 0;
        player->interface = &mwsfd_if;
    }
}

static int mwsfcre_MallocCompoWork(MwsPlayer* player)
{
    const MwsCreateParams* create =
        (const MwsCreateParams*)&player->file_type;
    int size = MWSFSFX_CalcHnWorkSiz(player->width, player->height);
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

static int mwsfcre_MallocRfb(MwsPlayer*, const MwsCreateParams*,
                             MwsReferenceBuffers*);
static SfdHandle* mwsfcre_CreateSfd(MwsPlayer*, const MwsCreateParams*);

MwsPlayer* mwPlyCreateSofdec(const MwsCreateParams* params)
{
    MwsLibraryWork* work;
    MwsLibraryWork* allocator_work;
    MwsPlayer* player;
    SfdHandle* sfd;
    int decoder_count;
    int flow_limit;
    int condition_value;
    int maximum_bps;
    int valid_format;
    int allocator_result;
    int index;
    float rounded_value;

    if (params == 0) {
        MWSFSVM_Error(create_parameter_null);
        return 0;
    }
    valid_format = 1;
    if (params->buffer_format != 0 && params->buffer_format != 3) {
        MWSFSVM_Error(invalid_buffer_format);
        valid_format = 0;
    }
    if (valid_format != 1) {
        return 0;
    }
    work = MWSFLIB_GetLibWorkPtr();
    player = &work->players[0];
    index = 0;
    if (player->active != 0) {
        player = &work->players[1]; index = 1;
        if (player->active != 0) {
            player = &work->players[2]; index = 2;
            if (player->active != 0) {
                player = &work->players[3]; index = 3;
                if (player->active != 0) {
                    player = &work->players[4]; index = 4;
                    if (player->active != 0) {
                        player = &work->players[5]; index = 5;
                        if (player->active != 0) {
                            player = &work->players[6]; index = 6;
                            if (player->active != 0) {
                                player = &work->players[7]; index = 7;
                                if (player->active != 0) index = 8;
                            }
                        }
                    }
                }
            }
        }
    }
    if (index == 8) {
        MWSFLIB_SetErrCode(-0xB);
        MWSFSVM_Error(player_limit_exceeded);
        return 0;
    }
    allocator_work = MWSFLIB_GetLibWorkPtr();
    allocator_result = 0;
    if (params->work == 0) {
        if (allocator_work->malloc_fn == 0) allocator_result = -1;
        if (allocator_work->free_fn == 0) allocator_result = -1;
    }
    if (allocator_result == -1) {
        MWSFSVM_Error(allocator_missing);
        return 0;
    }

    if (player != 0) memset(player, 0, sizeof(*player));
    player->arena = params->work;
    player->arena_size = params->work_size;
    player->arena_cursor = params->work;
    player->arena_used = 0;
    player->allocation_count = 0;
    index = 0;
    while (index < 32) {
        player->allocations[index] = 0;
        index++;
    }
    player->file_type = params->file_type;
    player->maximum_bps = params->maximum_bps;
    player->width = params->width;
    player->height = params->height;
    player->frame_count = params->frame_count;
    player->decoder_count = params->decoder_count;
    player->create_work = params->work;
    player->create_work_size = params->work_size;
    player->composition_mode = params->composition_mode;
    player->buffer_format = params->buffer_format;
    player->create_field_28 = params->field_28;
    player->create_field_2C = params->field_2C;

    sfd = mwsfcre_CreateSfd(player, params);
    player->sfd = sfd;
    if (player->sfd == 0) {
        MWSFSVM_Error(create_sfd_failed);
        mwSfdDestroy(player);
        return 0;
    }
    mwsfcre_AttachPictureUser(player, create_picture_literals.short_message);

    decoder_count = params->decoder_count;
    if (decoder_count <= 0) decoder_count = 1;
    maximum_bps = params->maximum_bps;
    if (params->file_type == 2) {
        flow_limit = decoder_count *
            (((maximum_bps / 8) / 0x800) * 0x800);
    } else if (params->file_type == 3) {
        flow_limit = decoder_count *
            (((maximum_bps / 8) / 0x800) * 0x800);
    } else {
        flow_limit = decoder_count *
            (((maximum_bps / 8) / 0x800) * 0x800);
    }
    SFD_SetCond(sfd, 8, 0);
    SFD_SetCond(sfd, 1, 1);
    SFD_SetCond(sfd, 0, 0);
    SFD_SetCond(sfd, 0x17, 4);
    rounded_value = create_picture_literals.rounding_half +
        (float)(work->maximum_width * work->decoder_count * 1000);
    condition_value = (int)rounded_value;
    if ((float)condition_value > rounded_value) condition_value--;
    SFD_SetCond(sfd, 0x2D, condition_value);
    SFD_SetCond(sfd, 0x2C, condition_value);
    SFD_SetCond(sfd, 0x2A, condition_value);
    SFD_SetCond(sfd, 0xF, 2);
    SFD_SetCond(sfd, 0x33, 0);
    SFD_SetCond(sfd, 0xE, 0);
    SFD_SetCond(sfd, 0x1C, 0);
    SFD_SetMpvCond(sfd, 5, 0);

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
    player->sfx = MWSFSFX_Create(player->composition_work,
                                 player->composition_work_size,
                                 params->width, params->height);
    if (player->sfx == 0) {
        MWSFSVM_Error(create_sfx_failed);
        mwSfdDestroy(player);
        return 0;
    }
    MWSFSFX_SetCompoMode(player, player->composition_mode);
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
static const char reset_stop_failed[0x34] =
    "E0203261: MWSFCRE_ResetSfdHn: SFD_Stop() failed.";
static const char reset_error_callback_failed[0x38] =
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
    SfdHandle* sfd = player->sfd;
    MwsPictureUserConfig* config;
    void* buffer;
    int frame_count;
    int element_size;
    if (SFD_Stop(sfd) != 0) {
        MWSFSVM_Error(reset_stop_failed);
        return -1;
    }
    if (SFD_SetErrFn(sfd, MWSFLIB_SfdErrFunc,
                     (SfdCallbackObject)player) != 0) {
        MWSFLIB_SetErrCode(-0x12F);
        MWSFSVM_Error(reset_error_callback_failed);
        return -1;
    }
    config = player->picture_user;
    if (config == 0) {
        MWSFSVM_Error(picture_user_internal_error);
    } else {
        frame_count = player->frame_count;
        element_size = config->element_size;
        buffer = config->buffer;
        if (config->buffer_size < (frame_count + 3) * element_size) {
            MWSFSVM_Error(create_picture_literals.short_message);
        } else if (MWSFD_GetUsePicUsr() == 1) {
            SFD_SetPicUsrBuf(player->sfd, buffer, frame_count + 3,
                             element_size);
        }
    }
    return 0;
}

static SfdHandle* mwsfcre_CreateSfd(MwsPlayer* player,
                                    const MwsCreateParams* params)
{
    MwsReferenceBuffers references;
    MwsCreateBss* buffers = &mwsfdcre_bufnum;
    void* frame_buffers[16];
    SfdCreateConfig create;
    SfdHandle* sfd;
    void* shared_work;
    void* stream_work;
    void* audio_stream_buffer;
    void* audio_decoder_work;
    void* picture_user_work;
    void* decoder_work;
    void* video_work;
    void* filename_work;
    int decoder_count;
    int packet_count;
    int frame_size;
    int frame_result;
    int rfb_result;
    int uses_audio;
    int output_format;
    int file_type;
    int maximum_bps;
    int width;
    int height;
    int frame_count;
    int decoder_work_size;
    int video_work_size;
    int index;

    decoder_count = params->decoder_count;
    if (decoder_count <= 0) decoder_count = 1;
    file_type = params->file_type;
    maximum_bps = params->maximum_bps;
    width = params->width;
    height = params->height;
    frame_count = params->frame_count;
    decoder_work_size = 0x4000;
    video_work_size = 0x700;
    if (file_type == 2) {
        packet_count = (maximum_bps / 8) / 0x800;
        buffers->system_input_buffer_size = 0;
        buffers->video_input_buffer_size = 0;
        buffers->audio_input_buffer_size = 0;
        buffers->adx_input_buffer_size = 0;
        buffers->adx_decoder_work_size = 0;
        buffers->stream_joint_buffer_size =
            decoder_count * (packet_count * 0x800);
    } else if (file_type == 3) {
        packet_count = (maximum_bps / 8) / 0x800;
        buffers->system_input_buffer_size = 0;
        buffers->audio_input_buffer_size = 0;
        buffers->adx_input_buffer_size = 0;
        buffers->adx_decoder_work_size = 0;
        buffers->stream_joint_buffer_size =
            decoder_count * (packet_count * 0x800);
        buffers->video_input_buffer_size =
            ((packet_count * 0x800) / 2) + 0x800;
    } else {
        packet_count = (maximum_bps / 8) / 0x800;
        buffers->system_input_buffer_size = 0;
        buffers->audio_input_buffer_size = 0x5DCC;
        buffers->stream_joint_buffer_size =
            decoder_count * (packet_count * 0x800);
        buffers->video_input_buffer_size =
            ((packet_count * 0x800) / 2) + 0x800;
        buffers->adx_input_buffer_size = 0x5F0C;
        buffers->adx_decoder_work_size = 0xC1C0;
    }

    if (buffers->buffer_count != 0) {
        buffers->reference_buffer_size = 0;
        buffers->table_size = 0;
    } else {
        if (params->buffer_format < 0 || params->buffer_format >= 4)
            MWSFSVM_Error(create_buffer_format_invalid);
        frame_size = mwsfcre_CalcFrameSize(params);
        buffers->reference_buffer_size = frame_size * 2;
        buffers->table_size = params->frame_count * frame_size;
    }

    shared_work = mwsfcre_Alloc(
        player, buffers->audio_input_buffer_size +
                    buffers->video_input_buffer_size +
                    buffers->system_input_buffer_size + 0x20);
    stream_work = mwsfcre_Alloc(
        player, buffers->stream_joint_buffer_size + 0x40);
    rfb_result = mwsfcre_MallocRfb(player, params, &references);
    if (params->buffer_format < 0 || params->buffer_format >= 4) {
        MWSFSVM_Error(create_buffer_format_invalid);
    }
    frame_size = mwsfcre_CalcFrameSize(params);
    frame_result = 0;
    if (buffers->buffer_count != 0) {
        if (buffers->buffer_count < frame_count + 2 ||
            buffers->buffer_size < frame_size) {
            frame_result = -1;
        } else {
            for (index = 0; index < frame_count; index++) {
                frame_buffers[index] = buffers->buffers[index + 2];
                if (frame_buffers[index] == 0) frame_result = -1;
            }
        }
    } else {
        for (index = 0; index < frame_count; index++) {
            frame_buffers[index] = mwsfcre_Alloc(player, frame_size);
            if (frame_buffers[index] == 0) frame_result = -1;
        }
    }

    uses_audio = mwsfcre_UsesAudio(file_type);
    if (uses_audio == 1) {
        audio_stream_buffer =
            mwsfcre_Alloc(player, buffers->adx_input_buffer_size);
        audio_decoder_work =
            mwsfcre_Alloc(player, buffers->adx_decoder_work_size);
    } else {
        audio_stream_buffer = 0;
        audio_decoder_work = 0;
    }
    picture_user_work = mwsfcre_Alloc(player, 0x800);
    decoder_work = mwsfcre_Alloc(player, decoder_work_size);
    video_work = mwsfcre_Alloc(player, video_work_size);
    filename_work = mwsfcre_Alloc(player, 0x100);
    if (shared_work == 0 || stream_work == 0 || rfb_result != 0 ||
        frame_result != 0 || picture_user_work == 0 || decoder_work == 0 ||
        filename_work == 0 || video_work == 0) {
        MWSFSVM_Error(create_work_failed);
        mwsfcre_FreeAll(player);
        return 0;
    }
    if (mwsfcre_UsesAudio(file_type) == 1 &&
        (audio_stream_buffer == 0 || audio_decoder_work == 0)) {
        MWSFSVM_Error(create_audio_work_failed);
        mwsfcre_FreeAll(player);
        return 0;
    }

    buffers->stream_joint_buffer = (unsigned char*)
        (((unsigned int)stream_work + 0x3F) & ~0x3F);
    mwsfd_mpvpara[0] = (((width / 2) + 31) / 32) * 32;
    mwsfd_mpvpara[1] = height / 2;
    mwsfd_mpvpara[2] = width;
    mwsfd_mpvpara[3] = height;
    mwsfd_mpvpara[4] = 0;
    mwsfd_mpvpara[5] = width;
    mwsfd_mpvpara[6] = height;
    mwsfd_mpvpara[7] = frame_count;
    mwsfd_mpvpara[8] = 0;
    mwsfd_adxtpara.stream_buffer = audio_stream_buffer;
    mwsfd_adxtpara.decoder_buffer = audio_decoder_work;

    switch (file_type) {
    case 1:
        create = mwsfd_sfdmps_crepara;
        player->input_buffer = buffers->stream_joint_buffer;
        player->input_buffer_size = buffers->stream_joint_buffer_size -
                                    mwsfd_packsize.value;
        player->input_buffer_extra_size = mwsfd_packsize.value;
        break;
    case 2:
        create = mwsfd_sfdmpv_crepara;
        player->input_buffer = buffers->stream_joint_buffer;
        player->input_buffer_size =
            buffers->stream_joint_buffer_size - 0x800;
        player->input_buffer_extra_size = 0x800;
        break;
    case 3:
        create = mwsfd_vonlysfd_crepara;
        player->input_buffer = buffers->stream_joint_buffer;
        player->input_buffer_size = buffers->stream_joint_buffer_size -
                                    mwsfd_packsize.value;
        player->input_buffer_extra_size = mwsfd_packsize.value;
        break;
    }
    if (buffers->system_input_buffer_size != 0) {
        buffers->system_input_buffer_size -=
            buffers->system_input_buffer_size % mwsfd_packsize.value;
    }

    switch (params->buffer_format) {
    case 0: output_format = 3; break;
    case 1: output_format = 1; break;
    case 2: output_format = 2; break;
    case 3: output_format = 3; break;
    default:
        MWSFSVM_Error(create_buffer_format_invalid);
        output_format = 3;
        break;
    }
    create.buffer.memory = shared_work;
    create.buffer.buffer_sizes[0] = buffers->system_input_buffer_size;
    create.buffer.buffer_sizes[1] = buffers->video_input_buffer_size;
    create.buffer.buffer_sizes[2] = buffers->audio_input_buffer_size;
    create.picture_user_buffer_minimum = frame_count;
    create.maximum_width = width;
    create.maximum_height = height;
    create.video_output_format = output_format;
    create.handle_memory = decoder_work;
    create.handle_memory_size = 0x4000;
    SFD_SetMpvParaTbl(mwsfd_mpvpara, references.buffers,
                       frame_buffers, 0x4000,
                       buffers->video_input_buffer_size,
                       buffers->system_input_buffer_size);
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
    if (SFD_SetErrFn(sfd, MWSFLIB_SfdErrFunc,
                     (SfdCallbackObject)player) != 0) {
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
    player->internal_picture_user.buffer =
        (unsigned char*)picture_user_work + 0x40;
    player->internal_picture_user.buffer_size = 0x7C0;
    player->internal_picture_user.element_size = 0x40;
    player->picture_user = &player->internal_picture_user;
    return sfd;
}

static int mwsfcre_MallocRfb(MwsPlayer* player,
                             const MwsCreateParams* params,
                             MwsReferenceBuffers* output)
{
    MwsCreateBss* buffers = &mwsfdcre_bufnum;
    int result = 0;
    int frame_size = mwsfcre_CalcFrameSize(params);
    if (buffers->buffer_count != 0) {
        if (buffers->buffer_count < 2 || buffers->buffer_size < frame_size) {
            output->buffers[0] = 0;
            output->buffers[1] = 0;
            result = -1;
        } else {
            output->buffers[0] = buffers->buffers[0];
            output->buffers[1] = buffers->buffers[1];
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
        supply.field_00 = 1;
        supply.stream_joint = stream;
        supply.buffer = player->memory_buffer;
        supply.buffer_size = player->memory_buffer_size;
        supply.field_10 = 0;
        supply.field_14 = 0;
    } else if (stream == player->input_sj) {
        supply.field_00 = 0;
        supply.stream_joint = stream;
        supply.buffer = player->input_buffer;
        supply.buffer_size = player->input_buffer_size;
        supply.field_10 = player->input_buffer_extra_size;
        supply.field_14 = 0;
    } else {
        supply.field_00 = player->supply_mode;
        supply.stream_joint = stream;
        supply.buffer = player->supply_buffer;
        supply.buffer_size = player->supply_buffer_size;
        supply.field_10 = player->supply_buffer_extra_size;
        supply.field_14 = 0;
    }
    if (SFD_SetSupplySj(sfd, &supply) != 0) {
        MWSFLIB_SetErrCode(-0x138);
        MWSFSVM_Error(supply_sj_failed);
    }
}
