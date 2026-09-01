#include "platform/gcmcard.h"

#include "game/memcard.h"
#include "game/nbc.h"
#include "game/plyrprofile.h"
#include "platform/gcmcardmsg.h"
#include "platform/gcmcicon.h"
#include "runtime/cstring.h"

/*
 * gcmcard.o - B20 Wave C: Midway platform memcard (CARD facade).
 * NonMatching: ASM still linked for DOL; C for objdiff progress.
 * Function order = retail emission.
 */

#pragma use_lmw_stmw on

extern PlayerProfile p1_profile;

extern void* mc_data_buffer;
extern int mc_data_buffer_size;
extern unsigned int mc_icon_file_size; /* defined unsigned in gcmcicon.c */
extern int f_writing_to_memcard;
extern int mcard_msg_incompatible_card_answer;
extern int msg_another_market_answer;
extern int mcard_msg_wrong_device_answer;
extern int msg_sys_corrupt_answer;
extern int mcard_msg_mu_removed_answer;
extern int msg_crc_failure_answer;
extern int mcard_msg_card_damaged_answer;
extern int msg_no_file_answer;

void gc_mem_card_status_changes_for_one_device(int device);

int mem_card_read(CARDFileInfo* fileInfo, void* buffer, int size);

static void detached_slot_a(s32 chan, s32 result);
static void detached_slot_b(s32 chan, s32 result);
static int gc_get_memcard_serial_number(int device, unsigned int* out);

/* .bss work areas for CARDMount (retail 0xA000 each) + IO buffer 0x8000. */
static unsigned char mc_workArea_0[0xA000];
static unsigned char mc_workArea_1[0xA000];
static unsigned char gc_memcard_io_buffer[0x8000];

/*
 * .sbss (retail ascending): force_insertions, force_removals, gc_seek_position,
 * insertions, removals, last_card_state. MWCC often reverses decl order.
 */
static int last_card_state;
static int removals;
static int insertions;
int gc_seek_position;
int force_removals;
int force_insertions;

/* Retail .data: per-slot serial cache (2 devices x 2 words). */
static unsigned int last_card_serial_no[2][2] = {{0, 0}, {0, 0}};

/*
 * .sdata2: per-slot bit masks (Slot A = 1, Slot B = 2); const keeps it there.
 * TODO(enum): slot masks (and the device indices used into mcmasks and the
 * per-device tables) likely become an enum once semantics are settled; keep
 * const so placement stays .sdata2.
 */
const int mcmasks[2] = {1, 2};

const char* get_device_reference_name(int device) {
    const char* name = "";

    if (device < 0 || device >= 2) {
        return name;
    }
    return nbc_find_text(gc_mc_default_name[device], 0);
}

int bad_load_region_data_result_resolution(int* result, int device) {
    switch (*result) {
    case 0:
        return 1;
    default:
        quit_from_konquest();
        return 1;
    }
}

int check_load_region_data_result(int* result, int device, int scratch, int flag) {
    int cont = 1;

    (void)scratch;
    DEVICE_AT(device)->status = *result;
    switch (*result) {
    case 0:
        storage_status_change_calculations(device);
        cont = 1;
        break;
    case 6:
        if (flag != 0) {
            mcard_msg_profile_damaged_in_konquest();
            quit_from_konquest();
        } else {
            region_data_corruption_message_handler();
        }
        /* Fall through to the removable-storage prompt. */
    case 1:
    case 2:
    case 5:
    case 7:
        mcard_msg_load_no_card_konq_region_hault(p1_profile.name, 0, device);
        if (msg_load_no_card_konq_region_hault_answer == 2) {
            quit_from_konquest();
            return 0;
        }
        break;
    case -1:
    case 3:
    case 4:
    case 8:
    case 9:
    case 10:
    case 11:
    default:
        region_data_corruption_message_handler();
        break;
    }
    summarize_unlocked_items();
    DEVICE_AT(device)->status = *result;
    check_new_mu_for_in_use_profiles(device);
    return cont;
}

int bad_save_region_data_result_resolution(int* result, int device) {
    switch (*result) {
    case 0:
        return 1;
    default:
        quit_from_konquest();
        return 1;
    }
}

int check_save_region_data_result(int* result, int device, int mode) {
    int cont;

    switch (*result) {
    case 0:
        storage_status_change_calculations(device);
        cont = 1;
        break;
    case 1:
    case 2:
    case 5:
    case 7:
        mcard_msg_save_no_card_konq_region_hault(p1_profile.name, 0);
        if (msg_save_no_card_konq_region_hault_answer == 1) {
            cont = 0;
        } else {
            mcard_msg_end();
            if (mode == 5) {
                cont = 1;
            } else {
                mcard_msg_quit_confirmation();
                if (msg_quit_confirmation_answer == 1)
                    cont = 1;
                else
                    cont = 0;
            }
        }
        break;
    case -1:
    case 3:
    case 4:
    case 6:
    case 8:
    case 9:
    case 10:
    case 11:
    default:
        mcard_msg_save_error_konq_region(device);
        if (msg_save_error_konq_region_answer == 1) {
            cont = 0;
        } else {
            mcard_msg_end();
            mcard_msg_quit_confirmation();
            if (msg_quit_confirmation_answer == 1)
                cont = 1;
            else
                cont = 0;
        }
        break;
    }
    summarize_unlocked_items();
    return cont;
}

