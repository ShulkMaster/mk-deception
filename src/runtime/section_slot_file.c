#include "runtime/section_slot_file.h"
#include "runtime/cstring.h"

#include "runtime/mk_hwfile.h"
#include "runtime/mk_proc.h"
#include "runtime/mk_vtbl.h"

typedef struct SsfReqQueue {
    SsfReqLink head;
    SsfReqLink* tail;
} SsfReqQueue;

extern MkProc* saved_aproc;

static void priv_sec_slot_file_read_all(SsfReq* request);
static SsfReq* sec_slot_file_open_file_async_withcallback(
    SecSlotFileEntry* file, SecSlot* slot, int field_0x1C, MkFileInfo* info,
    void* userdata, int queued, SsfReqCompletion completion);
static void sec_slot_file_queue_open_callback(void* user,
                                               MkFileEntry* file_entry,
                                               int success);

static SsfReq ssf_req_Pool[40];
static SsfReqQueue ssf_req_Queue;
static SsfReq* ssf_req_FreeList;
static SsfReq* ssf_req_CurrentItem;

static inline void wait_for_async_work(void) {
    if (aproc != 0 && aproc->stack_top != 0) {
        _mkproc_sleep_ticks = 1.0f;
        aproc->vtbl->sleep();
    } else {
        mk_hwfile_busywait_dowork();
    }
}

static inline int is_ssf_request_pending(MkFileEntry* ssf_file) {
    SsfReq* request;
    int found = 0;

    if (ssf_file != 0) {
        request = ssf_req_CurrentItem;
        if (request != 0 && request->ssf_file == ssf_file) {
            found = 1;
        }
        request = ssf_req_Queue.head.next;
        while (request != 0 && found == 0) {
            if (request->ssf_file == ssf_file) {
                found = 1;
            }
            request = request->link.next;
        }
    }
    return found;
}

static inline void recycle_request(SsfReq* request) {
    memset(request, 0, sizeof(*request));
    request->link.next = ssf_req_FreeList;
    ssf_req_FreeList = request;
}

static inline SsfReq* dequeue_request(void) {
    SsfReq* request = ssf_req_Queue.head.next;

    if (request != 0) {
        SsfReq* next = request->link.next;
        request->link.next = 0;
        ssf_req_Queue.head.next = next;
        if (next == 0) {
            ssf_req_Queue.tail = &ssf_req_Queue.head;
        }
        request->queued = 0;
    }
    return request;
}

int sec_slot_file_open_read_async(SecSlotFileEntry* file, SecSlot* slot,
                                  int field_0x1C, MkFileInfo* info,
                                  void* userdata) {
    return sec_slot_file_open_read_async_queued(file, slot, field_0x1C, info,
                                                userdata);
}

int sec_slot_file_open_read_async_queued(SecSlotFileEntry* file, SecSlot* slot,
                                         int field_0x1C, MkFileInfo* info,
                                         void* userdata) {
    sec_slot_file_open_file_async_withcallback(
        file, slot, field_0x1C, info, userdata, 1,
        priv_sec_slot_file_read_all);
    return 1;
}

static void priv_sec_slot_file_read_all(SsfReq* request) {
    SecSlotFileEntry* file = request->owner;
    SecSlot* slot = request->slot;
    int file_size = 0;
    unsigned int aligned_size;

    if (file == 0 || request->file_entry == 0) {
        return;
    }
    if (file->async_req != 0 && file->async_req->file_entry != 0) {
        file_size = mk_file_length(file->async_req->file_entry);
    }

    aligned_size = (file_size + 0x7FF) & ~0x7FF;
    if ((int)aligned_size >
        (int)((slot->base + (slot->buffer_size & 0x7FFFFFFF)) -
              file->buffer)) {
        if (file->buffer != 0 && file->next != 0 &&
            file->next->buffer == 0) {
            file->next->buffer = file->buffer + file->size_or_flag;
        }
        sec_slot_file_close_file(file);
        return;
    }

    if (file->async_req != 0 && file->async_req->file_entry != 0) {
        file->async_req->hwfile =
            mk_file_read_async(file->buffer, 1, file_size,
                               file->async_req->file_entry);
    }
    file->size_or_flag = (file_size + 0x7F) & ~0x7F;
    if (file->buffer != 0 && file->next != 0 && file->next->buffer == 0) {
        file->next->buffer = file->buffer + file->size_or_flag;
    }
    sec_slot_file_close_file(file);
}

