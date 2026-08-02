#include "platform/disc_error.h"

#include "libmkparticle/pfxfont.h"
#include "platform/display.h"
#include "runtime/fonts.h"

typedef int (*DiscErrorHandler)(int error, const char* text);

typedef struct DiscErrorMapEntry {
    int error;
    int message;
} DiscErrorMapEntry;

extern void OSYieldThread(void);
extern int DVDGetDriveStatus(void);
extern void VISetBlack(int black);
extern void VIFlush(void);
extern void RwCameraClear(RwCamera* camera, const unsigned int* color, int clear_mode);
extern void RwCameraBeginUpdate(RwCamera* camera);
extern void RwCameraEndUpdate(RwCamera* camera);
extern void RwCameraShowRaster(RwCamera* camera, void* device, int flags);

extern void pause_all_game_sounds(void);
extern void unpause_all_game_sounds(void);
extern void turn_all_rumble_motors_off(void);
extern void gc_movie_start(void);
extern void gc_stop_reset_watch(void);
extern void gc_grab_renderpipe(void);
extern void gc_release_renderpipe(void);
extern void gc_native_display_render_text(const char* text);
extern void handle_reset_switch(void);
extern void do_delayed_mem_frees(void);

extern int gameart_is_loaded;
extern int screen_height;
extern int screen_width;

static int fs_error_handler(int error, const char* text);

DiscErrorMapEntry error_map[30] = {
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

static DiscErrorHandler async_error_handler = fs_error_handler;
__declspec(section ".sdata") int gap_07_8050FC4C_sdata = 0;

int disc_error_occurred;
static int in_error_handler;

static __declspec(section ".sdata2") unsigned int disc_clear_color = 0xff;

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
    pfxfont_string_set(string, font, text, (float)width, 1);

    for (frame = 0; frame < 3; frame++) {
        unsigned int clear_color = disc_clear_color;

        RwCameraClear(Camera, &clear_color, 7);
        RwCameraBeginUpdate(Camera);
        pfxfont_begin_render();
        pfxfont_string_render(string, (float)left, (float)screen_height - (float)(top + height));
        pfxfont_end_render();
        RwCameraEndUpdate(Camera);
        RwCameraShowRaster(Camera, 0, 1);
    }
    pfxfont_string_cleanup(string);
    do_delayed_mem_frees();
}

static int fs_error_handler(int error, const char* text) {
    int drive_status;
    PfxFontString first_string;
    PfxFontString repeat_string;
    PfxFontString recovery_string;

    (void)error;
    if (in_error_handler != 0) {
        while (in_error_handler != 0) {
            OSYieldThread();
        }
        return 0;
    }

    in_error_handler = 1;
    drive_status = DVDGetDriveStatus();
    if (drive_status < 7) {
        if (drive_status != -1 && (drive_status < -1 || drive_status < 4)) {
            in_error_handler = 0;
            return 0;
        }
    } else if (drive_status != 11) {
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

    gc_grab_renderpipe();
    if (gameart_is_loaded != 0) {
        render_disc_message(&first_string, text);
    } else {
        gc_native_display_render_text(text);
    }
    gc_release_renderpipe();

    for (;;) {
        handle_reset_switch();
        if (drive_status != DVDGetDriveStatus()) {
            break;
        }
        gc_grab_renderpipe();
        if (gameart_is_loaded != 0) {
            render_disc_message(&repeat_string, text);
        } else {
            gc_native_display_render_text(text);
        }
        gc_release_renderpipe();
    }

    if (gameart_is_loaded != 0) {
        gc_grab_renderpipe();
        if (gameart_is_loaded != 0) {
            render_disc_message(&recovery_string, "");
        } else {
            gc_native_display_render_text("");
        }
        gc_release_renderpipe();
    } else {
        VISetBlack(1);
        VIFlush();
    }

    unpause_all_game_sounds();
    disc_error_occurred = 1;
    in_error_handler = 0;
    return 0;
}

int mwfile_error_callback(int operation, int error) {
    int message;
    const char* text;

    if (operation != 0) {
        return;
    }
    if (error == 0) {
        message = 0;
    } else {
        int index;

        message = 1;
        for (index = 0; index < 30; index++) {
            if (error == error_map[index].error) {
                message = error_map[index].message;
                break;
            }
        }
    }
    if (message == 0) {
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
