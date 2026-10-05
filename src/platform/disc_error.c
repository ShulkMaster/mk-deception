#include "platform/disc_error.h"

#include "game/controller.h"
#include "libmkparticle/pfxfont.h"
#include "dolphin/dvd.h"
#include "dolphin/os.h"
#include "dolphin/vi.h"
#include "platform/display.h"
#include "platform/gcutils.h"
#include "platform/main.h"
#include "platform/display_metrics.h"
#include "runtime/fonts.h"
#include "runtime/sound.h"
#include "runtime/mk_mem.h"
#include "rw/rwcamera_internal.h"

struct DiscErrorMapEntry {
    int error;
    int message;
};

extern void gc_native_display_render_text(const char* text);

static int fs_error_handler(int error, const char* text);

struct DiscErrorMapEntry error_map[30] = {
    {-1, 1},  {-2, 1},  {-3, 1},  {-4, 1},  {-5, 1},  {-6, 1},
    {-7, 1},  {-8, 1},  {-9, 1},  {-10, 1}, {-11, 1}, {-12, 1},
    {-13, 1}, {-14, 1}, {-15, 1}, {-16, 1}, {-17, 1}, {-18, 1},
    {-19, 6}, {-20, 4}, {-21, 5}, {-22, 3}, {-23, 2}, {-24, 1},
    {-25, 1}, {-26, 1}, {-27, 1}, {-28, 1}, {-29, 1}, {-1, 1},
};

