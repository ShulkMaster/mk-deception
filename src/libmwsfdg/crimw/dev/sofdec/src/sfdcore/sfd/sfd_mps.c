#include "cri/mps.h"
#include "sofdec/sfd_error.h"
#include "sofdec/sfd_player.h"
#include "sofdec/sfd_transport.h"
#include "sofdec/uty_mem.h"

typedef void (*SfmpsElementCallback)(void* argument, int element_id);

typedef struct SfmpsWork {
    MpsHandle* decoder;
    int max_video_bound;
    int max_audio_bound;
    int field_00C;
    long long video_pts_baseline;
    long long audio_pts_baseline;
    int concat_count;
    int previous_video_stream;
    int previous_audio_stream;
    int first_video_stream;
    int first_audio_stream;
    int selected_video_id;
    int selected_audio_id;
    int delimiter_state;
    SJ* element_outputs[0x44];
    SfmpsElementCallback element_callback;
    void* element_callback_argument;
    int scan_position;
    unsigned char field_15C[0x0C];
} SfmpsWork;

typedef struct SfmpsSavedSystemHeaders {
    unsigned char data[2][0xB0];
    int length[2];
} SfmpsSavedSystemHeaders;

typedef struct SfmpsSeekSnapshot {
    int active;
    int mux_rate;
    int rate_bound;
    int video_bound;
    int audio_bound;
    int field_014;
    long long scr_base;
    long long video_pts_baseline;
    int first_video_stream;
    int first_audio_stream;
    SfmpsSavedSystemHeaders system_headers;
} SfmpsSeekSnapshot;

typedef char SfmpsWorkSizeCheck[sizeof(SfmpsWork) == 0x168 ? 1 : -1];
typedef char SfmpsSeekSnapshotSizeCheck[
    sizeof(SfmpsSeekSnapshot) == 0x198 ? 1 : -1];
typedef char SfmpsSavedSystemHeadersSizeCheck[
    sizeof(SfmpsSavedSystemHeaders) == 0x168 ? 1 : -1];

typedef struct SfmpsMpsLibWork {
    MpsErrorCallback error_callback;
    MpsCallbackObject error_object;
    int error;
    int handle_count;
    MpsHandle handles[8];
} SfmpsMpsLibWork;

typedef char SfmpsMpsLibWorkSizeCheck[
    sizeof(SfmpsMpsLibWork) == 0x810 ? 1 : -1];

int copy_sj_error;
static SfmpsMpsLibWork sfmps_libwork;

extern void SFHDS_ReprocessHdr(SfdHandle* handle);
static int sfmps_ExecServerSub(SfdHandle* handle);

static SfmpsWork* sfmps_GetWork(SfdHandle* handle)
{
    return (SfmpsWork*)handle->transports[1].context;
}

int SFMPS_GetConcatCnt(SfdHandle* handle)
{
    return sfmps_GetWork(handle)->concat_count;
}

static inline SfmpsSeekSnapshot* sfmps_GetSeekSnapshot(SfdHandle* handle)
{
    SfdSeeWork* source = handle->seek_state.work;

    if (source == 0) {
        return 0;
    }
    if (sfmps_GetWork(handle)->concat_count > 0) {
        return 0;
    }
    return (SfmpsSeekSnapshot*)((unsigned char*)source + 0x8A0);
}

/* TODO: [near miss] 99.133330%; typed saved headers and separate guards
 * match retail CFG and stores; stop at equivalent work/decoder GPR coloring. */
static int SFMPS_Seek(SfdHandle* handle, int parameter, int value)
{
    SfmpsSeekSnapshot* snapshot = sfmps_GetSeekSnapshot(handle);
    SfmpsSavedSystemHeaders* saved;
    MpsHandle* decoder;
    SfmpsWork* work;
    int consumed;
    int header_flags;
    int first_result;
    int second_result;
    int result;

    if (snapshot == 0) {
        return 0;
    }
    if (snapshot->active == 0) {
        return 0;
    }

    work = sfmps_GetWork(handle);
    SFHDS_ReprocessHdr(handle);
    saved = &snapshot->system_headers;
    decoder = work->decoder;
    first_result = MPS_DecHd(decoder, saved->data[0],
                             saved->length[0], &consumed,
                             &header_flags);
    second_result = MPS_DecHd(decoder, saved->data[1],
                              saved->length[1], &consumed,
                              &header_flags);
    if (first_result != 0 || second_result != 0) {
        result = SFLIB_SetErr(handle, 0xFF000D0D);
    } else {
        result = 0;
    }
    if (result != 0) {
        return result;
    }

    work->first_video_stream = snapshot->first_video_stream;
    work->first_audio_stream = snapshot->first_audio_stream;
    handle->timer_state.field_0150 = snapshot->scr_base;
    work->video_pts_baseline = snapshot->video_pts_baseline;
    return 0;
}

