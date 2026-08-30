#include "platform/main.h"

#include "game/game_info.h"
#include "game/memcard.h"
#include "game/plyrprofile.h"
#include "game/settings.h"
#include "libmkparticle/particle.h"
#include "msl/mslcore.h"
#include "platform/display.h"
#include "platform/gcARam.h"
#include "platform/gcInit.h"
#include "platform/gcio.h"
#include "runtime/mk_fileinfo.h"
#include "runtime/mk_hwfile.h"
#include "runtime/mk_mem.h"
#include "runtime/mk_pdata.h"
#include "runtime/mk_vtbl.h"
#include "runtime/utils.h"

extern int __setjmp(void *buffer);

extern void longjmp(void *buffer, int value);

extern unsigned short GXMathSqrtTable[];

extern void gc_start_reset_watch(void);

extern void tile_image(void *image);

extern void gc_native_display_init(void);

extern void gc_grab_renderpipe(void);

extern void gc_native_display_render_image(void);

extern void gc_release_renderpipe(void);

extern void gc_native_display_pass_to_RW(void);

extern void mwMemUserConfigInitMemSystem(void);

extern void mwfile_init_for_mk(int use_debug_filesystem);

extern void mkpfx_init(void);

extern int refresh_rate(void);

extern void mk_system_init(void);

extern void init_section_system(void);

extern void init_switch_log(void);

extern void init_pakfile_system(void);

extern int init_sounds(void);

extern void konquest_state_init(void);

extern void setup_sound_banks(int load_mode);

extern void load_systemart_phase_1(void);

extern void load_systemart_phase_2(void);

extern void get_clean_system(void);

extern void sgenrand(unsigned int seed);

extern float p_attract_mode(void);

extern void scan_remote_switches(void);

extern int is_controller_removed(void);

extern int ck_mcard_msg(void);

extern void mcard_msg_handler(void);

extern void unstack_switches(void);

extern void mkpfx_set_environment(void);

extern void mwFileTick(void);

extern void Render(void);

extern unsigned char loading_image[];

static unsigned char exec_loop_jump_buffer[0x190];

int jump_target_mode = 0xc;
int gap_08_80510234_sbss;
int exec_tick_ctr;
int game_tick_ctr;
float msecs_per_tick;
int mode_of_play;
int jmp_where_id;
void *mab_generic_pdata;
void *empty_pdata;
float game_speed;
float inverse_game_speed;
float sqrt_game_speed;
extern int gameart_is_loaded;

void gamelogic_jump(int mode, MainProcEntryFn entry) {
    static MkProc *proc;
    static MkProcEntryFn func_addr;

    reset_game_state();
    push_game_state(1);
    turn_controllers_off();
    func_addr = entry;
    jump_target_mode = mode;
    if (aproc != 0) {
        aproc->vtbl->system_stack();
    }
    get_clean_system();
    proc = _create_mkproc_generic_bigstack(0x2001, 0x1f, func_addr, 0, 0);
    turn_controllers_on();
    longjmp(exec_loop_jump_buffer, jump_target_mode);
}

typedef union MainFloatBits {
    float value;
    unsigned int bits;
} MainFloatBits;

static inline float main_sqrt(float value) {
    MainFloatBits input;
    MainFloatBits estimate;

    input.value = value;
    estimate.bits = (unsigned int) GXMathSqrtTable[(input.bits >> 11) & 0x1fff] << 8;
    estimate.bits |= (((input.bits & 0x7f800000) + 0x3f800000) >> 1) & 0x7f800000;
    return 0.5f * (estimate.value * (3.0f - (estimate.value * estimate.value) / value));
}

int main(void) {
    gc_start_reset_watch();
    gc_aram_init();
    mwMemUserConfigInitMemSystem();
    gc_aram_mwmem_heap_setup();
    tile_image(loading_image);
    gc_native_display_init();
    gc_grab_renderpipe();
    gc_native_display_render_image();
    gc_release_renderpipe();
    gc_native_display_pass_to_RW();
    setup_fixed_block_heaps();
    start_usec_timer(3);
    if (hardware_init() == 0) {
        return 0;
    }
    if (init_display() == 0) {
        return 0;
    }
    mwfile_init_for_mk(0);
    mkpfx_init();
    initialize_language_settings();
    reset_game_speed();
    msecs_per_tick = 1000.0f / (float) refresh_rate();
    reset_game_state();
    push_game_state(0);
    mk_system_init();
    init_section_system();
    init_file_loading_table();
    init_switch_log();
    mk_hwfile_init();
    init_pakfile_system();
    if (init_controller() == 0) {
        return 0;
    }
    if (init_memcard() == 0) {
        return 0;
    }
    init_sounds();
    init_player_profiles();
    init_gsettings();
    konquest_state_init();
    setup_sound_banks(0);
    load_systemart_phase_1();
    get_clean_system();
    load_systemart_phase_2();
    gameart_is_loaded = 1;
    pfxsystem_init();
    sgenrand((unsigned int) stop_usec_timer(3));
    _create_mkproc_generic_bigstack(0x2001, 0x1f, p_attract_mode, 0, 0);
    exec_tick_ctr = 0;
    game_tick_ctr = 0;
    jmp_where_id = __setjmp(exec_loop_jump_buffer);

    for (;;) {
        exec_tick_ctr += 1;
        if (!g_game_info.pause_flag_bits.controller_disable_guard && network_pause_procs == 0) {
            game_tick_ctr += 1;
        }
        do_delayed_mem_frees();
        scan_switches();
        scan_remote_switches();
        service_game_timers();
        if (is_controller_removed() == 0 && ck_mcard_msg() != 0) {
            mcard_msg_handler();
        }
        if (!g_game_info.pause_flag_bits.controller_disable_guard) {
            unstack_switches();
        }
        mkpfx_set_environment();
        mkproc_dispatch();
        mwFileTick();
        mslTick();
        if (World != 0) {
            Render();
        }
        purge_master_clean_up_list();
    }
}

void reset_game_speed(void) {
    if (refresh_rate() == 50) {
        game_speed = 1.2f;
        inverse_game_speed = 0.8333333f;
        sqrt_game_speed = main_sqrt(1.2f);
    } else {
        game_speed = 1.0f;
        inverse_game_speed = 1.0f;
        sqrt_game_speed = main_sqrt(1.0f);
    }
}

float get_inverse_game_speed(void) {
    return inverse_game_speed;
}

float get_game_speed(void) {
    return game_speed;
}

void set_game_speed(float speed) {
    MainFloatBits estimate;
    MainFloatBits input;
    float correction;
    float estimate_squared;
    float square_root = 0.0f;

    game_speed = speed;
    if (speed != 0.0f) {
        input.value = speed;
        inverse_game_speed = 1.0f / speed;
        square_root = speed <= 0.0f
                          ? 0.0f
                          : (estimate.bits =
                                 (unsigned int)GXMathSqrtTable[
                                     (input.bits >> 11) & 0x1fff]
                                 << 8,
                             estimate.bits |=
                                 (((input.bits & 0x7f800000) + 0x3f800000) >>
                                  1) &
                                 0x7f800000,
                             estimate_squared = estimate.value * estimate.value,
                             correction = 3.0f - estimate_squared / speed,
                             0.5f * (estimate.value * correction));
        sqrt_game_speed = square_root;
    } else {
        inverse_game_speed = 1.0e9f;
        sqrt_game_speed = square_root;
    }
}

int gameart_is_loaded;
const float gap_09_805117CC_sdata2 = 0.0f;
