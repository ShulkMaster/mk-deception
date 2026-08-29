#include "libmkparticle/init.h"
#include "runtime/cstring.h"

#pragma scheduling on
static void init_transform_matrix(PfxVm* vm) {
    int i;

    i = 0;
    do {
        PfxTransform* transform;

        transform = &vm->transforms[i];
        mat_set_identity(&transform->matrix);
        i++;
    } while (i < 3);
}
#pragma scheduling reset

void pfxvm_init(PfxVm* vm) {
    int i;

    memset(vm, 0, sizeof(*vm));
    if (vm->emitters != 0) {
        memset(vm->emitters, 0, vm->emitter_count * sizeof(PfxVmEmitter));
        for (i = 0; i < vm->emitter_count; i++) {
            vm->emitters[i].field_40 = 0;
        }
    }

    vm->field1E4 = 0.0f;
    vm->field1E8 = -0.003f;
    vm->field1EC = 0.0f;
    vm->flag150_40 = 1;
    vm->billboard_size = 0.13f;
    vm->geometry_scale0 = 1.0f;
    vm->geometry_scale1 = 1.0f;
    vm->flag150_80 = 1;
    pfx_native_set_rgba(&vm->color1B4, 255.0f, 255.0f, 255.0f, 255.0f);
    vm->field238 = -1.0f;
    init_transform_matrix(vm);
}