static int SFMPS_AddRead(SfdHandle* handle, int parameter, int value)
{
    return SFLIB_SetErr(handle, 0xFF000D0B);
}

static int SFMPS_GetRead(SfdHandle* handle, void* buffer)
{
    return SFLIB_SetErr(handle, 0xFF000D0B);
}

static int SFMPS_AddWrite(SfdHandle* handle, int parameter, int value)
{
    return SFLIB_SetErr(handle, 0xFF000D0B);
}

static int SFMPS_GetWrite(SfdHandle* handle, void* buffer)
{
    return SFLIB_SetErr(handle, 0xFF000D0B);
}

static int SFMPS_Pause(SfdHandle* handle, int state)
{
    (void)state;
    return 0;
}

static int SFMPS_Stop(SfdHandle* handle)
{
    return 0;
}

static int SFMPS_Start(SfdHandle* handle)
{
    return 0;
}

static int SFMPS_Standby(SfdHandle* handle)
{
    return 0;
}

static int SFMPS_Destroy(SfdHandle* handle)
{
    if (MPS_Destroy(sfmps_GetWork(handle)->decoder) != 0) {
        return SFLIB_SetErr(handle, 0xFF000D0A);
    }
    return 0;
}

static void sfmps_ErrFn(MpsCallbackObject object, int error)
{
    SFLIB_SetErr((SfdHandle*)object, error);
}

static void sfmps_ClrOutSj(SfmpsWork* work)
{
    int i;

    for (i = 0; i < 0x44; i++) {
        work->element_outputs[i] = 0;
    }
}

static int SFMPS_Create(SfdHandle* handle)
{
    SfmpsWork* work = (SfmpsWork*)handle->mps_work_storage;
    MpsHandle* decoder;
    handle->transports[1].context = work;
    work->decoder = 0;
    work->max_video_bound = 0;
    work->max_audio_bound = 0;
    work->video_pts_baseline = 0x7FFFFFFFFFFFFFFFLL;
    work->audio_pts_baseline = 0x7FFFFFFFFFFFFFFFLL;
    work->concat_count = 0;
    work->previous_video_stream = 0x7FFFFFFF;
    work->previous_audio_stream = 0x7FFFFFFF;
    work->first_video_stream = -1;
    work->first_audio_stream = -1;
    work->selected_video_id = -1;
    work->selected_audio_id = -1;
    work->delimiter_state = 0;
    sfmps_ClrOutSj(work);
    work->element_callback = 0;
    work->element_callback_argument = 0;
    work->scan_position = -1;

    decoder = MPS_Create();
    if (decoder == 0) {
        return SFLIB_SetErr(0, 0xFF000D08);
    }
    if (MPS_SetErrFn(decoder, sfmps_ErrFn, handle) != 0) {
        MPS_Destroy(decoder);
        return SFLIB_SetErr(0, 0xFF000D09);
    }
    work->decoder = decoder;
    return 0;
}

static inline void sfmps_UpdateStreamBounds(SfmpsWork* work)
{
    MpsHandle* decoder = work->decoder;
    MpsSystemHeader header;
    int max_audio = 0;
    int max_video = 0;
    int index;

    for (index = 0; index < 3; index++) {
        int audio_bound;
        int video_bound;

        MPS_GetSysHd(decoder, &header, index);
        audio_bound = header.audio_bound;
        if (max_audio > audio_bound) {
            audio_bound = max_audio;
        }
        max_audio = audio_bound;
        video_bound = header.video_bound;
        if (max_video > video_bound) {
            video_bound = max_video;
        }
        max_video = video_bound;
    }
    work->max_audio_bound = max_audio;
    work->max_video_bound = max_video;
}

static inline void sfmps_ProcPrepSeek(SfdHandle* handle)
{
    SfmpsSeekSnapshot* snapshot = sfmps_GetSeekSnapshot(handle);
    SfmpsWork* work;

    if (snapshot == 0) {
        return;
    }

    work = sfmps_GetWork(handle);
    if (work->audio_pts_baseline == 0x7FFFFFFFFFFFFFFFLL) {
        return;
    }
    handle->timer_state.audio_start_pts =
        work->audio_pts_baseline - snapshot->video_pts_baseline;
    if (snapshot->active == 0) {
        snapshot->mux_rate = handle->playback_settings.values_18[0] * 50;
        snapshot->rate_bound = handle->playback_settings.values_18[1];
        snapshot->video_bound = work->max_video_bound;
        snapshot->audio_bound = work->max_audio_bound;
        snapshot->scr_base = handle->timer_state.field_0150;
        snapshot->video_pts_baseline = work->video_pts_baseline;
        snapshot->first_video_stream = work->first_video_stream;
        snapshot->first_audio_stream = work->first_audio_stream;
    }
}

