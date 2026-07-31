#ifndef MOVIE_CONFIG_H
#define MOVIE_CONFIG_H

#include "mw/mwMemHeap.h"
#include "mw/mwMem.h"

void mwMovLog(const char* message);
void mwMovFree(void* memory);
void* mwMovMalloc(unsigned long size);
void setMovieHeap__FP10_mwMemHeap(_mwMemHeap* heap);

#endif
