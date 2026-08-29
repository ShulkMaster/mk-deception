#include "runtime/mk_render.h"

#include "platform/display.h"
#include "rw/rwengine.h"
#include "runtime/mk_particle.h"
#include "runtime/mk_plugins.h"
#include "runtime/mk_vtbl.h"
#include "rw/rwframe.h"
#include "rw/rwcamera_internal.h"

typedef union TranslNodeFlags {
    unsigned int word;
    struct {
        unsigned char red : 1;
        unsigned char pfx : 1;
        unsigned char pfx_clone : 1;
        unsigned char pad_high : 5;
        unsigned char pad[3];
    } bits;
} TranslNodeFlags;

typedef struct TranslSortNode {
    struct TranslSortNode* parent;
    struct TranslSortNode* left;
    struct TranslSortNode* right;
    TranslNodeFlags flags;
    int priority;
    float depth;
    void* payload;
} TranslSortNode; /* 0x1C */

extern RwCamera* Camera;
extern RwMatrix* camera_mat;
extern int curr_pipeline_used;
extern int last_pipeline_used;
extern unsigned long f_render_all_atomics;

void atomic_set_transl_flag(RpAtomic* atomic);
void obj_set_rw_lights(MkObj* object);
RwSphere* RpAtomicGetWorldBoundingSphere(RpAtomic* atomic);

static TranslSortNode transl_sort_nodes[250];
static TranslSortNode* BTREE_ROOT;
static int num_render_nodes;
static int num_transl_callbacks;
static int in_batch;

static void BTreeInsert(TranslSortNode* node);
static void btree_render(TranslSortNode* node);

static inline int node_precedes(const TranslSortNode* a, const TranslSortNode* b) {
    return a->priority > b->priority || (a->priority == b->priority && a->depth >= b->depth);
}

static inline void rotate_left(TranslSortNode* node) {
    TranslSortNode* right = node->right;
    node->right = right->left;
    if (right->left != 0) {
        right->left->parent = node;
    }
    right->parent = node->parent;
    if (node->parent == 0) {
        BTREE_ROOT = right;
    } else if (node == node->parent->left) {
        node->parent->left = right;
    } else {
        node->parent->right = right;
    }
    right->left = node;
    node->parent = right;
}

static inline void rotate_right(TranslSortNode* node) {
    TranslSortNode* left = node->left;
    node->left = left->right;
    if (left->right != 0) {
        left->right->parent = node;
    }
    left->parent = node->parent;
    if (node->parent == 0) {
        BTREE_ROOT = left;
    } else if (node == node->parent->right) {
        node->parent->right = left;
    } else {
        node->parent->left = left;
    }
    left->right = node;
    node->parent = left;
}

void render_transl_atomics(void) {
    in_batch = 0;
    curr_pipeline_used = 0;
    btree_render(BTREE_ROOT);
    obj_set_rw_lights(0);
    if (in_batch != 0) {
        pfx_end_batch();
    }
    num_render_nodes = 0;
    BTREE_ROOT = 0;
}

#pragma inline_depth(2)
static void btree_render(TranslSortNode* node) {
    while (node != 0) {
        if (node->right != 0) {
            btree_render(node->right);
        }
        if (node->flags.bits.pfx || node->flags.bits.pfx_clone) {
            last_pipeline_used = curr_pipeline_used;
            curr_pipeline_used = 0;
            if (in_batch == 0) {
                in_batch = 1;
                pfx_start_batch();
            }
            if (node->flags.bits.pfx) {
                render_pfx(node->payload);
            } else {
                render_pfx_clone(node->payload);
            }
        } else {
            if (in_batch != 0) {
                in_batch = 0;
                pfx_end_batch();
                last_pipeline_used = curr_pipeline_used;
                curr_pipeline_used = 0;
            }
            obj_set_rw_lights(
                MK_CLUMP_PLUGIN(((RpAtomic*)node->payload)->clump)->owner);
            render_mkatomic((RpAtomic*)node->payload);
        }
        node = node->left;
    }
}
#pragma inline_depth reset

RpAtomic* set_transl_callback(RpAtomic* atomic, void* data) {
    atomic_set_transl_flag(atomic);
    return atomic;
}

void init_mk_render(void) {
    num_transl_callbacks = 0;
    num_render_nodes = 0;
    BTREE_ROOT = 0;
}