int check_load_profile_result(int* result, int device) {
    StorageDevice* dev;
    int cont;
    int answer;
    const char* name;
    int status;

    dev = DEVICE_AT(device);
    status = *result;
    dev->status = status;

    switch (status) {
    case 8:
        reset_storage_device_status_structure(device);
        strcpy(dev->name, "");
        DEVICE_AT(device)->freeBlocks = 0;
        name = nbc_find_text(0x70, 0);
        mcard_msg_incompatible_card(name, device);
        if (mcard_msg_incompatible_card_answer == 2)
            cont = 0;
        else
            cont = 1;
        break;
    case 9:
        reset_storage_device_status_structure(device);
        strcpy(dev->name, "");
        DEVICE_AT(device)->freeBlocks = 0;
        name = nbc_find_text(0x70, 0);
        mcard_msg_another_market(name, device);
        answer = msg_another_market_answer;
        switch (answer) {
        case 2:
            cont = 0;
            break;
        case 3:
            gc_format_procedure(device);
            cont = 0;
            break;
        default:
            cont = 1;
            break;
        }
        break;
    case 10:
        reset_storage_device_status_structure(device);
        strcpy(dev->name, "");
        DEVICE_AT(device)->freeBlocks = 0;
        name = nbc_find_text(0x70, 0);
        mcard_msg_wrong_device(name, device);
        if (mcard_msg_wrong_device_answer == 2)
            cont = 0;
        else
            cont = 1;
        break;
    case 11:
        reset_storage_device_status_structure(device);
        strcpy(dev->name, "");
        DEVICE_AT(device)->freeBlocks = 0;
        name = nbc_find_text(0x70, 0);
        mcard_msg_sys_corrupt(name, device);
        answer = msg_sys_corrupt_answer;
        switch (answer) {
        case 2:
            cont = 0;
            break;
        case 3:
            gc_format_procedure(device);
            cont = 0;
            break;
        default:
            cont = 1;
            break;
        }
        break;
    case 0:
        storage_status_change_calculations(device);
        cont = 1;
        break;
    case 1:
        reset_storage_device_status_structure(device);
        strcpy(dev->name, "");
        DEVICE_AT(device)->freeBlocks = 0;
        name = nbc_find_text(0x70, 0);
        mcard_msg_mu_removed(name, device);
        if (mcard_msg_mu_removed_answer == 1)
            cont = 1;
        else
            cont = 0;
        break;
    case 2:
        if (is_storage_device_full(device) != 0) {
            reset_storage_device_status_structure(device);
            strcpy(dev->name, "");
            *result = 5;
            dev->status = 5;
            name = nbc_find_text(0x70, 0);
            cont = gc_no_space_routine(name, device) == 0;
        } else {
            reset_storage_device_status_structure(device);
            cont = 1;
        }
        break;
    case 5:
        reset_storage_device_status_structure(device);
        strcpy(dev->name, "");
        name = nbc_find_text(0x70, 0);
        cont = gc_no_space_routine(name, device) == 0;
        break;
    case 6:
        reset_storage_device_status_structure(device);
        strcpy(dev->name, "");
        DEVICE_AT(device)->freeBlocks = 0;
        name = nbc_find_text(0x70, 0);
        mcard_msg_crc_failure(name, device);
        answer = msg_crc_failure_answer;
        switch (answer) {
        case 2:
            cont = 0;
            break;
        case 3:
            f_writing_to_memcard = 1;
            mcard_msg_deleting_file(device);
            if (gc_delete_file(device, "MKD") != 0) {
                mcard_msg_delete_successful(device);
                cont = 0;
                f_writing_to_memcard = 0;
            } else {
                mcard_msg_delete_failed(device);
                cont = 1;
                f_writing_to_memcard = 0;
            }
            break;
        default:
            cont = 1;
            break;
        }
        break;
    case 4:
        name = nbc_find_text(0x70, 0);
        mcard_msg_card_damaged(name, device);
        if (mcard_msg_card_damaged_answer == 1)
            cont = 1;
        else
            cont = 0;
        break;
    case 3:
    case 7:
    default:
        reset_storage_device_status_structure(device);
        strcpy(dev->name, "");
        cont = 1;
        DEVICE_AT(device)->freeBlocks = 0;
        break;
    }

    summarize_unlocked_items();
    DEVICE_AT(device)->status = *result;
    check_new_mu_for_in_use_profiles(device);
    return cont;
}

