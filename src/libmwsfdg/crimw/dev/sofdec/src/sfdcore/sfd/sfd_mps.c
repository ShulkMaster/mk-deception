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

typedef struct SfmpsSeekSnapshot {
    int active;
    int mux_rate;
    int rate_bound;
    int video_bound;
    int audio_bound;
    int field_014;
    int timer_value;
    int playback_value;
    long long video_pts_baseline;
    int first_video_stream;
    int first_audio_stream;
    unsigned char header0[0xB0];
    unsigned char header1[0xB0];
    int header0_size;
    int header1_size;
} SfmpsSeekSnapshot;

typedef char SfmpsWorkSizeCheck[sizeof(SfmpsWork) == 0x168 ? 1 : -1];
typedef char SfmpsSeekSnapshotSizeCheck[
    sizeof(SfmpsSeekSnapshot) == 0x198 ? 1 : -1];

typedef struct SfmpsMpsLibWork {
    MpsErrorCallback error_callback;
    int error_object;
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

static int SFMPS_Seek(SfdHandle* handle, int parameter, int value)
{
    SfdHandle* seek_source = handle->seek_state.source_handle;
    SfmpsSeekSnapshot* snapshot;
    SfmpsWork* work;
    int consumed;
    int header_flags;
    int first_result;
    int second_result;
    int result;

    if (seek_source == 0 || sfmps_GetWork(handle)->concat_count > 0) {
        snapshot = 0;
    } else {
        snapshot = (SfmpsSeekSnapshot*)((unsigned char*)seek_source + 0x8A0);
    }
    if (snapshot == 0 || snapshot->active == 0) {
        return 0;
    }

    work = sfmps_GetWork(handle);
    SFHDS_ReprocessHdr(handle);
    first_result = MPS_DecHd(work->decoder, snapshot->header0,
                             snapshot->header0_size, &consumed,
                             &header_flags);
    second_result = MPS_DecHd(work->decoder, snapshot->header1,
                              snapshot->header1_size, &consumed,
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
    handle->playback_runtime.tail_values[3] = snapshot->playback_value;
    handle->playback_runtime.tail_values[2] = snapshot->timer_value;
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

static int SFMPS_Create(SfdHandle* handle)
{
    SfmpsWork* work = (SfmpsWork*)handle->mps_work_storage;
    MpsHandle* decoder;
    int i;

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
    for (i = 0; i < 0x44; i++) {
        work->element_outputs[i] = 0;
    }
    work->element_callback = 0;
    work->element_callback_argument = 0;
    work->scan_position = -1;

    decoder = MPS_Create();
    if (decoder == 0) {
        return SFLIB_SetErr(0, 0xFF000D08);
    }
    if (MPS_SetErrFn(decoder, sfmps_ErrFn, (MpsCallbackObject)handle) != 0) {
        MPS_Destroy(decoder);
        return SFLIB_SetErr(0, 0xFF000D09);
    }
    work->decoder = decoder;
    return 0;
}

static void sfmps_ProcPrep(SfdHandle* handle)
{
    SfmpsWork* work = sfmps_GetWork(handle);
    MpsSystemHeader system;
    MpsPackHeader pack;
    SfmpsSeekSnapshot* snapshot;
    int max_audio = 0;
    int max_video = 0;
    int threshold;
    int i;

    for (i = 0; i < 3; i++) {
        MPS_GetSysHd(work->decoder, &system, i);
        if (system.audio_bound > max_audio) {
            max_audio = system.audio_bound;
        }
        if (system.video_bound > max_video) {
            max_video = system.video_bound;
        }
    }
    work->max_audio_bound = max_audio;
    work->max_video_bound = max_video;

    if ((SFBUF_GetPrepFlg(handle, handle->transports[1].parameter_18) |
         SFBUF_GetPrepFlg(handle, handle->transports[1].parameter_14) |
         SFBUF_GetPrepFlg(handle, handle->transports[1].parameter_1C)) != 1 &&
        SFBUF_GetPrepFlg(handle, handle->transports[1].parameter_10) == 1) {
        threshold = handle->create_config.buffer.buffer_sizes[0];
        if (threshold <= 0) {
            threshold =
                handle->buffers[handle->transports[1].parameter_10]
                    .work.ring.buffer_size;
        }
        if (threshold <= 0) {
            threshold = handle->conditions_primary[22];
        }
        if (threshold > handle->conditions_primary[22]) {
            threshold = handle->conditions_primary[22];
        }
        if (SFBUF_GetWTot(handle, 0) >= threshold) {
            SFBUF_SetPrepFlg(handle, handle->transports[1].parameter_18, 1);
            SFBUF_SetPrepFlg(handle, handle->transports[1].parameter_14, 1);
            SFBUF_SetPrepFlg(handle, handle->transports[1].parameter_1C, 1);
        }
    }

    MPS_GetPackHd(work->decoder, &pack);
    if (pack.mux_rate != -1 && pack.mux_rate > 0) {
        handle->playback_settings.values_18[0] = pack.mux_rate;
    }
    MPS_GetSysHd(work->decoder, &system, 1);
    if (system.fixed_flag != -1) {
        handle->playback_settings.values_18[1] = system.fixed_flag;
    }
    if (handle->playback_settings.values_18[3] == -1) {
        handle->playback_settings.values_18[3] = work->max_audio_bound;
    }
    if (handle->playback_settings.values_18[4] == -1) {
        handle->playback_settings.values_18[4] = work->max_video_bound;
    }

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

    if (handle->seek_state.source_handle == 0 || work->concat_count > 0) {
        snapshot = 0;
    } else {
        snapshot = (SfmpsSeekSnapshot*)((unsigned char*)
            handle->seek_state.source_handle + 0x8A0);
    }
    if (snapshot != 0 &&
        work->audio_pts_baseline != 0x7FFFFFFFFFFFFFFFLL) {
        handle->timer_state.audio_start_pts =
            work->audio_pts_baseline - snapshot->video_pts_baseline;
        if (snapshot->active == 0) {
            snapshot->mux_rate = pack.mux_rate * 50;
            snapshot->rate_bound = system.rate_bound;
            snapshot->video_bound = work->max_video_bound;
            snapshot->audio_bound = work->max_audio_bound;
            snapshot->timer_value = handle->timer_state.field_0150;
            snapshot->video_pts_baseline = work->video_pts_baseline;
            snapshot->first_video_stream = work->first_video_stream;
            snapshot->first_audio_stream = work->first_audio_stream;
        }
    }
}

static int sfmps_CopyDstBuft(SfdHandle* handle, int buffer_index,
                              const unsigned char* data, int size,
                              long long pts)
{
    SfdBufferTransfer transfer;
    int result;

    result = SFBUF_RingGetWrite(handle, buffer_index, &transfer);
    if (result != 0) {
        return result;
    }
    if (size > transfer.chunks[0].len + transfer.chunks[1].len) {
        return 0;
    }
    if (buffer_index == 1 && pts >= 0) {
        SfdPtsEntry entry;
        int full;

        if (SFPTS_IsPtsQueFull(handle, buffer_index) != 0) {
            return 0;
        }
        entry.pts = pts;
        entry.data = transfer.chunks[0].data;
        entry.size = size;
        result = SFPTS_WritePtsQue(handle, buffer_index, &entry, &full);
        if (result != 0) {
            return result;
        }
    } else if (buffer_index == 2 && SFPLY_SetPtsInfo != 0) {
        SfdPlayerPtsInfo info;

        info.pts = pts;
        info.size = size;
        if (SFPLY_SetPtsInfo(handle->timer_state.video_pts, &info) == -1) {
            return 0;
        }
    }
    if (size <= transfer.chunks[0].len) {
        MEM_Copy(transfer.chunks[0].data, data, size);
    } else {
        MEM_Copy(transfer.chunks[0].data, data, transfer.chunks[0].len);
        MEM_Copy(transfer.chunks[1].data,
                 data + transfer.chunks[0].len,
                 size - transfer.chunks[0].len);
    }
    result = SFBUF_RingAddWrite(handle, buffer_index, size,
                                transfer.field_14);
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
    SJCK chunk;
    int remaining;

    if (sj->interface->get_num_data(sj, 0) < size) {
        return 0;
    }
    sj->interface->get_chunk(sj, 0, size, &chunk);
    MEM_Copy(chunk.data, data, chunk.len);
    sj->interface->put_chunk(sj, 1, &chunk);
    if (chunk.len == 0) {
        return 0;
    }

    remaining = size - chunk.len;
    if (remaining > 0) {
        sj->interface->get_chunk(sj, 0, remaining, &chunk);
        MEM_Copy(chunk.data, data + size - remaining, chunk.len);
        sj->interface->put_chunk(sj, 1, &chunk);
        if (chunk.len != remaining) {
            copy_sj_error++;
        }
    }
    return 1;
}

static int sfmps_CopyPrvate(SfdHandle* handle, int stream_index,
                             const unsigned char* data, int size,
                             long long pts)
{
    SfdBufferChannel channel;
    int buffer_index = handle->transports[1].parameter_1C;
    int header_flag;
    int result;

    if (SFHDS_SetHdr(handle, stream_index, data, size, &header_flag) != 0) {
        if (header_flag != 0 && buffer_index != 8) {
            SFBUF_GetUoch(handle, buffer_index, 0, &channel);
            if (channel.stream_joint != 0) {
                result = sfmps_CopyToSj(channel.stream_joint, data - 0x12,
                                        size + 0x12);
                if (result != 0) {
                    if (channel.handle_callback != 0) {
                        channel.handle_callback(handle, 0);
                    }
                    if (channel.object_callback != 0) {
                        channel.object_callback(channel.object, 0);
                    }
                }
            }
        }
        return 1;
    }

    if (buffer_index == 8) {
        return 1;
    }
    SFBUF_GetUoch(handle, buffer_index, stream_index, &channel);
    if (channel.stream_joint == 0) {
        return 1;
    }
    result = sfmps_CopyToSj(channel.stream_joint, data, size);
    if (result != 0) {
        if (channel.handle_callback != 0) {
            channel.handle_callback(handle, stream_index);
        }
        if (channel.object_callback != 0) {
            channel.object_callback(channel.object, stream_index);
        }
    }
    return result;
}

static int sfmps_CopyVideo(SfdHandle* handle, int stream_index,
                            const unsigned char* data, int size,
                            long long pts)
{
    SfmpsWork* work;
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
            int index;
            int max_audio = 0;
            int max_video = 0;

            for (index = 0; index < 3; index++) {
                MpsSystemHeader header;
                int audio_bound;
                int video_bound;

                MPS_GetSysHd(work->decoder, &header, index);
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
            if (max_video >= 2) {
                candidate = 2;
            } else {
                candidate = stream_index;
            }
            break;
        }
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
            } else if (data[3] == 0xB3) {
                is_boundary = 1;
            } else {
                is_boundary = data[3] == 0xB8;
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
        if (pts < work->video_pts_baseline) {
            work->video_pts_baseline = pts;
        }
        if (pts < work->audio_pts_baseline) {
            work->audio_pts_baseline = pts;
        }
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

static int sfmps_CopyPketData(SfdHandle* handle, const unsigned char* data,
                              int available, int* consumed, int* copied)
{
    SfmpsWork* work = sfmps_GetWork(handle);
    MpsPacketHeader header;
    SJ* output;
    int result = 0;

    *consumed = 0;
    *copied = 0;
    if (MPS_GetPketHd(work->decoder, &header) != 0) {
        result = SFLIB_SetErr(handle, 0xFF000D06);
    }
    if (header.payload_length < 0) {
        return SFLIB_SetErr(handle, 0xFF000D0E);
    }
    if (header.payload_length == 0) {
        *consumed = 0;
        *copied = 1;
        return 0;
    }
    if (available < header.payload_length) {
        if (SFBUF_GetTermFlg(handle,
                             handle->transports[1].parameter_10) == 1) {
            SFBUF_SetTermFlg(handle, handle->transports[1].parameter_18, 1);
            SFBUF_SetTermFlg(handle, handle->transports[1].parameter_14, 1);
            SFBUF_SetTermFlg(handle, handle->transports[1].parameter_1C, 1);
        }
        return 0;
    }

    output = work->element_outputs[header.stream_id - 0xBC];
    if (output != 0) {
        int copy_result = sfmps_CopyToSj(output, data,
                                         header.payload_length);

        if (copy_result == 1 && work->element_callback != 0) {
            work->element_callback(work->element_callback_argument,
                                   header.stream_id);
        }
        *copied = copy_result;
    } else {
        *copied = sfmps_CopyPketFn[header.stream_type](
            handle, header.stream_index, data, header.payload_length,
            header.pts);
    }
    if (*copied == 1) {
        *consumed = header.payload_length;
    } else if (*copied < 0 || *copied > 1) {
        result = *copied;
    }
    return result;
}

static inline void sfmps_SetOutputTerminated(SfdHandle* handle)
{
    SFBUF_SetTermFlg(handle, handle->transports[1].parameter_18, 1);
    SFBUF_SetTermFlg(handle, handle->transports[1].parameter_14, 1);
    SFBUF_SetTermFlg(handle, handle->transports[1].parameter_1C, 1);
}

static inline int sfmps_IsInputTerminated(SfdHandle* handle)
{
    if (SFBUF_GetTermFlg(handle,
                         handle->transports[1].parameter_10) == 1) {
        sfmps_SetOutputTerminated(handle);
        return 1;
    }
    return 0;
}

static int sfmps_DecodeOneUnit(SfdHandle* handle, const unsigned char* data,
                               int size, int* consumed, int* copied,
                               int readable)
{
    SfmpsWork* work = sfmps_GetWork(handle);
    MpsHandle* decoder = work->decoder;
    int delimiter = 0;
    int decoded_size;
    int header_flags;
    int result = 0;

    *consumed = 0;
    *copied = 0;
    if (size >= 4) {
        delimiter = MPS_CheckDelim(data);
        if (delimiter == 0x80000) {
            if (handle->transports[1].state < 0) {
                handle->transports[1].state =
                    SFBUF_GetRTot(handle,
                        handle->transports[1].parameter_10) + 4;
            }
            work->delimiter_state = 1;
        } else if (delimiter != 0) {
            work->delimiter_state = 0;
        }
    }

    if (delimiter == 0x80000 && SFCON_IsEndcodeSkip(handle) == 0 &&
        SFCON_IsSystemEndcodeSkip(handle) == 0) {
        sfmps_SetOutputTerminated(handle);
        return 0;
    }
    if (readable < 4 && sfmps_IsInputTerminated(handle) != 0) {
        return 0;
    }
    if (size < 0x40 &&
        (delimiter == 0x10000 || delimiter == 0x40000)) {
        sfmps_IsInputTerminated(handle);
        return 0;
    }

    delimiter = size >= 4 ? MPS_CheckDelim(data) : 0;
    MPS_SetPsMapFn(decoder,
                   (MpsPsMapCallback)SFSET_GetCond(handle, 0x57),
                   SFSET_GetCond(handle, 0x58));
    MPS_SetPesFn(decoder,
                 (MpsPesCallback)SFSET_GetCond(handle, 0x5B),
                 SFSET_GetCond(handle, 0x5C));
    if (MPS_DecHd(decoder, data, size, &decoded_size, &header_flags) != 0) {
        result = SFLIB_SetErr(handle, 0xFF000D03);
    }

    if ((header_flags & 0x20000) != 0) {
        SfmpsSeekSnapshot* snapshot;

        if (handle->seek_state.source_handle == 0 || work->concat_count > 0) {
            snapshot = 0;
        } else {
            snapshot = (SfmpsSeekSnapshot*)((unsigned char*)
                handle->seek_state.source_handle + 0x8A0);
        }
        if (snapshot != 0 && snapshot->active == 0) {
            MpsSystemHeader system;
            unsigned char* destination;
            int copy_size = size < 0xB0 ? size : 0xB0;

            MPS_GetLastSysHd(decoder, &system);
            if (system.video_bound > 0) {
                snapshot->header0_size = copy_size;
                destination = snapshot->header0;
            } else if (system.audio_bound > 0) {
                snapshot->header1_size = copy_size;
                destination = snapshot->header1;
            } else {
                destination = 0;
            }
            if (destination != 0) {
                MEM_Copy(destination, data, copy_size);
            }
        }
    }

    if (header_flags == 0x80000 && SFCON_IsEndcodeSkip(handle) != 0) {
        work->concat_count++;
        *consumed = 4;
        work->scan_position = 4;
    } else if (header_flags == 0x80000 &&
               SFCON_IsSystemEndcodeSkip(handle) != 0) {
        *consumed = 4;
        work->scan_position = 4;
    } else if (delimiter == 0) {
        const unsigned char* cursor = data;
        int remaining = size;
        int amount = 0;
        int prefix_size = handle->create_config.buffer.field_24;
        int prefix_is_zero = 1;
        int i;

        *copied = 0;
        if (size >= prefix_size + 3) {
            for (i = 0; i < prefix_size; i++) {
                if ((signed char)data[i] != 0) {
                    prefix_is_zero = 0;
                    break;
                }
            }
            if (prefix_is_zero) {
                *copied = prefix_size;
            }
        }
        if (*copied == 0) {
            while (remaining >= 4) {
                if ((MPS_CheckDelim(cursor) & 0xD0000) != 0) {
                    break;
                }
                amount++;
                cursor++;
                remaining--;
            }
            if (remaining > 0 && remaining < 4) {
                SfdBufferState* input =
                    &handle->buffers[handle->transports[1].parameter_10];
                SfdBufferRingWork* ring = &input->work.ring;
                int at_end;

                if (input->terminated == 0 &&
                    (ring->buffer_size != 0 || ring->field_10 != 0)) {
                    at_end = 0;
                } else {
                    at_end = cursor + remaining ==
                             ring->buffer + ring->buffer_size;
                }
                if (at_end) {
                    amount += remaining;
                }
            }
            *copied = amount;
        }
        *consumed = *copied;
        if (*copied > 0 && work->scan_position >= 0) {
            if (work->scan_position >= prefix_size) {
                work->scan_position += *copied;
            } else if (work->scan_position + *copied > prefix_size) {
                *copied -= prefix_size - work->scan_position;
                work->scan_position = prefix_size + *copied;
            } else {
                work->scan_position += *copied;
                *copied = 0;
            }
        }
    } else if ((header_flags & 0x40000) == 0) {
        if (sfmps_IsInputTerminated(handle) == 0 &&
            size > handle->create_config.buffer.field_24) {
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

        result = sfmps_CopyPketData(handle, data + decoded_size,
                                    size - decoded_size, &packet_consumed,
                                    &packet_copied);
        if (packet_copied == 1) {
            *consumed = decoded_size + packet_consumed;
        }
        work->scan_position = -1;
    }
    return result;
}

static int sfmps_ExecServerSub(SfdHandle* handle)
{
    SfmpsWork* work;
    SfdBufferTransfer transfer;
    int result = 0;
    int consumed_total = 0;
    int copied_total = 0;
    int write_flow;
    int read_flow;
    unsigned char* input_data;
    int input_size;
    int readable;
    int consumed;
    int copied;

    if ((SFBUF_GetTermFlg(handle, handle->transports[1].parameter_18) &
         SFBUF_GetTermFlg(handle, handle->transports[1].parameter_14) &
         SFBUF_GetTermFlg(handle, handle->transports[1].parameter_1C)) == 1) {
        return 0;
    }
    work = sfmps_GetWork(handle);
    MPS_SetSystemFn(work->decoder,
                    (MpsSystemCallback)SFSET_GetCond(handle, 0x55),
                    SFSET_GetCond(handle, 0x56));

    while (consumed_total < 0x7FFFFFFF) {
        result = SFBUF_RingGetRead(handle,
                                   handle->transports[1].parameter_10,
                                   &transfer);
        if (result != 0) {
            break;
        }
        input_data = transfer.chunks[0].data;
        input_size = transfer.chunks[0].len;
        readable = input_size + transfer.chunks[1].len;
        result = sfmps_DecodeOneUnit(handle, input_data, input_size,
                                     &consumed, &copied, readable);
        if (result != 0 || consumed == 0) {
            break;
        }
        result = SFBUF_RingAddRead(handle,
                                   handle->transports[1].parameter_10,
                                   consumed);
        if (result != 0) {
            break;
        }
        copied_total += copied;
        consumed_total += consumed;
    }

    SFBUF_GetFlowCnt(
        handle->buffers[handle->transports[1].parameter_10]
            .work.ring.stream_joint,
        &write_flow, &read_flow);
    handle->playback_runtime.time_values[0] =
        SFBUF_UpdateFlowCnt(
            (int)(handle->playback_runtime.time_values[0] >> 32),
            (unsigned int)handle->playback_runtime.time_values[0],
            write_flow);
    handle->playback_runtime.time_values[1] += consumed_total;
    handle->playback_runtime.time_values[2] += copied_total;
    if (handle->playback_state == 2) {
        sfmps_ProcPrep(handle);
    }
    return result;
}

static int SFMPS_ExecServer(SfdHandle* handle)
{
    return sfmps_ExecServerSub(handle);
}

static int SFMPS_Finish(SfdHandle* handle)
{
    MPS_Finish();
    return 0;
}

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
