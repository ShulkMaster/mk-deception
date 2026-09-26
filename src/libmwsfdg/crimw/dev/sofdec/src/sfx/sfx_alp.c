#include "dolphin/types.h"
#include "runtime/cstring.h"

typedef float CFTArgbTable[3][256][4];

typedef struct SFXAObject {
    s32 active;
    s32 need_luminance_update;
    s32 reverse_luminance;
    s32 luminance_low;
    s32 luminance_high;
    u8 alpha0;
    u8 alpha1;
    u8 alpha2;
    u8 reserved_17;
} SFXAObject;

typedef struct SFXAWork {
    s32 active_count;
    s32 capacity;
    SFXAObject objects[8];
} SFXAWork;

extern void CFT_MakeArgb8888Alp3211Tbl(CFTArgbTable table, u8 alpha0,
                                       u8 alpha1, u8 alpha2);
extern void CFT_MakeArgb8888Alp3110Tbl(CFTArgbTable table, u8 alpha0,
                                       u8 alpha1, u8 alpha2);
extern void CFT_MakeArgb8888AlpLumiTbl(s32 reverse, s32 low, s32 high,
                                       CFTArgbTable table);

SFXAWork sfxa_work;

s32 SFXA_IsNeedUpdateLumiTbl(SFXAObject* object) {
    return object->need_luminance_update;
}

void SFXA_MakeAlp3211Tbl(SFXAObject* object, void* unused,
                         CFTArgbTable table) {
    CFT_MakeArgb8888Alp3211Tbl(table, object->alpha0, object->alpha1,
                               object->alpha2);
}

void SFXA_MakeAlp3110Tbl(SFXAObject* object, void* unused,
                         CFTArgbTable table) {
    CFT_MakeArgb8888Alp3110Tbl(table, object->alpha0, object->alpha1,
                               object->alpha2);
}

void SFXA_MakeAlpLumiTbl(SFXAObject* object, void* unused,
                         CFTArgbTable table) {
    CFT_MakeArgb8888AlpLumiTbl(object->reverse_luminance,
                               object->luminance_low,
                               object->luminance_high, table);
    object->need_luminance_update = 0;
}

void SFXA_Destroy(SFXAObject* object) {
    if (object != 0) {
        object->active = 0;
        sfxa_work.active_count--;
    }
}

static inline SFXAObject* SFXA_FindFreeObject(void) {
    SFXAObject* object = sfxa_work.objects;
    s32 remaining = sfxa_work.capacity;

    while (remaining > 0) {
        if (object->active == 0) {
            return object;
        }
        object++;
        remaining--;
    }
    return 0;
}

/* TODO: [near miss] 84.64%; CFG/stores exact; li 0xFF must schedule after stw 100
 * (shared r0); RE4's only match seeds that via register + inline asm, so stop. */
SFXAObject* SFXA_Create(void) {
    SFXAObject* object = SFXA_FindFreeObject();
    if (object == 0) {
        return object;
    }

    object->reverse_luminance = 0;
    object->luminance_low = 31;
    object->luminance_high = 100;
    object->need_luminance_update = 1;
    object->alpha0 = 0;
    object->alpha1 = 127;
    object->alpha2 = 255;
    sfxa_work.active_count++;
    object->active = 1;
    return object;
}

void SFXA_Finish(void) {
}

void SFXA_Init(void) {
    memset(&sfxa_work, 0, sizeof(sfxa_work));
    sfxa_work.capacity = 8;
}