const char* disc_error_string_table[42] = {
    "There's a problem with the disc you're using.\n\n       It may be dirty or damaged.",
    "Hay un problema con el disco en uso.\n\n        Puede estar sucio o da\361ado.",
    "Es gibt ein Problem mit der von Ihnen verwendeten Disc.\n\n       Sie ist m\366glicherweise schmutzig oder besch\344digt.",
    "Le disque utilis\351 pr\351sente une anomalie.\n\n       Il est peut-\352tre sale ou endommag\351.",
    "Il disco in uso ha qualche problema.\n\n       Potrebbe essere sporco o danneggiato.",
    "There's a problem with the disc you're using.\n\n       It may be dirty or damaged.",
    "The Disc Cover is open. If you want\nto continue the game, please close the\nDisc Cover.",
    "La tapa est\341 abierta. Si quieres\ncontinuar jugando, cierra la\ntapa.",
    "Der Disc-Deckel ist geoeffnet. Wenn Sie mit\ndem Spiel fortfahren wollen, schliessen Sie\nden Disc-Deckel.",
    "Le couvercle est ouvert. Si vous souhaitez\ncontinuer la partie, veuillez fermer le\ncouvercle.",
    "Il Coperchio disco \350 aperto. Se vuoi\ncontinuare il gioco, chiudi il\nCoperchio disco.",
    "The Disc Cover is open. If you want\nto continue the game, please close the\nDisc Cover.",
    "The Game Disc could not be read. Please\nread the Nintendo GameCube Instruction\nBooklet for more information.",
    "No puede leerse el disco.\nLee el manual de instrucciones de\nNintendo GameCubeTM para m\341s informaci\363n.",
    "Die Game-Disc konnte nicht gelesen werden.\nBitte lesen Sie die Nintendo GamecubeTM-\nBedienungsanleitung f\374r weitere Informationen.",
    "Imposs. de lire le disque de jeu. Veuillez lire\nle manuel d'instructions Nintendo GameCubeTM\npour plus de renseignements.",
    "Disco di gioco non leggibile.Consulta il\nlibretto di istruzioni del Nintendo GameCubeTM\nper ulteriori informazioni.",
    "The Game Disc could not be read. Please\nread the Nintendo GameCube Instruction\nBooklet for more information.",
    "Please insert the Mortal Kombat\nDeception Game Disc.",
    "Inserta el Mortal Kombat\nDeception disco del juego.",
    "Bitte legen Sie die Mortal Kombat\nDeception Game-Disc ein.",
    "Veuillez ins\351rer le Mortal Kombat\nDeception disque de jeu.",
    "Inserisci il Mortal Kombat\nDeception Disco di gioco.",
    "Please insert the Mortal Kombat\nDeception Game Disc.",
    "This is not the Mortal Kombat Deception\nGame Disc. Please insert the Mortal Kombat\nDeception Game Disc.",
    "\311ste no es el Mortal Kombat Deception\ndisco del juego. Inserta el Mortal Kombat\nDeception disco del juego.",
    "Das ist nicht die Mortal Kombat Deception\nGame-Disc. Bitte legen Sie die Mortal Kombat\nDeception Game-Disc ein.",
    "Ceci n'est pas le Mortal Kombat Deception\ndisque du jeu. Veuillez ins\351rer le Mortal Kombat\nDeception disque du jeu.",
    "Questo non \350 il Mortal Kombat Deception\nDisco di gioco. Inserisci il Mortal Kombat\nDeception Disco di gioco.",
    "This is not the Mortal Kombat Deception\nGame Disc. Please insert the Mortal Kombat\nDeception Game Disc.",
    "An error has occurred. Turn the power off\nand refer to the Nintendo GameCube\nInstruction Booklet for further instructions.",
    "Se ha producido un error. Apaga la consola\ny consulta el manual de instrucciones de\nNintendo GameCubeTM para m\341s informaci\363n.",
    "Ein Fehler ist aufgetreten. Schalten Sie\nden Strom aus und lesen Sie die\nNintendo GamecubeTM-Bedienungsanleitung\nf\374r weitere Informationen.",
    "Une erreur est survenue. Veuillez \351teindre\nla console et vous r\351f\351rer au manuel\nd'instructions Nintendo GameCubeTM pour plus\nde renseignements.",
    "Si \350 verificato un errore. Spegni la console\ne consulta il libretto di istruzioni del\nNintendo GameCubeTM per ulteriori informazioni.",
    "An error has occurred. Turn the power off\nand refer to the Nintendo GameCube\nInstruction Booklet for further instructions.",
    "An error has occurred. Turn the power off and\nrefer to the Nintendo GameCube Instruction\nBooklet for further instructions.",
    "Se ha producido un error. Apaga la consola\ny consulta el manual de instrucciones de\nNintendo GameCubeTM para m\341s informaci\363n.",
    "Ein Fehler ist aufgetreten. Schalten Sie\nden Strom aus und lesen Sie die\nNintendo GamecubeTM-Bedienungsanleitung\nf\374r weitere Informationen.",
    "Une erreur est survenue. Veuillez \351teindre\nla console et vous r\351f\351rer au manuel\nd'instructions Nintendo GameCubeTM pour plus\nde renseignements.",
    "Si \350 verificato un errore. Spegni la console\ne consulta il libretto di istruzioni del\nNintendo GameCubeTM per ulteriori informazioni.",
    "An error has occurred. Turn the power off and\nrefer to the Nintendo GameCube Instruction\nBooklet for further instructions.",
};

static int (*async_error_handler)(int error, const char* text) = fs_error_handler;

static int in_error_handler;
int disc_error_occurred;

static __declspec(section ".sdata2") RwRGBA disc_clear_color = {0, 0, 0, 0xff};

static inline void render_disc_message(PfxFontString* string, const char* text) {
    PfxFontSlot* font;
    int height;
    int width;
    int top;
    int left;
    int frame;

    font = load_font(6);
    height = pfxfont_get_height(font->metrics, text);
    width = pfxfont_get_width(font->metrics, text);
    top = (screen_height - height) / 2;
    left = (screen_width - width) / 2;
    pfxfont_string_init(string);
    pfxfont_string_set(string, font, text, width, 1);

    for (frame = 0; frame < 3; frame++) {
        RwRGBA clear_color = disc_clear_color;

        RwCameraClear(Camera, &clear_color, 7);
        RwCameraBeginUpdate(Camera);
        pfxfont_begin_render();
        pfxfont_string_render(string, left, (float)screen_height - (float)(top + height));
        pfxfont_end_render();
        RwCameraEndUpdate(Camera);
        RwCameraShowRaster(Camera, 0, 1);
    }
    pfxfont_string_cleanup(string);
    do_delayed_mem_frees();
}

