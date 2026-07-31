#ifndef LIBMKPARTICLE_RW_ENGINE_H
#define LIBMKPARTICLE_RW_ENGINE_H

/** Partial particle-facing RenderWare engine dispatch table. */
typedef struct PfxRwEngineInstance {
    char pad00[0x20]; /**< Retail offsets 0x00-0x1F; callbacks unknown. */
    int (*fpRenderStateSet)(int state, int value); /**< Retail offset 0x20. */
    void (*fpRenderStateGet)(int state, void* out); /**< Retail offset 0x24. */
} PfxRwEngineInstance;

extern PfxRwEngineInstance* RwEngineInstance;

#endif