/* TODO: [near miss] 98.538120%; direct typed buffer access shifts the
 * prep-size branch and regresses; retained typed pointer has one extra addi. */
static void sfmps_ProcPrep(SfdHandle* handle)
{
    SfmpsWork* work = sfmps_GetWork(handle);
    MpsHandle* decoder;
    MpsSystemHeader system;
    MpsPackHeader pack;
    SfdBufferState* input_buffer;
    int size;
    int need;
    int prep_output2;
    int prep_output;
    int prep_user;

    sfmps_UpdateStreamBounds(work);

    prep_output2 = SFBUF_GetPrepFlg(handle,
                                    handle->transports[1].parameter_18);
    prep_output = SFBUF_GetPrepFlg(handle,
                                   handle->transports[1].parameter_14);
    prep_user = SFBUF_GetPrepFlg(handle,
                                 handle->transports[1].buffer_output3);
    if ((prep_output2 | prep_output | prep_user) != 1 &&
        SFBUF_GetPrepFlg(handle, handle->transports[1].parameter_10) == 1) {
        size = handle->create_config.buffer.buffer_sizes[0];
        need = handle->conditions_primary[22];
        input_buffer = &handle->buffers[handle->transports[1].parameter_10];
        if (size <= 0) {
            size = input_buffer->work.ring.supply.buffer_size;
        }
        if (size <= 0) {
            size = need;
        }
        if (size < need) {
            need = size;
        }
        if (SFBUF_GetWTot(handle, 0) >= need) {
            SFBUF_SetPrepFlg(handle, handle->transports[1].parameter_18, 1);
            SFBUF_SetPrepFlg(handle, handle->transports[1].parameter_14, 1);
            SFBUF_SetPrepFlg(handle, handle->transports[1].buffer_output3, 1);
        }
    }

    work = sfmps_GetWork(handle);
    decoder = work->decoder;
    MPS_GetPackHd(decoder, &pack);
    if (pack.mux_rate != -1 && pack.mux_rate > 0) {
        handle->playback_settings.values_18[0] = pack.mux_rate;
    }
    MPS_GetSysHd(decoder, &system, 1);
    if (system.fixed_flag != -1) {
        handle->playback_settings.values_18[1] = system.fixed_flag;
    }
    if (handle->playback_settings.values_18[3] == -1) {
        handle->playback_settings.values_18[3] = work->max_audio_bound;
    }
    if (handle->playback_settings.values_18[4] == -1) {
        handle->playback_settings.values_18[4] = work->max_video_bound;
    }

    work = sfmps_GetWork(handle);
    if (SFSET_GetCond(handle, 6) != 0 &&
        SFSET_GetCond(handle, 0x50) != 0 &&
        SFBUF_GetWTot(handle, 2) == 0 && work->max_audio_bound == 0 &&
        SFTRN_GetPrepFlg(handle, 6) != 0) {
        SFSET_SetCond(handle, 6, 0);
    }
    if (SFSET_GetCond(handle, 5) != 0 &&
        SFSET_GetCond(handle, 0x4F) != 0 &&
        SFBUF_GetWTot(handle, 1) == 0 && work->max_video_bound == 0 &&
        SFTRN_GetPrepFlg(handle, 7) != 0) {
        SFSET_SetCond(handle, 5, 0);
    }

    sfmps_ProcPrepSeek(handle);
}

static int sfmps_CopyDstBuft(SfdHandle* handle, int buffer_index,
                              const unsigned char* data, int size,
                              long long pts)
{
    SfdBufferTransfer transfer;
    unsigned char* first_data;
    int first_length;
    unsigned char* second_data;
    int reserve;
    int result;

    result = SFBUF_RingGetWrite(handle, buffer_index, &transfer);
    if (result != 0) {
        return result;
    }
    first_data = transfer.chunks[0].data;
    first_length = transfer.chunks[0].len;
    second_data = transfer.chunks[1].data;
    reserve = transfer.reserved[1];
    if (size > first_length + transfer.chunks[1].len) {
        return 0;
    }
    if (buffer_index == 1) {
        if (pts >= 0) {
            SfdPtsEntry entry;
            int full;

            if (SFPTS_IsPtsQueFull(handle, buffer_index) != 0) {
                return 0;
            }
            entry.pts = pts;
            entry.data = first_data;
            entry.size = size;
            result = SFPTS_WritePtsQue(handle, buffer_index, &entry, &full);
            if (result != 0) {
                return result;
            }
        }
    } else if (buffer_index == 2 && SFPLY_SetPtsInfo != 0) {
        SfdPlayerPtsInfo info;

        info.pts = pts;
        info.size = size;
        if (SFPLY_SetPtsInfo(&handle->timer_state.video_pts, &info) == -1) {
            return 0;
        }
    }
    if (size <= first_length) {
        MEM_Copy(first_data, data, size);
    } else {
        MEM_Copy(first_data, data, first_length);
        MEM_Copy(second_data, data + first_length, size - first_length);
    }
    result = SFBUF_RingAddWrite(handle, buffer_index, size, reserve);
    if (result != 0) {
        return result;
    }
    return 1;
}

