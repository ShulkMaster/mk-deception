#ifndef CRI_SJ_H
#define CRI_SJ_H

typedef struct SJCK {
    unsigned char* data;
    int len;
} SJCK;

unsigned char* SJ_SearchTag(
    const SJCK* source, const char* tag, const char* terminator, SJCK* result);
void SJ_SplitChunk(const SJCK* source, int nbyte, SJCK* first, SJCK* remainder);

#endif
