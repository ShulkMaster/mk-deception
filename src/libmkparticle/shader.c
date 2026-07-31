#include "libmkparticle/shader.h"

int pfx_shader_estimate_size(void* shader) {
    if (_pfx_config.estimate_size != 0) {
        return _pfx_config.estimate_size(shader);
    }
    return 0;
}
