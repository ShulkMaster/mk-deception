#include "runtime/mk_fileinfo.h"
#include "runtime/cstring.h"

#include "platform/gcutils.h"
#include "runtime/mk_hwfile.h"
#include "runtime/section_slot_file.h"
#include "runtime/utils.h"

/* The disabled interface may still be called through RenderWare's varied
 * file-operation signatures, so retain an old-style generic function type. */
typedef int (*RwFileFunction)();

typedef struct RwFileInterface {
    RwFileFunction open;
    RwFileFunction close;
    RwFileFunction read;
    RwFileFunction write;
    RwFileFunction gets;
    RwFileFunction puts;
    RwFileFunction eof;
    RwFileFunction seek;
    RwFileFunction flush;
    RwFileFunction tell;
    RwFileFunction exists;
} RwFileInterface;

extern RwFileInterface* RwOsGetFileInterface(void);
extern int strcmp(const char* left, const char* right);
static int renderware_fs_not_implemented(void);
static MkFileEntry* ssf_member_open_async_withcallback(
    MkFileInfo* info, MkFileOpenCallback callback, void* user);
static void ssf_member_open_async_callback(void* entry_argument,
                                           MkHwFileRequest* request,
                                           int success);

static int f_loading;
static int ssf_open_linked_callback_attached;
static void* ssf_open_linked_user_data;
static MkFileOpenCallback ssf_open_linked_fn_user_callback;
SsfContext current_ssf;
SsfContext previous_ssf;
int num_files_loaded;

void disable_default_filesystem(void) {
    RwFileInterface* file_interface = RwOsGetFileInterface();

    file_interface->open = renderware_fs_not_implemented;
    file_interface->close = renderware_fs_not_implemented;
    file_interface->read = renderware_fs_not_implemented;
    file_interface->write = renderware_fs_not_implemented;
    file_interface->gets = renderware_fs_not_implemented;
    file_interface->puts = renderware_fs_not_implemented;
    file_interface->eof = renderware_fs_not_implemented;
    file_interface->seek = renderware_fs_not_implemented;
    file_interface->flush = renderware_fs_not_implemented;
    file_interface->tell = renderware_fs_not_implemented;
    file_interface->exists = renderware_fs_not_implemented;
}

static int renderware_fs_not_implemented(void) {
    return 0;
}

MkFileInfo* find_section_by_name(const char* name) {
    MkFileEntry* cursor = current_ssf.ssf_file + 1;
    MkFileInfo* info = cursor->info;

    while (info != 0) {
        if (strcmp(info->name, name) == 0) {
            return info;
        }
        cursor++;
        info = cursor->info;
    }
    return 0;
}

void* mk_file_read_async(void* buffer, int size, int count,
                         MkFileEntry* entry) {
    (void)entry;
    return mk_hwfile_read_async(current_ssf.hwfile,
                                mk_hwfile_tell(current_ssf.hwfile),
                                buffer, size * count);
}

int mk_file_length(MkFileEntry* entry) {
    return entry->size;
}

unsigned int mk_file_read(void* buffer, unsigned int size, unsigned int count,
                          MkFileEntry* entry) {
    (void)entry;
    return mk_hwfile_read(current_ssf.hwfile, buffer, size * count) / size;
}

int mk_file_close(MkFileEntry* entry) {
    (void)entry;
    num_files_loaded++;
    return 0;
}

MkFileEntry* mk_file_open_language(MkFileInfo* info, const char* mode,
                                   void* userdata) {
    int language = get_language();

    return mk_file_open(offset_mk_file_info(info, language), mode, userdata);
}

MkFileEntry* mk_file_open(MkFileInfo* info, const char* mode, void* userdata) {
    return mk_file_open_async_withcallback(info, mode, userdata, 0, 0);
}

MkFileEntry* mk_file_open_async_withcallback(MkFileInfo* info,
                                              const char* mode,
                                              void* userdata,
                                              MkFileOpenCallback callback,
                                              void* user) {
    (void)mode;
    (void)userdata;
    return ssf_member_open_async_withcallback(info, callback, user);
}

