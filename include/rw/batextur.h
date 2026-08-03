#ifndef RW_BATEXTUR_H
#define RW_BATEXTUR_H

#include "rw/rwcore_types.h"

extern int textureModule;
extern char textureTKList[];

int TextureAnnihilate(RwTexture* texture);
RwTexDictionary* RwTexDictionaryGetCurrent(void);
RwTexDictionary* RwTexDictionarySetCurrent(RwTexDictionary* dictionary);
RwTexDictionary* RwTexDictionaryCreate(void);
int RwTexDictionaryDestroy(RwTexDictionary* dictionary);
RwTexture* RwTexDictionaryAddTexture(RwTexDictionary* dictionary,
                                     RwTexture* texture);
RwTexDictionary* RwTexDictionaryForAllTextures(
    RwTexDictionary* dictionary,
    RwTexture* (*callback)(RwTexture*, void*), void* data);

#endif