static int sfmps_CopyPadding(SfdHandle* handle, int stream_index,
                             const unsigned char* data, int size,
                             long long pts)
{
    return 1;
}

static inline int sfmps_CopyToSj(SJ* sj, const unsigned char* data, int size)
{
    SJCK second_chunk;
    SJCK first_chunk;

    if (sj->interface->get_num_data(sj, 0) < size) {
        return 0;
    }
    sj->interface->get_chunk(sj, 0, size, &first_chunk);
    MEM_Copy(first_chunk.data, data, first_chunk.len);
    sj->interface->put_chunk(sj, 1, &first_chunk);
    if (first_chunk.len == 0) {
        return 0;
    }

    size -= first_chunk.len;
    data += first_chunk.len;
    if (size > 0) {
        sj->interface->get_chunk(sj, 0, size, &second_chunk);
        MEM_Copy(second_chunk.data, data, second_chunk.len);
        sj->interface->put_chunk(sj, 1, &second_chunk);
        if (second_chunk.len != size) {
            copy_sj_error++;
        }
    }
    return 1;
}

static int sfmps_CopyUoch(SfdHandle* handle, int stream_index,
                          const unsigned char* data, int size)
{
    SfdBufferChannel channel;
    SfdBufferHandleCallback handle_callback;
    SfdBufferObjectCallback object_callback;
    SfdCallbackObject object;
    int result;

    SFBUF_GetUoch(handle, handle->transports[1].buffer_output3,
                  stream_index, &channel);
    handle_callback = channel.handle_callback;
    object_callback = channel.object_callback;
    object = channel.object;
    if (channel.stream_joint == 0) {
        return 1;
    }
    result = sfmps_CopyToSj(channel.stream_joint, data, size);
    if (result == 1) {
        if (handle_callback != 0) {
            handle_callback(handle, stream_index);
        }
        if (object_callback != 0) {
            object_callback(object, stream_index);
        }
    }
    return result;
}

static int sfmps_CopyUo(SfdHandle* handle, int stream_index,
                        const unsigned char* data, int size)
{
    if (handle->transports[1].buffer_output3 == 8) {
        return 1;
    }
    return sfmps_CopyUoch(handle, stream_index, data, size);
}

/* TODO: [near miss] 98.958530%; donor channel helper and two-chunk copy
 * match the retail CFG and stack slots; stop at equivalent GPR coloring. */
static int sfmps_CopyPrvate(SfdHandle* handle, int stream_index,
                            const unsigned char* data, int size,
                            long long pts)
{
    int header_flag;

    if (SFHDS_SetHdr(handle, stream_index, data, size, &header_flag) != 0) {
        if (header_flag != 0) {
            data -= 0x12;
            size += 0x12;
            sfmps_CopyUo(handle, 0, data, size);
        }
        return 1;
    }
    return sfmps_CopyUo(handle, stream_index, data, size);
}

/* TODO: [near miss] 99.689650%; donor signed-byte boundary restores the
 * start-code mask; stop at equivalent bound-loop register coloring. */
