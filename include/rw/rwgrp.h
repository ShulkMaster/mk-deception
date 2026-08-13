#ifndef RW_RWGRP_H
#define RW_RWGRP_H

void* _rwChunkGroupOpen(void* instance, int offset, int size);
void* _rwChunkGroupClose(void* instance, int offset, int size);

#endif
