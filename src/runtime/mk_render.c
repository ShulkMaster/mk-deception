#include "runtime/mk_render.h"
#include "runtime/cam.h"

#include "platform/display.h"
#include "rw/rwengine.h"
#include "runtime/mk_particle.h"
#include "runtime/mk_plugins.h"
#include "runtime/mk_vtbl.h"
#include "rw/rwframe.h"
#include "rw/rwcamera_internal.h"

struct TranslNodeFlagBits {
    unsigned char black : 1;
    unsigned char pfx : 1;
    unsigned char pfx_clone : 1;
    unsigned char pad_high : 5;
    unsigned char pad[3];
};

union TranslNodeFlags {
    unsigned int word;
    struct TranslNodeFlagBits bits;
};

struct TranslSortNode {
    struct TranslSortNode* parent;
    struct TranslSortNode* left;
    struct TranslSortNode* right;
    union TranslNodeFlags flags;
    int priority;
    float depth;
    void* payload;
};

extern int curr_pipeline_used;
extern int last_pipeline_used;

static struct TranslSortNode transl_sort_nodes[250];
static struct TranslSortNode* BTREE_ROOT;
static int num_render_nodes;
static int num_transl_callbacks;
static int in_batch;

static void BTreeInsert(struct TranslSortNode* node);
static void btree_render(struct TranslSortNode* node);