static int sfmps_CopyVideo(SfdHandle* handle, int stream_index,
                            const unsigned char* data, int size,
                            long long pts)
{
    SfmpsWork* work;
    SfmpsWork* selected_work;
    int desired_stream;
    int candidate;

    if (SFSET_GetCond(handle, 5) == 0) {
        return 1;
    }
    work = sfmps_GetWork(handle);
    if (work->selected_video_id == -1) {
        switch (SFSET_GetCond(handle, 0x3B)) {
        case 1:
            candidate = stream_index;
            break;
        case 2: {
            selected_work = sfmps_GetWork(handle);
            sfmps_UpdateStreamBounds(selected_work);
            if (selected_work->max_video_bound >= 2) {
                candidate = 2;
            } else {
                candidate = stream_index;
            }
            break;
        }
        case 0:
        default:
            candidate = stream_index;
            break;
        }
        work->selected_video_id = candidate;
    }
    if (work->first_video_stream == -1) {
        work->first_video_stream = stream_index;
    }
    desired_stream = SFSET_GetCond(handle, 0x1D);
    if (desired_stream != -1) {
        if (SFSET_GetCond(handle, 0x37) != 0) {
            candidate = stream_index < work->previous_video_stream;
        } else {
            candidate = stream_index == work->first_video_stream;
        }
        if (candidate && work->selected_video_id != desired_stream) {
            int is_boundary;

            if (size < 4) {
                is_boundary = 0;
            } else if (data[0] != 0) {
                is_boundary = 0;
            } else if (data[1] != 0) {
                is_boundary = 0;
            } else if (data[2] != 1) {
                is_boundary = 0;
            } else {
                /* CRI narrows the signed scan byte before the final test. */
                unsigned char code = ((const signed char*)data)[3];

                if (code == 0xB3) {
                    is_boundary = 1;
                } else {
                    is_boundary = code == 0xB8;
                }
            }
            if (is_boundary) {
                work->selected_video_id = desired_stream;
            }
        }
    }
    work->previous_video_stream = stream_index;
    if (work->selected_video_id != stream_index) {
        return 1;
    }
    return sfmps_CopyDstBuft(handle,
                             handle->transports[1].parameter_14,
                             data, size, pts);
}

static int sfmps_CopyAudio(SfdHandle* handle, int stream_index,
                            const unsigned char* data, int size,
                            long long pts)
{
    SfmpsWork* work;
    int desired_stream;
    int candidate;
    long long minimum;

    if (SFSET_GetCond(handle, 6) == 0) {
        return 1;
    }
    work = sfmps_GetWork(handle);
    if (work->selected_audio_id == -1) {
        work->selected_audio_id = stream_index;
    }
    if (work->first_audio_stream == -1) {
        work->first_audio_stream = stream_index;
    }
    desired_stream = SFSET_GetCond(handle, 0x1E);
    if (desired_stream != -1) {
        if (SFSET_GetCond(handle, 0x37) != 0) {
            candidate = stream_index < work->previous_audio_stream;
        } else {
            candidate = stream_index == work->first_audio_stream;
        }
        if (candidate) {
            work->selected_audio_id = desired_stream;
        }
    }
    work->previous_audio_stream = stream_index;
    if (work->selected_audio_id != stream_index) {
        return 1;
    }
    if (pts >= 0) {
        minimum = work->video_pts_baseline;
        if (pts < minimum) {
            minimum = pts;
        }
        work->video_pts_baseline = minimum;
        minimum = work->audio_pts_baseline;
        if (pts < minimum) {
            minimum = pts;
        }
        work->audio_pts_baseline = minimum;
    }
    return sfmps_CopyDstBuft(handle,
                             handle->transports[1].parameter_18,
                             data, size, pts);
}

typedef int (*SfmpsCopyPacketFn)(SfdHandle* handle, int stream_index,
                                 const unsigned char* data, int size,
                                 long long pts);

static SfmpsCopyPacketFn const sfmps_CopyPketFn[4] = {
    sfmps_CopyAudio,
    sfmps_CopyVideo,
    sfmps_CopyPrvate,
    sfmps_CopyPadding,
};

/* TODO: [near miss] 99.137930%; donor header/callback snapshots and result
 * switch match retail operations and CFG; stop at equivalent GPR coloring. */
static int sfmps_CopyPketData(SfdHandle* handle, const unsigned char* data,
                              int available, int* consumed, int* copied)
{
    SfmpsWork* work;
    MpsPacketHeader header;
    SJ* output;
    SfmpsElementCallback callback;
    void* callback_argument;
    int stream_id;
    int stream_type;
    int stream_index;
    int payload_length;
    long long pts;
    int result;

    result = 0;
    *consumed = 0;
    *copied = 0;
    work = sfmps_GetWork(handle);
    if (MPS_GetPketHd(work->decoder, &header) != 0) {
        result = SFLIB_SetErr(handle, 0xFF000D06);
    }
    payload_length = header.payload_length;
    stream_id = header.stream_id;
    stream_type = header.stream_type;
    stream_index = header.stream_index;
    pts = header.pts;
    if (payload_length < 0) {
        return SFLIB_SetErr(handle, 0xFF000D0E);
    }
    if (payload_length == 0) {
        *consumed = 0;
        *copied = 1;
        return 0;
    }
    if (available < payload_length) {
        if (SFBUF_GetTermFlg(handle,
                             handle->transports[1].parameter_10) == 1) {
            SFBUF_SetTermFlg(handle, handle->transports[1].parameter_18, 1);
            SFBUF_SetTermFlg(handle, handle->transports[1].parameter_14, 1);
            SFBUF_SetTermFlg(handle, handle->transports[1].buffer_output3, 1);
        }
        return 0;
    }

    output = work->element_outputs[stream_id - 0xBC];
    if (output != 0) {
        int copy_result;

        callback_argument = work->element_callback_argument;
        callback = work->element_callback;
        copy_result = sfmps_CopyToSj(output, data, payload_length);

        if (copy_result == 1 && callback != 0) {
            callback(callback_argument, stream_id);
        }
        *copied = copy_result;
    } else {
        *copied = sfmps_CopyPketFn[stream_type](
            handle, stream_index, data, payload_length, pts);
    }
    switch (*copied) {
    case 1:
        *consumed = payload_length;
        break;
    case 0:
        break;
    default:
        result = *copied;
        break;
    }
    return result;
}

