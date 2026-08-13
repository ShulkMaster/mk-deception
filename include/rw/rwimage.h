#ifndef RW_RWIMAGE_H
#define RW_RWIMAGE_H

void* _rwImageOpen(void* instance, int offset, int size);
void* _rwImageClose(void* instance, int offset, int size);

#endif
