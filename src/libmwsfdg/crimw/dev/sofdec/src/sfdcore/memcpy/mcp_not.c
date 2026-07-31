extern void* memcpy(void* destination, const void* source, unsigned long size);

void* MEM_Copy(void* destination, const void* source, unsigned long size) {
    return memcpy(destination, source, size);
}