static inline void sfmps_SetOutputTerminated(SfdHandle* handle)
{
    SFBUF_SetTermFlg(handle, handle->transports[1].parameter_18, 1);
    SFBUF_SetTermFlg(handle, handle->transports[1].parameter_14, 1);
    SFBUF_SetTermFlg(handle, handle->transports[1].buffer_output3, 1);
}

static inline void sfmps_TermIfInputTerminated(SfdHandle* handle, int* term)
{
    int value;

    if (SFBUF_GetTermFlg(handle,
                         handle->transports[1].parameter_10) == 1) {
        sfmps_SetOutputTerminated(handle);
        value = 1;
    } else {
        value = 0;
    }
    if (term != 0) {
        *term = value;
    }
}

static inline int sfmps_IsInputTerminated(SfdHandle* handle)
{
    int term;

    sfmps_TermIfInputTerminated(handle, &term);
    return term;
}

static inline int sfmps_IsZero(const signed char* bytes, int count)
{
    int index;

    for (index = 0; index < count; index++) {
        if (*bytes++ != 0) {
            return 0;
        }
    }
    return 1;
}

static inline void sfmps_SetPesFns(SfdHandle* handle, MpsHandle* decoder)
{
    MpsCallbackObject object;
    MpsPesCallback callback;

    object = (MpsCallbackObject)SFSET_GetCond(handle, 0x5C);
    callback = (MpsPesCallback)SFSET_GetCond(handle, 0x5B);
    MPS_SetPesFn(decoder, callback, object);
}

static inline void sfmps_SaveSystemHeader(SfmpsSeekSnapshot* snapshot,
                                          MpsHandle* decoder,
                                          const unsigned char* data, int size)
{
    MpsSystemHeader system;
    SfmpsSavedSystemHeaders* saved = &snapshot->system_headers;
    unsigned char* destination;
    int copy_size;

    MPS_GetLastSysHd(decoder, &system);
    destination = saved->data[0];
    copy_size = size < 0xB0 ? size : 0xB0;
    if (system.video_bound > 0) {
        saved->length[0] = copy_size;
    } else if (system.audio_bound > 0) {
        saved->length[1] = copy_size;
        destination += sizeof(saved->data[0]);
    } else {
        return;
    }
    MEM_Copy(destination, data, copy_size);
}

static inline void sfmps_ScanSkip(SfdHandle* handle,
                                  const unsigned char* data, int size,
                                  int* copied)
{
    const unsigned char* cursor = data;
    int amount = 0;

    while (size >= 4) {
        if ((MPS_CheckDelim(cursor) & 0xD0000) != 0) {
            *copied = amount;
            return;
        }
        amount++;
        cursor++;
        size--;
    }
    if (size > 0 && size < 4) {
        SfdBufferState* input =
            &handle->buffers[handle->transports[1].parameter_10];
        SfdBufferRingWork* ring = &input->work.ring;
        int at_end;

        if (input->terminated == 0 &&
            (ring->supply.buffer_size != 0 ||
             ring->supply.extra_size != 0)) {
            at_end = 0;
        } else if (cursor + size ==
                   ring->supply.buffer + ring->supply.buffer_size) {
            at_end = 1;
        } else {
            at_end = 0;
        }
        if (at_end) {
            amount += size;
        }
    }
    *copied = amount;
}

/* TODO: [near miss] 98.723970%; header stack slots and +0x28 scan unit now
 * match retail; scan pointer materialization and register coloring remain. */
