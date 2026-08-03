#include "movie/MovieConfig.h"

_mwMemHeap* MovieHeap;

void mwMovLog(const char* message) {
    (void)message;
}

void mwMovFree(void* memory) {
    _mwMemFree(memory, 0, 0);
}

void* mwMovMalloc(unsigned long size) {
    return _mwMemMalloc(movie_heap, size, 5, 0, 0, 0);
}

void setMovieHeap(_mwMemHeap* heap) {
    MovieHeap = heap;
}
