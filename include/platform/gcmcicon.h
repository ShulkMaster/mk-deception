#ifndef PLATFORM_GCMCICON_H
#define PLATFORM_GCMCICON_H

#include "platform/gcmcard.h"

extern char* mc_data_buffer;
extern int mc_data_buffer_size;
extern unsigned int mc_icon_file_size;

void unload_memorycard_write_buffer(void);
int create_memorycard_write_buffer(const void* data, unsigned int size);
void load_icon_data(void);
int update_memory_card_status(const CARDFileInfo* file);

#endif