int check_save_profile_result(int* result, int device, int flag) {
    StorageDevice* dev;
    int cont;
    int answer;
    const char* name;
    int status;

    dev = DEVICE_AT(device);
    status = *result;
    dev->status = status;
    (void)flag;

    switch (status) {
    case 8:
        name = nbc_find_text(0x70, 0);
        mcard_msg_incompatible_card(name, device);
        if (mcard_msg_incompatible_card_answer == 2)
            cont = 0;
        else
            cont = 1;
        break;
    case 9:
        name = nbc_find_text(0x70, 0);
        mcard_msg_another_market(name, device);
        answer = msg_another_market_answer;
        switch (answer) {
        case 2:
            cont = 0;
            break;
        case 3:
            gc_format_procedure(device);
            cont = 0;
            break;
        default:
            cont = 1;
            break;
        }
        break;
    case 10:
        name = nbc_find_text(0x70, 0);
        mcard_msg_wrong_device(name, device);
        if (mcard_msg_wrong_device_answer == 2)
            cont = 0;
        else
            cont = 1;
        break;
    case 11:
        name = nbc_find_text(0x70, 0);
        mcard_msg_sys_corrupt(name, device);
        answer = msg_sys_corrupt_answer;
        switch (answer) {
        case 2:
            cont = 0;
            break;
        case 3:
            gc_format_procedure(device);
            cont = 0;
            break;
        default:
            cont = 1;
            break;
        }
        break;
    case 0:
        storage_status_change_calculations(device);
        cont = 1;
        break;
    case 1:
        name = nbc_find_text(0x70, 0);
        mcard_msg_mu_removed(name, device);
        if (mcard_msg_mu_removed_answer == 1)
            cont = 1;
        else
            cont = 0;
        break;
    case 2:
        if (is_storage_device_full(device) != 0) {
            *result = 5;
            DEVICE_AT(device)->status = 5;
            name = nbc_find_text(0x70, 0);
            cont = gc_no_space_routine(name, device) == 0;
        } else {
            mcard_msg_no_file(device);
            if (msg_no_file_answer == 1) {
                create_new_mk5_profile_file(device);
                cont = 0;
            } else {
                cont = 1;
            }
        }
        break;
    case 5:
        name = nbc_find_text(0x70, 0);
        cont = gc_no_space_routine(name, device) == 0;
        break;
    case 4:
        name = nbc_find_text(0x70, 0);
        mcard_msg_card_damaged(name, device);
        if (mcard_msg_card_damaged_answer == 1)
            cont = 1;
        else
            cont = 0;
        break;
    case 3:
    case 6:
    case 7:
    default:
        cont = 1;
        break;
    }

    summarize_unlocked_items();
    DEVICE_AT(device)->status = *result;
    return cont;
}

int format_card_and_create_mkda_file(int device) {
    int result;

    result = gc_format_procedure(device);
    if (result != 0) {
        reset_storage_device_status_structure(device);
        DEVICE_AT(device)->status = 2;
    }
    if (result != 0) {
        result = create_new_mk5_profile_file(device);
    }
    mcard_msg_end();
    return result;
}

static inline int gc_mount_checked(int device) {
    unsigned char* work;
    CARDCallback detach;
    s32 rc;

    if (device < 0 || device >= 2)
        return -99;
    switch (device) {
    case 0:
        work = mc_workArea_0;
        detach = detached_slot_a;
        break;
    case 1:
        work = mc_workArea_1;
        detach = detached_slot_b;
        break;
    default:
        return -99;
    }
    do {
        rc = CARDMount(device, work, detach);
    } while (rc == -1);
    if (rc == 0 || rc == -6) {
        do {
            rc = CARDCheck(device);
        } while (rc == -1);
    }
    if (rc == 0)
        return 0;
    if (rc == -3)
        return -10;
    if (rc == -2)
        return -0x32;
    if (rc == -6)
        return -0x33;
    if (rc == -13)
        return -0x34;
    return -99;
}

static inline int gc_unmount_checked(int device) {
    s32 rc;

    if (device < 0 || device >= 2)
        return -99;
    do {
        rc = CARDUnmount(device);
    } while (rc == -1);
    switch (rc) {
    case 0:
        return 0;
    case -3:
        return -10;
    case -128:
    default:
        return -99;
    }
}

