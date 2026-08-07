#ifndef RW_BATEXTUR_H
#define RW_BATEXTUR_H

#include "rw/rwcore_types.h"

int TextureAnnihilate(RwTexture* texture);
RwTexDictionary* RwTexDictionaryGetCurrent(void);
void RwTexDictionarySetCurrent(RwTexDictionary* dictionary);
RwTexDictionary* RwTexDictionaryCreate(void);
int RwTexDictionaryDestroy(RwTexDictionary* dictionary);
RwTexture* RwTexDictionaryAddTexture(RwTexDictionary* dictionary,
                                     RwTexture* texture);
RwTexDictionary* RwTexDictionaryForAllTextures(
    RwTexDictionary* dictionary,
    RwTexture* (*callback)(RwTexture*, void*), void* data);

#endif
