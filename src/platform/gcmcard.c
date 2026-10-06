#include "platform/gcmcard.h"

#include "game/memcard.h"
#include "game/mcardmsg.h"
#include "game/nbc.h"
#include "game/plyrprofile.h"
#include "platform/gcmcardmsg.h"
#include "platform/gcmcicon.h"
#include "runtime/cstring.h"

#pragma use_lmw_stmw on

void gc_mem_card_status_changes_for_one_device(int device);

int mem_card_read(CARDFileInfo* fileInfo, void* buffer, int size);

static void detached_slot_a(s32 chan, s32 result);
static void detached_slot_b(s32 chan, s32 result);
static int gc_get_memcard_serial_number(int device, u64* out);

static unsigned char mc_workArea_0[CARD_WORKAREA_SIZE];
static unsigned char mc_workArea_1[CARD_WORKAREA_SIZE];
static unsigned char gc_memcard_io_buffer[0x8000];

static int last_card_state;
static int removals;
static int insertions;
int gc_seek_position;
int force_removals;
int force_insertions;

static u64 last_card_serial_no[2] = {0, 0};

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

static inline int gc_profile_no_space_result(const char* name, int device)
{
    return gc_no_space_routine(name, device) == 0;
}

static inline int gc_resolve_profile_crc_answer(int device)
{
    int answer = msg_crc_failure_answer;
    int continue_loading;
    switch (answer) {
    case 2:
        continue_loading = 0;
        break;
    case 3:
        f_writing_to_memcard = 1;
        mcard_msg_deleting_file(device);
        if (gc_delete_file(device, "MKD") != 0) {
            mcard_msg_delete_successful(device);
            continue_loading = 0;
            f_writing_to_memcard = 0;
        } else {
            mcard_msg_delete_failed(device);
            continue_loading = 1;
            f_writing_to_memcard = 0;
        }
        break;
    case 1:
    default:
        continue_loading = 1;
        break;
    }
    return continue_loading;
}