int gc_format_procedure(int device) {
    unsigned int mask;
    s32 sectorSize;
    s32 rc;
    int chan;
    int cardChanged;
    int confirmed;
    int formatted;
    int i;
    unsigned char* workArea;
    CARDCallback detach;

    mask = (unsigned int)mcmasks[device];
    last_card_state |= (int)mask;
    removals &= (int)~mask;
    insertions &= (int)~mask;
    sectorSize = 0;
    confirmed = 0;
    formatted = 0;

    while (!confirmed) {
        mcard_msg_format_confirmation(device);
        if (msg_format_confirmation_answer != 1) {
            return 0;
        }
        confirmed = 1;
    }

    while (!formatted) {
        f_writing_to_memcard = 1;
        mcard_msg_formating(device);
        do {
            rc = CARDProbeEx(device, 0, &sectorSize);
        } while (rc == -1);

        for (i = 0; i < 2; i++) {
            gc_mem_card_status_changes_for_one_device(i);
        }

        cardChanged = 0;
        if ((removals & (int)mask) != 0 || (insertions & (int)mask) != 0) {
            cardChanged = 1;
        }
        if (cardChanged) {
            mcard_msg_card_changed_at_format(device);
            if (mcard_msg_card_changed_at_format_answer == 2) {
                f_writing_to_memcard = 0;
                return 0;
            }
            do {
                rc = CARDProbeEx(device, 0, &sectorSize);
            } while (rc == -1);
            for (i = 0; i < 2; i++) {
                gc_mem_card_status_changes_for_one_device(i);
            }
            mcard_msg_formating(device);
        }

        if (rc != 0) {
            f_writing_to_memcard = 0;
            return 0;
        }
        if (sectorSize != 0x2000) {
            f_writing_to_memcard = 0;
            return 0;
        }

        if (device < 0 || device > 1) {
            rc = -99;
        } else {
            if (device == 0) {
                workArea = mc_workArea_0;
                detach = detached_slot_a;
            } else {
                workArea = mc_workArea_1;
                detach = detached_slot_b;
            }
            do {
                rc = CARDMount(device, workArea, detach);
            } while (rc == -1);
            if (rc == 0 || rc == -6) {
                do {
                    rc = CARDCheck(device);
                } while (rc == -1);
            }
            if (rc == -5) {
                rc = -99;
            } else if (rc < -5) {
                if (rc == -13)
                    rc = -0x34;
                else if (rc < -13 || rc < -6)
                    rc = -99;
                else
                    rc = -0x33;
            } else if (rc == -1) {
                rc = -99;
            } else if (rc < -1) {
                if (rc == -3)
                    rc = -10;
                else if (rc < -3)
                    rc = -99;
                else
                    rc = -0x32;
            } else if (rc > 0) {
                rc = -99;
            } else {
                rc = 0;
            }
        }

        /* Accept 0 or Midway -0x34 (card needs format). */
        if (rc != 0 && (unsigned int)(rc + 0x34) > 1) {
            f_writing_to_memcard = 0;
            return 0;
        }

        do {
            rc = CARDFormat(device);
        } while (rc == -1);

        /* Map CARDFormat result: 0 ok, -3 -> -10, else -99. */
        if (rc == -4 || rc < -4) {
            chan = -99;
        } else if (rc == 0) {
            chan = 0;
        } else if (rc == -3) {
            chan = -10;
        } else {
            chan = -99;
        }

        if (chan == 0) {
            formatted = 1;
        } else {
            mcard_msg_format_failed(device);
            if (msg_format_failed_answer == 2) {
                f_writing_to_memcard = 0;
                return 0;
            }
        }
    }

    if (device < 0 || device > 1) {
        rc = -99;
    } else {
        do {
            rc = CARDUnmount(device);
        } while (rc == -1);
        if (rc == -3)
            rc = -10;
        else if (rc < -3 || rc != 0)
            rc = -99;
        else
            rc = 0;
    }
    if (rc != 0) {
        f_writing_to_memcard = 0;
        return 0;
    }
    mcard_msg_format_successful(device);
    f_writing_to_memcard = 0;
    return 1;
}

int gc_delete_file(int device, const char* fileName) {
    s32 rc;
    int mapped;

    if (device < 0 || device >= 2) {
        return 0;
    }

    mapped = gc_mount_checked(device);
    if (mapped == 0) {
        do {
            rc = CARDDelete(device, fileName);
        } while (rc == -1);
        if (rc == 0) {
            mapped = gc_unmount_checked(device);
            if (mapped != 0)
                mapped = 4;
            if (mapped == 0)
                return 1;
            return 0;
        }
    }

    if (device >= 0 && device < 2) {
        do {
            rc = CARDUnmount(device);
        } while (rc == -1);
    }
    return 0;
}

static inline int finish_memcard_load_after_unmount(int device, int result) {
    s32 rc;

    if (device >= 0 && device < 2) {
        do {
            rc = CARDUnmount(device);
        } while (rc == -1);
    }
    return result;
}

static inline int finish_memcard_load_after_close(int device, CARDFileInfo* fileInfo, int result) {
    s32 rc;

    gc_seek_position = 0;
    do {
        rc = CARDClose(fileInfo);
    } while (rc == -1);
    return finish_memcard_load_after_unmount(device, result);
}