static MkFileEntry* ssf_member_open_async_withcallback(
    MkFileInfo* info, MkFileOpenCallback callback, void* user) {
    int open_state = 1;
    MkFileInfo* requested_info = info;
    MkFileEntry* entry = current_ssf.ssf_file + 1;
    MkFileInfo* entry_info;

    while ((entry_info = entry->info) != 0) {
        if (strcmp(entry_info->name, requested_info->name) == 0) {
            break;
        }
        entry++;
    }
    if (entry_info == 0) {
        entry = 0;
    }

    if (mk_hwfile_is_file_ready(current_ssf.hwfile) == 0) {
        if (ssf_open_linked_callback_attached == 0) {
            ssf_open_linked_callback_attached = 1;
            ssf_open_linked_fn_user_callback = callback;
            ssf_open_linked_user_data = user;
            if (mk_hwfile_link_late_open_callback(
                    current_ssf.hwfile, ssf_member_open_async_callback,
                    entry) != 0) {
                open_state = 0;
            } else {
                open_state = 2;
                ssf_open_linked_callback_attached = 0;
            }
        } else {
            open_state = 3;
        }
    }

    if (open_state != 0) {
        mk_hwfile_seek(current_ssf.hwfile, entry->offset, 0);
        if (callback != 0) {
            callback(user, entry, 1);
        }
    }
    return entry;
}

static void ssf_member_open_async_callback(void* entry_argument,
                                           MkHwFileRequest* request,
                                           int success) {
    MkFileEntry* entry = entry_argument;

    (void)request;
    mk_hwfile_seek(current_ssf.hwfile, entry->offset, 0);
    if (ssf_open_linked_fn_user_callback != 0) {
        ssf_open_linked_fn_user_callback(ssf_open_linked_user_data, entry,
                                         success);
    }
    ssf_open_linked_callback_attached = 0;
}

void init_file_loading_table(void) {
    f_loading = 0;
    num_files_loaded = 0;
}

MkFileInfo* offset_mk_file_info(MkFileInfo* info, int language) {
    MkFileEntry* cursor;
    MkFileEntry* ssf_file = current_ssf.ssf_file;
    int index = 0;

    cursor = ssf_file + 1;

    while (cursor->info != 0) {
        if (cursor->info == info) {
            break;
        }
        index++;
        cursor++;
    }
    if (cursor->info == 0) {
        index = 0;
    }
    return ssf_file[index + language + 1].info;
}

int get_ssf_dir_index(MkFileInfo* info) {
    MkFileEntry* cursor = current_ssf.ssf_file + 1;
    int index = 0;

    while (cursor->info != 0) {
        if (cursor->info == info) {
            return index;
        }
        index++;
        cursor++;
    }
    return 0;
}

MkFileInfo* get_mk_file_info_from_current_ssf(int index) {
    return current_ssf.ssf_file[index + 1].info;
}

int num_files_in_ssf(MkFileEntry* table) {
    MkFileEntry* cursor = table + 1;
    int count = 0;

    while (cursor->info != 0) {
        count++;
        cursor++;
    }
    return count;
}

MkFileEntry* get_current_ssf_file(void) {
    return current_ssf.ssf_file;
}

void load_ssf(MkFileEntry* ssf_entry) {
    MkHwFileRequest* hwfile;

    if (current_ssf.ssf_file != 0) {
        sec_slot_file_wait_on_ssf(current_ssf.ssf_file);
        if (current_ssf.hwfile != 0) {
            mk_hwfile_close(current_ssf.hwfile);
            current_ssf.hwfile = 0;
        }
        current_ssf.ssf_file = 0;
    }

    hwfile = mk_hwfile_open(pathname_create(ssf_entry->info->name, 1), "rb");
    current_ssf.ssf_file = ssf_entry;
    current_ssf.hwfile = hwfile;
}

void restore_previous_ssf(void) {
    if (current_ssf.ssf_file != 0) {
        sec_slot_file_wait_on_ssf(current_ssf.ssf_file);
        if (current_ssf.hwfile != 0) {
            mk_hwfile_close(current_ssf.hwfile);
            current_ssf.hwfile = 0;
        }
        current_ssf.ssf_file = 0;
    }
    memcpy(&current_ssf, &previous_ssf, sizeof(current_ssf));
    memset(&previous_ssf, 0, sizeof(previous_ssf));
}

void save_current_ssf(void) {
    memcpy(&previous_ssf, &current_ssf, sizeof(previous_ssf));
    memset(&current_ssf, 0, sizeof(current_ssf));
}

void init_ssf_system(void) {
    memset(&previous_ssf, 0, sizeof(previous_ssf));
    memset(&current_ssf, 0, sizeof(current_ssf));
}
