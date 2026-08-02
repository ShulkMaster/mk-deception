#ifndef MWSCREENENGINE_SCREEN_TEXT_H
#define MWSCREENENGINE_SCREEN_TEXT_H

/*
 * ScreenText -- retail mode-select labels (sizeof 0x24).
 *
 * CreateElement('TEXT') -> CreateText: resolves font (load_named_font or
 * client TGA+%s_MET cache) + resourceLib string. Draw path:
 *   BroadcastEngine / ProcessEngineEvent 0x407/0x409 (0x408 no-op in ScreenText)
 *     -> create_wrapped_string(0x900C) + pfxfont_set_string_color
 *   ScreenText::Render
 *     -> pfxfont_begin_render / set_transform / string_render
 *     -> nativefont_* (gc_font) for GX display lists
 * Soft ceiling: ProcessEngineEvent ~98.5%; Render ~93.3%.
 *
 * Pump ProcessEngineEvent 0x407 or 0x409 (0x408 is a no-op) before
 * first Render so StringObj exists; link pfxfont + gc_font.
 * TEXT prefers Midway CreateText.
 *
 * Disc blob = SEText below (same offsets as SETextElement_t). string0/string1
 * are SeRef on disc; after PatchText they are live char* (ILP32).
 * Do not store widened non-ILP32 pointers inside the blob.
 */

#include "libmkparticle/pfxfont.h"

typedef struct StringObj StringObj;
typedef struct ScreenText ScreenText;

/* C view matching SETextElement_t (ScreenObject.h). */
typedef struct SEText {
    unsigned int typeTag; /* +0x00 'TEXT' */
    int pad04; /* +0x04 */
    ScreenText* liveObject; /* +0x08 -- after instancing */
    unsigned int unk0c; /* +0x0C */
    char* string0; /* +0x10 -- after Patch */
    char* string1; /* +0x14 -- font name after Patch */
    float posX; /* +0x18 */
    float posY; /* +0x1C -- screen Y via (480 - posY) */
    float unk20; /* +0x20 */
    float wrapW; /* +0x24 */
    float yOff; /* +0x28 */
    unsigned char color[4]; /* +0x2C */
    unsigned char halign; /* +0x30 */
    unsigned char valign; /* +0x31 */
} SEText;

struct ScreenText {
    void* vtbl; /* +0x00 */
    unsigned int flags; /* +0x04 */
    unsigned char pad08[8]; /* +0x08 */
    SEText* seData; /* +0x10 */
    StringObj* stringObj; /* +0x14 */
    int stringObjInstance; /* +0x18 */
    PfxFontSlot* font; /* +0x1C */
    char* string; /* +0x20 */
}; /* 0x24 */

enum {
    kScreenTextStringOid = 0x900C,
    kScreenTextEvtRebuildA = 0x407,
    kScreenTextEvtNoOp = 0x408, /* cascade hole -- ScreenText returns */
    kScreenTextEvtRebuildB = 0x409
};

#endif
