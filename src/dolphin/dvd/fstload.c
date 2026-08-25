#include "dolphin/dvd.h"
#include "dolphin/os.h"
#include "dolphin/os_alloc.h"
#include "runtime/cstring.h"

static unsigned char bb2Buffer[63];
static unsigned long status;
static DVDBB2* bb2;
static DVDDiskID* temporary_id;

static void cb(long result, DVDCommandBlock* block)
{
    if (result > 0) {
        switch (status) {
        case 0:
            status = 1;
            DVDReadAbsAsyncForBS(block, bb2, 0x20, 0x420, cb);
            return;
        case 1:
            status = 2;
            DVDReadAbsAsyncForBS(block, bb2->fst_address,
                                 (bb2->fst_length + 0x1F) & 0xFFFFFFE0,
                                 bb2->fst_position, cb);
        default:
            return;
        }
    }

    if (result == -1) {
        return;
    }
    if (result == -4) {
        status = 0;
        DVDReset();
        DVDReadDiskID(block, temporary_id, cb);
    }
}

void __fstLoad(void)
{
    OSBootInfo* boot_info;
    DVDDiskID* id;
    unsigned char id_buffer[63];
    int state;
    static DVDCommandBlock block;

    OSGetArenaHi();
    boot_info = (OSBootInfo*)OSPhysicalToCached(0);
    temporary_id = (DVDDiskID*)OSRoundUp32B(id_buffer);
    bb2 = (DVDBB2*)OSRoundUp32B(bb2Buffer);

    DVDReset();
    DVDReadDiskID(&block, temporary_id, cb);

    while (1) {
        state = DVDGetDriveStatus();
        if (state == 0) {
            break;
        }
    }

    boot_info->fst_location = bb2->fst_address;
    boot_info->fst_max_length = bb2->fst_max_length;
    id = &boot_info->disk_id;
    memcpy(id, temporary_id, 0x20);
    OSReport("\n");
    OSReport("  Game Name ... %c%c%c%c\n", id->gameName[0], id->gameName[1],
             id->gameName[2], id->gameName[3]);
    OSReport("  Company ..... %c%c\n", id->company[0], id->company[1]);
    OSReport("  Disk # ...... %d\n", id->diskNumber);
    OSReport("  Game ver .... %d\n", id->gameVersion);
    OSReport("  Streaming ... %s\n", id->streaming == 0 ? "OFF" : "ON");
    OSReport("\n");
    OSSetArenaHi(bb2->fst_address);
}