int load_from_memcard2(int device, int modeFlag, unsigned int offset, const char* unusedStr,
                       const char* fileName, void* buffer, int size, const char* unusedCardName,
                       int unusedNameLen, unsigned int* freeBlocks, int* freeBytes,
                       int* checksumFailOut) {
    s32 sectorSize;
    s32 rc;
    int status;
    int result;
    CARDFileInfo fileInfo;
    unsigned char* walk;
    unsigned int checksumLength;
    unsigned int remaining;
    unsigned char bit;
    int sum;
    int storedChecksum;

    sectorSize = 0;
    *freeBlocks = 0;
    *freeBytes = 0;
    *checksumFailOut = 0;
    gc_seek_position = 0;

    if (device < 0 || device >= 2)
        return 4;
    if (modeFlag != 0)
        return 4;

    do {
        rc = CARDProbeEx(device, 0, &sectorSize);
    } while (rc == -1);

    switch (rc) {
    case -3:
        return 1;
    case -2:
        return 10;
    default:
        return 4;
    case 0:
        break;
    }
    if (sectorSize != 0x2000)
        return 8;

    status = gc_mount_checked(device);
    switch (status) {
    case -10:
        return 1;
    case -0x32:
        return 10;
    case -0x33:
        return finish_memcard_load_after_unmount(device, 0xb);
    case -0x34:
        return finish_memcard_load_after_unmount(device, 9);
    default:
        return 4;
    case 0:
        break;
    }

    do {
        rc = CARDFreeBlocks(device, (s32*)freeBlocks, (s32*)freeBytes);
    } while (rc == -1);
    if (rc != 0) {
        *freeBlocks = 0;
        if (rc == -3)
            result = 1;
        else
            result = 4;
        return finish_memcard_load_after_unmount(device, result);
    }
    *freeBlocks = (*freeBlocks + 0x1FFF) >> 13;

    if (device < 0 || device >= 2)
        status = -99;
    else {
        do {
            rc = CARDOpen(device, fileName, &fileInfo);
        } while (rc == -1);
        if (rc == 0)
            status = 0;
        else if (rc == -3)
            status = -10;
        else if (rc == -4)
            status = -4;
        else
            status = -99;
    }
    if (status != 0) {
        if (status == -2)
            result = 7;
        else if (status == -6)
            result = 5;
        else if (status == -4)
            result = 2;
        else if (status == -10)
            result = 1;
        else
            result = 4;
        return finish_memcard_load_after_unmount(device, result);
    }

    if (offset >= 0x28B8U)
        gc_seek_position = (int)((offset - 0x28B8U) / 0x1F54U) + 2;
    else
        gc_seek_position = 0;

    rc = mem_card_read(&fileInfo, buffer, size);
    if (rc != 0) {
        if (rc == -2)
            result = 7;
        else if (rc == -10)
            result = 1;
        else if (rc == -4)
            result = 2;
        else if (rc == -0x35)
            result = 6;
        else
            result = 4;
        return finish_memcard_load_after_close(device, &fileInfo, result);
    }

    gc_seek_position = 0;
    do {
        rc = CARDClose(&fileInfo);
    } while (rc == -1);
    if (rc == 0)
        status = 0;
    else if (rc == -3)
        status = -10;
    else
        status = -99;
    if (status != 0) {
        if (status == -2)
            result = 7;
        else if (status == -10)
            result = 1;
        else if (status == -4)
            result = 2;
        else
            result = 4;
        return finish_memcard_load_after_unmount(device, result);
    }

    checksumLength = (unsigned int)size - 4;
    remaining = checksumLength;
    walk = (unsigned char*)buffer;
    bit = 0x80;
    sum = 0;
    while (remaining > 0) {
        sum += (unsigned char)(*walk++ | bit);
        bit >>= 1;
        if (bit == 0)
            bit = 0x80;
        remaining--;
    }
    storedChecksum = sum;
    if (compare_checksums((char*)buffer + checksumLength,
                          (const char*)&storedChecksum) == 0) {
        *checksumFailOut = 1;
        return finish_memcard_load_after_unmount(device, 6);
    }

    status = gc_unmount_checked(device);
    if (status == 0)
        return 0;
    if (status == -10)
        result = 1;
    else
        result = 4;
    return finish_memcard_load_after_unmount(device, result);
}

static inline int finish_memcard_save_after_unmount(int device, int result) {
    s32 rc;

    if (device >= 0 && device < 2) {
        do {
            rc = CARDUnmount(device);
        } while (rc == -1);
    }
    unload_memorycard_write_buffer();
    return result;
}

static inline int finish_memcard_save_after_close(int device, CARDFileInfo* fileInfo, int result) {
    s32 rc;

    gc_seek_position = 0;
    do {
        rc = CARDClose(fileInfo);
    } while (rc == -1);
    return finish_memcard_save_after_unmount(device, result);
}

