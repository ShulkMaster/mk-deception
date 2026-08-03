#ifndef MOVIE_CONFIG_H
#define MOVIE_CONFIG_H

#include "mw/mwMemHeap.h"
#include "mw/mwMem.h"

#ifdef __cplusplus
extern "C" {
#endif

void mwMovLog(const char* message);
void mwMovFree(void* memory);
void* mwMovMalloc(unsigned long size);
void setMovieHeap__FP10_mwMemHeap(_mwMemHeap* heap);

#ifdef __cplusplus
}
#endif

#endif
