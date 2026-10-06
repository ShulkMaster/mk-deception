#ifndef DOLPHIN_VM_H
#define DOLPHIN_VM_H

#include "dolphin/types.h"

#ifdef __cplusplus
extern "C" {
#endif

void VMInit(unsigned long virtual_memory_size, unsigned long aram_base,
            unsigned long aram_size);
void VMQuit(void);
int VMAlloc(void* virtual_address, unsigned long size);

void VMBASEInit(void (*swap_page_callback)(u32));
void VMBASEQuit(void);
void VMBASESetPageTableEntry(u32 virtual_address, void* physical_address,
                             u32 physical_page);
void VMBASEClearPageTableEntry(u32 virtual_address, u32 physical_page);
BOOL VMBASEIsPageValid(u32 virtual_address);
BOOL VMBASEIsPageReferenced(u32 virtual_address);
BOOL VMBASEIsPageDirty(u32 virtual_address);
void VMBASESetPageReferenced(u32 virtual_address, BOOL referenced);
u32 VMBASEGetVirtualAddrFromPageInMRAM(u32 physical_page);
BOOL VMBASEIsPageLocked(u32 physical_page);
/* TODO: [Scope warn] pointer return follows the blocked stub; recover its void retail ABI with the callee. */
void* __VMBASERestoreExceptionHandlers(void);
void __VMBASERestoreVMRegisters(void);
void __VMBASEInvalidateLockedPageTable(void);

u32 __VMGetNumPagesInMRAM(void);
u32 VMGetARAMSize(void);
u32 VMGetARAMBase(void);
void __VMAllocVirtualToARAMLUT(void);
void __VMAllocARAMToVirtualLUT(void);
void __VMSetARAMPageAsDirty(u32 virtual_address);
BOOL __VMIsARAMPageDirty(u32 virtual_address);
u32 __VMTranslateVMPageToARAMPage(u32 virtual_address);
BOOL __VMDoesMappingExist(u32 virtual_address);
void __VMMappingErrorAlert(u32 virtual_address);
void VMSetPageReplacementPolicy(int policy);
void __VMSetNextPageToSwap(u32 page);
void __VMSetFreePagesExist(BOOL free_pages_exist);
u32 __VMGetPageToReplace(void);
void __VMAllocMRAMSwapSpace(void);

#ifdef __cplusplus
}
#endif

#endif
