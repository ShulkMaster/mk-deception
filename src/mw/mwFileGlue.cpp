#include "mw/mwFile.h"
#include "mw/mwFileGlue.h"
#include "mw/mwFileMemTraits.h"
#include "mw/mwMemHeap.h"
#include "platform/disc_error.h"
#include "runtime/cstdlib.h"

union mwFileInitFlagValue {
    float value;
    unsigned int flags;
};


const mwFileInitFlagValue MWF_INIT_DVD = {4.484155085839415e-44f};

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
    mwFileMountPath("game", "/cdrom");
    mwFileMountPath("art", "/game/art");
    mwFileMountPath("sounds", "/game/sndsgc");
    mwFileMountPath("kryptmovies", "/game/moviegc/KRYPT");
    mwFileMountPath("movies", "/game/moviegc");
    mwFileMountPath("logs", "/game");
    mwFileMountPath("hostwrite", "/host");
    err = mwFileSetErrorCallback("cdrom", mwfile_error_callback, 0);
    if (err != 0) {
        exit(1);
    }
    mwFileMountPath("anims", "/game/anims");
    mwFileMountPath("mko", "/game/mko");
}

void mwFileMemTraits::deallocate(void* ptr) {
    _mwMemFree(ptr, 0, 0);
}

void* mwFileMemTraits::allocate(size_t size,
                                mwTargetMemAlign align,
                                const char* name) {
    return _mwMemMalloc(mwfile_heap, size, align, 0, 0, 0);
}
