#include "platform/gcmcicon.h"
#include "runtime/cstring.h"

#include "dolphin/card.h"
#include "mw/mwMem.h"
#include "runtime/mk_fileinfo.h"

extern _mwMemHeap* wave_heap;

extern char* strcpy(char* destination, const char* source);

extern MkFileEntry nameentryart_file_table[];
extern MkFileInfo sec_title;

static unsigned char icon_buffer[0x1040];
CARDStat cardstat;
unsigned int mc_icon_file_size;
int mc_data_buffer_size;
char* mc_data_buffer;
void unload_memorycard_write_buffer(void);
int create_memorycard_write_buffer(const void* data, unsigned int size);
void load_icon_data(void);
int update_memory_card_status(const CARDFileInfo* file);

/*
 * Retail builds this unit with -inline noauto,deferred, which emits functions
 * in reverse source order. The definitions below are therefore in reverse of
 * the retail .text order; that order also fixes the pooled-string layout and
 * the anonymous .rodata initializer order.
 */


int update_memory_card_status(const CARDFileInfo* file) {
    int result;
    long chan;
    long file_no;

    chan = file->chan;
    file_no = file->fileNo;
    do {
        result = CARDGetStatus(chan, file_no, &cardstat);
    } while (result == -1);

    if (result != 0) {
        return 0;
    }

    CARDSetCommentAddress(&cardstat, 0);
    CARDSetIconAddress(&cardstat, 0x40);
    CARDSetBannerFormat(&cardstat, CARD_STAT_BANNER_NONE);
    CARDSetIconAnim(&cardstat, CARD_STAT_ANIM_LOOP);
    CARDSetIconFormat(&cardstat, 0, CARD_STAT_ICON_RGB5A3);
    CARDSetIconSpeed(&cardstat, 0, CARD_STAT_SPEED_SLOW);
    CARDSetIconSpeed(&cardstat, 1, CARD_STAT_SPEED_END);

    do {
        result = CARDSetStatus(chan, file_no, &cardstat);
    } while (result == -1);

    return result == 0;
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
        memcpy(mc_data_buffer + 0x40 + (mc_icon_file_size - 0x40), data, size);
    } else {
        mc_data_buffer_size = size;
        mc_data_buffer_size = (mc_data_buffer_size + 0x1fff) & ~0x1fff;
        mc_data_buffer = _mwMemMalloc(wave_heap, mc_data_buffer_size, 5, 0, 0, 0);
        if (mc_data_buffer == 0) {
            return 0;
        }

        memset(mc_data_buffer, 0, 0x40);
        memcpy(mc_data_buffer, data, size);
    }

    return 1;
}

void unload_memorycard_write_buffer(void) {
    if (mc_data_buffer != 0) {
        _mwMemFree(mc_data_buffer, 0, 0);
    }
}
