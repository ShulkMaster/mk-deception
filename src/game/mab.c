#include "game/game_info.h"

#include "runtime/mk_obj.h"
#include "runtime/mk_proc.h"
#include "runtime/mk_struct.h"
#include "runtime/plyr_info.h"

typedef struct LightningAlphaStep {
    int alpha;
    int ticks;
} LightningAlphaStep;

/* Local view -- avoid mk_vtbl.h vs mk_struct.h destroy prototype clash. */
typedef struct MkVtableMkprocLocal {
    int (*fn0)(void);
    int (*fn1)(void);
    int (*fn2)(void);
    int (*fn3)(void);
    int (*destroy)(MkProc* proc);
    int (*dispatch)(void);
    int (*sleep)(void);
    int (*system_stack)(void);
    int (*local_stack)(void);
    float (*jump_sleep)(MkProcEntryFn entry);
} MkVtableMkprocLocal;

/* pdata for mkproc 0x2099 (lightning strike). */
typedef struct LightningPdata {
    MkHdr hdr;        /* +0x00 */
    float pos_x;     /* +0x08 */
    float pos_y;     /* +0x0C */
    float pos_z;     /* +0x10 */
    PlyrInfo* owner; /* +0x14 - field_04 selects art slot */
} LightningPdata;

static const char STR_BOLT_OBJECT[] = "BOLT_OBJECT";

const int gap_04_8031469C_rodata = 0;
static const float kProcSleepTicks = 20.0f;
static const float kCameraShakeAmount = 0.02f;
static const float kFadeSleepTicks = 1.0f;
static const float kProcReturnNegOne = -1.0f;
static const double kIntToFloatBias = 4503599627370496.0;

LightningAlphaStep lightning_alpha[] = {
    {0xFF, 6},
    {0, 4},
    {0xFF, 3},
    {0, 2},
    {0xFF, 3},
    {0, 1},
    {-1, -1},
};

extern float _mkproc_sleep_ticks;

void* load_named_model_from_slot(int slot, const char* name, int arg2, int arg3);
void snd_req(int sound_id);
void shake_camera(float amount, int frames);
/* Retail leaves success in r3; Matching header is void -- local view for the check. */
int _create_mkproc_generic_tinystack(int proc_id, int priority, MkProcEntryFn proc_fn,
                                     int pdata_size, MkHdr** pdata_out);
void proc_create(void* proc_fn, int proc_id);
void kill_all_fstyle_signs(void);
void del_string_obj_by_id(int id);
extern void p_move_pbars_off_screen(void);

static void mkproc_sleep(void) {
    ((MkVtableMkprocLocal*)aproc->vtbl)->sleep();
}

/* mkproc 0x2099: spawn BOLT_OBJECT, run alpha table, fade out, destroy. */
static float p_lightning_strike_effect(void) {
    LightningPdata* pdata;
    MkObj* bolt;
    MkSobj* sobj;
    int alpha;
    int art_slot;
    PlyrInfo* owner;
    int (*destroy_fn)(MkObj*);
    double int_bias;
    LightningAlphaStep* step;

    _mkproc_sleep_ticks = kProcSleepTicks;
    pdata = (LightningPdata*)apdata;
    mkproc_sleep();

    owner = pdata->owner;
    art_slot = 0x4000B;
    if (owner->controller_slot == 0) {
        art_slot = 0x3000B;
    }

    bolt = (MkObj*)load_named_model_from_slot(art_slot, STR_BOLT_OBJECT, 0x2099, 0);
    if (bolt != 0) {
        snd_req(0x2B3);
        insert_fgnd_mkobj(bolt);
        obj_set_pos(bolt, (const Vec*)&pdata->pos_x);
        sobj = (MkSobj*)obj_create_sobjs_by_id(bolt, 1);
        /* flags09 bit7 via rlwimi (retail). */
        sobj->flags09 = (unsigned char)((sobj->flags09 & 0x7F) | 0x80);
        sobj_set_priority(sobj, 0x13);
        update_obj_pos(bolt);

        if (sobj != 0) {
            shake_camera(kCameraShakeAmount, 2);
            /* flags09 bit5 via rlwimi (retail). */
            sobj->flags09 = (unsigned char)((sobj->flags09 & 0xDF) | 0x20);

            int_bias = kIntToFloatBias;
            step = lightning_alpha;
            while (step->alpha > -1) {
                union {
                    double d;
                    unsigned int i[2];
                } conv;

                obj_set_sobj_alpha(bolt, 1, step->alpha);
                conv.i[0] = 0x43300000u;
                conv.i[1] = (unsigned int)step->ticks;
                _mkproc_sleep_ticks = (float)(conv.d - int_bias);
                mkproc_sleep();
                step++;
            }
        }

        alpha = 0xFF;
        while (alpha > 0) {
            obj_set_sobj_alpha(bolt, 1, alpha);
            _mkproc_sleep_ticks = kFadeSleepTicks;
            mkproc_sleep();
            alpha -= 8;
        }

        if (bolt->hdr.instance != 0) {
            destroy_fn = (int (*)(MkObj*))bolt->hdr.vtbl->destroy;
            destroy_fn(bolt);
        }
    }
    return kProcReturnNegOne;
}

void do_lightning_strike(PlyrInfo* owner, const Vec* position) {
    LightningPdata* pdata;
    int ok;

    ok = _create_mkproc_generic_tinystack(0x2099, 0x1F, p_lightning_strike_effect, 0x18,
                                          (MkHdr**)&pdata);
    if (ok == 0) {
        return;
    }
    pdata->pos_x = position->x;
    pdata->pos_y = g_game_info.field_34;
    pdata->pos_z = position->z;
    pdata->owner = owner;
}

void setup_screen_for_fatality(void) {
    proc_create(p_move_pbars_off_screen, 0x2078);
    kill_all_fstyle_signs();
    del_string_obj_by_id(0x201E);
}
