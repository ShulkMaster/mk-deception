#include "runtime/mk_pebble.h"

#include "runtime/mk_mem.h"
#include "runtime/mk_plugins.h"
#include "runtime/mk_struct.h"
#include "runtime/mk_vtbl.h"
#include "rw/rtquat.h"
#include "rw/rwcamera_internal.h"
#include "rw/rwframe.h"

void* memcpy(void* dest, const void* src, unsigned int size);

extern int MksobjLocalOffset;
extern RwCamera* Camera;

RwSphere* RpAtomicGetWorldBoundingSphere(RpAtomic* atomic);
RpAtomic* AtomicDefaultRenderCallBack(RpAtomic* atomic);

static RpAtomic* pebble_render_nothing_callback(RpAtomic* atomic);
static RpAtomic* pebble_render_callback(RpAtomic* atomic);

int vdestroy_pebble(PebbleData* pebble_data) {
    if (pebble_data->pebbles != 0) {
        free_mem(pebble_data->pebbles);
    }
    pebble_data->hdr.instance = 0;
    mkhdr_memfree(&pebble_data->hdr);
    /* Retail's int vtable slot deliberately leaves r3 from mkhdr_memfree. */
}

/* Soft ceiling: create_pebble_userdata ~98.47% -- typed matrix-field indexing colors the loop differently. */
PebbleData* create_pebble_userdata(MkSobj* sobj, int count, int user_data_size) {
    RpAtomic* atomic;
    PebbleData* pebble_data;
    int matrix_total;
    int allocation_size;
    int i;

    if (sobj == 0 || (atomic = sobj->atomic) == 0) {
        return 0;
    }
    pebble_data = (PebbleData*)get_mkhdr(&vtbl_pebble, sizeof(PebbleData));
    if (pebble_data == 0) {
        return 0;
    }
    user_data_size *= count;
    matrix_total = count * sizeof(Pebble);
    allocation_size = matrix_total + sizeof(PebbleRenderData);
    allocation_size += user_data_size;
    allocation_size += count * sizeof(PebbleFlags);
    pebble_data->pebbles = get_mem(allocation_size);
    pebble_data->render_data = (PebbleRenderData*)((unsigned char*)pebble_data->pebbles +
                                                  matrix_total);
    pebble_data->user_data = pebble_data->render_data + 1;
    pebble_data->flags = (PebbleFlags*)((unsigned char*)pebble_data->user_data +
                                       user_data_size);
    pebble_data->render_data->callback = atomic->renderCallBack;
    if (pebble_data->render_data->callback == pebble_render_callback) {
        pebble_data->render_data->callback = pebble_render_nothing_callback;
    }
    for (i = 0; i < count; i++) {
        pebble_data->flags[i].word = 0;
        pebble_data->flags[i].bits.visible = 1;
    }
    atomic->renderCallBack = pebble_render_callback;
    if (atomic->renderCallBack == 0) {
        atomic->renderCallBack = AtomicDefaultRenderCallBack;
    }
    for (i = 0; i < count; i++) {
        pebble_data->pebbles[i].matrix.at.z = 1.0f;
        pebble_data->pebbles[i].matrix.up.y = 1.0f;
        pebble_data->pebbles[i].matrix.right.x = 1.0f;
        pebble_data->pebbles[i].matrix.up.x = 0.0f;
        pebble_data->pebbles[i].matrix.right.z = 0.0f;
        pebble_data->pebbles[i].matrix.right.y = 0.0f;
        pebble_data->pebbles[i].matrix.at.y = 0.0f;
        pebble_data->pebbles[i].matrix.at.x = 0.0f;
        pebble_data->pebbles[i].matrix.up.z = 0.0f;
        pebble_data->pebbles[i].matrix.pos.z = 0.0f;
        pebble_data->pebbles[i].matrix.pos.y = 0.0f;
        pebble_data->pebbles[i].matrix.pos.x = 0.0f;
        pebble_data->pebbles[i].matrix.flags |= 0x20003;
    }
    sobj->flags09_bits.has_pebbles = 1;
    pebble_data->count = count;
    pebble_data->active_count = count;
    sobj->bound_hdr = &pebble_data->hdr;
    sobj->bound_instance = pebble_data->hdr.instance;
    return pebble_data;
}

