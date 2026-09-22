#include "dolphin/ar.h"
#include "dolphin/types.h"
#include "runtime/cstring.h"

typedef struct RNAResource {
    int used;
    u32 buffer;
    u32 size;
} RNAResource;

void RNAERR_CallErrFunc(const char* message);

u32 rnares_init_cnt = 0;
u32 rnares_setup_fg = 0;
u32 rnares_nbuf = 0;
u32 rnares_aram_size = 0;
u32 rnares_aram_ptr = 0;
RNAResource rnares_obj[32];

u32 RNARES_GetBufSize(RNAResource* resource)
{
    return resource == 0 ? 0 : resource->size;
}

u32 RNARES_GetBuf(RNAResource* resource)
{
    return resource == 0 ? 0 : resource->buffer;
}

void RNARES_Destroy(RNAResource* resource)
{
    if (resource != 0) {
        resource->used = 0;
    }
}

RNAResource* RNARES_Create(void)
{
    int i;
    RNAResource* resource;

    for (i = 0; i < 32; i++) {
        if (rnares_obj[i].used == 0) {
            break;
        }
    }
    if (i == 32) {
        RNAERR_CallErrFunc("E1070313:Not enough RNARES handle.\n");
        return 0;
    }
    resource = &rnares_obj[i];
    resource->used = 1;
    return resource;
}

void RNARES_Finish(void)
{
    int i;
    u32 freed;

    rnares_init_cnt--;
    if (rnares_init_cnt == 0) {
        for (i = 0; i < 32; i++) {
            if (rnares_obj[i].used == 1) {
                RNARES_Destroy(&rnares_obj[i]);
            }
        }
        memset(rnares_obj, 0, sizeof(rnares_obj));
        if (rnares_setup_fg == 0) {
            ARFree(&freed);
            if (freed != rnares_aram_size) {
                RNAERR_CallErrFunc(
                    "E1090601:Free area other than ADX buffer.\n");
            }
            rnares_nbuf = 0;
            rnares_aram_size = 0;
            rnares_aram_ptr = 0;
        }
    }
}

/* TODO: [near miss] 96.489365%; pool construction matches, with only register coloring left in the unrolled fill. */
void RNARES_Init(void)
{
    u32 i;
    u32 offset;
    u32 nbuf;
    RNAResource* resource;

    if (rnares_init_cnt == 0) {
        if (rnares_setup_fg == 0) {
            rnares_nbuf = 32;
            rnares_aram_size = 0x40000;
            rnares_aram_ptr = ARAlloc(rnares_aram_size);
        }
        memset(rnares_obj, 0, sizeof(rnares_obj));
        nbuf = rnares_nbuf;
        resource = rnares_obj;
        offset = 0;
        for (i = 0; i < nbuf; i++, resource++) {
            resource->buffer = (rnares_aram_ptr + offset) >> 1;
            resource->size = 0x1000;
            offset += 0x2000;
        }
    }
    rnares_init_cnt++;
}
