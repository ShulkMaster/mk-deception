#include "dolphin/os.h"
#include "dolphin/types.h"

u32 __VMGetNumPagesInMRAM(void);
u32 VMBASEGetVirtualAddrFromPageInMRAM(u32 physical_page);
int VMBASEIsPageValid(u32 virtual_address);
int VMBASEIsPageReferenced(u32 virtual_address);
int VMBASEIsPageDirty(u32 virtual_address);
void VMBASESetPageReferenced(u32 virtual_address, int referenced);
int VMBASEIsPageLocked(u32 physical_page);

u32 __VMPageReplacementLRU(void);
u32 __VMPageReplacementRandom(void);
u32 __VMPageReplacementFIFO(void);

static int g_vmFreePagesExist = 1;
static int g_vmPageReplacementPolicy = 1;
static u32 g_vmNextPageToSwap;

void VMSetPageReplacementPolicy(int policy)
{
    g_vmPageReplacementPolicy = policy;
}

void __VMSetNextPageToSwap(u32 page)
{
    g_vmNextPageToSwap = page;
}

void __VMSetFreePagesExist(int free_pages_exist)
{
    g_vmFreePagesExist = free_pages_exist;
}

u32 __VMGetPageToReplace(void)
{
    if (g_vmPageReplacementPolicy == 0) {
        return __VMPageReplacementLRU();
    }
    if (g_vmPageReplacementPolicy == 1) {
        return __VMPageReplacementRandom();
    }
    return __VMPageReplacementFIFO();
}

u32 __VMPageReplacementLRU(void)
{
    u32 replacement_page = 0;
    u32 first_page = g_vmNextPageToSwap;
    int unreferenced_dirty_page = -1;
    int referenced_clean_page = -1;
    int referenced_dirty_page = -1;

    if (!g_vmFreePagesExist) {
        for (;;) {
            u32 virtual_address =
                VMBASEGetVirtualAddrFromPageInMRAM(g_vmNextPageToSwap);

            if (virtual_address != 0 && VMBASEIsPageValid(virtual_address)) {
                int referenced = VMBASEIsPageReferenced(virtual_address);
                int dirty = VMBASEIsPageDirty(virtual_address);

                if (!referenced && !dirty &&
                    !VMBASEIsPageLocked(g_vmNextPageToSwap)) {
                    replacement_page = g_vmNextPageToSwap;
                    break;
                }

                if (!referenced && dirty) {
                    if (unreferenced_dirty_page < 0 &&
                        !VMBASEIsPageLocked(g_vmNextPageToSwap)) {
                        unreferenced_dirty_page = g_vmNextPageToSwap;
                    }
                } else if (referenced && !dirty) {
                    if (referenced_clean_page < 0 &&
                        !VMBASEIsPageLocked(g_vmNextPageToSwap)) {
                        referenced_clean_page = g_vmNextPageToSwap;
                    }
                } else if (referenced_dirty_page < 0 &&
                           !VMBASEIsPageLocked(g_vmNextPageToSwap)) {
                    referenced_dirty_page = g_vmNextPageToSwap;
                }

                if (referenced) {
                    VMBASESetPageReferenced(virtual_address, 0);
                }

                if (first_page == g_vmNextPageToSwap) {
                    if (unreferenced_dirty_page >= 0) {
                        replacement_page = unreferenced_dirty_page;
                    } else if (referenced_clean_page >= 0) {
                        replacement_page = referenced_clean_page;
                    } else if (referenced_dirty_page >= 0) {
                        replacement_page = referenced_dirty_page;
                    }
                    break;
                }

                g_vmNextPageToSwap++;
                if (g_vmNextPageToSwap >= __VMGetNumPagesInMRAM()) {
                    g_vmNextPageToSwap = 0;
                }
            } else {
                replacement_page = g_vmNextPageToSwap;
                break;
            }
        }
    } else {
        replacement_page = first_page;
    }

    g_vmNextPageToSwap++;
    if (g_vmNextPageToSwap >= __VMGetNumPagesInMRAM()) {
        g_vmFreePagesExist = 0;
        g_vmNextPageToSwap = 0;
    }
    return replacement_page;
}

u32 __VMPageReplacementRandom(void)
{
    u32 replacement_page;

    do {
        if (g_vmFreePagesExist) {
            replacement_page = g_vmNextPageToSwap++;
            if (g_vmNextPageToSwap >= __VMGetNumPagesInMRAM()) {
                g_vmFreePagesExist = 0;
                g_vmNextPageToSwap = 0;
            }
        } else {
            replacement_page = OSGetTick() % __VMGetNumPagesInMRAM();
        }
    } while (VMBASEIsPageLocked(replacement_page));

    return replacement_page;
}

u32 __VMPageReplacementFIFO(void)
{
    u32 replacement_page;

    do {
        replacement_page = g_vmNextPageToSwap++;
        if (g_vmNextPageToSwap >= __VMGetNumPagesInMRAM()) {
            g_vmFreePagesExist = 0;
            g_vmNextPageToSwap = 0;
        }
    } while (VMBASEIsPageLocked(replacement_page));

    return replacement_page;
}
