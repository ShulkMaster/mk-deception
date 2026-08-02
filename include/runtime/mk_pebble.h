#ifndef MK_PEBBLE_H
#define MK_PEBBLE_H

#include "runtime/mk_obj.h"
#include "rw/rtquat.h"

typedef RpAtomic* (*PebbleRenderCallback)(RpAtomic* atomic);

typedef struct PebbleRenderData {
    void* field_00;
    PebbleRenderCallback callback;
} PebbleRenderData;

typedef union PebbleFlags {
    unsigned int word;
    struct {
        signed char visible : 1;
        unsigned char partly_visible : 1;
        unsigned char pad_high : 6;
        unsigned char pad[3];
    } bits;
} PebbleFlags;

typedef struct Pebble {
    RwMatrix matrix;
} Pebble;

typedef struct PebbleData {
    MkHdr hdr;                    /* +0x00 */
    Pebble* pebbles;              /* +0x08 */
    int count;                    /* +0x0C */
    int active_count;             /* +0x10 */
    PebbleRenderData* render_data;/* +0x14 */
    void* user_data;              /* +0x18 */
    PebbleFlags* flags;           /* +0x1C */
} PebbleData;

PebbleData* create_pebble_userdata(MkSobj* sobj, int count, int user_data_size);
int vdestroy_pebble(PebbleData* pebble_data);

#endif
