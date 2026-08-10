#include "runtime/mk_render.h"

#include "platform/display.h"
#include "runtime/mk_plugins.h"
#include "runtime/mk_vtbl.h"

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

typedef struct PfxSortTransformView {
    char pad00[0x30];
    RwV3d position;
    char pad3C[0x0C];
} PfxSortTransformView; /* 0x48 */

typedef struct PfxSortRuntimeView {
    char pad00[0x54];
    int active;          /* +0x54 / PfxSortView +0x94 */
    int transform_index; /* +0x58 / PfxSortView +0x98 */
    char pad5C[0x14];
    PfxSortTransformView transforms[1]; /* +0x70 */
} PfxSortRuntimeView;

typedef struct PfxSortView {
    char pad00[8];
    union {
        unsigned char flags_08;
        struct {
            unsigned char bit7 : 1;
            unsigned char bit6 : 1;
            unsigned char bit5 : 1;
            unsigned char bit4 : 1;
            unsigned char skip_translucent_sort : 1;
            unsigned char pad_low : 3;
        } flags_08_bits;
    };
    char pad09[0x1F];
    float depth_bias; /* +0x28 */
    int priority;     /* +0x2C */
    char pad30[0x10];
    PfxSortRuntimeView runtime; /* +0x40 */
} PfxSortView;

typedef struct PfxCloneSortView {
    MkVtable5* vtbl;
    unsigned int instance;
    char pad08[4];
    PfxSortView* pfx; /* +0x0C */
    char pad10[0x14];
    float depth_bias; /* +0x24 */
    int priority;     /* +0x28 */
} PfxCloneSortView;

typedef struct RenderEngineView {
    char pad00[0x20];
    int (*render_state_set)(int state, int value, void* engine);
    void (*render_state_get)(int state, void* out, void* engine);
} RenderEngineView;

extern RwCamera* Camera;
extern RwMatrix* camera_mat;
extern int curr_pipeline_used;
extern int last_pipeline_used;
extern unsigned long f_render_all_atomics;
extern RenderEngineView* RwEngineInstance;

void atomic_set_transl_flag(RpAtomic* atomic);
void obj_set_rw_lights(MkObj* object);
void pfx_start_batch(void);
void pfx_end_batch(void);
void render_pfx(void* pfx);
void render_pfx_clone(void* clone);
void mkpfx_get_origin(PfxSortView* pfx, RwV3d* origin);
RwMatrix* RwFrameGetLTM(RwFrame* frame);
RwSphere* RpAtomicGetWorldBoundingSphere(RpAtomic* atomic);
int RwCameraFrustumTestSphere(RwCamera* camera, RwSphere* sphere);

static TranslSortNode transl_sort_nodes[250];
static int in_batch;
static int num_transl_callbacks;
static int num_render_nodes;
static TranslSortNode* BTREE_ROOT;

static void BTreeInsert(TranslSortNode* node, int node_offset);
static void btree_render(TranslSortNode* node);

static int node_precedes(const TranslSortNode* a, const TranslSortNode* b) {
    return a->priority > b->priority || (a->priority == b->priority && a->depth >= b->depth);
}

static void rotate_left(TranslSortNode* node) {
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

static void rotate_right(TranslSortNode* node) {
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
            obj_set_rw_lights(MK_CLUMP_PLUGIN((RpClump*)((RpAtomic*)node->payload)->lights)->owner);
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

void InsertPFXCloneInTranslTree(void* clone_ptr) {
    PfxCloneSortView* clone = (PfxCloneSortView*)clone_ptr;
    PfxSortView* candidate;
    PfxSortView* valid_pfx;
    PfxSortRuntimeView* runtime;
    TranslSortNode* node;
    TranslNodeFlags flags_pair[2];
    RwV3d* position;
    RwMatrix* camera_matrix;
    float depth;
    int index;
    int node_index;
    int node_offset;
    if (clone == 0) {
        return;
    }
    candidate = clone->pfx;
    if (*(MkVtable5**)candidate == &vtbl_pfx) {
        valid_pfx = candidate;
    } else {
        valid_pfx = 0;
    }
    if (valid_pfx == 0) {
        if (clone->instance != 0) {
            ((int (*)(PfxCloneSortView*))clone->vtbl->destroy)(clone);
        }
        return;
    }
    runtime = &candidate->runtime;
    if (runtime->active == 0) {
        return;
    }
    index = runtime->transform_index;
    position = &runtime->transforms[index].position;
    camera_matrix = camera_mat;
    flags_pair[1].word = 0;
    flags_pair[1].bits.pfx_clone = 1;
    depth = (position->z - camera_matrix->pos.z) * camera_matrix->at.z +
            ((position->x - camera_matrix->pos.x) * camera_matrix->at.x +
             (position->y - camera_matrix->pos.y) * camera_matrix->at.y);
    flags_pair[0] = flags_pair[1];
    depth += clone->depth_bias;
    node_index = num_render_nodes;
    if (node_index >= 250) {
        return;
    }
    node_offset = node_index * sizeof(TranslSortNode);
    num_render_nodes = node_index + 1;
    node = &transl_sort_nodes[node_index];
    node->flags = flags_pair[0];
    node->payload = clone;
    node->priority = clone->priority;
    node->depth = depth;
    BTreeInsert(node, node_offset);
}

void InsertPFXInTranslTree(void* pfx_ptr) {
    PfxSortView* pfx = (PfxSortView*)pfx_ptr;
    TranslSortNode* node;
    TranslNodeFlags flags_pair[2];
    RwV3d origin;
    RwMatrix* camera_matrix;
    float depth;
    int node_index;
    int node_offset;
    int priority;
    if (pfx != 0 && !pfx->flags_08_bits.skip_translucent_sort && pfx->runtime.active != 0) {
        mkpfx_get_origin(pfx, &origin);
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
            node_offset = node_index * sizeof(TranslSortNode);
            num_render_nodes = node_index + 1;
            node = &transl_sort_nodes[node_index];
            node->flags = flags_pair[0];
            node->payload = pfx;
            node->priority = priority;
            node->depth = depth;
            BTreeInsert(node, node_offset);
        }
    }
}

static void BTreeInsert(TranslSortNode* node, int node_offset) {
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
            RwEngineInstance->render_state_get(0xE, &saved_state, RwEngineInstance);
            RwEngineInstance->render_state_set(0xE, 0, RwEngineInstance);
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
            RwEngineInstance->render_state_set(0xE, saved_state, RwEngineInstance);
    }
}

void render_mkobj(MkObj* object) {
    TranslNodeFlags flags_pair[2];
    int clump_index;
    int priority;
    int node_index;
    int node_offset;
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
                            node_offset = node_index * sizeof(TranslSortNode);
                            num_render_nodes = node_index + 1;
                            node = &transl_sort_nodes[node_index];
                            node->flags = flags_pair[1];
                            node->payload = atomic;
                            node->priority = priority;
                            node->depth = depth;
                            BTreeInsert(node, node_offset);
                        }
                    }
                    link = link->next;
                }
            }
            clump_index++;
        }
    }
}
