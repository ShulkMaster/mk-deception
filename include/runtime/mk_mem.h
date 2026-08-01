#ifndef RUNTIME_MK_MEM_H
#define RUNTIME_MK_MEM_H

void purge_delayed_mem_frees(void);
void do_delayed_mem_frees(void);
void free_mem_delayed(void *memory, int delay);
void *get_mem(unsigned int size);
void free_mem(void *memory);
void reset_wave_mem(void);

#endif