void InsertPFXCloneInTranslTree(MkHdr* clone_hdr) {
    PfxClone* clone = (PfxClone*)clone_hdr;
    MkPfx* candidate;
    MkPfx* valid_pfx;
    PfxVm* runtime;
    TranslSortNode* node;
    struct {
        TranslNodeFlags copy;
        TranslNodeFlags source;
    } flags;
    PfxVec3* position;
    RwMatrix* camera_matrix;
    float depth;
    int index;
    int node_index;
    if (clone == 0) {
        return;
    }
    candidate = clone->parent;
    if (candidate->hdr.vtbl == &vtbl_pfx) {
        valid_pfx = candidate;
    } else {
        valid_pfx = 0;
    }
    if (valid_pfx == 0) {
        if (clone->hdr.instance != 0) {
            clone->hdr.typed_vtbl->destroy(&clone->hdr);
        }
        return;
    }
    runtime = (PfxVm*)candidate->matrix;
    if (runtime->particle_cursor == 0) {
        return;
    }
    index = runtime->active_transform;
    position = &runtime->transforms[index].position;
    camera_matrix = camera_mat;
    flags.source.word = 0;
    flags.source.bits.pfx_clone = 1;
    depth = (position->z - camera_matrix->pos.z) * camera_matrix->at.z +
            ((position->x - camera_matrix->pos.x) * camera_matrix->at.x +
             (position->y - camera_matrix->pos.y) * camera_matrix->at.y);
    flags.copy = flags.source;
    depth += clone->depth_bias;
    node_index = num_render_nodes;
    if (node_index >= 250) {
        return;
    }
    num_render_nodes = node_index + 1;
    node = &transl_sort_nodes[node_index];
    node->flags = flags.copy;
    node->payload = clone;
    node->priority = clone->priority;
    node->depth = depth;
    BTreeInsert(node);
}

void InsertPFXInTranslTree(MkHdr* pfx_hdr) {
    MkPfx* pfx = (MkPfx*)pfx_hdr;
    TranslSortNode* node;
    TranslNodeFlags flags_pair[2];
    RwV3d origin;
    RwMatrix* camera_matrix;
    float depth;
    int node_index;
    int priority;
    if (pfx != 0 && !pfx->flag_bits.skip_translucent_sort && pfx->field_94 != 0) {
        mkpfx_get_origin(pfx, &origin.x);
        camera_matrix = camera_mat;
        flags_pair[1].word = 0;
        flags_pair[1].bits.pfx = 1;
        depth = (origin.z - camera_matrix->pos.z) * camera_matrix->at.z +
                ((origin.x - camera_matrix->pos.x) * camera_matrix->at.x +
                 (origin.y - camera_matrix->pos.y) * camera_matrix->at.y);
        flags_pair[0] = flags_pair[1];
        depth += pfx->depth_bias;
        node_index = num_render_nodes;
        priority = pfx->priority;
        if (node_index < 250) {
            num_render_nodes = node_index + 1;
            node = &transl_sort_nodes[node_index];
            node->flags = flags_pair[0];
            node->payload = pfx;
            node->priority = priority;
            node->depth = depth;
            BTreeInsert(node);
        }
    }
}

static void BTreeInsert(TranslSortNode* node) {
    TranslSortNode* parent = 0;
    TranslSortNode* current = BTREE_ROOT;
    TranslSortNode* uncle;
    while (current != 0) {
        parent = current;
        current = node_precedes(node, current) ? current->left : current->right;
    }
    node->parent = parent;
    node->left = 0;
    node->right = 0;
    if (parent == 0) {
        BTREE_ROOT = node;
    } else if (node_precedes(node, parent)) {
        parent->left = node;
    } else {
        parent->right = node;
    }
    node->flags.bits.red = 1;
    while (node != BTREE_ROOT && node->parent != 0 && node->parent->flags.bits.red) {
        if (node->parent == node->parent->parent->left) {
            uncle = node->parent->parent->right;
            if (uncle != 0 && uncle->flags.bits.red) {
                node->parent->flags.bits.red = 0;
                uncle->flags.bits.red = 0;
                node->parent->parent->flags.bits.red = 1;
                node = node->parent->parent;
            } else {
                if (node == node->parent->right) {
                    node = node->parent;
                    rotate_left(node);
                }
                node->parent->flags.bits.red = 0;
                node->parent->parent->flags.bits.red = 1;
                rotate_right(node->parent->parent);
            }
        } else {
            uncle = node->parent->parent->left;
            if (uncle != 0 && uncle->flags.bits.red) {
                node->parent->flags.bits.red = 0;
                uncle->flags.bits.red = 0;
                node->parent->parent->flags.bits.red = 1;
                node = node->parent->parent;
            } else {
                if (node == node->parent->left) {
                    node = node->parent;
                    rotate_right(node);
                }
                node->parent->flags.bits.red = 0;
                node->parent->parent->flags.bits.red = 1;
                rotate_left(node->parent->parent);
            }
        }
    }
    BTREE_ROOT->flags.bits.red = 0;
}