int save_to_memcard2(int device, int modeFlag, unsigned int offset, int createFlag,
                     const char* unusedStr, const char* fileName, void* buffer, int size,
                     unsigned int* freeBlocks,
                     int* freeBytes, int skipChecksum, int unused0, int unusedMode, int unused1) {
    s32 sectorSize;
    s32 rc;
    int status;
    int result;
    int i;
    int checksum;
    unsigned int n;
    unsigned int bit;
    unsigned char* walk;
    unsigned char* work;
    CARDCallback detach;
    CARDFileInfo fileInfo;
    int writeLen;
    int bufSize;
    void* srcBuf;
    sectorSize = 0;
    *freeBytes = 0;
    gc_seek_position = 0;
    result = 4;

    if (device < 0 || device >= 2) {
        return 0;
    }
    if (modeFlag != 0) {
        return 0;
    }

    checksum = 0;
    if (skipChecksum == 0) {
        n = size - 4;
        walk = (unsigned char*)buffer;
        bit = 0x80;
        while (n != 0) {
            checksum += (int)((unsigned char)(*walk | (unsigned char)bit));
            walk += 1;
            bit >>= 1;
            if (bit == 0) {
                bit = 0x80;
            }
            n -= 1;
        }
    }
    *(int*)((unsigned char*)buffer + size - 4) = checksum;

    do {
        rc = CARDProbeEx(device, 0, &sectorSize);
    } while (rc == -1);

    for (i = 0; i < 2; i++) {
        gc_mem_card_status_changes_for_one_device(i);
    }

    if (rc != 0) {
        if (rc == -3) {
            return 1;
        }
        if (rc < -3) {
            return 4;
        }
        if (rc >= -1) {
            return 4;
        }
        return 10;
    }
    if (sectorSize != 0x2000) {
        return 8;
    }

    if (offset >= 0x28B8U) {
        gc_seek_position = (int)((offset - 0x28B8U) / 0x1F54U) + 2;
    } else {
        gc_seek_position = 0;
    }

    if (device < 0 || device >= 2) {
        status = -99;
    } else {
        if (device == 0) {
            work = mc_workArea_0;
            detach = detached_slot_a;
        } else {
            work = mc_workArea_1;
            detach = detached_slot_b;
        }
        do {
            rc = CARDMount(device, work, detach);
        } while (rc == -1);
        if (rc == 0 || rc == -6) {
            do {
                rc = CARDCheck(device);
            } while (rc == -1);
        }
        if (rc == -5) {
            status = -99;
        } else if (rc < -5) {
            if (rc == -13) {
                status = -0x34;
            } else if (rc >= -6) {
                status = -0x33;
            } else {
                status = -99;
            }
        } else if (rc == -1) {
            status = -99;
        } else if (rc < -1) {
            if (rc == -3) {
                status = -10;
            } else if (rc >= -3) {
                status = -0x32;
            } else {
                status = -99;
            }
        } else if (rc > 0) {
            status = -99;
        } else {
            status = 0;
        }
    }

    if (status != 0) {
        if (status == -10)
            return 1;
        if (status == -0x32)
            return 10;
        if (status == -0x33)
            result = 0xb;
        else if (status == -0x34)
            result = 9;
        else
            return 4;
        if (device >= 0 && device < 2) {
            do {
                rc = CARDUnmount(device);
            } while (rc == -1);
        }
        return result;
    }

    *freeBlocks = 0;
    do {
        rc = CARDFreeBlocks(device, (s32*)freeBlocks, (s32*)freeBytes);
    } while (rc == -1);
    if (rc != 0) {
        *freeBlocks = 0;
        if (rc == -6)
            result = 0xb;
        else if (rc == -3)
            result = 1;
        else
            result = 4;
        return finish_memcard_save_after_close(device, &fileInfo, result);
    }
    *freeBlocks = (*freeBlocks + 0x1FFF) >> 13;

    if (create_memorycard_write_buffer(buffer, size) == 0)
        return finish_memcard_save_after_unmount(device, 4);

    if (createFlag != 0) {
        do {
            rc = CARDCreate(device, fileName, 0x74000, &fileInfo);
        } while (rc == -1);
        if (rc != 0) {
            if (rc == -7) {
                result = 6;
            } else if (rc == -8 || rc == -9) {
                result = 5;
            } else if (rc == -3) {
                result = 0;
            } else if (rc == -2) {
                result = 7;
            } else {
                if (device >= 0 && device < 2) {
                    last_card_serial_no[device][1] = 0;
                    last_card_serial_no[device][0] = 0;
                }
                result = 4;
            }
            return finish_memcard_save_after_unmount(device, result);
        }
        do {
            rc = CARDSetAttributes(fileInfo.chan, fileInfo.fileNo, 0xc);
        } while (rc == -1);
        if (rc != 0) {
            if (rc == -3)
                result = 1;
            else if (rc == -4)
                result = 2;
            else
                result = 4;
            return finish_memcard_save_after_unmount(device, result);
        }
    } else {
        if (device < 0 || device >= 2)
            return finish_memcard_save_after_unmount(device, 4);
        do {
            rc = CARDOpen(device, fileName, &fileInfo);
        } while (rc == -1);
        if (rc == 0)
            status = 0;
        else if (rc == -3)
            status = -10;
        else if (rc == -4)
            status = -4;
        else
            status = -99;
        if (status != 0) {
            if (status == -10)
                result = 1;
            else if (status == -4)
                result = 2;
            else
                result = 4;
            return finish_memcard_save_after_unmount(device, result);
        }
    }

    bufSize = mc_data_buffer_size;
    srcBuf = mc_data_buffer;
    writeLen = ((bufSize + 0x1FFF) / 0x2000) * 0x2000;
    if ((unsigned int)writeLen > sizeof(gc_memcard_io_buffer)) {
        status = -99;
    } else {
        memcpy(gc_memcard_io_buffer, srcBuf, (unsigned long)bufSize);
        if (writeLen - bufSize > 0)
            memset((unsigned char*)srcBuf + bufSize, 0, writeLen - bufSize);
        do {
            rc = CARDWrite(&fileInfo, gc_memcard_io_buffer, writeLen, gc_seek_position << 13);
        } while (rc == -1);
        if (rc == 0)
            status = 0;
        else if (rc == -3)
            status = -10;
        else if (rc == -4)
            status = -4;
        else
            status = -99;
    }
    if (status != 0) {
        if (status == -10)
            result = 1;
        else if (status == -4)
            result = 2;
        else
            result = 4;
        return finish_memcard_save_after_close(device, &fileInfo, result);
    }
    if (update_memory_card_status(&fileInfo) == 0)
        return finish_memcard_save_after_close(device, &fileInfo, 4);

    *freeBlocks = 0;
    do {
        rc = CARDFreeBlocks(device, (s32*)freeBlocks, (s32*)freeBytes);
    } while (rc == -1);
    if (rc != 0) {
        *freeBlocks = 0;
        if (rc == -6)
            result = 0xb;
        else if (rc == -3)
            result = 1;
        else
            result = 4;
        return finish_memcard_save_after_close(device, &fileInfo, result);
    }
    *freeBlocks = (*freeBlocks + 0x1FFF) >> 13;

    gc_seek_position = 0;
    do {
        rc = CARDClose(&fileInfo);
    } while (rc == -1);
    if (rc == -3)
        status = -10;
    else if (rc == 0)
        status = 0;
    else
        status = -99;
    if (status != 0) {
        result = status == -10 ? 1 : 4;
        return finish_memcard_save_after_unmount(device, result);
    }

    if (device < 0 || device >= 2)
        return finish_memcard_save_after_unmount(device, 4);
    do {
        rc = CARDUnmount(device);
    } while (rc == -1);
    if (rc == -3)
        status = -10;
    else if (rc == 0)
        status = 0;
    else
        status = -99;
    if (status != 0) {
        result = status == -10 ? 1 : 4;
        unload_memorycard_write_buffer();
        return result;
    }
    unload_memorycard_write_buffer();
    return 0;
}

