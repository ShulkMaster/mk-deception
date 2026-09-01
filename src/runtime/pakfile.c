#include "runtime/pakfile.h"

#include "runtime/hashtable.h"

static Hashtable member_hashtable;

int init_pakfile_system(void) {
    member_hashtable.initialized = 0;
    return 1;
}
