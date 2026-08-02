#ifndef RUNTIME_SECTION_SLOT_FILE_H
#define RUNTIME_SECTION_SLOT_FILE_H

#include "runtime/section_types.h"

int sec_slot_file_open_read_async(SecSlotFileEntry* file, SecSlot* slot,
                                  int field_0x1C, MkFileInfo* info,
                                  void* userdata);
int sec_slot_file_open_read_async_queued(SecSlotFileEntry* file, SecSlot* slot,
                                         int field_0x1C, MkFileInfo* info,
                                         void* userdata);
void sec_slot_file_wait_for_load(SecSlotFileEntry* file);
void sec_slot_file_cancel_async(SecSlotFileEntry* file);
void sec_slot_file_free_async(SecSlotFileEntry* file);
void sec_slot_file_close_file(SecSlotFileEntry* file);
void sec_slot_file_wait_on_ssf(MkFileEntry* ssf_file);
void init_sec_slot_files(void);

#endif
