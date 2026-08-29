#include "libmkparticle/shader.h"

int pfx_shader_estimate_size(unsigned int render_flags) {
    if (_pfx_config.estimate_size != 0) {
        return _pfx_config.estimate_size(render_flags);
    }
    return 0;
}
