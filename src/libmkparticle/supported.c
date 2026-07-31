#include "libmkparticle/supported.h"

unsigned int supported_render_flags = 0x00000372;

// .sdata section missing 4bytes

int pfx_native_is_supported_type(int type) {
    unsigned int masked;

    if ((type & 2) == 0) {
        return 0;
    }

    masked = type & supported_render_flags;
    return (masked - type) == 0;
}