void render_mkatomic(RpAtomic* atomic) {
    MkSobj* sobj;
    unsigned char saved_flags = 0;
    int saved_state = 0;
    RwFrameGetLTM((RwFrame*)atomic->object.parent);
    sobj = MK_ATOMIC_PLUGIN(atomic)->sobj;
    if (sobj == 0) {
        set_render_state(0x14, 2);
        if ((int)f_render_all_atomics != 0) {
            saved_flags = atomic->object.flags;
            atomic->object.flags |= 4;
            atomic->renderCallBack(atomic);
            atomic->object.flags = saved_flags;
        } else if (RwCameraFrustumTestSphere(Camera, RpAtomicGetWorldBoundingSphere(atomic)) != 0) {
            atomic->renderCallBack(atomic);
        }
        return;
    }
    if (sobj->flags09_bits.bit4 || (int)f_render_all_atomics != 0 ||
        RwCameraFrustumTestSphere(Camera, RpAtomicGetWorldBoundingSphere(atomic)) != 0) {
        if (sobj->flags09_bits.bit7)
            set_render_state(8, 0);
        if (sobj->flags09_bits.bit6)
            set_render_state(6, 0);
        if (sobj->render_flags != 0) {
            set_render_state(0xA, sobj->render_flags >> 16);
            set_render_state(0xB, (unsigned short)sobj->render_flags);
        }
        if (sobj->flags09_bits.bit0) {
            RwEngineInstance->dOpenDevice.fpRenderStateGet(0xE, &saved_state);
            RwEngineInstance->dOpenDevice.fpRenderStateSet(0xE, 0);
        }
        if ((sobj->id_flags & 0x20000000) != 0 || sobj->owner->oid == 0x5004) {
            set_render_state(0x14, 1);
        } else {
            set_render_state(0x14, 2);
        }
        if ((int)f_render_all_atomics != 0) {
            saved_flags = atomic->object.flags;
            atomic->object.flags |= 4;
        }
        if (sobj->flags09_bits.has_pebbles) {
            last_pipeline_used = curr_pipeline_used;
            curr_pipeline_used = 0;
            atomic->renderCallBack(atomic);
        } else {
            atomic->renderCallBack(atomic);
        }
        if ((int)f_render_all_atomics != 0)
            atomic->object.flags = saved_flags;
        if (sobj->flags09_bits.bit7)
            set_render_state(8, 1);
        if (sobj->flags09_bits.bit6)
            set_render_state(6, 1);
        if (sobj->render_flags != 0) {
            set_render_state(0xA, 5);
            set_render_state(0xB, 6);
        }
        if (sobj->flags09_bits.bit0)
            RwEngineInstance->dOpenDevice.fpRenderStateSet(0xE, saved_state);
    }
}

void render_mkobj(MkObj* object) {
    TranslNodeFlags flags_pair[2];
    int clump_index;
    int priority;
    int node_index;
    float depth_bias;
    float depth;
    RwMatrix* camera_matrix;
    RwMatrix* atomic_ltm;
    if (!object->hide_flag_bits.hidden || (int)f_render_all_atomics != 0) {
        clump_index = 0;
        while (clump_index < object->clump_count) {
            RpClump* clump = object->clumps[clump_index];
            RwLLLink* link;
            if (clump != 0) {
                link = clump->atomicList.next;
                while (link != &clump->atomicList) {
                    RpAtomic* atomic = RpAtomicFromClumpLink(link);
                    if ((atomic->object.flags & 4) != 0) {
                        MksobjPluginData* plugin = MK_ATOMIC_PLUGIN(atomic);
                        MkSobj* sobj = plugin->sobj;
                        TranslSortNode* node;
                        priority = 0x10;
                        depth_bias = 0.0f;
                        if (sobj != 0) {
                            priority = sobj->priority;
                            depth_bias = sobj->z_offset;
                        } else if ((plugin->flags & 0x80000000) != 0) {
                            priority = 0x12;
                        }
                        flags_pair[0].word = 0;
                        if (priority == 0x12) {
                            camera_matrix = camera_mat;
                            atomic_ltm = RwFrameGetLTM((RwFrame*)atomic->object.parent);
                            depth =
                                (atomic_ltm->pos.z - camera_matrix->pos.z) * camera_matrix->at.z +
                                ((atomic_ltm->pos.x - camera_matrix->pos.x) * camera_matrix->at.x +
                                 (atomic_ltm->pos.y - camera_matrix->pos.y) * camera_matrix->at.y) +
                                depth_bias;
                        } else {
                            depth = 0.0f;
                        }
                        flags_pair[1] = flags_pair[0];
                        node_index = num_render_nodes;
                        if (node_index < 250) {
                            num_render_nodes = node_index + 1;
                            node = &transl_sort_nodes[node_index];
                            node->flags = flags_pair[1];
                            node->payload = atomic;
                            node->priority = priority;
                            node->depth = depth;
                            BTreeInsert(node);
                        }
                    }
                    link = link->next;
                }
            }
            clump_index++;
        }
    }
}
