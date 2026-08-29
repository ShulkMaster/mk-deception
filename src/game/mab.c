#include "game/game_info.h"
#include "game/mab.h"

#include "runtime/asset.h"
#include "runtime/fonts.h"
#include "runtime/mk_obj.h"
#include "runtime/mk_pdata.h"
#include "runtime/mk_proc.h"
#include "runtime/mk_struct.h"
#include "runtime/plyr_info.h"
#include "runtime/sound.h"
#include "runtime/utils.h"

typedef struct LightningAlphaStep {
    int alpha;
    unsigned int ticks;
} LightningAlphaStep;

/* pdata for mkproc 0x2099 (lightning strike). */
typedef struct LightningPdata {
    MkHdr hdr;        /* +0x00 */
    Vec position;     /* +0x08 */
    PlyrInfo* owner;  /* +0x14 - field_04 selects art slot */
} LightningPdata;

typedef char LightningAlphaStepSizeCheck[
    sizeof(LightningAlphaStep) == 0x08 ? 1 : -1];
typedef char LightningPdataSizeCheck[
    sizeof(LightningPdata) == 0x18 ? 1 : -1];

static const char STR_BOLT_OBJECT[] = "BOLT_OBJECT";

static const float kProcSleepTicks = 20.0f;
static const float kCameraShakeAmount = 0.02f;
static const float kFadeSleepTicks = 1.0f;
static const float kProcReturnNegOne = -1.0f;

LightningAlphaStep lightning_alpha[] = {
    {0xFF, 6},
    {0, 4},
    {0xFF, 3},
    {0, 2},
    {0xFF, 3},
    {0, 1},
    {-1, 0xFFFFFFFF},
};

extern float _mkproc_sleep_ticks;

void shake_camera(int frames, float amount);
void kill_all_fstyle_signs(void);
extern float p_move_pbars_off_screen(void);

static inline void mkproc_sleep(void) {
    aproc->vtbl->sleep();
}

/* mkproc 0x2099: spawn BOLT_OBJECT, run alpha table, fade out, destroy. */
static float p_lightning_strike_effect(void) {
    LightningPdata* pdata;
    MkObj* bolt;
    MkSobj* sobj;
    int alpha;
    int art_slot;
    int step_index;
    LightningAlphaStep* step;
    LightningAlphaStep* steps;
    PlyrInfo* owner;

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
        obj_set_pos(bolt, &pdata->position);
        sobj = obj_create_sobjs_by_id(bolt, 1);
        sobj->flags09_bits.bit7 = 1;
        sobj_set_priority(sobj, 0x13);
        update_obj_pos(bolt);

        if (sobj != 0) {
            shake_camera(2, kCameraShakeAmount);
            sobj->flags09_bits.bit5 = 1;

            steps = lightning_alpha;
            for (step_index = 0;
                 steps[step_index].alpha > -1;
                 step_index++) {
                step = &steps[step_index];
                obj_set_sobj_alpha(bolt, 1, step->alpha);
                _mkproc_sleep_ticks = (float)step->ticks;
                mkproc_sleep();
            }
        }

        alpha = 0xFF;
        while (alpha > 0) {
            obj_set_sobj_alpha(bolt, 1, alpha);
            _mkproc_sleep_ticks = kFadeSleepTicks;
            alpha -= 8;
            mkproc_sleep();
        }

        if (bolt->hdr.instance != 0) {
            bolt->hdr.typed_vtbl->destroy(&bolt->hdr);
        }
    }
    return kProcReturnNegOne;
}

void do_lightning_strike(PlyrInfo* owner, Vec* position) {
    LightningPdata* pdata;

    if (_create_mkproc_generic_tinystack(0x2099, 0x1F, p_lightning_strike_effect, 0x18,
                                         (MkHdr**)&pdata) == 0) {
        return;
    }
    pdata->position.x = position->x;
    pdata->position.y = g_game_info.field_34;
    pdata->position.z = position->z;
    pdata->owner = owner;
}

void setup_screen_for_fatality(void) {
    proc_create(p_move_pbars_off_screen, 0x2078);
    kill_all_fstyle_signs();
    del_string_obj_by_id(0x201E);
}