static int sfmps_DecodeOneUnit(SfdHandle* handle, const unsigned char* data,
                               int size, int* consumed, int* copied,
                               int readable)
{
    SfmpsWork* work;
    MpsHandle* decoder;
    int buffer_index;
    int delimiter;
    int header_flags;
    int decoded_size;
    int result = 0;
    int proceed;

    *copied = *consumed = delimiter = 0;
    work = sfmps_GetWork(handle);
    buffer_index = handle->transports[1].parameter_10;
    decoder = work->decoder;
    if (size >= 4) {
        delimiter = MPS_CheckDelim(data);
        if (delimiter == 0x80000) {
            if (handle->transports[1].state < 0) {
                handle->transports[1].state =
                    SFBUF_GetRTot(handle, buffer_index) + 4;
            }
            work->delimiter_state = 1;
        } else if (delimiter != 0) {
            work->delimiter_state = 0;
        }
    }

    if (delimiter != 0x80000) {
        proceed = 0;
    } else if (SFCON_IsEndcodeSkip(handle) != 0 ||
               SFCON_IsSystemEndcodeSkip(handle) != 0) {
        proceed = 0;
    } else {
        proceed = 1;
    }
    if (proceed != 0) {
        sfmps_SetOutputTerminated(handle);
        proceed = 0;
    } else if (readable < 4 && sfmps_IsInputTerminated(handle) != 0) {
        proceed = 0;
    } else if (size < 0x40) {
        if (delimiter == 0x10000 || delimiter == 0x40000) {
            sfmps_TermIfInputTerminated(handle, 0);
            proceed = 0;
        } else {
            proceed = 1;
        }
    } else {
        proceed = 1;
    }
    if (proceed == 0) {
        return 0;
    }

    delimiter = size >= 4 ? MPS_CheckDelim(data) : 0;
    MPS_SetPsMapFn(decoder,
                   (MpsPsMapCallback)SFSET_GetCond(handle, 0x57),
                   (MpsCallbackObject)SFSET_GetCond(handle, 0x58));
    sfmps_SetPesFns(handle, decoder);
    if (MPS_DecHd(decoder, data, size, &decoded_size, &header_flags) != 0) {
        result = SFLIB_SetErr(handle, 0xFF000D03);
    }

    if ((header_flags & 0x20000) != 0) {
        SfmpsSeekSnapshot* snapshot = sfmps_GetSeekSnapshot(handle);

        if (snapshot != 0 && snapshot->active == 0) {
            sfmps_SaveSystemHeader(snapshot, decoder, data, size);
        }
    }

    if (header_flags == 0x80000 && SFCON_IsEndcodeSkip(handle) != 0) {
        sfmps_GetWork(handle)->concat_count++;
        *consumed = 4;
        work->scan_position = 4;
    } else if (header_flags == 0x80000 &&
               SFCON_IsSystemEndcodeSkip(handle) != 0) {
        *consumed = 4;
        work->scan_position = 4;
    } else if (delimiter == 0) {
        *copied = 0;
        if (size >= handle->create_config.buffer.ring_alignment + 3 &&
            sfmps_IsZero((const signed char*)data,
                         handle->create_config.buffer.ring_alignment)) {
            *copied = handle->create_config.buffer.ring_alignment;
        } else {
            sfmps_ScanSkip(handle, data, size, copied);
        }
        *consumed = *copied;
        if (*copied > 0 && work->scan_position >= 0) {
            if (work->scan_position >= handle->create_config.buffer.ring_alignment) {
                work->scan_position += *copied;
            } else if (work->scan_position + *copied >
                       handle->create_config.buffer.ring_alignment) {
                *copied -= handle->create_config.buffer.ring_alignment -
                           work->scan_position;
                work->scan_position =
                    handle->create_config.buffer.ring_alignment + *copied;
            } else {
                work->scan_position += *copied;
                *copied = 0;
            }
        }
    } else if ((header_flags & 0x40000) == 0) {
        int input_terminated;

        sfmps_TermIfInputTerminated(handle, &input_terminated);
        if (input_terminated == 0 &&
            size > handle->create_config.buffer.ring_alignment) {
            if (decoded_size > 0) {
                *consumed = decoded_size;
                *copied = decoded_size;
            } else {
                *consumed = 1;
                *copied = 1;
            }
        }
    } else {
        int packet_consumed;
        int packet_copied;

        data += decoded_size;
        size -= decoded_size;
        result = sfmps_CopyPketData(handle, data, size, &packet_consumed,
                                    &packet_copied);
        if (packet_copied == 1) {
            *consumed = decoded_size + packet_consumed;
        }
        work->scan_position = -1;
    }
    return result;
}

static inline int sfmps_GetRead(SfdHandle* handle, unsigned char** data,
                                int* size, int* readable)
{
    SfdBufferTransfer transfer;
    int result;

    result = SFBUF_RingGetRead(handle, handle->transports[1].parameter_10,
                               &transfer);
    if (result != 0) {
        return result;
    }
    *size = transfer.chunks[0].len;
    *data = transfer.chunks[0].data;
    *readable = *size + transfer.chunks[1].len;
    return 0;
}

