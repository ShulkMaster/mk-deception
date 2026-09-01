#include "mw/mwFile.h"
#include "mw/mwFileGlue.h"
#include "mw/mwMemHeap.h"
#include "platform/disc_error.h"
#include "runtime/cstdlib.h"

class mwFileMemTraits {
public:
    static void deallocate(void* ptr);
    static void* allocate(unsigned long size, mwTargetMemAlign align, const char* name);
};

typedef union mwFileInitFlagValue {
    float value;
    unsigned int flags;
} mwFileInitFlagValue;

static const char stringBase0[] =
    "game\0"
    "/cdrom\0"
    "art\0"
    "/game/art\0"
    "sounds\0"
    "/game/sndsgc\0"
    "kryptmovies\0"
    "/game/moviegc/KRYPT\0"
    "movies\0"
    "/game/moviegc\0"
    "logs\0"
    "/game\0"
    "hostwrite\0"
    "/host\0"
    "cdrom\0"
    "anims\0"
    "/game/anims\0"
    "mko\0"
    "/game/mko\0";

#define MOUNT_GAME (&stringBase0[0])
#define PATH_CDROM (&stringBase0[5])
#define MOUNT_ART (&stringBase0[0xC])
#define PATH_ART (&stringBase0[0x10])
#define MOUNT_SOUNDS (&stringBase0[0x1A])
#define PATH_SOUNDS (&stringBase0[0x21])
#define MOUNT_KRYPTMOVIES (&stringBase0[0x2E])
#define PATH_KRYPTMOVIES (&stringBase0[0x3A])
#define MOUNT_MOVIES (&stringBase0[0x4E])
#define PATH_MOVIES (&stringBase0[0x55])
#define MOUNT_LOGS (&stringBase0[0x63])
#define PATH_LOGS (&stringBase0[0x68])
#define MOUNT_HOSTWRITE (&stringBase0[0x6E])
#define PATH_HOSTWRITE (&stringBase0[0x78])
#define MOUNT_CDROM (&stringBase0[0x7E])
#define MOUNT_ANIMS (&stringBase0[0x84])
#define PATH_ANIMS (&stringBase0[0x8A])
#define MOUNT_MKO (&stringBase0[0x96])
#define PATH_MKO (&stringBase0[0x9A])

const int gap_04_802EA424_rodata = 0;

/* Force .sdata2 via const floats; MWF_INIT_DVD bit pattern is 0x00000020. */
const mwFileInitFlagValue MWF_INIT_DVD = {4.484155085839415e-44f}; /* 0x00000020 */
const float gap_09_805117EC_sdata2 = 0.0f;

void mwfile_init_for_mk(void* allocator_context) {
    mwFileInitParam init;
    int err;

    mwFileGetDefaultInitParam(&init);
    init.flags = 7;
    init.max_open_files = 5;
    init.allocator_context = allocator_context;
    init.flags = MWF_INIT_DVD.flags | 7;
    err = mwFileInit(&init);
    if (err != 0) {
        exit(1);
    }
    mwFileMountPath(MOUNT_GAME, PATH_CDROM);
    mwFileMountPath(MOUNT_ART, PATH_ART);
    mwFileMountPath(MOUNT_SOUNDS, PATH_SOUNDS);
    mwFileMountPath(MOUNT_KRYPTMOVIES, PATH_KRYPTMOVIES);
    mwFileMountPath(MOUNT_MOVIES, PATH_MOVIES);
    mwFileMountPath(MOUNT_LOGS, PATH_LOGS);
    mwFileMountPath(MOUNT_HOSTWRITE, PATH_HOSTWRITE);
    err = mwFileSetErrorCallback(MOUNT_CDROM, mwfile_error_callback, 0);
    if (err != 0) {
        exit(1);
    }
    mwFileMountPath(MOUNT_ANIMS, PATH_ANIMS);
    mwFileMountPath(MOUNT_MKO, PATH_MKO);
}

void mwFileMemTraits::deallocate(void* ptr) {
    _mwMemFree(ptr, 0, 0);
}

void* mwFileMemTraits::allocate(unsigned long size,
                                mwTargetMemAlign align,
                                const char* name) {
    (void)name;
    return _mwMemMalloc(mwfile_heap, size, align, 0, 0, 0);
}
