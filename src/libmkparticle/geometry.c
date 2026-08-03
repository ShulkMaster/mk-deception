#include "libmkparticle/geometry.h"
#include "libmkparticle/particle.h"

static const float kHalf = 0.5f;
static const float kZero = 0.0f;

/* Soft ceiling: pfx_get_billboard_vector ~98.84% -- constant-pool relocations
 * and one equivalent fneg scheduling difference. */
void pfx_get_billboard_vector(PfxVm* vm, PfxVec3* axis0,
                              PfxVec3* axis1) {
    PfxTransform* transform;
    float factor;
    float scale0;
    float scale1;

    factor = kZero;
    if (vm->flag150_02 != 0) {
        axis1->x = vm->billboard_size;
        axis1->y = factor;
        axis1->z = factor;
        axis0->x = factor;
        axis0->y = -vm->billboard_size;
        axis0->z = factor;
        return;
    }

    if (vm->flag150_40 != 0) {
        factor = kHalf * vm->billboard_size;
    }
    if ((vm->flags_0x1D4 & 0x20) != 0) {
        factor = kHalf;
    }

    scale0 = factor * vm->geometry_scale0;
    scale1 = factor * vm->geometry_scale1;
    if (vm->flag150_10 != 0) {
        axis1->x = vm->geometry_axis0.x * scale0;
        axis1->y = vm->geometry_axis0.y * scale0;
        axis1->z = vm->geometry_axis0.z * scale0;
        axis0->x = vm->geometry_axis1.x * scale1;
        axis0->y = vm->geometry_axis1.y * scale1;
        axis0->z = vm->geometry_axis1.z * scale1;
        return;
    }

    transform = &vm->transforms[vm->active_transform];
    if (vm->flag151_80 != 0) {
        float negative_factor;

        axis1->x = pfxsystem_globals.billboard_axis0.x;
        axis1->y = pfxsystem_globals.billboard_axis0.y;
        axis1->z = pfxsystem_globals.billboard_axis0.z;
        axis0->x = pfxsystem_globals.billboard_axis1.x;
        axis0->y = pfxsystem_globals.billboard_axis1.y;
        axis0->z = pfxsystem_globals.billboard_axis1.z;
        negative_factor = -factor;
        axis1->x *= negative_factor;
        axis1->y *= negative_factor;
        axis1->z *= negative_factor;
        axis0->x *= factor;
        axis0->y *= factor;
        axis0->z *= factor;
        return;
    }

    axis1->x = vm->basis0.x * transform->matrix.elements[0] +
               vm->basis0.y * transform->matrix.elements[1] +
               vm->basis0.z * transform->matrix.elements[2];
    axis1->y = vm->basis0.x * transform->matrix.elements[4] +
               vm->basis0.y * transform->matrix.elements[5] +
               vm->basis0.z * transform->matrix.elements[6];
    axis1->z = vm->basis0.x * transform->matrix.elements[8] +
               vm->basis0.y * transform->matrix.elements[9] +
               vm->basis0.z * transform->matrix.elements[10];
    axis0->x = vm->basis1.x * transform->matrix.elements[0] +
               vm->basis1.y * transform->matrix.elements[1] +
               vm->basis1.z * transform->matrix.elements[2];
    axis0->y = vm->basis1.x * transform->matrix.elements[4] +
               vm->basis1.y * transform->matrix.elements[5] +
               vm->basis1.z * transform->matrix.elements[6];
    axis0->z = vm->basis1.x * transform->matrix.elements[8] +
               vm->basis1.y * transform->matrix.elements[9] +
               vm->basis1.z * transform->matrix.elements[10];

    axis1->x *= scale0;
    axis1->y *= scale0;
    axis1->z *= scale0;
    axis0->x *= scale1;
    axis0->y *= scale1;
    axis0->z *= scale1;
}