/* TODO: [near miss] 98.09689%; answer dispatch restored; device/index coloring and redundant format-result branch remain. */
int check_load_profile_result(int* result, int device) {
    StorageDevice* dev;
    int cont;
    int answer;
    const char* name;

    dev = DEVICE_AT(device);
    dev->status = *result;

    switch (*result) {
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
        case 1:
        default:
            cont = 1;
            break;
        case 2:
            cont = 0;
            break;
        case 3:
            gc_format_procedure(device);
            cont = 0;
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
        case 1:
        default:
            cont = 1;
            break;
        case 2:
            cont = 0;
            break;
        case 3:
            gc_format_procedure(device);
            cont = 0;
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
            cont = gc_profile_no_space_result(name, device);
        } else {
            reset_storage_device_status_structure(device);
            cont = 1;
        }
        break;
    case 5:
        reset_storage_device_status_structure(device);
        strcpy(dev->name, "");
        name = nbc_find_text(0x70, 0);
        cont = gc_profile_no_space_result(name, device);
        break;
    case 6:
        reset_storage_device_status_structure(device);
        strcpy(dev->name, "");
        DEVICE_AT(device)->freeBlocks = 0;
        name = nbc_find_text(0x70, 0);
        mcard_msg_crc_failure(name, device);
        cont = gc_resolve_profile_crc_answer(device);
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
    int cont;
    int answer;
    const char* name;

    DEVICE_AT(device)->status = *result;

    switch (*result) {
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
        case 1:
        default:
            cont = 1;
            break;
        case 2:
            cont = 0;
            break;
        case 3:
            gc_format_procedure(device);
            cont = 0;
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
        case 1:
        default:
            cont = 1;
            break;
        case 2:
            cont = 0;
            break;
        case 3:
            gc_format_procedure(device);
            cont = 0;
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
            cont = gc_profile_no_space_result(name, device);
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
        cont = gc_profile_no_space_result(name, device);
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
    switch (rc) {
    case CARD_RESULT_READY:
        return 0;
    case CARD_RESULT_NOCARD:
        return -10;
    case CARD_RESULT_WRONGDEVICE:
        return -0x32;
    case CARD_RESULT_BROKEN:
        return -0x33;
    case CARD_RESULT_ENCODING:
        return -0x34;
    case CARD_RESULT_BUSY:
    case CARD_RESULT_IOERROR:
    case CARD_RESULT_FATAL_ERROR:
    default:
        return -99;
    }
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

/* TODO: [breakthrough needed] 94.72%; recover retail unmount's second checked index and wrapper boundary. */
int gc_format_procedure(int device) {
    unsigned int mask;
    s32 sectorSize;
    s32 rc;
    int chan;
    int cardChanged;
    int confirmed;
    int formatted;
    int i;

    mask = mcmasks[device];
    sectorSize = 0;
    confirmed = 0;
    removals &= (int)~mask;
    insertions &= (int)~mask;
    last_card_state |= (int)mask;

    while (!confirmed) {
        mcard_msg_format_confirmation(device);
        if (msg_format_confirmation_answer != 1) {
            return 0;
        }
        confirmed = 1;
    }

    formatted = 0;
    while (!formatted) {
        f_writing_to_memcard = 1;
        mcard_msg_formating(device);
        do {
            rc = CARDProbeEx(device, 0, &sectorSize);
        } while (rc == -1);

        for (i = 0; i < 2; i++) {
            gc_mem_card_status_changes_for_one_device(i);
        }

        if ((removals & mcmasks[device]) != 0 || (insertions & mcmasks[device]) != 0) {
            cardChanged = 1;
        } else {
            cardChanged = 0;
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

        if (rc != 0 || sectorSize != 0x2000) {
            break;
        }

        rc = gc_mount_checked(device);

        if (rc != 0 && (unsigned int)(rc + 0x34) > 1) {
            break;
        }

        do {
            rc = CARDFormat(device);
        } while (rc == -1);

        switch (rc) {
        case CARD_RESULT_READY:
            chan = 0;
            break;
        case CARD_RESULT_NOCARD:
            chan = -10;
            break;
        case CARD_RESULT_IOERROR:
        case CARD_RESULT_FATAL_ERROR:
        default:
            chan = -99;
            break;
        }

        if (chan != 0) {
            mcard_msg_format_failed(device);
            if (msg_format_failed_answer == 2) {
                f_writing_to_memcard = 0;
                return 0;
            }
        } else {
            formatted = 1;
        }
    }

    if (formatted) {
        rc = gc_unmount_checked(device);
        if (rc == 0) {
            mcard_msg_format_successful(device);
            f_writing_to_memcard = 0;
            return 1;
        }
    }
    f_writing_to_memcard = 0;
    return 0;
}

/* TODO: [near miss] 95.51%; mount/delete/unmount CFG agrees; device copies and register homes remain. */
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

/* TODO: [breakthrough] 91.44%; shared unmount and checksum values recovered;
 * read-error close placement, result lifetime, and mask scheduling remain. */
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
    unsigned int bit;
    int sum;
    int storedChecksum;
    unsigned int seekBlock;

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

    if (rc != 0) {
        switch (rc) {
        case -3: return 1;
        case -2: return 10;
        case -128:
        default: return 4;
        }
    }
    if (sectorSize != 0x2000)
        return 8;

    status = gc_mount_checked(device);
    switch (status) {
    case -10: return 1;
    case -50: return 10;
    case -51: result = 11; break;
    case -52: result = 9; break;
    case -99:
    default: return 4;
    case 0:
        do {
            rc = CARDFreeBlocks(device, (s32*)freeBlocks, (s32*)freeBytes);
        } while (rc == -1);
        if (rc != 0) {
            *freeBlocks = 0;
            switch (rc) {
            case -3: result = 1; break;
            case -6:
            case -128:
            default: result = 4; break;
            }
        } else {
            *freeBlocks = (*freeBlocks + 0x1FFF) >> 13;
            if (device < 0 || device >= 2) status = -99;
            else {
                do {
                    rc = CARDOpen(device, fileName, &fileInfo);
                } while (rc == -1);
                switch (rc) {
                case 0: status = 0; break;
                case -3: status = -10; break;
                case -4: status = -4; break;
                case -5:
                case -10:
                case -128:
                default: status = -99; break;
                }
            }
            if (status != 0) {
                switch (status) {
                case -2: result = 7; break;
                case -6: result = 5; break;
                case -4: result = 2; break;
                case -10: result = 1; break;
                case -3:
                case -99:
                default: result = 4; break;
                }
            } else {
                seekBlock = 0;
                if (offset >= 0x28B8U)
                    seekBlock = (offset - 0x28B8U) / 0x1F54U + 2;
                gc_seek_position = seekBlock;
                rc = mem_card_read(&fileInfo, buffer, size);
                if (rc != 0) {
                    switch (rc) {
                    case -2: result = 7; break;
                    case -10: result = 1; break;
                    case -4: result = 2; break;
                    case -53: result = 6; break;
                    case -3:
                    case -99:
                    default: result = 4; break;
                    }
                    gc_seek_position = 0;
                    do {
                        rc = CARDClose(&fileInfo);
                    } while (rc == -1);
                } else {
                    gc_seek_position = 0;
                    do {
                        rc = CARDClose(&fileInfo);
                    } while (rc == -1);
                    switch (rc) {
                    case 0: status = 0; break;
                    case -3: status = -10; break;
                    case -128:
                    default: status = -99; break;
                    }
                    if (status != 0) {
                        switch (status) {
                        case -2: result = 7; break;
                        case -10: result = 1; break;
                        case -4: result = 2; break;
                        case -3:
                        case -9:
                        case -99:
                        default: result = 4; break;
                        }
                    } else {
                        checksumLength = (unsigned int)size - 4;
                        walk = buffer;
                        bit = 0x80;
                        sum = 0;
                        for (remaining = 0; remaining < checksumLength; remaining++) {
                            unsigned char data = *walk;
                            unsigned char oldBit = bit;
                            bit = (unsigned char)bit >> 1;
                            walk++;
                            sum += (unsigned char)(data | oldBit);
                            if (bit == 0) bit = 0x80;
                        }
                        storedChecksum = sum;
                        if (compare_checksums((char*)buffer + checksumLength,
                            (const char*)&storedChecksum) == 0) {
                            *checksumFailOut = 1;
                            result = 6;
                        } else {
                            status = gc_unmount_checked(device);
                            switch (status) {
                            case 0: return 0;
                            case -10: result = 1; break;
                            case -99:
                            default: result = 4; break;
                            }
                        }
                    }
                }
            }
        }
        break;
    }
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

/* TODO: [breakthrough] 88.29%; remaining shared cleanup joins and checksum lowering differ from retail. */
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
    int checksumWord;
    unsigned int checksumLength;
    unsigned int remaining;
    unsigned int bit;
    unsigned int seekBlock;
    unsigned char* walk;
    CARDFileInfo fileInfo;
    int writeLen;
    int bufSize;
    void* srcBuf;

    checksum = 0;
    sectorSize = 0;
    *freeBytes = 0;
    gc_seek_position = 0;
    if (device < 0 || device >= 2) {
        return 0;
    }
    if (modeFlag != 0) {
        return 0;
    }

    checksumLength = (unsigned int)size - 4;
    if (skipChecksum != 0) {
        checksumWord = 0;
    } else {
        walk = buffer;
        bit = 0x80;
        for (remaining = checksumLength; remaining > 0; remaining--) {
            unsigned char data = *walk;
            unsigned char oldBit = bit;
            bit = (unsigned char)bit >> 1;
            walk++;
            checksum += (unsigned char)(data | oldBit);
            if (bit == 0) {
                bit = 0x80;
            }
        }
        checksumWord = checksum;
    }
    *(int*)((unsigned char*)buffer + checksumLength) = checksumWord;

    do {
        rc = CARDProbeEx(device, 0, &sectorSize);
    } while (rc == -1);
    for (i = 0; i < 2; i++) {
        gc_mem_card_status_changes_for_one_device(i);
    }
    if (rc != 0) {
        switch (rc) {
        case -3: result = 1; break;
        case -2: result = 10; break;
        case -128:
        default: result = 4; break;
        }
        return result;
    }
    if (sectorSize != 0x2000) {
        result = 8;
        return result;
    }

    seekBlock = 0;
    if (offset >= 0x28B8U) {
        seekBlock = (offset - 0x28B8U) / 0x1F54U + 2;
    }
    gc_seek_position = seekBlock;
    status = gc_mount_checked(device);
    if (status != 0) {
        switch (status) {
        case -10: result = 1; return result;
        case -50: result = 10; return result;
        case -51: result = 11; break;
        case -52: result = 9; break;
        case -99:
        default: result = 4; return result;
        }
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
        switch (rc) {
        case -3: result = 1; break;
        case -6: result = 11; break;
        case -128:
        default: result = 4; break;
        }
        gc_seek_position = 0;
        do {
            rc = CARDClose(&fileInfo);
        } while (rc == -1);
        if (device >= 0 && device < 2) {
            do {
                rc = CARDUnmount(device);
            } while (rc == -1);
        }
        unload_memorycard_write_buffer();
        return result;
    }
    *freeBlocks = (*freeBlocks + 0x1FFF) >> 13;

    /* Preparation failures share unmount and buffer release. */
    do {
        if (createFlag != 0) {
            if (create_memorycard_write_buffer(buffer, size) == 0) {
                result = 4;
                break;
            }
            do {
                rc = CARDCreate(device, fileName, 0x74000, &fileInfo);
            } while (rc == -1);
            if (rc != 0) {
                switch (rc) {
                case -3: result = 0; break;
                case -8:
                case -9: result = 5; break;
                case -2: result = 7; break;
                case -7: result = 6; break;
                case -5:
                case -12:
                case -128:
                default:
                    if (device >= 0 && device < 2) {
                        last_card_serial_no[device] = 0;
                    }
                    result = 4;
                    break;
                }
                break;
            }
            do {
                rc = CARDSetAttributes(fileInfo.chan, fileInfo.fileNo, 0xc);
            } while (rc == -1);
            if (rc != 0) {
                switch (rc) {
                case -4: result = 2; break;
                case -3: result = 1; break;
                case -5:
                case -10:
                case -128:
                default: result = 4; break;
                }
                break;
            }
        } else {
            if (create_memorycard_write_buffer(buffer, size) == 0) {
                result = 4;
                break;
            }
            if (device < 0 || device >= 2) {
                status = -99;
            } else {
                do {
                    rc = CARDOpen(device, fileName, &fileInfo);
                } while (rc == -1);
                switch (rc) {
                case 0: status = 0; break;
                case -3: status = -10; break;
                case -4: status = -4; break;
                case -5:
                case -10:
                case -128:
                default: status = -99; break;
                }
            }
            if (status != 0) {
                switch (status) {
                case -2: result = 7; break;
                case -6: result = 5; break;
                case -4: result = 2; break;
                case -10: result = 1; break;
                case -3:
                case -99:
                default: result = 4; break;
                }
                break;
            }
        }

        /* Write failures also close the file before that shared cleanup. */
        do {
            bufSize = mc_data_buffer_size;
            srcBuf = mc_data_buffer;
            writeLen = ((bufSize + 0x1FFF) / 0x2000) * 0x2000;
            if ((unsigned int)writeLen > sizeof(gc_memcard_io_buffer)) {
                status = -99;
            } else {
                memcpy(gc_memcard_io_buffer, srcBuf, bufSize);
                if (writeLen - bufSize > 0) {
                    memset((unsigned char*)srcBuf + bufSize, 0, writeLen - bufSize);
                }
                do {
                    rc = CARDWrite(&fileInfo, gc_memcard_io_buffer, writeLen, gc_seek_position << 13);
                } while (rc == -1);
                switch (rc) {
                case 0: status = 0; break;
                case -3: status = -10; break;
                case -4: status = -4; break;
                case -5:
                case -8:
                case -9:
                case -14:
                case -128:
                default: status = -99; break;
                }
            }
            if (status != 0) {
                switch (status) {
                case -10: result = 1; break;
                case -4: result = 2; break;
                case -99:
                default: result = 4; break;
                }
                break;
            }
            if (update_memory_card_status(&fileInfo) == 0) {
                result = 4;
                break;
            }
            *freeBlocks = 0;
            do {
                rc = CARDFreeBlocks(device, (s32*)freeBlocks, (s32*)freeBytes);
            } while (rc == -1);
            if (rc != 0) {
                *freeBlocks = 0;
                switch (rc) {
                case -3: result = 1; break;
                case -6: result = 11; break;
                case -128:
                default: result = 4; break;
                }
                break;
            }
            *freeBlocks = (*freeBlocks + 0x1FFF) >> 13;
            gc_seek_position = 0;
            do {
                rc = CARDClose(&fileInfo);
            } while (rc == -1);
            switch (rc) {
            case 0: status = 0; break;
            case -3: status = -10; break;
            case -128:
            default: status = -99; break;
            }
            if (status != 0) {
                switch (status) {
                case -2: result = 7; break;
                case -10: result = 1; break;
                case -4: result = 2; break;
                case -3:
                case -9:
                case -99:
                default: result = 4; break;
                }
                if (device >= 0 && device < 2) {
                    do {
                        rc = CARDUnmount(device);
                    } while (rc == -1);
                }
                unload_memorycard_write_buffer();
                return result;
            }
            status = gc_unmount_checked(device);
            if (status != 0) {
                switch (status) {
                case -10: result = 1; break;
                case -99:
                default: result = 4; break;
                }
                unload_memorycard_write_buffer();
                return result;
            }
            unload_memorycard_write_buffer();
            return status;
        } while (0);

        gc_seek_position = 0;
        do {
            rc = CARDClose(&fileInfo);
        } while (rc == -1);
    } while (0);
    if (device >= 0 && device < 2) {
        do {
            rc = CARDUnmount(device);
        } while (rc == -1);
    }
    unload_memorycard_write_buffer();
    return result;
}


/* TODO: [near miss] 99.39%; 64-bit serial and removal state recovered; flag publication homes remain. */
void gc_mem_card_status_changes_for_one_device(int device) {
    int serialRc;
    u64 serial;

    serial = 0;
    do {
    } while (CARDProbeEx(device, 0, 0) == -1);

    serialRc = gc_get_memcard_serial_number(device, &serial);

    switch (serialRc) {
    case 0: {
        unsigned int mask = mcmasks[device];
        if ((last_card_state & (int)mask) != 0) {
            if (serial != last_card_serial_no[device]) {
                removals |= (int)mask;
                insertions |= (int)mask;
                last_card_state |= (int)mask;
                last_card_serial_no[device] = serial;
            }
            return;
        }
        insertions |= (int)mask;
        last_card_state |= (int)mask;
        last_card_serial_no[device] = serial;
        return;
    }
    case -6:
    case -2:
    case -0xd:
    case -0x80: {
        unsigned int mask = mcmasks[device];
        if ((last_card_state & (int)mask) == 0) {
            last_card_state |= (int)mask;
            insertions |= (int)mask;
        }
        last_card_serial_no[device] = 0;
        return;
    }
    case -3: {
        unsigned int mask = mcmasks[device];
        int state = last_card_state;
        int pending = insertions;
        unsigned int retained_mask = ~mask;

        insertions = pending & (int)retained_mask;
        if ((state & (int)mask) != 0) {
            int removed = removals;
            last_card_state = state & (int)retained_mask;
            removals = removed | (int)mask;
        }
        last_card_serial_no[device] = 0;
        return;
    }
    default:
        last_card_serial_no[device] = 0;
        break;
    }
}

static inline int gc_mount_for_serial(int device) {
    unsigned char* workArea;
    CARDCallback detach;
    s32 rc;

    if (device < 0 || device >= 2) {
        return -99;
    }
    if (device == 0) {
        workArea = mc_workArea_0;
        detach = detached_slot_a;
    } else {
        workArea = mc_workArea_1;
        detach = detached_slot_b;
    }
    do {
        rc = CARDMount(device, workArea, detach);
    } while (rc == CARD_RESULT_BUSY);
    switch (rc) {
    case CARD_RESULT_READY:
        return 0;
    case CARD_RESULT_NOCARD:
        return -10;
    case CARD_RESULT_WRONGDEVICE:
        return -0x32;
    case CARD_RESULT_BROKEN:
        return -0x33;
    case CARD_RESULT_ENCODING:
        return -0x34;
    case CARD_RESULT_BUSY:
    case CARD_RESULT_IOERROR:
    case CARD_RESULT_FATAL_ERROR:
    default:
        return -99;
    }
}

static int gc_get_memcard_serial_number(int device, u64* out) {
    s32 serialRc;
    u64 serial;
    int mapped;
    int result;
    int readSerial;

    result = 0;
    serial = 0;
    *out = 0;

    if (device > 0 || device < 2) {
        mapped = gc_mount_for_serial(device);

        switch (mapped) {
        case 0:
            result = 0;
            readSerial = 1;
            break;
        case -0x33:
            result = CARD_RESULT_BROKEN;
            readSerial = 1;
            break;
        case -0x34:
            result = CARD_RESULT_ENCODING;
            readSerial = 1;
            break;
        case -99:
            result = CARD_RESULT_FATAL_ERROR;
            readSerial = 0;
            break;
        case -10:
            result = CARD_RESULT_NOCARD;
            readSerial = 0;
            break;
        case -0x32:
            result = CARD_RESULT_WRONGDEVICE;
            readSerial = 0;
            break;
        default:
            result = CARD_RESULT_FATAL_ERROR;
            readSerial = 0;
            break;
        }

        if (readSerial) {
            do {
                serialRc = CARDGetSerialNo(device, &serial);
            } while (serialRc == CARD_RESULT_BUSY);
            switch (serialRc) {
            case CARD_RESULT_READY:
                *out = serial;
                break;
            case CARD_RESULT_NOCARD:
            case CARD_RESULT_FATAL_ERROR:
            default:
                result = serialRc;
                *out = 0;
                break;
            }
            if (device >= 0 && device < 2) {
                do {
                    serialRc = CARDUnmount(device);
                } while (serialRc == CARD_RESULT_BUSY);
            }
        }
    }
    return result;
}

/* TODO: [near miss] 93.95%; device/result registers and two post-callback load pairs differ. */
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

/* TODO: [near miss] 94.44%; inline device/mask allocation and post-callback load ordering remain. */
int update_storage_status(int flag) {
    int device;
    int any = 0;

    for (device = 0; device < 2; device++) {
        if (update_storage_status_for_one_device(device)) {
            any = 1;
        }
    }
    return any;
}

int mem_card_read(CARDFileInfo* fileInfo, void* buffer, int size) {
    CARDStat stat;
    int readLen;
    int result;

    do {
        result = CARDGetStatus(fileInfo->chan, fileInfo->fileNo, &stat);
    } while (result == CARD_RESULT_BUSY);

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
    } while (result == CARD_RESULT_BUSY);

    switch (result) {
    case CARD_RESULT_READY:
        if (gc_seek_position == 0) {
            memcpy(buffer, gc_memcard_io_buffer + stat.offsetData, size);
        } else {
            memcpy(buffer, gc_memcard_io_buffer, size);
        }
        return 0;
    case CARD_RESULT_NOCARD:
        return -0xA;
    case CARD_RESULT_NOFILE:
        return -0x4;
    case CARD_RESULT_NOPERM:
    case CARD_RESULT_LIMIT:
    case CARD_RESULT_CANCELED:
    case CARD_RESULT_FATAL_ERROR:
    default:
        return -0x63;
    }
}

static void detached_slot_b(s32 chan, s32 result) {
}

static void detached_slot_a(s32 chan, s32 result) {
}

int init_gc_memcard(void) {
    CARDInit();
    last_card_state = 0;
    load_icon_data();
    return 1;
}