static inline int sfmps_AddRead(SfdHandle* handle, int size)
{
    int read_result;
    int result;

    read_result = SFBUF_RingAddRead(handle,
                                    handle->transports[1].parameter_10, size);
    result = 0;
    if (read_result != 0) {
        result = read_result;
    }
    return result;
}

static inline int sfmps_ExecServerLoop(SfdHandle* handle, int* input_size)
{
    int result = 0;
    int consumed_total = 0;
    int copied_total = 0;
    int write_flow;
    int read_flow;
    unsigned char* input_data;
    int readable;
    int consumed;
    int copied;

    while (consumed_total < 0x7FFFFFFF) {
        result = sfmps_GetRead(handle, &input_data, input_size, &readable);
        if (result != 0) {
            break;
        }
        result = sfmps_DecodeOneUnit(handle, input_data, *input_size,
                                     &consumed, &copied, readable);
        if (result != 0 || consumed == 0) {
            break;
        }
        result = sfmps_AddRead(handle, consumed);
        if (result != 0) {
            break;
        }
        copied_total += copied;
        consumed_total += consumed;
    }

    SFBUF_GetFlowCnt(
        handle->buffers[0].work.ring.supply.stream_joint,
        &write_flow, &read_flow);
    handle->playback_runtime.time_values[0] =
        SFBUF_UpdateFlowCnt(handle->playback_runtime.time_values[0], write_flow);
    handle->playback_runtime.time_values[1] += consumed_total;
    handle->playback_runtime.time_values[2] += copied_total;
    if (handle->playback_state == 2) {
        sfmps_ProcPrep(handle);
    }
    return result;
}

/* TODO: [near miss] 99.401710%; donor-backed inline loop and caller-owned
 * input size restore retail structure; inspect final register-color residue. */
static int sfmps_ExecServerSub(SfdHandle* handle)
{
    SfmpsWork* work;
    MpsHandle* decoder;
    int input_size;
    int term1;
    int term2;
    int term3;

    term1 = SFBUF_GetTermFlg(handle, handle->transports[1].parameter_18);
    term2 = SFBUF_GetTermFlg(handle, handle->transports[1].parameter_14);
    term3 = SFBUF_GetTermFlg(handle, handle->transports[1].buffer_output3);
    if ((term1 & term2 & term3) == 1) {
        return 0;
    }
    work = sfmps_GetWork(handle);
    decoder = work->decoder;
    MPS_SetSystemFn(decoder,
                    (MpsSystemCallback)SFSET_GetCond(handle, 0x55),
                    (MpsCallbackObject)SFSET_GetCond(handle, 0x56));
    return sfmps_ExecServerLoop(handle, &input_size);
}

/* Retail keeps this dispatch wrapper out of line after the read helper folds. */
#pragma dont_inline on
static int SFMPS_ExecServer(SfdHandle* handle)
{
    return sfmps_ExecServerSub(handle);
}
#pragma dont_inline off

static int SFMPS_Finish(SfdHandle* handle)
{
    MPS_Finish();
    return 0;
}

#pragma optimization_level 1
static int SFMPS_Init(SfdHandle* handle)
{
    int result;
    int work_size = sizeof(SfmpsWork);

    if (work_size > 0x200) {
        result = SFLIB_SetErr(0, 0xFF000D0C);
    } else {
        result = 0;
    }
    if (result != 0) {
        for (;;) {
        }
    }
    if (MPS_Init(8, (MpsLibWork*)&sfmps_libwork) != 0) {
        return SFLIB_SetErr(0, 0xFF000D01);
    }
    copy_sj_error = 0;
    return 0;
}
#pragma optimization_level 4

int SFD_SetElementOutSj(SfdHandle* handle, int element_id, SJ* stream,
                        SfmpsElementCallback callback,
                        void* callback_argument)
{
    SfmpsWork* work;

    if (SFLIB_CheckHn(handle) != 0) {
        return SFLIB_SetErr(0, 0xFF000171);
    }
    if (element_id < 0xBC || element_id > 0xFF) {
        return 0;
    }
    work = sfmps_GetWork(handle);
    work->element_callback = callback;
    work->element_callback_argument = callback_argument;
    work->element_outputs[element_id - 0xBC] = stream;
    return 0;
}

const SfdTransportInterface SFD_tr_sd_mps = {
    SFMPS_Init,
    SFMPS_Finish,
    SFMPS_ExecServer,
    SFMPS_Create,
    SFMPS_Destroy,
    SFMPS_Standby,
    SFMPS_Start,
    SFMPS_Stop,
    SFMPS_Pause,
    SFMPS_GetWrite,
    SFMPS_AddWrite,
    SFMPS_GetRead,
    SFMPS_AddRead,
    SFMPS_Seek,
};
