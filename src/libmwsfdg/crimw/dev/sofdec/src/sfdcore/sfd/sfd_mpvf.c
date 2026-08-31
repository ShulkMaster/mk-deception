#include "sofdec/sfd_error.h"
#include "sofdec/sfd_mpvf.h"
#include "sofdec/sfd_player.h"
#include "sofdec/sfd_timer.h"
#include "sofdec/sfd_transport.h"

static inline SfdMpvFrameWork* sfmpvf_GetWork(SfdHandle* handle)
{
    return (SfdMpvFrameWork*)handle->transports[2].context;
}

static inline int sfmpvf_IsEarlier(const SfdMpvFrame* candidate,
                                   const SfdMpvFrame* current)
{
    if (current == 0) {
        return 1;
    }
    if (candidate->picture_order != current->picture_order) {
        return candidate->picture_order < current->picture_order;
    }
    if (candidate->picture_info.order.field_order !=
        current->picture_info.order.field_order) {
        return candidate->picture_info.order.field_order <
               current->picture_info.order.field_order;
    }
    if (candidate->picture_info.order.decode_order !=
        current->picture_info.order.decode_order) {
        return candidate->picture_info.order.decode_order <
               current->picture_info.order.decode_order;
    }
    if (candidate->picture_info.order.temporal_reference -
            current->picture_info.order.temporal_reference > 0x200) {
        return 1;
    }
    if (current->picture_info.order.temporal_reference -
            candidate->picture_info.order.temporal_reference > 0x200) {
        return 0;
    }
    return candidate->picture_info.order.temporal_reference <
           current->picture_info.order.temporal_reference;
}

static SfdMpvFrame* sfmpvf_ReferNextFrmReady(SfdHandle* handle)
{
    SfdMpvFrameWork* work;
    SfdMpvFrame* first = 0;
    SfdMpvFrame* second = 0;
    SfdMpvFrame* frame;
    int ready_count = 0;
    int can_return;
    int token;
    int i;

    SFLIB_LockCs(&token);
    work = sfmpvf_GetWork(handle);
    frame = work->frames;
    for (i = 0; i < work->frame_count; i++, frame++) {
        if (frame->state == 2 || frame->state == 4) {
            ready_count++;
            if (sfmpvf_IsEarlier(frame, first)) {
                second = first;
                first = frame;
            } else if (sfmpvf_IsEarlier(frame, second)) {
                second = frame;
            }
        }
    }
    if (handle->playback_state != 4) {
        can_return = 0;
    } else if (ready_count <= 1) {
        can_return = 0;
    } else if (ready_count == 2 && work->decoder_terminated == 0) {
        can_return = 0;
    } else if (SFSET_GetCond(handle, 0x0F) == 0) {
        can_return = 1;
    } else if (SFTIM_IsGetFrmTimeTunit(handle, second->display_time_value,
                                       second->display_time_scale) != 0) {
        can_return = 1;
    } else {
        can_return = 0;
    }
    if (can_return == 0) {
        second = 0;
    }
    SFLIB_UnlockCs(&token);
    return second;
}

int SFMPVF_IsNextFrmReady(SfdHandle* handle)
{
    return sfmpvf_ReferNextFrmReady(handle) != 0;
}

SfdMpvFrame* SFMPVF_HoldFrm(SfdHandle* handle, int* sole_frame)
{
    SfdMpvFrameWork* work;
    SfdMpvFrame* selected = 0;
    SfdMpvFrame* frame;
    int ready_count = 0;
    int token;
    int i;

    SFLIB_LockCs(&token);
    work = sfmpvf_GetWork(handle);
    *sole_frame = 0;
    frame = work->frames;
    for (i = 0; i < work->frame_count; i++, frame++) {
        if (frame->state == 2 || frame->state == 4) {
            ready_count++;
            if (sfmpvf_IsEarlier(frame, selected)) {
                selected = frame;
            }
        }
    }
    if (ready_count == 1) {
        if (work->decoder_terminated != 0) {
            *sole_frame = 1;
        } else if (work->gop_state == 0) {
            selected = 0;
        }
    }
    SFLIB_UnlockCs(&token);
    return selected;
}

void SFMPVF_EndRefFrm(SfdMpvFrame* frame)
{
    if (frame != 0) {
        if (frame->state == 4) {
            frame->state = 2;
        } else {
            frame->state = 0;
        }
    }
}

void SFMPVF_EndDrawFrm(SfdMpvFrame* frame)
{
    if (frame != 0) {
        if (frame->state == 4) {
            frame->state = 3;
        } else {
            frame->state = 0;
        }
    }
}

void SFMPVF_RefStbyFrm(SfdMpvFrame* frame)
{
    if (frame != 0) {
        frame->state = 4;
    }
}

void SFMPVF_StbyFrm(SfdMpvFrame* frame)
{
    if (frame != 0) {
        frame->state = 2;
    }
}

void SFMPVF_FreeFrm(SfdMpvFrame* frame)
{
    if (frame != 0) {
        frame->state = 0;
    }
}

SfdMpvFrame* SFMPVF_AllocFrm(SfdHandle* handle)
{
    SfdMpvFrameWork* work;
    SfdMpvFrame* frame;
    int token;
    int i;

    SFLIB_LockCs(&token);
    work = sfmpvf_GetWork(handle);
    frame = work->frames;
    for (i = 0; i < work->frame_count; i++, frame++) {
        if (frame->state == 0 && frame->reference_count == 0) {
            frame->state = 1;
            break;
        }
    }
    if (i == work->frame_count) {
        frame = 0;
    }
    SFLIB_UnlockCs(&token);
    return frame;
}

int SFMPVF_GetNumFrm(SfdHandle* handle)
{
    SfdMpvFrameWork* work;
    SfdMpvFrame* frame;
    int count = 0;
    int token;
    int i;

    SFLIB_LockCs(&token);
    work = sfmpvf_GetWork(handle);
    frame = work->frames;
    for (i = 0; i < work->frame_count; i++, frame++) {
        if (frame->state == 2 || frame->state == 4) {
            count++;
        }
    }
    if (work->decoder_terminated == 1 && count == 0) {
        count = -1;
    }
    SFLIB_UnlockCs(&token);
    return count;
}

void SFMPVF_SetGopStat(SfdHandle* handle, int state)
{
    sfmpvf_GetWork(handle)->gop_state = state;
}

int SFMPVF_IsTermDec(SfdHandle* handle)
{
    return sfmpvf_GetWork(handle)->decoder_terminated;
}

void SFMPVF_TermDec(SfdHandle* handle)
{
    sfmpvf_GetWork(handle)->decoder_terminated = 1;
}

SfdVideoFrameState* SFMPVF_SearchVfrmData(SfdHandle* handle,
                                          const SfdMpvFrame* frame)
{
    SfdMpvFrameWork* work = sfmpvf_GetWork(handle);
    int i;

    for (i = 0; i < work->frame_count; i++) {
        if (&work->frames[i] == frame) {
            return &handle->video_frames[i];
        }
    }
    return 0;
}

SfdMpvFrame* SFMPVF_SearchFrmObj(SfdHandle* handle, const void* frame_data)
{
    SfdMpvFrameWork* work = sfmpvf_GetWork(handle);
    int i;

    for (i = 0; i < 16; i++) {
        if (handle->video_frames[i].data.payload == frame_data) {
            return &work->frames[i];
        }
    }
    return 0;
}