void sec_slot_file_wait_for_load(SecSlotFileEntry* file) {
    SsfReq* request = file->async_req;

    if (request != 0 && request->loading != 0) {
        while (request->loading != 0 && request->hwfile == 0) {
            wait_for_async_work();
            if (request != file->async_req || request->owner != file) {
                return;
            }
        }
    }
    if (request != 0 && request->hwfile != 0) {
        mk_hwfile_wait_for_completion_or_null_request(
            &request->hwfile);
    }
}

void sec_slot_file_cancel_async(SecSlotFileEntry* file) {
    SsfReq* request;

    if (file->buffer != 0 && file->next != 0 && file->next->buffer == 0) {
        file->next->buffer = file->buffer + file->size_or_flag;
    }

    request = file->async_req;
    if (request == 0) {
        return;
    }
    if (request->queued != 0) {
        SsfReqLink* link = &ssf_req_Queue.head;
        SsfReq* item = link->next;

        while (item != 0) {
            if (item == request) {
                link->next = item->link.next;
                if (item->link.next == 0) {
                    ssf_req_Queue.tail = link;
                }
                request->link.next = 0;
                request->queued = 0;
                break;
            }
            link = &item->link;
            item = item->link.next;
        }
        file->async_req = 0;
        request->owner = 0;
        recycle_request(request);
        return;
    }
    if (request == ssf_req_CurrentItem) {
        request->cancelled = 1;
        request->owner = 0;
        file->async_req = 0;
        return;
    }
    if (request->hwfile != 0 && request != 0) {
        if (request->hwfile != 0) {
            saved_aproc = aproc;
            aproc = 0;
            mk_hwfile_cancel(request->hwfile);
            aproc = saved_aproc;
            mk_hwfile_free_request(request->hwfile);
            request->hwfile = 0;
        }
        if (request->file_entry == 0) {
            file->async_req = 0;
            recycle_request(request);
        }
    }
}

void sec_slot_file_free_async(SecSlotFileEntry* file) {
    SsfReq* request = file->async_req;

    if (request != 0) {
        if (request->hwfile != 0) {
            mk_hwfile_free_request(request->hwfile);
            request->hwfile = 0;
        }
        if (request->file_entry == 0) {
            file->async_req = 0;
            recycle_request(request);
        }
    }
}

#pragma dont_inline on
void sec_slot_file_close_file(SecSlotFileEntry* file) {
    SsfReq* request = file->async_req;

    if (request == 0) {
        return;
    }
    if (request->file_entry != 0) {
        mk_file_close(request->file_entry);
        request->file_entry = 0;
    }
    if (ssf_req_CurrentItem == request) {
        SsfReq* next_request = ssf_req_Queue.head.next;
        MkFileEntry* previous_ssf = 0;
        MkFileEntry* requested_ssf;

        ssf_req_CurrentItem = 0;
        if (next_request != 0) {
            SsfReq* following = next_request->link.next;

            next_request->link.next = 0;
            ssf_req_Queue.head.next = following;
            if (following == 0) {
                ssf_req_Queue.tail = &ssf_req_Queue.head;
            }
            next_request->queued = 0;
        }
        if (next_request != 0) {
            ssf_req_CurrentItem = next_request;
            requested_ssf = next_request->ssf_file;
            if (requested_ssf != 0) {
                previous_ssf = get_current_ssf_file();
                if (previous_ssf != requested_ssf) {
                    load_ssf(requested_ssf);
                }
            }
            mk_file_open_async_withcallback(
                next_request->info, "rb", next_request->userdata,
                sec_slot_file_queue_open_callback, next_request);
            if (previous_ssf != requested_ssf && previous_ssf != 0) {
                load_ssf(previous_ssf);
            }
        }
    }
    if (request->hwfile == 0) {
        file->async_req = 0;
        memset(request, 0, sizeof(*request));
        request->link.next = ssf_req_FreeList;
        ssf_req_FreeList = request;
    }
}
#pragma dont_inline reset