static inline void rotate_left(struct TranslSortNode* node) {
    struct TranslSortNode* right = node->right;
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

static inline void rotate_right(struct TranslSortNode* node) {
    struct TranslSortNode* left = node->left;
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
static void btree_render(struct TranslSortNode* node) {
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
            render_mkatomic(node->payload);
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
    struct TranslSortNode* node;
    struct {
        union TranslNodeFlags copy;
        union TranslNodeFlags source;
    } flags;
    PfxTransform* transform;
    RwMatrix* camera_matrix;
    float delta_x;
    float delta_y;
    float depth;
    int index;
    int node_index;
    int priority;
    if (clone == 0) {
        return;
    }
    candidate = clone->parent;
    if (candidate->hdr.vtbl == MK_VTABLE_ADDRESS(vtbl_pfx)) {
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
    transform = &runtime->transforms[index];
    camera_matrix = camera_mat;
    flags.source.word = 0;
    flags.source.bits.pfx_clone = 1;
    delta_y = transform->position.y;
    delta_y -= camera_matrix->pos.y;
    delta_x = transform->position.x;
    delta_x -= camera_matrix->pos.x;
    depth = transform->position.z;
    depth -= camera_matrix->pos.z;
    depth = depth * camera_matrix->at.z +
            (delta_x * camera_matrix->at.x + delta_y * camera_matrix->at.y);
    flags.copy = flags.source;
    priority = clone->priority;
    depth += clone->depth_bias;
    node_index = num_render_nodes;
    if (node_index >= (int)(sizeof(transl_sort_nodes) / sizeof(transl_sort_nodes[0]))) {
        return;
    }
    node = &transl_sort_nodes[node_index];
    num_render_nodes = node_index + 1;
    node->flags = flags.copy;
    node->payload = clone;
    node->priority = priority;
    node->depth = depth;
    BTreeInsert(node);
}

static inline int translucent_pfx_priority(const MkPfx* pfx)
{
    return pfx->priority;
}

void InsertPFXInTranslTree(MkHdr* pfx_hdr) {
    MkPfx* pfx = (MkPfx*)pfx_hdr;
    struct TranslSortNode* node;
    union TranslNodeFlags flags_pair[2];
    RwV3d origin;
    RwMatrix* camera_matrix;
    float depth;
    int node_index;
    int priority;
    if (pfx != 0 && !pfx->flag_bits.skip_translucent_sort && pfx->field_94 != 0) {
        float delta_x;
        float delta_y;

        mkpfx_get_origin(pfx, &origin.x);
        camera_matrix = camera_mat;
        flags_pair[1].word = 0;
        flags_pair[1].bits.pfx = 1;
        depth = origin.z - camera_matrix->pos.z;
        delta_x = origin.x - camera_matrix->pos.x;
        delta_y = origin.y - camera_matrix->pos.y;
        depth = depth * camera_matrix->at.z +
                (delta_x * camera_matrix->at.x + delta_y * camera_matrix->at.y);
        flags_pair[0] = flags_pair[1];
        depth += pfx->depth_bias;
        node_index = num_render_nodes;
        priority = translucent_pfx_priority(pfx);
        if (node_index < (int)(sizeof(transl_sort_nodes) / sizeof(transl_sort_nodes[0]))) {
            node = &transl_sort_nodes[node_index];
            num_render_nodes = node_index + 1;
            node->flags = flags_pair[0];
            node->payload = pfx;
            node->priority = priority;
            node->depth = depth;
            BTreeInsert(node);
        }
    }
}

static void BTreeInsert(struct TranslSortNode* node) {
    struct TranslSortNode* current = BTREE_ROOT;
    struct TranslSortNode* parent = 0;
    struct TranslSortNode* uncle;
    while (current != 0) {
        parent = current;
        if (node->priority > current->priority ||
            (node->priority == current->priority && node->depth <= current->depth)) {
            current = current->left;
        } else {
            current = current->right;
        }
    }
    node->parent = parent;
    node->left = 0;
    node->right = 0;
    if (parent == 0) {
        BTREE_ROOT = node;
    } else if (node->priority > parent->priority ||
               (node->priority == parent->priority && node->depth <= parent->depth)) {
        parent->left = node;
    } else {
        parent->right = node;
    }
    node->flags.bits.black = 0;
    while (node != BTREE_ROOT && node->parent != 0 && !node->parent->flags.bits.black) {
        if (node->parent == node->parent->parent->left) {
            uncle = node->parent->parent->right;
            if (uncle != 0 && !uncle->flags.bits.black) {
                node->parent->flags.bits.black = 1;
                uncle->flags.bits.black = 1;
                node->parent->parent->flags.bits.black = 0;
                node = node->parent->parent;
            } else {
                if (node == node->parent->right) {
                    node = node->parent;
                    rotate_left(node);
                }
                node->parent->flags.bits.black = 1;
                node->parent->parent->flags.bits.black = 0;
                rotate_right(node->parent->parent);
            }
        } else {
            uncle = node->parent->parent->left;
            if (uncle != 0 && !uncle->flags.bits.black) {
                node->parent->flags.bits.black = 1;
                uncle->flags.bits.black = 1;
                node->parent->parent->flags.bits.black = 0;
                node = node->parent->parent;
            } else {
                if (node == node->parent->left) {
                    node = node->parent;
                    rotate_right(node);
                }
                node->parent->flags.bits.black = 1;
                node->parent->parent->flags.bits.black = 0;
                rotate_left(node->parent->parent);
            }
        }
    }
    BTREE_ROOT->flags.bits.black = 1;
}

void render_mkatomic(RpAtomic* atomic) {
    MkSobj* sobj;
    RwSphere* sphere;
    unsigned char saved_flags = 0;
    int saved_state = 0;
    RwFrameGetLTM(atomic->object.parent);
    sobj = MK_ATOMIC_PLUGIN(atomic)->sobj;
    if (sobj == 0) {
        set_render_state(0x14, 2);
        if ((int)f_render_all_atomics != 0) {
            saved_flags = atomic->object.flags;
            atomic->object.flags |= 4;
            atomic->renderCallBack(atomic);
            atomic->object.flags = saved_flags;
        } else {
            sphere = RpAtomicGetWorldBoundingSphere(atomic);
            if (RwCameraFrustumTestSphere(Camera, sphere) != 0) {
                atomic->renderCallBack(atomic);
            }
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

static inline void insert_translucent_atomic(const union TranslNodeFlags* source_flags,
                                              RpAtomic* atomic, int priority, float depth)
{
    union TranslNodeFlags flags;
    int node_index = num_render_nodes;
    flags = *source_flags;
    if (node_index < (int)(sizeof(transl_sort_nodes) / sizeof(transl_sort_nodes[0]))) {
        struct TranslSortNode* node;
        num_render_nodes = node_index + 1;
        node = &transl_sort_nodes[node_index];
        node->flags = flags;
        node->payload = atomic;
        node->priority = priority;
        node->depth = depth;
        BTreeInsert(node);
    }
}

/* TODO: [breakthrough needed] 98.99%; dot product and sentinel recovered; flag slots and insertion/iterator coloring remain. */
void render_mkobj(MkObj* object) {
    RwLLLink* link;
    RwLLLink* list_head;
    int clump_index;
    int priority;
    float depth_bias;
    float depth;
    RwMatrix* camera_matrix;
    RwMatrix* atomic_ltm;
    if (!object->hide_flag_bits.hidden || (int)f_render_all_atomics != 0) {
        clump_index = 0;
        while (clump_index < object->clump_count) {
            RpClump* clump = object->clumps[clump_index];
            if (clump != 0) {
                link = clump->atomicList.next;
                list_head = &clump->atomicList;
                while (link != list_head) {
                    RpAtomic* atomic = rpAtomicFromClumpNode(link);
                    if ((atomic->object.flags & 4) != 0) {
                        MksobjPluginData* plugin = MK_ATOMIC_PLUGIN(atomic);
                        MkSobj* sobj = plugin->sobj;
                        union TranslNodeFlags flags;
                        priority = 0x10;
                        depth_bias = 0.0f;
                        if (sobj != 0) {
                            priority = sobj->priority;
                            depth_bias = sobj->z_offset;
                        } else if ((plugin->flags & 0x80000000) != 0) {
                            priority = 0x12;
                        }
                        flags.word = 0;
                        if (priority == 0x12) {
                            float delta_x, delta_y, delta_z;
                            camera_matrix = camera_mat;
                            atomic_ltm = RwFrameGetLTM(atomic->object.parent);
                            delta_x = atomic_ltm->pos.x - camera_matrix->pos.x;
                            delta_y = atomic_ltm->pos.y - camera_matrix->pos.y;
                            delta_z = atomic_ltm->pos.z - camera_matrix->pos.z;
                            depth =
                                delta_z * camera_matrix->at.z +
                                (delta_x * camera_matrix->at.x +
                                 delta_y * camera_matrix->at.y);
                            depth += depth_bias;
                        } else {
                            depth = 0.0f;
                        }
                        insert_translucent_atomic(&flags, atomic, priority, depth);
                    }
                    link = link->next;
                }
            }
            clump_index++;
        }
    }
}