static int gc_get_memcard_serial_number(int device, unsigned int* out) {
    s32 rc;
    s32 serialRc;
    u64 serial;
    unsigned char* workArea;
    CARDCallback detach;
    int mapped;
    int readSerial;

    out[0] = 0;
    out[1] = 0;
    serial = 0;

    if (device < 0 || device > 1) {
        mapped = -99;
    } else {
        if (device == 0) {
            workArea = mc_workArea_0;
            detach = detached_slot_a;
        } else {
            workArea = mc_workArea_1;
            detach = detached_slot_b;
        }
        do {
            rc = CARDMount(device, workArea, detach);
        } while (rc == -1);

        if (rc == -5) {
            mapped = -99;
        } else if (rc < -5) {
            if (rc == -13)
                mapped = -0x34;
            else if (rc < -13 || rc < -6)
                mapped = -99;
            else
                mapped = -0x33;
        } else if (rc == -1) {
            mapped = -99;
        } else if (rc < -1) {
            if (rc == -3)
                mapped = -10;
            else if (rc < -3)
                mapped = -99;
            else
                mapped = -0x32;
        } else if (rc > 0) {
            mapped = -99;
        } else {
            mapped = 0;
        }
    }

    readSerial = 0;
    switch (mapped) {
    case 0:
        mapped = 0;
        readSerial = 1;
        break;
    case -10:
        mapped = -3;
        break;
    case -0x32:
        mapped = -2;
        break;
    case -0x34:
        mapped = -0xd;
        readSerial = 1;
        break;
    case -0x33:
        mapped = -6;
        readSerial = 1;
        break;
    case -99:
    default:
        mapped = -0x80;
        break;
    }

    if (readSerial) {
        do {
            serialRc = CARDGetSerialNo(device, &serial);
        } while (serialRc == -1);
        if (serialRc != 0) {
            out[0] = 0;
            out[1] = 0;
            mapped = (int)serialRc;
        } else {
            out[0] = (unsigned int)(serial >> 32);
            out[1] = (unsigned int)serial;
        }
        if (device >= 0 && device < 2) {
            do {
                serialRc = CARDUnmount(device);
            } while (serialRc == -1);
        }
    }
    return mapped;
}