static SsfReq* sec_slot_file_open_file_async_withcallback(
    SecSlotFileEntry* file, SecSlot* slot, int field_0x1C, MkFileInfo* info,
    void* userdata, int queued, SsfReqCompletion completion) {
    SsfReq* request;
    SsfReq* result;

    file->section_info = info;
    request = ssf_req_FreeList;
    if (request != 0) {
        ssf_req_FreeList = request->link.next;
        memset(request, 0, sizeof(*request));
    }
    file->async_req = request;
    result = request;
    request->owner = file;
    request->slot = slot;
    request->field_0x1C = field_0x1C;

    if (queued != 0) {
        SsfReq* next_request;
        MkFileEntry* previous_ssf = 0;
        MkFileEntry* requested_ssf;

        request->ssf_file = get_current_ssf_file();
        request->loading = 1;
        request->userdata = userdata;
        request->completion = completion;
        request->info = info;
        ssf_req_Queue.tail->next = request;
        ssf_req_Queue.tail = &request->link;
        request->link.next = 0;
        request->queued = 1;

        if (ssf_req_CurrentItem == 0) {
            next_request = dequeue_request();
            if (next_request != 0) {
                ssf_req_CurrentItem = next_request;
                requested_ssf = next_request->ssf_file;
                if (requested_ssf != 0) {
                    previous_ssf = get_current_ssf_file();
                    if (previous_ssf != requested_ssf) {
                        load_ssf(requested_ssf);
                    }
                }
                mk_file_open_async_withcallback(
                    next_request->info, "rb", next_request->userdata,
                    sec_slot_file_queue_open_callback, next_request);
                if (previous_ssf != requested_ssf && previous_ssf != 0) {
                    load_ssf(previous_ssf);
                }
            }
        }
        result = file->async_req;
    } else {
        request->file_entry = mk_file_open(info, "rb", userdata);
    }
    return result;
}

void sec_slot_file_wait_on_ssf(MkFileEntry* ssf_file) {
    while (is_ssf_request_pending(ssf_file) != 0) {
        wait_for_async_work();
    }
}

static void sec_slot_file_queue_open_callback(void* user,
                                               MkFileEntry* file_entry,
                                               int success) {
    SsfReq* request = user;

    request->loading = 0;
    if (request->cancelled != 0) {
        SsfReq* next_request;
        MkFileEntry* previous_ssf = 0;
        MkFileEntry* requested_ssf;

        mk_file_close(file_entry);
        request->file_entry = 0;
        request->completion(request);
        next_request = dequeue_request();
        ssf_req_CurrentItem = 0;
        if (next_request != 0) {
            ssf_req_CurrentItem = next_request;
            requested_ssf = next_request->ssf_file;
            if (requested_ssf != 0) {
                previous_ssf = get_current_ssf_file();
                if (previous_ssf != requested_ssf) {
                    load_ssf(requested_ssf);
                }
            }
            mk_file_open_async_withcallback(
                next_request->info, "rb", next_request->userdata,
                sec_slot_file_queue_open_callback, next_request);
            if (previous_ssf != requested_ssf && previous_ssf != 0) {
                load_ssf(previous_ssf);
            }
        }
        recycle_request(request);
        return;
    }

    if (file_entry != 0 && success == 0) {
        mk_file_close(file_entry);
        file_entry = 0;
    }
    request->file_entry = file_entry;
    request->completion(request);
    if (success == 0 && request->owner != 0) {
        SecSlotFileEntry* file = request->owner;

        if (file->buffer != 0 && file->next != 0 &&
            file->next->buffer == 0) {
            file->next->buffer = file->buffer + file->size_or_flag;
        }
        file->size_or_flag = 0;
        sec_slot_file_close_file(file);
    }
}

void init_sec_slot_files(void) {
    int index;

    ssf_req_Queue.head.next = 0;
    ssf_req_Queue.tail = &ssf_req_Queue.head;
    ssf_req_CurrentItem = 0;
    ssf_req_FreeList = ssf_req_Pool;
    for (index = 0; index < 39; index++) {
        ssf_req_Pool[index].link.next = &ssf_req_Pool[index + 1];
    }
    ssf_req_Pool[39].link.next = 0;
}