static inline void show_disc_message(const char* text) {
    PfxFontString string;

    gc_grab_renderpipe();
    if (gameart_is_loaded != 0) {
        render_disc_message(&string, text);
    } else {
        gc_native_display_render_text(text);
    }
    gc_release_renderpipe();
}

/* TODO: [near miss] 99.94%; all three messages share show_disc_message, but the first expansion keeps
 * top/left in r29/r26 where retail has r26/r29 (loop and recovery copies agree); context-only coloring. */
static int fs_error_handler(int error, const char* text) {
    int drive_status;

    if (in_error_handler != 0) {
        while (in_error_handler != 0) {
            OSYieldThread();
        }
        return 0;
    }

    in_error_handler = 1;
    drive_status = DVDGetDriveStatus();
    switch (drive_status) {
    case -1:
    case 4:
    case 5:
    case 6:
    case 11:
        break;
    default:
        in_error_handler = 0;
        return 0;
    }

    pause_all_game_sounds();
    turn_all_rumble_motors_off();
    VISetBlack(0);
    gc_movie_start();
    if (drive_status == -1) {
        gc_stop_reset_watch();
    }

    show_disc_message(text);

    for (;;) {
        handle_reset_switch();
        if (drive_status != DVDGetDriveStatus()) {
            break;
        }
        show_disc_message(text);
    }

    if (gameart_is_loaded != 0) {
        show_disc_message("");
    } else {
        VISetBlack(1);
        VIFlush();
    }

    unpause_all_game_sounds();
    disc_error_occurred = 1;
    in_error_handler = 0;
    return 0;
}

static inline int disc_error_message(int error) {
    int index;

    if (error == 0) {
        return 0;
    }
    for (index = 0; index < sizeof(error_map) / sizeof(error_map[0]); index++) {
        if (error == error_map[index].error) {
            return error_map[index].message;
        }
    }
    return 1;
}

/* TODO: [near miss] 99.27%; message lookup and handler call match; retail keeps the error_map base in r3
 * and offset in r5 while ours uses r5/r6 (9 register rows in the loop). */
int mwfile_error_callback(int operation, int error) {
    int message;
    const char* text;

    if (operation == 0) {
        message = disc_error_message(error);
        if (message != 0) {
            switch (message) {
            case 6:
                text = get_string_ext(disc_error_string_table, 9, 1);
                break;
            case 3:
                text = get_string_ext(disc_error_string_table, 9, 2);
                break;
            case 4:
                text = get_string_ext(disc_error_string_table, 9, 3);
                break;
            case 5:
                text = get_string_ext(disc_error_string_table, 9, 4);
                break;
            case 2:
                text = get_string_ext(disc_error_string_table, 9, 5);
                break;
            default:
                text = get_string_ext(disc_error_string_table, 9, 6);
                break;
            }
            return async_error_handler(message, text);
        }
    }
    return operation;
}

void check_handle_disc_error(void) {
    int message;
    const char* text;

    switch (DVDGetDriveStatus()) {
    case 5:
        message = 6;
        break;
    case -1:
        message = 2;
        break;
    case 11:
        message = 3;
        break;
    case 4:
        message = 4;
        break;
    case 6:
        message = 5;
        break;
    default:
        return;
    }
    switch (message) {
    case 6:
        text = get_string_ext(disc_error_string_table, 9, 1);
        break;
    case 3:
        text = get_string_ext(disc_error_string_table, 9, 2);
        break;
    case 4:
        text = get_string_ext(disc_error_string_table, 9, 3);
        break;
    case 5:
        text = get_string_ext(disc_error_string_table, 9, 4);
        break;
    case 2:
        text = get_string_ext(disc_error_string_table, 9, 5);
        break;
    default:
        text = get_string_ext(disc_error_string_table, 9, 6);
        break;
    }
    async_error_handler(message, text);
}
