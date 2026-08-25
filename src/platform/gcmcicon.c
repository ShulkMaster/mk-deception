#include "platform/gcmcicon.h"

#include "dolphin/card.h"
#include "mw/mwMem.h"
#include "runtime/mk_fileinfo.h"

extern _mwMemHeap* wave_heap;

extern void* memset(void* destination, int value, unsigned long size);
extern void* memcpy(void* destination, const void* source, unsigned long size);
extern char* strcpy(char* destination, const char* source);

extern MkFileEntry nameentryart_file_table[];
extern MkFileInfo sec_title;

CARDStat cardstat;
static unsigned char icon_buffer[0x1040];
char* mc_data_buffer;
unsigned int mc_data_buffer_size;
unsigned int mc_icon_file_size;

void unload_memorycard_write_buffer(void) {
    if (mc_data_buffer != 0) {
        _mwMemFree(mc_data_buffer, 0, 0);
    }
}

int create_memorycard_write_buffer(const void* data, unsigned int size) {
    if (gc_seek_position == 0) {
        if (mc_icon_file_size == 0) {
            return 0;
        }

        mc_data_buffer_size = mc_icon_file_size + size;
        mc_data_buffer_size = (mc_data_buffer_size + 0x1fff) & ~0x1fff;
        mc_data_buffer = _mwMemMalloc(wave_heap, mc_data_buffer_size, 5, 0, 0, 0);
        if (mc_data_buffer == 0) {
            return 0;
        }

        memset(mc_data_buffer, 0, 0x40);
        strcpy(mc_data_buffer, "Mortal Kombat Deception");
        strcpy(mc_data_buffer + 0x20, "profiles and game settings");
        memcpy(mc_data_buffer + 0x40, icon_buffer, mc_icon_file_size);
        memcpy(mc_data_buffer + 0x40, mc_data_buffer + 0x80, mc_icon_file_size - 0x40);
        memcpy(mc_icon_file_size + mc_data_buffer, data, size);
    } else {
        mc_data_buffer_size = size;
        size = (size + 0x1fff) & ~0x1fff;
        mc_data_buffer_size = size;
        mc_data_buffer = _mwMemMalloc(wave_heap, mc_data_buffer_size, 5, 0, 0, 0);
        if (mc_data_buffer == 0) {
            return 0;
        }

        memset(mc_data_buffer, 0, 0x40);
        memcpy(mc_data_buffer, data, size);
    }

    return 1;
}

void load_icon_data(void) {
    MkFileEntry* file;

    load_ssf(nameentryart_file_table);
    file = mk_file_open(&sec_title, "rb", (void*)1);
    if (file == 0) {
        restore_previous_ssf();
        return;
    }

    mc_icon_file_size = mk_file_length(file);
    if (mc_icon_file_size > sizeof(icon_buffer)) {
        restore_previous_ssf();
        return;
    }

    mk_file_read(icon_buffer, 1, mc_icon_file_size, file);
    mk_file_close(file);
}

int update_memory_card_status(const CARDFileInfo* file) {
    int result;
    CARDStat* status;
    long file_no;
    long chan;
    unsigned char banner_format;
    unsigned short icon_speed;

    chan = file->chan;
    file_no = file->fileNo;
    do {
        result = CARDGetStatus(chan, file_no, &cardstat);
    } while (result == -1);

    if (result != 0) {
        return 0;
    }

    status = &cardstat;
    banner_format = status->bannerFormat;
    icon_speed = (status->iconSpeed & ~3) | 3;
    status->bannerFormat = banner_format & ~3;
    status->iconSpeed = icon_speed;
    status->commentAddr = 0;
    status->iconAddr = 0x40;
    status->bannerFormat = banner_format & ~7;
    status->iconFormat = (status->iconFormat & ~3) | 2;
    status->iconSpeed = icon_speed & ~0xc;

    do {
        result = CARDSetStatus(chan, file_no, status);
    } while (result == -1);

    return result == 0;
}