static RpAtomic* pebble_render_nothing_callback(RpAtomic* atomic) {
    return 0;
}

/* Soft ceiling: pebble_render_callback ~92.24% -- remaining register coloring and stack-slot placement. */
static RpAtomic* pebble_render_callback(RpAtomic* atomic) {
    RwFrame* frame;
    MkSobj* sobj;
    PebbleData* pebble_data;
    RwMatrix saved_matrix;
    RwSphere render_sphere;
    RwSphere saved_sphere;
    Vec sphere_offset;
    RwSphere test_sphere;
    RwSphere* atomic_sphere;
    RwMatrix* atomic_ltm;
    RwMatrix* cull_ltm;
    RwCamera* camera;
    int visible_count;
    int i;

    if (atomic == 0) {
        return atomic;
    }
    frame = (RwFrame*)atomic->object.parent;
    if (frame == 0) {
        return atomic;
    }
    sobj = MK_ATOMIC_PLUGIN(atomic)->sobj;
    if (sobj == 0) {
        return atomic;
    }
    pebble_data = (PebbleData*)sobj->bound_hdr;
    if (pebble_data != 0) {
        if (pebble_data->hdr.instance != sobj->bound_instance) {
            pebble_data = 0;
        }
    } else {
        pebble_data = 0;
    }
    if (pebble_data == 0) {
        return atomic;
    }
    if (pebble_data->pebbles == 0) {
        return atomic;
    }
    if (pebble_data->render_data->callback == 0) {
        return atomic;
    }
    atomic_ltm = RwFrameGetLTM(frame);
    if (atomic_ltm == 0) {
        return atomic;
    }
    if (!sobj->flags09_bits.bit3) {
        camera = Camera;
        visible_count = 0;
        cull_ltm = RwFrameGetLTM((RwFrame*)atomic->object.parent);
        atomic_sphere = RpAtomicGetWorldBoundingSphere(atomic);
        sphere_offset.x = atomic_sphere->center.x - cull_ltm->pos.x;
        sphere_offset.y = atomic_sphere->center.y - cull_ltm->pos.y;
        sphere_offset.z = atomic_sphere->center.z - cull_ltm->pos.z;
        test_sphere.radius = atomic_sphere->radius + PSVECMag(&sphere_offset);
        for (i = 0; i < pebble_data->count; i++) {
            test_sphere.center.x = pebble_data->pebbles[i].matrix.pos.x;
            test_sphere.center.y = pebble_data->pebbles[i].matrix.pos.y;
            test_sphere.center.z = pebble_data->pebbles[i].matrix.pos.z;
            pebble_data->flags[i].bits.visible = 1;
            switch (RwCameraFrustumTestSphere(camera, &test_sphere)) {
            case 0:
                pebble_data->flags[i].bits.visible = 0;
                break;
            case 2:
                pebble_data->flags[i].bits.partly_visible = 0;
                break;
            case 1:
                pebble_data->flags[i].bits.partly_visible = 1;
                break;
            }
            if (pebble_data->flags[i].bits.visible) {
                visible_count++;
            }
        }
        if (visible_count == 0) {
            return 0;
        }
    }
    memcpy(&saved_matrix, atomic_ltm, sizeof(saved_matrix));
    saved_sphere = *RpAtomicGetWorldBoundingSphere(atomic);
    render_sphere.radius = saved_sphere.radius;
    for (i = 0; i < pebble_data->count; i++) {
        if (pebble_data->flags[i].bits.visible) {
            RwMatrixMultiply(&frame->ltm, &saved_matrix,
                             &pebble_data->pebbles[i].matrix);
            render_sphere.center.x = saved_sphere.center.x + frame->ltm.pos.x;
            render_sphere.center.y = saved_sphere.center.y + frame->ltm.pos.y;
            render_sphere.center.z = saved_sphere.center.z + frame->ltm.pos.z;
            atomic->worldBoundingSphere = render_sphere;
            pebble_data->render_data->callback(atomic);
        }
    }
    atomic->worldBoundingSphere = saved_sphere;
    memcpy(atomic_ltm, &saved_matrix, sizeof(saved_matrix));
    return atomic;
}