void gc_mem_card_status_changes_for_one_device(int device) {
    int serialRc;
    unsigned int serial[2];

    serial[1] = 0;
    serial[0] = 0;
    do {
    } while (CARDProbeEx(device, 0, 0) == -1);

    serialRc = gc_get_memcard_serial_number(device, serial);

    switch (serialRc) {
    case 0: {
        unsigned int mask = (unsigned int)mcmasks[device];
        if ((last_card_state & (int)mask) != 0) {
            if (((serial[1] ^ last_card_serial_no[device][1]) |
                 (serial[0] ^ last_card_serial_no[device][0])) != 0) {
                last_card_serial_no[device][1] = serial[1];
                removals |= (int)mask;
                insertions |= (int)mask;
                last_card_state |= (int)mask;
                last_card_serial_no[device][0] = serial[0];
            }
            return;
        }
        last_card_serial_no[device][1] = serial[1];
        insertions |= (int)mask;
        last_card_state |= (int)mask;
        last_card_serial_no[device][0] = serial[0];
        return;
    }
    case -6:
    case -2:
    case -0xd:
    case -0x80: {
        unsigned int mask = (unsigned int)mcmasks[device];
        if ((last_card_state & (int)mask) == 0) {
            last_card_state |= (int)mask;
            insertions |= (int)mask;
        }
        last_card_serial_no[device][1] = 0;
        last_card_serial_no[device][0] = 0;
        return;
    }
    case -3: {
        unsigned int mask = (unsigned int)mcmasks[device];
        insertions &= (int)~mask;
        if ((last_card_state & (int)mask) != 0) {
            last_card_state &= (int)~mask;
            removals |= (int)mask;
        }
        last_card_serial_no[device][1] = 0;
        last_card_serial_no[device][0] = 0;
        return;
    }
    default:
        last_card_serial_no[device][1] = 0;
        last_card_serial_no[device][0] = 0;
        break;
    }
}

/*
 * Soft ceiling: ~94% -- retail homes device in r29 / changed in r28, MWCC
 * colors them the other way; ops and scheduling otherwise exact. Stop.
 */
int update_storage_status_for_one_device(int device) {
    int changed = 0;

    if (device < 0 || device >= 2) {
        return changed;
    }
    gc_mem_card_status_changes_for_one_device(device);
    if ((removals & mcmasks[device]) || (force_removals & mcmasks[device])) {
        reset_ppwls_timeout();
        removals &= ~mcmasks[device];
        remove_mu(device, 0, device);
        changed = 1;
        force_removals &= ~mcmasks[device];
    }
    if ((insertions & mcmasks[device]) || (force_insertions & mcmasks[device])) {
        reset_ppwls_timeout();
        insertions &= ~mcmasks[device];
        changed = 1;
        insert_mu(device, 0, device);
        force_insertions &= ~mcmasks[device];
    }
    return changed;
}

/*
 * Retail inlines the helper above (-inline auto, same TU): the emitted loop
 * body carries the helper's redundant 0..1 range check and its exact changed=1
 * scheduling, so the source is this call, not a hand-expanded copy.
 * Soft ceiling: ~94% -- same r28/r29 coloring swap propagated by the inline.
 */
int update_storage_status(int flag) {
    int any = 0;
    int device;

    for (device = 0; device < 2; device++) {
        if (update_storage_status_for_one_device(device)) {
            any = 1;
        }
    }
    return any;
}

/*
 * Soft ceiling: ~94% -- MWCC roots the CARD-result compare tree at case -4
 * where retail roots at the -9 boundary; same case set, unknown root-selection
 * heuristic. Header checks, rounding, reads, and copies are exact. Stop.
 */
int mem_card_read(CARDFileInfo* fileInfo, void* buffer, int size) {
    CARDStat stat;
    int readLen;
    int result;

    do {
        result = CARDGetStatus(fileInfo->chan, fileInfo->fileNo, &stat);
    } while (result == -1);

    if (stat.iconAddr != 0x40) {
        return -0x35;
    }
    if (stat.commentAddr != 0) {
        return -0x35;
    }

    if (gc_seek_position == 0) {
        if (mc_icon_file_size == 0) {
            return 0;
        }
        /* Signed rounding through the int intermediate: retail emits
         * srawi/addze, which an unsigned one-expression form strength-reduces
         * to clrrwi instead. */
        readLen = mc_icon_file_size + 0x1FF;
        readLen = size + readLen;
        readLen = (readLen / 0x200) * 0x200;
    } else {
        readLen = size + 0x1FF;
        readLen = (readLen / 0x200) * 0x200;
    }
    do {
        result = CARDRead(fileInfo, gc_memcard_io_buffer, readLen,
                          gc_seek_position << 13);
    } while (result == -1);

    switch (result) {
    case 0:
        if (gc_seek_position == 0) {
            memcpy(buffer, gc_memcard_io_buffer + stat.offsetData, size);
        } else {
            memcpy(buffer, gc_memcard_io_buffer, size);
        }
        return 0;
    case -3:
        return -0xA;
    case -4:
        return -0x4;
    case -14:
    case -128:
    default:
        return -0x63;
    }
}

static void detached_slot_b(s32 chan, s32 result) {
    (void)chan;
    (void)result;
}

static void detached_slot_a(s32 chan, s32 result) {
    (void)chan;
    (void)result;
}

int init_gc_memcard(void) {
    CARDInit();
    last_card_state = 0;
    load_icon_data();
    return 1;
}
